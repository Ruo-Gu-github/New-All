#include "pch.h"
#include "MaskManager.h"
#include <algorithm>
#include <queue>
#include <stack>
#include <cmath>
#include <cstring>
#include <tuple>

// 取消Windows.h定义的min/max宏
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

// ==================== MaskData方法实现 ====================

void MaskData::UpdateStatistics() {
    voxelCount = 0;
    double sumX = 0, sumY = 0, sumZ = 0;
    
    for (int z = 0; z < depth; z++) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (Get(x, y, z) > 0) {
                    voxelCount++;
                    sumX += x;
                    sumY += y;
                    sumZ += z;
                }
            }
        }
    }
    
    if (voxelCount > 0) {
        centerX = static_cast<float>(sumX / voxelCount);
        centerY = static_cast<float>(sumY / voxelCount);
        centerZ = static_cast<float>(sumZ / voxelCount);
    } else {
        centerX = centerY = centerZ = 0.0f;
    }
}

// ==================== MaskManager实现 ====================

bool MaskManager::IsValidIndex(int index) const {
    return index >= 0 && index < static_cast<int>(masks_.size());
}

std::shared_ptr<MaskData> MaskManager::CreateMaskCopy(const MaskData& source) {
    auto copy = std::make_shared<MaskData>(source.width, source.height, source.depth);
    copy->data = source.data;
    copy->name = source.name;
    std::memcpy(copy->color, source.color, sizeof(copy->color));
    copy->minThreshold = source.minThreshold;
    copy->maxThreshold = source.maxThreshold;
    copy->visible = source.visible;
    copy->opacity = source.opacity;
    copy->voxelCount = source.voxelCount;
    copy->centerX = source.centerX;
    copy->centerY = source.centerY;
    copy->centerZ = source.centerZ;
    return copy;
}

// ==================== Mask创建 ====================

int MaskManager::CreateFromThreshold(const uint16_t* volumeData, int width, int height, int depth,
                                    float minThreshold, float maxThreshold,
                                    const char* name) {
    if (!volumeData) return -1;
    
    auto mask = std::make_shared<MaskData>(width, height, depth);
    mask->name = name;
    mask->minThreshold = minThreshold;
    mask->maxThreshold = maxThreshold;
    
    // 设置随机颜色（基于索引）
    int idx = static_cast<int>(masks_.size());
    float hue = (idx * 137.5f); // 黄金角
    while (hue >= 360.0f) hue -= 360.0f;
    // 简单HSV到RGB转换（饱和度=1，亮度=1）
    float c = 1.0f;
    float x = c * (1.0f - std::abs(std::fmod(hue / 60.0f, 2.0f) - 1.0f));
    if (hue < 60) { mask->color[0] = c; mask->color[1] = x; mask->color[2] = 0; }
    else if (hue < 120) { mask->color[0] = x; mask->color[1] = c; mask->color[2] = 0; }
    else if (hue < 180) { mask->color[0] = 0; mask->color[1] = c; mask->color[2] = x; }
    else if (hue < 240) { mask->color[0] = 0; mask->color[1] = x; mask->color[2] = c; }
    else if (hue < 300) { mask->color[0] = x; mask->color[1] = 0; mask->color[2] = c; }
    else { mask->color[0] = c; mask->color[1] = 0; mask->color[2] = x; }
    mask->color[3] = 0.5f;  // Alpha
    
    // 应用阈值
    int totalVoxels = width * height * depth;
    for (int i = 0; i < totalVoxels; i++) {
        uint16_t value = volumeData[i];
        if (value >= minThreshold && value <= maxThreshold) {
            mask->data[i] = 255;
        }
    }
    
    mask->UpdateStatistics();
    
    masks_.push_back(mask);
    return static_cast<int>(masks_.size()) - 1;
}

int MaskManager::CreateEmpty(int width, int height, int depth, const char* name) {
    auto mask = std::make_shared<MaskData>(width, height, depth);
    mask->name = name;
    masks_.push_back(mask);
    return static_cast<int>(masks_.size()) - 1;
}

int MaskManager::Clone(int maskIndex) {
    if (!IsValidIndex(maskIndex)) return -1;
    
    auto copy = CreateMaskCopy(*masks_[maskIndex]);
    copy->name += " (Copy)";
    masks_.push_back(copy);
    return static_cast<int>(masks_.size()) - 1;
}

