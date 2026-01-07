#include <napi.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <gl/GL.h>

#include <unordered_map>
#include <cstdint>
#include <string>
#include <cstring>

namespace {

struct WindowState {
  HWND hwnd = nullptr;
  HDC hdc = nullptr;
  HGLRC hglrc = nullptr;
  UINT_PTR timerId = 0;
  int widthPx = 1;
  int heightPx = 1;
  bool transparentInput = false;

  // Mouse interaction + debug overlay
  bool trackingLeave = false;
  bool leftDown = false;
  bool rightDown = false;
  int mouseX = 0;
  int mouseY = 0;
  int lastX = 0;
  int lastY = 0;
  float panX = 0.0f;
  float panY = 0.0f;
  float rotX = 20.0f;
  float rotY = -30.0f;
  float zoom = 1.0f;
  GLuint fontBase = 0;

  uint64_t paintCount = 0;
};

std::unordered_map<HWND, WindowState*> g_windows;
ATOM g_classAtom = 0;
constexpr UINT WM_OGL_DBLCLICK = WM_APP + 0x233;

UINT GetDpiForHwndSafe(HWND hwnd) {
  HMODULE user32 = ::GetModuleHandleW(L"user32.dll");
  if (user32) {
    using Fn = UINT(WINAPI*)(HWND);
    auto fn = reinterpret_cast<Fn>(::GetProcAddress(user32, "GetDpiForWindow"));
    if (fn) {
      UINT dpi = fn(hwnd);
      return dpi ? dpi : 96;
    }
  }
  return 96;
}

int DipToPx(HWND parent, int dip) {
  UINT dpi = GetDpiForHwndSafe(parent);
  return ::MulDiv(dip, static_cast<int>(dpi), 96);
}

void EnsureFontLists(WindowState* st) {
  if (!st || st->fontBase) return;
  if (!st->hdc || !st->hglrc) return;

  ::wglMakeCurrent(st->hdc, st->hglrc);
  st->fontBase = ::glGenLists(96);

  // Create simple ASCII glyph bitmaps (32..127)
  HFONT font = ::CreateFontW(
      -14, 0, 0, 0,
      FW_NORMAL,
      FALSE, FALSE, FALSE,
      DEFAULT_CHARSET,
      OUT_DEFAULT_PRECIS,
      CLIP_DEFAULT_PRECIS,
      ANTIALIASED_QUALITY,
      FF_DONTCARE,
      L"Consolas");

  HGDIOBJ old = nullptr;
  if (font) {
    old = ::SelectObject(st->hdc, font);
  }
  ::wglUseFontBitmapsW(st->hdc, 32, 96, st->fontBase);
  if (font) {
    ::SelectObject(st->hdc, old);
    ::DeleteObject(font);
  }
  ::wglMakeCurrent(nullptr, nullptr);
}

Napi::Object RectToObj(Napi::Env env, const RECT& r) {
  auto o = Napi::Object::New(env);
  o.Set("left", Napi::Number::New(env, r.left));
  o.Set("top", Napi::Number::New(env, r.top));
  o.Set("right", Napi::Number::New(env, r.right));
  o.Set("bottom", Napi::Number::New(env, r.bottom));
  o.Set("width", Napi::Number::New(env, r.right - r.left));
  o.Set("height", Napi::Number::New(env, r.bottom - r.top));
  return o;
}

void RenderText(WindowState* st, int xPx, int yPx, const char* text) {
  if (!st || !st->fontBase || !text) return;
  ::glRasterPos2i(xPx, yPx);
  ::glListBase(st->fontBase - 32);
  ::glCallLists(static_cast<GLsizei>(::strlen(text)), GL_UNSIGNED_BYTE, text);
}

void TrackLeave(HWND hwnd, WindowState* st) {
  if (!st || st->trackingLeave) return;
  TRACKMOUSEEVENT tme{};
  tme.cbSize = sizeof(tme);
  tme.dwFlags = TME_LEAVE;
  tme.hwndTrack = hwnd;
  if (::TrackMouseEvent(&tme)) {
    st->trackingLeave = true;
  }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  auto it = g_windows.find(hwnd);
  WindowState* st = (it != g_windows.end()) ? it->second : nullptr;

  switch (msg) {
    case WM_MOUSEACTIVATE: {
      // Never activate/focus the OpenGL child window.
      // Prevents focus/z-order fights with the Electron main window during resize/maximize.
      return MA_NOACTIVATE;
    }
    case WM_ERASEBKGND: {
      // Prevent GDI background erasing to reduce flicker.
      return 1;
    }
    case WM_NCCREATE:
      return TRUE;

    case WM_SIZE: {
      if (st) {
        st->widthPx = LOWORD(lParam) ? static_cast<int>(LOWORD(lParam)) : 1;
        st->heightPx = HIWORD(lParam) ? static_cast<int>(HIWORD(lParam)) : 1;
        if (st->hglrc && st->hdc) {
          ::wglMakeCurrent(st->hdc, st->hglrc);
          ::glViewport(0, 0, st->widthPx, st->heightPx);
          ::wglMakeCurrent(nullptr, nullptr);
        }
      }
      return 0;
    }

    case WM_MOUSEMOVE: {
      if (st) {
        TrackLeave(hwnd, st);
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        st->mouseX = x;
        st->mouseY = y;

        int dx = x - st->lastX;
        int dy = y - st->lastY;
        st->lastX = x;
        st->lastY = y;

        if (st->leftDown) {
          // Pan in normalized-ish units
          st->panX += static_cast<float>(dx) / 200.0f;
          st->panY -= static_cast<float>(dy) / 200.0f;
        }
        if (st->rightDown) {
          st->rotY += static_cast<float>(dx) * 0.5f;
          st->rotX += static_cast<float>(dy) * 0.5f;
        }
        ::InvalidateRect(hwnd, nullptr, FALSE);
      }
      return 0;
    }

    case WM_MOUSELEAVE: {
      if (st) {
        st->trackingLeave = false;
        ::InvalidateRect(hwnd, nullptr, FALSE);
      }
      return 0;
    }

    case WM_LBUTTONDOWN: {
      if (st) {
        st->leftDown = true;
        st->lastX = GET_X_LPARAM(lParam);
        st->lastY = GET_Y_LPARAM(lParam);
        ::SetCapture(hwnd);
        ::InvalidateRect(hwnd, nullptr, FALSE);
      }
      return 0;
    }

    case WM_LBUTTONUP: {
      if (st) {
        st->leftDown = false;
        if (!st->rightDown) ::ReleaseCapture();
        ::InvalidateRect(hwnd, nullptr, FALSE);
      }
      return 0;
    }

    case WM_RBUTTONDOWN: {
      if (st) {
        st->rightDown = true;
        st->lastX = GET_X_LPARAM(lParam);
        st->lastY = GET_Y_LPARAM(lParam);
        ::SetCapture(hwnd);
        ::InvalidateRect(hwnd, nullptr, FALSE);
      }
      return 0;
    }

    case WM_RBUTTONUP: {
      if (st) {
        st->rightDown = false;
        if (!st->leftDown) ::ReleaseCapture();
        ::InvalidateRect(hwnd, nullptr, FALSE);
      }
      return 0;
    }

    case WM_LBUTTONDBLCLK: {
      // Forward double-click to parent (Electron hooks this custom message).
      HWND parent = ::GetParent(hwnd);
      if (parent) {
        ::PostMessageW(parent, WM_OGL_DBLCLICK,
                       reinterpret_cast<WPARAM>(hwnd), 0);
      }
      return 0;
    }

    case WM_MOUSEWHEEL: {
      if (st) {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        // Wheel up -> zoom in
        st->zoom *= (delta > 0) ? 1.08f : 0.92f;
        if (st->zoom < 0.05f) st->zoom = 0.05f;
        if (st->zoom > 20.0f) st->zoom = 20.0f;
        ::InvalidateRect(hwnd, nullptr, FALSE);
      }
      return 0;
    }

    case WM_TIMER: {
      if (st) {
        ::InvalidateRect(hwnd, nullptr, FALSE);
      }
      return 0;
    }

    case WM_PAINT: {
      if (!st || !st->hglrc || !st->hdc) {
        PAINTSTRUCT ps;
        ::BeginPaint(hwnd, &ps);
        ::EndPaint(hwnd, &ps);
        return 0;
      }

      PAINTSTRUCT ps;
      ::BeginPaint(hwnd, &ps);

      ::wglMakeCurrent(st->hdc, st->hglrc);

      st->paintCount++;

        EnsureFontLists(st);

      static float t = 0.0f;
      t += 0.01f;
      float r = (sinf(t) + 1.0f) * 0.5f;
      float g = (sinf(t + 2.094f) + 1.0f) * 0.5f;
      float b = (sinf(t + 4.188f) + 1.0f) * 0.5f;

      ::glViewport(0, 0, st->widthPx, st->heightPx);

        ::glEnable(GL_DEPTH_TEST);
        ::glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        ::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Simple 3D-ish triangle with pan/rotate/zoom
        ::glMatrixMode(GL_PROJECTION);
        ::glLoadIdentity();
        float aspect = (st->heightPx > 0) ? (static_cast<float>(st->widthPx) / static_cast<float>(st->heightPx)) : 1.0f;
        const float zNear = 0.1f;
        const float zFar = 50.0f;
        const float f = 1.0f;
        ::glFrustum(-aspect * zNear * f, aspect * zNear * f, -zNear * f, zNear * f, zNear, zFar);

        ::glMatrixMode(GL_MODELVIEW);
        ::glLoadIdentity();
        ::glTranslatef(st->panX, st->panY, -2.8f * st->zoom);
        ::glRotatef(st->rotX, 1.0f, 0.0f, 0.0f);
        ::glRotatef(st->rotY, 0.0f, 1.0f, 0.0f);

        // Draw a colorful triangle
        ::glBegin(GL_TRIANGLES);
        ::glColor3f(1.0f, 0.2f, 0.2f);
        ::glVertex3f(-0.6f, -0.4f, 0.0f);
        ::glColor3f(0.2f, 1.0f, 0.2f);
        ::glVertex3f(0.6f, -0.4f, 0.0f);
        ::glColor3f(0.2f, 0.6f, 1.0f);
        ::glVertex3f(0.0f, 0.7f, 0.0f);
        ::glEnd();

        // Overlay debug text in top-left
        ::glDisable(GL_DEPTH_TEST);
        ::glMatrixMode(GL_PROJECTION);
        ::glLoadIdentity();
        ::glOrtho(0, st->widthPx, 0, st->heightPx, -1, 1);
        ::glMatrixMode(GL_MODELVIEW);
        ::glLoadIdentity();
        ::glColor3f(0.85f, 0.95f, 1.0f);

        char buf[256];
        // Show client coords (px) and basic state.
        ::wsprintfA(
          buf,
          "x=%d y=%d  L=%d R=%d  zoom=%.2f  pan(%.2f,%.2f)  rot(%.1f,%.1f)",
          st->mouseX,
          st->mouseY,
          st->leftDown ? 1 : 0,
          st->rightDown ? 1 : 0,
          st->zoom,
          st->panX,
          st->panY,
          st->rotX,
          st->rotY);
        RenderText(st, 8, st->heightPx - 18, buf);

        // Subtle animated color bar (to confirm repaint)
        ::glBegin(GL_QUADS);
        ::glColor4f(r, g, b, 1.0f);
        ::glVertex2f(0.0f, 0.0f);
        ::glVertex2f(static_cast<float>(st->widthPx), 0.0f);
        ::glVertex2f(static_cast<float>(st->widthPx), 6.0f);
        ::glVertex2f(0.0f, 6.0f);
        ::glEnd();

      ::SwapBuffers(st->hdc);
      ::wglMakeCurrent(nullptr, nullptr);

      ::EndPaint(hwnd, &ps);
      return 0;
    }

    case WM_DESTROY: {
      if (st) {
        if (st->timerId) {
          ::KillTimer(hwnd, st->timerId);
          st->timerId = 0;
        }
        if (st->fontBase) {
          ::wglMakeCurrent(st->hdc, st->hglrc);
          ::glDeleteLists(st->fontBase, 96);
          st->fontBase = 0;
          ::wglMakeCurrent(nullptr, nullptr);
        }
        if (st->hglrc) {
          ::wglMakeCurrent(nullptr, nullptr);
          ::wglDeleteContext(st->hglrc);
          st->hglrc = nullptr;
        }
        if (st->hdc) {
          ::ReleaseDC(hwnd, st->hdc);
          st->hdc = nullptr;
        }
        g_windows.erase(hwnd);
        delete st;
      }
      return 0;
    }

    default:
      break;
  }

  return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}

void EnsureWindowClass() {
  if (g_classAtom) return;

  WNDCLASSW wc{};
  wc.style = CS_OWNDC | CS_DBLCLKS;
  wc.lpfnWndProc = WndProc;
  wc.hInstance = ::GetModuleHandleW(nullptr);
  wc.lpszClassName = L"HiscanOpenGLChildWindow";

  g_classAtom = ::RegisterClassW(&wc);
}

void SetupPixelFormat(HDC hdc) {
  PIXELFORMATDESCRIPTOR pfd{};
  pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
  pfd.nVersion = 1;
  pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.cColorBits = 32;
  pfd.cAlphaBits = 8;
  pfd.cDepthBits = 24;
  pfd.cStencilBits = 8;
  pfd.iLayerType = PFD_MAIN_PLANE;

  int pf = ::ChoosePixelFormat(hdc, &pfd);
  ::SetPixelFormat(hdc, pf, &pfd);
}

HWND BufferToHwnd(const Napi::Value& v) {
  if (!v.IsBuffer()) return nullptr;
  auto buf = v.As<Napi::Buffer<uint8_t>>();
  if (buf.Length() < sizeof(void*)) return nullptr;
  void* ptr = *reinterpret_cast<void**>(buf.Data());
  return reinterpret_cast<HWND>(ptr);
}

HWND BigIntToHwnd(const Napi::Value& v) {
  if (!v.IsBigInt()) return nullptr;
  bool lossless = false;
  uint64_t val = v.As<Napi::BigInt>().Uint64Value(&lossless);
  if (!lossless) return nullptr;
  return reinterpret_cast<HWND>(static_cast<uintptr_t>(val));
}

Napi::Value CreateChildWindow(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (info.Length() < 5) {
    Napi::TypeError::New(env, "Expected (parentHwndBuffer, x, y, width, height, [transparentInput])").ThrowAsJavaScriptException();
    return env.Null();
  }

  HWND parent = BufferToHwnd(info[0]);
  if (!parent) {
    Napi::TypeError::New(env, "Invalid parent hwnd buffer").ThrowAsJavaScriptException();
    return env.Null();
  }

  // x/y are expected to be SCREEN coordinates in DIPs (CSS px). We'll convert to
  // parent client pixels using ScreenToClient for correct placement.
  int xScreenDip = info[1].As<Napi::Number>().Int32Value();
  int yScreenDip = info[2].As<Napi::Number>().Int32Value();
  int wDip = info[3].As<Napi::Number>().Int32Value();
  int hDip = info[4].As<Napi::Number>().Int32Value();
  bool transparentInput = false;
  if (info.Length() >= 6 && info[5].IsBoolean()) {
    transparentInput = info[5].As<Napi::Boolean>().Value();
  }

  EnsureWindowClass();

  int xScreenPx = DipToPx(parent, xScreenDip);
  int yScreenPx = DipToPx(parent, yScreenDip);
  int w = DipToPx(parent, wDip);
  int h = DipToPx(parent, hDip);
  if (w < 1) w = 1;
  if (h < 1) h = 1;

  POINT pt{ xScreenPx, yScreenPx };
  ::ScreenToClient(parent, &pt);
  int x = pt.x;
  int y = pt.y;

  DWORD exStyle = 0;
  if (transparentInput) {
    exStyle |= WS_EX_TRANSPARENT;
  }
  // Don't let the child HWND take activation/focus.
  exStyle |= WS_EX_NOACTIVATE;

  HWND hwnd = ::CreateWindowExW(
      exStyle,
      reinterpret_cast<LPCWSTR>(MAKEINTATOM(g_classAtom)),
      L"",
      WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
      x, y, w, h,
      parent,
      nullptr,
      ::GetModuleHandleW(nullptr),
      nullptr);

  if (!hwnd) {
    Napi::Error::New(env, "CreateWindowExW failed").ThrowAsJavaScriptException();
    return env.Null();
  }

  auto* st = new WindowState();
  st->hwnd = hwnd;
  st->transparentInput = transparentInput;
  st->widthPx = w;
  st->heightPx = h;

  st->hdc = ::GetDC(hwnd);
  SetupPixelFormat(st->hdc);
  st->hglrc = ::wglCreateContext(st->hdc);
  ::wglMakeCurrent(st->hdc, st->hglrc);
  ::glViewport(0, 0, st->widthPx, st->heightPx);
  ::wglMakeCurrent(nullptr, nullptr);

  st->timerId = ::SetTimer(hwnd, 1, 16, nullptr);

  // Avoid changing Z-order during layout; it can cause flicker and focus fights.
  ::SetWindowPos(hwnd, nullptr, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);

  g_windows[hwnd] = st;

  uint64_t handle = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hwnd));
  return Napi::BigInt::New(env, handle);
}

