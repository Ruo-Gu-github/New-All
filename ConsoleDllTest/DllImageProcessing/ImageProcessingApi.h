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

// ==================== Mask 管理器 ====================
typedef void* MaskManagerHandle;

/// 创建Mask管理器
IMGPROC_API MaskManagerHandle MaskManager_Create();
/// 销毁Mask管理器
IMGPROC_API void MaskManager_Destroy(MaskManagerHandle handle);
/// 获取Mask数量
IMGPROC_API int MaskManager_GetCount(MaskManagerHandle handle);
/// 删除指定Mask
IMGPROC_API NativeResult MaskManager_DeleteMask(MaskManagerHandle handle, int index);
/// 清空所有Mask
IMGPROC_API void MaskManager_Clear(MaskManagerHandle handle);

// ==================== Mask 创建 ====================
/// 从阈值创建Mask
IMGPROC_API int MaskManager_CreateFromThreshold(MaskManagerHandle handle, VolumeHandle volume,
                                                float minThreshold, float maxThreshold, const char* name);
/// 创建空白Mask
IMGPROC_API int MaskManager_CreateEmpty(MaskManagerHandle handle, int width, int height, int depth, const char* name);
/// 克隆Mask
IMGPROC_API int MaskManager_Clone(MaskManagerHandle handle, int index);

// ==================== Mask 属性 ====================
/// 设置Mask名称
IMGPROC_API void MaskManager_SetName(MaskManagerHandle handle, int index, const char* name);
/// 获取Mask名称
IMGPROC_API const char* MaskManager_GetName(MaskManagerHandle handle, int index);
/// 设置Mask颜色
IMGPROC_API void MaskManager_SetColor(MaskManagerHandle handle, int index, float r, float g, float b, float a);
/// 获取Mask颜色
IMGPROC_API void MaskManager_GetColor(MaskManagerHandle handle, int index, float* r, float* g, float* b, float* a);
/// 设置Mask可见性
IMGPROC_API void MaskManager_SetVisible(MaskManagerHandle handle, int index, bool visible);
/// 获取Mask可见性
IMGPROC_API bool MaskManager_GetVisible(MaskManagerHandle handle, int index);
/// 获取Mask数据指针
IMGPROC_API const uint8_t* MaskManager_GetData(MaskManagerHandle handle, int index);
/// 获取Mask尺寸
IMGPROC_API void MaskManager_GetDimensions(MaskManagerHandle handle, int index, int* width, int* height, int* depth);

// ==================== 2D 编辑工具 ====================
/// 在指定切片上绘制圆形（添加mask）
/// sliceDir: 0=Axial(XY), 1=Coronal(XZ), 2=Sagittal(YZ)
/// sliceIndex: 切片索引
/// cx, cy: 圆心坐标（像素）
/// radius: 半径（像素）
IMGPROC_API void MaskManager_DrawCircle2D(MaskManagerHandle handle, int index, int sliceDir, int sliceIndex, float cx, float cy, float radius);
/// 在指定切片上擦除圆形（删除mask）
IMGPROC_API void MaskManager_EraseCircle2D(MaskManagerHandle handle, int index, int sliceDir, int sliceIndex, float cx, float cy, float radius);

// ==================== 布尔运算 ====================
/// 并集
IMGPROC_API int MaskManager_Union(MaskManagerHandle handle, int indexA, int indexB, const char* name);
/// 交集
IMGPROC_API int MaskManager_Intersection(MaskManagerHandle handle, int indexA, int indexB, const char* name);
/// 差集
IMGPROC_API int MaskManager_Difference(MaskManagerHandle handle, int indexA, int indexB, const char* name);
/// 异或
IMGPROC_API int MaskManager_XOR(MaskManagerHandle handle, int indexA, int indexB, const char* name);
/// 反转
IMGPROC_API int MaskManager_Invert(MaskManagerHandle handle, int index, const char* name);

