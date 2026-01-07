#pragma once
#include "../Common/NativeInterfaces.h"

#ifdef __cplusplus
extern "C" {
#endif

// ==================== 错误处理 ====================
/// 获取最后错误信息
__declspec(dllexport) const char* Core_GetLastError();

// ==================== 内存管理 ====================
/// 分配内存
__declspec(dllexport) void* Core_Malloc(size_t size);
/// 释放内存
__declspec(dllexport) void Core_Free(void* ptr);
/// 内存拷贝
__declspec(dllexport) void Core_Memcpy(void* dest, const void* src, size_t size);
/// 获取内存使用统计
__declspec(dllexport) NativeResult Core_GetMemoryStats(size_t* totalAllocated, size_t* peakUsage);

// ==================== 日志系统 ====================
/// 日志级别
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARNING = 2,
    LOG_LEVEL_ERROR = 3
} LogLevel;

/// 初始化日志系统
__declspec(dllexport) NativeResult Core_InitLogger(const char* logFilePath, LogLevel minLevel);
/// 写日志
__declspec(dllexport) void Core_Log(LogLevel level, const char* message);
/// 关闭日志系统
__declspec(dllexport) void Core_ShutdownLogger();

// ==================== 线程池 ====================
/// 任务回调函数
typedef void (*TaskCallback)(void* userData);

/// 创建线程池
__declspec(dllexport) ThreadPoolHandle Core_CreateThreadPool(int threadCount);
/// 销毁线程池
__declspec(dllexport) void Core_DestroyThreadPool(ThreadPoolHandle handle);
/// 提交任务
__declspec(dllexport) NativeResult Core_SubmitTask(ThreadPoolHandle handle, TaskCallback callback, void* userData);
/// 等待所有任务完成
__declspec(dllexport) NativeResult Core_WaitAllTasks(ThreadPoolHandle handle);
/// 获取线程池状态
__declspec(dllexport) NativeResult Core_GetThreadPoolStats(ThreadPoolHandle handle, int* queuedTasks, int* activeTasks);

// ==================== 性能计时 ====================
/// 创建计时器
__declspec(dllexport) TimerHandle Core_CreateTimer();
/// 销毁计时器
__declspec(dllexport) void Core_DestroyTimer(TimerHandle handle);
/// 开始计时
__declspec(dllexport) void Core_StartTimer(TimerHandle handle);
/// 停止计时并返回耗时（毫秒）
__declspec(dllexport) double Core_StopTimer(TimerHandle handle);

// ==================== 配置管理 ====================
/// 加载配置文件（JSON）
__declspec(dllexport) ConfigHandle Core_LoadConfig(const char* filepath);
/// 销毁配置
__declspec(dllexport) void Core_DestroyConfig(ConfigHandle handle);
/// 获取字符串配置
__declspec(dllexport) const char* Core_GetConfigString(ConfigHandle handle, const char* key, const char* defaultValue);
/// 获取整数配置
__declspec(dllexport) int Core_GetConfigInt(ConfigHandle handle, const char* key, int defaultValue);
/// 获取浮点配置
__declspec(dllexport) float Core_GetConfigFloat(ConfigHandle handle, const char* key, float defaultValue);
/// 获取布尔配置
__declspec(dllexport) bool Core_GetConfigBool(ConfigHandle handle, const char* key, bool defaultValue);

// ==================== Win32 窗口管理 ====================
/// 设置窗口 Z-Order（层级）
/// hwnd: 窗口句柄
/// topmost: true=置顶(HWND_TOPMOST), false=取消置顶(HWND_NOTOPMOST)
__declspec(dllexport) NativeResult Core_SetWindowTopmost(void* hwnd, bool topmost);

// ==================== 版本信息 ====================
/// 获取 Core 版本号
__declspec(dllexport) const char* Core_GetVersion();

#ifdef __cplusplus
}
#endif