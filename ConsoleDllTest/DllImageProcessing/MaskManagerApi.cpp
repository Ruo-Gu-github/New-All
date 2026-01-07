#include "pch.h"
#include "ImageProcessingApi.h"
#include "MaskManager.h"
#include <string>
#include <algorithm>

// 前向声明Dicom Volume API
extern "C" {
    void* Dicom_Volume_GetData(VolumeHandle handle);
    NativeResult Dicom_Volume_GetDimensions(VolumeHandle handle, int* width, int* height, int* depth);
}

// 全局错误信息
static thread_local std::string g_lastError;

static void SetLastError(const std::string& error) {
    g_lastError = error;
}

// 注意：ImageProcessing_GetLastError 在 ImageProcessingApi.cpp 中定义

// ==================== Mask管理器 ====================

MaskManagerHandle MaskManager_Create() {
    try {
        return new MaskManager();
    } catch (const std::exception& e) {
        SetLastError(e.what());
        return nullptr;
    }
}

void MaskManager_Destroy(MaskManagerHandle handle) {
    if (handle) {
        delete static_cast<MaskManager*>(handle);
    }
}

int MaskManager_GetCount(MaskManagerHandle handle) {
    if (!handle) return 0;
    return static_cast<MaskManager*>(handle)->GetMaskCount();
}

NativeResult MaskManager_DeleteMask(MaskManagerHandle handle, int index) {
    if (!handle) return NATIVE_E_INVALID_ARGUMENT;
    return static_cast<MaskManager*>(handle)->DeleteMask(index) ? NATIVE_OK : NATIVE_E_INTERNAL_ERROR;
}

void MaskManager_Clear(MaskManagerHandle handle) {
    if (handle) {
        static_cast<MaskManager*>(handle)->Clear();
    }
}

// ==================== Mask创建 ====================

int MaskManager_CreateFromThreshold(MaskManagerHandle handle, VolumeHandle volume,
                                    float minThreshold, float maxThreshold, const char* name) {
    if (!handle || !volume) return -1;
    
    try {
        // 获取体数据维度
        int width, height, depth;
        if (Dicom_Volume_GetDimensions(volume, &width, &height, &depth) != NATIVE_OK) {
            SetLastError("Failed to get volume dimensions");
            return -1;
        }
        
        // 获取体数据指针
        const uint16_t* volumeData = static_cast<const uint16_t*>(Dicom_Volume_GetData(volume));
        if (!volumeData) {
            SetLastError("Failed to get volume data");
            return -1;
        }
        
        return static_cast<MaskManager*>(handle)->CreateFromThreshold(
            volumeData, width, height, depth,
            minThreshold, maxThreshold, name ? name : "Mask");
    } catch (const std::exception& e) {
        SetLastError(e.what());
        return -1;
    }
}

int MaskManager_CreateEmpty(MaskManagerHandle handle, int width, int height, int depth, const char* name) {
    if (!handle) return -1;
    return static_cast<MaskManager*>(handle)->CreateEmpty(width, height, depth, name ? name : "Empty");
}

int MaskManager_Clone(MaskManagerHandle handle, int index) {
    if (!handle) return -1;
    return static_cast<MaskManager*>(handle)->Clone(index);
}

// ==================== Mask属性 ====================

void MaskManager_SetName(MaskManagerHandle handle, int index, const char* name) {
    if (handle && name) {
        static_cast<MaskManager*>(handle)->SetMaskName(index, name);
    }
}

const char* MaskManager_GetName(MaskManagerHandle handle, int index) {
    if (!handle) return "";
    auto mask = static_cast<MaskManager*>(handle)->GetMask(index);
    return mask ? mask->name.c_str() : "";
}

void MaskManager_SetColor(MaskManagerHandle handle, int index, float r, float g, float b, float a) {
    if (handle) {
        static_cast<MaskManager*>(handle)->SetMaskColor(index, r, g, b, a);
    }
}

void MaskManager_GetColor(MaskManagerHandle handle, int index, float* r, float* g, float* b, float* a) {
    if (!handle) return;
    auto mask = static_cast<MaskManager*>(handle)->GetMask(index);
    if (mask) {
        if (r) *r = mask->color[0];
        if (g) *g = mask->color[1];
        if (b) *b = mask->color[2];
        if (a) *a = mask->color[3];
    }
}

void MaskManager_SetVisible(MaskManagerHandle handle, int index, bool visible) {
    if (handle) {
        static_cast<MaskManager*>(handle)->SetMaskVisible(index, visible);
    }
}

