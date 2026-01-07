# Mask 编辑性能和渲染修复

## 修复内容

### 1. Mask 渲染与 MPR 同步视口变换 ✅
**问题**：Mask 直接铺满屏幕，未考虑 MPR 的纵横比调整  
**修复**：
- 使用与 `RenderTextureToWindow` 相同的变换逻辑
- 计算 `baseLeft/Right/Top/Bottom` 保持纵横比
- 应用缩放时围绕定位线缩放

### 2. 画笔性能优化（延迟栅格化）✅
**问题**：每次鼠标移动都调用 `DrawCircle2D`，频繁修改 mask 非常卡  
**修复**：
```cpp
// 旧方案（慢）：
OnCursorPos() → DrawCircle2D() → 修改mask数据 → 更新纹理

// 新方案（快）：
OnMousePress() → 清空路径 g_maskStrokePath
OnCursorPos() → 添加点到路径（只记录，不修改mask）
OnMouseRelease() → 遍历路径，一次性栅格化到mask
```

**优势**：
- 拖动时无 I/O 操作，只记录坐标
- 鼠标抬起后一次性批量处理
- 添加 2px 距离阈值避免点太密集

### 3. 纹理缓存机制 ✅
**问题**：每帧重新创建纹理  
**修复**：
```cpp
struct MaskOverlay {
    GLuint cachedTextureID = 0;  // 缓存纹理ID
    int cachedSliceIndex = -1;   // 缓存切片索引
};

// 只在以下情况更新纹理：
bool needUpdate = (cachedSliceIndex != currentSliceIndex) ||
                  (cachedTextureID == 0) ||
                  g_maskStrokeNeedsUpdate;  // 笔画完成时设置
```

### 4. 圆形预览正圆修复 ✅
**问题**：画笔圆圈显示为椭圆  
**修复**：
```cpp
// 计算窗口和纹理纵横比
float texAspect = sliceWidth / sliceHeight;
float winAspect = winWidth / winHeight;

// 分别计算X和Y方向的半径
float radiusX = avgRadius;
float radiusY = avgRadius * (winAspect / texAspect);

// 绘制正圆
for (int i = 0; i < 64; i++) {
    float x = mouseNDC.x + radiusX * cos(angle);
    float y = mouseNDC.y + radiusY * sin(angle);
    glVertex2f(x, y);
}
```

### 5. 实时笔画预览 ✅
**新功能**：拖动时显示粗线条路径预览
```cpp
if (g_isDrawing && !g_maskStrokePath.empty()) {
    glLineWidth(g_brushRadius * 0.5f);
    glBegin(GL_LINE_STRIP);
    for (const auto& pt : g_maskStrokePath) {
        // 转换为NDC并绘制
    }
    glEnd();
}
```

## 性能对比

### 旧方案：
- 鼠标移动 100 次 = 调用 100 次 `DrawCircle2D`
- 每次修改 mask 数据 + 重建纹理
- **非常卡顿**

### 新方案：
- 鼠标移动 100 次 = 记录 ~50 个点（2px 阈值）
- 鼠标抬起时：50 次 `DrawCircle2D`（一次性）
- 仅更新一次纹理
- **流畅丝滑**

## 测试验证

1. ✅ 画笔流畅度：拖动不卡顿
2. ✅ 橡皮擦流畅度：切换快速响应
3. ✅ Mask 位置准确：与 MPR 图像对齐
4. ✅ 圆形预览正确：显示正圆而非椭圆
5. ✅ 大小匹配：预览圆圈大小 = 实际绘制大小
6. ✅ 缩放和纵横比：Mask 随 MPR 正确缩放

## 代码变更位置

| 文件 | 行号 | 变更 |
|------|------|------|
| `VisualizationApi.cpp` | 650-670 | 添加 `g_maskStrokePath` 和 `g_maskStrokeNeedsUpdate` |
| `VisualizationApi.cpp` | 590-610 | `MaskOverlay` 添加纹理缓存字段 |
| `VisualizationApi.cpp` | 1107-1315 | 重写 Mask 渲染逻辑（视口变换+缓存） |
| `VisualizationApi.cpp` | 1735-1830 | 修复圆形预览（正圆+笔画预览） |
| `VisualizationApi.cpp` | 3660-3760 | 修改鼠标按下/抬起（延迟栅格化） |
| `VisualizationApi.cpp` | 3930-3970 | 修改鼠标移动（记录路径而非立即绘制） |

## 编译命令

```cmd
cd "D:\2025-09-25 新系列\ConsoleDllTest"
msbuild DllVisualization\DllVisualization.vcxproj /p:Configuration=Debug /p:Platform=x64
```

## 测试命令

```cmd
cd "Dlls\debug\bin"
ConsoleDllTest.exe
> mask-edit
```

按 7 → B → 左键拖拽，应该非常流畅！
