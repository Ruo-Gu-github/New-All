<script setup lang="ts">
import "element-plus/dist/index.css";
import { computed, nextTick, onMounted, onUnmounted, ref, watch } from "vue";
import {
  ElButton,
  ElCard,
  ElMessage,
  ElRadioButton,
  ElRadioGroup,
} from "element-plus";

const props = defineProps<{
  panelData?: {
    id: string;
    folderPath: string;
    patientName?: string;
    imageCount: number;
    width: number;
    height: number;
  } | null;
  // Increment to force reload even when folderPath is unchanged.
  reloadKey?: number;
}>();

type Stage = 1 | 2 | 3;
const stage = ref<Stage>(1);

type LayoutChoice = "quad" | "axial" | "coronal" | "sagittal" | "view3d";
const layoutChoice = ref<LayoutChoice>("quad");

const sessionId = ref<string>("");
const isLoaded = ref(false);
let isLoading = false;

const viewAxialRef = ref<HTMLDivElement | null>(null);
const viewCoronalRef = ref<HTMLDivElement | null>(null);
const viewSagittalRef = ref<HTMLDivElement | null>(null);
const view3dRef = ref<HTMLDivElement | null>(null);

const windowIds = ref<{
  axial?: string;
  coronal?: string;
  sagittal?: string;
  view3d?: string;
}>({});

function computeRelativeRect(element: HTMLDivElement | null) {
  if (!element) return null;
  const rect = element.getBoundingClientRect();
  return {
    x: Math.round(rect.left),
    y: Math.round(rect.top),
    width: Math.max(1, Math.round(rect.width)),
    height: Math.max(1, Math.round(rect.height)),
  };
}

function stageVisible() {
  // 脂肪分析：阶段化显示（示例）
  // 1) 三切面定位
  // 2) 三切面（阈值/分割结果复核）
  // 3) 3D（脂肪体积/区域）复核
  if (stage.value === 1)
    return { axial: true, coronal: true, sagittal: true, view3d: false };
  if (stage.value === 2)
    return { axial: true, coronal: true, sagittal: true, view3d: false };
  return { axial: true, coronal: true, sagittal: false, view3d: true };
}

const visibility = computed(stageVisible);

function normalizeLayoutChoice() {
  const v = visibility.value;
  if (layoutChoice.value !== "quad") {
    if (layoutChoice.value === "axial" && !v.axial) layoutChoice.value = "quad";
    if (layoutChoice.value === "coronal" && !v.coronal)
      layoutChoice.value = "quad";
    if (layoutChoice.value === "sagittal" && !v.sagittal)
      layoutChoice.value = "quad";
    if (layoutChoice.value === "view3d" && !v.view3d)
      layoutChoice.value = "quad";
  }

  const enabled = [v.axial, v.coronal, v.sagittal, v.view3d].filter(
    Boolean
  ).length;
  if (layoutChoice.value === "quad" && enabled === 1) {
    if (v.axial) layoutChoice.value = "axial";
    else if (v.coronal) layoutChoice.value = "coronal";
    else if (v.sagittal) layoutChoice.value = "sagittal";
    else layoutChoice.value = "view3d";
  }
}

async function embedWindow(container: HTMLDivElement, windowId: string) {
  const metrics = computeRelativeRect(container);
  if (!metrics) return;
  const { x, y, width, height } = metrics;
  await window.visualizationApi.embedWindow(windowId, x, y, width, height);
}

