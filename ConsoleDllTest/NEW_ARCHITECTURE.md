# 医学影像查看器新架构设计

## 架构概览

```
┌─────────────────────────────────────────────────────────────┐
│                   Electron (UI 层)                          │
│─────────────────────────────────────────────────────────────│
│ • 页面切换 / 按钮 / 进度条 / 文本显示                        │
│ • Canvas 或 DIV 用于嵌入渲染窗口                             │
│ • IPC 调用 Node Addon：发送鼠标键盘事件                      │
│ • 接收 DLL 状态（帧率、鼠标坐标、测量值等）更新 UI           │
└─────────────────────────────────────────────────────────────┘
                           │
                           │ node-addon-api 调用
                           ▼
┌─────────────────────────────────────────────────────────────┐
│            NativeHost (C++ Bridge 层)                        │
│─────────────────────────────────────────────────────────────│
│ • 创建 Win32 窗口 (HWND)                                     │
│ • 负责消息循环 / 鼠标事件 / 窗口管理                         │
│ • 调用 DLL 的渲染接口                                        │
│ • 监听渲染状态，回传状态给 Electron (JSON 等)                │
│ • **只做参数转换，无业务逻辑**                               │
└─────────────────────────────────────────────────────────────┘
                           │
                           │ 内部调用
                           ▼
┌─────────────────────────────────────────────────────────────┐
│           RenderCore.dll (OpenGL 渲染层)                    │
│─────────────────────────────────────────────────────────────│
│ • 处理 DICOM 图像 / 体绘制 / MPR / 标注线渲染                │
│ • 绘制到 Framebuffer / OpenGL Context                        │
│ • 输出状态数据 (例如当前光标坐标、测量长度等)                │
│ • 后台渲染，输出像素数据给Bridge层                           │
└─────────────────────────────────────────────────────────────┘
```

## 核心设计原则

### 1. UTF-8全面支持
- **所有源文件**: 使用UTF-8 with BOM编码
- **API接口**: 所有字符串参数使用UTF-8编码
- **文件路径**: 统一使用`EncodingUtils`进行转换
- **配置文件**: JSON格式，UTF-8编码

### 2. 简化的Bridge层
- **只做转换**: 参数类型转换、编码转换
- **无业务逻辑**: 所有逻辑在DLL层实现
- **状态透传**: 将DLL状态序列化为JSON返回给Electron

### 3. 后台渲染模式
- **离屏FBO**: 渲染到Framebuffer Object
- **像素传输**: 通过共享内存或回调传递像素数据
- **性能优化**: 避免窗口刷新开销

### 4. 统一的鼠标工具管理
- **工具基类**: `MouseTool` 统一接口
- **工具管理器**: `MouseToolManager` 负责工具切换和事件分发
- **工具类型**: 测量、画笔、橡皮擦、ROI等

## 模块详细设计

### 模块1: NativeHost Bridge层

**文件**: `Common/BridgeAPI.h/cpp`

**核心API**:
```cpp
// 窗口管理
void* Bridge_CreateWindow(int width, int height, void* parentHWND);
void Bridge_DestroyWindow(void* windowHandle);
bool Bridge_ProcessMessages(void* windowHandle);

// DICOM加载
VolumeHandle Bridge_LoadDicomSeries(const char* pathUtf8, ProgressCallback cb);
const char* Bridge_GetVolumeInfo(VolumeHandle volume);

// 渲染控制
MPRHandle Bridge_CreateMPRRenderer(VolumeHandle volume, int direction);
FrameBuffer* Bridge_RenderMPR(MPRHandle mpr);  // 后台渲染
void Bridge_SetMPRSliceIndex(MPRHandle mpr, int index);

// 鼠标工具
void Bridge_SetMouseTool(int toolType);
void Bridge_SendMouseEvent(int x, int y, int button, int action);

// 状态查询
const char* Bridge_GetRenderStatus();  // JSON格式
const char* Bridge_GetMeasurements();  // JSON格式
```

**特点**:
- 所有函数都是C接口（extern "C"）
- 参数简单，易于从Node Addon调用
- 返回JSON字符串，便于JavaScript解析

### 模块2: Win32窗口管理器

**文件**: `Common/WindowManager.h/cpp`

