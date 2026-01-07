<script setup lang="ts">
import * as Vue from "vue";
import { ElButton, ElRadioButton, ElRadioGroup } from "element-plus";
import ViewportGrid, { type ViewportDef } from "./ViewportGrid.vue";

const { computed, ref } = Vue as any;

const props = defineProps<{ active: boolean }>();

const step = ref<1 | 2 | 3>(1);

const viewports = computed(() => {
  if (step.value === 1) {
    // Step 1: APR 三窗（用 2x2 留一个空位）。
    const vps: ViewportDef[] = [
      { id: "axial", title: "APR 轴位", kind: "opengl" },
      { id: "coronal", title: "APR 冠状位", kind: "opengl" },
      { id: "sagittal", title: "APR 矢状位", kind: "opengl" },
      { id: "empty", title: "(空)", kind: "html" },
    ];
    return vps;
  }

  // Step 2/3: MPR 轴位单窗
  return [
    { id: "mpr_axial", title: "MPR 轴位", kind: "opengl" },
  ] as ViewportDef[];
});

const layout = computed(() => {
  return step.value === 1 ? ("2x2" as const) : ("1x1" as const);
});

async function openPopup() {
  try {
    await window.ipcRenderer?.invoke?.("browserwindow:open", {
      title: `分析模块 Step ${step.value} Popup`,
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
      <div
        style="display: flex; gap: 12px; align-items: center; flex-wrap: wrap"
      >
        <ElButton type="primary" @click="openPopup"
          >打开弹窗（新窗口）</ElButton
        >
        <ElRadioGroup v-model="step" size="small">
          <ElRadioButton :label="1">步骤 1（APR 三窗）</ElRadioButton>
          <ElRadioButton :label="2">步骤 2（MPR 轴位单窗）</ElRadioButton>
          <ElRadioButton :label="3">步骤 3（MPR 轴位单窗）</ElRadioButton>
        </ElRadioGroup>
        <div class="analyzer-tab-hint">
          用于验证：步骤切换、窗口布局变化、鼠标消息与弹窗焦点。
        </div>
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
