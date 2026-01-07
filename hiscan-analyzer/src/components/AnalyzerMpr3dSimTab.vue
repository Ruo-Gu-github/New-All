<script setup lang="ts">
import * as Vue from "vue";
import { ElButton, ElCheckbox } from "element-plus";
import ViewportGrid, { type ViewportDef } from "./ViewportGrid.vue";

const { computed, ref } = Vue as any;

const props = defineProps<{ active: boolean }>();

const allOpenGL = ref(true);

const viewports = computed(() => {
  const kind2d = allOpenGL.value ? "opengl" : "html";
  const vps: ViewportDef[] = [
    { id: "axial", title: "轴位视图", kind: kind2d },
    { id: "coronal", title: "冠状位视图", kind: kind2d },
    { id: "sagittal", title: "矢状位视图", kind: kind2d },
    { id: "view3d", title: "3D 视图", kind: "opengl" },
  ];
  return vps;
});

async function openPopup() {
  try {
    await window.ipcRenderer?.invoke?.("browserwindow:open", {
      title: "MPR+3D Popup",
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
      <ElCheckbox v-model="allOpenGL">全部视口用 OpenGL（压力测试）</ElCheckbox>
      <div class="analyzer-tab-hint">
        双击任意视口：放大到 4 窗区域；再次双击还原。
      </div>
    </div>

    <div class="analyzer-tab-body">
      <ViewportGrid
        layout="2x2"
        :viewports="viewports"
        :active="props.active"
      />
    </div>
  </div>
</template>
