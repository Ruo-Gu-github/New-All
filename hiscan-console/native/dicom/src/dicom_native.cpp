#define NOMINMAX
#include <napi.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include <vtkDICOMImageReader.h>
#include <vtkExtractVOI.h>
#include <vtkImageData.h>
#include <vtkImageResample.h>
#include <vtkImageShiftScale.h>
#include <vtkNew.h>
#include <vtkPNGWriter.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkUnsignedCharArray.h>

#include <vtksys/SystemTools.hxx>

namespace {

constexpr double kEpsilon = 1e-6;

std::string Base64Encode(const unsigned char* input, std::size_t length) {
  static constexpr char kEncodingTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string output;
  output.reserve(((length + 2) / 3) * 4);

  std::size_t index = 0;
  while (index < length) {
    const uint32_t octetA = input[index++];
    const bool hasOctetB = index < length;
    const uint32_t octetB = hasOctetB ? input[index++] : 0u;
    const bool hasOctetC = index < length;
    const uint32_t octetC = hasOctetC ? input[index++] : 0u;

    const uint32_t triple = (octetA << 16) | (octetB << 8) | octetC;

    output.push_back(kEncodingTable[(triple >> 18) & 0x3Fu]);
    output.push_back(kEncodingTable[(triple >> 12) & 0x3Fu]);
    output.push_back(hasOctetB ? kEncodingTable[(triple >> 6) & 0x3Fu] : '=');
    output.push_back(hasOctetC ? kEncodingTable[triple & 0x3Fu] : '=');
  }

  return output;
}

std::string GuessSampleName(const vtkStringArray* fileNames) {
  if (!fileNames || fileNames->GetNumberOfValues() == 0) {
    return "Unnamed Sample";
  }
  const std::string firstPath = fileNames->GetValue(0);
  std::string name = vtksys::SystemTools::GetFilenameWithoutExtension(firstPath);
  if (name.empty()) {
    name = vtksys::SystemTools::GetFilenameName(firstPath);
  }
  if (name.empty()) {
    name = "Unnamed Sample";
  }
  return name;
}

vtkSmartPointer<vtkStringArray> BuildFileNameArray(const Napi::Env& env, const Napi::Array& files) {
  vtkNew<vtkStringArray> names;
  const uint32_t length = files.Length();
  for (uint32_t idx = 0; idx < length; ++idx) {
    Napi::Value entry = files.Get(idx);
    if (!entry.IsObject()) {
      Napi::TypeError::New(env, "files array contains invalid entry").ThrowAsJavaScriptException();
      return nullptr;
    }
    Napi::Object file = entry.As<Napi::Object>();
    if (!file.Has("path")) {
      Napi::TypeError::New(env, "file entries must provide a path field").ThrowAsJavaScriptException();
      return nullptr;
    }
    Napi::Value pathValue = file.Get("path");
    if (!pathValue.IsString()) {
      Napi::TypeError::New(env, "file path must be a string").ThrowAsJavaScriptException();
      return nullptr;
    }
    const std::string path = pathValue.As<Napi::String>().Utf8Value();
    if (path.empty()) {
      Napi::Error::New(env, "file path cannot be empty").ThrowAsJavaScriptException();
      return nullptr;
    }
    names->InsertNextValue(path.c_str());
  }
  if (names->GetNumberOfValues() == 0) {
    Napi::Error::New(env, "no DICOM file paths provided").ThrowAsJavaScriptException();
    return nullptr;
  }
  return names;
}

vtkSmartPointer<vtkImageData> ExtractPreviewSlice(vtkImageData* volume, const int extent[6]) {
  vtkNew<vtkExtractVOI> extractor;
  extractor->SetInputData(volume);
  extractor->SetVOI(extent[0], extent[1], extent[2], extent[3], extent[4], extent[4]);
  extractor->Update();
  return extractor->GetOutput();
}

vtkSmartPointer<vtkImageData> NormalizeToUChar(vtkImageData* image) {
  double range[2] = {0.0, 0.0};
  image->GetScalarRange(range);
  const double span = std::max(range[1] - range[0], kEpsilon);

  vtkNew<vtkImageShiftScale> shifter;
  shifter->SetInputData(image);
  shifter->SetShift(-range[0]);
  shifter->SetScale(255.0 / span);
  shifter->SetOutputScalarTypeToUnsignedChar();
  shifter->ClampOverflowOn();
  shifter->Update();
  return shifter->GetOutput();
}

vtkSmartPointer<vtkImageData> ResizeForThumbnail(vtkImageData* image, int width, int height) {
  const int target = 256;
  const double scale = std::min(
      static_cast<double>(target) / std::max(width, 1),
      static_cast<double>(target) / std::max(height, 1));

  if (scale >= 0.999 && scale <= 1.001) {
    return image;
  }

  vtkNew<vtkImageResample> resample;
  resample->SetInputData(image);
  resample->SetAxisMagnificationFactor(0, scale);
  resample->SetAxisMagnificationFactor(1, scale);
  resample->SetAxisMagnificationFactor(2, 1.0);
  resample->Update();
  return resample->GetOutput();
}

std::string EncodeThumbnail(vtkImageData* image) {
  vtkNew<vtkPNGWriter> writer;
  writer->SetInputData(image);
  writer->SetWriteToMemory(true);
  writer->Write();
  vtkUnsignedCharArray* buffer = writer->GetResult();
  if (!buffer) {
    return std::string();
  }
  const unsigned char* data = buffer->GetPointer(0);
  const vtkIdType length = buffer->GetNumberOfValues();
  if (!data || length <= 0) {
    return std::string();
  }
  std::string encoded = Base64Encode(data, static_cast<std::size_t>(length));
  return std::string("data:image/png;base64,") + encoded;
}

}  // namespace

