# Node Addon集成示例

本文档展示如何使用node-addon-api封装BridgeAPI，实现JavaScript调用C++ DLL。

---

## 1. 项目结构

```
ElectronApp/
├── package.json
├── main.js                    # Electron主进程
├── renderer.js                # Electron渲染进程
├── index.html                 # UI页面
├── native/
│   ├── binding.cpp            # N-API封装代码
│   ├── binding.gyp            # node-gyp构建配置
│   └── NativeHostBridge.dll   # 复制的Bridge DLL
└── build/
    └── Release/
        └── native.node        # 编译后的Node Addon
```

---

## 2. binding.gyp配置

```python
{
    "targets": [
        {
            "target_name": "native",
            "sources": [ "binding.cpp" ],
            "include_dirs": [
                "<!@(node -p \"require('node-addon-api').include\")",
                "../Common"  # 包含BridgeAPI.h
            ],
            "libraries": [
                "../NativeHostBridge.lib"  # 链接Bridge DLL
            ],
            "defines": [ "NAPI_DISABLE_CPP_EXCEPTIONS" ],
            "cflags!": [ "-fno-exceptions" ],
            "cflags_cc!": [ "-fno-exceptions" ],
            "msvs_settings": {
                "VCCLCompilerTool": {
                    "ExceptionHandling": 1
                }
            }
        }
    ]
}
```

---

## 3. binding.cpp - N-API封装

