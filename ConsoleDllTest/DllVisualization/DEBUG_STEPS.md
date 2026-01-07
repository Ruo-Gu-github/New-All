# APR窗口切换Tab卡死问题 - 调试步骤

## 问题现象
当APR窗口显示时，切换Tab就会卡死，Electron主线程无响应。

## 可能的原因
1. **渲染线程持续调用 RedrawWindow**（已通过 isHidden 标志修复）
2. **WM_PAINT 消息处理卡死**（OpenGL 上下文切换问题）
3. **窗口消息循环卡死**（Win32WindowProc 中某个消息阻塞）
4. **Z-order 刷新卡死**（SetWindowPos 或 RedrawWindow 同步等待）
5. **Window_PollEvents 卡死**（消息循环死锁）

## 测试步骤

### 测试1: 禁用所有渲染（WM_PAINT返回0）
**目的**: 确认是否是OpenGL渲染导致的卡死

修改 `Win32WindowProc` 中的 `WM_PAINT` 处理：
```cpp
case WM_PAINT:
    // TEST: 完全禁用渲染
    ValidateRect(hwnd, nullptr);
    return 0;
```

**期望结果**: 如果不卡死，说明问题在OpenGL渲染；如果还卡死，说明问题在窗口消息循环。

---

### 测试2: 禁用渲染线程
**目的**: 确认是否是渲染线程导致的问题

在 `Window_HideAllWindows` 前就注释掉渲染线程启动：
```cpp
// 在 main.ts 或前端代码中，不调用 startRenderLoop
// 或者在 C++ 中直接注释掉 Window_StartRenderLoop
```

**期望结果**: 如果不卡死，说明是渲染线程和窗口消息循环的竞争问题。

---

### 测试3: 禁用 WM_SIZE 中的 RedrawWindow
**目的**: 确认是否是窗口大小变化时的重绘导致卡死

```cpp
case WM_SIZE:
    if (ctx) {
        int newWidth = LOWORD(lParam);
        int newHeight = HIWORD(lParam);
        ctx->width = newWidth;
        ctx->height = newHeight;
        
        if (ctx->hdc && ctx->hglrc) {
            if (wglMakeCurrent(ctx->hdc, ctx->hglrc)) {
                glViewport(0, 0, newWidth, newHeight);
                wglMakeCurrent(nullptr, nullptr);
            }
        }
        
        // TEST: 注释掉立即重绘
        // RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
    }
    return 0;
```

---

### 测试4: 禁用 Window_RefreshZOrder 中的 RedrawWindow
**目的**: 确认是否是Z-order刷新时的重绘导致卡死

在 `Window_RefreshZOrder` 函数末尾：
```cpp
if (hidden) {
    ShowWindow(hwnd, SW_HIDE);
} else {
    // TEST: 注释掉强制重绘
    // RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
}
```

---

### 测试5: 完全跳过 Window_PollEvents
**目的**: 确认是否是消息循环本身导致卡死

找到调用 `Window_PollEvents` 的地方（可能在 N-API 或 Electron main.ts），临时注释掉。

---

### 测试6: 简化 Win32WindowProc，只处理 WM_CLOSE
**目的**: 确认是否是某个特定的消息处理导致卡死

```cpp
static LRESULT CALLBACK Win32WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    WindowContext* ctx = reinterpret_cast<WindowContext*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    
    switch (msg) {
    case WM_CLOSE:
        return 0;
    
    // TEST: 注释掉所有其他消息处理
    /*
    case WM_PAINT:
        ...
    case WM_SIZE:
        ...
    case WM_LBUTTONDOWN:
        ...
    */
    
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}
```

---

## 推荐测试顺序

1. **先测试1**: 禁用WM_PAINT渲染 → 快速排除OpenGL问题
2. **再测试6**: 简化消息循环 → 确认是否是消息处理本身的问题
3. **然后测试2**: 禁用渲染线程 → 确认线程竞争问题
4. **最后测试3、4**: 定位具体的RedrawWindow调用

## 当前代码状态
- ✅ 已添加 `isHidden` 标志
- ✅ 已在 `Window_Invalidate` 中检查 `isHidden`
- ✅ 已在 `Window_HideAllWindows` 中先停止渲染线程
- ✅ 已在 `Window_RefreshZOrder`、`Window_SetParentWindow`、`Window_Resize` 中检查 `isHidden`

## 下一步
根据测试结果，我们可以：
1. 如果是OpenGL问题 → 检查上下文切换和同步
2. 如果是消息循环问题 → 检查PeekMessage和消息分发
3. 如果是线程竞争 → 添加互斥锁保护窗口操作
4. 如果是RedrawWindow问题 → 改用异步InvalidateRect

## 临时解决方案（如果修复时间过长）
在切换Tab时，不隐藏窗口，而是将窗口移出屏幕：
```cpp
SetWindowPos(hwnd, HWND_BOTTOM, -10000, -10000, width, height, SWP_NOACTIVATE);
```