bool MaskManager_GetVisible(MaskManagerHandle handle, int index) {
    if (!handle) return false;
    auto mask = static_cast<MaskManager*>(handle)->GetMask(index);
    return mask ? mask->visible : false;
}

const uint8_t* MaskManager_GetData(MaskManagerHandle handle, int index) {
    if (!handle) return nullptr;
    auto mask = static_cast<MaskManager*>(handle)->GetMask(index);
    return mask ? mask->data.data() : nullptr;
}

void MaskManager_GetDimensions(MaskManagerHandle handle, int index, int* width, int* height, int* depth) {
    if (!handle) return;
    auto mask = static_cast<MaskManager*>(handle)->GetMask(index);
    if (mask) {
        if (width) *width = mask->width;
        if (height) *height = mask->height;
        if (depth) *depth = mask->depth;
    }
}

// ==================== 2D 编辑工具 ====================
void MaskManager_DrawCircle2D(MaskManagerHandle handle, int index, int sliceDir, int sliceIndex, float cx, float cy, float radius) {
    if (!handle) return;
    static_cast<MaskManager*>(handle)->DrawCircle2D(index, sliceDir, sliceIndex, cx, cy, radius);
}

void MaskManager_EraseCircle2D(MaskManagerHandle handle, int index, int sliceDir, int sliceIndex, float cx, float cy, float radius) {
    if (!handle) return;
    static_cast<MaskManager*>(handle)->EraseCircle2D(index, sliceDir, sliceIndex, cx, cy, radius);
}


// ==================== 布尔运算 ====================

int MaskManager_Union(MaskManagerHandle handle, int indexA, int indexB, const char* name) {
    if (!handle) return -1;
    return static_cast<MaskManager*>(handle)->Union(indexA, indexB, name ? name : "Union");
}

int MaskManager_Intersection(MaskManagerHandle handle, int indexA, int indexB, const char* name) {
    if (!handle) return -1;
    return static_cast<MaskManager*>(handle)->Intersection(indexA, indexB, name ? name : "Intersection");
}

int MaskManager_Difference(MaskManagerHandle handle, int indexA, int indexB, const char* name) {
    if (!handle) return -1;
    return static_cast<MaskManager*>(handle)->Difference(indexA, indexB, name ? name : "Difference");
}

int MaskManager_XOR(MaskManagerHandle handle, int indexA, int indexB, const char* name) {
    if (!handle) return -1;
    return static_cast<MaskManager*>(handle)->XOR(indexA, indexB, name ? name : "XOR");
}

int MaskManager_Invert(MaskManagerHandle handle, int index, const char* name) {
    if (!handle) return -1;
    return static_cast<MaskManager*>(handle)->Invert(index, name ? name : "Inverted");
}

// ==================== 形态学操作 ====================

int MaskManager_Dilate(MaskManagerHandle handle, int index, int radius, const char* name) {
    if (!handle) return -1;
    return static_cast<MaskManager*>(handle)->Dilate(index, radius, name ? name : "Dilated");
}

int MaskManager_Erode(MaskManagerHandle handle, int index, int radius, const char* name) {
    if (!handle) return -1;
    return static_cast<MaskManager*>(handle)->Erode(index, radius, name ? name : "Eroded");
}

int MaskManager_Opening(MaskManagerHandle handle, int index, int radius, const char* name) {
    if (!handle) return -1;
    return static_cast<MaskManager*>(handle)->Opening(index, radius, name ? name : "Opened");
}

int MaskManager_Closing(MaskManagerHandle handle, int index, int radius, const char* name) {
    if (!handle) return -1;
    return static_cast<MaskManager*>(handle)->Closing(index, radius, name ? name : "Closed");
}

// ==================== 连通域操作 ====================

int MaskManager_FloodFill(MaskManagerHandle handle, int index, int x, int y, int z, const char* name) {
    if (!handle) return -1;
    return static_cast<MaskManager*>(handle)->FloodFill(index, x, y, z, name ? name : "FloodFill");
}

int MaskManager_FloodFillMulti(MaskManagerHandle handle, int index,
                               const int* seedsX, const int* seedsY, const int* seedsZ,
                               int numSeeds, const char* name) {
    if (!handle) return -1;
    return static_cast<MaskManager*>(handle)->FloodFillMulti(
        index, seedsX, seedsY, seedsZ, numSeeds, name ? name : "FloodFill");
}

int MaskManager_GetConnectedComponents(MaskManagerHandle handle, int index, int* indices, int maxCount) {
    if (!handle) return 0;
    auto components = static_cast<MaskManager*>(handle)->GetConnectedComponents(index);
    int count = (std::min)(static_cast<int>(components.size()), maxCount);
    for (int i = 0; i < count; i++) {
        indices[i] = components[i];
    }
    return static_cast<int>(components.size());
}