async function updateNativeWindowLayouts() {
  if (!isLoaded.value) return;
  const ids = windowIds.value;
  if (!ids.axial || !ids.coronal || !ids.sagittal || !ids.view3d) return;

  const OFFSCREEN_X = -20000;
  const OFFSCREEN_Y = -20000;
  const OFFSCREEN_W = 1;
  const OFFSCREEN_H = 1;

  const views = [
    { key: "axial" as const, ref: viewAxialRef, id: ids.axial },
    { key: "coronal" as const, ref: viewCoronalRef, id: ids.coronal },
    { key: "sagittal" as const, ref: viewSagittalRef, id: ids.sagittal },
    { key: "view3d" as const, ref: view3dRef, id: ids.view3d },
  ];

  const vis = visibility.value;

  if (layoutChoice.value === "quad") {
    await Promise.all(
      views.map(async (v) => {
        if (!vis[v.key]) {
          await window.visualizationApi.resizeWindow(
            v.id,
            OFFSCREEN_X,
            OFFSCREEN_Y,
            OFFSCREEN_W,
            OFFSCREEN_H
          );
          return;
        }
        const metrics = computeRelativeRect(v.ref.value);
        if (!metrics) return;
        const { x, y, width, height } = metrics;
        await window.visualizationApi.resizeWindow(v.id, x, y, width, height);
      })
    );
    return;
  }

  const selectedKey = layoutChoice.value;
  const selected = views.find((v) => v.key === selectedKey);
  const selectedMetrics = selected
    ? computeRelativeRect(selected.ref.value)
    : null;

  await Promise.all(
    views.map(async (v) => {
      if (v.key === selectedKey && selectedMetrics && vis[v.key]) {
        const { x, y, width, height } = selectedMetrics;
        await window.visualizationApi.resizeWindow(v.id, x, y, width, height);
      } else {
        await window.visualizationApi.resizeWindow(
          v.id,
          OFFSCREEN_X,
          OFFSCREEN_Y,
          OFFSCREEN_W,
          OFFSCREEN_H
        );
      }
    })
  );
}

async function stopRenderLoop() {
  try {
    await window.visualizationApi.stopRenderLoop();
  } catch {
    // ignore
  }
}

async function destroySession() {
  if (!sessionId.value) return;
  try {
    await window.visualizationApi.destroyAPR(sessionId.value);
  } catch {
    // ignore
  } finally {
    sessionId.value = "";
    isLoaded.value = false;
    windowIds.value = {};
  }
}

async function loadApr(folderPath: string) {
  if (isLoading) return;
  isLoading = true;
  try {
    await stopRenderLoop();
    await destroySession();

    sessionId.value = `fat_${Date.now()}_${Math.random()
      .toString(36)
      .slice(2)}`;
    const res = await window.visualizationApi.createAPR(
      sessionId.value,
      folderPath
    );
    if (!res.success) throw new Error(res.error || "创建 APR 失败");

    const axial = res.windowIdAxial ?? String(res.hwndAxial ?? "");
    const coronal = res.windowIdCoronal ?? String(res.hwndCoronal ?? "");
    const sagittal = res.windowIdSagittal ?? String(res.hwndSagittal ?? "");
    const view3d = res.windowId3D ?? String(res.hwnd3D ?? "");
    if (!axial || !coronal || !sagittal || !view3d)
      throw new Error("APR 窗口 ID 缺失");
    windowIds.value = { axial, coronal, sagittal, view3d };

    layoutChoice.value = "quad";
    await nextTick();
    await new Promise((r) => setTimeout(r, 120));

    if (viewAxialRef.value) await embedWindow(viewAxialRef.value, axial);
    if (viewCoronalRef.value) await embedWindow(viewCoronalRef.value, coronal);
    if (viewSagittalRef.value)
      await embedWindow(viewSagittalRef.value, sagittal);
    if (view3dRef.value) await embedWindow(view3dRef.value, view3d);

    isLoaded.value = true;
    await window.visualizationApi.startRenderLoop(60);
    await updateNativeWindowLayouts();
  } catch (e: any) {
    console.error("[脂肪分析] loadApr failed:", e);
    ElMessage.error(`脂肪分析加载失败: ${e?.message ?? String(e)}`);
    await destroySession();
  } finally {
    isLoading = false;
  }
}

