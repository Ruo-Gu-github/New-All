# 当前测试配置说明

## 已应用的测试修改

### ✅ 测试1: 禁用 WM_PAINT 渲染
**位置**: `Win32WindowProc` 的 `WM_PAINT` 消息处理
**修改**: 完全跳过OpenGL渲染，只调用 `ValidateRect`
**目的**: 排除OpenGL上下文切换导致的卡死

### ✅ 测试3: 禁用 WM_SIZE 重绘
**位置**: `Win32WindowProc` 的 `WM_SIZE` 消息处理
**修改**: 注释掉 `RedrawWindow` 调用
**目的**: 排除窗口大小变化时的同步重绘导致的卡死

### ✅ 测试4: 禁用 Window_RefreshZOrder 重绘
**位置**: `Window_RefreshZOrder` 函数末尾
**修改**: 注释掉非隐藏窗口的 `RedrawWindow` 调用
**目的**: 排除Z-order刷新时的强制重绘导致的卡死

### ✅ 增强日志
**位置**: `Window_HideAllWindows`, `Window_PollEvents`
**修改**: 添加详细的步骤日志
**目的**: 追踪卡死发生在哪个步骤

## 测试步骤

1. **编译DLL**
   ```cmd
   cd d:\2025-09-25 新系列\ConsoleDllTest
   msbuild ConsoleDllTest.sln /p:Configuration=Debug /p:Platform=x64 /t:DllVisualization
   ```

2. **运行Electron应用**
   ```cmd
   cd d:\2025-09-25 新系列\hiscan-analyzer
   npm run dev
   ```

3. **测试场景**
   - 加载一个图像数据
   - 切换到"图像浏览"或"3维重建" Tab（APR窗口显示）
   - **观察窗口是否显示**（当前WM_PAINT被禁用，窗口应该是黑色/空白）
   - 切换到其他Tab（如"图像管理"）
   - **观察是否卡死**

4. **查看日志**
   - 查看控制台输出
   - 重点关注：
     ```
     [Window_HideAllWindows] ===== START =====
     [Window_HideAllWindows] Stopping render loop...
     [Window_HideAllWindows] Render loop stopped
     [Window_HideAllWindows] Processing window 1/N...
     [Window_HideAllWindows]   Calling ShowWindow(SW_HIDE)...
     ```
   - **如果卡在某个步骤不继续输出，说明卡死点在那里**

## 预期结果分析

### 场景A: 不再卡死
- **结论**: 问题确实是OpenGL渲染导致的
- **解决方案**: 
  1. 检查 `wglMakeCurrent` 是否在隐藏窗口时被调用
  2. 改用异步渲染（FBO）
  3. 确保上下文切换在正确的线程

### 场景B: 仍然卡死，卡在 `ShowWindow` 调用
- **结论**: 问题在Win32窗口操作本身
- **解决方案**:
  1. 使用异步窗口操作（PostMessage）
  2. 不隐藏窗口，改为移出屏幕
  3. 检查是否有其他线程持有窗口锁

### 场景C: 仍然卡死，卡在 `EnableWindow` 调用
- **结论**: 窗口消息队列有未处理的消息阻塞
- **解决方案**:
  1. 先调用 `PeekMessage` 清空消息队列
  2. 使用 `SendMessageTimeout` 代替同步调用
  3. 检查是否有模态对话框或消息钩子

### 场景D: 卡死在渲染线程停止
- **结论**: 渲染线程的 `join()` 等待超时
- **解决方案**:
  1. 检查渲染线程是否真的在运行
  2. 增加超时检测
  3. 使用 `detach()` 代替 `join()`

## 下一步测试

如果当前测试不卡死，逐步恢复功能：

1. **恢复 Window_RefreshZOrder 的 RedrawWindow**
2. **恢复 WM_SIZE 的 RedrawWindow**
3. **恢复 WM_PAINT 的渲染**（每次恢复后测试）

找到导致卡死的最小代码后，针对性优化。

## 回退代码

如果需要恢复原始代码，查找以下注释标记：
- `// ===== 测试1: 完全禁用渲染`
- `// ===== 测试3: 禁用立即重绘`
- `// ===== 测试4: 禁用强制重绘`

取消注释原始代码，删除测试代码即可。
