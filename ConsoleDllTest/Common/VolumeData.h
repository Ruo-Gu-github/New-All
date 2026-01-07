#pragma once

#include <vector>
#include <cstdint>

/// 体数据上下文（统一定义，供DllDicom和DllVisualization共享）
struct VolumeContext {
    std::vector<uint16_t> data;  // 体数据
    int width = 0;
    int height = 0;
    int depth = 0;
    float spacingX = 1.0f;
    float spacingY = 1.0f;
    float spacingZ = 1.0f;
    float originX = 0.0f;
    float originY = 0.0f;
    float originZ = 0.0f;

    // DICOM rescale (Modality LUT): outputValue = storedValue * slope + intercept
    // Defaults chosen to preserve stored values when tags are missing.
    float rescaleSlope = 1.0f;
    float rescaleIntercept = 0.0f;
    
    // 辅助方法：获取体素值（边界安全）
    uint16_t GetVoxel(int x, int y, int z) const {
        if (x < 0 || x >= width || y < 0 || y >= height || z < 0 || z >= depth) {
            return 0;
        }
        return data[static_cast<size_t>(z) * width * height + y * width + x];
    }
    
    // 辅助方法：设置体素值（边界安全）
    void SetVoxel(int x, int y, int z, uint16_t value) {
        if (x < 0 || x >= width || y < 0 || y >= height || z < 0 || z >= depth) {
            return;
        }
        data[static_cast<size_t>(z) * width * height + y * width + x] = value;
    }
    
    // 辅助方法：获取总体素数
    size_t GetTotalVoxels() const {
        return static_cast<size_t>(width) * height * depth;
    }
    
    // 辅助方法：清空数据
    void Clear() {
        data.clear();
        width = height = depth = 0;
        spacingX = spacingY = spacingZ = 1.0f;
        originX = originY = originZ = 0.0f;

        rescaleSlope = 1.0f;
        rescaleIntercept = 0.0f;
    }
};