Napi::Value CreateStandaloneWindow(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  int w = 900;
  int h = 650;
  if (info.Length() >= 2) {
    if (!info[0].IsNumber() || !info[1].IsNumber()) {
      Napi::TypeError::New(env, "Expected (width, height)").ThrowAsJavaScriptException();
      return env.Null();
    }
    w = info[0].As<Napi::Number>().Int32Value();
    h = info[1].As<Napi::Number>().Int32Value();
  }
  if (w < 100) w = 100;
  if (h < 100) h = 100;

  EnsureWindowClass();

  // Create a top-level window to validate OpenGL rendering/input independent of Electron.
  HWND hwnd = ::CreateWindowExW(
      0,
      reinterpret_cast<LPCWSTR>(MAKEINTATOM(g_classAtom)),
      L"OpenGL Standalone Test",
      WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
      120,
      120,
      w,
      h,
      nullptr,
      nullptr,
      ::GetModuleHandleW(nullptr),
      nullptr);

  if (!hwnd) {
    Napi::Error::New(env, "CreateWindowExW (standalone) failed").ThrowAsJavaScriptException();
    return env.Null();
  }

  auto* st = new WindowState();
  st->hwnd = hwnd;
  st->transparentInput = false;
  st->widthPx = w;
  st->heightPx = h;

  st->hdc = ::GetDC(hwnd);
  SetupPixelFormat(st->hdc);
  st->hglrc = ::wglCreateContext(st->hdc);
  ::wglMakeCurrent(st->hdc, st->hglrc);
  ::glViewport(0, 0, st->widthPx, st->heightPx);
  ::wglMakeCurrent(nullptr, nullptr);

  st->timerId = ::SetTimer(hwnd, 1, 16, nullptr);
  g_windows[hwnd] = st;

  ::ShowWindow(hwnd, SW_SHOW);
  ::UpdateWindow(hwnd);

  uint64_t handle = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(hwnd));
  return Napi::BigInt::New(env, handle);
}