// ==================== Mask管理 ====================

MaskData* MaskManager::GetMask(int index) {
    if (!IsValidIndex(index)) return nullptr;
    return masks_[index].get();
}

const MaskData* MaskManager::GetMask(int index) const {
    if (!IsValidIndex(index)) return nullptr;
    return masks_[index].get();
}

bool MaskManager::DeleteMask(int index) {
    if (!IsValidIndex(index)) return false;
    masks_.erase(masks_.begin() + index);
    return true;
}

void MaskManager::Clear() {
    masks_.clear();
}

void MaskManager::SetMaskName(int index, const char* name) {
    if (IsValidIndex(index) && name) {
        masks_[index]->name = name;
    }
}

void MaskManager::SetMaskColor(int index, float r, float g, float b, float a) {
    if (!IsValidIndex(index)) return;
    masks_[index]->color[0] = r;
    masks_[index]->color[1] = g;
    masks_[index]->color[2] = b;
    masks_[index]->color[3] = a;
}

void MaskManager::SetMaskVisible(int index, bool visible) {
    if (IsValidIndex(index)) {
        masks_[index]->visible = visible;
    }
}

void MaskManager::SetMaskOpacity(int index, float opacity) {
    if (IsValidIndex(index)) {
        masks_[index]->opacity = std::max(0.0f, std::min(1.0f, opacity));
    }
}

// ==================== 布尔运算 ====================

int MaskManager::Union(int indexA, int indexB, const char* name) {
    if (!IsValidIndex(indexA) || !IsValidIndex(indexB)) return -1;
    
    auto maskA = masks_[indexA];
    auto maskB = masks_[indexB];
    
    if (maskA->width != maskB->width || maskA->height != maskB->height || 
        maskA->depth != maskB->depth) {
        return -1; // 尺寸不匹配
    }
    
    auto result = std::make_shared<MaskData>(maskA->width, maskA->height, maskA->depth);
    result->name = name;
    
    int totalVoxels = maskA->width * maskA->height * maskA->depth;
    for (int i = 0; i < totalVoxels; i++) {
        result->data[i] = (maskA->data[i] > 0 || maskB->data[i] > 0) ? 255 : 0;
    }
    
    result->UpdateStatistics();
    masks_.push_back(result);
    return static_cast<int>(masks_.size()) - 1;
}

int MaskManager::Intersection(int indexA, int indexB, const char* name) {
    if (!IsValidIndex(indexA) || !IsValidIndex(indexB)) return -1;
    
    auto maskA = masks_[indexA];
    auto maskB = masks_[indexB];
    
    if (maskA->width != maskB->width || maskA->height != maskB->height || 
        maskA->depth != maskB->depth) {
        return -1;
    }
    
    auto result = std::make_shared<MaskData>(maskA->width, maskA->height, maskA->depth);
    result->name = name;
    
    int totalVoxels = maskA->width * maskA->height * maskA->depth;
    for (int i = 0; i < totalVoxels; i++) {
        result->data[i] = (maskA->data[i] > 0 && maskB->data[i] > 0) ? 255 : 0;
    }
    
    result->UpdateStatistics();
    masks_.push_back(result);
    return static_cast<int>(masks_.size()) - 1;
}

int MaskManager::Difference(int indexA, int indexB, const char* name) {
    if (!IsValidIndex(indexA) || !IsValidIndex(indexB)) return -1;
    
    auto maskA = masks_[indexA];
    auto maskB = masks_[indexB];
    
    if (maskA->width != maskB->width || maskA->height != maskB->height || 
        maskA->depth != maskB->depth) {
        return -1;
    }
    
    auto result = std::make_shared<MaskData>(maskA->width, maskA->height, maskA->depth);
    result->name = name;
    
    int totalVoxels = maskA->width * maskA->height * maskA->depth;
    for (int i = 0; i < totalVoxels; i++) {
        result->data[i] = (maskA->data[i] > 0 && maskB->data[i] == 0) ? 255 : 0;
    }
    
    result->UpdateStatistics();
    masks_.push_back(result);
    return static_cast<int>(masks_.size()) - 1;
}

