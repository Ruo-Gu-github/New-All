#pragma once

/// NativeHost Bridge API - Electron通过Node Addon调用的简化接口
/// 原则：只做参数转换，不包含业务逻辑

#ifdef _WIN32
#define BRIDGE_API __declspec(dllexport)
#else
#define BRIDGE_API
#endif

#include "../Common/NativeInterfaces.h"

extern "C" {

// ==================== 窗口管理 ====================

/// 创建渲染窗口
/// @param width 窗口宽度
/// @param height 窗口高度  
/// @param parentHWND 父窗口句柄（Electron窗口），传0表示独立窗口
/// @return 窗口句柄（HWND），失败返回nullptr
BRIDGE_API void* Bridge_CreateWindow(int width, int height, void* parentHWND);

/// 销毁窗口
BRIDGE_API void Bridge_DestroyWindow(void* windowHandle);

/// 处理窗口消息（非阻塞）
/// @return 是否有消息处理
BRIDGE_API bool Bridge_ProcessMessages(void* windowHandle);

/// 显示/隐藏窗口
BRIDGE_API void Bridge_ShowWindow(void* windowHandle, bool visible);

/// 获取窗口尺寸
BRIDGE_API void Bridge_GetWindowSize(void* windowHandle, int* width, int* height);

// ==================== DICOM加载 ====================

/// 加载DICOM系列（带进度回调）
/// @param folderPath DICOM文件夹路径（UTF-8编码）
/// @param progressCallback 进度回调函数指针（可为null）
/// @return VolumeHandle，失败返回nullptr
BRIDGE_API VolumeHandle Bridge_LoadDicomSeries(const char* folderPathUtf8, ProgressCallback progressCallback);

/// 销毁Volume
BRIDGE_API void Bridge_DestroyVolume(VolumeHandle volume);

/// 获取Volume信息（JSON格式）
/// @return JSON字符串，需调用Bridge_FreeString释放
BRIDGE_API const char* Bridge_GetVolumeInfo(VolumeHandle volume);

// ==================== 渲染控制 ====================

/// 创建MPR渲染器
BRIDGE_API MPRHandle Bridge_CreateMPRRenderer(VolumeHandle volume, int sliceDirection);

/// 销毁MPR渲染器
BRIDGE_API void Bridge_DestroyMPRRenderer(MPRHandle mpr);

/// 渲染MPR到离屏缓冲区
/// @return 渲染结果（FrameBuffer*），包含像素数据
BRIDGE_API FrameBuffer* Bridge_RenderMPR(MPRHandle mpr);

/// 设置MPR切片索引
BRIDGE_API void Bridge_SetMPRSliceIndex(MPRHandle mpr, int sliceIndex);

/// 设置MPR窗宽窗位
BRIDGE_API void Bridge_SetMPRWindowLevel(MPRHandle mpr, int windowWidth, int windowLevel);

/// 设置MPR缩放
BRIDGE_API void Bridge_SetMPRZoom(MPRHandle mpr, float zoom);

/// 设置MPR平移
BRIDGE_API void Bridge_SetMPRPan(MPRHandle mpr, float panX, float panY);

// ==================== 鼠标工具 ====================

/// 设置当前激活的鼠标工具
/// @param toolType 工具类型（0=none, 1=pan, 2=zoom, 3=measure, 4=brush等）
BRIDGE_API void Bridge_SetMouseTool(int toolType);

/// 获取当前工具类型
BRIDGE_API int Bridge_GetMouseTool();

/// 发送鼠标事件
/// @param x, y 鼠标坐标
/// @param button 按钮（0=left, 1=right, 2=middle, -1=none）
/// @param action 动作（0=release, 1=press, 2=move）
BRIDGE_API void Bridge_SendMouseEvent(int x, int y, int button, int action);

/// 设置画笔大小
BRIDGE_API void Bridge_SetBrushSize(int size);

/// 获取画笔大小
BRIDGE_API int Bridge_GetBrushSize();

// ==================== 状态查询 ====================

/// 获取渲染状态（JSON格式）
/// @return JSON字符串，包含帧率、鼠标坐标、测量结果等，需调用Bridge_FreeString释放
BRIDGE_API const char* Bridge_GetRenderStatus();

/// 获取测量结果列表（JSON格式）
/// @return JSON字符串数组，需调用Bridge_FreeString释放
BRIDGE_API const char* Bridge_GetMeasurements();

// ==================== 内存管理 ====================

/// 释放Bridge层分配的字符串
BRIDGE_API void Bridge_FreeString(const char* str);

/// 释放FrameBuffer
BRIDGE_API void Bridge_FreeFrameBuffer(FrameBuffer* fb);

// ==================== 错误处理 ====================

/// 获取最后的错误信息
BRIDGE_API const char* Bridge_GetLastError();

} // extern "C"

// ==================== C++ 辅助类（内部使用）====================

/// Bridge上下文（单例模式）
class BridgeContext {
public:
    static BridgeContext& Instance();

    // 窗口管理
    void* CreateRenderWindow(int width, int height, void* parent);
    void DestroyRenderWindow(void* handle);
    
    // Volume管理
    VolumeHandle LoadDicom(const char* path, ProgressCallback callback);
    
    // 渲染管理
    MPRHandle CreateMPR(VolumeHandle volume, int direction);
    FrameBuffer* RenderFrame(MPRHandle mpr);
    
    // 鼠标工具管理
    void SetTool(int toolType);
    void HandleMouse(int x, int y, int button, int action);
    
    // 状态管理
    std::string GetStatusJSON();
    
private:
    BridgeContext() = default;
    ~BridgeContext() = default;
    
    // 禁止拷贝
    BridgeContext(const BridgeContext&) = delete;
    BridgeContext& operator=(const BridgeContext&) = delete;
};
