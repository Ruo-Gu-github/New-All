# Mask保存和加载功能实现总结

## 已完成的功能

### 1. 移除测试数据 ✅
- 将`region2DList`和`region3DList`初始化为空数组
- 添加了TypeScript类型定义，包含`maskId`和`name`字段

### 2. 删除功能增强 ✅
**功能描述：** 删除2D掩膜时同步删除C++ mask vector中的对应项

**实现代码：**
```typescript
async function deleteSelectedRegion2D() {
  if (!selectedRegion2D.value) {
    ElMessage.warning('请先选择要删除的2D区域');
    return;
  }
  
  const index = region2DList.value.indexOf(selectedRegion2D.value);
  if (index === -1) return;
  
  const maskId = selectedRegion2D.value.maskId;
  
  try {
    // 调用C++ API删除mask vector中的项
    if (sessionId.value && maskId !== undefined) {
      await (window as any).visualizationApi?.deleteMask?.(sessionId.value, maskId);
    }
    
    // 从列表中删除
    region2DList.value.splice(index, 1);
    selectedRegion2D.value = null;
    
    ElMessage.success('2D区域删除成功');
  } catch (error) {
    console.error('[ROI] 删除mask失败:', error);
    ElMessage.error('删除mask失败');
  }
}
```

### 3. 保存功能 ✅
**功能描述：** 点击"保存"按钮弹出对话框，输入名称后保存所有mask到图像文件夹下的`masks`目录

**UI实现：**
- 对话框标题："保存Mask"
- 输入框：mask名称（支持回车快捷键）
- 默认名称：`mask_YYYY-MM-DDTHH-mm-ss`（时间戳格式）
- 提示信息：
  - 将保存所有2D掩膜数据
  - 文件将保存在图像文件夹下的`masks`目录
  - 文件格式：`[名称].json`

**保存逻辑：**
```typescript
async function saveMask() {
  if (!maskSaveName.value.trim()) {
    ElMessage.warning('请输入mask名称');
    return;
  }
  
  if (!sessionId.value || !currentFolderPath.value) {
    ElMessage.error('未加载图像数据');
    return;
  }
  
  try {
    // 调用C++ API保存mask到文件夹
    // masks文件夹会在C++侧创建
    const result = await (window as any).visualizationApi?.saveMasks?.(
      sessionId.value,
      currentFolderPath.value,
      maskSaveName.value.trim()
    );
    
    if (result?.success) {
      saveMaskDialogVisible.value = false;
      ElMessage.success(`Mask已保存: ${maskSaveName.value}`);
    } else {
      throw new Error(result?.error || '保存失败');
    }
  } catch (error: any) {
    console.error('[ROI] 保存mask失败:', error);
    ElMessage.error(`保存mask失败: ${error.message}`);
  }
}
```

**保存文件结构：**
```
D:\DicomData\Patient01\
├── dcm files...
└── masks\
    ├── mask_2025-11-11T14-30-00.json
    └── mask_bone_tissue.json
```

### 4. 加载功能 ✅
**功能描述：** 点击"加载"按钮打开文件选择对话框，选择mask文件后加载并添加到表格和C++ vector

**加载逻辑：**
```typescript
async function loadMask() {
  if (!currentFolderPath.value) {
    ElMessage.error('未加载图像数据');
    return;
  }
  
  try {
    // 调用C++ API打开文件选择对话框
    const result = await (window as any).visualizationApi?.loadMasks?.(
      sessionId.value,
      currentFolderPath.value
    );
    
    if (result?.success && result.masks && result.masks.length > 0) {
      // 将加载的masks添加到列表
      for (const mask of result.masks) {
        region2DList.value.push({
          maskId: mask.maskId,
          color: mask.color,
          visible: mask.visible ?? true,
          min: mask.minThreshold,
          max: mask.maxThreshold,
          name: mask.name
        });
      }
      
      ElMessage.success(`已加载 ${result.masks.length} 个mask`);
    } else if (result?.cancelled) {
      // 用户取消选择
      return;
    } else {
      throw new Error(result?.error || '加载失败');
    }
  } catch (error: any) {
    console.error('[ROI] 加载mask失败:', error);
    ElMessage.error(`加载mask失败: ${error.message}`);
  }
}
```

