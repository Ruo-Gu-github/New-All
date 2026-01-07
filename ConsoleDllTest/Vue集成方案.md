# Vue + Node.js + C++ DLL 医学影像处理系统 - 集成方案

## 📌 系统架构

```
┌─────────────────────────────────────────────────────────────┐
│                        Vue 前端界面                          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │  MPR Viewer  │  │  3D Renderer │  │  ROI Editor  │      │
│  │   <canvas>   │  │   <canvas>   │  │   <canvas>   │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
                          ↕ (Base64 / SharedArrayBuffer)
┌─────────────────────────────────────────────────────────────┐
│                   Node.js (node-addon-api)                   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Native Module Wrapper (C++ Bindings)                │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
                          ↕ (Function Calls)
┌─────────────────────────────────────────────────────────────┐
│                        C++ DLL 层                            │
│  ┌────────┐  ┌──────────┐  ┌──────────────┐  ┌──────────┐  │
│  │DllCore │  │DllDicom  │  │DllVisualization│  │DllImage│  │
│  │        │  │          │  │                │  │Process │  │
│  │ Memory │  │ GDCM     │  │ OpenGL (FBO)   │  │ Mask   │  │
│  │ Logger │  │ Volume   │  │ MPR/APR/3D     │  │ ROI    │  │
│  │ Thread │  │          │  │ OffscreenRender│  │        │  │
│  └────────┘  └──────────┘  └──────────────┘  └──────────┘  │
└─────────────────────────────────────────────────────────────┘
```

## 🎯 窗口渲染方案对比

### 方案 1: GDI 窗口（❌ 不推荐）
- **适用场景**：纯 2D 图像显示
- **优点**：系统原生，简单
- **缺点**：性能差，无 3D 加速，难以嵌入网页

### 方案 2: 独立 GLFW 窗口（⚠️ 仅测试用）
- **适用场景**：控制台独立测试
- **优点**：性能好，易于调试
- **缺点**：无法嵌入网页，不适合 Vue

### 方案 3: 离屏 OpenGL (FBO) + 像素传递（✅ 推荐）
- **适用场景**：Vue 网页集成
- **优点**：高性能 3D 渲染，灵活的数据传递
- **缺点**：需要额外的数据传递层

## ✅ 推荐实现方案：离屏 OpenGL (FBO)

### 实现流程

```cpp
// 1. C++ DLL 端（离屏渲染）
WindowHandle context = OffscreenContext_Create(512, 512);
MPRHandle mpr = MPR_Create();
MPR_SetVolume(mpr, volume);

// 渲染到 FBO
FrameBuffer* buffer = OffscreenContext_RenderToBuffer(context, mpr, RENDERER_TYPE_MPR);
// buffer->pixels 包含 RGBA 像素数据
// buffer->width, buffer->height
```

```javascript
// 2. Node.js 端（node-addon-api 封装）
const nativeModule = require('./build/Release/medical_image_addon.node');

// 调用 C++ 函数，返回图像数据
const imageData = nativeModule.renderMPR({
  volume: volumeHandle,
  width: 512,
  height: 512,
  sliceIndex: 50
});

// imageData 可以是：
// - Buffer (二进制数据)
// - Base64 字符串
// - SharedArrayBuffer (零拷贝)
```

```vue
<!-- 3. Vue 端（显示图像） -->
<template>
  <div class="mpr-viewer">
    <!-- 方式 A: 使用 img 标签 + Base64 -->
    <img :src="imageDataUrl" />
    
    <!-- 方式 B: 使用 Canvas + ImageData -->
    <canvas ref="mprCanvas" width="512" height="512"></canvas>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue';
const nativeModule = window.require('./medical_image_addon.node');

const imageDataUrl = ref('');
const mprCanvas = ref(null);

function renderMPR() {
  // 调用 C++ 渲染
  const buffer = nativeModule.renderMPR({ /*...*/ });
  
  // 方式 A: Base64
  imageDataUrl.value = `data:image/png;base64,${buffer.toString('base64')}`;
  
  // 方式 B: Canvas
  const ctx = mprCanvas.value.getContext('2d');
  const imageData = new ImageData(
    new Uint8ClampedArray(buffer),
    512, 512
  );
  ctx.putImageData(imageData, 0, 0);
}

onMounted(() => {
  renderMPR();
});
</script>
```

## 📂 测试数据准备

### 1. 创建测试数据文件夹
```
D:/TestData/
├── Dicom/
│   ├── CT_001.dcm          # 单个 DICOM 文件
│   ├── CT_002.dcm
│   └── Series1/            # DICOM 序列文件夹
│       ├── IM_0001.dcm
│       ├── IM_0002.dcm
│       ├── ...
│       └── IM_0100.dcm
└── Output/                 # 输出文件夹
    ├── processed/
    └── masks/
```

### 2. 更新配置文件
编辑 `test_config.json`：
```json
{
  "dicom": {
    "single_file": "D:/TestData/Dicom/CT_001.dcm",
    "series_folder": "D:/TestData/Dicom/Series1",
    "output_folder": "D:/TestData/Output"
  }
}
```

## 🎮 控制台测试命令