```cpp
#include <napi.h>
#include "../Common/BridgeAPI.h"
#include <string>
#include <memory>

// ============================================================================
// 辅助函数
// ============================================================================

// 从JavaScript Buffer创建Uint8Array视图
Napi::Uint8Array CreateBufferFromPixels(Napi::Env env, const FrameBuffer* fb) {
    if (!fb || !fb->pixels) {
        return Napi::Uint8Array::New(env, 0);
    }
    
    size_t size = fb->width * fb->height * fb->channels;
    auto buffer = Napi::Buffer<uint8_t>::Copy(env, fb->pixels, size);
    return Napi::Uint8Array::New(env, size, buffer, 0);
}

// ============================================================================
// 窗口管理API
// ============================================================================

Napi::Value CreateWindow(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    // 参数校验
    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "Expected (width: number, height: number, parentHWND?: number)")
            .ThrowAsJavaScriptException();
        return env.Null();
    }
    
    int width = info[0].As<Napi::Number>().Int32Value();
    int height = info[1].As<Napi::Number>().Int32Value();
    void* parentHWND = nullptr;
    
    if (info.Length() >= 3 && info[2].IsNumber()) {
        int64_t hwnd = info[2].As<Napi::Number>().Int64Value();
        parentHWND = reinterpret_cast<void*>(hwnd);
    }
    
    // 调用Bridge API
    void* windowHandle = Bridge_CreateWindow(width, height, parentHWND);
    
    if (!windowHandle) {
        Napi::Error::New(env, "Failed to create window").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    // 返回句柄（作为外部指针）
    return Napi::External<void>::New(env, windowHandle);
}

Napi::Value DestroyWindow(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsExternal()) {
        Napi::TypeError::New(env, "Expected window handle").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    void* windowHandle = info[0].As<Napi::External<void>>().Data();
    Bridge_DestroyWindow(windowHandle);
    
    return env.Undefined();
}

Napi::Value ProcessMessages(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsExternal()) {
        Napi::TypeError::New(env, "Expected window handle").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
    
    void* windowHandle = info[0].As<Napi::External<void>>().Data();
    bool result = Bridge_ProcessMessages(windowHandle);
    
    return Napi::Boolean::New(env, result);
}

// ============================================================================
// DICOM加载API
// ============================================================================

// 进度回调包装
class ProgressCallback {
public:
    ProgressCallback(Napi::Env env, Napi::Function callback) 
        : m_env(env), m_callback(Napi::Persistent(callback)) {}
    
    void Call(double progress, const std::string& message) {
        m_callback.Call({
            Napi::Number::New(m_env, progress),
            Napi::String::New(m_env, message)
        });
    }
    
private:
    Napi::Env m_env;
    Napi::FunctionReference m_callback;
};

Napi::Value LoadDicomSeries(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected (path: string, callback?: function)")
            .ThrowAsJavaScriptException();
        return env.Null();
    }
    
    std::string path = info[0].As<Napi::String>().Utf8Value();
    
    // 可选的进度回调
    ::ProgressCallback bridgeCallback = nullptr;
    std::unique_ptr<ProgressCallback> jsCallback;
    
    if (info.Length() >= 2 && info[1].IsFunction()) {
        jsCallback = std::make_unique<ProgressCallback>(env, info[1].As<Napi::Function>());
        bridgeCallback = [](double progress, const char* message, void* userData) {
            auto* callback = static_cast<ProgressCallback*>(userData);
            callback->Call(progress, message);
        };
    }
    
    // 调用Bridge API
    VolumeHandle volume = Bridge_LoadDicomSeries(
        path.c_str(), 
        bridgeCallback ? [=](double p, const char* m) { bridgeCallback(p, m, jsCallback.get()); } : nullptr
    );
    
    if (!volume) {
        Napi::Error::New(env, "Failed to load DICOM series").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    return Napi::External<void>::New(env, volume);
}

Napi::Value GetVolumeInfo(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsExternal()) {
        Napi::TypeError::New(env, "Expected volume handle").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    VolumeHandle volume = info[0].As<Napi::External<void>>().Data();
    const char* jsonStr = Bridge_GetVolumeInfo(volume);
    
    // 解析JSON并返回JavaScript对象
    Napi::Object result = env.Global().Get("JSON").As<Napi::Object>()
        .Get("parse").As<Napi::Function>()
        .Call({Napi::String::New(env, jsonStr)}).As<Napi::Object>();
    
    return result;
}

// ============================================================================
// MPR渲染API
// ============================================================================

Napi::Value CreateMPRRenderer(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2 || !info[0].IsExternal() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "Expected (volume: handle, direction: number)")
            .ThrowAsJavaScriptException();
        return env.Null();
    }
    
    VolumeHandle volume = info[0].As<Napi::External<void>>().Data();
    int direction = info[1].As<Napi::Number>().Int32Value();
    
    MPRHandle mpr = Bridge_CreateMPRRenderer(volume, direction);
    
    if (!mpr) {
        Napi::Error::New(env, "Failed to create MPR renderer").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    return Napi::External<void>::New(env, mpr);
}

Napi::Value RenderMPR(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsExternal()) {
        Napi::TypeError::New(env, "Expected MPR handle").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    MPRHandle mpr = info[0].As<Napi::External<void>>().Data();
    FrameBuffer* fb = Bridge_RenderMPR(mpr);
    
    if (!fb) {
        return env.Null();
    }
    
    // 创建返回对象
    Napi::Object result = Napi::Object::New(env);
    result.Set("width", Napi::Number::New(env, fb->width));
    result.Set("height", Napi::Number::New(env, fb->height));
    result.Set("channels", Napi::Number::New(env, fb->channels));
    result.Set("pixels", CreateBufferFromPixels(env, fb));
    
    return result;
}

Napi::Value SetMPRSliceIndex(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 2 || !info[0].IsExternal() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "Expected (mpr: handle, index: number)")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    MPRHandle mpr = info[0].As<Napi::External<void>>().Data();
    int index = info[1].As<Napi::Number>().Int32Value();
    
    Bridge_SetMPRSliceIndex(mpr, index);
    return env.Undefined();
}

// ============================================================================
// 鼠标工具API
// ============================================================================

Napi::Value SetMouseTool(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 1 || !info[0].IsNumber()) {
        Napi::TypeError::New(env, "Expected tool type number").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    int toolType = info[0].As<Napi::Number>().Int32Value();
    Bridge_SetMouseTool(toolType);
    
    return env.Undefined();
}

Napi::Value SendMouseEvent(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    if (info.Length() < 4) {
        Napi::TypeError::New(env, "Expected (x, y, button, action)")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    int x = info[0].As<Napi::Number>().Int32Value();
    int y = info[1].As<Napi::Number>().Int32Value();
    int button = info[2].As<Napi::Number>().Int32Value();
    int action = info[3].As<Napi::Number>().Int32Value();
    
    Bridge_SendMouseEvent(x, y, button, action);
    return env.Undefined();
}

// ============================================================================
// 状态查询API
// ============================================================================

Napi::Value GetRenderStatus(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    const char* jsonStr = Bridge_GetRenderStatus();
    
    // 解析JSON
    Napi::Object result = env.Global().Get("JSON").As<Napi::Object>()
        .Get("parse").As<Napi::Function>()
        .Call({Napi::String::New(env, jsonStr)}).As<Napi::Object>();
    
    return result;
}

Napi::Value GetMeasurements(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    
    const char* jsonStr = Bridge_GetMeasurements();
    
    Napi::Array result = env.Global().Get("JSON").As<Napi::Object>()
        .Get("parse").As<Napi::Function>()
        .Call({Napi::String::New(env, jsonStr)}).As<Napi::Array>();
    
    return result;
}

// ============================================================================
// 模块初始化
// ============================================================================

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    // 初始化Bridge
    Bridge_Initialize();
    
    // 导出函数
    exports.Set("createWindow", Napi::Function::New(env, CreateWindow));
    exports.Set("destroyWindow", Napi::Function::New(env, DestroyWindow));
    exports.Set("processMessages", Napi::Function::New(env, ProcessMessages));
    
    exports.Set("loadDicomSeries", Napi::Function::New(env, LoadDicomSeries));
    exports.Set("getVolumeInfo", Napi::Function::New(env, GetVolumeInfo));
    
    exports.Set("createMPRRenderer", Napi::Function::New(env, CreateMPRRenderer));
    exports.Set("renderMPR", Napi::Function::New(env, RenderMPR));
    exports.Set("setMPRSliceIndex", Napi::Function::New(env, SetMPRSliceIndex));
    
    exports.Set("setMouseTool", Napi::Function::New(env, SetMouseTool));
    exports.Set("sendMouseEvent", Napi::Function::New(env, SendMouseEvent));
    
    exports.Set("getRenderStatus", Napi::Function::New(env, GetRenderStatus));
    exports.Set("getMeasurements", Napi::Function::New(env, GetMeasurements));
    
    // 导出常量
    Napi::Object toolTypes = Napi::Object::New(env);
    toolTypes.Set("NONE", Napi::Number::New(env, 0));
    toolTypes.Set("MEASURE", Napi::Number::New(env, 1));
    toolTypes.Set("BRUSH", Napi::Number::New(env, 2));
    toolTypes.Set("ERASER", Napi::Number::New(env, 3));
    toolTypes.Set("ROI_RECT", Napi::Number::New(env, 4));
    exports.Set("ToolType", toolTypes);
    
    return exports;
}

NODE_API_MODULE(native, Init)
```

