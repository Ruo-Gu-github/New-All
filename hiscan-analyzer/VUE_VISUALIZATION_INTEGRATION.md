# Vue 集成 Visualization.dll 完整方案

## 📋 概述

基于 ConsoleDllTest 的实现，将 APR/MPR 可视化功能集成到 Electron + Vue 应用中。

## 🏗️ 架构设计

```
┌─────────────────────────────────────────────────────────┐
│ Vue 组件 (AnalyzerViewerTab.vue / AnalyzerRoiTab.vue)  │
│  - 提供 UI 控件（滑块、按钮）                             │
│  - 显示 4 个 Canvas 视图                                 │
└────────────────┬────────────────────────────────────────┘
                 │
┌────────────────▼────────────────────────────────────────┐
│ Electron Preload API (window.visualizationApi)         │
│  - 封装 IPC 调用                                         │
└────────────────┬────────────────────────────────────────┘
                 │
┌────────────────▼────────────────────────────────────────┐
│ Electron Main Process (main.ts)                        │
│  - 加载 native addon                                     │
│  - 处理 IPC 请求                                         │
│  - 调用 Visualization.dll API                           │
└────────────────┬────────────────────────────────────────┘
                 │
┌────────────────▼────────────────────────────────────────┐
│ Native Addon (visualization_wrapper.cpp)               │
│  - N-API 封装                                            │
│  - 调用 VisualizationApi.h 的 C++ 接口                 │
└────────────────┬────────────────────────────────────────┘
                 │
┌────────────────▼────────────────────────────────────────┐
│ DllVisualization.dll                                    │
│  - APR/MPR 渲染                                          │
│  - 离屏渲染 (OffscreenContext)                           │
│  - 测量工具 (ToolManager)                                │
└─────────────────────────────────────────────────────────┘
```

## 🔑 核心 API 接口

### 1. APR (任意平面重建) - 用于图像浏览

```cpp
// 创建 APR 渲染器（每个视图一个）
APRHandle APR_Create();

// 设置体数据（从文件夹加载）
APR_SetVolume(APRHandle handle, VolumeHandle volume);

// 设置切片方向（0=Axial, 1=Coronal, 2=Sagittal）
APR_SetSliceDirection(APRHandle handle, int direction);

// 设置中心点（进度条控制）
APR_SetCenter(APRHandle handle, float x, float y, float z);

// 设置旋转（3个旋转滑块）
APR_SetRotation(APRHandle handle, float angleX, float angleY, float angleZ);

// 显示/隐藏定位线
APR_SetShowCrossHair(APRHandle handle, bool show);

// 获取切片像素数据（渲染到 Canvas）
void* APR_GetSlice(APRHandle handle, int direction, int* width, int* height);

// 链接多个 APR（同步中心点）
APR_LinkCenter(APRHandle* handles, 3);
```

### 2. MPR (多平面重建) - 用于 ROI 编辑

```cpp
// 创建 MPR 渲染器
MPRHandle MPR_Create();

// 设置体数据
MPR_SetVolume(MPRHandle handle, VolumeHandle volume);

// 设置切片方向
MPR_SetSliceDirection(MPRHandle handle, MPRSliceDirection direction);

// 添加 Mask（ROI 叠加显示）
MPR_AddMask(MPRHandle handle, MaskManagerHandle maskManager, int maskIndex);

// 设置 Mask 颜色和透明度
MPR_SetMaskColor(MPRHandle handle, int maskIndex, float r, float g, float b, float a);
MPR_SetMaskOpacity(MPRHandle handle, int maskIndex, float opacity);

// 显示/隐藏 Mask
MPR_SetMaskVisible(MPRHandle handle, int maskIndex, bool visible);

// 获取切片
void* MPR_GetSlice(MPRHandle handle, int direction, int* width, int* height);
```

### 3. 离屏渲染（Web 集成关键）

```cpp
// 创建离屏上下文（不创建窗口）
WindowHandle OffscreenContext_Create(int width, int height);

// 渲染到 FBO 并获取像素数据（RGBA）
FrameBuffer* OffscreenContext_RenderToBuffer(
    WindowHandle handle, 
    void* rendererHandle,  // APRHandle 或 MPRHandle
    int rendererType       // 0=APR, 1=MPR
);

// 像素数据结构
typedef struct {
    int width;
    int height;
    unsigned char* data;  // RGBA 格式
} FrameBuffer;
```

### 4. 测量工具

