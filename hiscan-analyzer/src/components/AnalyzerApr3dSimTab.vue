<script setup lang="ts">
import * as Vue from "vue";
import {
  ElButton,
  ElCheckbox,
  ElRadioButton,
  ElRadioGroup,
} from "element-plus";
import ViewportGrid, { type ViewportDef } from "./ViewportGrid.vue";

const { computed, ref } = Vue as any;

const props = defineProps<{ active: boolean }>();

const allOpenGL = ref(true);
type LayoutChoice = "quad" | "axial" | "coronal" | "sagittal" | "view3d";
const layoutChoice = ref<LayoutChoice>("quad");

const layout = computed(() => {
  return layoutChoice.value === "quad" ? ("2x2" as const) : ("1x1" as const);
});

const viewports = computed(() => {
  if (layoutChoice.value !== "quad") {
    const titleMap: Record<Exclude<LayoutChoice, "quad">, string> = {
      axial: "轴位视图（单窗）",
      coronal: "冠状位视图（单窗）",
      sagittal: "矢状位视图（单窗）",
      view3d: "3D 视图（单窗）",
    };
    return [
      {
        id: layoutChoice.value,
        title: titleMap[layoutChoice.value],
        kind: "opengl",
      },
    ] as ViewportDef[];
  }

  return [
    {
      id: "axial",
      title: "轴位视图",
      kind: allOpenGL.value ? "opengl" : "html",
    },
    {
      id: "coronal",
      title: "冠状位视图",
      kind: allOpenGL.value ? "opengl" : "html",
    },
    {
      id: "sagittal",
      title: "矢状位视图",
      kind: allOpenGL.value ? "opengl" : "html",
    },
    {
      id: "view3d",
      title: "3D 视图",
      kind: "opengl",
    },
  ] as ViewportDef[];
});

async function openPopup() {
  try {
    await window.ipcRenderer?.invoke?.("browserwindow:open", {
      title: "APR+3D Popup",
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
      <ElRadioGroup v-model="layoutChoice" size="small">
        <ElRadioButton label="quad">全部</ElRadioButton>
        <ElRadioButton label="axial">轴位</ElRadioButton>
        <ElRadioButton label="coronal">冠状位</ElRadioButton>
        <ElRadioButton label="sagittal">矢状位</ElRadioButton>
        <ElRadioButton label="view3d">3D</ElRadioButton>
      </ElRadioGroup>
      <ElCheckbox v-model="allOpenGL">全部视口用 OpenGL（压力测试）</ElCheckbox>
      <div class="analyzer-tab-hint">
        通过按钮组切换：四窗 / 单窗（指定视口）。
      </div>
    </div>

    <div class="analyzer-tab-body">
      <ViewportGrid
        :layout="layout"
        :viewports="viewports"
        :active="props.active"
      />
    </div>
  </div>
</template>