### 5. 文件夹路径管理 ✅
- 在`loadAPRViews()`中保存当前图像文件夹路径
- 用于后续保存mask时确定保存位置

### 6. TypeScript类型定义 ✅

**preload.ts 新增API：**
```typescript
// 删除Mask
deleteMask: (sessionId: string, maskId: number) =>
  ipcRenderer.invoke('viz:delete-mask', sessionId, maskId),

// 保存Masks到文件
saveMasks: (sessionId: string, folderPath: string, maskName: string) =>
  ipcRenderer.invoke('viz:save-masks', sessionId, folderPath, maskName),

// 加载Masks从文件（打开文件选择对话框）
loadMasks: (sessionId: string, folderPath: string) =>
  ipcRenderer.invoke('viz:load-masks', sessionId, folderPath),
```

**electron-env.d.ts 类型定义：**
```typescript
interface Window {
  visualizationApi: {
    // ... 现有API ...
    
    deleteMask: (sessionId: string, maskId: number) => Promise<void>
    
    saveMasks: (sessionId: string, folderPath: string, maskName: string) => Promise<{
      success: boolean
      error?: string
      filePath?: string
    }>
    
    loadMasks: (sessionId: string, folderPath: string) => Promise<{
      success: boolean
      cancelled?: boolean
      error?: string
      masks?: Array<{
        maskId: number
        name: string
        color: string
        visible: boolean
        minThreshold: number
        maxThreshold: number
      }>
    }>
  }
}
```

## 需要在C++端实现的功能

### 重要说明：所有Mask操作都在MPR中，使用MPR_前缀

**架构说明：**
- ROI编辑功能在MPR视图中进行
- 所有mask相关的C++ API应该使用`MPR_`前缀，不是`APR_`
- Mask数据存储在MPRContext中，不是APRContext
- MPR的4个窗口（Axial、Sagittal、Coronal、3D）都需要显示mask叠加层

### 1. MPR_DeleteMask
```cpp
VIZ_API NativeResult MPR_DeleteMask(
    const char* sessionId,
    int maskId
) {
    // 1. 获取MPRContext（不是APRContext！）
    // 2. 从ctx->masks vector中删除指定maskId的项
    // 3. 重新索引后续mask的ID（或保持ID不变）
    // 4. 触发MPR所有4个窗口重绘
}
```

### 2. MPR_SaveMasks
```cpp
VIZ_API NativeResult MPR_SaveMasks(
    const char* sessionId,
    const char* folderPath,
    const char* maskName,
    char* outFilePath,     // 输出：保存的文件路径
    int outFilePathSize
) {
    // 1. 获取MPRContext
    // 2. 创建masks文件夹（如果不存在）
    //    路径：folderPath/masks/
    // 3. 序列化所有ctx->masks到JSON
    //    格式：
    //    {
    //      "version": "1.0",
    //      "volumeSize": {width, height, depth},
    //      "masks": [
    //        {
    //          "name": "Mask_1",
    //          "color": "#ff0000",
    //          "visible": true,
    //          "minThreshold": -200,
    //          "maxThreshold": 200,
    //          "data": "base64编码的mask数据"
    //        }
    //      ]
    //    }
    // 4. 写入文件：folderPath/masks/[maskName].json
    // 5. 返回完整文件路径
}
```

### 3. MPR_LoadMasks
```cpp
VIZ_API NativeResult MPR_LoadMasks(
    const char* sessionId,
    const char* folderPath,
    int* outMaskCount,              // 输出：加载的mask数量
    MaskInfo** outMaskInfos         // 输出：mask信息数组（需要调用者释放）
) {
    // 1. 打开Windows文件选择对话框
    //    初始路径：folderPath/masks/
    //    过滤器：*.json
    // 2. 用户选择文件（可以取消）
    // 3. 解析JSON文件
    // 4. 解码base64数据，还原mask
    // 5. 添加到MPRContext->masks vector
    // 6. 触发MPR所有4个窗口重绘
    // 7. 返回加载的mask信息（用于Vue更新表格）
    
    struct MaskInfo {
        int maskId;
        char name[256];
        char color[16];       // #rrggbb
        bool visible;
        float minThreshold;
        float maxThreshold;
    };
}
```

