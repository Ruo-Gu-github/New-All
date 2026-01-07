# MPR Mask功能实现计划

## 概述
本文档详细说明了需要在VisualizationApi.cpp中实现的Mask相关功能，用于支持ROI编辑页面的阈值分割和mask显示。

## 需要实现的C++ API

### 1. 获取体数据直方图
```cpp
// VisualizationApi.cpp
VIZ_API NativeResult APR_GetVolumeHistogram(
    const char* sessionId,
    int* binCount,           // 输出：直方图bin数量
    int** histogramData,     // 输出：每个bin的像素计数（需要分配内存）
    float* minValue,         // 输出：CT值最小值
    float* maxValue          // 输出：CT值最大值
) {
    // 1. 根据sessionId获取APRContext
    // 2. 获取volume数据指针
    // 3. 遍历所有体素，统计CT值分布
    // 4. 创建256或512个bins
    // 5. 计算每个bin的像素数量
    // 6. 返回直方图数据和CT值范围
    
    // 实现要点：
    // - 使用short* volume数据，范围通常是-1024~3071（CT值）
    // - Bins可以设为512个，每个bin覆盖约8个HU值
    // - 需要处理边界情况（最小值、最大值）
}
```

**N-API包装：**
```cpp
// visualization_wrapper.cpp
Napi::Value GetVolumeHistogram(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    
    int binCount = 0;
    int* histogramData = nullptr;
    float minValue = 0, maxValue = 0;
    
    NativeResult result = APR_GetVolumeHistogram(
        sessionId.c_str(),
        &binCount,
        &histogramData,
        &minValue,
        &maxValue
    );
    
    if (result != NATIVE_OK) {
        Napi::Error::New(env, "Failed to get histogram").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    // 创建JavaScript数组
    Napi::Array jsArray = Napi::Array::New(env, binCount);
    for (int i = 0; i < binCount; i++) {
        jsArray[i] = Napi::Number::New(env, histogramData[i]);
    }
    
    // 释放C++内存
    delete[] histogramData;
    
    Napi::Object result = Napi::Object::New(env);
    result.Set("data", jsArray);
    result.Set("minValue", Napi::Number::New(env, minValue));
    result.Set("maxValue", Napi::Number::New(env, maxValue));
    
    return result;
}
```

### 2. 更新预览Mask（阈值调节时实时显示）
```cpp
// VisualizationApi.cpp
VIZ_API NativeResult APR_UpdatePreviewMask(
    const char* sessionId,
    float minThreshold,
    float maxThreshold,
    const char* hexColor      // 格式: "#ff0000"
) {
    // 1. 获取APRContext
    // 2. 如果previewMask存在，删除旧的
    // 3. 创建临时mask（不添加到maskVector）
    // 4. 对volume进行阈值处理
    // 5. 设置mask颜色和透明度（alpha=0.5）
    // 6. 保存到ctx->previewMask
    // 7. 标记窗口需要重绘
    
    // 实现要点：
    // - previewMask只用于显示，不计入mask列表
    // - 颜色格式转换：#ff0000 -> r=1.0, g=0.0, b=0.0
    // - 需要invalidate所有4个APR窗口
}
```

### 3. 创建Mask（应用阈值分割）
```cpp
// VisualizationApi.cpp
VIZ_API NativeResult APR_CreateMaskFromThreshold(
    const char* sessionId,
    float minThreshold,
    float maxThreshold,
    const char* hexColor,
    const char* maskName,
    int* outMaskId           // 输出：新创建的mask ID
) {
    // 1. 获取APRContext
    // 2. 创建新的MaskData结构
    // 3. 对volume进行阈值处理，生成mask数据
    // 4. 设置mask属性（name, color, visible=true）
    // 5. 添加到ctx->masks vector
    // 6. 清除previewMask
    // 7. 返回mask ID（vector索引）
    
    // MaskData结构：
    struct MaskData {
        int id;
        std::string name;
        std::vector<uint8_t> data;  // width*height*depth, 0=背景, 255=前景
        int width, height, depth;
        float color[4];             // RGBA
        bool visible;
        float minThreshold, maxThreshold;  // 记录阈值范围
    };
}
```

### 4. 清除预览Mask
```cpp
// VisualizationApi.cpp
VIZ_API NativeResult APR_ClearPreviewMask(const char* sessionId) {
    // 1. 获取APRContext
    // 2. 如果previewMask存在，删除并释放内存
    // 3. 设置ctx->previewMask = nullptr
    // 4. 标记窗口需要重绘
}
```