### Core 模块
```
core          - 测试 Core 版本和内存统计
memory        - 测试内存分配和释放
log           - 测试日志系统
thread        - 测试线程池（10个任务，4个线程）
timer         - 测试性能计时器
```

### DICOM 模块
```
dicom-read    - 读取单个 DICOM 文件
              → 输入文件路径：D:/TestData/Dicom/CT_001.dcm
              → 显示：Patient Name, Modality, Image Size

dicom-series  - 读取 DICOM 序列
              → 输入文件夹路径：D:/TestData/Dicom/Series1
              → 显示：文件数量

dicom-volume  - 创建 Volume
              → 输入文件夹路径：D:/TestData/Dicom/Series1
              → 显示：Dimensions, Spacing
```

### Visualization 模块
```
viz-window    - 创建 GLFW 窗口（独立测试窗口）
              → 打开 800x600 的 OpenGL 窗口
              → 用于验证 OpenGL 环境

viz-offscreen - 创建离屏上下文（FBO）
              → 创建 512x512 的离屏 OpenGL 上下文
              → 说明 Vue 集成流程
```

### Image Processing 模块
```
mask-create   - 创建 Mask
              → 创建 512x512x100 的 Mask
              → 显示尺寸信息
```

## 🔧 Node.js Addon 开发指南

### 1. 安装依赖
```bash
npm install node-addon-api
npm install --save-dev cmake-js
```

### 2. 创建 Node.js Addon（示例）
```cpp
// medical_image_addon.cpp
#include <napi.h>
#include "DllCore/CoreApi.h"
#include "DllDicom/DicomApi.h"
#include "DllVisualization/VisualizationApi.h"

Napi::Object RenderMPR(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  // 1. 获取参数
  int width = info[0].As<Napi::Number>().Int32Value();
  int height = info[1].As<Napi::Number>().Int32Value();
  
  // 2. 调用 C++ DLL
  WindowHandle context = OffscreenContext_Create(width, height);
  MPRHandle mpr = MPR_Create();
  // ... 设置 Volume
  
  FrameBuffer* buffer = OffscreenContext_RenderToBuffer(context, mpr, 0);
  
  // 3. 转换为 Node.js Buffer
  Napi::Buffer<unsigned char> result = Napi::Buffer<unsigned char>::Copy(
    env, 
    buffer->pixels, 
    buffer->pixelCount * 4  // RGBA
  );
  
  // 4. 清理
  FrameBuffer_Destroy(buffer);
  
  return result;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set("renderMPR", Napi::Function::New(env, RenderMPR));
  return exports;
}

NODE_API_MODULE(medical_image_addon, Init)
```

### 3. 编译 Addon
```bash
npm run build  # 使用 cmake-js
```

## 🚀 运行测试

### 控制台测试
```bash
cd ConsoleDllTest/x64/Debug
ConsoleDllTest.exe

# 输入命令测试
> help
> core
> dicom-read
D:/TestData/Dicom/CT_001.dcm
> viz-offscreen
> exit
```

### Vue 集成测试
```bash
# 1. 创建 Vue 项目
npm create vue@latest medical-image-viewer
cd medical-image-viewer

# 2. 复制编译好的 DLL 和 Node Addon
cp ../ConsoleDllTest/x64/Debug/*.dll ./native/
cp ./build/Release/medical_image_addon.node ./native/

# 3. 在 Vue 中使用
npm run dev
```

## 📊 性能优化建议

### 1. 数据传递优化
- **小图像（< 1MB）**：使用 Base64（简单）
- **大图像（> 1MB）**：使用 SharedArrayBuffer（零拷贝）
- **实时渲染**：使用 WebSocket 流式传输

### 2. 渲染优化
- 使用 FBO 多缓冲（避免阻塞）
- 异步渲染（Worker 线程）
- 按需渲染（只在数据变化时渲染）

### 3. 内存管理
- 及时销毁不用的 Handle
- 使用 Core_GetMemoryStats 监控内存
- 实现对象池（频繁创建销毁的对象）

## 🎯 下一步工作

1. ✅ **完善 DLL 实现**（当前阶段）
   - 实现 DllDicom、DllVisualization、DllImageProcessing
   - 控制台测试验证功能

2. ⏭️ **开发 Node.js Addon**
   - 使用 node-addon-api 封装 C++ 函数
   - 实现数据转换（C++ ↔ JavaScript）

3. ⏭️ **Vue 界面开发**
   - 创建 MPR/3D Viewer 组件
   - 实现交互（鼠标、键盘、ROI 绘制）

4. ⏭️ **分析模块开发**
   - 骨骼、肺部、脂肪分析算法
   - 结果可视化

## 📝 总结

**窗口方案选择：**
- **控制台测试**：使用 GLFW 窗口（`Window_Create`）
- **Vue 集成**：使用离屏 OpenGL（`OffscreenContext_Create`）

**数据流向：**
```
DICOM Files → C++ DLL (Volume) → OpenGL (FBO Render) 
  → Pixel Buffer → Node.js → Base64/Buffer → Vue <canvas>
```

**关键技术：**
- C++：OpenGL FBO（离屏渲染）
- Node.js：node-addon-api（C++ 绑定）
- Vue：Canvas API（显示像素数据）