```cpp
// 创建工具管理器
ToolManagerHandle ToolManager_Create();

// 创建测量工具
ToolHandle Tool_CreateLine(ToolManagerHandle mgr);     // 1: 直线测距
ToolHandle Tool_CreateAngle(ToolManagerHandle mgr);    // 2: 角度测量
ToolHandle Tool_CreateRect(ToolManagerHandle mgr);     // 3: 矩形
ToolHandle Tool_CreateCircle(ToolManagerHandle mgr);   // 4: 圆形
ToolHandle Tool_CreateSpline(ToolManagerHandle mgr);   // 5: 样条曲线
ToolHandle Tool_CreateFreehand(ToolManagerHandle mgr); // 6: 自由曲线

// 设置激活工具
Window_SetActiveTool(WindowHandle window, ToolHandle tool);
```

## 📦 实现步骤

### Step 1: 创建 Native Addon Wrapper

在 `native/console-dll/src/visualization_wrapper.cpp`:

```cpp
#include "visualization_wrapper.h"
#include "VisualizationApi.h"
#include <node_api.h>

// 创建 APR 渲染器（返回 4 个句柄：轴向、冠状、矢状、3D）
Napi::Value CreateAPRViews(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  // 参数：folderPath (string)
  std::string folderPath = info[0].As<Napi::String>().Utf8Value();
  
  // 1. 加载 DICOM 体数据
  VolumeHandle volume = Dicom_Volume_Create();
  Dicom_Volume_LoadFolder(volume, folderPath.c_str());
  
  // 2. 创建 4 个 APR 渲染器
  APRHandle aprAxial = APR_Create();
  APRHandle aprCoronal = APR_Create();
  APRHandle aprSagittal = APR_Create();
  APRHandle apr3D = APR_Create();
  
  // 3. 设置体数据
  APR_SetVolume(aprAxial, volume);
  APR_SetVolume(aprCoronal, volume);
  APR_SetVolume(aprSagittal, volume);
  APR_SetVolume(apr3D, volume);
  
  // 4. 设置切片方向
  APR_SetSliceDirection(aprAxial, 0);    // Axial
  APR_SetSliceDirection(aprCoronal, 1);  // Coronal
  APR_SetSliceDirection(aprSagittal, 2); // Sagittal
  
  // 5. 获取尺寸并设置中心点
  int width, height, depth;
  Dicom_Volume_GetDimensions(volume, &width, &height, &depth);
  
  float cx = width / 2.0f, cy = height / 2.0f, cz = depth / 2.0f;
  APR_SetCenter(aprAxial, cx, cy, cz);
  APR_SetCenter(aprCoronal, cx, cy, cz);
  APR_SetCenter(aprSagittal, cx, cy, cz);
  APR_SetCenter(apr3D, cx, cy, cz);
  
  // 6. 链接中心点（同步）
  APRHandle aprs[] = { aprAxial, aprCoronal, aprSagittal, apr3D };
  APR_LinkCenter(aprs, 4);
  
  // 7. 返回句柄（保存在 JS 对象中）
  Napi::Object result = Napi::Object::New(env);
  result.Set("axial", Napi::External<void>::New(env, aprAxial));
  result.Set("coronal", Napi::External<void>::New(env, aprCoronal));
  result.Set("sagittal", Napi::External<void>::New(env, aprSagittal));
  result.Set("volume3d", Napi::External<void>::New(env, apr3D));
  result.Set("volume", Napi::External<void>::New(env, volume));
  result.Set("width", Napi::Number::New(env, width));
  result.Set("height", Napi::Number::New(env, height));
  result.Set("depth", Napi::Number::New(env, depth));
  
  return result;
}

// 渲染切片到 Canvas（返回像素数据）
Napi::Value RenderAPRSlice(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  // 参数：aprHandle (External), canvasWidth, canvasHeight
  APRHandle apr = info[0].As<Napi::External<void>>().Data();
  int canvasWidth = info[1].As<Napi::Number>().Int32Value();
  int canvasHeight = info[2].As<Napi::Number>().Int32Value();
  
  // 离屏渲染
  WindowHandle offscreen = OffscreenContext_Create(canvasWidth, canvasHeight);
  FrameBuffer* fb = OffscreenContext_RenderToBuffer(offscreen, apr, 0); // 0=APR
  
  // 转换为 Napi::Buffer
  Napi::Buffer<unsigned char> buffer = Napi::Buffer<unsigned char>::Copy(
    env, fb->data, fb->width * fb->height * 4
  );
  
  // 清理
  FrameBuffer_Destroy(fb);
  OffscreenContext_Destroy(offscreen);
  
  return buffer;
}

// 更新 APR 中心点
Napi::Value UpdateAPRCenter(const Napi::CallbackInfo& info) {
  APRHandle apr = info[0].As<Napi::External<void>>().Data();
  float x = info[1].As<Napi::Number>().FloatValue();
  float y = info[2].As<Napi::Number>().FloatValue();
  float z = info[3].As<Napi::Number>().FloatValue();
  
  APR_SetCenter(apr, x, y, z);
  return info.Env().Undefined();
}

// 更新 APR 旋转
Napi::Value UpdateAPRRotation(const Napi::CallbackInfo& info) {
  APRHandle apr = info[0].As<Napi::External<void>>().Data();
  float angleX = info[1].As<Napi::Number>().FloatValue();
  float angleY = info[2].As<Napi::Number>().FloatValue();
  float angleZ = info[3].As<Napi::Number>().FloatValue();
  
  APR_SetRotation(apr, angleX, angleY, angleZ);
  return info.Env().Undefined();
}

// 导出函数
void InitVisualizationModule(Napi::Env env, Napi::Object& exports) {
  exports.Set("createAPRViews", Napi::Function::New(env, CreateAPRViews));
  exports.Set("renderAPRSlice", Napi::Function::New(env, RenderAPRSlice));
  exports.Set("updateAPRCenter", Napi::Function::New(env, UpdateAPRCenter));
  exports.Set("updateAPRRotation", Napi::Function::New(env, UpdateAPRRotation));
}
```