**功能**:
```cpp
class WindowManager {
    // 创建窗口（可嵌入Electron）
    bool CreateWindow(int width, int height, const wchar_t* title, HWND parent);
    
    // 消息循环（非阻塞）
    bool ProcessMessages();
    
    // 事件回调
    void SetMouseCallback(MouseCallback cb);
    void SetKeyCallback(KeyCallback cb);
    void SetResizeCallback(ResizeCallback cb);
};
```

**特点**:
- 支持作为子窗口嵌入Electron
- 非阻塞消息处理
- 事件回调机制，解耦窗口和业务逻辑

### 模块3: 鼠标工具管理器

**文件**: `Common/MouseToolManager.h/cpp`

**架构**:
```cpp
class MouseTool {  // 基类
    virtual bool OnMouseDown(int x, int y, int button) = 0;
    virtual bool OnMouseMove(int x, int y) = 0;
    virtual bool OnMouseUp(int x, int y, int button) = 0;
    virtual void Render() = 0;
};

class MeasureTool : public MouseTool { /* 测量工具 */ };
class BrushTool : public MouseTool { /* 画笔工具 */ };
class EraserTool : public MouseTool { /* 橡皮擦工具 */ };
class ROIRectangleTool : public MouseTool { /* ROI工具 */ };

class MouseToolManager {
    void SetActiveTool(ToolType type);
    bool DispatchMouseDown/Move/Up(...);
    void RenderAll();
};
```

**特点**:
- 统一的工具接口
- 工具之间互不干扰
- 易于扩展新工具

### 模块4: 离屏渲染层

**文件**: `DllVisualization/OffscreenRenderer.h/cpp`

**功能**:
```cpp
class OffscreenRenderer {
    // 创建FBO
    bool Initialize(int width, int height);
    
    // 开始离屏渲染
    void BeginRender();
    
    // 结束渲染，获取像素数据
    FrameBuffer* EndRender();
    
    // 调整大小
    void Resize(int width, int height);
};
```

**渲染流程**:
1. `BeginRender()` - 绑定FBO
2. 执行OpenGL绘制命令（MPR/3D/标注）
3. `EndRender()` - 读取像素数据到FrameBuffer
4. 将FrameBuffer传递给Bridge层
5. Bridge层将数据发送到Electron

### 模块5: 编码转换工具

**文件**: `Common/EncodingUtils.h/cpp`（已完成）

**功能**:
- `GbkToUtf8()` / `Utf8ToGbk()`
- `GetUtf8Path()` - 用于DICOM路径
- `OpenFile()` - 支持中文路径

## Electron集成方案

### Node Addon接口示例

```javascript
// native.node (Node Addon)
const native = require('./native.node');

// 创建窗口
const windowHandle = native.createWindow(800, 600, electronWindow.getNativeWindowHandle());

// 加载DICOM
const volume = native.loadDicomSeries('D:/医学影像/CT/', (progress, message) => {
    console.log(`加载进度: ${progress}% - ${message}`);
});

// 创建MPR渲染器
const mprHandle = native.createMPRRenderer(volume, 0 /* Axial */);

// 设置鼠标工具
native.setMouseTool(3); // 测量工具

// 发送鼠标事件
canvas.addEventListener('mousemove', (e) => {
    native.sendMouseEvent(e.offsetX, e.offsetY, -1, 2);
});

// 渲染循环
setInterval(() => {
    const frameBuffer = native.renderMPR(mprHandle);
    // 将frameBuffer显示到Canvas
    const imageData = new ImageData(
        new Uint8ClampedArray(frameBuffer.pixels),
        frameBuffer.width,
        frameBuffer.height
    );
    ctx.putImageData(imageData, 0, 0);
    
    // 获取状态更新UI
    const status = JSON.parse(native.getRenderStatus());
    updateUI(status);
}, 16); // 60 FPS
```

### Electron主进程代码示例

```javascript
const { app, BrowserWindow } = require('electron');
const native = require('./native.node');

app.on('ready', () => {
    const mainWindow = new BrowserWindow({
        width: 1200,
        height: 800,
        webPreferences: {
            nodeIntegration: true,
            contextIsolation: false
        }
    });

    // 创建嵌入式渲染窗口
    const hwnd = mainWindow.getNativeWindowHandle();
    const renderWindow = native.createWindow(800, 600, hwnd);
    
    mainWindow.loadFile('index.html');
});
```

## 状态管理与通信

### 状态JSON格式

