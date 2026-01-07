#include "WindowManager.h"
#include <windowsx.h>

const wchar_t* WindowManager::WINDOW_CLASS_NAME = L"MedicalImageViewer";
bool WindowManager::classRegistered_ = false;

WindowManager::WindowManager() {
}

WindowManager::~WindowManager() {
    DestroyWindow();
}

bool WindowManager::CreateWindow(int width, int height, const wchar_t* title, HWND parent) {
    if (created_) {
        return false;
    }

    width_ = width;
    height_ = height;

    // 注册窗口类（只需一次）
    if (!classRegistered_) {
        WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = WINDOW_CLASS_NAME;
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

        if (!RegisterClassExW(&wc)) {
            return false;
        }
        classRegistered_ = true;
    }

    // 计算窗口大小（包括边框）
    DWORD style = parent ? WS_CHILD | WS_VISIBLE : WS_OVERLAPPEDWINDOW;
    RECT rect = { 0, 0, width, height };
    AdjustWindowRect(&rect, style, FALSE);

    // 创建窗口
    hwnd_ = CreateWindowExW(
        0,
        WINDOW_CLASS_NAME,
        title,
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        parent,
        nullptr,
        GetModuleHandle(nullptr),
        this  // 传递this指针
    );

    if (!hwnd_) {
        return false;
    }

    created_ = true;
    return true;
}

void WindowManager::DestroyWindow() {
    if (hwnd_) {
        ::DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
    created_ = false;
}

bool WindowManager::ProcessMessages() {
    if (!hwnd_) {
        return false;
    }

    MSG msg;
    bool hadMessage = false;

    // 处理所有待处理的消息（非阻塞）
    while (PeekMessageW(&msg, hwnd_, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        hadMessage = true;
    }

    return hadMessage;
}

void WindowManager::Show(bool visible) {
    if (hwnd_) {
        ShowWindow(hwnd_, visible ? SW_SHOW : SW_HIDE);
    }
}

void WindowManager::SetPosition(int x, int y) {
    if (hwnd_) {
        SetWindowPos(hwnd_, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }
}

LRESULT CALLBACK WindowManager::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    WindowManager* manager = nullptr;

    if (msg == WM_CREATE) {
        // 从CreateWindowEx传递的this指针
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        manager = reinterpret_cast<WindowManager*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(manager));
    } else {
        manager = reinterpret_cast<WindowManager*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (manager) {
        return manager->HandleMessage(msg, wParam, lParam);
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT WindowManager::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_SIZE: {
        width_ = LOWORD(lParam);
        height_ = HIWORD(lParam);
        if (resizeCallback_) {
            resizeCallback_(width_, height_);
        }
        return 0;
    }

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN: {
        if (mouseCallback_) {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            int button = (msg == WM_LBUTTONDOWN) ? 0 : (msg == WM_RBUTTONDOWN) ? 1 : 2;
            mouseCallback_(x, y, button, 1);  // 1 = press
        }
        return 0;
    }

    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP: {
        if (mouseCallback_) {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            int button = (msg == WM_LBUTTONUP) ? 0 : (msg == WM_RBUTTONUP) ? 1 : 2;
            mouseCallback_(x, y, button, 0);  // 0 = release
        }
        return 0;
    }

    case WM_MOUSEMOVE: {
        if (mouseCallback_) {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            mouseCallback_(x, y, -1, 2);  // -1 = no button, 2 = move
        }
        return 0;
    }

    case WM_KEYDOWN:
    case WM_KEYUP: {
        if (keyCallback_) {
            int key = static_cast<int>(wParam);
            int action = (msg == WM_KEYDOWN) ? 1 : 0;
            keyCallback_(key, action);
        }
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd_, msg, wParam, lParam);
}