---

## 4. JavaScript使用示例

### main.js - Electron主进程

```javascript
const { app, BrowserWindow } = require('electron');
const native = require('./build/Release/native.node');

let mainWindow;
let windowHandle;

app.on('ready', () => {
    mainWindow = new BrowserWindow({
        width: 1200,
        height: 800,
        webPreferences: {
            nodeIntegration: true,
            contextIsolation: false
        }
    });

    // 创建嵌入式渲染窗口
    const hwnd = mainWindow.getNativeWindowHandle().readInt32LE(0);
    windowHandle = native.createWindow(800, 600, hwnd);

    mainWindow.loadFile('index.html');

    // 消息循环
    setInterval(() => {
        native.processMessages(windowHandle);
    }, 16);
});

app.on('window-all-closed', () => {
    if (windowHandle) {
        native.destroyWindow(windowHandle);
    }
    app.quit();
});
```

### renderer.js - 渲染进程

```javascript
const native = require('./build/Release/native.node');

let volume = null;
let mprHandle = null;
const canvas = document.getElementById('mprCanvas');
const ctx = canvas.getContext('2d');

// 加载DICOM
async function loadDicom(path) {
    volume = native.loadDicomSeries(path, (progress, message) => {
        console.log(`Loading: ${progress}% - ${message}`);
        document.getElementById('progress').textContent = `${progress}%`;
    });

    if (volume) {
        const info = native.getVolumeInfo(volume);
        console.log('Volume Info:', info);

        mprHandle = native.createMPRRenderer(volume, 0 /* Axial */);
        startRenderLoop();
    }
}

// 渲染循环
function startRenderLoop() {
    setInterval(() => {
        const fb = native.renderMPR(mprHandle);
        if (fb && fb.pixels) {
            const imageData = new ImageData(
                new Uint8ClampedArray(fb.pixels),
                fb.width,
                fb.height
            );
            ctx.putImageData(imageData, 0, 0);
        }

        // 更新状态
        const status = native.getRenderStatus();
        updateUI(status);
    }, 16); // 60 FPS
}

// 更新UI
function updateUI(status) {
    document.getElementById('fps').textContent = status.fps.toFixed(1);
    document.getElementById('slice').textContent = status.currentSlice;
    document.getElementById('mousePos').textContent = 
        `(${status.mousePosition.x}, ${status.mousePosition.y})`;
}

// 鼠标事件
canvas.addEventListener('mousedown', (e) => {
    native.sendMouseEvent(e.offsetX, e.offsetY, e.button, 0);
});

canvas.addEventListener('mousemove', (e) => {
    native.sendMouseEvent(e.offsetX, e.offsetY, -1, 1);
});

canvas.addEventListener('mouseup', (e) => {
    native.sendMouseEvent(e.offsetX, e.offsetY, e.button, 2);
});

// 工具切换
document.getElementById('btnMeasure').addEventListener('click', () => {
    native.setMouseTool(native.ToolType.MEASURE);
});

document.getElementById('btnBrush').addEventListener('click', () => {
    native.setMouseTool(native.ToolType.BRUSH);
});

// 切片导航
document.getElementById('sliceSlider').addEventListener('input', (e) => {
    const index = parseInt(e.target.value);
    native.setMPRSliceIndex(mprHandle, index);
});

// 启动
loadDicom('D:/MedicalData/CT/series1/');
```

