#include "pch.h"
#include "MaskManager.h"
#include <algorithm>
#include <queue>
#include <set>
#include <tuple>

// 取消Windows.h定义的min/max宏
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

// ==================== 形态学操作实现 ====================

int MaskManager::Dilate(int index, int radius, const char* name) {
    if (!IsValidIndex(index)) return -1;
    
    auto mask = masks_[index];
    auto result = std::make_shared<MaskData>(mask->width, mask->height, mask->depth);
    result->name = name;
    
    // 3D膨胀：每个前景点扩展到周围球形邻域
    for (int z = 0; z < mask->depth; z++) {
        for (int y = 0; y < mask->height; y++) {
            for (int x = 0; x < mask->width; x++) {
                if (mask->Get(x, y, z) > 0) {
                    // 扩展到半径范围内的所有点
                    for (int dz = -radius; dz <= radius; dz++) {
                        for (int dy = -radius; dy <= radius; dy++) {
                            for (int dx = -radius; dx <= radius; dx++) {
                                // 检查是否在球形内
                                if (dx*dx + dy*dy + dz*dz <= radius*radius) {
                                    result->Set(x + dx, y + dy, z + dz, 255);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    result->UpdateStatistics();
    masks_.push_back(result);
    return static_cast<int>(masks_.size()) - 1;
}

int MaskManager::Erode(int index, int radius, const char* name) {
    if (!IsValidIndex(index)) return -1;
    
    auto mask = masks_[index];
    auto result = std::make_shared<MaskData>(mask->width, mask->height, mask->depth);
    result->name = name;
    
    // 3D腐蚀：只保留周围球形邻域全为前景的点
    for (int z = 0; z < mask->depth; z++) {
        for (int y = 0; y < mask->height; y++) {
            for (int x = 0; x < mask->width; x++) {
                if (mask->Get(x, y, z) > 0) {
                    // 检查周围邻域是否全为前景
                    bool allForeground = true;
                    for (int dz = -radius; dz <= radius && allForeground; dz++) {
                        for (int dy = -radius; dy <= radius && allForeground; dy++) {
                            for (int dx = -radius; dx <= radius && allForeground; dx++) {
                                if (dx*dx + dy*dy + dz*dz <= radius*radius) {
                                    if (mask->Get(x + dx, y + dy, z + dz) == 0) {
                                        allForeground = false;
                                    }
                                }
                            }
                        }
                    }
                    if (allForeground) {
                        result->Set(x, y, z, 255);
                    }
                }
            }
        }
    }
    
    result->UpdateStatistics();
    masks_.push_back(result);
    return static_cast<int>(masks_.size()) - 1;
}

int MaskManager::Opening(int index, int radius, const char* name) {
    // 开运算 = 先腐蚀后膨胀
    int eroded = Erode(index, radius, "temp_eroded");
    if (eroded < 0) return -1;
    
    int result = Dilate(eroded, radius, name);
    DeleteMask(eroded);  // 删除临时Mask
    
    return result;
}

int MaskManager::Closing(int index, int radius, const char* name) {
    // 闭运算 = 先膨胀后腐蚀
    int dilated = Dilate(index, radius, "temp_dilated");
    if (dilated < 0) return -1;
    
    int result = Erode(dilated, radius, name);
    DeleteMask(dilated);  // 删除临时Mask
    
    return result;
}

int MaskManager::MorphologicalGradient(int index, int radius, const char* name) {
    if (!IsValidIndex(index)) return -1;
    
    // 形态学梯度 = 膨胀 - 腐蚀
    int dilated = Dilate(index, radius, "temp_dilated");
    int eroded = Erode(index, radius, "temp_eroded");
    
    if (dilated < 0 || eroded < 0) {
        if (dilated >= 0) DeleteMask(dilated);
        if (eroded >= 0) DeleteMask(eroded);
        return -1;
    }
    
    int result = Difference(dilated, eroded, name);
    
    DeleteMask(eroded);
    DeleteMask(dilated - 1);  // 删除dilated后索引会变
    
    return result;
}

// ==================== 连通域操作 ====================

int MaskManager::FloodFill(int index, int seedX, int seedY, int seedZ, const char* name) {
    if (!IsValidIndex(index)) return -1;
    
    auto mask = masks_[index];
    
    // 检查种子点是否有效
    if (mask->Get(seedX, seedY, seedZ) == 0) {
        return -1;  // 种子点不在前景中
    }
    
    auto result = std::make_shared<MaskData>(mask->width, mask->height, mask->depth);
    result->name = name;
    
    // 3D FloodFill（6-连通）
    std::queue<std::tuple<int, int, int>> queue;
    std::set<std::tuple<int, int, int>> visited;
    
    queue.push({seedX, seedY, seedZ});
    visited.insert({seedX, seedY, seedZ});
    
    const int dx[] = {1, -1, 0, 0, 0, 0};
    const int dy[] = {0, 0, 1, -1, 0, 0};
    const int dz[] = {0, 0, 0, 0, 1, -1};
    
    while (!queue.empty()) {
        auto [x, y, z] = queue.front();
        queue.pop();
        
        result->Set(x, y, z, 255);
        
        // 检查6个邻居
        for (int i = 0; i < 6; i++) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            int nz = z + dz[i];
            
            auto pos = std::make_tuple(nx, ny, nz);
            if (visited.find(pos) == visited.end() && mask->Get(nx, ny, nz) > 0) {
                visited.insert(pos);
                queue.push(pos);
            }
        }
    }
    
    result->UpdateStatistics();
    masks_.push_back(result);
    return static_cast<int>(masks_.size()) - 1;
}

int MaskManager::FloodFillMulti(int index, const int* seedsX, const int* seedsY, 
                               const int* seedsZ, int numSeeds, const char* name) {
    if (!IsValidIndex(index) || numSeeds <= 0) return -1;
    
    auto mask = masks_[index];
    auto result = std::make_shared<MaskData>(mask->width, mask->height, mask->depth);
    result->name = name;
    
    std::queue<std::tuple<int, int, int>> queue;
    std::set<std::tuple<int, int, int>> visited;
    
    // 添加所有种子点
    for (int i = 0; i < numSeeds; i++) {
        if (mask->Get(seedsX[i], seedsY[i], seedsZ[i]) > 0) {
            auto pos = std::make_tuple(seedsX[i], seedsY[i], seedsZ[i]);
            queue.push(pos);
            visited.insert(pos);
        }
    }
    
    const int dx[] = {1, -1, 0, 0, 0, 0};
    const int dy[] = {0, 0, 1, -1, 0, 0};
    const int dz[] = {0, 0, 0, 0, 1, -1};
    
    while (!queue.empty()) {
        auto [x, y, z] = queue.front();
        queue.pop();
        
        result->Set(x, y, z, 255);
        
        for (int i = 0; i < 6; i++) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            int nz = z + dz[i];
            
            auto pos = std::make_tuple(nx, ny, nz);
            if (visited.find(pos) == visited.end() && mask->Get(nx, ny, nz) > 0) {
                visited.insert(pos);
                queue.push(pos);
            }
        }
    }
    
    result->UpdateStatistics();
    masks_.push_back(result);
    return static_cast<int>(masks_.size()) - 1;
}

std::vector<int> MaskManager::GetConnectedComponents(int index) {
    if (!IsValidIndex(index)) return {};
    
    auto mask = masks_[index];
    std::vector<uint8_t> visited(mask->width * mask->height * mask->depth, 0);
    std::vector<int> componentIndices;
    
    const int dx[] = {1, -1, 0, 0, 0, 0};
    const int dy[] = {0, 0, 1, -1, 0, 0};
    const int dz[] = {0, 0, 0, 0, 1, -1};
    
    int componentCount = 0;
    
    // 遍历所有体素
    for (int z = 0; z < mask->depth; z++) {
        for (int y = 0; y < mask->height; y++) {
            for (int x = 0; x < mask->width; x++) {
                int idx = z * mask->width * mask->height + y * mask->width + x;
                
                if (mask->data[idx] > 0 && visited[idx] == 0) {
                    // 找到新的连通域
                    auto component = std::make_shared<MaskData>(mask->width, mask->height, mask->depth);
                    component->name = mask->name + " CC" + std::to_string(componentCount++);
                    
                    // BFS标记这个连通域
                    std::queue<std::tuple<int, int, int>> queue;
                    queue.push({x, y, z});
                    visited[idx] = 1;
                    
                    while (!queue.empty()) {
                        auto [cx, cy, cz] = queue.front();
                        queue.pop();
                        
                        component->Set(cx, cy, cz, 255);
                        
                        for (int i = 0; i < 6; i++) {
                            int nx = cx + dx[i];
                            int ny = cy + dy[i];
                            int nz = cz + dz[i];
                            
                            if (nx >= 0 && nx < mask->width &&
                                ny >= 0 && ny < mask->height &&
                                nz >= 0 && nz < mask->depth) {
                                
                                int nidx = nz * mask->width * mask->height + ny * mask->width + nx;
                                if (mask->data[nidx] > 0 && visited[nidx] == 0) {
                                    visited[nidx] = 1;
                                    queue.push({nx, ny, nz});
                                }
                            }
                        }
                    }
                    
                    component->UpdateStatistics();
                    masks_.push_back(component);
                    componentIndices.push_back(static_cast<int>(masks_.size()) - 1);
                }
            }
        }
    }
    
    return componentIndices;
}

int MaskManager::RemoveSmallRegions(int index, int minVoxels, const char* name) {
    if (!IsValidIndex(index)) return -1;
    
    // 获取所有连通域
    auto components = GetConnectedComponents(index);
    if (components.empty()) return -1;
    
    // 收集大于阈值的连通域索引
    std::vector<int> largeComponents;
    std::vector<int> smallComponents;
    
    for (int compIdx : components) {
        auto comp = masks_[compIdx];
        if (comp->voxelCount >= minVoxels) {
            largeComponents.push_back(compIdx);
        } else {
            smallComponents.push_back(compIdx);
        }
    }
    
    // 如果没有大连通域，返回-1
    if (largeComponents.empty()) {
        // 删除所有小连通域（从后向前删除以避免索引问题）
        std::sort(smallComponents.begin(), smallComponents.end(), std::greater<int>());
        for (int idx : smallComponents) {
            DeleteMask(idx);
        }
        return -1;
    }
    
    // 合并所有大连通域
    int resultIdx = largeComponents[0];
    masks_[resultIdx]->name = name;
    
    for (size_t i = 1; i < largeComponents.size(); i++) {
        int compIdx = largeComponents[i];
        // 执行并集操作
        int tempIdx = Union(resultIdx, compIdx, name);
        
        // 删除旧的两个mask（从大索引开始删除）
        int delIdx1 = std::max(resultIdx, compIdx);
        int delIdx2 = std::min(resultIdx, compIdx);
        DeleteMask(delIdx1);
        DeleteMask(delIdx2);
        
        // 更新结果索引（新mask总是在最后）
        resultIdx = tempIdx;
    }
    
    // 删除所有小连通域（从后向前删除）
    std::sort(smallComponents.begin(), smallComponents.end(), std::greater<int>());
    for (int idx : smallComponents) {
        // 如果索引大于resultIdx，需要调整
        if (idx > resultIdx) {
            DeleteMask(idx);
        } else if (idx < resultIdx) {
            DeleteMask(idx);
            resultIdx--;  // 结果索引需要前移
        }
    }
    
    return resultIdx;
}

int MaskManager::KeepLargestRegion(int index, const char* name) {
    if (!IsValidIndex(index)) return -1;
    
    auto components = GetConnectedComponents(index);
    
    if (components.empty()) return -1;
    
    // 找到最大的连通域
    int largestIdx = components[0];
    int largestSize = masks_[largestIdx]->voxelCount;
    
    for (size_t i = 1; i < components.size(); i++) {
        int compIdx = components[i];
        if (masks_[compIdx]->voxelCount > largestSize) {
            largestIdx = compIdx;
            largestSize = masks_[compIdx]->voxelCount;
        }
    }
    
    // 收集要删除的索引（除了最大的）
    std::vector<int> toDelete;
    for (int compIdx : components) {
        if (compIdx != largestIdx) {
            toDelete.push_back(compIdx);
        }
    }
    
    // 从后向前删除（避免索引变化问题）
    std::sort(toDelete.begin(), toDelete.end(), std::greater<int>());
    for (int idx : toDelete) {
        if (idx > largestIdx) {
            DeleteMask(idx);
        } else {
            DeleteMask(idx);
            largestIdx--;  // 如果删除的在前面，最大索引需要前移
        }
    }
    
    masks_[largestIdx]->name = name;
    return largestIdx;
}

// ==================== ROI绘制 ====================

void MaskManager::DrawRectangle(int index, int z, int x1, int y1, int x2, int y2, bool fill) {
    if (!IsValidIndex(index)) return;
    auto mask = masks_[index];
    
    if (z < 0 || z >= mask->depth) return;
    
    int minX = std::min(x1, x2);
    int maxX = std::max(x1, x2);
    int minY = std::min(y1, y2);
    int maxY = std::max(y1, y2);
    
    minX = std::max(0, minX);
    maxX = std::min(mask->width - 1, maxX);
    minY = std::max(0, minY);
    maxY = std::min(mask->height - 1, maxY);
    
    if (fill) {
        // 填充矩形
        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                mask->Set(x, y, z, 255);
            }
        }
    } else {
        // 只绘制边框
        for (int x = minX; x <= maxX; x++) {
            mask->Set(x, minY, z, 255);
            mask->Set(x, maxY, z, 255);
        }
        for (int y = minY; y <= maxY; y++) {
            mask->Set(minX, y, z, 255);
            mask->Set(maxX, y, z, 255);
        }
    }
    
    mask->UpdateStatistics();
}

void MaskManager::DrawCircle(int index, int z, int cx, int cy, int radius, bool fill) {
    if (!IsValidIndex(index)) return;
    auto mask = masks_[index];
    
    if (z < 0 || z >= mask->depth) return;
    
    int minX = std::max(0, cx - radius);
    int maxX = std::min(mask->width - 1, cx + radius);
    int minY = std::max(0, cy - radius);
    int maxY = std::min(mask->height - 1, cy + radius);
    
    int r2 = radius * radius;
    
    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            int dx = x - cx;
            int dy = y - cy;
            int dist2 = dx * dx + dy * dy;
            
            if (fill) {
                if (dist2 <= r2) {
                    mask->Set(x, y, z, 255);
                }
            } else {
                // 边界：距离在[r-1, r+1]之间
                if (dist2 >= (radius - 1) * (radius - 1) && dist2 <= r2) {
                    mask->Set(x, y, z, 255);
                }
            }
        }
    }
    
    mask->UpdateStatistics();
}

void MaskManager::DrawBrush(int index, int z, int x, int y, int brushRadius) {
    if (!IsValidIndex(index)) return;
    auto mask = masks_[index];
    
    if (z < 0 || z >= mask->depth) return;
    
    int minX = std::max(0, x - brushRadius);
    int maxX = std::min(mask->width - 1, x + brushRadius);
    int minY = std::max(0, y - brushRadius);
    int maxY = std::min(mask->height - 1, y + brushRadius);
    
    int r2 = brushRadius * brushRadius;
    
    for (int py = minY; py <= maxY; py++) {
        for (int px = minX; px <= maxX; px++) {
            int dx = px - x;
            int dy = py - y;
            if (dx * dx + dy * dy <= r2) {
                mask->Set(px, py, z, 255);
            }
        }
    }
    
    mask->UpdateStatistics();
}

void MaskManager::DrawPolygon(int index, int z, const int* pointsX, const int* pointsY, int numPoints, bool fill) {
    if (!IsValidIndex(index) || numPoints < 3) return;
    auto mask = masks_[index];
    
    if (z < 0 || z >= mask->depth) return;
    
    // 简单实现：先绘制边，如果fill则扫描线填充
    for (int i = 0; i < numPoints; i++) {
        int x1 = pointsX[i];
        int y1 = pointsY[i];
        int x2 = pointsX[(i + 1) % numPoints];
        int y2 = pointsY[(i + 1) % numPoints];
        
        DrawLine(index, z, x1, y1, x2, y2);
    }
    
    if (fill) {
        // 扫描线填充算法
        int minY = mask->height, maxY = 0;
        for (int i = 0; i < numPoints; i++) {
            minY = std::min(minY, pointsY[i]);
            maxY = std::max(maxY, pointsY[i]);
        }
        
        for (int y = minY; y <= maxY; y++) {
            std::vector<int> intersections;
            
            for (int i = 0; i < numPoints; i++) {
                int x1 = pointsX[i];
                int y1 = pointsY[i];
                int x2 = pointsX[(i + 1) % numPoints];
                int y2 = pointsY[(i + 1) % numPoints];
                
                if ((y1 <= y && y < y2) || (y2 <= y && y < y1)) {
                    int x = x1 + (y - y1) * (x2 - x1) / (y2 - y1);
                    intersections.push_back(x);
                }
            }
            
            std::sort(intersections.begin(), intersections.end());
            
            for (size_t i = 0; i + 1 < intersections.size(); i += 2) {
                for (int x = intersections[i]; x <= intersections[i + 1]; x++) {
                    if (x >= 0 && x < mask->width) {
                        mask->Set(x, y, z, 255);
                    }
                }
            }
        }
    }
    
    mask->UpdateStatistics();
}

void MaskManager::DrawLine(int index, int z, int x1, int y1, int x2, int y2) {
    if (!IsValidIndex(index)) return;
    auto mask = masks_[index];
    
    if (z < 0 || z >= mask->depth) return;
    
    // Bresenham直线算法
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx - dy;
    
    while (true) {
        if (x1 >= 0 && x1 < mask->width && y1 >= 0 && y1 < mask->height) {
            mask->Set(x1, y1, z, 255);
        }
        
        if (x1 == x2 && y1 == y2) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
    
    mask->UpdateStatistics();
}

// ==================== 导入导出 ====================

bool MaskManager::SaveToFile(int index, const char* filepath) {
    if (!IsValidIndex(index)) return false;
    auto mask = masks_[index];
    
    FILE* file = fopen(filepath, "wb");
    if (!file) return false;
    
    // 写入头部信息
    fwrite(&mask->width, sizeof(int), 1, file);
    fwrite(&mask->height, sizeof(int), 1, file);
    fwrite(&mask->depth, sizeof(int), 1, file);
    
    // 写入名称
    int nameLen = static_cast<int>(mask->name.length());
    fwrite(&nameLen, sizeof(int), 1, file);
    fwrite(mask->name.c_str(), 1, nameLen, file);
    
    // 写入颜色和元数据
    fwrite(mask->color, sizeof(float), 4, file);
    fwrite(&mask->minThreshold, sizeof(float), 1, file);
    fwrite(&mask->maxThreshold, sizeof(float), 1, file);
    
    // 写入数据
    fwrite(mask->data.data(), 1, mask->data.size(), file);
    
    fclose(file);
    return true;
}

int MaskManager::LoadFromFile(const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (!file) return -1;
    
    int width, height, depth;
    fread(&width, sizeof(int), 1, file);
    fread(&height, sizeof(int), 1, file);
    fread(&depth, sizeof(int), 1, file);
    
    auto mask = std::make_shared<MaskData>(width, height, depth);
    
    // 读取名称
    int nameLen;
    fread(&nameLen, sizeof(int), 1, file);
    mask->name.resize(nameLen);
    fread(&mask->name[0], 1, nameLen, file);
    
    // 读取颜色和元数据
    fread(mask->color, sizeof(float), 4, file);
    fread(&mask->minThreshold, sizeof(float), 1, file);
    fread(&mask->maxThreshold, sizeof(float), 1, file);
    
    // 读取数据
    fread(mask->data.data(), 1, mask->data.size(), file);
    
    fclose(file);
    
    mask->UpdateStatistics();
    masks_.push_back(mask);
    return static_cast<int>(masks_.size()) - 1;
}

// ==================== 测量分析 ====================

int MaskManager::CalculateVolume(int index) {
    if (!IsValidIndex(index)) return 0;
    return masks_[index]->voxelCount;
}

void MaskManager::CalculateCentroid(int index, float* cx, float* cy, float* cz) {
    if (!IsValidIndex(index)) return;
    auto mask = masks_[index];
    
    if (cx) *cx = mask->centerX;
    if (cy) *cy = mask->centerY;
    if (cz) *cz = mask->centerZ;
}

void MaskManager::CalculateBoundingBox(int index, int* minX, int* minY, int* minZ,
                                       int* maxX, int* maxY, int* maxZ) {
    if (!IsValidIndex(index)) return;
    auto mask = masks_[index];
    
    int minXVal = mask->width, minYVal = mask->height, minZVal = mask->depth;
    int maxXVal = 0, maxYVal = 0, maxZVal = 0;
    
    for (int z = 0; z < mask->depth; z++) {
        for (int y = 0; y < mask->height; y++) {
            for (int x = 0; x < mask->width; x++) {
                if (mask->Get(x, y, z) > 0) {
                    minXVal = std::min(minXVal, x);
                    maxXVal = std::max(maxXVal, x);
                    minYVal = std::min(minYVal, y);
                    maxYVal = std::max(maxYVal, y);
                    minZVal = std::min(minZVal, z);
                    maxZVal = std::max(maxZVal, z);
                }
            }
        }
    }
    
    if (minX) *minX = minXVal;
    if (minY) *minY = minYVal;
    if (minZ) *minZ = minZVal;
    if (maxX) *maxX = maxXVal;
    if (maxY) *maxY = maxYVal;
    if (maxZ) *maxZ = maxZVal;
}

int MaskManager::ExtractBoundary(int index, const char* name) {
    if (!IsValidIndex(index)) return -1;
    auto mask = masks_[index];
    
    auto boundary = std::make_shared<MaskData>(mask->width, mask->height, mask->depth);
    boundary->name = name;
    
    // 边界：前景像素且至少有一个背景邻居
    const int dx[] = {1, -1, 0, 0, 0, 0};
    const int dy[] = {0, 0, 1, -1, 0, 0};
    const int dz[] = {0, 0, 0, 0, 1, -1};
    
    for (int z = 0; z < mask->depth; z++) {
        for (int y = 0; y < mask->height; y++) {
            for (int x = 0; x < mask->width; x++) {
                if (mask->Get(x, y, z) > 0) {
                    bool isBoundary = false;
                    
                    for (int i = 0; i < 6; i++) {
                        int nx = x + dx[i];
                        int ny = y + dy[i];
                        int nz = z + dz[i];
                        
                        if (nx < 0 || nx >= mask->width ||
                            ny < 0 || ny >= mask->height ||
                            nz < 0 || nz >= mask->depth ||
                            mask->Get(nx, ny, nz) == 0) {
                            isBoundary = true;
                            break;
                        }
                    }
                    
                    if (isBoundary) {
                        boundary->Set(x, y, z, 255);
                    }
                }
            }
        }
    }
    
    boundary->UpdateStatistics();
    masks_.push_back(boundary);
    return static_cast<int>(masks_.size()) - 1;
}

void MaskManager::CalculateHistogram(int index, const uint16_t* volumeData, int width, int height, int depth,
                                     int* histogram, int numBins, uint16_t* minValue, uint16_t* maxValue) {
    if (!IsValidIndex(index) || !volumeData || !histogram) return;
    auto mask = masks_[index];
    
    if (mask->width != width || mask->height != height || mask->depth != depth) return;
    
    // 初始化直方图
    for (int i = 0; i < numBins; i++) {
        histogram[i] = 0;
    }
    
    // 找到Mask内的最小最大值
    uint16_t minVal = 65535, maxVal = 0;
    bool hasValue = false;
    
    for (int z = 0; z < depth; z++) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (mask->Get(x, y, z) > 0) {
                    int idx = z * width * height + y * width + x;
                    uint16_t val = volumeData[idx];
                    
                    if (!hasValue) {
                        minVal = maxVal = val;
                        hasValue = true;
                    } else {
                        minVal = std::min(minVal, val);
                        maxVal = std::max(maxVal, val);
                    }
                }
            }
        }
    }
    
    if (!hasValue) return;
    
    if (minValue) *minValue = minVal;
    if (maxValue) *maxValue = maxVal;
    
    // 计算直方图
    float range = static_cast<float>(maxVal - minVal);
    if (range < 1.0f) range = 1.0f;
    
    for (int z = 0; z < depth; z++) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (mask->Get(x, y, z) > 0) {
                    int idx = z * width * height + y * width + x;
                    uint16_t val = volumeData[idx];
                    
                    int bin = static_cast<int>((val - minVal) * (numBins - 1) / range);
                    bin = std::max(0, std::min(numBins - 1, bin));
                    histogram[bin]++;
                }
            }
        }
    }
}

