# Mask编辑工具使用说明（2D版本 - 仅MPR）

## 功能概述

新增了基于鼠标的2D Mask编辑工具，支持画笔和橡皮擦功能，可以在MPR视图的当前切片上实时增加或删除mask区域。

**重要说明**：
- ✅ **仅支持 MPR 视图**（Axial、Coronal、Sagittal）
- ✅ **2D 编辑模式**：只修改当前显示的切片，不影响其他层
- ❌ **不支持 APR 视图**

## 工具类型

- **Tool 7: Brush（画笔）** - 在当前切片上增加mask范围
- **Tool 8: Eraser（橡皮擦）** - 在当前切片上删除mask范围

两种工具都使用圆形笔刷，可以调节半径大小。

## 快捷键

### 工具切换
- **7** - 切换到画笔工具（Brush）
- **8** - 切换到橡皮擦工具（Eraser）
- **0-6** - 切换回测量工具或定位线

### 画笔大小调节
- **[** - 减小画笔半径（Shift+[ 微调减小0.5mm，否则粗调减小2mm）
- **]** - 增大画笔半径（Shift+] 微调增加0.5mm，否则粗调增加2mm）
- 半径范围：0.5mm - 50mm

## 使用方法

### 1. 设置当前编辑的Mask

```cpp
// 假设你有一个MaskManager实例和想要编辑的mask索引
MaskManager* maskMgr = ...;
int maskIndex = 0;  // 第一个mask

// 设置当前编辑目标
Mask_SetCurrentMask(maskMgr, maskIndex);
```

### 2. 切换到画笔或橡皮擦工具

按 **7** 键切换到画笔，或按 **8** 键切换到橡皮擦。

控制台会显示：
```
[Tool] Switched to Brush Tool (Mask editing, radius=5.0mm)
```

### 3. 调节画笔大小

- 按 **[** 键减小画笔
- 按 **]** 键增大画笔
- 配合 **Shift** 键可以微调

### 4. 绘制/擦除（仅在MPR视图中）

在 **MPR视图**（Axial/Coronal/Sagittal）中：
- **鼠标左键按下并拖动**：在当前切片上连续绘制/擦除
- **鼠标右键**：取消当前操作
- **切换切片**：不同切片的mask独立编辑

屏幕上会显示一个圆形预览圈，指示当前画笔大小和位置：
- **绿色圈** - 画笔模式
- **红色圈** - 橡皮擦模式

**注意**：在APR视图中，画笔/橡皮擦工具不可用。

## API函数

### Mask_SetCurrentMask
```cpp
void Mask_SetCurrentMask(void* maskManager, int maskIndex);
```
设置当前要编辑的mask。
- `maskManager`: MaskManager实例指针
- `maskIndex`: mask索引（0-based）

### Mask_SetBrushRadius
```cpp
void Mask_SetBrushRadius(float radius);
```
设置画笔/橡皮擦半径（单位：mm）。

### Mask_GetBrushRadius
```cpp
float Mask_GetBrushRadius();
```
获取当前画笔半径。

## 内部实现

### MaskManager新增方法

#### DrawCircle2D
```cpp
void DrawCircle2D(int index, int sliceDir, int sliceIndex, float cx, float cy, float radius);
```
在指定切片的2D平面上，以(cx, cy)为中心、指定半径的圆形区域内设置mask为255。

参数：
- `sliceDir`: 切片方向 (0=Axial/XY, 1=Coronal/XZ, 2=Sagittal/YZ)
- `sliceIndex`: 切片索引（该方向上的层数）
- `cx, cy`: 2D圆心坐标（在切片平面上）
- `radius`: 圆形半径（单位：像素）

#### EraseCircle2D
```cpp
void EraseCircle2D(int index, int sliceDir, int sliceIndex, float cx, float cy, float radius);
```
在指定切片的2D平面上，以(cx, cy)为中心、指定半径的圆形区域内设置mask为0。

### 实现细节

1. **鼠标事件处理**：
   - 在 `OnMouseButton` 中检测tool 7/8的鼠标按下/释放（仅MPR，rendererType==1）
   - 左键按下时设置 `g_isDrawing = true` 并立即在当前位置绘制
   - 左键释放时设置 `g_isDrawing = false`
   - 自动获取当前切片索引和方向

2. **实时绘制**：
   - 在 `OnCursorPos` 中，当 `g_isDrawing = true` 时，持续调用 `DrawCircle2D` 或 `EraseCircle2D`
   - 鼠标移动时自动更新 `g_currentMousePos`（3D世界坐标）
   - 根据 `sliceDirection` 自动提取2D坐标（cx, cy）

3. **可视化预览**：
   - 只在 `MPR_Render` 中绘制圆形预览圈
   - 将画笔半径（mm）转换为2D NDC坐标系的圆形半径
   - 使用不同颜色区分画笔（绿色）和橡皮擦（红色）

4. **坐标转换**：
   - 使用 `MPR_NDCToWorld` 将屏幕坐标转换为3D坐标
   - 根据切片方向提取2D坐标：
     * Axial (sliceDir=0): cx=x, cy=y, sliceIndex=z
     * Coronal (sliceDir=1): cx=x, cy=z, sliceIndex=y
     * Sagittal (sliceDir=2): cx=y, cy=z, sliceIndex=x

5. **2D圆形绘制算法**：
   - 计算圆形边界框避免遍历整个切片
   - 使用距离平方判断点是否在圆内
   - 只修改当前切片，不影响其他层

## 示例代码

```cpp
// 创建或获取MaskManager
MaskManager maskMgr;

// 创建一个mask（假设已有体数据）
int maskIndex = maskMgr.CreateEmpty(width, height, depth, "EditableMask");

// 设置为当前编辑目标
Mask_SetCurrentMask(&maskMgr, maskIndex);

// 设置画笔大小
Mask_SetBrushRadius(10.0f);  // 10mm半径

// 现在用户可以：
// 1. 按7切换到画笔
// 2. 在视图中拖动鼠标添加mask区域
// 3. 按8切换到橡皮擦
// 4. 在视图中拖动鼠标删除mask区域
// 5. 按[或]调节画笔大小
```

## 注意事项

1. **必须先调用 `Mask_SetCurrentMask`**：在使用画笔/橡皮擦工具前，必须设置当前编辑的mask，否则工具不会生效。

2. **仅支持MPR视图**：画笔和橡皮擦工具只在MPR视图中可用（Axial、Coronal、Sagittal）。在APR视图中，这些工具不会响应。

3. **2D编辑模式**：每次绘制只影响当前显示的切片，不会影响其他层。要编辑其他层，需要切换到对应的切片。

4. **实时更新**：每次鼠标移动都会调用 `UpdateStatistics()` 更新mask统计信息。如果性能受影响，可以考虑延迟统计更新。

5. **坐标单位**：画笔半径使用像素为单位（不是mm），在不同spacing的数据上表现一致。

6. **多mask编辑**：可以随时调用 `Mask_SetCurrentMask` 切换编辑不同的mask。

## 未来扩展

可以考虑添加的功能：
- **ROI编辑工具**：矩形、圆形、多边形选区（在当前切片上绘制）
- **连通域选择**：点击选择某个连通区域（2D或3D）
- **Flood Fill**：种子点填充（2D或3D）
- **智能插值**：在关键帧切片上绘制ROI，自动插值中间切片
- **笔刷形状**：方形、椭圆形等
- **绘制撤销/重做**：记录编辑历史
- **平滑边缘**：对绘制的mask进行边缘平滑处理
- **3D模式切换**：可选择切换到3D球形笔刷模式（影响多个切片）
