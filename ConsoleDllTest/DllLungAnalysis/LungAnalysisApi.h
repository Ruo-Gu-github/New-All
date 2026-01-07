#pragma once
#include "../Common/NativeInterfaces.h"

#ifdef __cplusplus
extern "C" {
#endif

// ==================== 错误处理 ====================
/// 获取最后错误信息
const char* LungAnalysis_GetLastError();

// ==================== 肺分割 ====================
/// 自动肺分割（左右肺分离）
NativeResult LungAnalysis_SegmentLungs(VolumeHandle volume, MaskHandle* leftLung, MaskHandle* rightLung);
/// 肺叶分割（5 叶）
NativeResult LungAnalysis_SegmentLobes(VolumeHandle volume, MaskHandle lungMask, MaskHandle* lobes, int* lobeCount);

// ==================== 气道分析 ====================
/// 气道分割
MaskHandle LungAnalysis_SegmentAirway(VolumeHandle volume);
/// 气道树骨架提取
MaskHandle LungAnalysis_ExtractAirwaySkeleton(MaskHandle airwayMask);
/// 计算气道直径
NativeResult LungAnalysis_MeasureAirwayDiameter(MaskHandle airwayMask, int x, int y, int z, float* diameterMm);

// ==================== 肺结节检测 ====================
/// 自动肺结节检测
NativeResult LungAnalysis_DetectNodules(VolumeHandle volume, MaskHandle lungMask, 
                                        MaskHandle* noduleMasks, int* noduleCount, int maxNodules);
/// 获取结节位置
NativeResult LungAnalysis_GetNoduleLocation(MaskHandle noduleMask, int* x, int* y, int* z);
/// 计算结节直径
NativeResult LungAnalysis_MeasureNoduleDiameter(MaskHandle noduleMask, float* diameterMm);
/// 结节良恶性评估（基于特征）
NativeResult LungAnalysis_ClassifyNodule(VolumeHandle volume, MaskHandle noduleMask, 
                                         char* resultBuffer, int bufferSize);

// ==================== 肺气肿分析 ====================
/// 肺气肿检测（基于低密度区域）
MaskHandle LungAnalysis_DetectEmphysema(VolumeHandle volume, MaskHandle lungMask, float thresholdHU);
/// 计算肺气肿百分比
NativeResult LungAnalysis_CalculateEmphysemaPercentage(MaskHandle lungMask, MaskHandle emphysemaMask, float* percentage);

// ==================== 肺密度分析 ====================
/// 计算平均肺密度
NativeResult LungAnalysis_CalculateAverageDensity(VolumeHandle volume, MaskHandle lungMask, float* avgHU);
/// 计算肺体积
NativeResult LungAnalysis_CalculateLungVolume(MaskHandle lungMask, float* volumeL);

// ==================== 肺功能评估 ====================
/// 计算肺容积（TLC, FRC, RV）
NativeResult LungAnalysis_CalculateCapacity(MaskHandle lungMask, float* tlc, float* frc, float* rv);
/// 通气功能评估
NativeResult LungAnalysis_AssessVentilation(VolumeHandle volume, MaskHandle lungMask, 
                                             AnalysisResultHandle result);

// ==================== 综合分析 ====================
/// 执行完整肺部分析
AnalysisResultHandle LungAnalysis_Analyze(VolumeHandle volume, AnalysisContextHandle context);

#ifdef __cplusplus
}
#endif
