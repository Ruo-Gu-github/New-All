# 图像浏览模块 3D 渲染完全指南

## 目录

1. [核心文件概览](#1-核心文件概览)
2. [定位线计算与旋转](#2-定位线计算与旋转)
3. [Slider 交互机制](#3-slider交互机制)
4. [渲染函数列表](#4-渲染函数列表)
5. [鼠标坐标变换](#5-鼠标坐标变换)
6. [3D 正交渲染原理](#6-3d正交渲染原理)
7. [比例尺和方向立方体](#7-比例尺和方向立方体)
8. [测量工具点变换](#8-测量工具点变换)
9. [3D 渲染调试历程与解决方案](#9-3d渲染调试历程与解决方案)
10. [常见问题快速排查](#10-常见问题快速排查)

---

## 1. 核心文件概览

| 文件                                   | 行数   | 用途                                |
| -------------------------------------- | ------ | ----------------------------------- |
| `ImageBrowserOrthogonal3DRenderer.cpp` | ~1650  | 3D 正交三平面渲染器核心             |
| `ImageBrowserOrthogonal3DRenderer.h`   | ~50    | 渲染器类声明                        |
| `VisualizationApi.cpp`                 | ~17500 | 主 API 实现（APR 渲染、窗口管理等） |
| `VisualizationApi.h`                   | ~900   | API 声明                            |

---

## 2. 定位线计算与旋转

### 2.1 关键概念

3D 视图中的定位线需要与 2D APR 切片视图中显示的定位点位置一致。由于 APR 支持旋转，需要进行坐标变换。

**坐标系关系**：

- `real坐标`：体数据的实际体素坐标 (centerX, centerY, centerZ)
- `virtual坐标`：APR 渲染时使用的虚拟坐标（旋转后的显示坐标）
- `归一化坐标`：3D 渲染使用的归一化坐标 [-0.5, 0.5]

### 2.2 virtualCenter 计算公式

```cpp
// 文件：ImageBrowserOrthogonal3DRenderer.cpp，第565-585行

// 获取APR的旋转矩阵
float aprRotMat[16];
APR_GetRotationMatrix(axial, aprRotMat);

// 旋转矩阵元素（列优先存储）
const float r00 = aprRotMat[0];  // R^T 第一行
const float r01 = aprRotMat[4];
const float r02 = aprRotMat[8];
const float r10 = aprRotMat[1];  // R^T 第二行
const float r11 = aprRotMat[5];
const float r12 = aprRotMat[9];
const float r20 = aprRotMat[2];  // R^T 第三行
const float r21 = aprRotMat[6];
const float r22 = aprRotMat[10];

// 旋转中心（体积中心）
const float pivotX = (float)volW * 0.5f;
const float pivotY = (float)volH * 0.5f;
const float pivotZ = (float)volD * 0.5f;

// 相对于旋转中心的偏移
const float dx = centerX - pivotX;
const float dy = centerY - pivotY;
const float dz = centerZ - pivotZ;

// 应用R^T变换：virtual = Pivot + R^T * (real - Pivot)
const float virtualCenterX = pivotX + (r00 * dx + r10 * dy + r20 * dz);
const float virtualCenterY = pivotY + (r01 * dx + r11 * dy + r21 * dz);
const float virtualCenterZ = pivotZ + (r02 * dx + r12 * dy + r22 * dz);
```

### 2.3 归一化到 3D 坐标

```cpp
// 归一化因子
const int maxDim = std::max(volW, std::max(volH, volD));
const float normW = (float)volW / (float)maxDim;
const float normH = (float)volH / (float)maxDim;
const float normD = (float)volD / (float)maxDim;

const float denomW = (float)std::max(1, volW - 1);
const float denomH = (float)std::max(1, volH - 1);
const float denomD = (float)std::max(1, volD - 1);

// 转换到归一化3D坐标（中心为原点）
const float posX = ((virtualCenterX / denomW) - 0.5f) * normW;
const float posY = ((virtualCenterY / denomH) - 0.5f) * normH;
const float posZ = ((virtualCenterZ / denomD) - 0.5f) * normD;
```

### 2.4 定位线绘制

```cpp
// 半尺寸
const float hw = normW * 0.5f;  // 体积宽度的一半
const float hh = normH * 0.5f;  // 体积高度的一半
const float hd = normD * 0.5f;  // 体积深度的一半

glBegin(GL_LINES);
// X轴（红色）- 沿X方向穿过交点
glColor3f(1.0f, 0.0f, 0.0f);
glVertex3f(-hw, posY, posZ);
glVertex3f(hw, posY, posZ);

// Y轴（绿色）
glColor3f(0.0f, 1.0f, 0.0f);
glVertex3f(posX, -hh, posZ);
glVertex3f(posX, hh, posZ);

// Z轴（蓝色）
glColor3f(0.0f, 0.0f, 1.0f);
glVertex3f(posX, posY, -hd);
glVertex3f(posX, posY, hd);
glEnd();
```

---

## 3. Slider 交互机制

### 3.1 没有直接的 Slider 变量

代码中**没有** `sliderX/sliderY` 变量。切片位置通过 `APR_SetCenter()` 控制。

### 3.2 APRContext 结构

```cpp
// 文件：VisualizationApi.cpp

struct APRContext {
    VolumeHandle volume = nullptr;
    float centerX = 0, centerY = 0, centerZ = 0;  // 切片中心点（体素坐标）
    int sliceDirection = 1;  // 0=Axial, 1=Coronal, 2=Sagittal
    float crosshairU = 0, crosshairV = 0;  // 十字线UV坐标
    // ... 其他字段
};
```

### 3.3 APR_SetCenter 函数

```cpp
void APR_SetCenter(APRHandle handle, float x, float y, float z) {
    auto ctx = static_cast<APRContext*>(handle);

    ctx->centerX = x;
    ctx->centerY = y;
    ctx->centerZ = z;

    // 同步crosshair UV坐标（根据切片方向映射）
    if (ctx->sliceDirection == 0) {  // Axial: U=X, V=Y
        ctx->crosshairU = x;
        ctx->crosshairV = y;
    } else if (ctx->sliceDirection == 1) {  // Coronal: U=X, V=Z
        ctx->crosshairU = x;
        ctx->crosshairV = z;
    } else if (ctx->sliceDirection == 2) {  // Sagittal: U=Y, V=Z
        ctx->crosshairU = y;
        ctx->crosshairV = z;
    }

    // 同步关联的APR视图（同一session内）
    // ...
}
```

### 3.4 鼠标滚轮翻页

```cpp
case WM_MOUSEWHEEL:
    const int step = (delta > 0) ? 1 : -1;
    float cx = aprCtx->centerX;
    float cy = aprCtx->centerY;
    float cz = aprCtx->centerZ;

    if (aprCtx->sliceDirection == 0) cz += step;       // Axial: 改变Z
    else if (aprCtx->sliceDirection == 1) cy += step;  // Coronal: 改变Y
    else cx += step;                                    // Sagittal: 改变X

    APR_SetCenter(aprCtx, cx, cy, cz);
```

---

## 4. 渲染函数列表

### 4.1 主渲染入口

| 函数                                         | 文件                                 | 用途                     |
| -------------------------------------------- | ------------------------------------ | ------------------------ |
| `APR_Render()`                               | VisualizationApi.cpp                 | APR 2D 切片渲染          |
| `APR_UpdateSlice()`                          | VisualizationApi.cpp                 | 更新切片数据（CPU 计算） |
| `ImageBrowserOrthogonal3DRenderer::Render()` | ImageBrowserOrthogonal3DRenderer.cpp | 3D 正交三平面渲染        |

### 4.2 辅助渲染函数

| 函数                             | 用途                         |
| -------------------------------- | ---------------------------- |
| `DrawTexturedQuad_ScreenSpace()` | 屏幕空间纹理四边形（调试用） |
| `UploadGray8()`                  | 上传灰度纹理到 GPU           |
| `DrawCropBoxCube()`              | 绘制立方体裁切框             |
| `DrawCropBoxSphere()`            | 绘制球体裁切框               |
| `DrawCropBoxCylinder()`          | 绘制圆柱体裁切框             |
| `DrawScaleBar()`                 | 简易 OpenGL 比例尺（回退）   |

### 4.3 NanoVG 渲染函数

| 函数                          | 用途                     |
| ----------------------------- | ------------------------ |
| `DrawHorizontalScaleBarNVG()` | 水平比例尺（地图样式）   |
| `DrawOrientationCubeNVG()`    | 方向指示立方体           |
| `DrawVerticalScaleBarNVG()`   | 垂直比例尺（APR 视图用） |

---

## 5. 鼠标坐标变换

### 5.0 坐标系与变换总表（按这个写，永远不乱）

这部分是“以后不用你反复解释也能写对”的核心约定：**先明确你现在在哪个坐标系，再用固定的正/逆变换模板**。

#### 5.0.1 坐标系定义（必须先对齐的 4 层）

| 层级 | 名称                    | 符号            | 原点/方向                                               | 单位 | 典型用途                                       |
| ---- | ----------------------- | --------------- | ------------------------------------------------------- | ---- | ---------------------------------------------- |
| A    | 屏幕坐标                | $(s_x, s_y)$    | Windows 消息：左上为原点，$+x$ 右，$+y$ 下              | 像素 | 鼠标消息输入                                   |
| B    | 视口/帧内像素坐标       | $(f_x, f_y)$    | 以当前渲染帧（可能是子区域）左上为原点                  | 像素 | 处理有 padding / viewport / scissor 的 UI 布局 |
| C    | NDC（归一化设备坐标）   | $(n_x, n_y)$    | OpenGL NDC：$[-1,1]$，$+y$ 上                           | 无   | 进入投影/反投影之前的统一坐标                  |
| D    | 体数据体素坐标（real）  | $(x,y,z)$       | 体数据索引空间（中心点、切片索引）                      | 体素 | `APR_SetCenter/GetCenter`、切片位置            |
| E    | APR 虚拟坐标（virtual） | $(x_v,y_v,z_v)$ | “显示空间”的体素坐标（考虑 APR 旋转）                   | 体素 | 让 3D 交点/定位线与 2D APR 显示一致            |
| F    | 3D 归一化体积坐标       | $(p_x,p_y,p_z)$ | 以体积中心为原点，范围约 $[-0.5,0.5]$（按最大维度归一） | 无   | 三张切片摆到一个统一 3D 空间里渲染             |

> 关键点：这套模块里**切片中心（slider/滚轮/同步）永远用 real 体素坐标**（D 层），3D 里用于摆放的交点永远用 (E→F)。

#### 5.0.2 三条铁律（最常见回归点都在这里）

1. **Windows 鼠标 $s_y$ 向下，OpenGL/NDC 的 $n_y$ 向上**：不处理翻转就一定会“上下反”。
2. **APR 旋转相关：real→virtual 用 $R^T$，virtual→real 用 $R$**：写反了就会“旋转方向不对/交点跑偏”。
3. **只要涉及子视口/裁剪（viewport/scissor/padding），必须先算帧内坐标 B，再进 NDC**：否则“鼠标在边缘不准/只在某一块准”。

#### 5.0.3 A→C：屏幕/帧内像素 → NDC（固定模板）

如果当前 3D/2D 渲染区域是一个子矩形（左上角 $(frameLeft, frameTop)$，大小 $(frameW, frameH)$），推荐统一写成：

```cpp
// A -> B
const float fx = (float)screenX - frameLeft;
const float fy = (float)screenY - frameTop;

// B -> C  （注意：fy向下，NDC向上）
const float nx = (fx / frameW) * 2.0f - 1.0f;
const float ny = 1.0f - (fy / frameH) * 2.0f;
```

> 如果你看到代码里是 `ny = (screenY / winHeight) * 2 - 1`，那通常意味着它把屏幕 Y 当成“向上”的了（回归高发）。

#### 5.0.4 D↔E：real 体素 ↔ virtual 体素（固定模板）

约定：`APR_GetRotationMatrix()` 返回 $R$（列优先，OpenGL 风格），表示“virtual→real”的旋转。

- **real→virtual（用于显示对齐）**：

$$
v = P + R^T (r - P)
$$

- **virtual→real（用于设置中心点）**：

$$
r = P + R (v - P)
$$

其中 $P$ 是体积中心（pivot）。代码模板在本说明的“2.2 virtualCenter 计算公式”和“5.2 虚拟坐标 ↔ 体数据坐标”里。

#### 5.0.5 E→F：virtual 体素 → 3D 归一化坐标（固定模板）

这一步决定了三张切片在 3D 里摆放是否一致：

```cpp
const int maxDim = std::max(volW, std::max(volH, volD));
const float normW = (float)volW / (float)maxDim;
const float normH = (float)volH / (float)maxDim;
const float normD = (float)volD / (float)maxDim;

const float denomW = (float)std::max(1, volW - 1);
const float denomH = (float)std::max(1, volH - 1);
const float denomD = (float)std::max(1, volD - 1);

const float px = ((virtualX / denomW) - 0.5f) * normW;
const float py = ((virtualY / denomH) - 0.5f) * normH;
const float pz = ((virtualZ / denomD) - 0.5f) * normD;
```

#### 5.0.6 C→F（可选）：3D 拾取/鼠标射线（以后坏了直接按这个查）

当你需要“鼠标点一下，算出 3D 空间里对应点/射线”，固定流程是：

1. A→C 得到 $(n_x,n_y)$
2. 构造裁剪空间两点：$z=-1$（near）和 $z=+1$（far）
3. 用 $inverse(P\*V)$ 把它们反投影到世界/3D 归一化空间
4. 得到射线：`origin = nearPoint`, `dir = normalize(farPoint - nearPoint)`

这个模块是 fixed pipeline，也可以用 `glGetFloatv(PROJECTION_MATRIX/MODELVIEW_MATRIX)` 配合手动矩阵求逆做（你在 debugStage 日志里能看到两者是否正确）。

---

### 5.1 屏幕 → 纹理 → 图像坐标

```cpp
// 步骤1：屏幕坐标 → NDC
const float ndcX = (screenX / winWidth) * 2.0f - 1.0f;
const float ndcY = (screenY / winHeight) * 2.0f - 1.0f;

// 步骤2：NDC → 相对坐标（考虑视口映射）
const float relX = (ndcX - map.baseLeft) / (map.baseRight - map.baseLeft);
const float relY = (ndcY - map.baseBottom) / (map.baseTop - map.baseBottom);

// 步骤3：相对坐标 → 纹理坐标
const float texU = map.texLeft + relX * (map.texRight - map.texLeft);
const float texV = map.texBottom + relY * (map.texTop - map.texBottom);

// 步骤4：纹理坐标 → 图像坐标
const float imgX = texU * (sliceWidth - 1);
const float imgY = (1.0f - texV) * (sliceHeight - 1);  // Y轴翻转
```

### 5.2 虚拟坐标 ↔ 体数据坐标

```cpp
// 体数据坐标 → 虚拟坐标（用于显示）
// virtual = Pivot + R^T * (real - Pivot)
const float virtualX = pivotX + (r00*dx + r10*dy + r20*dz);
const float virtualY = pivotY + (r01*dx + r11*dy + r21*dz);
const float virtualZ = pivotZ + (r02*dx + r12*dy + r22*dz);

// 虚拟坐标 → 体数据坐标（用于设置center）
// real = Pivot + R * (virtual - Pivot)
const float realX = pivotX + (r00*dvx + r01*dvy + r02*dvz);
const float realY = pivotY + (r10*dvx + r11*dvy + r12*dvz);
const float realZ = pivotZ + (r20*dvx + r21*dvy + r22*dvz);
```

---

## 6. 3D 正交渲染原理

### 6.1 投影设置

```cpp
// 正交投影参数
const float aspect = (float)winWidth / (float)winHeight;
const float zoom = viewZoom > 0.1f ? viewZoom : 1.0f;
const float orthoSize = 0.8f / zoom;  // 基础视野大小

// OpenGL正交投影矩阵
glMatrixMode(GL_PROJECTION);
glLoadIdentity();
glOrtho(-aspect * orthoSize, aspect * orthoSize,  // left, right
        -orthoSize, orthoSize,                     // bottom, top
        -10.0, 10.0);                              // near, far
```

### 6.2 模型视图矩阵

```cpp
glMatrixMode(GL_MODELVIEW);
glLoadIdentity();
glTranslatef(viewPanX * 0.001f, viewPanY * 0.001f, 0.0f);  // 平移
glMultMatrixf(camRot);  // 相机旋转
```

### 6.3 切片绘制

三个切片分别在 XY、XZ、YZ 平面上绘制：

```cpp
// Axial切片 (XY平面, z=posZ)
glBindTexture(GL_TEXTURE_2D, texAxial);
glBegin(GL_QUADS);
glTexCoord2f(0, 1); glVertex3f(-hw, -hh, posZ);
glTexCoord2f(0, 0); glVertex3f(-hw,  hh, posZ);
glTexCoord2f(1, 0); glVertex3f( hw,  hh, posZ);
glTexCoord2f(1, 1); glVertex3f( hw, -hh, posZ);
glEnd();

// Coronal切片 (XZ平面, y=posY)
// Sagittal切片 (YZ平面, x=posX)
// ... 类似
```

---

## 7. 比例尺和方向立方体

### 7.1 3D 视图比例尺计算

```cpp
// 计算每像素对应的物理尺寸（毫米）
// - 正交投影高度为 2*orthoSize
// - 归一化空间1.0对应物理尺寸 maxDim * minSpacing
const float mmPerPixel = (orthoSize * maxDim * minSpacing) / winHeight;
const float effectiveZoom = 1.0f / mmPerPixel;

// 调用水平比例尺
DrawHorizontalScaleBarNVG(winWidth, winHeight, effectiveZoom, volume);
```

### 7.2 方向立方体

```cpp
// 左下角绘制，应用相机旋转矩阵
DrawOrientationCubeNVG(winWidth, winHeight, camRot);

// 内部实现：
// 1. 8个顶点应用camRot变换
// 2. 投影到屏幕空间
// 3. 绘制12条红色边
// 4. 在面向观察者的面上标注A/C/S
```

---

## 8. 测量工具点变换

测量点需要与切片视图同步变换：

```cpp
// 测量点存储为体数据坐标
struct MeasurementPoint {
    float x, y, z;  // 体素坐标
};

// 显示时需要变换到当前视图坐标
// 1. 应用APR旋转变换（如果有）
// 2. 根据切片方向投影到2D
// 3. 应用zoom和pan变换
```

---

## 9. 3D 渲染调试历程与解决方案

### 9.1 问题现象

最初 3D 视图完全不显示任何内容，只有黑色背景。

### 9.2 调试方法：分阶段渲染

创建了 `debugStage` 机制，通过文件控制渲染阶段：

```
文件位置：D:\3d_stage.txt 或 DLL目录下的 3d_stage.txt
内容：一个整数 0-7

阶段说明：
stage 0 = 仅屏幕空间2D贴图（验证纹理数据）
stage 1 = NDC正交投影(-1..1) + MV=I，只画axial在z=0
stage 2 = 仍用NDC投影，但用posX/posY/posZ
stage 3 = 使用3D的glOrtho(aspect*orthoSize)，MV=I
stage 4 = 加pan平移
stage 5 = 加camRot旋转
stage 6 = 启用深度测试，画3张切片
stage 7 = 诊断模式（半透明+无深度）
```

### 9.3 发现的问题

1. **深度缓冲问题**：

   - `glDepthMask(GL_FALSE)` 导致 `glClear(GL_DEPTH_BUFFER_BIT)` 无效
   - 解决：渲染前强制 `glDepthMask(GL_TRUE)`

2. **Scissor 测试残留**：

   - 上游代码留下的 scissor 导致渲染被裁切
   - 解决：保存/恢复 scissor 状态

3. **virtualCenter 计算错误**：

   - 原来使用 `R * (real - C)` 而不是 `R^T * (real - C)`
   - 解决：使用转置矩阵

4. **纹理数据全黑**：
   - `APR_UpdateSlice()` 没有被调用
   - 解决：在 Render 前显式调用

### 9.4 最终工作的代码流程

```
1. 调用 APR_UpdateSlice() 更新三个切片
2. 获取切片数据 APR_GetSlice()
3. 上传纹理 UploadGray8()
4. 设置OpenGL状态：
   - glDepthMask(GL_TRUE)
   - glDisable(GL_SCISSOR_TEST)
   - glClearColor(0,0,0,1)
   - glClear(COLOR | DEPTH)
5. 设置投影矩阵 glOrtho()
6. 设置模型视图矩阵（平移 + 旋转）
7. 计算virtualCenter和posX/Y/Z
8. 绘制三张切片（带深度测试）
9. 绘制定位线（禁用深度测试）
10. 绘制裁切框（如果启用）
11. 绘制比例尺和方向立方体
```

---

## 10. 常见问题快速排查

### 10.1 3D 视图全黑

检查：

1. `D:\3d_render_debug.log` 日志
2. 切片数据是否有效：`axSlice=%p (%dx%d)`
3. 纹理上传是否成功
4. `glDepthMask` 状态

### 10.2 定位线位置不对

检查：

1. virtualCenter 计算是否使用了 R^T
2. posX/posY/posZ 归一化是否正确
3. 与 2D 视图的 crosshair 位置对比

### 10.3 旋转后定位线偏移

关键：确保使用 `R^T * (real - Pivot)` 而不是 `R * (real - Pivot)`

### 10.4 比例尺显示不正确

检查：

1. `mmPerPixel` 计算公式
2. `orthoSize` 与 `zoom` 的关系
3. `maxDim` 和 `minSpacing` 的值

### 10.5 启用调试模式

1. 创建文件 `D:\3d_stage.txt`
2. 写入阶段数字（0-7）
3. 重新触发渲染
4. 查看日志 `D:\3d_render_debug.log`

---

## 附录：调试日志格式

```
========== [3D Render Start] ==========
[3D] Render called: axial=%p coronal=%p sagittal=%p win=800x600
[3D] APR_GetSlice results:
  axSlice=0x12345678 (512x512) valid=1
  coSlice=0x12345680 (512x256) valid=1
  saSlice=0x12345688 (512x256) valid=1
[3D] Uploading textures to OpenGL...
  Uploaded axial texture: 512x512 to GL texture 1
[3D][STAGE] stage=6 slicesDrawn=3
```
