<script setup lang="ts">
import "element-plus/dist/index.css";
import { computed, nextTick, onMounted, onUnmounted, ref, watch } from "vue";
import {
  ElButton,
  ElCard,
  ElInputNumber,
  ElMessage,
  ElOption,
  ElRadioButton,
  ElRadioGroup,
  ElSelect,
  ElSlider,
  ElSwitch,
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
}>();

type Stage = 1 | 2 | 3;
const stage = ref<Stage>(1);

type LayoutChoice = "quad" | "axial" | "coronal" | "sagittal" | "view3d";
const layoutChoice = ref<LayoutChoice>("quad");

const sessionId = ref<string>("");
const isLoaded = ref(false);
let isLoading = false;

const his4dPath = ref<string>("");
const frameCount = ref<number>(0);
const currentFrame = ref<number>(0);
const isPlaying = ref(false);
const frameIntervalMs = ref<number>(100);
const frameStep = ref<number>(1);
let playTimer: number | null = null;
let isSwitchingFrame = false;

// 3D 显示：默认用 Raycast（体渲染），更符合“动态 3D”预期
const useRaycast3d = ref(true);
const vramOptimized3d = ref(true);
// 3D 显示：使用当前帧肺mask等值面（shader hit，不生成网格）
const maskIsoSurface3d = ref(false);

const maskStoreBase = ref<string>("");
const savedMaskFrames = new Set<number>();
const syncMaskDuringPlayback = ref(false);

const exportMaskId = ref<number>(1);
const exportStep = ref<number>(1);

// 肺部分析：本模块约束“每帧仅一个mask”。
// 二值化时会替换（删除旧mask并创建新mask），后续仅编辑这个mask。
const currentSingleMaskId = ref<number | null>(null);
const binarizeMin = ref<number>(-1024);
const binarizeMax = ref<number>(-300);
const binarizeColor = "#00ff00";
const binarizeName = "lung";

function requireIpcRenderer() {
  if (!window.ipcRenderer) {
    throw new Error("ipcRenderer not available (not running under Electron?)");
  }
  return window.ipcRenderer;
}

function resetMaskFrameStore() {
  maskStoreBase.value = "";
  savedMaskFrames.clear();
}

function frameFolderName(frameIndex: number) {
  return `frame_${String(frameIndex).padStart(3, "0")}`;
}

function getFrameMaskFolder(frameIndex: number) {
  if (!maskStoreBase.value) return "";
  return `${maskStoreBase.value}\\${frameFolderName(frameIndex)}`;
}

async function saveMasksForFrame(frameIndex: number, silent = false) {
  if (!sessionId.value) return false;
  const folderPath = getFrameMaskFolder(frameIndex);
  if (!folderPath) return false;

  try {
    const res = await window.visualizationApi.saveMasks(
      sessionId.value,
      folderPath,
      "lung"
    );
    if (!res?.success) throw new Error(res?.error || "保存Mask失败");
    savedMaskFrames.add(frameIndex);
    if (!silent) ElMessage.success(`已保存: ${frameFolderName(frameIndex)}`);
    return true;
  } catch (e: any) {
    if (!silent) ElMessage.error(`保存Mask失败: ${e?.message ?? String(e)}`);
    return false;
  }
}

async function enforceSingleMaskForThisFrame(
  loadedMasks: any[] | undefined,
  silent = false
) {
  const masks = Array.isArray(loadedMasks) ? loadedMasks : [];
  if (masks.length <= 1) {
    if (masks.length === 1 && masks[0]?.maskId != null) {
      exportMaskId.value = Number(masks[0].maskId);
      currentSingleMaskId.value = Number(masks[0].maskId);
    } else {
      currentSingleMaskId.value = null;
    }
    return;
  }

  const keep = masks[0];
  const keepId = keep?.maskId;
  if (keepId != null) {
    exportMaskId.value = Number(keepId);
    currentSingleMaskId.value = Number(keepId);
  }

  // 删除多余的mask，确保“每帧仅一个mask”（本模块约束）。
  for (let i = 1; i < masks.length; i++) {
    const id = masks[i]?.maskId;
    if (id == null) continue;
    try {
      // eslint-disable-next-line no-await-in-loop
      await window.visualizationApi.deleteMask(sessionId.value, Number(id));
    } catch (e) {
      console.warn("[肺部分析] delete extra mask failed:", id, e);
    }
  }

  if (!silent) {
    ElMessage.warning(
      `本帧检测到多个mask，已自动保留 maskId=${keepId}，删除其余 ${
        masks.length - 1
      } 个`
    );
  }
}