// ==================== 形态学操作 ====================
/// 膨胀
IMGPROC_API int MaskManager_Dilate(MaskManagerHandle handle, int index, int radius, const char* name);
/// 腐蚀
IMGPROC_API int MaskManager_Erode(MaskManagerHandle handle, int index, int radius, const char* name);
/// 开运算
IMGPROC_API int MaskManager_Opening(MaskManagerHandle handle, int index, int radius, const char* name);
/// 闭运算
IMGPROC_API int MaskManager_Closing(MaskManagerHandle handle, int index, int radius, const char* name);

// ==================== 连通域操作 ====================
/// FloodFill（单个种子点）
IMGPROC_API int MaskManager_FloodFill(MaskManagerHandle handle, int index, int x, int y, int z, const char* name);
/// FloodFill（多个种子点）
IMGPROC_API int MaskManager_FloodFillMulti(MaskManagerHandle handle, int index,
                                          const int* seedsX, const int* seedsY, const int* seedsZ,
                                          int numSeeds, const char* name);
/// 获取所有连通域（返回连通域数量，索引保存在indices数组中）
IMGPROC_API int MaskManager_GetConnectedComponents(MaskManagerHandle handle, int index, int* indices, int maxCount);
/// 移除小连通域
IMGPROC_API int MaskManager_RemoveSmallRegions(MaskManagerHandle handle, int index, int minVoxels, const char* name);
/// 保留最大连通域
IMGPROC_API int MaskManager_KeepLargestRegion(MaskManagerHandle handle, int index, const char* name);

// ==================== ROI绘制 ====================
/// 绘制矩形
IMGPROC_API void MaskManager_DrawRectangle(MaskManagerHandle handle, int index, int z, 
                                          int x1, int y1, int x2, int y2, bool fill);
/// 绘制圆形
IMGPROC_API void MaskManager_DrawCircle(MaskManagerHandle handle, int index, int z,
                                       int cx, int cy, int radius, bool fill);
/// 绘制画笔
IMGPROC_API void MaskManager_DrawBrush(MaskManagerHandle handle, int index, int z,
                                      int x, int y, int brushRadius);
/// 绘制多边形
IMGPROC_API void MaskManager_DrawPolygon(MaskManagerHandle handle, int index, int z,
                                        const int* pointsX, const int* pointsY, int numPoints, bool fill);
/// 绘制线段
IMGPROC_API void MaskManager_DrawLine(MaskManagerHandle handle, int index, int z,
                                     int x1, int y1, int x2, int y2);

// ==================== 测量分析 ====================
/// 计算体积（体素数量）
IMGPROC_API int MaskManager_CalculateVolume(MaskManagerHandle handle, int index);
/// 计算质心
IMGPROC_API void MaskManager_CalculateCentroid(MaskManagerHandle handle, int index,
                                               float* cx, float* cy, float* cz);
/// 计算边界框
IMGPROC_API void MaskManager_CalculateBoundingBox(MaskManagerHandle handle, int index,
                                                  int* minX, int* minY, int* minZ,
                                                  int* maxX, int* maxY, int* maxZ);
/// 提取边界
IMGPROC_API int MaskManager_ExtractBoundary(MaskManagerHandle handle, int index, const char* name);

/// 计算直方图（Mask内的体数据）
IMGPROC_API void MaskManager_CalculateHistogram(MaskManagerHandle handle, int index,
                                               const uint16_t* volumeData, int width, int height, int depth,
                                               int* histogram, int numBins, uint16_t* minValue, uint16_t* maxValue);

/// 保存Mask到文件
IMGPROC_API bool MaskManager_SaveToFile(MaskManagerHandle handle, int index, const char* filepath);

/// 从文件加载Mask
IMGPROC_API int MaskManager_LoadFromFile(MaskManagerHandle handle, const char* filepath);

// ==================== 高级操作 ====================
/// 填充孔洞
IMGPROC_API int MaskManager_FillHoles(MaskManagerHandle handle, int index, const char* name);
/// 形态学梯度
IMGPROC_API int MaskManager_MorphologicalGradient(MaskManagerHandle handle, int index, int radius, const char* name);

// 原有的API保持不变...

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
