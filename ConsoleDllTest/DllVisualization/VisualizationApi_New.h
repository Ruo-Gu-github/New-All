#pragma once
#include "../Common/NativeInterfaces.h"

// 导出宏
#ifdef DLLVISUALIZATION_EXPORTS
#define VIZ_API __declspec(dllexport)
#else
#define VIZ_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ==================== 错误处理 ====================
/// 获取最后错误信息
VIZ_API const char* Visualization_GetLastError();

// ==================== APR (任意平面重建) ====================
/// 创建 APR 渲染器
VIZ_API APRHandle APR_Create();
/// 销毁 APR 渲染器
VIZ_API void APR_Destroy(APRHandle handle);
/// 设置体数据
VIZ_API NativeResult APR_SetVolume(APRHandle handle, VolumeHandle volume);
/// 设置中心点
VIZ_API void APR_SetCenter(APRHandle handle, float x, float y, float z);
/// 获取中心点
VIZ_API void APR_GetCenter(APRHandle handle, float* x, float* y, float* z);
/// 设置旋转角度（欧拉角：绕X、Y、Z轴的旋转角度，单位：度）
VIZ_API void APR_SetRotation(APRHandle handle, float angleX, float angleY, float angleZ);
/// 获取旋转角度
VIZ_API void APR_GetRotation(APRHandle handle, float* angleX, float* angleY, float* angleZ);
/// 获取指定方向的切片（0=轴向/横断面, 1=矢状面, 2=冠状面）
VIZ_API void* APR_GetSlice(APRHandle handle, int direction, int* width, int* height);
/// 显示/隐藏定位线
VIZ_API void APR_SetShowCrossHair(APRHandle handle, bool show);
/// 获取定位线显示状态
VIZ_API bool APR_GetShowCrossHair(APRHandle handle);
/// 渲染（刷新）
VIZ_API NativeResult APR_Render(APRHandle handle);

// ==================== MPR (多平面重建) ====================
/// 创建 MPR 渲染器
VIZ_API MPRHandle MPR_Create();
/// 销毁 MPR 渲染器
VIZ_API void MPR_Destroy(MPRHandle handle);
/// 设置体数据
VIZ_API NativeResult MPR_SetVolume(MPRHandle handle, VolumeHandle volume);
/// 设置中心点
VIZ_API void MPR_SetCenter(MPRHandle handle, float x, float y, float z);
/// 获取中心点
VIZ_API void MPR_GetCenter(MPRHandle handle, float* x, float* y, float* z);
/// 获取指定方向的切片
VIZ_API void* MPR_GetSlice(MPRHandle handle, int direction, int* width, int* height);
/// 显示/隐藏定位线
VIZ_API void MPR_SetShowCrossHair(MPRHandle handle, bool show);
/// 渲染（刷新）
VIZ_API NativeResult MPR_Render(MPRHandle handle);

// ==================== 3D 体绘制 ====================
/// 创建 3D 体绘制渲染器
VIZ_API Volume3DHandle Volume3D_Create();
/// 销毁 3D 体绘制渲染器
VIZ_API void Volume3D_Destroy(Volume3DHandle handle);
/// 添加体数据
VIZ_API NativeResult Volume3D_AddVolume(Volume3DHandle handle, VolumeHandle volume);
/// 移除体数据
VIZ_API NativeResult Volume3D_RemoveVolume(Volume3DHandle handle, int index);
/// 获取体数据数量
VIZ_API int Volume3D_GetVolumeCount(Volume3DHandle handle);
/// 设置传递函数（为指定的体数据设置）
VIZ_API NativeResult Volume3D_SetTransferFunction(Volume3DHandle handle, int volumeIndex, TransferFunctionHandle tf);
/// 设置光照参数
VIZ_API void Volume3D_SetLightParameters(Volume3DHandle handle, float ambient, float diffuse, float specular);
/// 获取光照参数
VIZ_API void Volume3D_GetLightParameters(Volume3DHandle handle, float* ambient, float* diffuse, float* specular);
/// 渲染（刷新）
VIZ_API NativeResult Volume3D_Render(Volume3DHandle handle);

// ==================== 传递函数 ====================
/// 创建传递函数
VIZ_API TransferFunctionHandle TransferFunction_Create();
/// 销毁传递函数
VIZ_API void TransferFunction_Destroy(TransferFunctionHandle handle);
/// 添加控制点（value: 数据值, r/g/b/a: 颜色和透明度 0.0-1.0）
VIZ_API NativeResult TransferFunction_AddControlPoint(TransferFunctionHandle handle, float value, float r, float g, float b, float a);
/// 移除控制点
VIZ_API NativeResult TransferFunction_RemoveControlPoint(TransferFunctionHandle handle, int index);
/// 清空所有控制点
VIZ_API void TransferFunction_Clear(TransferFunctionHandle handle);
/// 获取控制点数量
VIZ_API int TransferFunction_GetControlPointCount(TransferFunctionHandle handle);

// ==================== 窗口管理 ====================
/// 创建渲染窗口（GLFW 窗口 - 用于独立测试）
VIZ_API WindowHandle Window_Create(int width, int height, const char* title);
/// 销毁渲染窗口
VIZ_API void Window_Destroy(WindowHandle handle);
/// 绑定渲染器到窗口（支持 APR/MPR/Volume3D）
VIZ_API NativeResult Window_BindRenderer(WindowHandle handle, void* rendererHandle, int rendererType);
/// 刷新窗口
VIZ_API void Window_Refresh(WindowHandle handle);
/// 获取窗口句柄（用于绑定 Electron div）
VIZ_API void* Window_GetNativeHandle(WindowHandle handle);
/// 窗口事件循环（非阻塞）
VIZ_API bool Window_PollEvents(WindowHandle handle);

// ==================== 离屏渲染（用于 Web 集成） ====================
/// 创建离屏 OpenGL 上下文（FBO）
VIZ_API WindowHandle OffscreenContext_Create(int width, int height);
/// 销毁离屏上下文
VIZ_API void OffscreenContext_Destroy(WindowHandle handle);
/// 渲染到 FBO 并获取像素数据（RGBA格式）
VIZ_API FrameBuffer* OffscreenContext_RenderToBuffer(WindowHandle handle, void* rendererHandle, int rendererType);
/// 释放 FrameBuffer
VIZ_API void FrameBuffer_Destroy(FrameBuffer* buffer);

#ifdef __cplusplus
}
#endif
