#pragma once
#include "../Common/NativeInterfaces.h"

#ifdef __cplusplus
extern "C" {
#endif

// ==================== 错误处理 ====================
/// 获取最后错误信息
const char* BoneAnalysis_GetLastError();

// ==================== 骨分割 ====================
/// 自动骨分割（基于阈值和连通域）
MaskHandle BoneAnalysis_SegmentBone(VolumeHandle volume, AnalysisContextHandle context);
/// 骨密度分级分割（基于 HU 值范围）
MaskHandle BoneAnalysis_SegmentByDensity(VolumeHandle volume, float minHU, float maxHU);

// ==================== 骨密度分析 ====================
/// 计算平均骨密度（Average HU）
NativeResult BoneAnalysis_CalculateAverageDensity(VolumeHandle volume, MaskHandle mask, float* avgHU);
/// 计算骨体积
NativeResult BoneAnalysis_CalculateVolume(VolumeHandle volume, MaskHandle mask, float* volumeCm3);
/// 计算骨质量（需要校准参数）
NativeResult BoneAnalysis_CalculateMass(VolumeHandle volume, MaskHandle mask, float* massGrams);

// ==================== 骨质疏松分析 ====================
/// 骨质疏松评估（返回 T-Score）
NativeResult BoneAnalysis_CalculateTScore(VolumeHandle volume, MaskHandle mask, float* tScore);
/// 骨质疏松风险分级（正常、骨量减少、骨质疏松）
NativeResult BoneAnalysis_ClassifyOsteoporosis(float tScore, char* resultBuffer, int bufferSize);

// ==================== 骨折检测 ====================
/// 自动骨折检测（基于断裂检测算法）
MaskHandle BoneAnalysis_DetectFractures(VolumeHandle volume, MaskHandle boneMask);
/// 获取骨折点数量
int BoneAnalysis_GetFractureCount(MaskHandle fractureMask);
/// 获取骨折点坐标
NativeResult BoneAnalysis_GetFractureLocation(MaskHandle fractureMask, int index, 
                                               int* x, int* y, int* z);

// ==================== 骨骼测量 ====================
/// 测量骨骼长度（沿主轴）
NativeResult BoneAnalysis_MeasureLength(MaskHandle boneMask, float* lengthMm);
/// 测量骨骼厚度（平均）
NativeResult BoneAnalysis_MeasureThickness(MaskHandle boneMask, float* thicknessMm);
/// 测量骨骼表面积
NativeResult BoneAnalysis_MeasureSurfaceArea(MaskHandle boneMask, float* areaCm2);

// ==================== 综合分析 ====================
/// 执行完整骨骼分析（返回分析结果对象）
AnalysisResultHandle BoneAnalysis_Analyze(VolumeHandle volume, AnalysisContextHandle context);

#ifdef __cplusplus
}
#endif