watch(
  () => ({
    folderPath: props.panelData?.folderPath,
    reloadKey: props.reloadKey ?? 0,
  }),
  async (next, prev) => {
    const changed =
      !prev ||
      next.folderPath !== prev.folderPath ||
      next.reloadKey !== prev.reloadKey;
    if (!next.folderPath || !changed) return;
    await loadApr(next.folderPath);
  },
  { immediate: true }
);

watch([stage, layoutChoice], async () => {
  normalizeLayoutChoice();
  await nextTick();
  await updateNativeWindowLayouts();
});

onMounted(() => {
  let resizeTimer: number | null = null;
  const ro = new ResizeObserver(() => {
    if (!isLoaded.value) return;
    if (resizeTimer) window.clearTimeout(resizeTimer);
    resizeTimer = window.setTimeout(() => {
      void updateNativeWindowLayouts();
    }, 120);
  });
  if (viewAxialRef.value) ro.observe(viewAxialRef.value);
  if (viewCoronalRef.value) ro.observe(viewCoronalRef.value);
  if (viewSagittalRef.value) ro.observe(viewSagittalRef.value);
  if (view3dRef.value) ro.observe(view3dRef.value);

  const boundsListener = () => {
    void updateNativeWindowLayouts();
  };
  window.ipcRenderer?.on("electron-window-bounds-changed", boundsListener);

  (onUnmounted as any)(() => {
    ro.disconnect();
    window.ipcRenderer?.off("electron-window-bounds-changed", boundsListener);
  });
});

onUnmounted(async () => {
  await stopRenderLoop();
  await destroySession();
});

const stageHint = computed(() => {
  if (stage.value === 1) return "阶段 1：三切面定位（不显示 3D）";
  if (stage.value === 2) return "阶段 2：阈值/分割复核（三切面）";
  return "阶段 3：3D 结果复核（矢状位隐藏）";
});

function goPrevStage() {
  stage.value = Math.max(1, stage.value - 1) as Stage;
}
function goNextStage() {
  stage.value = Math.min(3, stage.value + 1) as Stage;
}

// Fat Analysis Logic
const minScanThreshold = ref(-190);
const maxScanThreshold = ref(-30);
const fatAnalysisResult = ref<{
  subcutaneous: number;
  visceral: number;
} | null>(null);
const isAnalyzing = ref(false);

async function runFatAnalysis() {
  if (!sessionId.value) return;
  isAnalyzing.value = true;
  try {
    // 1. Separate Fat
    // Using slice index 0 as start/seed or just passing a dummy value if the Algo iterates all z
    // The C++ algo 'SeprateFat' takes 5 args, including 'nowPos'.
    // We'll pass 0 or a middle slice if needed. Let's assume 0 for now.
    const res = await window.visualizationApi.fatAnalyzeSeparateFat(
      sessionId.value,
      0, // maskId / nowPos
      minScanThreshold.value,
      maxScanThreshold.value
    );

    if (res.success) {
      ElMessage.success("脂肪分离完成");

      // Calculate Stats: Count non-zero pixels in result buffers if they were returned to JS,
      // but passing large buffers to JS is slow.
      // Ideally the C++ returns the stats directly.
      // The current wrapper returns { visceral: Buffer, subcutaneous: Buffer ... }
      // We can count them here or just show success.
      // For now, let's just log it. Visualizing these buffers requires more work (creating masks from buffers).

      // TODO: Create masks from returned buffers to visualize them.
      // But for now, we just triggered the analysis.
    } else {
      ElMessage.error("脂肪分离失败: " + (res.error || "Unknown"));
    }
  } catch (e: any) {
    ElMessage.error("分析错误: " + e.message);
  } finally {
    isAnalyzing.value = false;
  }
}
</script>