async function replaceMaskByBinarization() {
  if (!isLoaded.value || !sessionId.value) {
    ElMessage.error("请先加载 .his4d");
    return;
  }

  pausePlayback();

  // 删除旧mask（本模块约束：只维护一个mask）
  if (currentSingleMaskId.value != null) {
    try {
      await window.visualizationApi.deleteMask(
        sessionId.value,
        currentSingleMaskId.value
      );
    } catch (e) {
      console.warn("[肺部分析] delete previous mask failed:", e);
    } finally {
      currentSingleMaskId.value = null;
    }
  }

  const minT = Number(binarizeMin.value);
  const maxT = Number(binarizeMax.value);
  if (!Number.isFinite(minT) || !Number.isFinite(maxT) || minT > maxT) {
    ElMessage.error("二值化阈值不合法：要求 min <= max");
    return;
  }

  try {
    const res = await window.visualizationApi.createMaskFromThreshold(
      sessionId.value,
      minT,
      maxT,
      binarizeColor,
      binarizeName
    );
    if (!res?.success) throw new Error(res?.error || "二值化创建mask失败");

    const newId = Number(res.maskId);
    if (!Number.isFinite(newId) || newId < 0)
      throw new Error("二值化返回maskId无效");

    currentSingleMaskId.value = newId;
    exportMaskId.value = newId;

    // 让 ROI / FloodFill 等编辑操作绑定到这个mask
    await window.visualizationApi.selectMaskForEditing(sessionId.value, newId);

    ElMessage.success(`已二值化生成新mask: maskId=${newId}`);
  } catch (e: any) {
    ElMessage.error(`二值化失败: ${e?.message ?? String(e)}`);
  }
}

async function loadMasksForFrame(frameIndex: number, silent = false) {
  if (!sessionId.value) return false;
  const folderPath = getFrameMaskFolder(frameIndex);
  if (!folderPath) return false;

  try {
    const res = await window.visualizationApi.loadMasks(
      sessionId.value,
      folderPath
    );
    if (!res?.success) {
      if (res?.cancelled) return false;
      throw new Error(res?.error || "加载Mask失败");
    }

    // 本模块：每帧只允许一个mask。
    await enforceSingleMaskForThisFrame(res?.masks, true);

    // 如果存在mask，确保编辑目标也指向它。
    if (currentSingleMaskId.value != null) {
      try {
        await window.visualizationApi.selectMaskForEditing(
          sessionId.value,
          currentSingleMaskId.value
        );
      } catch (e) {
        console.warn("[肺部分析] selectMaskForEditing after load failed:", e);
      }
    }

    savedMaskFrames.add(frameIndex);
    if (!silent) ElMessage.success(`已加载: ${frameFolderName(frameIndex)}`);
    return true;
  } catch (e: any) {
    if (!silent) ElMessage.error(`加载Mask失败: ${e?.message ?? String(e)}`);
    return false;
  }
}

async function saveCurrentFrameMasks() {
  if (!isLoaded.value || !sessionId.value) {
    ElMessage.error("请先加载 .his4d");
    return;
  }
  pausePlayback();
  await saveMasksForFrame(currentFrame.value);
}

// 将当前 mask 作为“基线”保存到后续所有帧（这样后续可逐帧单独修改并保存）
async function applyMasksToLaterFrames() {
  if (!isLoaded.value || !sessionId.value) {
    ElMessage.error("请先加载 .his4d");
    return;
  }
  pausePlayback();

  const start = clampFrameIndex(currentFrame.value);
  const total = Math.max(0, frameCount.value);
  if (total <= 0) return;

  for (let i = start; i < total; i++) {
    // 不切帧：保存的是同一份 mask（作为基线），写入各帧独立目录
    // 后续用户在某一帧修改后，再“保存本帧”即可形成逐帧差异。
    // eslint-disable-next-line no-await-in-loop
    await saveMasksForFrame(i, true);
  }

  ElMessage.success("已应用到后续帧（基线已逐帧保存）");
}

