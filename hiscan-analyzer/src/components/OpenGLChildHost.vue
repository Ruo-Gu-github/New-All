<script setup lang="ts">
import * as Vue from "vue";

const { computed, onMounted, onUnmounted, ref, watch } = Vue as any;

const props = defineProps<{
  visible: boolean;
  transparentInput?: boolean;
}>();

const emit = defineEmits<{
  (e: "ogl-dblclick"): void;
}>();

const hostRef = ref(null as HTMLDivElement | null);
const childHwnd = ref(null as bigint | null);
const lastError = ref("" as string);
const oglStatus = ref(null as any);
const lastRect = ref(null as any);
const lastSetRectOk = ref(null as null | boolean);
const lastSetVisibleOk = ref(null as null | boolean);
const winInfo = ref(null as any);

const lastAppliedRect = ref(
  null as null | {
    left: number;
    top: number;
    width: number;
    height: number;
  }
);

const isElectron = computed(() => {
  return typeof window !== "undefined" && !!window.ipcRenderer?.invoke;
});

function handleNativeDblClick(_event: any, payload: any) {
  if (!childHwnd.value) return;
  const payloadHwnd =
    typeof payload?.hwnd === "string"
      ? payload.hwnd
      : payload?.hwnd != null
      ? String(payload.hwnd)
      : "";

  if (!payloadHwnd) return;
  if (payloadHwnd === childHwnd.value.toString()) {
    emit("ogl-dblclick");
  }
}