### Step 2: Electron Main Process IPC 处理

在 `electron/main.ts`:

```typescript
import { ipcMain } from 'electron';

// 加载 native addon
const visualizationAddon = require('../native/console-dll/build/Release/console_dll_addon.node');

// IPC: 创建 APR 视图
ipcMain.handle('viz:create-apr', async (_event, folderPath: string) => {
  try {
    const result = visualizationAddon.createAPRViews(folderPath);
    // 保存句柄到全局（供后续使用）
    global.aprViews = result;
    return {
      success: true,
      width: result.width,
      height: result.height,
      depth: result.depth
    };
  } catch (error) {
    return { success: false, error: error.message };
  }
});

// IPC: 渲染切片
ipcMain.handle('viz:render-apr-slice', async (_event, view: string, width: number, height: number) => {
  try {
    const aprHandle = global.aprViews[view]; // 'axial', 'coronal', 'sagittal', 'volume3d'
    const pixels = visualizationAddon.renderAPRSlice(aprHandle, width, height);
    return { success: true, pixels };
  } catch (error) {
    return { success: false, error: error.message };
  }
});

// IPC: 更新中心点
ipcMain.handle('viz:update-center', async (_event, x: number, y: number, z: number) => {
  visualizationAddon.updateAPRCenter(global.aprViews.axial, x, y, z);
  return { success: true };
});

// IPC: 更新旋转
ipcMain.handle('viz:update-rotation', async (_event, angleX: number, angleY: number, angleZ: number) => {
  visualizationAddon.updateAPRRotation(global.aprViews.axial, angleX, angleY, angleZ);
  return { success: true };
});
```

### Step 3: Preload API 封装

在 `electron/preload.ts`:

```typescript
contextBridge.exposeInMainWorld('visualizationApi', {
  createAPR: (folderPath: string) => ipcRenderer.invoke('viz:create-apr', folderPath),
  renderSlice: (view: string, width: number, height: number) => 
    ipcRenderer.invoke('viz:render-apr-slice', view, width, height),
  updateCenter: (x: number, y: number, z: number) => 
    ipcRenderer.invoke('viz:update-center', x, y, z),
  updateRotation: (angleX: number, angleY: number, angleZ: number) => 
    ipcRenderer.invoke('viz:update-rotation', angleX, angleY, angleZ),
});
```

### Step 4: Vue 组件实现

在 `AnalyzerViewerTab.vue`:

