#pragma once
#include <napi.h>

// 初始化 ImageProcessing 模块
void InitImageProcessingModule(Napi::Env env, Napi::Object& exports);

// Mask 管理类
class MaskManager : public Napi::ObjectWrap<MaskManager> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  MaskManager(const Napi::CallbackInfo& info);
  ~MaskManager();

private:
  // Mask 操作方法
  Napi::Value CreateMask(const Napi::CallbackInfo& info);
  Napi::Value SetMaskData(const Napi::CallbackInfo& info);
  Napi::Value GetMaskData(const Napi::CallbackInfo& info);
  Napi::Value ApplyMorphology(const Napi::CallbackInfo& info);
  Napi::Value FillHoles(const Napi::CallbackInfo& info);
  
  void* maskHandle_;  // MaskHandle
};

// ROI 管理类
class ROIManager : public Napi::ObjectWrap<ROIManager> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  ROIManager(const Napi::CallbackInfo& info);
  ~ROIManager();

private:
  // ROI 操作方法
  Napi::Value CreateROI(const Napi::CallbackInfo& info);
  Napi::Value SetROIRegion(const Napi::CallbackInfo& info);
  Napi::Value GetROIStatistics(const Napi::CallbackInfo& info);
  
  void* roiHandle_;  // ROIHandle
};
