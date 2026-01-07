<script setup lang="ts">
import * as Vue from "vue";
import OpenGLChildHost from "./OpenGLChildHost.vue";

const { computed, ref } = Vue as any;

export type ViewportKind = "html" | "opengl";

export type ViewportDef = {
  id: string;
  title: string;
  kind: ViewportKind;
};

const props = defineProps<{
  layout: "2x2" | "1x1";
  viewports: ViewportDef[];
  active: boolean;
}>();

const maximizedId = ref(null as string | null);

function toggleMax(id: string) {
  maximizedId.value = maximizedId.value === id ? null : id;
}

const gridStyle = computed(() => {
  if (props.layout === "1x1" || maximizedId.value) {
    return {
      display: "grid",
      gridTemplateColumns: "1fr",
      gridTemplateRows: "1fr",
      gap: "12px",
      width: "100%",
      height: "100%",
    } as any;
  }

  return {
    display: "grid",
    gridTemplateColumns: "1fr 1fr",
    gridTemplateRows: "1fr 1fr",
    gap: "12px",
    width: "100%",
    height: "100%",
  } as any;
});

function isVisibleViewport(id: string) {
  if (!props.active) return false;
  if (!maximizedId.value) return true;
  return maximizedId.value === id;
}
</script>

<template>
  <div :style="gridStyle">
    <div
      v-for="vp in viewports"
      :key="vp.id"
      @dblclick="toggleMax(vp.id)"
      :style="{
        position: 'relative',
        border: '1px solid rgba(255,255,255,0.15)',
        borderRadius: '10px',
        overflow: 'hidden',
        background: 'rgba(7,26,41,0.35)',
        display: isVisibleViewport(vp.id) ? 'block' : 'none',
      }"
    >
      <div
        style="
          position: absolute;
          left: 10px;
          top: 10px;
          min-width: 140px;
          display: inline-flex;
          gap: 8px;
          align-items: center;
          padding: 6px 10px;
          border-radius: 8px;
          background: rgba(6, 18, 30, 0.65);
          color: #d9f1ff;
          font-size: 13px;
          line-height: 1;
          z-index: 12;
          pointer-events: auto;
          user-select: none;
          cursor: pointer;
        "
        @click.stop="toggleMax(vp.id)"
        @dblclick.stop="toggleMax(vp.id)"
        title="单击或双击放大/还原"
      >
        <span>{{ vp.title }}</span>
        <span style="opacity: 0.8; font-size: 12px">⤢</span>
      </div>

      <div
        style="
          position: absolute;
          inset: 0;
          display: flex;
          align-items: center;
          justify-content: center;
          pointer-events: none;
          opacity: 0.55;
          font-size: 14px;
        "
      >
        {{ vp.title }}
      </div>

      <OpenGLChildHost
        v-if="vp.kind === 'opengl' && isVisibleViewport(vp.id)"
        :visible="props.active && isVisibleViewport(vp.id)"
        :key="vp.id + (maximizedId ? '-max' : '')"
        @ogl-dblclick="toggleMax(vp.id)"
        style="position: absolute; inset: 0"
      />
    </div>
  </div>
</template>