Napi::Value LoadSeries(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (info.Length() < 1 || !info[0].IsObject()) {
    Napi::TypeError::New(env, "payload with files array is required").ThrowAsJavaScriptException();
    return env.Null();
  }

  Napi::Object payload = info[0].As<Napi::Object>();
  if (!payload.Has("files")) {
    Napi::TypeError::New(env, "payload missing files field").ThrowAsJavaScriptException();
    return env.Null();
  }

  Napi::Value filesValue = payload.Get("files");
  if (!filesValue.IsArray()) {
    Napi::TypeError::New(env, "files must be an array").ThrowAsJavaScriptException();
    return env.Null();
  }

  vtkSmartPointer<vtkStringArray> fileNames = BuildFileNameArray(env, filesValue.As<Napi::Array>());
  if (!fileNames) {
    return env.Null();
  }

  vtkNew<vtkDICOMImageReader> reader;
  reader->SetFileNames(fileNames);
  reader->Update();

  vtkImageData* volume = reader->GetOutput();
  if (!volume) {
    Napi::Error::New(env, "failed to read DICOM image data").ThrowAsJavaScriptException();
    return env.Null();
  }

  int extent[6] = {0, 0, 0, 0, 0, 0};
  volume->GetExtent(extent);
  const int width = extent[1] - extent[0] + 1;
  const int height = extent[3] - extent[2] + 1;
  const int sliceCount = extent[5] - extent[4] + 1;

  double spacing[3] = {1.0, 1.0, 1.0};
  volume->GetSpacing(spacing);

  vtkSmartPointer<vtkImageData> slice = ExtractPreviewSlice(volume, extent);
  if (!slice) {
    Napi::Error::New(env, "failed to extract preview slice").ThrowAsJavaScriptException();
    return env.Null();
  }

  vtkSmartPointer<vtkImageData> normalized = NormalizeToUChar(slice);
  vtkSmartPointer<vtkImageData> resized = ResizeForThumbnail(normalized, width, height);
  const std::string thumbnail = EncodeThumbnail(resized);

  Napi::Object result = Napi::Object::New(env);
  result.Set("sampleName", Napi::String::New(env, GuessSampleName(fileNames)));
  result.Set("width", Napi::Number::New(env, width));
  result.Set("height", Napi::Number::New(env, height));
  result.Set("sliceCount", Napi::Number::New(env, sliceCount));

  Napi::Array spacingArray = Napi::Array::New(env, 2);
  spacingArray.Set(uint32_t{0}, Napi::Number::New(env, spacing[0]));
  spacingArray.Set(uint32_t{1}, Napi::Number::New(env, spacing[1]));
  result.Set("pixelSpacing", spacingArray);
  result.Set("thumbnail", Napi::String::New(env, thumbnail));

  return result;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set("loadSeries", Napi::Function::New(env, LoadSeries));
  return exports;
}

NODE_API_MODULE(dicom_native, Init)