int MaskManager::XOR(int indexA, int indexB, const char* name) {
    if (!IsValidIndex(indexA) || !IsValidIndex(indexB)) return -1;
    
    auto maskA = masks_[indexA];
    auto maskB = masks_[indexB];
    
    if (maskA->width != maskB->width || maskA->height != maskB->height || 
        maskA->depth != maskB->depth) {
        return -1;
    }
    
    auto result = std::make_shared<MaskData>(maskA->width, maskA->height, maskA->depth);
    result->name = name;
    
    int totalVoxels = maskA->width * maskA->height * maskA->depth;
    for (int i = 0; i < totalVoxels; i++) {
        bool a = maskA->data[i] > 0;
        bool b = maskB->data[i] > 0;
        result->data[i] = (a != b) ? 255 : 0;
    }
    
    result->UpdateStatistics();
    masks_.push_back(result);
    return static_cast<int>(masks_.size()) - 1;
}

int MaskManager::Invert(int index, const char* name) {
    if (!IsValidIndex(index)) return -1;
    
    auto mask = masks_[index];
    auto result = std::make_shared<MaskData>(mask->width, mask->height, mask->depth);
    result->name = name;
    
    int totalVoxels = mask->width * mask->height * mask->depth;
    for (int i = 0; i < totalVoxels; i++) {
        result->data[i] = (mask->data[i] > 0) ? 0 : 255;
    }
    
    result->UpdateStatistics();
    masks_.push_back(result);
    return static_cast<int>(masks_.size()) - 1;
}

// ==================== 形态学操作 ====================
// (实现将在下一个文件中继续...)

// ==================== ROI绘制 - 2D画笔工具 ====================

void MaskManager::DrawCircle2D(int index, int sliceDir, int sliceIndex, float cx, float cy, float radius) {
    if (!IsValidIndex(index)) return;
    
    auto mask = masks_[index];
    
    // 计算圆形边界框（2D）
    int minX = std::max(0, static_cast<int>(cx - radius - 1));
    int maxX = std::min(static_cast<int>(cx + radius + 1), static_cast<int>(mask->width - 1));
    int minY = std::max(0, static_cast<int>(cy - radius - 1));
    int maxY = std::min(static_cast<int>(cy + radius + 1), static_cast<int>(mask->height - 1));
    
    float radiusSq = radius * radius;
    
    // 根据切片方向绘制
    if (sliceDir == 0) {  // Axial (XY平面，固定Z)
        int z = sliceIndex;
        if (z < 0 || z >= mask->depth) return;
        
        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                float dx = x - cx;
                float dy = y - cy;
                float distSq = dx * dx + dy * dy;
                
                if (distSq <= radiusSq) {
                    mask->Set(x, y, z, 255);
                }
            }
        }
    }
    else if (sliceDir == 1) {  // Coronal (XZ平面，固定Y)
        int y = sliceIndex;
        if (y < 0 || y >= mask->height) return;
        
        for (int z = minY; z <= maxY; z++) {  // cy对应Z
            for (int x = minX; x <= maxX; x++) {  // cx对应X
                float dx = x - cx;
                float dz = z - cy;
                float distSq = dx * dx + dz * dz;
                
                if (distSq <= radiusSq) {
                    mask->Set(x, y, z, 255);
                }
            }
        }
    }
    else if (sliceDir == 2) {  // Sagittal (YZ平面，固定X)
        int x = sliceIndex;
        if (x < 0 || x >= mask->width) return;
        
        for (int z = minY; z <= maxY; z++) {  // cy对应Z
            for (int y = minX; y <= maxX; y++) {  // cx对应Y
                float dy = y - cx;
                float dz = z - cy;
                float distSq = dy * dy + dz * dz;
                
                if (distSq <= radiusSq) {
                    mask->Set(x, y, z, 255);
                }
            }
        }
    }
    
    // 更新统计信息
    mask->UpdateStatistics();
}