int MaskManager_RemoveSmallRegions(MaskManagerHandle handle, int index, int minVoxels, const char* name) {
    if (!handle) return -1;
    return static_cast<MaskManager*>(handle)->RemoveSmallRegions(index, minVoxels, name ? name : "Filtered");
}

int MaskManager_KeepLargestRegion(MaskManagerHandle handle, int index, const char* name) {
    if (!handle) return -1;
    return static_cast<MaskManager*>(handle)->KeepLargestRegion(index, name ? name : "Largest");
}

// ==================== ROI绘制 ====================

void MaskManager_DrawRectangle(MaskManagerHandle handle, int index, int z,
                               int x1, int y1, int x2, int y2, bool fill) {
    if (handle) {
        static_cast<MaskManager*>(handle)->DrawRectangle(index, z, x1, y1, x2, y2, fill);
    }
}

void MaskManager_DrawCircle(MaskManagerHandle handle, int index, int z,
                            int cx, int cy, int radius, bool fill) {
    if (handle) {
        static_cast<MaskManager*>(handle)->DrawCircle(index, z, cx, cy, radius, fill);
    }
}

void MaskManager_DrawBrush(MaskManagerHandle handle, int index, int z,
                           int x, int y, int brushRadius) {
    if (handle) {
        static_cast<MaskManager*>(handle)->DrawBrush(index, z, x, y, brushRadius);
    }
}

void MaskManager_DrawPolygon(MaskManagerHandle handle, int index, int z,
                            const int* pointsX, const int* pointsY, int numPoints, bool fill) {
    if (handle) {
        static_cast<MaskManager*>(handle)->DrawPolygon(index, z, pointsX, pointsY, numPoints, fill);
    }
}

void MaskManager_DrawLine(MaskManagerHandle handle, int index, int z,
                         int x1, int y1, int x2, int y2) {
    if (handle) {
        static_cast<MaskManager*>(handle)->DrawLine(index, z, x1, y1, x2, y2);
    }
}

// ==================== 测量分析 ====================

int MaskManager_CalculateVolume(MaskManagerHandle handle, int index) {
    if (!handle) return 0;
    return static_cast<MaskManager*>(handle)->CalculateVolume(index);
}

void MaskManager_CalculateCentroid(MaskManagerHandle handle, int index,
                                   float* cx, float* cy, float* cz) {
    if (handle) {
        static_cast<MaskManager*>(handle)->CalculateCentroid(index, cx, cy, cz);
    }
}

void MaskManager_CalculateBoundingBox(MaskManagerHandle handle, int index,
                                     int* minX, int* minY, int* minZ,
                                     int* maxX, int* maxY, int* maxZ) {
    if (handle) {
        static_cast<MaskManager*>(handle)->CalculateBoundingBox(index, minX, minY, minZ, maxX, maxY, maxZ);
    }
}

int MaskManager_ExtractBoundary(MaskManagerHandle handle, int index, const char* name) {
    if (!handle) return -1;
    return static_cast<MaskManager*>(handle)->ExtractBoundary(index, name ? name : "Boundary");
}

void MaskManager_CalculateHistogram(MaskManagerHandle handle, int index,
                                   const uint16_t* volumeData, int width, int height, int depth,
                                   int* histogram, int numBins, uint16_t* minValue, uint16_t* maxValue) {
    if (handle) {
        static_cast<MaskManager*>(handle)->CalculateHistogram(index, volumeData, width, height, depth,
                                                             histogram, numBins, minValue, maxValue);
    }
}

bool MaskManager_SaveToFile(MaskManagerHandle handle, int index, const char* filepath) {
    if (!handle) return false;
    return static_cast<MaskManager*>(handle)->SaveToFile(index, filepath);
}

int MaskManager_LoadFromFile(MaskManagerHandle handle, const char* filepath) {
    if (!handle) return -1;
    return static_cast<MaskManager*>(handle)->LoadFromFile(filepath);
}

// ==================== 高级操作 ====================

int MaskManager_FillHoles(MaskManagerHandle handle, int index, const char* name) {
    if (!handle) return -1;
    return static_cast<MaskManager*>(handle)->FillHoles(index, name ? name : "Filled");
}

int MaskManager_MorphologicalGradient(MaskManagerHandle handle, int index, int radius, const char* name) {
    if (!handle) return -1;
    return static_cast<MaskManager*>(handle)->MorphologicalGradient(index, radius, name ? name : "Gradient");
}