<template>
  <div class="viewer-page">
    <div class="left-panel">
      <ElCard class="control-card">
        <div class="section">
          <div class="section-title">窗口切换</div>
          <ElRadioGroup v-model="layoutChoice" size="small">
            <ElRadioButton label="quad">全部</ElRadioButton>
            <ElRadioButton label="axial" :disabled="!visibility.axial"
              >轴位</ElRadioButton
            >
            <ElRadioButton label="coronal" :disabled="!visibility.coronal"
              >冠状位</ElRadioButton
            >
            <ElRadioButton label="sagittal" :disabled="!visibility.sagittal"
              >矢状位</ElRadioButton
            >
            <ElRadioButton label="view3d" :disabled="!visibility.view3d"
              >3D</ElRadioButton
            >
          </ElRadioGroup>
        </div>

        <div class="section">
          <div class="section-title">流程阶段</div>
          <ElRadioGroup v-model="stage" size="small">
            <ElRadioButton :label="1">阶段 1</ElRadioButton>
            <ElRadioButton :label="2">阶段 2</ElRadioButton>
            <ElRadioButton :label="3">阶段 3</ElRadioButton>
          </ElRadioGroup>
          <div class="analyzer-tab-hint">{{ stageHint }}</div>
          <div class="btn-row">
            <ElButton size="small" @click="goPrevStage">上一步</ElButton>
            <ElButton type="primary" size="small" @click="goNextStage"
              >下一步</ElButton
            >
          </div>
        </div>

        <div class="section" v-if="stage === 2">
          <div class="section-title">脂肪阈值设置</div>
          <div
            style="
              display: flex;
              gap: 10px;
              align-items: center;
              margin-bottom: 5px;
            "
          >
            <span style="width: 30px">Min:</span>
            <input
              type="number"
              v-model.number="minScanThreshold"
              style="width: 60px"
            />
          </div>
          <div style="display: flex; gap: 10px; align-items: center">
            <span style="width: 30px">Max:</span>
            <input
              type="number"
              v-model.number="maxScanThreshold"
              style="width: 60px"
            />
          </div>
          <ElButton
            type="primary"
            size="small"
            style="margin-top: 10px; width: 100%"
            :loading="isAnalyzing"
            @click="runFatAnalysis"
          >
            运行脂肪分离
          </ElButton>
        </div>

        <div class="section">
          <div class="section-title">脂肪分析</div>
          <div class="btn-row">
            <ElButton type="primary" size="small" :disabled="!props.panelData"
              >开始/继续</ElButton
            >
            <ElButton size="small" :disabled="!isLoaded">刷新布局</ElButton>
          </div>
          <div class="analyzer-tab-hint">
            右侧为 OpenGL 视图；每个视口显示位置标签，背景为黑色。
          </div>
        </div>
      </ElCard>
    </div>

    <div class="right-panel">
      <div v-if="layoutChoice === 'quad'" class="grid2x2">
        <ElCard class="view-cell"
          ><div ref="viewAxialRef" class="view-container">
            <div
              style="
                position: absolute;
                left: 10px;
                top: 10px;
                padding: 6px 10px;
                border-radius: 8px;
                background: rgba(6, 18, 30, 0.65);
                z-index: 2;
                pointer-events: none;
              "
            >
              轴位视图
            </div>
            <div
              style="
                position: absolute;
                inset: 0;
                display: flex;
                align-items: center;
                justify-content: center;
                opacity: 0.55;
                pointer-events: none;
              "
            >
              轴位视图
            </div>
          </div></ElCard
        >
        <ElCard class="view-cell"
          ><div ref="viewCoronalRef" class="view-container">
            <div
              style="
                position: absolute;
                left: 10px;
                top: 10px;
                padding: 6px 10px;
                border-radius: 8px;
                background: rgba(6, 18, 30, 0.65);
                z-index: 2;
                pointer-events: none;
              "
            >
              冠状位视图
            </div>
            <div
              style="
                position: absolute;
                inset: 0;
                display: flex;
                align-items: center;
                justify-content: center;
                opacity: 0.55;
                pointer-events: none;
              "
            >
              冠状位视图
            </div>
          </div></ElCard
        >
        <ElCard class="view-cell"
          ><div ref="viewSagittalRef" class="view-container">
            <div
              style="
                position: absolute;
                left: 10px;
                top: 10px;
                padding: 6px 10px;
                border-radius: 8px;
                background: rgba(6, 18, 30, 0.65);
                z-index: 2;
                pointer-events: none;
              "
            >
              矢状位视图
            </div>
            <div
              style="
                position: absolute;
                inset: 0;
                display: flex;
                align-items: center;
                justify-content: center;
                opacity: 0.55;
                pointer-events: none;
              "
            >
              矢状位视图
            </div>
          </div></ElCard
        >
        <ElCard class="view-cell"
          ><div ref="view3dRef" class="view-container">
            <div
              style="
                position: absolute;
                left: 10px;
                top: 10px;
                padding: 6px 10px;
                border-radius: 8px;
                background: rgba(6, 18, 30, 0.65);
                z-index: 2;
                pointer-events: none;
              "
            >
              3D 视图
            </div>
            <div
              style="
                position: absolute;
                inset: 0;
                display: flex;
                align-items: center;
                justify-content: center;
                opacity: 0.55;
                pointer-events: none;
              "
            >
              3D 视图
            </div>
          </div></ElCard
        >
      </div>

      <div v-else class="grid1x1">
        <ElCard class="view-cell" v-if="layoutChoice === 'axial'">
          <div ref="viewAxialRef" class="view-container">
            <div
              style="
                position: absolute;
                left: 10px;
                top: 10px;
                padding: 6px 10px;
                border-radius: 8px;
                background: rgba(6, 18, 30, 0.65);
                z-index: 2;
                pointer-events: none;
              "
            >
              轴位视图
            </div>
            <div
              style="
                position: absolute;
                inset: 0;
                display: flex;
                align-items: center;
                justify-content: center;
                opacity: 0.55;
                pointer-events: none;
              "
            >
              轴位视图
            </div>
          </div>
        </ElCard>

        <ElCard class="view-cell" v-else-if="layoutChoice === 'coronal'">
          <div ref="viewCoronalRef" class="view-container">
            <div
              style="
                position: absolute;
                left: 10px;
                top: 10px;
                padding: 6px 10px;
                border-radius: 8px;
                background: rgba(6, 18, 30, 0.65);
                z-index: 2;
                pointer-events: none;
              "
            >
              冠状位视图
            </div>
            <div
              style="
                position: absolute;
                inset: 0;
                display: flex;
                align-items: center;
                justify-content: center;
                opacity: 0.55;
                pointer-events: none;
              "
            >
              冠状位视图
            </div>
          </div>
        </ElCard>

        <ElCard class="view-cell" v-else-if="layoutChoice === 'sagittal'">
          <div ref="viewSagittalRef" class="view-container">
            <div
              style="
                position: absolute;
                left: 10px;
                top: 10px;
                padding: 6px 10px;
                border-radius: 8px;
                background: rgba(6, 18, 30, 0.65);
                z-index: 2;
                pointer-events: none;
              "
            >
              矢状位视图
            </div>
            <div
              style="
                position: absolute;
                inset: 0;
                display: flex;
                align-items: center;
                justify-content: center;
                opacity: 0.55;
                pointer-events: none;
              "
            >
              矢状位视图
            </div>
          </div>
        </ElCard>

        <ElCard class="view-cell" v-else>
          <div ref="view3dRef" class="view-container">
            <div
              style="
                position: absolute;
                left: 10px;
                top: 10px;
                padding: 6px 10px;
                border-radius: 8px;
                background: rgba(6, 18, 30, 0.65);
                z-index: 2;
                pointer-events: none;
              "
            >
              3D 视图
            </div>
            <div
              style="
                position: absolute;
                inset: 0;
                display: flex;
                align-items: center;
                justify-content: center;
                opacity: 0.55;
                pointer-events: none;
              "
            >
              3D 视图
            </div>
          </div>
        </ElCard>
      </div>
    </div>
  </div>
</template>