### 5. 渲染Mask Overlay（在WM_PAINT中调用）
```cpp
// APR_Render函数中添加mask渲染逻辑
void RenderMaskOverlay(APRContext* ctx, int viewIndex) {
    // 遍历ctx->masks
    for (const auto& mask : ctx->masks) {
        if (!mask.visible) continue;
        
        // 根据viewIndex（0=axial, 1=coronal, 2=sagittal）
        // 提取当前切片的mask数据
        int sliceIndex = GetCurrentSliceIndex(ctx, viewIndex);
        
        // 将mask数据叠加到纹理上
        // 使用glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
        // 绘制半透明彩色overlay
    }
    
    // 渲染previewMask（如果存在）
    if (ctx->previewMask) {
        // 同样的渲染逻辑
    }
}
```

## 数据结构设计

### APRContext扩展
```cpp
struct APRContext {
    // ... 现有字段 ...
    
    // Mask管理
    std::vector<MaskData> masks;        // 所有mask列表
    MaskData* previewMask;              // 预览mask（临时）
    VolumeContext* volume;              // volume数据指针
    
    // Mask渲染
    GLuint maskTextures[4];             // 每个view的mask纹理
    bool maskDirty[4];                  // 标记mask需要更新
};
```

### MaskData结构
```cpp
struct MaskData {
    int id;
    std::string name;
    std::vector<uint8_t> data;      // 1字节per voxel, 0=背景, 255=前景
    int width, height, depth;
    float color[4];                 // RGBA (0-1范围)
    bool visible;
    float minThreshold;             // 记录创建时的阈值
    float maxThreshold;
    
    // 辅助方法
    uint8_t GetVoxel(int x, int y, int z) const {
        return data[z * width * height + y * width + x];
    }
    
    void SetVoxel(int x, int y, int z, uint8_t value) {
        data[z * width * height + y * width + x] = value;
    }
};
```

## 实现步骤

### 阶段1：直方图计算（优先级：高）
1. ✅ 在APR_GetVolumeHistogram中实现CT值统计
2. ✅ 添加N-API包装函数
3. ✅ Vue中调用并显示直方图

### 阶段2：预览Mask（优先级：高）
1. ⏳ 实现APR_UpdatePreviewMask
2. ⏳ 在APR_Render中添加previewMask渲染
3. ⏳ Vue中实时调用updatePreviewMask

### 阶段3：创建Mask（优先级：高）
1. ⏳ 实现APR_CreateMaskFromThreshold
2. ⏳ 添加mask到vector
3. ⏳ Vue中应用创建mask

### 阶段4：Mask渲染优化（优先级：中）
1. ⏳ 优化mask纹理更新（只在需要时）
2. ⏳ 支持多mask叠加
3. ⏳ 添加mask可见性切换

### 阶段5：高级功能（优先级：低）
1. ⏳ 形态学操作（膨胀、腐蚀）
2. ⏳ 布尔操作（并集、交集、差集）
3. ⏳ 反色、连通域分析
4. ⏳ 3D mesh生成

## 性能优化建议

1. **直方图计算**：
   - 只在加载volume时计算一次
   - 缓存直方图数据
   - 使用多线程加速（OpenMP）

2. **Mask渲染**：
   - 使用纹理而不是逐像素绘制
   - 只更新变化的mask
   - 预分配mask纹理内存

3. **预览Mask**：
   - 降低更新频率（防抖：200ms）
   - 只更新当前可见的切片

## 测试检查清单

- [ ] 直方图显示正确（Y轴为log scale）
- [ ] 阈值滑块调节时预览mask实时更新
- [ ] 应用后mask添加到表格
- [ ] Mask在3个正交视图中正确显示
- [ ] Mask颜色和透明度正确
- [ ] 多个mask可以叠加显示
- [ ] 切换mask可见性工作正常
- [ ] 删除mask后渲染正确
- [ ] 性能：实时预览无明显卡顿（<33ms）

## 相关文件

- `ConsoleDllTest/DllVisualization/VisualizationApi.cpp` - C++ API实现
- `ConsoleDllTest/DllVisualization/visualization_wrapper.cpp` - N-API包装
- `hiscan-analyzer/src/components/AnalyzerRoiTab.vue` - Vue UI
- `hiscan-analyzer/electron/preload/visualizationApi.ts` - TypeScript类型定义

## 参考
- ConsoleDllTest/test_mask_edit.cpp - Mask编辑测试示例
- MPR_AddMaskOverlay() - 现有mask overlay API
