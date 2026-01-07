<script setup lang="ts">
import * as Vue from "vue";

const { onMounted, onUnmounted, ref } = Vue as any;

type WindowLabType = "normal" | "opengl";

type WindowLabRow = {
  id: string;
  type: WindowLabType;
  title: string;
  bounds: { x: number; y: number; width: number; height: number };
  isVisible: boolean;
  isMinimized: boolean;
  isMaximized: boolean;
  isDestroyed: boolean;
};

const rows = ref([] as WindowLabRow[]);
const lastError = ref("");

async function refresh() {
  try {
    lastError.value = "";
    const res = await window.ipcRenderer?.invoke?.("winlab:list");
    if (!res?.success) {
      lastError.value = res?.error ?? "winlab:list failed";
      rows.value = [];
      return;
    }
    rows.value = res.windows ?? [];
  } catch (e: any) {
    lastError.value = e?.message ?? String(e);
  }
}

async function create(type: WindowLabType) {
  const res = await window.ipcRenderer?.invoke?.("winlab:create", { type });
  if (!res?.success) lastError.value = res?.error ?? "winlab:create failed";
  await refresh();
}

async function action(id: string, act: any, bounds?: any) {
  const res = await window.ipcRenderer?.invoke?.("winlab:action", {
    id,
    action: act,
    bounds,
  });
  if (!res?.success) lastError.value = res?.error ?? "winlab:action failed";
  await refresh();
}

const editBounds = ref(
  {} as Record<string, { x: number; y: number; width: number; height: number }>
);

function ensureEdit(
  id: string,
  b: { x: number; y: number; width: number; height: number }
) {
  if (!editBounds.value[id]) editBounds.value[id] = { ...b };
  return editBounds.value[id];
}

let t: number | null = null;

onMounted(async () => {
  await refresh();
  t = window.setInterval(refresh, 500);
});

onUnmounted(() => {
  if (t != null) window.clearInterval(t);
});
</script>

<template>
  <div class="analyzer-tab-page">
    <div class="analyzer-tab-toolbar">
      <button type="button" @click="create('normal')">创建普通窗口</button>
      <button type="button" @click="create('opengl')">创建 OpenGL 窗口</button>
      <button type="button" @click="refresh">刷新</button>
      <span v-if="lastError" class="analyzer-tab-hint" style="color: #ff9b9b">
        {{ lastError }}
      </span>
    </div>

    <div
      style="
        flex: 1;
        overflow: auto;
        border: 1px solid rgba(255, 255, 255, 0.15);
        border-radius: 6px;
      "
    >
      <table style="width: 100%; border-collapse: collapse; font-size: 12px">
        <thead>
          <tr
            style="
              text-align: left;
              border-bottom: 1px solid rgba(255, 255, 255, 0.15);
            "
          >
            <th style="padding: 8px">ID</th>
            <th style="padding: 8px">类型</th>
            <th style="padding: 8px">状态</th>
            <th style="padding: 8px">Bounds</th>
            <th style="padding: 8px">动作</th>
          </tr>
        </thead>
        <tbody>
          <tr
            v-for="r in rows"
            :key="r.id"
            style="border-bottom: 1px solid rgba(255, 255, 255, 0.08)"
          >
            <td style="padding: 8px; white-space: nowrap">{{ r.id }}</td>
            <td style="padding: 8px">{{ r.type }}</td>
            <td style="padding: 8px; white-space: nowrap">
              <span v-if="r.isDestroyed">destroyed</span>
              <span v-else>
                <span v-if="r.isVisible">visible</span
                ><span v-else>hidden</span> ·
                <span v-if="r.isMinimized">min</span
                ><span v-else>no-min</span> ·
                <span v-if="r.isMaximized">max</span><span v-else>no-max</span>
              </span>
            </td>
            <td style="padding: 8px">
              <div
                style="
                  display: flex;
                  gap: 6px;
                  align-items: center;
                  flex-wrap: wrap;
                "
              >
                <input
                  type="number"
                  style="width: 72px"
                  v-model.number="ensureEdit(r.id, r.bounds).x"
                />
                <input
                  type="number"
                  style="width: 72px"
                  v-model.number="ensureEdit(r.id, r.bounds).y"
                />
                <input
                  type="number"
                  style="width: 72px"
                  v-model.number="ensureEdit(r.id, r.bounds).width"
                />
                <input
                  type="number"
                  style="width: 72px"
                  v-model.number="ensureEdit(r.id, r.bounds).height"
                />
                <button
                  type="button"
                  @click="action(r.id, 'setBounds', ensureEdit(r.id, r.bounds))"
                >
                  应用
                </button>
              </div>
            </td>
            <td style="padding: 8px">
              <div style="display: flex; gap: 6px; flex-wrap: wrap">
                <button type="button" @click="action(r.id, 'show')">
                  show
                </button>
                <button type="button" @click="action(r.id, 'hide')">
                  hide
                </button>
                <button type="button" @click="action(r.id, 'minimize')">
                  min
                </button>
                <button type="button" @click="action(r.id, 'maximize')">
                  max
                </button>
                <button type="button" @click="action(r.id, 'restore')">
                  restore
                </button>
                <button type="button" @click="action(r.id, 'center')">
                  center
                </button>
                <button type="button" @click="action(r.id, 'focus')">
                  focus
                </button>
                <button type="button" @click="action(r.id, 'close')">
                  close
                </button>
              </div>
            </td>
          </tr>
        </tbody>
      </table>
    </div>

    <div class="analyzer-tab-hint">
      说明：这里测试的是 Electron
      窗口动作（show/hide/min/max/move/resize）。OpenGL 窗口会自动打开 OpenGL
      测试页。
    </div>
  </div>
</template>
