#include <napi.h>
#include "dicom_wrapper.h"
#include "image_processing_wrapper.h"
#include "visualization_wrapper.h"

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  // 初始化 DICOM 模块（只导出 loadDicomFolderInfo）
  InitDicomModule(env, exports);
  
  // 初始化 ImageProcessing 模块
  InitImageProcessingModule(env, exports);
  
  // 初始化 Visualization 模块（APR/MPR 渲染）
  InitVisualizationModule(env, exports);
  
  return exports;
}

NODE_API_MODULE(console_dll_addon, InitAll)
