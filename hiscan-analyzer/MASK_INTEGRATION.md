# ImageProcessing Mask 功能集成说明

## 概述

本项目已创建完整的 Mask 管理和 ROI 编辑功能界面，包括：

1. **MaskManager.vue** - Mask 管理组件（左侧面板）
2. **MprView.vue** - MPR 视图组件（支持 Mask 叠加显示）
3. **imageProcessing.ts** - Native API 封装

## 组件使用

### 1. 在 AnalyzerViewerTab.vue 中集成

```vue
<template>
  <div class="viewer-page">
    <!-- 左侧：Mask 管理面板 -->
    <aside class="left-panel">
      <MaskManager 
        ref="maskManagerRef"
        @mask-updated="onMaskUpdated"
      />
    </aside>

    <!-- 右侧：MPR 视图 -->
    <main class="right-panel">
      <div class="grid2x2">
        <MprView
          v-for="(view, idx) in mprViews"
          :key="idx"
          :view-type="view.type"
          :slice-index="view.sliceIndex"
          :masks="currentMasks"
          :draw-mode="currentDrawMode"
          :brush-size="brushSize"
          @click="onMprClick"
          @draw="onRoiDraw"
        />
      </div>
    </main>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue';
import MaskManager from './MaskManager.vue';
import MprView from './MprView.vue';

const maskManagerRef = ref<InstanceType<typeof MaskManager> | null>(null);

const mprViews = ref([
  { type: 'axial', sliceIndex: 256 },
  { type: 'coronal', sliceIndex: 256 },
  { type: 'sagittal', sliceIndex: 256 },
  { type: '3d', sliceIndex: 0 }
]);

const currentMasks = ref([]);
const currentDrawMode = ref('none');
const brushSize = ref(10);

function onMaskUpdated() {
  // 当 Mask 更新时，重新获取数据
  currentMasks.value = maskManagerRef.value?.masks || [];
}

function onMprClick(x: number, y: number, z: number) {
  // 处理 MPR 视图点击（FloodFill）
  maskManagerRef.value?.onMprClick(x, y, z, 0);
}

function onRoiDraw(roi: any) {
  // 处理 ROI 绘制完成
  console.log('ROI drawn:', roi);
}
</script>
```

## 功能列表

### Mask 管理
- ✅ 新建空白 Mask
- ✅ 从阈值创建 Mask
- ✅ 克隆 Mask
- ✅ 删除 Mask
- ✅ 显示/隐藏 Mask
- ✅ 设置 Mask 颜色（带透明度）

### 布尔运算
- ✅ 并集 (A ∪ B)
- ✅ 交集 (A ∩ B)
- ✅ 差集 (A - B)
- ✅ 异或 (A ⊕ B)
- ✅ 反转

### 连通域操作
- ✅ FloodFill（鼠标点击选择连通域）
- ✅ 保留最大连通域
- ✅ 移除小区域

### 形态学操作
- ✅ 膨胀
- ✅ 腐蚀
- ✅ 开运算
- ✅ 闭运算

### ROI 绘制工具
- ✅ 矩形
- ✅ 圆形
- ✅ 多边形
- ✅ 画笔

### MPR 视图叠加
- ✅ 三层 Canvas 架构：
  - MPR 图像层（底层）
  - Mask 叠加层（半透明，中层）
  - ROI 绘制层（顶层）
- ✅ 鼠标交互
- ✅ 窗宽窗位显示
- ✅ 像素值显示

## 待实现功能

### Native API 集成
所有 `imageProcessing.ts` 中的 TODO 需要替换为实际的 Native 调用：

```typescript
// 示例：
// 当前：
createEmpty(width: number, height: number, depth: number, name: string): number {
  // TODO: 调用 Native API
  return -1;
}

// 实现后：
createEmpty(width: number, height: number, depth: number, name: string): number {
  if (!window.nativeApi?.maskManager) {
    console.error('Native API not available');
    return -1;
  }
  return window.nativeApi.maskManager.createEmpty(this.handle, width, height, depth, name);
}
```

### MPR 数据加载
`MprView.vue` 中的 `renderMPR()` 函数需要：
1. 从 `props.volumeData` 提取当前切片
2. 应用窗宽窗位转换
3. 渲染到 Canvas

### Mask 数据同步
`MprView.vue` 中的 `renderMasks()` 函数需要：
1. 从 Native API 获取 Mask 数据
2. 根据当前切片索引提取 2D 切片
3. 应用颜色和透明度
4. 渲染到 Canvas

## 测试步骤

1. **启动开发服务器**
   ```bash
   cd hiscan-analyzer
   npm run dev
   ```

2. **测试 UI 功能**
   - 点击"新建 Mask"按钮
   - 选择两个 Mask，执行布尔运算
   - 选择 ROI 工具，在视图中绘制
   - 调整阈值滑块，创建阈值 Mask

3. **集成 Native API 后测试**
   - 加载 DICOM 数据
   - 从阈值创建 Mask
   - 点击 MPR 视图，执行 FloodFill
   - 验证 Mask 正确显示在 MPR 上

## 架构说明

```
┌─────────────────────────────────────────────────┐
│          hiscan-analyzer (Electron/Vue)         │
├─────────────────────────────────────────────────┤
│  ┌──────────────┐        ┌──────────────┐      │
│  │ MaskManager  │ ◄───►  │   MprView    │      │
│  │    .vue      │        │    .vue      │      │
│  └──────┬───────┘        └──────┬───────┘      │
│         │                       │               │
│         └───────────┬───────────┘               │
│                     ▼                            │
│         ┌───────────────────────┐               │
│         │ imageProcessing.ts    │               │
│         └───────────┬───────────┘               │
├─────────────────────┼───────────────────────────┤
│                     ▼                            │
│         ┌───────────────────────┐               │
│         │  Electron IPC Bridge  │               │
│         └───────────┬───────────┘               │
└─────────────────────┼───────────────────────────┘
                      ▼
          ┌───────────────────────┐
          │ DllImageProcessing    │
          │  (Native C++ DLL)     │
          └───────────────────────┘
```

## 关键文件

- `src/components/MaskManager.vue` - Mask 管理界面
- `src/components/MprView.vue` - MPR 视图和 Mask 叠加
- `src/services/imageProcessing.ts` - Native API 封装
- `ConsoleDllTest/DllImageProcessing/ImageProcessingApi.h` - Native API 定义

## 下一步

1. 实现 Electron Main Process 的 IPC 通信
2. 封装 DllImageProcessing 的调用
3. 实现 MPR 数据加载和渲染
4. 实现 Mask 数据同步和显示
5. 添加测试用例
