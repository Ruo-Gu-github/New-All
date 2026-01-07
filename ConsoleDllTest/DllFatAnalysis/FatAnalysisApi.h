#pragma once
#include "../Common/NativeInterfaces.h"

#ifdef __cplusplus
extern "C" {
#endif

// ==================== 错误处理 ====================
/// 获取最后错误信息
const char* FatAnalysis_GetLastError();

// ==================== 脂肪分割 ====================
/// 皮下脂肪分割
MaskHandle FatAnalysis_SegmentSubcutaneous(VolumeHandle volume, float minHU, float maxHU);
/// 内脏脂肪分割
MaskHandle FatAnalysis_SegmentVisceral(VolumeHandle volume, float minHU, float maxHU);
/// 肌间脂肪分割
MaskHandle FatAnalysis_SegmentIntramuscular(VolumeHandle volume, MaskHandle muscleMask);

// ==================== 脂肪定量分析 ====================
/// 计算脂肪体积
NativeResult FatAnalysis_CalculateVolume(MaskHandle fatMask, float* volumeCm3);
/// 计算脂肪质量（基于密度转换）
NativeResult FatAnalysis_CalculateMass(VolumeHandle volume, MaskHandle fatMask, float* massKg);
/// 计算脂肪面积（2D 切片）
NativeResult FatAnalysis_CalculateArea(MaskHandle fatMask, int sliceIndex, float* areaCm2);

// ==================== 内脏脂肪专项分析 ====================
/// 计算腹部脂肪分布（L1-L5 椎体水平）
NativeResult FatAnalysis_CalculateAbdominalFat(VolumeHandle volume, 
                                                int startSlice, int endSlice,
                                                float* visceralVol, float* subcutaneousVol);
/// 计算内脏脂肪/皮下脂肪比值（V/S Ratio）
NativeResult FatAnalysis_CalculateVSRatio(float visceralVol, float subcutaneousVol, float* ratio);

// ==================== 肝脏脂肪分析 ====================
/// 肝脏脂肪浸润检测
MaskHandle FatAnalysis_DetectHepaticSteatosis(VolumeHandle volume, MaskHandle liverMask);
/// 计算肝脏脂肪含量百分比
NativeResult FatAnalysis_CalculateHepaticFatPercentage(VolumeHandle volume, MaskHandle liverMask, float* percentage);

// ==================== 心包脂肪分析 ====================
/// 心包脂肪分割
MaskHandle FatAnalysis_SegmentPericardial(VolumeHandle volume);
/// 计算心包脂肪体积
NativeResult FatAnalysis_CalculatePericardialVolume(MaskHandle pericardialMask, float* volumeCm3);

// ==================== 肥胖度评估 ====================
/// 计算体脂百分比（基于全身扫描）
NativeResult FatAnalysis_CalculateBodyFatPercentage(VolumeHandle volume, float* percentage);
/// 肥胖风险评估（基于内脏脂肪）
NativeResult FatAnalysis_AssessObesityRisk(float visceralVolume, char* resultBuffer, int bufferSize);

// ==================== 综合分析 ====================
/// 执行完整脂肪分析
AnalysisResultHandle FatAnalysis_Analyze(VolumeHandle volume, AnalysisContextHandle context);

#ifdef __cplusplus
}
#endif
