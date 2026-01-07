<script setup lang="ts">
import * as Vue from "vue";
import { ElButton, ElSwitch } from "element-plus";

const { computed, onMounted, onUnmounted, ref, watch } = Vue as any;

const hostRef = ref(null as HTMLDivElement | null);
const childHwnd = ref(null as bigint | null);
const transparentInput = ref(false);
const standaloneHwnd = ref(null as bigint | null);

const isElectron = computed(() => {
  return typeof window !== "undefined" && !!window.ipcRenderer?.invoke;
});

function getHostRectScreenDip() {
  const el = hostRef.value;
  if (!el) return null;
  const rect = el.getBoundingClientRect();

  // Convert viewport (CSS px) to screen coordinates (CSS px).
  // On Windows, outer-inner diffs represent window borders/titlebar.
  const borderX = Math.round((window.outerWidth - window.innerWidth) / 2);
  const borderY = Math.round(window.outerHeight - window.innerHeight);

  const screenX = Math.round(window.screenX + borderX + rect.left);
  const screenY = Math.round(window.screenY + borderY + rect.top);
  return {
    x: screenX,
    y: screenY,
    width: Math.max(1, Math.round(rect.width)),
    height: Math.max(1, Math.round(rect.height)),
  };
}

async function ensureChild() {
  if (!isElectron.value) return;
  if (childHwnd.value) return;
  const rect = getHostRectScreenDip();
  if (!rect) return;

  const res = await window.ipcRenderer!.invoke!("ogl:create", {
    ...rect,
    transparentInput: transparentInput.value,
  });

  if (res?.success && typeof res.hwnd === "string") {
    // bigint comes back as string (ipc serialization)
    childHwnd.value = BigInt(res.hwnd);
  }
}

async function updateRect() {
  if (!isElectron.value) return;
  if (!childHwnd.value) return;
  const rect = getHostRectScreenDip();
  if (!rect) return;
  await window.ipcRenderer!.invoke!("ogl:set-rect", {
    hwnd: childHwnd.value.toString(),
    ...rect,
  });
}

async function destroyChild() {
  if (!isElectron.value) return;
  if (!childHwnd.value) return;
  await window.ipcRenderer!.invoke!("ogl:destroy", {
    hwnd: childHwnd.value.toString(),
  });
  childHwnd.value = null;
}

async function openStandalone() {
  if (!isElectron.value) return;
  const res = await window.ipcRenderer!.invoke!("ogl:standalone-open", {
    width: 900,
    height: 650,
  });
  if (res?.success && typeof res.hwnd === "string") {
    standaloneHwnd.value = BigInt(res.hwnd);
  }
}

async function closeStandalone() {
  if (!isElectron.value) return;
  if (!standaloneHwnd.value) return;
  await window.ipcRenderer!.invoke!("ogl:destroy", {
    hwnd: standaloneHwnd.value.toString(),
  });
  standaloneHwnd.value = null;
}

let ro: ResizeObserver | null = null;
let rafId: number | null = null;

function startTracking() {
  stopTracking();
  const el = hostRef.value;
  if (!el) return;

  ro = new ResizeObserver(() => {
    void updateRect();
  });
  ro.observe(el);

  const tick = () => {
    void updateRect();
    rafId = requestAnimationFrame(tick);
  };
  rafId = requestAnimationFrame(tick);
}

function stopTracking() {
  if (ro) {
    ro.disconnect();
    ro = null;
  }
  if (rafId != null) {
    cancelAnimationFrame(rafId);
    rafId = null;
  }
}

watch(transparentInput, async (v: boolean) => {
  if (!isElectron.value) return;
  if (!childHwnd.value) return;
  await window.ipcRenderer!.invoke!("ogl:set-transparent-input", {
    hwnd: childHwnd.value.toString(),
    enabled: v,
  });
});

onMounted(async () => {
  await ensureChild();
  startTracking();
});

onUnmounted(async () => {
  stopTracking();
  await destroyChild();
  await closeStandalone();
});
</script>

<template>
  <div class="analyzer-tab-page">
    <div class="analyzer-tab-toolbar">
      <ElButton type="primary" :disabled="!isElectron" @click="ensureChild"
        >创建窗口</ElButton
      >
      <ElButton :disabled="!isElectron || !childHwnd" @click="destroyChild"
        >销毁窗口</ElButton
      >
      <ElButton type="success" :disabled="!isElectron" @click="openStandalone"
        >打开独立 OpenGL 窗口</ElButton
      >
      <ElButton
        :disabled="!isElectron || !standaloneHwnd"
        @click="closeStandalone"
        >关闭独立窗口</ElButton
      >
      <div style="display: flex; align-items: center; gap: 8px">
        <span>鼠标穿透</span>
        <ElSwitch
          v-model="transparentInput"
          :disabled="!isElectron || !childHwnd"
        />
      </div>
      <div class="analyzer-tab-hint">
        <span v-if="!isElectron">(当前是 Web 模式：不会创建原生窗口)</span>
        <span v-else-if="childHwnd">HWND: {{ childHwnd.toString() }}</span>
        <span v-else-if="standaloneHwnd"
          >Standalone HWND: {{ standaloneHwnd.toString() }}</span
        >
      </div>
    </div>

    <div
      ref="hostRef"
      style="
        flex: 1;
        position: relative;
        border: 1px solid rgba(255, 255, 255, 0.2);
        border-radius: 6px;
        overflow: hidden;
      "
    >
      <div
        style="
          position: absolute;
          inset: 0;
          display: flex;
          align-items: center;
          justify-content: center;
          opacity: 0.6;
          pointer-events: none;
        "
      >
        OpenGL 子窗口将嵌入到这里
      </div>
    </div>
  </div>
</template>