### index.html - UI

```html
<!DOCTYPE html>
<html>
<head>
    <title>Medical Viewer</title>
    <style>
        body { margin: 0; padding: 20px; font-family: Arial; }
        #mprCanvas { border: 1px solid #ccc; }
        .toolbar { margin-bottom: 10px; }
        .status { margin-top: 10px; }
    </style>
</head>
<body>
    <div class="toolbar">
        <button id="btnMeasure">测量工具</button>
        <button id="btnBrush">画笔工具</button>
        <button id="btnEraser">橡皮擦</button>
        <button id="btnROI">ROI工具</button>
    </div>

    <canvas id="mprCanvas" width="512" height="512"></canvas>

    <div class="status">
        <div>FPS: <span id="fps">0</span></div>
        <div>当前切片: <span id="slice">0</span></div>
        <div>鼠标位置: <span id="mousePos">(0, 0)</span></div>
        <div>加载进度: <span id="progress">0%</span></div>
    </div>

    <div>
        <label>切片导航: </label>
        <input type="range" id="sliceSlider" min="0" max="300" value="150">
    </div>

    <script src="renderer.js"></script>
</body>
</html>
```

---

## 5. 编译和运行

### 安装依赖

```bash
cd ElectronApp
npm install electron
npm install node-addon-api
npm install --save-dev node-gyp
```

### 编译Node Addon

```bash
cd native
node-gyp configure
node-gyp build
```

### 运行Electron

```bash
cd ..
npm start
```

---

## 6. TypeScript类型定义

```typescript
// native.d.ts
declare module 'native' {
    export enum ToolType {
        NONE = 0,
        MEASURE = 1,
        BRUSH = 2,
        ERASER = 3,
        ROI_RECT = 4
    }

    export interface VolumeInfo {
        width: number;
        height: number;
        depth: number;
        spacingX: number;
        spacingY: number;
        spacingZ: number;
        dataType: string;
    }

    export interface FrameBuffer {
        width: number;
        height: number;
        channels: number;
        pixels: Uint8Array;
    }

    export interface RenderStatus {
        fps: number;
        mousePosition: { x: number; y: number };
        currentSlice: number;
        windowWidth: number;
        windowLevel: number;
        zoom: number;
        pan: { x: number; y: number };
        activeTool: string;
    }

    export function createWindow(width: number, height: number, parentHWND?: number): any;
    export function destroyWindow(windowHandle: any): void;
    export function processMessages(windowHandle: any): boolean;

    export function loadDicomSeries(
        path: string, 
        callback?: (progress: number, message: string) => void
    ): any;
    export function getVolumeInfo(volume: any): VolumeInfo;

    export function createMPRRenderer(volume: any, direction: number): any;
    export function renderMPR(mpr: any): FrameBuffer | null;
    export function setMPRSliceIndex(mpr: any, index: number): void;

    export function setMouseTool(toolType: ToolType): void;
    export function sendMouseEvent(x: number, y: number, button: number, action: number): void;

    export function getRenderStatus(): RenderStatus;
    export function getMeasurements(): Array<any>;
}
```

---

## 7. package.json

```json
{
    "name": "medical-viewer",
    "version": "1.0.0",
    "description": "Medical Image Viewer with Electron",
    "main": "main.js",
    "scripts": {
        "start": "electron .",
        "build-native": "cd native && node-gyp rebuild",
        "rebuild": "npm run build-native && npm start"
    },
    "dependencies": {
        "electron": "^28.0.0",
        "node-addon-api": "^7.0.0"
    },
    "devDependencies": {
        "node-gyp": "^10.0.0"
    }
}
```

---

## 8. 总结

此示例展示了完整的Electron + Node Addon + C++ DLL集成方案：

✅ **N-API封装**: 使用node-addon-api提供类型安全的JavaScript接口  
✅ **内存管理**: 正确处理C++和JavaScript之间的数据传递  
✅ **回调机制**: 支持异步操作（DICOM加载进度）  
✅ **类型定义**: 提供TypeScript声明文件  
✅ **完整示例**: 包含窗口管理、渲染、鼠标工具等所有功能  

**下一步**: 根据此示例创建实际的Electron项目！
