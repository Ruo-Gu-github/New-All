#pragma once
#include "../Common/NativeInterfaces.h"

// 导出宏
#ifdef DLLIMAGEPROCESSING_EXPORTS
#define IMGPROC_API __declspec(dllexport)
#else
#define IMGPROC_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ==================== 错误处理 ====================
/// 获取最后错误信息
IMGPROC_API const char* ImageProcessing_GetLastError();

// ==================== Mask 管理 ====================
/// 创建空 Mask
IMGPROC_API MaskHandle Mask_Create(int width, int height, int depth);
/// 销毁 Mask
IMGPROC_API void Mask_Destroy(MaskHandle handle);
/// 从阈值创建 Mask
IMGPROC_API MaskHandle Mask_CreateFromThreshold(VolumeHandle volume, float minValue, float maxValue);
/// 获取 Mask 数据指针
IMGPROC_API void* Mask_GetData(MaskHandle handle);
/// 获取 Mask 尺寸
IMGPROC_API void Mask_GetDimensions(MaskHandle handle, int* width, int* height, int* depth);

// ==================== Mask 集合操作 ====================
/// Mask 并集
IMGPROC_API MaskHandle Mask_Union(MaskHandle mask1, MaskHandle mask2);
/// Mask 交集
IMGPROC_API MaskHandle Mask_Intersect(MaskHandle mask1, MaskHandle mask2);
/// Mask 差集
IMGPROC_API MaskHandle Mask_Subtract(MaskHandle mask1, MaskHandle mask2);

// ==================== 连通域分析 ====================
/// 获取最大连通域
IMGPROC_API MaskHandle Mask_GetLargestConnectedComponent(MaskHandle mask);
/// FloodFill 算法（从指定点开始）
IMGPROC_API MaskHandle Mask_FloodFill(MaskHandle mask, int x, int y, int z);
/// 多点 FloodFill（从多个种子点开始，返回这些点的连通域集合）
IMGPROC_API MaskHandle Mask_FloodFillMultiSeed(MaskHandle mask, int* seedPoints, int seedCount);

// ==================== Mask 持久化 ====================
/// 保存 Mask 到文件
IMGPROC_API NativeResult Mask_SaveToFile(MaskHandle handle, const char* filepath);
/// 从文件加载 Mask
IMGPROC_API MaskHandle Mask_LoadFromFile(const char* filepath);

// ==================== 基于 Mask 的数据提取 ====================
/// 根据 Mask 提取体数据（只保留 Mask 覆盖的部分）
IMGPROC_API VolumeHandle Volume_ExtractByMask(VolumeHandle volume, MaskHandle mask);

// ==================== ROI 工具 ====================
/// 创建直线工具
IMGPROC_API ROIHandle ROI_CreateLine();
/// 创建角度测量工具
IMGPROC_API ROIHandle ROI_CreateAngle();
/// 创建矩形工具
IMGPROC_API ROIHandle ROI_CreateRectangle();
/// 创建圆形工具
IMGPROC_API ROIHandle ROI_CreateCircle();
/// 创建贝塞尔曲线工具
IMGPROC_API ROIHandle ROI_CreateBezier();
/// 销毁 ROI 工具
IMGPROC_API void ROI_Destroy(ROIHandle handle);

// ==================== ROI 状态管理 ====================
/// 保存 ROI 状态（包括旋转角、中心点、3D坐标）
IMGPROC_API NativeResult ROI_SaveState(ROIHandle handle, float centerX, float centerY, float centerZ, 
                           float rotX, float rotY, float rotZ);
/// 加载 ROI 状态
IMGPROC_API NativeResult ROI_LoadState(ROIHandle handle, float* centerX, float* centerY, float* centerZ,
                           float* rotX, float* rotY, float* rotZ);
/// 添加控制点（用于构建 ROI）
IMGPROC_API NativeResult ROI_AddPoint(ROIHandle handle, float x, float y, float z);
/// 获取控制点数量
IMGPROC_API int ROI_GetPointCount(ROIHandle handle);
/// 获取控制点坐标
IMGPROC_API NativeResult ROI_GetPoint(ROIHandle handle, int index, float* x, float* y, float* z);

// ==================== ROI 与 Mask 交互 ====================
/// 在 Mask 上应用 ROI（添加）
IMGPROC_API NativeResult Mask_AddROI(MaskHandle mask, ROIHandle roi);
/// 在 Mask 上应用 ROI（减少）
IMGPROC_API NativeResult Mask_SubtractROI(MaskHandle mask, ROIHandle roi);
/// 在 Mask 上应用 ROI（交集）
IMGPROC_API NativeResult Mask_IntersectROI(MaskHandle mask, ROIHandle roi);

// ==================== ROI 渲染 ====================
/// 渲染 ROI（用于在 2D/3D 视图中显示）
IMGPROC_API void ROI_Render(ROIHandle handle);
/// 设置 ROI 渲染颜色
IMGPROC_API void ROI_SetColor(ROIHandle handle, float r, float g, float b, float a);

#ifdef __cplusplus
}
#endif
