#include "dicom_wrapper.h"
#include "utils.h"
#include "DicomApi.h"
#include <algorithm>
#include <vector>
#include <cmath>
#include <filesystem>

namespace fs = std::filesystem;

// ==================== 辅助函数 ====================

static std::vector<std::string> ListDicomFilesInFolder(const std::string& folderPath) {
  std::vector<std::string> files;
  try {
    for (const auto& entry : fs::directory_iterator(folderPath)) {
      if (!entry.is_regular_file()) continue;
      std::string ext = entry.path().extension().string();
      std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

      // 检查是否是 DICOM 文件（.dcm 或无扩展名）
      if (ext == ".dcm" || ext.empty()) {
        files.push_back(entry.path().string());
      }
    }
  } catch (...) {
    return {};
  }

  std::sort(files.begin(), files.end());
  return files;
}

static std::string PickMiddleDicomFile(const std::vector<std::string>& files) {
  if (files.empty()) return "";
  return files[files.size() / 2];
}

static bool TryParseFirstNumber(const char* s, float* outValue) {
  if (!s || !outValue) return false;
  std::string str(s);
  // DICOM values can be multi-valued like "40\\60". Take the first.
  const auto slashPos = str.find('\\');
  if (slashPos != std::string::npos) {
    str = str.substr(0, slashPos);
  }

  // Trim
  auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
  while (!str.empty() && isSpace((unsigned char)str.front())) str.erase(str.begin());
  while (!str.empty() && isSpace((unsigned char)str.back())) str.pop_back();
  if (str.empty()) return false;

  try {
    *outValue = std::stof(str);
    return true;
  } catch (...) {
    return false;
  }
}

// ==================== LoadDicomFolderInfo 函数实现 ====================