Napi::Value SetRect(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (info.Length() < 5) {
    Napi::TypeError::New(env, "Expected (hwndBigInt, x, y, width, height)").ThrowAsJavaScriptException();
    return env.Null();
  }

  HWND hwnd = BigIntToHwnd(info[0]);
  if (!hwnd) {
    Napi::TypeError::New(env, "Invalid hwnd").ThrowAsJavaScriptException();
    return env.Null();
  }

  HWND parent = ::GetParent(hwnd);
  int xScreenDip = info[1].As<Napi::Number>().Int32Value();
  int yScreenDip = info[2].As<Napi::Number>().Int32Value();
  int wDip = info[3].As<Napi::Number>().Int32Value();
  int hDip = info[4].As<Napi::Number>().Int32Value();

  int xScreenPx = parent ? DipToPx(parent, xScreenDip) : xScreenDip;
  int yScreenPx = parent ? DipToPx(parent, yScreenDip) : yScreenDip;
  int w = parent ? DipToPx(parent, wDip) : wDip;
  int h = parent ? DipToPx(parent, hDip) : hDip;
  if (w < 1) w = 1;
  if (h < 1) h = 1;

  int x = xScreenPx;
  int y = yScreenPx;
  if (parent) {
    POINT pt{ xScreenPx, yScreenPx };
    ::ScreenToClient(parent, &pt);
    x = pt.x;
    y = pt.y;
  }

  // Don't change Z-order while resizing/moving.
  ::SetWindowPos(hwnd, nullptr, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
  return Napi::Boolean::New(env, true);
}

Napi::Value SetTransparentInput(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (info.Length() < 2) {
    Napi::TypeError::New(env, "Expected (hwndBigInt, enabled)").ThrowAsJavaScriptException();
    return env.Null();
  }

  HWND hwnd = BigIntToHwnd(info[0]);
  bool enabled = info[1].As<Napi::Boolean>().Value();
  if (!hwnd) {
    Napi::TypeError::New(env, "Invalid hwnd").ThrowAsJavaScriptException();
    return env.Null();
  }

  LONG_PTR exStyle = ::GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
  if (enabled) exStyle |= WS_EX_TRANSPARENT;
  else exStyle &= ~static_cast<LONG_PTR>(WS_EX_TRANSPARENT);
  ::SetWindowLongPtrW(hwnd, GWL_EXSTYLE, exStyle);

  return Napi::Boolean::New(env, true);
}

Napi::Value GetWindowInfo(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (info.Length() < 1) {
    Napi::TypeError::New(env, "Expected (hwndBigInt)").ThrowAsJavaScriptException();
    return env.Null();
  }

  HWND hwnd = BigIntToHwnd(info[0]);
  auto out = Napi::Object::New(env);
  out.Set("isWindow", Napi::Boolean::New(env, hwnd && ::IsWindow(hwnd)));
  if (!hwnd || !::IsWindow(hwnd)) {
    return out;
  }

  out.Set("isVisible", Napi::Boolean::New(env, ::IsWindowVisible(hwnd)));
  HWND parent = ::GetParent(hwnd);
  out.Set("parentHwnd", Napi::BigInt::New(env, static_cast<uint64_t>(reinterpret_cast<uintptr_t>(parent))));

  LONG_PTR style = ::GetWindowLongPtrW(hwnd, GWL_STYLE);
  LONG_PTR exStyle = ::GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
  out.Set("style", Napi::BigInt::New(env, static_cast<uint64_t>(style)));
  out.Set("exStyle", Napi::BigInt::New(env, static_cast<uint64_t>(exStyle)));

  RECT wr{};
  RECT cr{};
  if (::GetWindowRect(hwnd, &wr)) {
    out.Set("windowRect", RectToObj(env, wr));
  }
  if (::GetClientRect(hwnd, &cr)) {
    auto c = Napi::Object::New(env);
    c.Set("width", Napi::Number::New(env, cr.right - cr.left));
    c.Set("height", Napi::Number::New(env, cr.bottom - cr.top));
    out.Set("client", c);
  }

  auto it = g_windows.find(hwnd);
  if (it != g_windows.end() && it->second) {
    out.Set("paintCount", Napi::BigInt::New(env, it->second->paintCount));
    out.Set("mouseX", Napi::Number::New(env, it->second->mouseX));
    out.Set("mouseY", Napi::Number::New(env, it->second->mouseY));
    out.Set("leftDown", Napi::Boolean::New(env, it->second->leftDown));
    out.Set("rightDown", Napi::Boolean::New(env, it->second->rightDown));
    out.Set("zoom", Napi::Number::New(env, it->second->zoom));
  }

  return out;
}

Napi::Value SetVisible(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (info.Length() < 2) {
    Napi::TypeError::New(env, "Expected (hwndBigInt, visible)").ThrowAsJavaScriptException();
    return env.Null();
  }

  HWND hwnd = BigIntToHwnd(info[0]);
  if (!hwnd) {
    Napi::TypeError::New(env, "Invalid hwnd").ThrowAsJavaScriptException();
    return env.Null();
  }

  bool visible = info[1].As<Napi::Boolean>().Value();
  ::ShowWindow(hwnd, visible ? SW_SHOWNA : SW_HIDE);
  if (visible) {
    // IMPORTANT: when the Electron window loses/gains focus, Chromium may reshuffle
    // internal child HWNDs and the embedded OpenGL child can end up occluded.
    // Re-assert top Z-order within the parent without activating/focusing it.
    ::SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
  }
  return Napi::Boolean::New(env, true);
}

Napi::Value Destroy(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (info.Length() < 1) {
    Napi::TypeError::New(env, "Expected (hwndBigInt)").ThrowAsJavaScriptException();
    return env.Null();
  }

  HWND hwnd = BigIntToHwnd(info[0]);
  if (!hwnd) {
    return Napi::Boolean::New(env, false);
  }

  ::DestroyWindow(hwnd);
  return Napi::Boolean::New(env, true);
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set("createChildWindow", Napi::Function::New(env, CreateChildWindow));
  exports.Set("createStandaloneWindow", Napi::Function::New(env, CreateStandaloneWindow));
  exports.Set("setRect", Napi::Function::New(env, SetRect));
  exports.Set("setTransparentInput", Napi::Function::New(env, SetTransparentInput));
  exports.Set("setVisible", Napi::Function::New(env, SetVisible));
  exports.Set("getWindowInfo", Napi::Function::New(env, GetWindowInfo));
  exports.Set("destroy", Napi::Function::New(env, Destroy));
  return exports;
}

}  // namespace

NODE_API_MODULE(opengl_child, Init)