```json
{
    "fps": 60.2,
    "mousePosition": { "x": 320, "y": 240 },
    "currentSlice": 125,
    "windowWidth": 400,
    "windowLevel": 40,
    "zoom": 1.5,
    "pan": { "x": 10, "y": -5 },
    "activeTool": "measure",
    "measurements": [
        {
            "id": 1,
            "type": "distance",
            "value": 45.6,
            "unit": "mm",
            "points": [[100, 100], [200, 150]]
        }
    ]
}
```

## 项目结构

```
ConsoleDllTest/
├── Common/                      # 公共代码
│   ├── NativeInterfaces.h       # 基础类型定义
│   ├── VolumeData.h            # Volume数据结构
│   ├── EncodingUtils.h/cpp     # 编码转换工具 ✅
│   ├── AnalysisEngineBase.h/cpp # Analysis基类 ✅
│   ├── WindowManager.h/cpp     # Win32窗口管理 ✅
│   ├── MouseToolManager.h/cpp  # 鼠标工具管理 ✅
│   └── BridgeAPI.h/cpp         # Bridge层API ✅
│
├── DllCore/                    # 核心DLL
├── DllDicom/                   # DICOM处理 (重构完成✅)
├── DllVisualization/           # 渲染层
│   ├── OffscreenRenderer.h/cpp # 离屏渲染
│   ├── MPRRenderer.h/cpp       # MPR渲染
│   ├── APRRenderer.h/cpp       # APR渲染
│   └── Volume3DRenderer.h/cpp  # 3D体绘制
│
├── DllImageProcessing/         # 图像处理
├── DllBoneAnalysis/            # 骨骼分析 (重构中)
├── DllFatAnalysis/             # 脂肪分析
├── DllLungAnalysis/            # 肺部分析
│
├── NativeHostBridge/           # Bridge DLL (新建)
│   ├── BridgeAPI.cpp          # Bridge实现
│   └── NativeHostBridge.vcxproj
│
└── ElectronApp/                # Electron应用 (新建)
    ├── package.json
    ├── main.js                # 主进程
    ├── renderer.js            # 渲染进程
    ├── native/                # Node Addon
    │   └── binding.cpp        # N-API绑定
    └── assets/                # 资源文件
```

## 开发步骤

### 阶段1: 基础设施 (已完成40%)
- [x] EncodingUtils - 编码转换
- [x] VolumeData - 统一数据结构
- [x] AnalysisEngineBase - 分析基类
- [x] WindowManager - 窗口管理
- [x] MouseToolManager - 工具管理
- [x] BridgeAPI设计
- [ ] DllDicom重构完成
- [ ] Analysis DLL基类重构

### 阶段2: Bridge层开发
- [ ] 实现BridgeAPI.cpp
- [ ] 创建NativeHostBridge.dll项目
- [ ] 编写测试程序验证API

### 阶段3: 离屏渲染
- [ ] 实现OffscreenRenderer
- [ ] 修改MPR/APR支持离屏模式
- [ ] 性能测试和优化

### 阶段4: Node Addon开发
- [ ] 使用node-addon-api封装Bridge API
- [ ] JavaScript接口设计
- [ ] 内存管理和错误处理

### 阶段5: Electron集成
- [ ] 创建Electron项目
- [ ] UI设计和交互
- [ ] 窗口嵌入和消息传递
- [ ] 完整功能测试

### 阶段6: UTF-8迁移
- [ ] 所有源文件转换为UTF-8
- [ ] 项目配置UTF-8编译选项
- [ ] 全面测试中文路径和显示

## 优势与特点

✅ **简化集成** - Bridge层API简单，易于Node Addon调用  
✅ **职责清晰** - 每层只做自己的事，无耦合  
✅ **易于调试** - C++ DLL可独立测试，不依赖Electron  
✅ **性能优越** - 后台渲染，避免窗口开销  
✅ **可扩展** - 工具管理器易于添加新工具  
✅ **跨平台潜力** - Bridge层可适配不同平台  

## 下一步

建议按照以下顺序继续：
1. 完成Analysis DLL基类重构（快速完成）
2. 实现MouseToolManager.cpp（核心功能）
3. 创建NativeHostBridge项目并实现BridgeAPI.cpp
4. 编写C++测试程序验证整个流程
5. 开始Node Addon开发

需要我继续实现哪个部分？
