#pragma once

#include <Windows.h>
#include <string>
#include <functional>

/// Win32窗口管理器 - 负责窗口创建、消息循环和事件分发
class WindowManager {
public:
    /// 鼠标事件回调类型
    using MouseCallback = std::function<void(int x, int y, int button, int action)>;
    /// 键盘事件回调类型  
    using KeyCallback = std::function<void(int key, int action)>;
    /// 窗口大小改变回调
    using ResizeCallback = std::function<void(int width, int height)>;

    WindowManager();
    ~WindowManager();

    /// 创建Win32窗口
    /// @param width 窗口宽度
    /// @param height 窗口高度
    /// @param title 窗口标题
    /// @param parent 父窗口句柄（用于嵌入到Electron）
    /// @return 成功返回true
    bool CreateWindow(int width, int height, const wchar_t* title, HWND parent = nullptr);

    /// 销毁窗口
    void DestroyWindow();

    /// 获取窗口句柄
    HWND GetHWND() const { return hwnd_; }

    /// 获取窗口尺寸
    void GetWindowSize(int& width, int& height) const {
        width = width_;
        height = height_;
    }

    /// 处理消息循环（非阻塞）
    /// @return 是否有消息处理
    bool ProcessMessages();

    /// 设置鼠标事件回调
    void SetMouseCallback(MouseCallback callback) { mouseCallback_ = callback; }

    /// 设置键盘事件回调  
    void SetKeyCallback(KeyCallback callback) { keyCallback_ = callback; }

    /// 设置窗口大小改变回调
    void SetResizeCallback(ResizeCallback callback) { resizeCallback_ = callback; }

    /// 显示/隐藏窗口
    void Show(bool visible);

    /// 设置窗口位置
    void SetPosition(int x, int y);

private:
    /// 窗口过程回调
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    /// 实际的消息处理
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

private:
    HWND hwnd_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    bool created_ = false;

    // 事件回调
    MouseCallback mouseCallback_;
    KeyCallback keyCallback_;
    ResizeCallback resizeCallback_;

    // 窗口类名
    static const wchar_t* WINDOW_CLASS_NAME;
    static bool classRegistered_;
};