### 4. MPR_GetVolumeHistogram（已经存在的API）
```cpp
VIZ_API NativeResult MPR_GetVolumeHistogram(
    const char* sessionId,
    int* outData,           // 输出：256个bin的频次数组
    int* outMinValue,       // 输出：CT值最小值
    int* outMaxValue        // 输出：CT值最大值
) {
    // 从MPRContext中获取体数据
    // 计算直方图统计
}
```

### 5. MPR_UpdatePreviewMask（实时预览）
```cpp
VIZ_API NativeResult MPR_UpdatePreviewMask(
    const char* sessionId,
    float minThreshold,
    float maxThreshold,
    const char* hexColor    // #rrggbb格式
) {
    // 1. 获取MPRContext
    // 2. 创建临时预览mask（不添加到vector）
    // 3. 在4个MPR窗口上叠加显示
    // 4. 使用半透明颜色（alpha=0.5）
}
```

### 6. MPR_ClearPreviewMask（取消预览）
```cpp
VIZ_API NativeResult MPR_ClearPreviewMask(
    const char* sessionId
) {
    // 1. 获取MPRContext
    // 2. 释放临时预览mask
    // 3. 重绘MPR窗口（移除预览叠加层）
}
```

### 7. MPR_CreateMaskFromThreshold（应用创建）
```cpp
VIZ_API NativeResult MPR_CreateMaskFromThreshold(
    const char* sessionId,
    float minThreshold,
    float maxThreshold,
    const char* hexColor,
    const char* maskName,
    int* outMaskId          // 输出：新创建的maskId
) {
    // 1. 获取MPRContext
    // 2. 基于阈值创建permanent mask
    // 3. 添加到ctx->masks vector
    // 4. 分配新的maskId
    // 5. 清除预览mask
    // 6. 重绘MPR窗口
    // 7. 返回maskId
}
```

**N-API包装：**
```cpp
// visualization_wrapper.cpp

Napi::Value DeleteMask(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    int maskId = info[1].As<Napi::Number>().Int32Value();
    
    // 调用MPR API，不是APR！
    NativeResult result = MPR_DeleteMask(sessionId.c_str(), maskId);
    
    if (result != NATIVE_OK) {
        Napi::Error::New(env, "Failed to delete mask").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    return env.Undefined();
}

Napi::Value SaveMasks(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    std::string folderPath = info[1].As<Napi::String>().Utf8Value();
    std::string maskName = info[2].As<Napi::String>().Utf8Value();
    
    char filePath[1024] = {0};
    // 调用MPR API，不是APR！
    NativeResult result = MPR_SaveMasks(
        sessionId.c_str(),
        folderPath.c_str(),
        maskName.c_str(),
        filePath,
        sizeof(filePath)
    );
    
    Napi::Object resultObj = Napi::Object::New(env);
    resultObj.Set("success", Napi::Boolean::New(env, result == NATIVE_OK));
    
    if (result == NATIVE_OK) {
        resultObj.Set("filePath", Napi::String::New(env, filePath));
    } else {
        resultObj.Set("error", Napi::String::New(env, "Save failed"));
    }
    
    return resultObj;
}

Napi::Value LoadMasks(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    std::string sessionId = info[0].As<Napi::String>().Utf8Value();
    std::string folderPath = info[1].As<Napi::String>().Utf8Value();
    
    int maskCount = 0;
    MaskInfo* maskInfos = nullptr;
    
    // 调用MPR API，不是APR！
    NativeResult result = MPR_LoadMasks(
        sessionId.c_str(),
        folderPath.c_str(),
        &maskCount,
        &maskInfos
    );
    
    Napi::Object resultObj = Napi::Object::New(env);
    
    if (result == NATIVE_USER_CANCELLED) {
        resultObj.Set("success", Napi::Boolean::New(env, false));
        resultObj.Set("cancelled", Napi::Boolean::New(env, true));
        return resultObj;
    }
    
    if (result != NATIVE_OK) {
        resultObj.Set("success", Napi::Boolean::New(env, false));
        resultObj.Set("error", Napi::String::New(env, "Load failed"));
        return resultObj;
    }
    
    // 转换为JavaScript数组
    Napi::Array masksArray = Napi::Array::New(env, maskCount);
    for (int i = 0; i < maskCount; i++) {
        Napi::Object maskObj = Napi::Object::New(env);
        maskObj.Set("maskId", Napi::Number::New(env, maskInfos[i].maskId));
        maskObj.Set("name", Napi::String::New(env, maskInfos[i].name));
        maskObj.Set("color", Napi::String::New(env, maskInfos[i].color));
        maskObj.Set("visible", Napi::Boolean::New(env, maskInfos[i].visible));
        maskObj.Set("minThreshold", Napi::Number::New(env, maskInfos[i].minThreshold));
        maskObj.Set("maxThreshold", Napi::Number::New(env, maskInfos[i].maxThreshold));
        masksArray[i] = maskObj;
    }
    
    // 释放C++内存
    delete[] maskInfos;
    
    resultObj.Set("success", Napi::Boolean::New(env, true));
    resultObj.Set("masks", masksArray);
    
    return resultObj;
}
```

