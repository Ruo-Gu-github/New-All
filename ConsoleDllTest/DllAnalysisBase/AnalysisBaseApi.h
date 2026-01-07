#pragma once
#include "../Common/NativeInterfaces.h"

#ifdef __cplusplus
extern "C" {
#endif

// ==================== 错误处理 ====================
/// 获取最后错误信息
const char* AnalysisBase_GetLastError();

// ==================== 分析上下文管理 ====================
/// 创建分析上下文
AnalysisContextHandle AnalysisContext_Create();
/// 销毁分析上下文
void AnalysisContext_Destroy(AnalysisContextHandle handle);
/// 设置输入体数据
NativeResult AnalysisContext_SetVolume(AnalysisContextHandle handle, VolumeHandle volume);
/// 设置输入 Mask
NativeResult AnalysisContext_SetMask(AnalysisContextHandle handle, MaskHandle mask);

// ==================== 分析结果管理 ====================
/// 创建分析结果
AnalysisResultHandle AnalysisResult_Create();
/// 销毁分析结果
void AnalysisResult_Destroy(AnalysisResultHandle handle);
/// 获取结果项数量
int AnalysisResult_GetItemCount(AnalysisResultHandle handle);
/// 获取结果项（键值对）
NativeResult AnalysisResult_GetItem(AnalysisResultHandle handle, int index, 
                                    char* keyBuffer, int keyBufferSize,
                                    char* valueBuffer, int valueBufferSize);
/// 添加结果项
NativeResult AnalysisResult_AddItem(AnalysisResultHandle handle, const char* key, const char* value);

// ==================== 分析结果导出 ====================
/// 导出为 JSON
NativeResult AnalysisResult_ExportJSON(AnalysisResultHandle handle, const char* filepath);
/// 导出为 CSV
NativeResult AnalysisResult_ExportCSV(AnalysisResultHandle handle, const char* filepath);
/// 导出为 XML
NativeResult AnalysisResult_ExportXML(AnalysisResultHandle handle, const char* filepath);

// ==================== 进度回调 ====================
/// 设置进度回调函数（用于长时间分析任务）
typedef void (*ProgressCallback)(float progress, const char* message, void* userData);
NativeResult AnalysisContext_SetProgressCallback(AnalysisContextHandle handle, 
                                                  ProgressCallback callback, void* userData);

// ==================== 通用参数设置 ====================
/// 设置字符串参数
NativeResult AnalysisContext_SetStringParam(AnalysisContextHandle handle, const char* key, const char* value);
/// 设置整数参数
NativeResult AnalysisContext_SetIntParam(AnalysisContextHandle handle, const char* key, int value);
/// 设置浮点参数
NativeResult AnalysisContext_SetFloatParam(AnalysisContextHandle handle, const char* key, float value);
/// 设置布尔参数
NativeResult AnalysisContext_SetBoolParam(AnalysisContextHandle handle, const char* key, bool value);

#ifdef __cplusplus
}
#endif