function getHostRectScreenDip() {
  const el = hostRef.value;
  if (!el) return null;
  const rect = el.getBoundingClientRect();

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

function getHostRectClientDip() {
  const el = hostRef.value;
  if (!el) return null;
  const rect = el.getBoundingClientRect();
  return {
    left: Math.round(rect.left),
    top: Math.round(rect.top),
    width: Math.max(1, Math.round(rect.width)),
    height: Math.max(1, Math.round(rect.height)),
  };
}

async function ensureChild() {
  if (!isElectron.value) return;
  if (childHwnd.value) return;
  const rectClient = getHostRectClientDip();
  if (!rectClient) return;

  try {
    // Prefer client-relative placement; fall back to legacy screen-rect if handler not present.
    let res: any;
    try {
      res = await window.ipcRenderer!.invoke!("ogl:create-client", {
        ...rectClient,
        transparentInput: !!props.transparentInput,
      });
    } catch {
      const legacy = getHostRectScreenDip();
      if (!legacy) return;
      res = await window.ipcRenderer!.invoke!("ogl:create", {
        ...legacy,
        transparentInput: !!props.transparentInput,
      });
    }

    if (res?.success && typeof res.hwnd === "string") {
      const hwndStr = res.hwnd.trim();
      try {
        childHwnd.value = BigInt(hwndStr);
      } catch {
        childHwnd.value = null;
        lastError.value = `Invalid hwnd from native: ${hwndStr}`;
        return;
      }
      lastError.value = "";
    } else {
      lastError.value = res?.error ?? "ogl:create failed";
    }
  } catch (e: any) {
    lastError.value = e?.message ?? String(e);
  }
}

async function updateRect() {
  if (!isElectron.value) return;
  if (!childHwnd.value) return;
  const rectClient = getHostRectClientDip();
  if (!rectClient) return;
  lastRect.value = rectClient;

  // Skip no-op updates to reduce native window churn (helps flicker).
  const prev = lastAppliedRect.value;
  if (
    prev &&
    prev.left === rectClient.left &&
    prev.top === rectClient.top &&
    prev.width === rectClient.width &&
    prev.height === rectClient.height
  ) {
    return;
  }
  lastAppliedRect.value = rectClient;

  try {
    let res: any;
    try {
      res = await window.ipcRenderer!.invoke!("ogl:set-rect-client", {
        hwnd: childHwnd.value.toString(),
        ...rectClient,
      });
    } catch {
      const legacy = getHostRectScreenDip();
      if (!legacy) return;
      res = await window.ipcRenderer!.invoke!("ogl:set-rect", {
        hwnd: childHwnd.value.toString(),
        ...legacy,
      });
    }
    lastSetRectOk.value = !!res?.success;
    if (!res?.success && res?.error) lastError.value = res.error;
  } catch {
    // ignore
  }
}

async function setVisible(v: boolean) {
  if (!isElectron.value) return;
  if (!childHwnd.value) return;
  try {
    const res = await window.ipcRenderer!.invoke!("ogl:set-visible", {
      hwnd: childHwnd.value.toString(),
      visible: v,
    });
    lastSetVisibleOk.value = !!res?.success;
    if (!res?.success && res?.error) lastError.value = res.error;
  } catch {
    // ignore
  }
}

async function destroyChild() {
  if (!isElectron.value) return;
  if (!childHwnd.value) return;
  try {
    await window.ipcRenderer!.invoke!("ogl:destroy", {
      hwnd: childHwnd.value.toString(),
    });
  } catch {
    // ignore
  } finally {
    childHwnd.value = null;
  }
}

let infoTimer: number | null = null;
let visibleSeq = 0;

let focusListenerInstalled = false;

function installFocusVisibilityHooks() {
  if (focusListenerInstalled) return;
  focusListenerInstalled = true;

  const onFocus = () => {
    // Re-assert show + rect to avoid occlusion after focus changes.
    if (!props.visible) return;
    void setVisible(true);
    void updateRect();
  };
  const onBlur = () => {
    // Keep visible even when inactive; Chromium can reshuffle HWND z-order on blur.
    if (!props.visible) return;
    void setVisible(true);
  };

  window.addEventListener("focus", onFocus);
  window.addEventListener("blur", onBlur);

  onUnmounted(() => {
    window.removeEventListener("focus", onFocus);
    window.removeEventListener("blur", onBlur);
    focusListenerInstalled = false;
  });
}

async function refreshWindowInfo() {
  if (!isElectron.value) return;
  if (!childHwnd.value) {
    winInfo.value = null;
    return;
  }
  try {
    const res = await window.ipcRenderer!.invoke!("ogl:get-window-info", {
      hwnd: childHwnd.value.toString(),
    });
    if (res?.success) winInfo.value = res?.info ?? null;
    else winInfo.value = { error: res?.error ?? "unknown" };
  } catch (e: any) {
    winInfo.value = { error: e?.message ?? String(e) };
  }
}

let ro: ResizeObserver | null = null;
let rafId: number | null = null;
let resizePending = false;

function scheduleUpdateRect() {
  if (resizePending) return;
  resizePending = true;
  rafId = requestAnimationFrame(() => {
    resizePending = false;
    rafId = null;
    void updateRect();
  });
}

function startTracking() {
  stopTracking();
  const el = hostRef.value;
  if (!el) return;

  ro = new ResizeObserver(() => {
    scheduleUpdateRect();
  });
  ro.observe(el);

  // Initial sync
  scheduleUpdateRect();

  infoTimer = window.setInterval(() => {
    void refreshWindowInfo();
  }, 500);
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
  resizePending = false;
  if (infoTimer != null) {
    clearInterval(infoTimer);
    infoTimer = null;
  }
}

watch(
  () => props.visible,
  async (v: boolean) => {
    if (!isElectron.value) return;
    const seq = ++visibleSeq;
    if (v) {
      await ensureChild();
      if (seq !== visibleSeq) return;
      await setVisible(true);
      if (seq !== visibleSeq) return;
      startTracking();
      return;
    }

    // Hide path: stop tracking first, then hide if we have a window.
    stopTracking();
    if (seq !== visibleSeq) return;
    if (childHwnd.value) {
      await setVisible(false);
    }
  },
  { immediate: true }
);

onMounted(async () => {
  if (isElectron.value) {
    try {
      oglStatus.value = await window.ipcRenderer!.invoke!("ogl:status");
    } catch (e: any) {
      oglStatus.value = { success: false, error: e?.message ?? String(e) };
    }

    window.ipcRenderer?.on?.("ogl:dblclick", handleNativeDblClick);
  }

  installFocusVisibilityHooks();

  if (props.visible) {
    await ensureChild();
    await setVisible(true);
    startTracking();
  }
  await refreshWindowInfo();
});

onUnmounted(async () => {
  if (isElectron.value) {
    window.ipcRenderer?.off?.("ogl:dblclick", handleNativeDblClick);
  }
  stopTracking();
  await destroyChild();
});
</script>

<template>
  <div
    ref="hostRef"
    style="position: relative; width: 100%; height: 100%; overflow: hidden"
  >
    <div
      v-if="isElectron"
      style="
        position: absolute;
        left: 8px;
        top: 8px;
        right: 8px;
        font-size: 11px;
        opacity: 0.9;
        pointer-events: none;
        white-space: pre-wrap;
        z-index: 10;
      "
    >
      <div v-if="oglStatus" style="margin-bottom: 4px">
        ogl:status: {{ JSON.stringify(oglStatus) }}
      </div>
      <div>ogl:hwnd: {{ childHwnd ? childHwnd.toString() : "(none)" }}</div>
      <div v-if="lastRect">ogl:rect: {{ JSON.stringify(lastRect) }}</div>
      <div v-if="winInfo">ogl:winInfo: {{ JSON.stringify(winInfo) }}</div>
      <div>
        ogl:setRect:
        {{ lastSetRectOk === null ? "(n/a)" : String(lastSetRectOk) }}
      </div>
      <div>
        ogl:setVisible:
        {{ lastSetVisibleOk === null ? "(n/a)" : String(lastSetVisibleOk) }}
      </div>
      <div v-if="lastError">ogl:error: {{ lastError }}</div>
      <div v-else-if="!childHwnd">ogl:waiting for hwnd...</div>
    </div>

    <div
      v-if="!isElectron"
      style="
        position: absolute;
        inset: 0;
        display: flex;
        align-items: center;
        justify-content: center;
        opacity: 0.6;
        pointer-events: none;
        font-size: 12px;
      "
    >
      (Web 模式：不会创建原生 OpenGL 子窗口)
    </div>
  </div>
</template>