void MaskManager::EraseCircle2D(int index, int sliceDir, int sliceIndex, float cx, float cy, float radius) {
    if (!IsValidIndex(index)) return;
    
    auto mask = masks_[index];
    
    // 计算圆形边界框（2D）
    int minX = std::max(0, static_cast<int>(cx - radius - 1));
    int maxX = std::min(static_cast<int>(cx + radius + 1), static_cast<int>(mask->width - 1));
    int minY = std::max(0, static_cast<int>(cy - radius - 1));
    int maxY = std::min(static_cast<int>(cy + radius + 1), static_cast<int>(mask->height - 1));
    
    float radiusSq = radius * radius;
    
    // 根据切片方向擦除
    if (sliceDir == 0) {  // Axial (XY平面，固定Z)
        int z = sliceIndex;
        if (z < 0 || z >= mask->depth) return;
        
        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                float dx = x - cx;
                float dy = y - cy;
                float distSq = dx * dx + dy * dy;
                
                if (distSq <= radiusSq) {
                    mask->Set(x, y, z, 0);
                }
            }
        }
    }
    else if (sliceDir == 1) {  // Coronal (XZ平面，固定Y)
        int y = sliceIndex;
        if (y < 0 || y >= mask->height) return;
        
        for (int z = minY; z <= maxY; z++) {
            for (int x = minX; x <= maxX; x++) {
                float dx = x - cx;
                float dz = z - cy;
                float distSq = dx * dx + dz * dz;
                
                if (distSq <= radiusSq) {
                    mask->Set(x, y, z, 0);
                }
            }
        }
    }
    else if (sliceDir == 2) {  // Sagittal (YZ平面，固定X)
        int x = sliceIndex;
        if (x < 0 || x >= mask->width) return;
        
        for (int z = minY; z <= maxY; z++) {
            for (int y = minX; y <= maxX; y++) {
                float dy = y - cx;
                float dz = z - cy;
                float distSq = dy * dy + dz * dz;
                
                if (distSq <= radiusSq) {
                    mask->Set(x, y, z, 0);
                }
            }
        }
    }
    
    // 更新统计信息
    mask->UpdateStatistics();
}

// ==================== ROI绘制 - 3D画笔工具（保留用于其他功能）====================

void MaskManager::DrawBrushSphere3D(int index, float cx, float cy, float cz, float radius) {
    if (!IsValidIndex(index)) return;
    
    auto mask = masks_[index];
    
    // 计算球形边界框（避免遍历整个体积）
    int minX = std::max(0, static_cast<int>(cx - radius - 1));
    int maxX = std::min(mask->width - 1, static_cast<int>(cx + radius + 1));
    int minY = std::max(0, static_cast<int>(cy - radius - 1));
    int maxY = std::min(mask->height - 1, static_cast<int>(cy + radius + 1));
    int minZ = std::max(0, static_cast<int>(cz - radius - 1));
    int maxZ = std::min(mask->depth - 1, static_cast<int>(cz + radius + 1));
    
    float radiusSq = radius * radius;
    
    // 遍历球形区域内的所有体素
    for (int z = minZ; z <= maxZ; z++) {
        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                // 计算到球心的距离平方
                float dx = x - cx;
                float dy = y - cy;
                float dz = z - cz;
                float distSq = dx * dx + dy * dy + dz * dz;
                
                // 如果在球内，设置为255
                if (distSq <= radiusSq) {
                    mask->Set(x, y, z, 255);
                }
            }
        }
    }
    
    // 更新统计信息
    mask->UpdateStatistics();
}

void MaskManager::EraseSphere3D(int index, float cx, float cy, float cz, float radius) {
    if (!IsValidIndex(index)) return;
    
    auto mask = masks_[index];
    
    // 计算球形边界框
    int minX = std::max(0, static_cast<int>(cx - radius - 1));
    int maxX = std::min(mask->width - 1, static_cast<int>(cx + radius + 1));
    int minY = std::max(0, static_cast<int>(cy - radius - 1));
    int maxY = std::min(mask->height - 1, static_cast<int>(cy + radius + 1));
    int minZ = std::max(0, static_cast<int>(cz - radius - 1));
    int maxZ = std::min(mask->depth - 1, static_cast<int>(cz + radius + 1));
    
    float radiusSq = radius * radius;
    
    // 遍历球形区域内的所有体素
    for (int z = minZ; z <= maxZ; z++) {
        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                // 计算到球心的距离平方
                float dx = x - cx;
                float dy = y - cy;
                float dz = z - cz;
                float distSq = dx * dx + dy * dy + dz * dz;
                
                // 如果在球内，设置为0
                if (distSq <= radiusSq) {
                    mask->Set(x, y, z, 0);
                }
            }
        }
    }
    
    // 更新统计信息
    mask->UpdateStatistics();
}