async function exportSelectedMaskToStl() {
  if (!isLoaded.value || !sessionId.value) {
    ElMessage.error("请先加载 .his4d");
    return;
  }

  const maskId = Math.floor(Number(exportMaskId.value) || 0);
  const step = clampFrameStep(exportStep.value);
  if (maskId < 0) {
    ElMessage.error("maskId 必须 >= 0");
    return;
  }

  pausePlayback();

  const defaultName = `mask_${maskId}_step${step}.stl`;
  const filePath = await window.electronAPI?.invoke(
    "app:save-stl-file",
    defaultName
  );
  if (!filePath) return;

  try {
    const res = await window.visualizationApi.exportMaskToStl(
      sessionId.value,
      maskId,
      String(filePath),
      step
    );
    if (!res?.success) throw new Error(res?.error || "导出失败");
    ElMessage.success(`已导出 STL: ${filePath}`);
  } catch (e: any) {
    ElMessage.error(`导出 STL 失败: ${e?.message ?? String(e)}`);
  }
}

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
  // 肺部分析：阶段化显示（示例）
  // 1) 三切面定位
  // 2) 三切面 + 3D（结节/分割复核）
  // 3) 只看 3D（定量结果）
  if (stage.value === 1)
    return { axial: true, coronal: true, sagittal: true, view3d: false };
  if (stage.value === 2)
    return { axial: true, coronal: true, sagittal: true, view3d: true };
  // 阶段 3：仅 3D（独立播放/复核）
  return { axial: false, coronal: false, sagittal: false, view3d: true };
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

function stopPlayback() {
  if (playTimer) {
    window.clearInterval(playTimer);
    playTimer = null;
  }
  isPlaying.value = false;
}

function pausePlayback() {
  if (!isPlaying.value) return;
  stopPlayback();
}

// 打开 ROI 编辑对话框（非模态）
async function openRoiEditDialog() {
  if (!sessionId.value) {
    ElMessage.error("请先加载 .his4d");
    return;
  }

  pausePlayback();

  try {
    if (window.nativeBridge?.dialog?.open) {
      const result = await window.nativeBridge.dialog.open("roiedit", {
        sessionId: sessionId.value,
      });

      if (!result.success) {
        ElMessage.error(result.error || "打开ROI编辑失败");
      }
    } else {
      ElMessage.error("Dialog API 未初始化");
    }
  } catch (e: any) {
    ElMessage.error(`打开ROI编辑失败: ${e?.message ?? String(e)}`);
  }
}

function clampFrameIndex(i: number) {
  const max = Math.max(0, frameCount.value - 1);
  return Math.min(max, Math.max(0, Math.floor(i)));
}

function clampFrameStep(v: number) {
  const n = Math.floor(Number(v) || 1);
  return Math.min(32, Math.max(1, n));
}

async function apply3DRenderSettings() {
  if (!isLoaded.value) return;
  const id3d = windowIds.value.view3d;
  if (!id3d) return;
  try {
    await window.visualizationApi.set3DOrthogonalMode(
      id3d,
      !useRaycast3d.value
    );
    await (window as any).visualizationApi?.set3DMaskIsoSurface?.(
      id3d,
      !!maskIsoSurface3d.value
    );
    await (window as any).visualizationApi?.set3DVramOptimized?.(
      id3d,
      !!vramOptimized3d.value
    );
  } catch (e) {
    console.warn("[肺部分析] apply3DRenderSettings failed:", e);
  }
}

async function setHis4dFrame(i: number) {
  if (!sessionId.value) return;
  const idx = clampFrameIndex(i);
  currentFrame.value = idx;
  try {
    const res = await window.visualizationApi.his4dSetFrame(
      sessionId.value,
      idx
    );
    if (!res?.success) throw new Error(res?.error || "切帧失败");

    // 默认：仅在暂停状态下按需加载该帧mask快照；播放时避免磁盘IO
    // 可选：开启“播放同步Mask”后，将按帧加载（可能会卡顿）
    const shouldLoadMask =
      savedMaskFrames.has(idx) &&
      (!isPlaying.value || syncMaskDuringPlayback.value);
    if (shouldLoadMask) {
      await loadMasksForFrame(idx, true);
    }
  } catch (e: any) {
    stopPlayback();
    ElMessage.error(`切帧失败: ${e?.message ?? String(e)}`);
  }
}

