#include "pch.h"

#include "../Common/NativeInterfaces.h"

#include <cmath>
#include <memory>
#include <string>
#include <iostream>
#include <sstream>

// vcpkg OpenGL/GLEW
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <GL/glew.h>

#if !defined(_WIN32)
#include <GLFW/glfw3.h>
#endif

#ifdef _DEBUG
#pragma comment(lib, "glew32d.lib")
#else
#pragma comment(lib, "glew32.lib")
#endif
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GlU32.lib")


struct RendererContext
{
    bool initialized = false;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    float lastSlice = 0.0f;
    std::uint32_t frameCount = 0;
};

static thread_local std::string g_lastError;

static void SetLastError(const std::string& message)
{
    g_lastError = message;
}

static NativeResult ValidateHandle(RendererHandle handle, RendererContext*& outContext)
{
    if (!handle)
    {
        SetLastError("Renderer handle is null");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    outContext = static_cast<RendererContext*>(handle);
    return NATIVE_OK;
}

extern "C" __declspec(dllexport) const char* Renderer_GetLastError()
{
    return g_lastError.c_str();
}

extern "C" __declspec(dllexport) RendererHandle Renderer_Create()
{
    try
    {
        RendererContext* context = new RendererContext();
        std::cout << "[Renderer DLL] Renderer_Create called" << std::endl;
        std::cout << "[Renderer DLL] Renderer_Create success" << std::endl;
        return static_cast<RendererHandle>(context);
    }
    catch (const std::bad_alloc&)
    {
        SetLastError("Failed to allocate renderer context");
        std::cout << "[Renderer DLL] Renderer_Create failed" << std::endl;
        return nullptr;
    }
}

extern "C" __declspec(dllexport) void Renderer_Destroy(RendererHandle handle)
{
    if (!handle)
    {
        return;
    }

    RendererContext* context = nullptr;
    if (ValidateHandle(handle, context) != NATIVE_OK)
    {
        return;
    }

    std::cout << "[Renderer DLL] Renderer_Destroy called" << std::endl;
    if (!handle)
    {
        std::cout << "[Renderer DLL] Renderer_Destroy: handle is nullptr" << std::endl;
        return;
    }

    delete context;
    std::cout << "[Renderer DLL] Renderer_Destroy success" << std::endl;
}

extern "C" __declspec(dllexport) NativeResult Renderer_Initialize(RendererHandle handle, std::uint32_t width, std::uint32_t height)
{
    RendererContext* context = nullptr;
    NativeResult result = ValidateHandle(handle, context);
    if (result != NATIVE_OK)
    {
        return result;
    }

    if (width == 0 || height == 0)
    {
        SetLastError("Renderer surface dimensions must be non-zero");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    context->width = width;
    context->height = height;
    context->initialized = true;
    context->lastSlice = 0.0f;
    context->frameCount = 0;

    return NATIVE_OK;
}

extern "C" __declspec(dllexport) NativeResult Renderer_RenderSlice(RendererHandle handle, float sliceLocation)
{
    RendererContext* context = nullptr;
    NativeResult result = ValidateHandle(handle, context);
    if (result != NATIVE_OK)
    {
        return result;
    }

    if (!context->initialized)
    {
        SetLastError("Renderer not initialized");
        return NATIVE_E_NOT_INITIALIZED;
    }

    if (!std::isfinite(sliceLocation))
    {
        SetLastError("Slice location is not finite");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    context->lastSlice = sliceLocation;
    context->frameCount += 1;

    return NATIVE_OK;
}

extern "C" __declspec(dllexport) NativeResult Renderer_RunSelfTest(RendererDiagnostics* diagnostics)
{
    if (diagnostics == nullptr)
    {
        SetLastError("Diagnostics pointer is null");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    // Create a temporary OpenGL context to verify GLEW/OpenGL wiring.
#if defined(_WIN32)
    WNDCLASSW wc{};
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"RendererSelfTestWGL";
    RegisterClassW(&wc); // ok if already registered

    HWND hwnd = CreateWindowW(wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW,
                              0, 0, 1, 1, nullptr, nullptr, wc.hInstance, nullptr);
    if (!hwnd) {
        SetLastError("DllRenderer: Win32 window creation failed");
        return NATIVE_E_INTERNAL_ERROR;
    }

    HDC hdc = GetDC(hwnd);
    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pf = ChoosePixelFormat(hdc, &pfd);
    if (!pf || !SetPixelFormat(hdc, pf, &pfd)) {
        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);
        SetLastError("DllRenderer: SetPixelFormat failed");
        return NATIVE_E_INTERNAL_ERROR;
    }

    HGLRC rc = wglCreateContext(hdc);
    if (!rc || !wglMakeCurrent(hdc, rc)) {
        if (rc) wglDeleteContext(rc);
        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);
        SetLastError("DllRenderer: wglCreateContext/wglMakeCurrent failed");
        return NATIVE_E_INTERNAL_ERROR;
    }

    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK) {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(rc);
        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);
        SetLastError("DllRenderer: GLEW init failed");
        return NATIVE_E_INTERNAL_ERROR;
    }

    const GLubyte* glVersion = glGetString(GL_VERSION);
    const GLubyte* glewVersion = glewGetString(GLEW_VERSION);
    std::ostringstream oss;
    oss << "DllRenderer: OpenGL=" << (glVersion ? (const char*)glVersion : "unknown")
        << ", GLEW=" << (glewVersion ? (const char*)glewVersion : "unknown");
    SetLastError(oss.str());

    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(rc);
    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
#else
    if (!glfwInit())
    {
        SetLastError("DllRenderer: GLFW init failed");
        return NATIVE_E_INTERNAL_ERROR;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(640, 480, "OffscreenTest", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        SetLastError("DllRenderer: GLFW window creation failed");
        return NATIVE_E_INTERNAL_ERROR;
    }

    glfwMakeContextCurrent(window);
    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK)
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        SetLastError("DllRenderer: GLEW init failed");
        return NATIVE_E_INTERNAL_ERROR;
    }

    const GLubyte* glVersion = glGetString(GL_VERSION);
    const GLubyte* glewVersion = glewGetString(GLEW_VERSION);
    std::ostringstream oss;
    oss << "DllRenderer: OpenGL=" << (glVersion ? (const char*)glVersion : "unknown")
        << ", GLEW=" << (glewVersion ? (const char*)glewVersion : "unknown")
        << ", GLFW=" << glfwGetVersionString();
    SetLastError(oss.str());

    glfwDestroyWindow(window);
    glfwTerminate();
#endif

    RendererHandle handle = Renderer_Create();
    if (!handle)
    {
        return NATIVE_E_INTERNAL_ERROR;
    }

    NativeResult initResult = Renderer_Initialize(handle, 640, 480);
    if (initResult != NATIVE_OK)
    {
        Renderer_Destroy(handle);
        return initResult;
    }

    for (int frame = 0; frame < 5; ++frame)
    {
        const float sliceLocation = 0.5f * static_cast<float>(frame);
        NativeResult renderResult = Renderer_RenderSlice(handle, sliceLocation);
        if (renderResult != NATIVE_OK)
        {
            Renderer_Destroy(handle);
            return renderResult;
        }
    }

    RendererContext* context = static_cast<RendererContext*>(handle);
    diagnostics->surfaceWidth = context->width;
    diagnostics->surfaceHeight = context->height;
    diagnostics->lastSliceLocation = context->lastSlice;
    diagnostics->renderedFrameCount = context->frameCount;

    Renderer_Destroy(handle);
    return NATIVE_OK;
}