// ==================== 高级操作 ====================

int MaskManager::FillHoles(int index, const char* name) {
    if (!IsValidIndex(index)) return -1;
    auto mask = masks_[index];
    
    // 从边界开始FloodFill背景，剩余的背景就是孔洞
    auto background = std::make_shared<MaskData>(mask->width, mask->height, mask->depth);
    
    std::queue<std::tuple<int, int, int>> queue;
    std::set<std::tuple<int, int, int>> visited;
    
    // 从所有边界的背景像素开始
    for (int z = 0; z < mask->depth; z++) {
        for (int y = 0; y < mask->height; y++) {
            for (int x = 0; x < mask->width; x++) {
                bool isBorder = (x == 0 || x == mask->width - 1 ||
                               y == 0 || y == mask->height - 1 ||
                               z == 0 || z == mask->depth - 1);
                
                if (isBorder && mask->Get(x, y, z) == 0) {
                    auto pos = std::make_tuple(x, y, z);
                    if (visited.find(pos) == visited.end()) {
                        queue.push(pos);
                        visited.insert(pos);
                    }
                }
            }
        }
    }
    
    // BFS标记外部背景
    const int dx[] = {1, -1, 0, 0, 0, 0};
    const int dy[] = {0, 0, 1, -1, 0, 0};
    const int dz[] = {0, 0, 0, 0, 1, -1};
    
    while (!queue.empty()) {
        auto [x, y, z] = queue.front();
        queue.pop();
        
        background->Set(x, y, z, 255);
        
        for (int i = 0; i < 6; i++) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            int nz = z + dz[i];
            
            auto pos = std::make_tuple(nx, ny, nz);
            if (visited.find(pos) == visited.end() && mask->Get(nx, ny, nz) == 0) {
                visited.insert(pos);
                queue.push(pos);
            }
        }
    }
    
    // 结果 = 原始 OR (NOT 外部背景)
    auto filled = std::make_shared<MaskData>(mask->width, mask->height, mask->depth);
    filled->name = name;
    
    for (size_t i = 0; i < mask->data.size(); i++) {
        filled->data[i] = (mask->data[i] > 0 || background->data[i] == 0) ? 255 : 0;
    }
    
    filled->UpdateStatistics();
    masks_.push_back(filled);
    return static_cast<int>(masks_.size()) - 1;
}
