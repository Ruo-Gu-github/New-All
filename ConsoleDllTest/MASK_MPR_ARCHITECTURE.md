# Mask功能架构说明 - MPR vs APR

## 核心概念

### ROI编辑在MPR中进行
- **ROI编辑页面** = **MPR视图**
- 所有mask相关操作都在MPR的4个窗口中进行：
  - Axial（轴位）
  - Sagittal（矢状位）
  - Coronal（冠状位）
  - 3D视图

### APR vs MPR 的区别

| 功能 | APR | MPR |
|-----|-----|-----|
| 全称 | Advanced Processing and Reconstruction | Multi-Planar Reconstruction |
| 用途 | 高级处理和3D重建 | 多平面重建和ROI编辑 |
| 视图 | 3D渲染视图 | 4个正交视图（3个2D切面 + 1个3D） |
| Mask操作 | ❌ 不支持 | ✅ 支持 |
| 用户界面 | 独立的3D查看器 | ROI编辑页面 |

## C++ API命名规范

### ✅ 正确的Mask API（MPR前缀）

```cpp
// 直方图获取
MPR_GetVolumeHistogram(sessionId, outData, outMinValue, outMaxValue)

// 预览mask（实时调整阈值时）
MPR_UpdatePreviewMask(sessionId, minThreshold, maxThreshold, hexColor)
MPR_ClearPreviewMask(sessionId)

// 创建permanent mask
MPR_CreateMaskFromThreshold(sessionId, minThreshold, maxThreshold, hexColor, maskName, outMaskId)

// 删除mask
MPR_DeleteMask(sessionId, maskId)

// 保存/加载mask
MPR_SaveMasks(sessionId, folderPath, maskName, outFilePath, outFilePathSize)
MPR_LoadMasks(sessionId, folderPath, outMaskCount, outMaskInfos)
```

### ❌ 错误的命名（不要使用APR前缀）

```cpp
// ❌ 错误！Mask操作不在APR中
APR_DeleteMask(...)
APR_SaveMasks(...)
APR_LoadMasks(...)
APR_CreateMaskFromThreshold(...)
```

## 数据存储结构

### MPRContext 结构（应该包含）

```cpp
struct MPRContext {
    std::string sessionId;
    
    // 体数据
    uint16_t* volumeData;
    int volumeWidth;
    int volumeHeight;
    int volumeDepth;
    
    // 窗口句柄（4个视图）
    HWND hwndAxial;
    HWND hwndSagittal;
    HWND hwndCoronal;
    HWND hwnd3D;
    
    // Mask数据（核心！）
    std::vector<MaskData> masks;
    MaskData* previewMask;  // 临时预览mask
    
    // 直方图缓存
    int histogram[256];
    int histogramMinValue;
    int histogramMaxValue;
    bool histogramCalculated;
};

struct MaskData {
    int id;
    std::string name;
    std::string color;      // #rrggbb格式
    bool visible;
    float minThreshold;
    float maxThreshold;
    uint8_t* data;          // 512×512×512 mask数组
    size_t dataSize;
};
```

### APRContext 结构（不应该包含mask）

```cpp
struct APRContext {
    std::string sessionId;
    
    // 体数据
    uint16_t* volumeData;
    int volumeWidth;
    int volumeHeight;
    int volumeDepth;
    
    // 3D渲染窗口
    HWND hwnd3D;
    
    // 3D渲染参数
    RenderSettings renderSettings;
    
    // ❌ 不要在APR中存储mask！
    // std::vector<MaskData> masks;  // 这是错误的！
};
```

## Vue组件调用流程

### 完整的Mask生命周期

```
1. 用户打开ROI编辑页面
   ↓
   loadAPRViews() → createAPR() → 创建MPR 4个窗口
   ↓
   getVolumeHistogram() → 获取直方图数据
   ↓
   
2. 用户点击"添加"按钮
   ↓
   openRigMarkDialog() → 显示阈值分割对话框
   ↓
   drawHistogram() → 绘制直方图
   ↓
   
3. 用户调整阈值滑块
   ↓
   updateMaskPreview() → MPR_UpdatePreviewMask()
   ↓
   MPR窗口实时显示半透明mask叠加层
   ↓
   
4. 用户点击"应用"
   ↓
   applyRigMark() → MPR_CreateMaskFromThreshold()
   ↓
   MPR_ClearPreviewMask() → 清除预览
   ↓
   Mask添加到MPRContext->masks vector
   ↓
   Mask添加到Vue的region2DList表格
   ↓
   
5. 用户点击"保存"
   ↓
   saveMask() → MPR_SaveMasks()
   ↓
   序列化所有masks到 {folder}/masks/{name}.json
   ↓
   
6. 用户点击"加载"
   ↓
   loadMask() → MPR_LoadMasks()
   ↓
   Windows文件选择对话框
   ↓
   解析JSON文件 → 反序列化masks
   ↓
   添加到MPRContext->masks vector
   ↓
   返回mask数据给Vue → 更新region2DList表格
   ↓
   
7. 用户点击"删除"
   ↓
   deleteSelectedRegion2D() → MPR_DeleteMask()
   ↓
   从MPRContext->masks vector中删除
   ↓
   从region2DList表格中删除
```

## Mask渲染机制

