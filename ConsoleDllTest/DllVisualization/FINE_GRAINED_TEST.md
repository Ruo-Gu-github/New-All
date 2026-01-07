# WM_PAINT 细粒度测试步骤

## 问题
恢复渲染后又卡死了，需要找出是哪个OpenGL调用导致的。

## 测试方法
在 `Win32WindowProc` 的 `WM_PAINT` 中，我准备了5个测试级别，**逐个启用**来定位问题。

---

## 测试A: 只 ValidateRect（当前启用）
```cpp
ValidateRect(hwnd, nullptr);
return 0;
```

**测试**:
1. 编译运行
2. 切换Tab
3. **预期**: 不卡死（已验证✅）

---

## 测试B: 启用 wglMakeCurrent

**操作**: 
1. 注释掉"测试A"的代码
2. 取消注释"测试B"的代码
3. 编译运行

**代码**:
```cpp
if (wglMakeCurrent(hdc, ctx->hglrc)) {
    printf("[WM_PAINT] wglMakeCurrent SUCCESS\n");
    wglMakeCurrent(nullptr, nullptr);
    printf("[WM_PAINT] Context released\n");
}
```

**关键点**: 只做上下文切换，不调用任何OpenGL函数

**如果卡死** → 问题在 `wglMakeCurrent` 本身（可能是上下文被其他线程占用）

**如果不卡死** → 继续测试C

---

## 测试C: 启用 glViewport

**操作**: 
1. 注释掉"测试B"的代码
2. 取消注释"测试C"的代码
3. 编译运行

**代码**:
```cpp
if (wglMakeCurrent(hdc, ctx->hglrc)) {
    printf("[WM_PAINT] Setting viewport %dx%d\n", ctx->width, ctx->height);
    glViewport(0, 0, ctx->width, ctx->height);
    printf("[WM_PAINT] Viewport set\n");
    wglMakeCurrent(nullptr, nullptr);
}
```

**关键点**: 添加 `glViewport` 调用

**如果卡死** → 问题在 `glViewport`

**如果不卡死** → 继续测试D

---

## 测试D: 启用 APR_Render（不含 SwapBuffers）

**操作**: 
1. 注释掉"测试C"的代码
2. 取消注释"测试D"的代码
3. 编译运行

**代码**:
```cpp
if (wglMakeCurrent(hdc, ctx->hglrc)) {
    glViewport(0, 0, ctx->width, ctx->height);
    printf("[WM_PAINT] Before APR_Render\n");
    if (ctx->boundRenderer) {
        if (ctx->rendererType == 0 || ctx->rendererType == 2)
            APR_Render(static_cast<APRHandle>(ctx->boundRenderer));
        // ...
    }
    printf("[WM_PAINT] After Render, before SwapBuffers\n");
    wglMakeCurrent(nullptr, nullptr);
}
```

**关键点**: 执行渲染但不交换缓冲区

**如果卡死** → 问题在 `APR_Render` 内部的OpenGL调用

**如果不卡死** → 继续测试E

---

## 测试E: 完整渲染（包含 SwapBuffers）

**操作**: 
1. 注释掉"测试D"的代码
2. 取消注释"测试E"的代码
3. 编译运行

**代码**:
```cpp
// 完整的渲染流程，包括 SwapBuffers
```

**如果卡死** → 问题在 `SwapBuffers`（通常是V-Sync或驱动问题）

**如果不卡死** → 渲染完全正常！

---

## 快速测试命令

```cmd
# 1. 修改代码（按上面的步骤）
# 2. 编译
cd d:\2025-09-25 新系列\ConsoleDllTest
msbuild ConsoleDllTest.sln /p:Configuration=Debug /p:Platform=x64 /t:DllVisualization

# 3. 运行
cd d:\2025-09-25 新系列\hiscan-analyzer
npm run dev

# 4. 测试
#    - 加载图像
#    - 切换到"图像浏览" Tab
#    - 切换到其他 Tab
#    - 观察是否卡死 + 查看日志
```

---

## 根据结果的解决方案

### 场景1: 测试B卡死（wglMakeCurrent）
**原因**: OpenGL上下文被其他线程占用或未正确释放

**解决方案**:
```cpp
// 在隐藏窗口前，确保释放所有上下文
for (auto handle : g_AllWindows) {
    auto ctx = static_cast<WindowContext*>(handle);
    if (ctx->hdc && ctx->hglrc) {
        wglMakeCurrent(ctx->hdc, ctx->hglrc);
        wglMakeCurrent(nullptr, nullptr);  // 释放
    }
}
```

### 场景2: 测试C卡死（glViewport）
**原因**: OpenGL驱动问题或状态错误

**解决方案**:
```cpp
// 在设置viewport前清理状态
glGetError();  // 清除旧错误
glViewport(0, 0, ctx->width, ctx->height);
GLenum err = glGetError();
if (err != GL_NO_ERROR) {
    printf("glViewport error: %d\n", err);
}
```

### 场景3: 测试D卡死（APR_Render）
**原因**: 渲染过程中的某个OpenGL调用阻塞

**解决方案**: 需要进一步分析 `APR_Render` 内部，可能需要：
1. 跳过纹理上传
2. 跳过Shader编译
3. 使用 `glFinish()` 确保命令完成

### 场景4: 测试E卡死（SwapBuffers）
**原因**: V-Sync等待或窗口状态异常

**解决方案**:
```cpp
// 禁用V-Sync
wglSwapIntervalEXT(0);

// 或者使用异步交换
if (ctx->isVisible) {  // 只有可见窗口才交换
    SwapBuffers(hdc);
}
```

---

## 当前状态
✅ 测试A已验证：不卡死
⏳ 等待测试B：启用 wglMakeCurrent

## 下一步
按照测试B → C → D → E的顺序，找到第一个导致卡死的调用。
