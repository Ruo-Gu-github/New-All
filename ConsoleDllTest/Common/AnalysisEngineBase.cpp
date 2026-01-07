#include "pch.h"
#include "AnalysisEngineBase.h"
#include <algorithm>
#include <numeric>

NativeResult AnalysisEngineBase::LoadSyntheticVolume(
    std::uint32_t width,
    std::uint32_t height,
    std::uint32_t depth,
    double spacing)
{
    if (!initialized_) {
        SetError("Engine not initialized");
        return NATIVE_E_NOT_INITIALIZED;
    }

    if (width == 0 || height == 0 || depth == 0 || spacing <= 0.0) {
        SetError("Invalid synthetic volume parameters");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    const std::size_t voxelCount = static_cast<std::size_t>(width) * 
                                   static_cast<std::size_t>(height) * 
                                   static_cast<std::size_t>(depth);

    try {
        voxels_.resize(voxelCount);
    }
    catch (const std::bad_alloc&) {
        SetError("Failed to allocate synthetic volume data");
        return NATIVE_E_INTERNAL_ERROR;
    }

    // 生成合成数据
    for (std::size_t z = 0; z < depth; ++z) {
        for (std::size_t y = 0; y < height; ++y) {
            for (std::size_t x = 0; x < width; ++x) {
                const std::size_t index = (z * static_cast<std::size_t>(height) + y) * 
                                         static_cast<std::size_t>(width) + x;
                const double value = static_cast<double>((x + y + z) % 4096);
                voxels_[index] = value;
            }
        }
    }

    width_ = width;
    height_ = height;
    depth_ = depth;
    spacing_ = spacing;

    return NATIVE_OK;
}

NativeResult AnalysisEngineBase::GetVolumeStats(EngineVolumeStats* outStats) {
    if (outStats == nullptr) {
        SetError("Volume stats output pointer is null");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    if (!initialized_) {
        SetError("Engine not initialized");
        return NATIVE_E_NOT_INITIALIZED;
    }

    if (voxels_.empty()) {
        SetError("No volume has been loaded");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    const auto minmax = std::minmax_element(voxels_.begin(), voxels_.end());
    const double sum = std::accumulate(voxels_.begin(), voxels_.end(), 0.0);

    outStats->width = width_;
    outStats->height = height_;
    outStats->depth = depth_;
    outStats->spacing = spacing_;
    outStats->minimumValue = *minmax.first;
    outStats->maximumValue = *minmax.second;
    outStats->meanValue = sum / static_cast<double>(voxels_.size());

    return NATIVE_OK;
}