Napi::Value LoadDicomFolderInfo(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  // 1. 检查参数
  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::TypeError::New(env, "Folder path string expected").ThrowAsJavaScriptException();
    return env.Null();
  }

  // 2. 获取文件夹路径并转换为 GBK
  std::string folderPathUtf8 = info[0].As<Napi::String>().Utf8Value();
  std::string folderPathGbk = Utf8ToAnsi(folderPathUtf8);
  
  // 3. 列出全部 DICOM 文件并选择中间一张（第一张有时为空白）
  const auto dicomFiles = ListDicomFilesInFolder(folderPathGbk);
  if (dicomFiles.empty()) {
    Napi::Error::New(env, "No DICOM files found in folder").ThrowAsJavaScriptException();
    return env.Null();
  }

  const int fileCount = static_cast<int>(dicomFiles.size());
  std::string sampleFile = PickMiddleDicomFile(dicomFiles);
  if (sampleFile.empty()) {
    sampleFile = dicomFiles.front();
  }

  // 4. 创建 DicomHandle 用于读取文件
  DicomHandle handle = Dicom_CreateReader();
  if (!handle) {
    Napi::Error::New(env, "Failed to create DICOM handle").ThrowAsJavaScriptException();
    return env.Null();
  }

  // 5. 读取样本 DICOM 文件（优先中间一张）
  NativeResult result = Dicom_ReadFile(handle, sampleFile.c_str());
  if (result != NATIVE_OK) {
    const char* error = Dicom_GetLastError();
    // 回退：如果中间一张不可用，尝试第一张
    if (!dicomFiles.empty() && sampleFile != dicomFiles.front()) {
      result = Dicom_ReadFile(handle, dicomFiles.front().c_str());
    }
    if (result != NATIVE_OK) {
      error = Dicom_GetLastError();
      Dicom_DestroyReader(handle);
      Napi::Error::New(env, error ? error : "Failed to read DICOM file").ThrowAsJavaScriptException();
      return env.Null();
    }
  }

  // 6. 获取图像尺寸
  int width = 0, height = 0;
  result = Dicom_GetImageSize(handle, &width, &height);
  if (result != NATIVE_OK) {
    width = 512;  // 默认值
    height = 512;
  }

  // 7. 获取常用 Tag 信息
  char patientName[256] = {0};
  char patientID[256] = {0};
  char studyDate[256] = {0};
  char studyDescription[256] = {0};
  char seriesDescription[256] = {0};
  char modality[64] = {0};

  // Window/Level defaults
  char windowCenterRaw[256] = {0};
  char windowWidthRaw[256] = {0};
  
  // PatientName (0010,0010)
  Dicom_GetTag(handle, 0x0010, 0x0010, patientName, sizeof(patientName));
  // PatientID (0010,0020)
  Dicom_GetTag(handle, 0x0010, 0x0020, patientID, sizeof(patientID));
  // StudyDate (0008,0020)
  Dicom_GetTag(handle, 0x0008, 0x0020, studyDate, sizeof(studyDate));
  // StudyDescription (0008,1030)
  Dicom_GetTag(handle, 0x0008, 0x1030, studyDescription, sizeof(studyDescription));
  // SeriesDescription (0008,103E)
  Dicom_GetTag(handle, 0x0008, 0x103E, seriesDescription, sizeof(seriesDescription));
  // Modality (0008,0060)
  Dicom_GetTag(handle, 0x0008, 0x0060, modality, sizeof(modality));

  // Window Center (0028,1050) and Window Width (0028,1051)
  Dicom_GetTag(handle, 0x0028, 0x1050, windowCenterRaw, sizeof(windowCenterRaw));
  Dicom_GetTag(handle, 0x0028, 0x1051, windowWidthRaw, sizeof(windowWidthRaw));

  // 8. 生成缩略图
  int pixelWidth = 0, pixelHeight = 0, pixelDepth = 0;
  
  Napi::Value thumbnailBuffer = env.Null();
  Napi::Object validation = Napi::Object::New(env);

  // 生成缩略图（优先走 DllDicom 内置接口）
  // 注意：部分 Electron/Node 环境禁用 External Buffer（会抛出 "External buffers are not allowed"），
  // 因此这里使用 Copy() 创建 JS 管理的 Buffer。
  unsigned char* bmpData = nullptr;
  int bmpSize = 0;
  try {
    NativeResult thumbResult = Dicom_GenerateThumbnailBMP(handle, &bmpData, &bmpSize);
    if (thumbResult == NATIVE_OK && bmpData && bmpSize > 0) {
      // Copy into a JS-owned Buffer, then free native memory.
      thumbnailBuffer = Napi::Buffer<unsigned char>::Copy(
        env,
        bmpData,
        static_cast<size_t>(bmpSize)
      );
      Dicom_FreeBuffer(static_cast<void*>(bmpData));
      bmpData = nullptr;
      validation.Set("isValid", Napi::Boolean::New(env, true));
      validation.Set("message", Napi::String::New(env, "Thumbnail generated"));
    } else {
      // Ensure we don't leak if native returned something unexpected.
      if (bmpData) {
        Dicom_FreeBuffer(static_cast<void*>(bmpData));
        bmpData = nullptr;
      }
      validation.Set("isValid", Napi::Boolean::New(env, false));
      validation.Set("message", Napi::String::New(env, "Thumbnail generation failed"));
    }
  } catch (...) {
    if (bmpData) {
      Dicom_FreeBuffer(static_cast<void*>(bmpData));
      bmpData = nullptr;
    }
    validation.Set("isValid", Napi::Boolean::New(env, false));
    validation.Set("message", Napi::String::New(env, "Thumbnail generation threw exception"));
  }
  
  /* 
  void* pixelData = nullptr;
  try {
    pixelData = Dicom_GetPixelData(handle, &pixelWidth, &pixelHeight, &pixelDepth);
    
    if (pixelData && pixelWidth > 0 && pixelHeight > 0) {
      // 像素数据处理...
    }
  } catch (const std::exception& e) {
    validation.Set("error", Napi::String::New(env, std::string("Exception: ") + e.what()));
  }
  
  if (pixelData) {
    delete[] static_cast<char*>(pixelData);
  }
  */

  // 10. 清理资源
  Dicom_DestroyReader(handle);

  // 11. 构建返回对象
  Napi::Object resultObj = Napi::Object::New(env);
  resultObj.Set("fileCount", Napi::Number::New(env, fileCount));
  resultObj.Set("width", Napi::Number::New(env, pixelWidth > 0 ? pixelWidth : width));
  resultObj.Set("height", Napi::Number::New(env, pixelHeight > 0 ? pixelHeight : height));
  
  // Tag 信息 - DICOM 标签可能是 GBK 编码，需要转换为 UTF-8
  Napi::Object tags = Napi::Object::New(env);
  tags.Set("patientName", Napi::String::New(env, AnsiToUtf8(patientName)));
  tags.Set("patientID", Napi::String::New(env, AnsiToUtf8(patientID)));
  tags.Set("studyDate", Napi::String::New(env, studyDate));  // 日期是纯 ASCII，不需要转换
  tags.Set("studyDescription", Napi::String::New(env, AnsiToUtf8(studyDescription)));
  tags.Set("seriesDescription", Napi::String::New(env, AnsiToUtf8(seriesDescription)));
  tags.Set("modality", Napi::String::New(env, modality));  // modality 通常是纯 ASCII

  // Preserve raw strings for debugging/inspection.
  tags.Set("windowCenterRaw", Napi::String::New(env, windowCenterRaw));
  tags.Set("windowWidthRaw", Napi::String::New(env, windowWidthRaw));

  // Provide parsed numeric defaults (if available).
  float windowCenter = 0.0f;
  float windowWidth = 0.0f;
  const bool hasCenter = TryParseFirstNumber(windowCenterRaw, &windowCenter);
  const bool hasWidth = TryParseFirstNumber(windowWidthRaw, &windowWidth);
  if (hasCenter) {
    tags.Set("windowCenter", Napi::Number::New(env, windowCenter));
  }
  if (hasWidth) {
    tags.Set("windowWidth", Napi::Number::New(env, windowWidth));
  }
  resultObj.Set("tags", tags);
  
  // 缩略图数据
  resultObj.Set("thumbnail", thumbnailBuffer);
  
  // 数据验证信息
  resultObj.Set("validation", validation);
  
  return resultObj;
}

// ==================== 模块初始化 ====================

void InitDicomModule(Napi::Env env, Napi::Object& exports) {
  // 导出独立函数
  exports.Set("loadDicomFolderInfo", Napi::Function::New(env, LoadDicomFolderInfo));
}


