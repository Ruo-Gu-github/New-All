<script setup lang="ts">
import { ElButton } from "element-plus";
import ViewportGrid, { type ViewportDef } from "./ViewportGrid.vue";

const props = defineProps<{ active: boolean }>();

const viewports: ViewportDef[] = [
  { id: "view3d", title: "3D 视图", kind: "opengl" },
];

async function openPopup() {
  try {
    await window.ipcRenderer?.invoke?.("browserwindow:open", {
      title: "3D Popup",
      showInactive: true,
    });
  } catch {
    // ignore
  }
}
</script>

<template>
  <div v-show="props.active" class="analyzer-tab-page">
    <div class="analyzer-tab-toolbar">
      <ElButton type="primary" @click="openPopup">打开弹窗（新窗口）</ElButton>
      <div class="analyzer-tab-hint">单窗口 3D 渲染模拟（OpenGL 子窗口）。</div>
    </div>

    <div class="analyzer-tab-body">
      <ViewportGrid
        layout="1x1"
        :viewports="viewports"
        :active="props.active"
      />
    </div>
  </div>
</template>
