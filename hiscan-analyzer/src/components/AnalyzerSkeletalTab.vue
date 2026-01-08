<script setup lang="ts">
import "element-plus/dist/index.css";
import { nextTick, onMounted, onUnmounted, ref, watch } from "vue";
import {
  ElButton,
  ElCard,
  ElMessage,
  ElRadioButton,
  ElRadioGroup,
  ElInput,
  ElInputNumber,
  ElSlider,
  ElTable,
  ElTableColumn,
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

const emit = defineEmits<{
  (e: "switch-tab", tabName: string, panelData?: any): void;
}>();

type Stage = 1 | 2 | 3;
const stage = ref<Stage>(1);

// 阶段完成标志（仿 MFC 版本：步骤完成后才显示下一步按钮）
const stage1Completed = ref(false); // 裁切范围已应用
const stage2Completed = ref(false); // ROI 编辑已确认
const stage3Completed = ref(false); // 二值化已完成

type SessionKind = "apr" | "mpr";
const sessionKind = ref<SessionKind>("apr");

type LayoutChoice = "quad" | "axial" | "coronal" | "sagittal" | "view3d";
const layoutChoice = ref<LayoutChoice>("quad");

const sessionId = ref<string>("");
const isLoaded = ref(false);
let isLoading = false;

const currentFolderPath = ref<string>("");

const viewAxialRef = ref<HTMLDivElement | null>(null);
const viewCoronalRef = ref<HTMLDivElement | null>(null);
const viewSagittalRef = ref<HTMLDivElement | null>(null);

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

type MaskItem = {
  maskId?: number;
  name: string;
  color: string;
  visible: boolean;
  min: number;
  max: number;
};

const histogramCanvas = ref<HTMLCanvasElement | null>(null);
const histogramData = ref<number[]>([]);
const histogramMinValue = ref(-1024);
const histogramMaxValue = ref(3071);

const thresholdMin = ref(150);
const thresholdMax = ref(3071);
const previewColor = ref("#ff0000");
const maskName = ref("Bone");

const masks = ref<MaskItem[]>([]);
const selectedMask = ref<MaskItem | null>(null);
const selectedRoiMaskId = ref<number | null>(null);
const saveName = ref("bone_masks");

type AprCropBoxRange = {
  xStart: number;
  xEnd: number;
  yStart: number;
  yEnd: number;
  zStart: number;
  zEnd: number;
};

const aprCropRange = ref<AprCropBoxRange>({
  xStart: 0,
  xEnd: 0,
  yStart: 0,
  yEnd: 0,
  zStart: 0,
  zEnd: 0,
});
const aprCropRangeLoading = ref(false);

async function refreshAprCropRange() {
  if (!sessionId.value) return;
  if (!window.visualizationApi?.getAPRCropBox) return;

  aprCropRangeLoading.value = true;
  try {
    const box = await window.visualizationApi.getAPRCropBox(sessionId.value);
    if (box && typeof box === "object") {
      aprCropRange.value = {
        xStart: Number((box as any).xStart ?? 0),
        xEnd: Number((box as any).xEnd ?? 0),
        yStart: Number((box as any).yStart ?? 0),
        yEnd: Number((box as any).yEnd ?? 0),
        zStart: Number((box as any).zStart ?? 0),
        zEnd: Number((box as any).zEnd ?? 0),
      };
    }
  } catch (e) {
    console.warn("[骨骼分析] getAPRCropBox failed:", e);
  } finally {
    aprCropRangeLoading.value = false;
  }
}

async function applyAprCropRange() {
  if (!isLoaded.value || !sessionId.value) {
    ElMessage.warning("请先加载序列");
    return;
  }
  if (!window.visualizationApi?.setAPRCropBoxRange) {
    ElMessage.error("当前版本不支持裁切范围设置");
    return;
  }

  const r = aprCropRange.value;
  try {
    await window.visualizationApi.setAPRCropBoxRange(
      sessionId.value,
      Number(r.xStart),
      Number(r.xEnd),
      Number(r.yStart),
      Number(r.yEnd),
      Number(r.zStart),
      Number(r.zEnd)
    );
    await refreshAprCropRange();
    stage1Completed.value = true; // 标记阶段 1 完成
    ElMessage.success("已应用裁切范围，可进入阶段 2");
  } catch (e: any) {
    console.error("[骨骼分析] setAPRCropBoxRange failed:", e);
    ElMessage.error(`设置裁切范围失败: ${e?.message ?? String(e)}`);
  }
}

async function resetAprCropRange() {
  aprCropRange.value = {
    xStart: 0,
    xEnd: 0,
    yStart: 0,
    yEnd: 0,
    zStart: 0,
    zEnd: 0,
  };
  await applyAprCropRange();
}

async function openRoiEdit() {
  if (!isLoaded.value || !sessionId.value) {
    ElMessage.warning("请先加载序列");
    return;
  }

  try {
    // Ensure we have an ROI mask to draw into.
    let roiMaskId: number | undefined = selectedRoiMaskId.value ?? undefined;
    if (roiMaskId == null) {
      const created = await window.visualizationApi.createEmptyMask(
        sessionId.value,
        "#ffff00",
        "ROI"
      );
      if (!created?.success)
        throw new Error(created?.error || "创建ROI mask失败");
      if (created.maskId == null)
        throw new Error("创建ROI mask失败: maskId为空");
      roiMaskId = created.maskId;
      selectedRoiMaskId.value = roiMaskId ?? null;

      // Also show it in the list for later selection.
      masks.value.push({
        maskId: roiMaskId,
        name: "ROI",
        color: "#ffff00",
        visible: true,
        min: 0,
        max: 0,
      });
    }

    if (roiMaskId == null) throw new Error("ROI maskId为空");

    await window.visualizationApi.selectMaskForEditing(
      sessionId.value,
      roiMaskId
    );
    // MaskEdit mode = toolType 7
    const ids = windowIds.value;
    if (ids.axial)
      await window.visualizationApi.setWindowToolType(ids.axial, 7);
    if (ids.coronal)
      await window.visualizationApi.setWindowToolType(ids.coronal, 7);
    if (ids.sagittal)
      await window.visualizationApi.setWindowToolType(ids.sagittal, 7);

    // Default to Brush
    await window.visualizationApi.setMaskTool(1);

    await window.nativeBridge?.dialog?.open?.("roiedit", {
      sessionId: sessionId.value,
      roiColor: "#ffff00",
    });
  } catch (e: any) {
    console.error("[骨骼分析] openRoiEdit failed:", e);
    ElMessage.error(`打开ROI编辑失败: ${e?.message ?? String(e)}`);
  }
}

// 确认 ROI（标记阶段 2 完成）
function confirmRoi() {
  if (selectedRoiMaskId.value == null) {
    ElMessage.warning("请先创建并编辑 ROI");
    return;
  }
  stage2Completed.value = true;
  ElMessage.success("ROI 已确认，可进入阶段 3");
}

// 确认二值化（标记阶段 3 完成）
function confirmBinarization() {
  if (masks.value.length === 0) {
    ElMessage.warning("请先生成阈值分割掩膜");
    return;
  }
  stage3Completed.value = true;
  ElMessage.success("二值化已确认，可进行指标计算");
}

type BoneMetrics = {
  maskId: number;
  roiMaskId: number;
  voxelCount: number;
  roiVoxelCount: number;
  tvRoiMm3: number;
  mvRoiMm3: number;
  bv_tv_roi: number;
  volumeMm3: number;
  volumeCm3: number;
  surfaceAreaMm2: number;
  surfaceAreaCm2: number;
  bs_bv_1_per_mm: number;
  tvBoxMm3: number;
  bv_tv: number;
  tbThMm: number;
  tbSpMm: number;
  tbNm_1_per_mm: number;
  smi: number;
  da: number;
  daEigen1: number;
  daEigen2: number;
  daEigen3: number;
  centroidXmm: number;
  centroidYmm: number;
  centroidZmm: number;
  bboxMinX: number;
  bboxMinY: number;
  bboxMinZ: number;
  bboxMaxX: number;
  bboxMaxY: number;
  bboxMaxZ: number;
};

const boneMetrics = ref<BoneMetrics | null>(null);
const boneMetricsLoading = ref(false);

function fmtNum(v: any, digits = 3) {
  const n = Number(v);
  if (!Number.isFinite(n)) return "-";
  return n.toFixed(digits);
}

async function calculateSelectedMetrics() {
  if (!sessionId.value) {
    ElMessage.warning("请先加载序列");
    return;
  }
  if (!selectedMask.value || selectedMask.value.maskId == null) {
    ElMessage.warning("请先选择一个掩膜");
    return;
  }

  boneMetricsLoading.value = true;
  try {
    const res = await window.visualizationApi.calculateBoneMetrics(
      sessionId.value,
      selectedMask.value.maskId,
      selectedRoiMaskId.value ?? undefined
    );
    if (res?.success === false) {
      throw new Error(res?.error || "计算失败");
    }
    boneMetrics.value = res as BoneMetrics;
  } catch (e: any) {
    console.error("[骨骼分析] calculateSelectedMetrics failed:", e);
    ElMessage.error(`计算指标失败: ${e?.message ?? String(e)}`);
    boneMetrics.value = null;
  } finally {
    boneMetricsLoading.value = false;
  }
}

const boneMetricsRows = ref<{ name: string; value: string }[]>([]);
watch([boneMetrics], () => {
  const m = boneMetrics.value;
  if (!m) {
    boneMetricsRows.value = [];
    return;
  }
  boneMetricsRows.value = [
    { name: "体素数", value: String(m.voxelCount ?? 0) },
    { name: "TV (ROI, mm³)", value: fmtNum(m.tvRoiMm3, 2) },
    { name: "MV (ROI, mm³)", value: fmtNum(m.mvRoiMm3, 2) },
    { name: "体积 (mm³)", value: fmtNum(m.volumeMm3, 2) },
    { name: "体积 (cm³)", value: fmtNum(m.volumeCm3, 4) },
    { name: "表面积 (mm²)", value: fmtNum(m.surfaceAreaMm2, 2) },
    { name: "BS/BV (1/mm)", value: fmtNum(m.bs_bv_1_per_mm, 5) },
    { name: "BV/TV (ROI)", value: fmtNum(m.bv_tv_roi, 5) },
    { name: "BV/TV (bbox)", value: fmtNum(m.bv_tv, 5) },
    { name: "Tb.Th (mm)", value: fmtNum(m.tbThMm, 5) },
    { name: "Tb.Sp (mm)", value: fmtNum(m.tbSpMm, 5) },
    { name: "Tb.N (1/mm)", value: fmtNum(m.tbNm_1_per_mm, 5) },
    { name: "SMI", value: fmtNum(m.smi, 4) },
    { name: "DA", value: fmtNum(m.da, 4) },
    { name: "DA Eigen1", value: fmtNum(m.daEigen1, 4) },
    { name: "DA Eigen2", value: fmtNum(m.daEigen2, 4) },
    { name: "DA Eigen3", value: fmtNum(m.daEigen3, 4) },
    {
      name: "质心 (mm)",
      value: `${fmtNum(m.centroidXmm, 2)}, ${fmtNum(
        m.centroidYmm,
        2
      )}, ${fmtNum(m.centroidZmm, 2)}`,
    },
    {
      name: "BBox (voxel)",
      value: `[${m.bboxMinX},${m.bboxMinY},${m.bboxMinZ}] - [${m.bboxMaxX},${m.bboxMaxY},${m.bboxMaxZ}]`,
    },
  ];
});

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
  ];

  // Skeletal tab does NOT embed a native 3D view.
  // Keep the native 3D window offscreen, and provide a "jump to 3D recon" entry instead.
  await window.visualizationApi.resizeWindow(
    ids.view3d,
    OFFSCREEN_X,
    OFFSCREEN_Y,
    OFFSCREEN_W,
    OFFSCREEN_H
  );

  if (layoutChoice.value === "quad") {
    await Promise.all(
      views.map(async (v) => {
        const metrics = computeRelativeRect(v.ref.value);
        if (!metrics) return;
        const { x, y, width, height } = metrics;
        await window.visualizationApi.resizeWindow(v.id, x, y, width, height);
      })
    );
    return;
  }

  if (layoutChoice.value === "view3d") {
    // User chose "3D": show only the jump card in UI.
    await Promise.all(
      views.map((v) =>
        window.visualizationApi.resizeWindow(
          v.id,
          OFFSCREEN_X,
          OFFSCREEN_Y,
          OFFSCREEN_W,
          OFFSCREEN_H
        )
      )
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
      if (v.key === selectedKey && selectedMetrics) {
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

async function destroySession(kind: SessionKind) {
  if (!sessionId.value) return;
  try {
    if (kind === "mpr") {
      await window.visualizationApi.destroyMPR(sessionId.value);
    } else {
      await window.visualizationApi.destroyAPR(sessionId.value);
    }
  } catch {
    // ignore
  } finally {
    sessionId.value = "";
    isLoaded.value = false;
    windowIds.value = {};
  }
}

async function openSaveMaskDialog() {
  if (!isLoaded.value || !sessionId.value) {
    ElMessage.warning("请先加载序列");
    return;
  }
  try {
    await window.nativeBridge?.dialog?.open?.("savemask", {
      sessionId: sessionId.value,
    });
  } catch (e: any) {
    ElMessage.error(`打开保存对话框失败: ${e?.message ?? String(e)}`);
  }
}

async function loadStage1Apr(folderPath: string) {
  if (isLoading) return;
  isLoading = true;
  try {
    await stopRenderLoop();
    await destroySession(sessionKind.value);

    stage.value = 1;
    sessionKind.value = "apr";

    currentFolderPath.value = folderPath;

    sessionId.value = `skeletal_${Date.now()}_${Math.random()
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

    isLoaded.value = true;
    await window.visualizationApi.startRenderLoop(60);

    // Load histogram + set default thresholds
    await refreshHistogram();

    // Step1: read current APR crop range
    await refreshAprCropRange();

    await updateNativeWindowLayouts();
  } catch (e: any) {
    console.error("[骨骼分析] loadApr failed:", e);
    ElMessage.error(`骨骼分析加载失败: ${e?.message ?? String(e)}`);
    await destroySession(sessionKind.value);
  } finally {
    isLoading = false;
  }
}

async function loadStage2Mpr(folderPath: string) {
  if (isLoading) return;
  isLoading = true;
  try {
    await stopRenderLoop();
    await destroySession(sessionKind.value);

    sessionKind.value = "mpr";
    currentFolderPath.value = folderPath;

    sessionId.value = `skeletal_mpr_${Date.now()}_${Math.random()
      .toString(36)
      .slice(2)}`;

    const res = await window.visualizationApi.createMPR(
      sessionId.value,
      folderPath
    );
    if (!res.success) throw new Error(res.error || "创建 MPR 失败");

    const axial = res.windowIdAxial ?? String(res.hwndAxial ?? "");
    const coronal = res.windowIdCoronal ?? String(res.hwndCoronal ?? "");
    const sagittal = res.windowIdSagittal ?? String(res.hwndSagittal ?? "");
    const view3d = res.windowId3D ?? String(res.hwnd3D ?? "");
    if (!axial || !coronal || !sagittal || !view3d)
      throw new Error("MPR 窗口 ID 缺失");
    windowIds.value = { axial, coronal, sagittal, view3d };

    layoutChoice.value = "quad";
    await nextTick();
    await new Promise((r) => setTimeout(r, 120));

    if (viewAxialRef.value) await embedWindow(viewAxialRef.value, axial);
    if (viewCoronalRef.value) await embedWindow(viewCoronalRef.value, coronal);
    if (viewSagittalRef.value)
      await embedWindow(viewSagittalRef.value, sagittal);

    isLoaded.value = true;
    await window.visualizationApi.startRenderLoop(60);

    // MPR also supports histogram and threshold-based masks.
    await refreshHistogram();
    await updateNativeWindowLayouts();
  } catch (e: any) {
    console.error("[骨骼分析] loadMpr failed:", e);
    ElMessage.error(`骨骼分析加载失败: ${e?.message ?? String(e)}`);
    await destroySession(sessionKind.value);
  } finally {
    isLoading = false;
  }
}

async function goToStage2() {
  if (!isLoaded.value || sessionKind.value !== "apr") {
    ElMessage.warning("请先完成阶段 1（APR + 裁切）加载");
    return;
  }
  if (!stage1Completed.value) {
    ElMessage.warning("请先应用裁切范围");
    return;
  }
  await loadStage2Mpr(currentFolderPath.value);
  stage.value = 2;
  stage2Completed.value = false; // 重置阶段2完成标志
}

function goToStage3() {
  if (!isLoaded.value || sessionKind.value !== "mpr") {
    ElMessage.warning("请先进入阶段 2（MPR + ROI）");
    return;
  }
  if (!stage2Completed.value) {
    ElMessage.warning("请先完成 ROI 编辑并确认");
    return;
  }
  stage.value = 3;
  stage3Completed.value = false; // 重置阶段3完成标志
}

async function backToStage1() {
  if (!currentFolderPath.value) return;
  // 重置所有阶段完成标志
  stage1Completed.value = false;
  stage2Completed.value = false;
  stage3Completed.value = false;
  await loadStage1Apr(currentFolderPath.value);
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
    // New series or explicit reload: reset workflow to Stage 1.
    selectedRoiMaskId.value = null;
    boneMetrics.value = null;
    stage.value = 1;
    sessionKind.value = "apr";
    // 重置所有阶段完成标志
    stage1Completed.value = false;
    stage2Completed.value = false;
    stage3Completed.value = false;
    await loadStage1Apr(next.folderPath);
  },
  { immediate: true }
);

watch([layoutChoice], async () => {
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
  // Destroy all sessions (APR-based)
  await destroySession("apr");
});

async function refreshLayout() {
  try {
    await nextTick();
    await updateNativeWindowLayouts();
  } catch {
    // ignore
  }
}

function drawHistogram() {
  if (!histogramCanvas.value || histogramData.value.length === 0) return;
  const canvas = histogramCanvas.value;
  const ctx = canvas.getContext("2d");
  if (!ctx) return;

  const width = canvas.width;
  const height = canvas.height;
  const padding = 20;
  const chartWidth = width - padding * 2;
  const chartHeight = height - padding * 2;

  ctx.clearRect(0, 0, width, height);
  ctx.fillStyle = "#0b2238";
  ctx.fillRect(0, 0, width, height);

  const logData = histogramData.value.map((val) =>
    val > 0 ? Math.log10(val + 1) : 0
  );
  const maxLog = Math.max(...logData);
  if (maxLog <= 0) return;

  const barWidth = chartWidth / histogramData.value.length;
  const minT = thresholdMin.value;
  const maxT = thresholdMax.value;
  const minV = histogramMinValue.value;
  const maxV = histogramMaxValue.value;
  const span = Math.max(1, maxV - minV);

  for (let i = 0; i < histogramData.value.length; i++) {
    const normalizedHeight = (logData[i] / maxLog) * chartHeight;
    const x = padding + i * barWidth;
    const y = padding + chartHeight - normalizedHeight;

    const ctValue = minV + (i / histogramData.value.length) * span;
    const inRange = ctValue >= minT && ctValue <= maxT;

    ctx.fillStyle = inRange
      ? "rgba(255,255,255,0.85)"
      : "rgba(160,190,210,0.35)";
    ctx.fillRect(x, y, barWidth, normalizedHeight);
  }

  ctx.strokeStyle = "rgba(220,245,255,0.55)";
  ctx.lineWidth = 1;
  ctx.beginPath();
  ctx.moveTo(padding, padding);
  ctx.lineTo(padding, padding + chartHeight);
  ctx.lineTo(padding + chartWidth, padding + chartHeight);
  ctx.stroke();
}

async function refreshHistogram() {
  if (!sessionId.value) return;
  try {
    const hist = await window.visualizationApi.getVolumeHistogram(
      sessionId.value
    );
    if (!hist?.data || !Array.isArray(hist.data) || hist.data.length === 0)
      return;

    histogramData.value = hist.data;
    histogramMinValue.value = Number(hist.minValue ?? -1024);
    histogramMaxValue.value = Number(hist.maxValue ?? 3071);

    const minValue = histogramMinValue.value;
    const maxValue = histogramMaxValue.value;
    const span = Math.max(1, maxValue - minValue);

    // Heuristic defaults: keep a sane minimum for bone-like HU.
    thresholdMin.value = Math.max(150, Math.round(minValue + span * 0.6));
    thresholdMax.value = Math.round(maxValue);

    await nextTick();
    drawHistogram();
  } catch (e) {
    console.warn("[骨骼分析] 获取直方图失败:", e);
  }
}

let previewTimer: number | null = null;
function schedulePreviewUpdate() {
  if (!isLoaded.value || !sessionId.value) return;
  if (previewTimer) window.clearTimeout(previewTimer);
  previewTimer = window.setTimeout(() => {
    void updatePreviewMask();
  }, 120);
}

async function updatePreviewMask() {
  if (!sessionId.value) return;
  try {
    await window.visualizationApi.updatePreviewMask(
      sessionId.value,
      thresholdMin.value,
      thresholdMax.value,
      previewColor.value
    );
  } catch (e) {
    console.warn("[骨骼分析] 预览mask失败:", e);
  } finally {
    drawHistogram();
  }
}

async function clearPreviewMask() {
  if (!sessionId.value) return;
  try {
    await window.visualizationApi.clearPreviewMask(sessionId.value);
  } catch (e) {
    console.warn("[骨骼分析] 清除预览失败:", e);
  }
}

async function applyThresholdMask() {
  if (!isLoaded.value || !sessionId.value) {
    ElMessage.warning("请先加载序列");
    return;
  }
  try {
    const name = maskName.value.trim() || "Bone";
    const res = await window.visualizationApi.createMaskFromThreshold(
      sessionId.value,
      thresholdMin.value,
      thresholdMax.value,
      previewColor.value,
      name
    );
    if (!res?.success) throw new Error(res?.error || "创建mask失败");

    masks.value.push({
      maskId: res.maskId,
      name,
      color: previewColor.value,
      visible: true,
      min: thresholdMin.value,
      max: thresholdMax.value,
    });
    await clearPreviewMask();
    ElMessage.success(`已生成掩膜: ${name}`);
  } catch (e: any) {
    console.error("[骨骼分析] applyThresholdMask failed:", e);
    ElMessage.error(`生成掩膜失败: ${e?.message ?? String(e)}`);
  }
}

async function deleteSelectedMask() {
  if (!selectedMask.value) {
    ElMessage.warning("请先选择一个掩膜");
    return;
  }
  if (!sessionId.value) return;

  const idx = masks.value.indexOf(selectedMask.value);
  const maskId = selectedMask.value.maskId;
  try {
    if (maskId !== undefined) {
      await window.visualizationApi.deleteMask(sessionId.value, maskId);
    }
    if (idx >= 0) masks.value.splice(idx, 1);
    selectedMask.value = null;
    if (maskId != null && selectedRoiMaskId.value === maskId) {
      selectedRoiMaskId.value = null;
    }
    boneMetrics.value = null;
    ElMessage.success("掩膜已删除");
  } catch (e) {
    console.error("[骨骼分析] deleteSelectedMask failed:", e);
    ElMessage.error("删除掩膜失败");
  }
}

async function saveAllMasks() {
  if (!sessionId.value || !currentFolderPath.value) {
    ElMessage.error("未加载图像数据");
    return;
  }
  const name = saveName.value.trim();
  if (!name) {
    ElMessage.warning("请输入保存名称");
    return;
  }
  try {
    const result = await window.visualizationApi.saveMasks(
      sessionId.value,
      currentFolderPath.value,
      name
    );
    if (!result?.success) throw new Error(result?.error || "保存失败");
    ElMessage.success(`Mask已保存: ${name}`);
  } catch (e: any) {
    console.error("[骨骼分析] saveAllMasks failed:", e);
    ElMessage.error(`保存失败: ${e?.message ?? String(e)}`);
  }
}

async function loadMasksFromDisk() {
  if (!sessionId.value || !currentFolderPath.value) {
    ElMessage.error("未加载图像数据");
    return;
  }
  try {
    const result = await window.visualizationApi.loadMasks(
      sessionId.value,
      currentFolderPath.value
    );
    if (result?.cancelled) return;
    if (!result?.success) throw new Error(result?.error || "加载失败");
    const loaded = Array.isArray(result.masks) ? result.masks : [];
    const existing = new Set(masks.value.map((m) => m.maskId));
    for (const m of loaded) {
      if (m?.maskId != null && existing.has(m.maskId)) continue;
      masks.value.push({
        maskId: m.maskId,
        name: m.name || `Mask_${masks.value.length + 1}`,
        color: m.color || "#ff0000",
        visible: m.visible ?? true,
        min: m.minThreshold ?? thresholdMin.value,
        max: m.maxThreshold ?? thresholdMax.value,
      });
    }
    ElMessage.success(`已加载 ${loaded.length} 个mask`);
  } catch (e: any) {
    console.error("[骨骼分析] loadMasksFromDisk failed:", e);
    ElMessage.error(`加载失败: ${e?.message ?? String(e)}`);
  }
}

function jumpTo3DRecon() {
  emit("switch-tab", "recon3d", props.panelData);
}

watch([thresholdMin, thresholdMax, previewColor], () => {
  schedulePreviewUpdate();
});
</script>

<template>
  <div class="viewer-page">
    <div class="left-panel">
      <ElCard class="control-card">
        <div class="section">
          <div class="section-title">骨密度分析阶段</div>
          <div style="margin-bottom: 6px; opacity: 0.85; font-size: 12px">
            当前：阶段 {{ stage }}（{{
              stage === 1
                ? "APR + 裁切"
                : stage === 2
                ? "MPR + ROI"
                : "MPR + 二值化 + 计算"
            }}）
          </div>
          <!-- 只有在非阶段1时才显示返回按钮 -->
          <div class="btn-row" v-if="stage !== 1" style="margin-bottom: 6px">
            <ElButton size="small" @click="backToStage1">返回阶段 1</ElButton>
          </div>
        </div>

        <div class="section">
          <div class="section-title">窗口切换</div>
          <ElRadioGroup v-model="layoutChoice" size="small">
            <ElRadioButton label="quad">全部</ElRadioButton>
            <ElRadioButton label="axial">轴位</ElRadioButton>
            <ElRadioButton label="coronal">冠状位</ElRadioButton>
            <ElRadioButton label="sagittal">矢状位</ElRadioButton>
            <ElRadioButton label="view3d">3D</ElRadioButton>
          </ElRadioGroup>
        </div>

        <div class="section" v-if="stage === 1">
          <div class="section-title">Step1：APR裁切范围</div>
          <div class="btn-row" style="margin-bottom: 6px">
            <ElButton
              size="small"
              :disabled="!isLoaded"
              :loading="aprCropRangeLoading"
              @click="refreshAprCropRange"
              >读取当前</ElButton
            >
            <ElButton
              type="primary"
              size="small"
              :disabled="!isLoaded"
              @click="applyAprCropRange"
              >应用</ElButton
            >
            <ElButton
              size="small"
              :disabled="!isLoaded"
              @click="resetAprCropRange"
              >重置</ElButton
            >
          </div>
          <div class="btn-row">
            <span style="min-width: 54px">X</span>
            <ElInputNumber v-model="aprCropRange.xStart" size="small" />
            <ElInputNumber v-model="aprCropRange.xEnd" size="small" />
          </div>
          <div class="btn-row">
            <span style="min-width: 54px">Y</span>
            <ElInputNumber v-model="aprCropRange.yStart" size="small" />
            <ElInputNumber v-model="aprCropRange.yEnd" size="small" />
          </div>
          <div class="btn-row">
            <span style="min-width: 54px">Z</span>
            <ElInputNumber v-model="aprCropRange.zStart" size="small" />
            <ElInputNumber v-model="aprCropRange.zEnd" size="small" />
          </div>
          <!-- 阶段 1 完成后显示进入下一阶段按钮 -->
          <div v-if="stage1Completed" class="btn-row" style="margin-top: 10px">
            <ElButton
              type="success"
              size="small"
              :disabled="!isLoaded"
              @click="goToStage2"
              >✓ 进入阶段 2（MPR + ROI）</ElButton
            >
          </div>
          <div v-else style="margin-top: 8px; opacity: 0.65; font-size: 12px">
            请设置裁切范围并点击"应用"后进入下一阶段
          </div>
        </div>

        <div class="section" v-if="stage === 2">
          <div class="section-title">Step2：MPR + ROI（交集输入）</div>
          <div class="btn-row" style="margin-bottom: 6px">
            <ElButton size="small" :disabled="!isLoaded" @click="openRoiEdit"
              >ROI 编辑</ElButton
            >
            <ElButton
              size="small"
              :disabled="!isLoaded || selectedRoiMaskId == null"
              @click="confirmRoi"
              >确认 ROI</ElButton
            >
          </div>
          <div class="btn-row" style="margin-bottom: 6px">
            <ElButton
              size="small"
              type="primary"
              :disabled="!isLoaded"
              @click="openSaveMaskDialog"
              >使用当前图像保存</ElButton
            >
            <ElButton
              size="small"
              type="primary"
              :disabled="!props.panelData"
              @click="jumpTo3DRecon"
              >去 3D 重建查看</ElButton
            >
          </div>
          <!-- 阶段 2 完成后显示进入下一阶段按钮 -->
          <div v-if="stage2Completed" class="btn-row" style="margin-top: 10px">
            <ElButton
              type="success"
              size="small"
              :disabled="!isLoaded"
              @click="goToStage3"
              >✓ 进入阶段 3（二值化 + 计算）</ElButton
            >
          </div>
          <div v-else style="margin-top: 8px; opacity: 0.65; font-size: 12px">
            请先编辑 ROI 并点击"确认 ROI"后进入下一阶段
          </div>
        </div>

        <div class="section" v-if="stage === 3">
          <div class="section-title">Step3：MPR + 二值化 + 计算</div>
          <div class="btn-row" style="margin-bottom: 6px">
            <ElButton
              size="small"
              type="primary"
              :disabled="!isLoaded"
              @click="openSaveMaskDialog"
              >使用当前图像保存</ElButton
            >
            <ElButton
              size="small"
              type="primary"
              :disabled="!props.panelData"
              @click="jumpTo3DRecon"
              >去 3D 重建查看</ElButton
            >
          </div>
        </div>

        <div class="section" v-if="stage === 3">
          <div class="section-title">阈值分割（骨）</div>
          <div class="slider-row">
            <div class="btn-row">
              <ElButton
                size="small"
                :disabled="!isLoaded"
                @click="refreshHistogram"
                >自动阈值</ElButton
              >
              <ElButton
                size="small"
                :disabled="!isLoaded"
                @click="updatePreviewMask"
                >预览</ElButton
              >
              <ElButton
                size="small"
                :disabled="!isLoaded"
                @click="clearPreviewMask"
                >清除预览</ElButton
              >
            </div>
            <div class="btn-row">
              <span style="min-width: 54px">Min</span>
              <ElInputNumber
                v-model="thresholdMin"
                :min="histogramMinValue"
                :max="thresholdMax"
                size="small"
              />
              <span style="min-width: 54px">Max</span>
              <ElInputNumber
                v-model="thresholdMax"
                :min="thresholdMin"
                :max="histogramMaxValue"
                size="small"
              />
            </div>
            <ElSlider
              v-model="thresholdMin"
              :min="histogramMinValue"
              :max="histogramMaxValue"
              :step="1"
            />
            <ElSlider
              v-model="thresholdMax"
              :min="histogramMinValue"
              :max="histogramMaxValue"
              :step="1"
            />
            <div class="btn-row">
              <span style="min-width: 54px">颜色</span>
              <ElInput v-model="previewColor" size="small" />
            </div>
            <div class="btn-row">
              <span style="min-width: 54px">名称</span>
              <ElInput v-model="maskName" size="small" />
            </div>
            <div class="btn-row">
              <ElButton
                type="primary"
                size="small"
                :disabled="!isLoaded"
                @click="applyThresholdMask"
                >生成Mask</ElButton
              >
              <ElButton
                size="small"
                :disabled="!isLoaded || masks.length === 0"
                @click="confirmBinarization"
                >确认二值化</ElButton
              >
              <ElButton
                size="small"
                :disabled="!isLoaded"
                @click="refreshLayout"
                >刷新布局</ElButton
              >
            </div>
            <!-- 阶段 3 完成后提示可进行计算 -->
            <div
              v-if="stage3Completed"
              style="margin-top: 8px; color: #67c23a; font-size: 12px"
            >
              ✓ 二值化已确认，可进行骨指标计算
            </div>
            <div v-else style="margin-top: 8px; opacity: 0.65; font-size: 12px">
              请生成 Mask 并"确认二值化"后进行指标计算
            </div>
            <canvas
              ref="histogramCanvas"
              width="280"
              height="110"
              style="
                border-radius: 10px;
                border: 1px solid rgba(11, 205, 212, 0.25);
              "
            />
          </div>
        </div>

        <div class="section" v-if="stage >= 2">
          <div class="section-title">掩膜管理</div>
          <ElTable
            :data="masks"
            size="small"
            border
            class="roi-table"
            highlight-current-row
            @current-change="(row) => (selectedMask = row)"
            tabindex="0"
          >
            <ElTableColumn prop="name" label="名称" />
            <ElTableColumn prop="color" label="颜色" width="90">
              <template #default="scope">
                <span
                  :style="{
                    display: 'inline-block',
                    width: '18px',
                    height: '18px',
                    background: scope.row.color,
                    borderRadius: '4px',
                    border: '1px solid #aaa',
                  }"
                ></span>
              </template>
            </ElTableColumn>
            <ElTableColumn prop="min" label="Min" width="72" />
            <ElTableColumn prop="max" label="Max" width="72" />
            <ElTableColumn prop="visible" label="显示" width="70">
              <template #default="scope">
                <el-switch v-model="scope.row.visible" size="small" />
              </template>
            </ElTableColumn>
          </ElTable>
          <div class="btn-row" style="margin-top: 10px">
            <ElButton
              size="small"
              :disabled="!isLoaded"
              @click="deleteSelectedMask"
              >删除</ElButton
            >
            <ElButton
              size="small"
              :disabled="!isLoaded"
              @click="loadMasksFromDisk"
              >加载</ElButton
            >
          </div>
          <div class="btn-row" style="margin-top: 10px">
            <span style="min-width: 54px">保存名</span>
            <ElInput v-model="saveName" size="small" />
            <ElButton size="small" :disabled="!isLoaded" @click="saveAllMasks"
              >保存</ElButton
            >
          </div>
        </div>

        <div class="section" v-if="stage === 3">
          <div class="section-title">骨指标</div>
          <div class="btn-row" style="margin-bottom: 6px">
            <span style="min-width: 54px">ROI</span>
            <ElSelect
              v-model="selectedRoiMaskId"
              size="small"
              clearable
              placeholder="(可选) 选择ROI掩膜"
              style="min-width: 180px"
            >
              <ElOption
                v-for="m in masks"
                :key="m.maskId ?? m.name"
                :label="m.name"
                :value="m.maskId"
                :disabled="m.maskId == null"
              />
            </ElSelect>
            <ElButton size="small" :disabled="!isLoaded" @click="openRoiEdit"
              >编辑ROI</ElButton
            >
          </div>
          <div class="btn-row">
            <ElButton
              type="primary"
              size="small"
              :disabled="
                !isLoaded || !selectedMask || selectedMask.maskId == null
              "
              :loading="boneMetricsLoading"
              @click="calculateSelectedMetrics"
              >计算指标</ElButton
            >
          </div>
          <ElTable
            v-if="boneMetrics"
            :data="boneMetricsRows"
            size="small"
            border
            class="roi-table"
            style="margin-top: 10px"
          >
            <ElTableColumn prop="name" label="指标" width="130" />
            <ElTableColumn prop="value" label="值" />
          </ElTable>
          <div v-else style="margin-top: 8px; opacity: 0.75">
            选择一个掩膜，然后点击“计算指标”。
          </div>
        </div>
      </ElCard>
    </div>

    <div class="right-panel">
      <div v-if="layoutChoice === 'quad'" class="grid2x2">
        <ElCard class="view-cell">
          <div ref="viewAxialRef" class="view-container">
            <div class="view-placeholder" style="left: 10px; top: 10px">
              轴位视图
            </div>
            <div
              class="view-placeholder"
              style="
                inset: 0;
                display: flex;
                align-items: center;
                justify-content: center;
                opacity: 0.55;
              "
            >
              轴位视图
            </div>
          </div>
        </ElCard>
        <ElCard class="view-cell">
          <div ref="viewCoronalRef" class="view-container">
            <div class="view-placeholder" style="left: 10px; top: 10px">
              冠状位视图
            </div>
            <div
              class="view-placeholder"
              style="
                inset: 0;
                display: flex;
                align-items: center;
                justify-content: center;
                opacity: 0.55;
              "
            >
              冠状位视图
            </div>
          </div>
        </ElCard>
        <ElCard class="view-cell">
          <div ref="viewSagittalRef" class="view-container">
            <div class="view-placeholder" style="left: 10px; top: 10px">
              矢状位视图
            </div>
            <div
              class="view-placeholder"
              style="
                inset: 0;
                display: flex;
                align-items: center;
                justify-content: center;
                opacity: 0.55;
              "
            >
              矢状位视图
            </div>
          </div>
        </ElCard>
        <ElCard class="view-cell">
          <div
            class="view-container"
            style="
              display: flex;
              align-items: center;
              justify-content: center;
              flex-direction: column;
              gap: 12px;
            "
          >
            <div
              class="view-placeholder"
              style="position: static; opacity: 0.9"
            >
              3D（跳转）
            </div>
            <ElButton type="primary" size="small" @click="jumpTo3DRecon"
              >打开 3维重建</ElButton
            >
          </div>
        </ElCard>
      </div>

      <div v-else class="grid1x1">
        <ElCard class="view-cell" v-if="layoutChoice === 'axial'">
          <div ref="viewAxialRef" class="view-container">
            <div class="view-placeholder" style="left: 10px; top: 10px">
              轴位视图
            </div>
            <div
              class="view-placeholder"
              style="
                inset: 0;
                display: flex;
                align-items: center;
                justify-content: center;
                opacity: 0.55;
              "
            >
              轴位视图
            </div>
          </div>
        </ElCard>

        <ElCard class="view-cell" v-else-if="layoutChoice === 'coronal'">
          <div ref="viewCoronalRef" class="view-container">
            <div class="view-placeholder" style="left: 10px; top: 10px">
              冠状位视图
            </div>
            <div
              class="view-placeholder"
              style="
                inset: 0;
                display: flex;
                align-items: center;
                justify-content: center;
                opacity: 0.55;
              "
            >
              冠状位视图
            </div>
          </div>
        </ElCard>

        <ElCard class="view-cell" v-else-if="layoutChoice === 'sagittal'">
          <div ref="viewSagittalRef" class="view-container">
            <div class="view-placeholder" style="left: 10px; top: 10px">
              矢状位视图
            </div>
            <div
              class="view-placeholder"
              style="
                inset: 0;
                display: flex;
                align-items: center;
                justify-content: center;
                opacity: 0.55;
              "
            >
              矢状位视图
            </div>
          </div>
        </ElCard>

        <ElCard class="view-cell" v-else>
          <div
            class="view-container"
            style="
              display: flex;
              align-items: center;
              justify-content: center;
              flex-direction: column;
              gap: 12px;
            "
          >
            <div
              class="view-placeholder"
              style="position: static; opacity: 0.9"
            >
              3D（跳转）
            </div>
            <ElButton type="primary" size="small" @click="jumpTo3DRecon"
              >打开 3维重建</ElButton
            >
          </div>
        </ElCard>
      </div>
    </div>
  </div>
</template>
