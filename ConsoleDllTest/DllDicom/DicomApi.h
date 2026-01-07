#pragma once
#include "../Common/NativeInterfaces.h"

// 导出宏
#ifdef DLLDICOM_EXPORTS
#define DICOM_API __declspec(dllexport)
#else
#define DICOM_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif
// 生成 DICOM 缩略图 BMP，返回 BMP 数据指针和长度（需调用 Dicom_FreeBuffer 释放）
DICOM_API NativeResult Dicom_GenerateThumbnailBMP(DicomHandle handle, unsigned char** outData, int* outSize);
// 释放 DllDicom 分配的内存
DICOM_API void Dicom_FreeBuffer(void* ptr);

// ==================== 错误处理 ====================
/// 获取最后错误信息
DICOM_API const char* Dicom_GetLastError();

// ==================== DICOM 文件读写 ====================
/// 创建 DICOM 读取器
DICOM_API DicomHandle Dicom_CreateReader();
/// 销毁 DICOM 读取器
DICOM_API void Dicom_DestroyReader(DicomHandle handle);
/// 读取 DICOM 文件
DICOM_API NativeResult Dicom_ReadFile(DicomHandle handle, const char* filepath);
/// 读取 DICOM 目录（多文件序列）
DICOM_API NativeResult Dicom_ReadDirectory(DicomHandle handle, const char* dirpath, int* fileCount);

// ==================== DICOM Tag 操作 ====================
/// 获取 DICOM Tag 值（使用 group 和 element）
DICOM_API NativeResult Dicom_GetTag(DicomReaderHandle handle, uint16_t group, uint16_t element, char* buffer, int bufferSize);
/// 设置 DICOM Tag 值
DICOM_API NativeResult Dicom_SetTag(DicomReaderHandle handle, uint16_t group, uint16_t element, const char* value);
/// 获取图像尺寸
DICOM_API NativeResult Dicom_GetImageSize(DicomReaderHandle handle, int* width, int* height);

// ==================== DICOM 数据操作 ====================
/// 获取像素数据指针
DICOM_API void* Dicom_GetPixelData(DicomHandle handle, int* width, int* height, int* depth);
/// 设置像素数据
DICOM_API NativeResult Dicom_SetPixelData(DicomHandle handle, void* data, int width, int height, int depth);
/// 创建新的 DICOM 文件
DICOM_API NativeResult Dicom_CreateNew(DicomHandle handle, int width, int height, int depth);
/// 保存 DICOM 文件
DICOM_API NativeResult Dicom_SaveFile(DicomHandle handle, const char* filepath);

// ==================== 体数据管理 ====================
/// 进度回调函数类型 (当前进度 0-100, 消息)
typedef void (*ProgressCallback)(int progress, const char* message);

/// 创建体数据
DICOM_API VolumeHandle Dicom_Volume_Create();
/// 销毁体数据
DICOM_API void Dicom_Volume_Destroy(VolumeHandle handle);
/// 从 DICOM 序列加载体数据
DICOM_API NativeResult Dicom_Volume_LoadFromDicomSeries(VolumeHandle handle, const char* folderPath);
/// 从 DICOM 序列加载体数据（带进度回调）
DICOM_API NativeResult Dicom_Volume_LoadFromDicomSeriesWithProgress(VolumeHandle handle, const char* folderPath, ProgressCallback callback);
/// 从 DICOM 读取器加载体数据
DICOM_API NativeResult Dicom_Volume_LoadFromDicomReader(VolumeHandle handle, DicomHandle dicomHandle);
/// 获取体数据指针
DICOM_API void* Dicom_Volume_GetData(VolumeHandle handle);
/// 获取体数据尺寸
DICOM_API NativeResult Dicom_Volume_GetDimensions(VolumeHandle handle, int* width, int* height, int* depth);
/// 分配/重置体数据存储并设置尺寸（用于从外部原始数据填充）
DICOM_API NativeResult Dicom_Volume_Allocate(VolumeHandle handle, int width, int height, int depth);
/// 获取体素间距
DICOM_API NativeResult Dicom_Volume_GetSpacing(VolumeHandle handle, float* spacingX, float* spacingY, float* spacingZ);
/// 获取 DICOM Rescale Slope/Intercept（用于从存储值转换到显示值/HU）
DICOM_API NativeResult Dicom_Volume_GetRescale(VolumeHandle handle, float* slope, float* intercept);
/// 设置体素间距
DICOM_API NativeResult Dicom_Volume_SetSpacing(VolumeHandle handle, float spacingX, float spacingY, float spacingZ);
/// 保存体数据到 DICOM 序列
DICOM_API NativeResult Dicom_Volume_SaveToDicomSeries(VolumeHandle handle, const char* outputDir);

#ifdef __cplusplus
}
#endif