async function destroySession() {
  stopPlayback();
  if (!sessionId.value) return;
  try {
    await window.visualizationApi.destroyAPR(sessionId.value);
  } catch {
    // ignore
  } finally {
    sessionId.value = "";
    isLoaded.value = false;
    windowIds.value = {};
    his4dPath.value = "";
    frameCount.value = 0;
    currentFrame.value = 0;
    resetMaskFrameStore();
  }
}

function computeFrameIntervalFromTimestamps(timestampsMs?: number[]) {
  if (!timestampsMs || timestampsMs.length < 2) return 100;
  let sum = 0;
  let n = 0;
  for (let i = 1; i < timestampsMs.length; i++) {
    const dt = Number(timestampsMs[i]) - Number(timestampsMs[i - 1]);
    if (Number.isFinite(dt) && dt > 0) {
      sum += dt;
      n++;
    }
  }
  if (n <= 0) return 100;
  const avg = Math.round(sum / n);
  return Math.min(1000, Math.max(30, avg));
}

async function loadAprFromHis4d(filePath: string) {
  if (isLoading) return;
  isLoading = true;
  try {
    stopPlayback();
    await stopRenderLoop();
    await destroySession();

    sessionId.value = `lung4d_${Date.now()}_${Math.random()
      .toString(36)
      .slice(2)}`;
    const res = await window.visualizationApi.createAPRFromHis4d(
      sessionId.value,
      filePath
    );
    if (!res?.success) throw new Error(res?.error || "创建 HIS4D APR 失败");

    his4dPath.value = res.his4dPath ?? filePath;
    // 每次加载新his4d，重置逐帧mask存储
    resetMaskFrameStore();
    maskStoreBase.value = `${his4dPath.value}.masks\\${sessionId.value}`;
    frameCount.value = Number(res.frameCount ?? 0);
    frameIntervalMs.value = computeFrameIntervalFromTimestamps(
      res.timestampsMs
    );

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
    currentSingleMaskId.value = null;
    await apply3DRenderSettings();
    await window.visualizationApi.startRenderLoop(60);
    await updateNativeWindowLayouts();

    currentFrame.value = 0;
    if (frameCount.value > 1) {
      await setHis4dFrame(0);
    }
  } catch (e: any) {
    console.error("[肺部分析] loadAprFromHis4d failed:", e);
    ElMessage.error(`HIS4D 加载失败: ${e?.message ?? String(e)}`);
    await destroySession();
  } finally {
    isLoading = false;
  }
}

async function chooseHis4dAndLoad() {
  const ipc = requireIpcRenderer();
  const p = await ipc.invoke("app:select-his4d-file");
  if (!p) return;
  await loadAprFromHis4d(String(p));
}

async function chooseFoldersPackAndLoad() {
  const ipc = requireIpcRenderer();
  const folders = (await ipc.invoke("app:select-directories")) as
    | string[]
    | null;
  if (!folders || folders.length === 0) return;
  const outputPath = (await ipc.invoke("app:save-his4d-file")) as string | null;
  if (!outputPath) return;

  const sortedFolders = [...folders].map(String).sort();
  try {
    const packRes = await window.visualizationApi.packHis4dFromFolders(
      outputPath,
      sortedFolders
    );
    if (!packRes?.success) throw new Error(packRes?.error || "打包失败");
    const out = packRes.outputPath ?? outputPath;
    await loadAprFromHis4d(out);
  } catch (e: any) {
    ElMessage.error(`打包 HIS4D 失败: ${e?.message ?? String(e)}`);
  }
}

async function togglePlayback() {
  if (!isLoaded.value || frameCount.value <= 1) return;
  if (isPlaying.value) {
    stopPlayback();
    return;
  }

  isPlaying.value = true;
  const interval = Math.max(30, Number(frameIntervalMs.value) || 100);
  playTimer = window.setInterval(() => {
    if (!isPlaying.value) return;
    if (isSwitchingFrame) return;
    isSwitchingFrame = true;
    const step = clampFrameStep(frameStep.value);
    const next = (currentFrame.value + step) % Math.max(1, frameCount.value);
    void setHis4dFrame(next).finally(() => {
      isSwitchingFrame = false;
    });
  }, interval);
}

function onFrameSliderChange(v: number | number[]) {
  pausePlayback();
  const value = Array.isArray(v) ? v[0] ?? 0 : v;
  void setHis4dFrame(value);
}

async function loadApr(folderPath: string) {
  // NOTE: Lung module must load independently (HIS4D only).
  // This function is intentionally disabled to avoid reusing cached volumes.
  void folderPath;
  ElMessage.warning(
    "肺容积分析仅支持在模块内加载 .his4d（不支持从序列自动加载）"
  );
}

