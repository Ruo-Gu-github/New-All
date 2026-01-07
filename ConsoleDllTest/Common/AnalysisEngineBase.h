#pragma once

#include "NativeInterfaces.h"
#include <vector>
#include <string>
#include <cstdint>

/// Analysis引擎基类，为Bone/Fat/Lung等分析DLL提供统一接口
class AnalysisEngineBase {
protected:
    bool initialized_ = false;
    std::vector<double> voxels_;
    std::uint32_t width_ = 0;
    std::uint32_t height_ = 0;
    std::uint32_t depth_ = 0;
    double spacing_ = 1.0;
    std::string lastError_;
    
    /// 设置错误信息
    void SetError(const std::string& error) {
        lastError_ = error;
    }
    
public:
    virtual ~AnalysisEngineBase() = default;
    
    /// 初始化引擎
    virtual NativeResult Initialize() {
        initialized_ = true;
        return NATIVE_OK;
    }
    
    /// 加载合成体数据（用于测试）
    virtual NativeResult LoadSyntheticVolume(
        std::uint32_t width,
        std::uint32_t height,
        std::uint32_t depth,
        double spacing);
    
    /// 获取体数据统计信息
    virtual NativeResult GetVolumeStats(EngineVolumeStats* outStats);
    
    /// 获取最后的错误信息
    const char* GetLastError() const {
        return lastError_.c_str();
    }
    
    /// 检查是否已初始化
    bool IsInitialized() const {
        return initialized_;
    }
    
    /// 获取体数据尺寸
    void GetDimensions(std::uint32_t& width, std::uint32_t& height, std::uint32_t& depth) const {
        width = width_;
        height = height_;
        depth = depth_;
    }
    
    /// 子类可重写的虚方法，用于特定分析算法
    virtual NativeResult RunAnalysis() {
        SetError("Analysis not implemented in base class");
        return NATIVE_E_NOT_IMPLEMENTED;
    }
};