### MPR窗口的WM_PAINT处理

```cpp
LRESULT CALLBACK MPRWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            HDC hdc = BeginPaint(hwnd, &ps);
            
            // 1. 绘制CT切面图像
            DrawCTSlice(hdc, ...);
            
            // 2. 绘制所有可见的permanent masks
            for (const auto& mask : ctx->masks) {
                if (mask.visible) {
                    DrawMaskOverlay(hdc, mask, 0.5f);  // 50% alpha
                }
            }
            
            // 3. 绘制预览mask（如果存在）
            if (ctx->previewMask) {
                DrawMaskOverlay(hdc, *ctx->previewMask, 0.3f);  // 30% alpha
            }
            
            // 4. 绘制十字线和标注
            DrawCrosshair(hdc, ...);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
    }
}
```

### Mask叠加渲染算法

```cpp
void DrawMaskOverlay(HDC hdc, const MaskData& mask, float alpha) {
    // 1. 解析颜色
    int r, g, b;
    ParseHexColor(mask.color, r, g, b);
    
    // 2. 创建半透明画刷
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, (BYTE)(alpha * 255), 0};
    
    // 3. 遍历当前切面的mask数据
    int sliceIndex = GetCurrentSliceIndex();
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int voxelIndex = sliceIndex * width * height + y * width + x;
            if (mask.data[voxelIndex] > 0) {
                // 4. 绘制彩色像素
                HBRUSH brush = CreateSolidBrush(RGB(r, g, b));
                RECT rect = {x, y, x+1, y+1};
                FillRect(hdc, &rect, brush);
                DeleteObject(brush);
            }
        }
    }
}
```

## UI层级问题修复

### 对话框z-index设置

所有对话框都需要添加这两个属性：

```vue
<ElDialog
  v-model="dialogVisible"
  :append-to-body="true"
  :z-index="9999"
  ...
>
```

**说明：**
- `:append-to-body="true"` - 将对话框DOM插入到body最后，避免被父容器遮挡
- `:z-index="9999"` - 设置足够高的z-index，确保在MPR窗口之上

**已修复的对话框：**
- ✅ RigMark对话框（阈值分割）
- ✅ Save Mask对话框（保存mask）
- ✅ Screenshot对话框（截图选择）

### 层级顺序（从底到上）

```
1. MPR窗口（HWND，z-index由操作系统管理）
   ↓
2. Vue页面内容（z-index: 1-1000）
   ↓
3. Element Plus 遮罩层（z-index: 2000+）
   ↓
4. Dialog对话框（z-index: 9999，在最顶层）
```

## 文件结构

### masks文件夹位置

```
D:\DicomData\Patient01\
├── IM-0001-0001.dcm
├── IM-0001-0002.dcm
├── ...
└── masks\                          # MPR_SaveMasks() 创建
    ├── mask_2025-11-11.json       # 保存的mask文件
    └── bone_tissue_01.json
```

### JSON文件格式

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
      "minThreshold": 200.0,
      "maxThreshold": 3000.0,
      "data": "base64_encoded_mask_data..."
    }
  ]
}
```

## 实现检查清单

### C++ DLL实现（待完成）

- [ ] **MPRContext扩展**
  - [ ] 添加 `std::vector<MaskData> masks`
  - [ ] 添加 `MaskData* previewMask`
  - [ ] 添加直方图缓存字段

- [ ] **直方图计算**
  - [ ] `MPR_GetVolumeHistogram()` - 计算256个bin的频次
  - [ ] 缓存结果，避免重复计算

- [ ] **预览mask系统**
  - [ ] `MPR_UpdatePreviewMask()` - 创建临时mask
  - [ ] `MPR_ClearPreviewMask()` - 清除临时mask
  - [ ] 在WM_PAINT中渲染preview mask

- [ ] **Permanent mask创建**
  - [ ] `MPR_CreateMaskFromThreshold()` - 基于阈值创建
  - [ ] 分配maskId
  - [ ] 添加到vector

- [ ] **Mask管理**
  - [ ] `MPR_DeleteMask()` - 从vector删除
  - [ ] 触发4个MPR窗口重绘

- [ ] **文件I/O**
  - [ ] `MPR_SaveMasks()` - JSON序列化
  - [ ] `MPR_LoadMasks()` - JSON反序列化
  - [ ] Windows文件选择对话框集成
  - [ ] Base64编码/解码mask数据

- [ ] **N-API包装**
  - [ ] DeleteMask() → MPR_DeleteMask()
  - [ ] SaveMasks() → MPR_SaveMasks()
  - [ ] LoadMasks() → MPR_LoadMasks()

### Vue前端（已完成）

- [x] ✅ RigMark对话框 z-index 修复
- [x] ✅ Save Mask对话框 z-index 修复
- [x] ✅ Screenshot对话框 z-index 修复
- [x] ✅ 所有IPC调用正确定义
- [x] ✅ TypeScript类型完整

## 总结

1. **核心原则：ROI编辑 = MPR，所有mask操作都在MPR中**
2. **命名规范：使用 MPR_ 前缀，不是 APR_**
3. **数据存储：masks存储在MPRContext，不是APRContext**
4. **UI层级：对话框使用 append-to-body + z-index:9999**
5. **文件位置：{dicomFolder}/masks/{name}.json**