watch([useRaycast3d, vramOptimized3d, maskIsoSurface3d], async () => {
  await apply3DRenderSettings();
});

watch(useRaycast3d, (enabled) => {
  // iso-surface 依赖 raycast 路径；关闭 raycast 时自动关闭等值面
  if (!enabled) maskIsoSurface3d.value = false;
});

watch(maskIsoSurface3d, (enabled) => {
  // 打开等值面时强制启用 raycast
  if (enabled) useRaycast3d.value = true;
});

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
  stopPlayback();
  await stopRenderLoop();
  await destroySession();
});

const stageHint = computed(() => {
  if (stage.value === 1) return "阶段 1：三切面定位（不显示 3D）";
  if (stage.value === 2) return "阶段 2：三切面 + 3D 复核（分割/结节）";
  return "阶段 3：仅 3D（独立播放/复核）";
});

function goPrevStage() {
  stage.value = Math.max(1, stage.value - 1) as Stage;
}
function goNextStage() {
  stage.value = Math.min(3, stage.value + 1) as Stage;
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

        <div class="section">
          <div class="section-title">肺容积分析（4D）</div>
          <div class="btn-row">
            <ElButton type="primary" size="small" @click="chooseHis4dAndLoad"
              >加载 .his4d</ElButton
            >
            <ElButton size="small" @click="chooseFoldersPackAndLoad"
              >选择文件夹并打包</ElButton
            >
          </div>

          <div class="btn-row" style="margin-top: 10px; align-items: center">
            <div style="font-size: 12px; opacity: 0.8; min-width: 54px">
              二值化
            </div>
            <span style="font-size: 12px; opacity: 0.8">Min</span>
            <ElInputNumber
              v-model="binarizeMin"
              size="small"
              controls-position="right"
              style="width: 120px"
            />
            <span style="font-size: 12px; opacity: 0.8">Max</span>
            <ElInputNumber
              v-model="binarizeMax"
              size="small"
              controls-position="right"
              style="width: 120px"
            />
            <ElButton
              size="small"
              type="primary"
              :disabled="!isLoaded"
              @click="replaceMaskByBinarization"
            >
              二值化(新建Mask)
            </ElButton>
            <div style="font-size: 12px; opacity: 0.8; padding-left: 8px">
              每次二值化会删除旧mask，仅保留1个。
            </div>
          </div>

          <div class="btn-row" style="margin-top: 10px">
            <ElButton
              size="small"
              type="primary"
              :disabled="!isLoaded"
              @click="openRoiEditDialog"
            >
              ROI编辑
            </ElButton>
            <ElButton
              size="small"
              :disabled="!isLoaded"
              @click="saveCurrentFrameMasks"
            >
              保存本帧Mask
            </ElButton>
            <ElButton
              size="small"
              :disabled="!isLoaded || frameCount <= 1"
              @click="applyMasksToLaterFrames"
            >
              应用到后续帧
            </ElButton>
            <div style="font-size: 12px; opacity: 0.8; padding-left: 8px">
              ROI完成后点“应用到后续帧”；不对的帧再单独改并保存。
            </div>
          </div>

          <div class="btn-row" style="margin-top: 10px; align-items: center">
            <div style="font-size: 12px; opacity: 0.8">导出 STL</div>
            <ElInputNumber
              v-model="exportMaskId"
              :min="0"
              :step="1"
              size="small"
              controls-position="right"
              style="width: 120px"
            />
            <ElSelect v-model="exportStep" size="small" style="width: 120px">
              <ElOption :value="1" label="step 1 (高)" />
              <ElOption :value="2" label="step 2" />
              <ElOption :value="4" label="step 4" />
              <ElOption :value="8" label="step 8 (低)" />
            </ElSelect>
            <ElButton
              size="small"
              :disabled="!isLoaded"
              @click="exportSelectedMaskToStl"
            >
              导出当前 maskId
            </ElButton>
          </div>
          <div class="btn-row" style="margin-top: 10px">
            <ElButton
              size="small"
              :disabled="!isLoaded || frameCount <= 1"
              @click="togglePlayback"
            >
              {{ isPlaying ? "暂停" : "播放" }}
            </ElButton>
            <div
              style="
                display: flex;
                align-items: center;
                gap: 8px;
                padding-left: 12px;
              "
            >
              <ElSwitch
                v-model="useRaycast3d"
                :disabled="!isLoaded || !visibility.view3d"
                size="small"
              />
              <div style="font-size: 12px; opacity: 0.8">
                3D 体渲染（Raycast）
              </div>
            </div>
            <div
              style="
                display: flex;
                align-items: center;
                gap: 8px;
                padding-left: 12px;
              "
            >
              <ElSwitch
                v-model="maskIsoSurface3d"
                :disabled="!isLoaded || !visibility.view3d"
                size="small"
              />
              <div style="font-size: 12px; opacity: 0.8">3D 肺Mask等值面</div>
            </div>
            <div
              style="
                display: flex;
                align-items: center;
                gap: 8px;
                padding-left: 12px;
              "
            >
              <ElSwitch
                v-model="vramOptimized3d"
                :disabled="!isLoaded || !visibility.view3d"
                size="small"
              />
              <div style="font-size: 12px; opacity: 0.8">3D 显存优化</div>
            </div>
            <div
              style="
                display: flex;
                align-items: center;
                gap: 8px;
                padding-left: 12px;
              "
            >
              <ElSwitch
                v-model="syncMaskDuringPlayback"
                :disabled="!isLoaded || frameCount <= 1"
                size="small"
              />
              <div style="font-size: 12px; opacity: 0.8">
                播放同步Mask（可能卡顿）
              </div>
            </div>
            <div style="font-size: 12px; opacity: 0.8; padding-left: 8px">
              帧 {{ currentFrame + 1 }} / {{ Math.max(frameCount, 1) }}
              <span v-if="his4dPath" style="opacity: 0.75"
                >（{{ his4dPath }}）</span
              >
            </div>
          </div>
          <div class="btn-row" style="margin-top: 10px; align-items: center">
            <div style="font-size: 12px; opacity: 0.8; width: 92px">
              播放间隔
            </div>
            <ElSlider
              v-model="frameIntervalMs"
              :min="30"
              :max="1000"
              :step="10"
              :disabled="!isLoaded || frameCount <= 1"
              style="flex: 1"
            />
            <div
              style="
                font-size: 12px;
                opacity: 0.8;
                width: 72px;
                text-align: right;
              "
            >
              {{ frameIntervalMs }}ms
            </div>
          </div>
          <div class="btn-row" style="margin-top: 6px; align-items: center">
            <div style="font-size: 12px; opacity: 0.8; width: 92px">
              步长（抽帧）
            </div>
            <ElSlider
              v-model="frameStep"
              :min="1"
              :max="16"
              :step="1"
              :disabled="!isLoaded || frameCount <= 1"
              style="flex: 1"
            />
            <div
              style="
                font-size: 12px;
                opacity: 0.8;
                width: 72px;
                text-align: right;
              "
            >
              x{{ frameStep }}
            </div>
          </div>
          <ElSlider
            v-model="currentFrame"
            :min="0"
            :max="Math.max(0, frameCount - 1)"
            :disabled="!isLoaded || frameCount <= 1"
            @change="onFrameSliderChange"
          />
          <div class="analyzer-tab-hint">
            右侧为 OpenGL 视图；每个视口显示位置标签，背景为黑色。
          </div>
        </div>
      </ElCard>
    </div>

    <div class="right-panel">
      <div v-if="layoutChoice === 'quad'" class="grid2x2">
        <ElCard class="view-cell"
          ><div
            ref="viewAxialRef"
            class="view-container"
            @mousedown="pausePlayback"
          >
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
          ><div
            ref="viewCoronalRef"
            class="view-container"
            @mousedown="pausePlayback"
          >
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
          ><div
            ref="viewSagittalRef"
            class="view-container"
            @mousedown="pausePlayback"
          >
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
          ><div
            ref="view3dRef"
            class="view-container"
            @mousedown="pausePlayback"
          >
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
          <div
            ref="viewAxialRef"
            class="view-container"
            @mousedown="pausePlayback"
          >
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
          <div
            ref="viewCoronalRef"
            class="view-container"
            @mousedown="pausePlayback"
          >
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
          <div
            ref="viewSagittalRef"
            class="view-container"
            @mousedown="pausePlayback"
          >
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
          <div
            ref="view3dRef"
            class="view-container"
            @mousedown="pausePlayback"
          >
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