```vue
<template>
  <div class="viewer-container">
    <div class="view-grid">
      <canvas ref="view1" class="view-canvas" @mousedown="onViewClick('axial')"></canvas>
      <canvas ref="view2" class="view-canvas" @mousedown="onViewClick('coronal')"></canvas>
      <canvas ref="view3" class="view-canvas" @mousedown="onViewClick('sagittal')"></canvas>
      <canvas ref="view4" class="view-canvas" @mousedown="onViewClick('volume3d')"></canvas>
    </div>
    
    <div class="controls">
      <el-slider v-model="centerX" :min="0" :max="volumeWidth" @change="onCenterChange" label="X" />
      <el-slider v-model="centerY" :min="0" :max="volumeHeight" @change="onCenterChange" label="Y" />
      <el-slider v-model="centerZ" :min="0" :max="volumeDepth" @change="onCenterChange" label="Z" />
      
      <el-slider v-model="rotateX" :min="-180" :max="180" @change="onRotationChange" label="旋转X" />
      <el-slider v-model="rotateY" :min="-180" :max="180" @change="onRotationChange" label="旋转Y" />
      <el-slider v-model="rotateZ" :min="-180" :max="180" @change="onRotationChange" label="旋转Z" />
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, watch, onMounted } from 'vue';

const props = defineProps<{ panelData: any }>();

const view1 = ref<HTMLCanvasElement>();
const view2 = ref<HTMLCanvasElement>();
const view3 = ref<HTMLCanvasElement>();
const view4 = ref<HTMLCanvasElement>();

const volumeWidth = ref(512);
const volumeHeight = ref(512);
const volumeDepth = ref(512);

const centerX = ref(256);
const centerY = ref(256);
const centerZ = ref(256);

const rotateX = ref(0);
const rotateY = ref(0);
const rotateZ = ref(0);

// 加载 APR 视图
async function loadAPR(folderPath: string) {
  const result = await window.visualizationApi.createAPR(folderPath);
  if (result.success) {
    volumeWidth.value = result.width;
    volumeHeight.value = result.height;
    volumeDepth.value = result.depth;
    
    centerX.value = result.width / 2;
    centerY.value = result.height / 2;
    centerZ.value = result.depth / 2;
    
    // 渲染 4 个视图
    await renderAllViews();
  }
}

// 渲染所有视图
async function renderAllViews() {
  await renderView('axial', view1.value);
  await renderView('coronal', view2.value);
  await renderView('sagittal', view3.value);
  await renderView('volume3d', view4.value);
}

// 渲染单个视图
async function renderView(viewName: string, canvas: HTMLCanvasElement) {
  const ctx = canvas.getContext('2d');
  const result = await window.visualizationApi.renderSlice(viewName, canvas.width, canvas.height);
  
  if (result.success) {
    const imageData = new ImageData(
      new Uint8ClampedArray(result.pixels),
      canvas.width,
      canvas.height
    );
    ctx.putImageData(imageData, 0, 0);
  }
}

// 中心点变化
async function onCenterChange() {
  await window.visualizationApi.updateCenter(centerX.value, centerY.value, centerZ.value);
  await renderAllViews();
}

// 旋转变化
async function onRotationChange() {
  await window.visualizationApi.updateRotation(rotateX.value, rotateY.value, rotateZ.value);
  await renderAllViews();
}

watch(() => props.panelData, (newData) => {
  if (newData?.folderPath) {
    loadAPR(newData.folderPath);
  }
}, { immediate: true });
</script>

<style scoped>
.view-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  grid-template-rows: 1fr 1fr;
  gap: 8px;
}

.view-canvas {
  width: 100%;
  height: 100%;
  background: #000;
  border: 1px solid #0bcd94;
}
</style>
```

## 🎯 测量工具集成

参考 ConsoleDllTest 的按键绑定：

```vue
<el-button @click="setTool(1)">直线测距</el-button>
<el-button @click="setTool(2)">角度测量</el-button>
<el-button @click="setTool(3)">矩形</el-button>
<el-button @click="setTool(4)">圆形</el-button>
<el-button @click="setTool(5)">样条曲线</el-button>
<el-button @click="setTool(6)">自由曲线</el-button>

<script>
async function setTool(toolType: number) {
  await window.visualizationApi.setActiveTool(toolType);
}
</script>
```

## ✅ 总结

1. **APR 用于图像浏览**：4 个视图（轴向、冠状、矢状、3D），支持旋转和中心点调整
2. **MPR 用于 ROI 编辑**：支持 Mask 叠加显示，颜色/透明度可调
3. **离屏渲染**：无需创建窗口，直接渲染到 Canvas
4. **测量工具**：6 种工具可绑定到 Vue 按钮

下一步：创建 `visualization_wrapper.cpp` 并编译为 native addon。
