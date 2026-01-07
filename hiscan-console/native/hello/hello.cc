#define NOMINMAX
#include <napi.h>
#include <windows.h>
#include <thread>
#include <string>
#include <vector>
#include <algorithm>

// 预览配置
int g_previewWidth = 640;
int g_previewHeight = 480;
constexpr int PREVIEW_BPP = 32; // 32位BGRA
std::string g_previewText = "hello world";

// 全局DIB缓冲区
HBITMAP g_hDib = nullptr;
std::vector<uint8_t> g_dibPixels;

// 创建DIBSection并返回其HDC
HDC CreatePreviewDIB(int width, int height, HBITMAP& outBitmap, std::vector<uint8_t>& outPixels) {
  BITMAPINFO bmi = {};
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = width;
  bmi.bmiHeader.biHeight = -height; // top-down
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = PREVIEW_BPP;
  bmi.bmiHeader.biCompression = BI_RGB;
  void* pBits = nullptr;
  HDC hdcScreen = GetDC(NULL);
  HBITMAP hDib = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
  ReleaseDC(NULL, hdcScreen);
  if (!hDib || !pBits) return nullptr;
  outBitmap = hDib;
  outPixels.assign(width * height * 4, 0);
  HDC hdcMem = CreateCompatibleDC(NULL);
  SelectObject(hdcMem, hDib);
  return hdcMem;
}

// GDI绘制到DIB
void DrawHelloWorldToDIB() {
  if (g_hDib) {
    DeleteObject(g_hDib);
    g_hDib = nullptr;
  }
  HDC hdc = CreatePreviewDIB(g_previewWidth, g_previewHeight, g_hDib, g_dibPixels);
  if (!hdc) return;
  RECT rect = { 0, 0, g_previewWidth, g_previewHeight };
  HBRUSH bgBrush = CreateSolidBrush(RGB(16, 24, 40));
  FillRect(hdc, &rect, bgBrush);
  DeleteObject(bgBrush);

  SetBkMode(hdc, TRANSPARENT);
  SetTextColor(hdc, RGB(255, 0, 0));
  HFONT hFont = CreateFontA(std::max(g_previewHeight / 6, 24), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
          DEFAULT_PITCH | FF_SWISS, "Arial");
  HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
  const std::string text = g_previewText.empty() ? std::string("hello world") : g_previewText;
  SIZE textSize = {};
  GetTextExtentPoint32A(hdc, text.c_str(), static_cast<int>(text.size()), &textSize);
  int x = (g_previewWidth - textSize.cx) / 2;
  int y = (g_previewHeight - textSize.cy) / 2;
  TextOutA(hdc, x, y, text.c_str(), static_cast<int>(text.size()));
  SelectObject(hdc, hOldFont);
  DeleteObject(hFont);
  GdiFlush();

  // 读取像素到g_dibPixels
  BITMAPINFO bmi = {};
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = g_previewWidth;
  bmi.bmiHeader.biHeight = -g_previewHeight;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = PREVIEW_BPP;
  bmi.bmiHeader.biCompression = BI_RGB;
  GetDIBits(hdc, g_hDib, 0, g_previewHeight, g_dibPixels.data(), &bmi, DIB_RGB_COLORS);
  DeleteDC(hdc);
}

HWND g_hwnd = nullptr;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (msg == WM_PAINT) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    SetTextColor(hdc, RGB(255, 0, 0));
    SetBkMode(hdc, TRANSPARENT);
    HFONT hFont = CreateFontA(48, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                  DEFAULT_PITCH | FF_SWISS, "Arial");
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    TextOutA(hdc, 60, 80, "hello world", 11);
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
    EndPaint(hwnd, &ps);
    return 0;
  }
  if (msg == WM_DESTROY) {
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProc(hwnd, msg, wParam, lParam);
}


// 兼容旧API，仍然弹窗（可选，后续可移除）
Napi::Value CreatePreviewWindow(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  int width = g_previewWidth;
  int height = g_previewHeight;
  std::string text = g_previewText;

  if (info.Length() >= 1) {
    if (info[0].IsObject()) {
      Napi::Object cfg = info[0].As<Napi::Object>();
      if (cfg.Has("width") && cfg.Get("width").IsNumber()) {
        width = std::max(1, cfg.Get("width").As<Napi::Number>().Int32Value());
      }
      if (cfg.Has("height") && cfg.Get("height").IsNumber()) {
        height = std::max(1, cfg.Get("height").As<Napi::Number>().Int32Value());
      }
      if (cfg.Has("text") && cfg.Get("text").IsString()) {
        text = cfg.Get("text").As<Napi::String>().Utf8Value();
      }
    } else if (info.Length() >= 2 && info[0].IsNumber() && info[1].IsNumber()) {
      width = std::max(1, info[0].As<Napi::Number>().Int32Value());
      height = std::max(1, info[1].As<Napi::Number>().Int32Value());
      if (info.Length() >= 3 && info[2].IsString()) {
        text = info[2].As<Napi::String>().Utf8Value();
      }
    }
  }

  bool sizeChanged = width != g_previewWidth || height != g_previewHeight;
  bool textChanged = text != g_previewText;
  if (sizeChanged) {
    g_previewWidth = width;
    g_previewHeight = height;
  }
  if (textChanged) {
    g_previewText = text;
  }

  if (sizeChanged || textChanged || !g_hDib) {
    DrawHelloWorldToDIB();
  }

  return Napi::String::New(env, "preview configured");
}

// Node-API导出像素buffer
Napi::Value GetPreviewBuffer(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  // 若未生成DIB，则先绘制一次
  if (g_dibPixels.empty()) {
    DrawHelloWorldToDIB();
  }
  // 创建ArrayBuffer并拷贝像素数据
  Napi::ArrayBuffer buffer = Napi::ArrayBuffer::New(env, g_dibPixels.size());
  memcpy(buffer.Data(), g_dibPixels.data(), g_dibPixels.size());
  // 用Uint8Array包装
  Napi::Uint8Array data = Napi::Uint8Array::New(env, g_dibPixels.size(), buffer, 0);
  Napi::Object result = Napi::Object::New(env);
  result.Set("width", g_previewWidth);
  result.Set("height", g_previewHeight);
  result.Set("data", data);
  result.Set("text", g_previewText);
  return result;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(Napi::String::New(env, "createPreviewWindow"), Napi::Function::New(env, CreatePreviewWindow));
  exports.Set(Napi::String::New(env, "getPreviewBuffer"), Napi::Function::New(env, GetPreviewBuffer));
  return exports;
}



NODE_API_MODULE(hello, Init)