## 文件序列化格式

**JSON格式（masks/[name].json）：**
```json
{
  "version": "1.0",
  "created": "2025-11-11T14:30:00Z",
  "volumeSize": {
    "width": 512,
    "height": 512,
    "depth": 512
  },
  "masks": [
    {
      "id": 0,
      "name": "Bone Tissue",
      "color": "#ffffff",
      "visible": true,
      "minThreshold": 200,
      "maxThreshold": 3000,
      "data": "base64编码的uint8_t数组（512*512*512字节）",
      "statistics": {
        "volume": 125600,
        "mean": 450.5,
        "stddev": 120.3
      }
    },
    {
      "id": 1,
      "name": "Soft Tissue",
      "color": "#ff0000",
      "visible": true,
      "minThreshold": -100,
      "maxThreshold": 100,
      "data": "..."
    }
  ]
}
```

## 测试检查清单

- [x] 删除测试数据，初始表格为空
- [ ] 创建mask后添加到表格
- [ ] 删除mask时同步删除C++ vector项
- [ ] 点击"保存"打开对话框
- [ ] 输入名称后保存到masks文件夹
- [ ] 保存文件格式正确（JSON）
- [ ] 点击"加载"打开文件选择对话框
- [ ] 选择mask文件后加载到表格
- [ ] 加载的mask在MPR视图中正确显示
- [ ] 用户取消文件选择时不报错
- [ ] 保存/加载错误时显示提示信息

## 相关文件

**已修改：**
- ✅ `hiscan-analyzer/src/components/AnalyzerRoiTab.vue` (1618行)
- ✅ `hiscan-analyzer/electron/preload.ts` (添加3个mask API)
- ✅ `hiscan-analyzer/electron/electron-env.d.ts` (添加类型定义)

**待修改：**
- ⏳ `hiscan-analyzer/electron/main.ts` (添加3个IPC handlers)
- ⏳ `ConsoleDllTest/DllVisualization/VisualizationApi.cpp` (实现C++ API)
- ⏳ `ConsoleDllTest/DllVisualization/visualization_wrapper.cpp` (添加N-API包装)

## 实现优先级

1. **高优先级：** APR_DeleteMask - 删除功能立即可用
2. **高优先级：** APR_SaveMasks - 保存功能，避免数据丢失
3. **高优先级：** APR_LoadMasks - 加载功能，实现工作流完整性
4. **中优先级：** JSON序列化优化（压缩、增量保存）
5. **低优先级：** 自动保存、版本管理

## 性能优化建议

1. **数据压缩：** 使用zlib压缩mask数据（512×512×512 = 128MB原始数据）
2. **增量保存：** 只保存changed masks
3. **异步I/O：** 文件读写使用异步操作，避免阻塞UI
4. **Base64优化：** 考虑使用二进制格式替代Base64（减少33%大小）

## 用户工作流

```
1. 加载DICOM序列
   ↓
2. 切换到ROI编辑页面
   ↓
3. 点击"添加"创建mask（阈值分割）
   ↓
4. 调整阈值，实时预览
   ↓
5. 点击"应用"，mask添加到表格
   ↓
6. 重复3-5创建多个mask
   ↓
7. 点击"保存"，输入名称
   ↓
8. Mask保存到 [图像文件夹]/masks/[名称].json
   ↓
9. 下次加载同一序列时，点击"加载"
   ↓
10. 选择之前保存的mask文件
    ↓
11. Mask加载到表格和视图中
```
