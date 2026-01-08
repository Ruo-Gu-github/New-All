<script setup lang="ts">
import "element-plus/dist/index.css";
import { ref, shallowRef, watch, onMounted, onUnmounted, nextTick } from "vue";
import {
  ElButton,
  ElCard,
  ElMessage,
  ElMessageBox,
  ElTooltip,
  ElSlider,
  ElRadioButton,
  ElRadioGroup,
  ElSelect,
  ElOption,
  ElInputNumber,
  ElSwitch,
  ElCheckbox,
  ElCheckboxGroup,
} from "element-plus";
import {
  Crop,
  Refresh,
  Plus,
  EditPen,
  Connection,
  View,
  Picture,
  Scissor,
  Setting,
} from "@element-plus/icons-vue";

// 接收 panel 数据（包含路径）
const props = defineProps<{
  panelData?: {
    id: string;
    folderPath: string;
    patientName?: string;
    imageCount: number;
    width: number;
    height: number;
    windowWidth?: number;
    windowLevel?: number;
  } | null;
  // Increment to force reload even when folderPath is unchanged.
  reloadKey?: number;
}>();

// 定义事件发射器（用于切换标签页）
const emit = defineEmits<{
  (e: "switch-tab", tabName: string, panelData?: any): void;
}>();

// UI状态
// 图像浏览：是否强制旋转归零（正交切片模式）。默认关闭，避免旋转“回弹”。
const fixedZeroRotation = ref(false);
const aprProgress = ref([50, 50, 50]); // 三面进度 (0-100)
const aprRotate = ref([0, 0, 0]); // xyz旋转
const mipValue = ref(0);
const minipValue = ref(0);

// Window/level (WW/WL)
const FALLBACK_WINDOW_WIDTH = 4096;
const FALLBACK_WINDOW_LEVEL = 2048;
const windowWidth = ref<number>(
  props.panelData?.windowWidth ?? FALLBACK_WINDOW_WIDTH
);
const windowLevel = ref<number>(
  props.panelData?.windowLevel ?? FALLBACK_WINDOW_LEVEL
);

// WW/WL inline panel (no dialog)
const showWindowLevelPanel = ref(false);
const windowWidthInput = ref<string>(String(windowWidth.value));
const windowLevelInput = ref<string>(String(windowLevel.value));

// Screenshot inline panel (no dialog)
const showScreenshotPanel = ref(false);
const screenshotSelection = ref<string[]>([
  "axial",
  "coronal",
  "sagittal",
  "3d",
]);

// Screenshot status display
const screenshotStatus = ref<{
  show: boolean;
  type: "success" | "error" | "info";
  message: string;
  details?: string[];
}>({ show: false, type: "info", message: "" });

// Auto-hide screenshot status after 10 seconds for success, 15 seconds for errors
let screenshotStatusTimer: NodeJS.Timeout | null = null;
function showScreenshotStatus(status: typeof screenshotStatus.value) {
  if (screenshotStatusTimer) {
    clearTimeout(screenshotStatusTimer);
    screenshotStatusTimer = null;
  }

  screenshotStatus.value = status;

  if (status.show) {
    const hideDelay = status.type === "success" ? 10000 : 15000;
    screenshotStatusTimer = setTimeout(() => {
      screenshotStatus.value.show = false;
      screenshotStatusTimer = null;
    }, hideDelay);
  }
}

// 工具状态
const showCrosshair = ref(true); // 是否显示定位线
const showCropBox = ref(false); // 是否显示裁切框
const currentToolType = ref(0); // 当前工具类型：0=定位线, 1=Line, 2=Angle, 3=Rect, 4=Circle, 5=Bezier

// 裁切形状设置
// 0=立方体, 1=球, 2=轴向圆柱, 3=冠状圆柱, 4=矢状圆柱
const cropShape = ref(0);
const cropUnitMode = ref<"pixel" | "mm">("pixel"); // 单位模式：像素或物理长度(mm)
// 立方体尺寸（像素）
const cropBoxSizeX = ref(100);
const cropBoxSizeY = ref(100);
const cropBoxSizeZ = ref(100);
// 球半径（像素）
const cropSphereRadius = ref(50);
// 圆柱参数（像素）
const cropCylinderRadius = ref(50);
const cropCylinderHeight = ref(100);
// 体数据spacing（物理尺寸用于换算）
const volumeSpacing = ref({ x: 1, y: 1, z: 1 });

// APR 视图引用（4个div容器）
const view1Ref = ref<HTMLDivElement | null>(null);
const view2Ref = ref<HTMLDivElement | null>(null);
const view3Ref = ref<HTMLDivElement | null>(null);
const view4Ref = ref<HTMLDivElement | null>(null);

type ViewerLayoutChoice = "quad" | "view1" | "view2" | "view3" | "view4";
const layoutChoice = ref<ViewerLayoutChoice>("quad");

// APR Session ID（用于标识此次会话）
const sessionId = ref<string>("");
const isLoaded = ref(false);

// Native child HWNDs for each view (used for HWND-based screenshots)
// IMPORTANT: must be a plain cloneable object for Electron IPC (no Vue Proxy)
const viewHwnds = shallowRef<Record<string, string>>({});

// ==================== 双向同步（Native -> Electron）====================
let aprStatePollTimer: number | null = null;
let suppressAprUiWatch = false;
let suppressCropUiWatch = false; // 裁切框参数同步抑制标志
let lastUiCenterUpdateAt = 0;
let lastUiRotationUpdateAt = 0;
let lastCropUiUpdateAt = 0; // 裁切框UI更新时间戳

// 加载状态标志
let isLoading = false;

// 体数据尺寸和中心点（从 DLL 获取）
const volumeWidth = ref(512);
const volumeHeight = ref(512);
const volumeDepth = ref(512);
const volumeCenter = ref({ x: 256, y: 256, z: 256 });

// 监听 panelData 变化，加载图像
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
    if (next.folderPath && changed) {
      await loadAPRViews(next.folderPath);
    }
  },
  { immediate: true }
);

// 加载 APR 视图（调用 Visualization.dll）
async function loadAPRViews(folderPath: string) {
  // 防止重复加载
  if (isLoading) return;

  isLoading = true;

  try {
    console.log("[图像浏览] 加载 APR 视图:", folderPath);

    // 停止旧的渲染循环
    await stopRenderLoop();

    // 清理旧会话
    if (sessionId.value) {
      await window.visualizationApi.destroyAPR(sessionId.value);
      sessionId.value = "";
      isLoaded.value = false;
    }

    // 创建新会话 ID
    sessionId.value = `apr_${Date.now()}_${Math.random()
      .toString(36)
      .substr(2, 9)}`;

    // 创建 APR 视图（4个窗口：axial, coronal, sagittal, 3d）
    const result = await window.visualizationApi.createAPR(
      sessionId.value,
      folderPath
    );

    if (!result.success) {
      throw new Error(result.error || "创建 APR 视图失败");
    }

    // 保存体数据尺寸和中心点
    if (result.width && result.height && result.depth) {
      volumeWidth.value = result.width;
      volumeHeight.value = result.height;
      volumeDepth.value = result.depth;

      if (
        result.centerX !== undefined &&
        result.centerY !== undefined &&
        result.centerZ !== undefined
      ) {
        volumeCenter.value = {
          x: result.centerX,
          y: result.centerY,
          z: result.centerZ,
        };

        // 初始化进度条为中心位置（50%）
        aprProgress.value = [50, 50, 50];
      }
    }

    // 嵌入原生窗口到 div 中（Win32 child HWND，宿主消息循环驱动）
    if (
      (result.windowIdAxial || result.hwndAxial) &&
      (result.windowIdCoronal || result.hwndCoronal) &&
      (result.windowIdSagittal || result.hwndSagittal) &&
      (result.windowId3D || result.hwnd3D)
    ) {
      // Store HWNDs (string) for screenshot capture by window handle.
      viewHwnds.value = {
        axial: result.hwndAxial ? String(result.hwndAxial) : "",
        coronal: result.hwndCoronal ? String(result.hwndCoronal) : "",
        sagittal: result.hwndSagittal ? String(result.hwndSagittal) : "",
        "3d": result.hwnd3D ? String(result.hwnd3D) : "",
        // Also keep windowId keys as a fallback for downstream lookup.
        ...(result.windowIdAxial && result.hwndAxial
          ? { [String(result.windowIdAxial)]: String(result.hwndAxial) }
          : {}),
        ...(result.windowIdCoronal && result.hwndCoronal
          ? { [String(result.windowIdCoronal)]: String(result.hwndCoronal) }
          : {}),
        ...(result.windowIdSagittal && result.hwndSagittal
          ? { [String(result.windowIdSagittal)]: String(result.hwndSagittal) }
          : {}),
        ...(result.windowId3D && result.hwnd3D
          ? { [String(result.windowId3D)]: String(result.hwnd3D) }
          : {}),
      };

      // 嵌入阶段必须保证 4 个视图容器都在 DOM 中
      layoutChoice.value = "quad";

      // 等待 DOM 更新
      await nextTick();
      await new Promise((resolve) => setTimeout(resolve, 200));

      if (
        !view1Ref.value ||
        !view2Ref.value ||
        !view3Ref.value ||
        !view4Ref.value
      ) {
        throw new Error("View containers not ready");
      }

      await embedWindow(
        view1Ref.value,
        result.windowIdAxial ?? String(result.hwndAxial)
      );
      await embedWindow(
        view2Ref.value,
        result.windowIdCoronal ?? String(result.hwndCoronal)
      );
      await embedWindow(
        view3Ref.value,
        result.windowIdSagittal ?? String(result.hwndSagittal)
      );
      await embedWindow(
        view4Ref.value,
        result.windowId3D ?? String(result.hwnd3D)
      );
    }

    isLoaded.value = true;
    console.log("[图像浏览] APR 视图创建成功");

    // 初始化裁切框（使用体数据尺寸）
    await window.visualizationApi.setAPRCropBox(
      sessionId.value,
      result.width || volumeWidth.value,
      result.height || volumeHeight.value,
      result.depth || volumeDepth.value
    );

    // 获取体数据spacing并初始化裁切框尺寸
    try {
      const spacingResult = await window.visualizationApi.getVolumeSpacing(
        sessionId.value
      );
      if (spacingResult) {
        volumeSpacing.value = {
          x: spacingResult.spacingX || spacingResult.x || 1,
          y: spacingResult.spacingY || spacingResult.y || 1,
          z: spacingResult.spacingZ || spacingResult.z || 1,
        };
        console.log("[图像浏览] Volume spacing:", volumeSpacing.value);
      }
      // 初始化裁切框尺寸为体数据尺寸的一半
      const initSize =
        Math.min(volumeWidth.value, volumeHeight.value, volumeDepth.value) / 2;
      cropBoxSizeX.value = Math.round(initSize);
      cropBoxSizeY.value = Math.round(initSize);
      cropBoxSizeZ.value = Math.round(initSize);
      cropSphereRadius.value = Math.round(initSize / 2);
      cropCylinderRadius.value = Math.round(initSize / 2);
      cropCylinderHeight.value = Math.round(initSize);
    } catch (error) {
      console.warn("[图像浏览] 获取spacing失败:", error);
    }

    // 打开图像时优先使用 DICOM 默认窗宽窗位
    try {
      const ww = props.panelData?.windowWidth;
      const wl = props.panelData?.windowLevel;
      if (typeof ww === "number" && typeof wl === "number" && ww > 0) {
        windowWidth.value = ww;
        windowLevel.value = wl;
        await window.visualizationApi.setWindowLevel(sessionId.value, ww, wl);
      } else {
        const st = await window.visualizationApi.getWindowLevel(
          sessionId.value
        );
        const nativeWw = st?.success ? st.windowWidth : undefined;
        const nativeWl = st?.success ? st.windowLevel : undefined;
        if (
          typeof nativeWw === "number" &&
          nativeWw > 0 &&
          typeof nativeWl === "number"
        ) {
          windowWidth.value = nativeWw;
          windowLevel.value = nativeWl;
        } else {
          windowWidth.value = FALLBACK_WINDOW_WIDTH;
          windowLevel.value = FALLBACK_WINDOW_LEVEL;
          await window.visualizationApi.setWindowLevel(
            sessionId.value,
            FALLBACK_WINDOW_WIDTH,
            FALLBACK_WINDOW_LEVEL
          );
        }
      }
    } catch (error) {
      console.warn("[图像浏览] 应用窗宽窗位失败:", error);
    }

    // “显示正交切片”：切换 3D 窗口渲染模式（正交三平面 vs raycast）
    try {
      await window.visualizationApi.set3DOrthogonalMode(
        `${sessionId.value}_3d`,
        fixedZeroRotation.value
      );
    } catch (error) {
      console.error("[图像浏览] 设置3D正交模式失败:", error);
    }

    // 启动固定帧率渲染循环（60fps）以获得流畅的交互体验
    await window.visualizationApi.startRenderLoop(60);

    // 嵌入完成后再同步一次原生窗口布局
    await updateNativeWindowLayouts();

    // App Tab 切换可能会 hide/disable 原生 HWND（SW_HIDE + EnableWindow(FALSE)）。
    // 在 embed/layout 完成后显式 show/enable，并强制重绘，以确保窗宽窗位 / 切片 HUD 一直显示。
    try {
      await window.visualizationApi.showAllWindows?.();
      await window.visualizationApi.invalidateAllWindows?.();
    } catch (error) {
      console.warn("[图像浏览] showAllWindows/invalidate failed:", error);
    }
  } catch (error: any) {
    console.error("[图像浏览] 加载APR失败:", error);
  } finally {
    isLoading = false;
  }
}

function clampInt(v: number, min: number, max: number) {
  return Math.max(min, Math.min(max, Math.round(v)));
}

function sanitizeIntegerString(raw: string, allowNegative: boolean) {
  let s = String(raw ?? "");
  s = s.replace(/[^0-9\-]/g, "");
  if (allowNegative) {
    s = s.replace(/(?!^)-/g, "");
    s = s.replace(/^-{2,}/, "-");
  } else {
    s = s.replace(/-/g, "");
  }
  return s;
}

function toggleWindowLevelPanel() {
  showWindowLevelPanel.value = !showWindowLevelPanel.value;
  if (showWindowLevelPanel.value) {
    windowWidthInput.value = String(windowWidth.value);
    windowLevelInput.value = String(windowLevel.value);
  }
}

async function applyWindowLevelAll() {
  if (!sessionId.value) {
    ElMessage.warning("请先打开一个影像");
    return;
  }

  const ww = parseInt(windowWidthInput.value, 10);
  const wl = parseInt(windowLevelInput.value, 10);

  if (!Number.isFinite(ww) || ww <= 0) {
    ElMessage.warning("窗宽请输入正整数");
    return;
  }
  if (!Number.isFinite(wl)) {
    ElMessage.warning("窗位请输入整数");
    return;
  }

  windowWidth.value = ww;
  windowLevel.value = wl;

  try {
    await window.visualizationApi.setWindowLevel(sessionId.value, ww, wl);
    await window.visualizationApi.invalidateAllWindows?.();
  } catch (error) {
    console.warn("[图像浏览] 设置窗宽窗位失败:", error);
    ElMessage.error("设置窗宽窗位失败");
  }
}

function sliceIndexAxial() {
  const d = Math.max(1, volumeDepth.value);
  return clampInt((aprProgress.value[2] / 100) * (d - 1), 0, d - 1);
}

function sliceIndexCoronal() {
  const h = Math.max(1, volumeHeight.value);
  return clampInt((aprProgress.value[1] / 100) * (h - 1), 0, h - 1);
}

function sliceIndexSagittal() {
  const w = Math.max(1, volumeWidth.value);
  return clampInt((aprProgress.value[0] / 100) * (w - 1), 0, w - 1);
}

// ==================== 渲染循环管理 ====================
async function stopRenderLoop() {
  if (!window.visualizationApi?.stopRenderLoop) return;
  try {
    await window.visualizationApi.stopRenderLoop();
  } catch (error) {
    console.error("[图像浏览] 停止渲染循环失败:", error);
  }
}

// 统一计算相对于 Electron WebContents 客户区的坐标（CSS px / DIP）
function computeRelativeRect(element: HTMLDivElement | null) {
  if (!element) return null;
  const rect = element.getBoundingClientRect();
  // Embedded native child windows use physical pixels on Windows in many setups.
  // Convert from CSS pixels (DIP) to device pixels to avoid DPI-induced offset.
  const scale = Number.isFinite(window.devicePixelRatio)
    ? Math.max(1, window.devicePixelRatio)
    : 1;
  return {
    x: Math.round(rect.left * scale),
    y: Math.round(rect.top * scale),
    width: Math.max(1, Math.round(rect.width * scale)),
    height: Math.max(1, Math.round(rect.height * scale)),
  };
}

// ==================== 工具切换函数 ====================

// 显示定位线（切换到定位线工具，toolType=0）
async function toggleCrosshair() {
  showCrosshair.value = !showCrosshair.value;

  if (sessionId.value) {
    try {
      await window.visualizationApi.setCrosshairVisible(
        sessionId.value,
        showCrosshair.value
      );
    } catch (error) {
      console.error("[图像浏览] setCrosshairVisible失败:", error);
    }
  }

  if (showCrosshair.value) {
    // 关闭裁切框
    showCropBox.value = false;
    if (sessionId.value) {
      const ids = [
        `${sessionId.value}_axial`,
        `${sessionId.value}_coronal`,
        `${sessionId.value}_sagittal`,
        `${sessionId.value}_3d`,
      ];
      for (const id of ids) {
        await window.visualizationApi.setWindowCropBoxVisible(id, false);
      }
    }
    await window.visualizationApi.enableAPRCropBox(sessionId.value, false);

    // 切换到定位线工具
    currentToolType.value = 0;
    await setAllWindowsToolType(0);
  }
}

// 显示裁切框（启用裁切框拖拽）
async function toggleCropBox() {
  showCropBox.value = !showCropBox.value;

  if (showCropBox.value) {
    // 关闭定位线
    showCrosshair.value = false;
    if (sessionId.value) {
      try {
        await window.visualizationApi.setCrosshairVisible(
          sessionId.value,
          false
        );
      } catch (error) {
        console.error("[图像浏览] setCrosshairVisible失败:", error);
      }
    }

    // 启用裁切框
    await window.visualizationApi.enableAPRCropBox(sessionId.value, true);

    // 仅对当前 session 的 4 个窗口显示裁切框（避免串到 ROI 编辑等其他实例）
    if (sessionId.value) {
      const ids = [
        `${sessionId.value}_axial`,
        `${sessionId.value}_coronal`,
        `${sessionId.value}_sagittal`,
        `${sessionId.value}_3d`,
      ];
      for (const id of ids) {
        await window.visualizationApi.setWindowCropBoxVisible(id, true);
      }
    }

    // 切换到定位线工具（toolType=0，但裁切框启用时会优先处理）
    currentToolType.value = 0;
    await setAllWindowsToolType(0);
  } else {
    // 禁用裁切框
    if (sessionId.value) {
      const ids = [
        `${sessionId.value}_axial`,
        `${sessionId.value}_coronal`,
        `${sessionId.value}_sagittal`,
        `${sessionId.value}_3d`,
      ];
      for (const id of ids) {
        await window.visualizationApi.setWindowCropBoxVisible(id, false);
      }
    }
    await window.visualizationApi.enableAPRCropBox(sessionId.value, false);
  }
}

// 执行裁切操作
async function executeCrop() {
  console.log("[裁切] 开始执行裁切, sessionId:", sessionId.value);

  if (!sessionId.value) {
    ElMessage.warning("请先打开一个影像");
    return;
  }

  try {
    // Ensure native side has processed latest mouse-driven crop box edits.
    await window.visualizationApi.processWindowEvents?.();

    console.log("[裁切] 调用cropVolume API...");
    // 调用裁切API
    const result = await window.visualizationApi.cropVolume(sessionId.value);
    console.log("[裁切] cropVolume结果:", result);

    if (!result?.success) {
      ElMessage.error(result?.error || "裁切失败");
      return;
    }

    console.log(
      `[裁切] 裁切成功: ${result.width} x ${result.height} x ${result.depth}`
    );

    // 直接应用裁切结果（不弹对话框，避免与OpenGL窗口冲突）
    console.log("[裁切] 调用applyCroppedVolume...");
    const applyResult = await window.visualizationApi.applyCroppedVolume(
      sessionId.value
    );
    console.log("[裁切] applyCroppedVolume结果:", applyResult);

    if (applyResult?.success) {
      console.log("[裁切] 应用成功，开始隐藏裁切框...");
      ElMessage.success(
        `已应用裁切结果: ${applyResult.width} × ${applyResult.height} × ${applyResult.depth}`
      );
      // 隐藏裁切框
      showCropBox.value = false;
      if (sessionId.value) {
        const ids = [
          `${sessionId.value}_axial`,
          `${sessionId.value}_coronal`,
          `${sessionId.value}_sagittal`,
          `${sessionId.value}_3d`,
        ];
        for (const id of ids) {
          await window.visualizationApi.setWindowCropBoxVisible(id, false);
        }
      }
      await window.visualizationApi.enableAPRCropBox(sessionId.value, false);
      // 刷新视图
      await window.visualizationApi.invalidateAllWindows();

      console.log(
        "[裁切] 裁切完成，保留在当前视图（不跳转，避免重新加载原始数据）"
      );
      // 注意：不跳转到 ROI 模块，因为跳转会创建新 session 并重新加载原始 DICOM 数据
      // 裁切后的 g_activeVolume 已经在当前 3D 视图中生效
    } else {
      console.log("[裁切] 应用失败:", applyResult?.error);
      ElMessage.error(applyResult?.error || "应用裁切失败");
    }
  } catch (error: any) {
    console.error("[裁切] 执行失败:", error);
    ElMessage.error(error?.message || "裁切操作失败");
  }
}

// 打开裁切设置对话框
async function openCropSettingsDialog() {
  if (!sessionId.value) {
    ElMessage.warning("请先打开一个影像");
    return;
  }

  try {
    // 获取当前裁切设置
    const cropSettings = await window.visualizationApi.getCropSettings(
      sessionId.value
    );
    const volumeSpacing = await window.visualizationApi.getVolumeSpacing(
      sessionId.value
    );
    const aprState = await window.visualizationApi.getAPRState(
      sessionId.value,
      "axial"
    );

    // 打开对话框窗口
    if (window.nativeBridge?.dialog?.open) {
      await window.nativeBridge.dialog.open("cropsettings", {
        shape: cropSettings?.shape ?? 0,
        cylinderDirection: cropSettings?.cylinderDirection ?? 0,
        cropBox: cropSettings?.cropBox ?? {},
        spacing: volumeSpacing ?? { x: 1, y: 1, z: 1 },
        volumeDims: aprState
          ? {
              width: aprState.volumeWidth,
              height: aprState.volumeHeight,
              depth: aprState.volumeDepth,
            }
          : { width: 512, height: 512, depth: 256 },
      });
    } else {
      ElMessage.error("Dialog API 未初始化");
    }
  } catch (error: any) {
    console.error("[裁切设置] 打开对话框失败:", error);
    ElMessage.error(error?.message || "打开裁切设置失败");
  }
}

// 更新裁切框形状和尺寸（实时同步到Native侧）
async function updateCropSettings() {
  if (!sessionId.value || !showCropBox.value) return;

  // 记录UI更新时间戳，防止轮询覆盖
  lastCropUiUpdateAt = Date.now();

  try {
    // 根据形状类型设置shape和cylinderDirection
    let shape = 0; // box
    let cylinderDirection = 0;
    if (cropShape.value === 1) {
      shape = 1; // sphere
    } else if (cropShape.value >= 2 && cropShape.value <= 4) {
      shape = 2; // cylinder
      cylinderDirection = cropShape.value - 2; // 0=axial, 1=coronal, 2=sagittal
    }

    // 设置形状类型
    await window.visualizationApi.setCropShape(shape);
    if (shape === 2) {
      await window.visualizationApi.setCropCylinderDirection(cylinderDirection);
    }

    // 计算像素尺寸
    let sizeX = cropBoxSizeX.value;
    let sizeY = cropBoxSizeY.value;
    let sizeZ = cropBoxSizeZ.value;

    if (cropUnitMode.value === "mm") {
      // 从物理尺寸转换为像素尺寸
      sizeX = Math.round(cropBoxSizeX.value / volumeSpacing.value.x);
      sizeY = Math.round(cropBoxSizeY.value / volumeSpacing.value.y);
      sizeZ = Math.round(cropBoxSizeZ.value / volumeSpacing.value.z);
    }

    // 根据形状计算包围盒尺寸
    if (cropShape.value === 1) {
      // 球体：用球半径计算包围盒
      let radius = cropSphereRadius.value;
      if (cropUnitMode.value === "mm") {
        // 使用最小spacing来转换（保持球形）
        const minSpacing = Math.min(
          volumeSpacing.value.x,
          volumeSpacing.value.y,
          volumeSpacing.value.z
        );
        radius = Math.round(radius / minSpacing);
      }
      sizeX = sizeY = sizeZ = radius * 2;
    } else if (cropShape.value >= 2 && cropShape.value <= 4) {
      // 圆柱：用半径和高度计算包围盒
      let radius = cropCylinderRadius.value;
      let height = cropCylinderHeight.value;
      if (cropUnitMode.value === "mm") {
        const minSpacing = Math.min(
          volumeSpacing.value.x,
          volumeSpacing.value.y,
          volumeSpacing.value.z
        );
        radius = Math.round(radius / minSpacing);
        height = Math.round(height / minSpacing);
      }
      const diameter = radius * 2;
      if (cropShape.value === 2) {
        // 轴向圆柱（沿Z轴）
        sizeX = sizeY = diameter;
        sizeZ = height;
      } else if (cropShape.value === 3) {
        // 冠状圆柱（沿Y轴）
        sizeX = sizeZ = diameter;
        sizeY = height;
      } else {
        // 矢状圆柱（沿X轴）
        sizeY = sizeZ = diameter;
        sizeX = height;
      }
    }

    // 设置裁切框尺寸
    await window.visualizationApi.setCropBoxSize(
      sessionId.value,
      sizeX,
      sizeY,
      sizeZ,
      volumeWidth.value,
      volumeHeight.value,
      volumeDepth.value
    );

    // 刷新视图
    await window.visualizationApi.invalidateAllWindows();
  } catch (error: any) {
    console.error("[裁切设置] 更新失败:", error);
  }
}

// 监听裁切形状变化
watch(cropShape, () => {
  updateCropSettings();
});

// 监听裁切尺寸变化（立方体）
watch([cropBoxSizeX, cropBoxSizeY, cropBoxSizeZ], () => {
  if (suppressCropUiWatch) return;
  if (cropShape.value === 0) {
    updateCropSettings();
  }
});

// 监听球半径变化
watch(cropSphereRadius, () => {
  if (suppressCropUiWatch) return;
  if (cropShape.value === 1) {
    updateCropSettings();
  }
});

// 监听圆柱参数变化
watch([cropCylinderRadius, cropCylinderHeight], () => {
  if (suppressCropUiWatch) return;
  if (cropShape.value >= 2 && cropShape.value <= 4) {
    updateCropSettings();
  }
});

// 监听单位模式变化（重新计算尺寸）
watch(cropUnitMode, (newMode, oldMode) => {
  // 切换单位时转换当前值
  if (newMode === "mm" && oldMode === "pixel") {
    // 像素 -> 毫米
    cropBoxSizeX.value =
      Math.round(cropBoxSizeX.value * volumeSpacing.value.x * 10) / 10;
    cropBoxSizeY.value =
      Math.round(cropBoxSizeY.value * volumeSpacing.value.y * 10) / 10;
    cropBoxSizeZ.value =
      Math.round(cropBoxSizeZ.value * volumeSpacing.value.z * 10) / 10;
    const minSpacing = Math.min(
      volumeSpacing.value.x,
      volumeSpacing.value.y,
      volumeSpacing.value.z
    );
    cropSphereRadius.value =
      Math.round(cropSphereRadius.value * minSpacing * 10) / 10;
    cropCylinderRadius.value =
      Math.round(cropCylinderRadius.value * minSpacing * 10) / 10;
    cropCylinderHeight.value =
      Math.round(cropCylinderHeight.value * minSpacing * 10) / 10;
  } else if (newMode === "pixel" && oldMode === "mm") {
    // 毫米 -> 像素
    cropBoxSizeX.value = Math.round(cropBoxSizeX.value / volumeSpacing.value.x);
    cropBoxSizeY.value = Math.round(cropBoxSizeY.value / volumeSpacing.value.y);
    cropBoxSizeZ.value = Math.round(cropBoxSizeZ.value / volumeSpacing.value.z);
    const minSpacing = Math.min(
      volumeSpacing.value.x,
      volumeSpacing.value.y,
      volumeSpacing.value.z
    );
    cropSphereRadius.value = Math.round(cropSphereRadius.value / minSpacing);
    cropCylinderRadius.value = Math.round(
      cropCylinderRadius.value / minSpacing
    );
    cropCylinderHeight.value = Math.round(
      cropCylinderHeight.value / minSpacing
    );
  }
});

// 切换测量工具（1=Line, 2=Angle, 3=Rect, 4=Circle, 5=Bezier）
async function setMeasurementTool(toolType: number) {
  // 关闭定位线和裁切框
  showCrosshair.value = false;
  showCropBox.value = false;
  if (sessionId.value) {
    const ids = [
      `${sessionId.value}_axial`,
      `${sessionId.value}_coronal`,
      `${sessionId.value}_sagittal`,
      `${sessionId.value}_3d`,
    ];
    for (const id of ids) {
      await window.visualizationApi.setWindowCropBoxVisible(id, false);
    }
  }
  await window.visualizationApi.enableAPRCropBox(sessionId.value, false);
  if (sessionId.value) {
    try {
      await window.visualizationApi.setCrosshairVisible(sessionId.value, false);
    } catch (error) {
      console.error("[图像浏览] setCrosshairVisible失败:", error);
    }
  }

  // 设置工具类型
  currentToolType.value = toolType;
  await setAllWindowsToolType(toolType);

  const toolNames = [
    "定位线",
    "直线测量",
    "角度测量",
    "矩形测量",
    "圆形测量",
    "贝塞尔曲线",
  ];
  console.log("[图像浏览] 切换工具:", toolNames[toolType] || "未知工具");
}

async function openMeasurementsDialog() {
  if (!window.nativeBridge?.dialog?.open) {
    console.error("[Viewer] Dialog API not available");
    return;
  }

  await window.nativeBridge.dialog.open("measurements", {
    sessionId: sessionId.value,
    dialogType: "measurements",
  });
}

async function openScreenshotDialog() {
  // Legacy entry point; keep name for template binding.
  // New UX: do NOT open a new window. Toggle an inline panel instead.
  if (!sessionId.value) {
    ElMessage.warning("请先打开一个影像");
    return;
  }
  showScreenshotPanel.value = !showScreenshotPanel.value;
}

function getScreenshotRequestSize() {
  const dpr = Number.isFinite(window.devicePixelRatio)
    ? Math.max(1, window.devicePixelRatio)
    : 1;

  const getUnionRect = (els: Array<HTMLElement | null | undefined>) => {
    const rects = els
      .map((el) => el?.getBoundingClientRect?.())
      .filter(
        (r): r is DOMRect => !!r && Number.isFinite(r.width) && r.width > 0
      );
    if (rects.length === 0) return null;

    let left = rects[0].left;
    let top = rects[0].top;
    let right = rects[0].right;
    let bottom = rects[0].bottom;

    for (const r of rects.slice(1)) {
      left = Math.min(left, r.left);
      top = Math.min(top, r.top);
      right = Math.max(right, r.right);
      bottom = Math.max(bottom, r.bottom);
    }

    return {
      width: Math.max(1, Math.round((right - left) * dpr)),
      height: Math.max(1, Math.round((bottom - top) * dpr)),
    };
  };

  const pick = () => {
    if (layoutChoice.value === "view1") return view1Ref.value;
    if (layoutChoice.value === "view2") return view2Ref.value;
    if (layoutChoice.value === "view3") return view3Ref.value;
    // 注意：2x2 布局的值是 'quad'，不是 'view4'
    if (layoutChoice.value === "quad") return null;
    return view1Ref.value ?? view2Ref.value ?? view3Ref.value ?? view4Ref.value;
  };

  // quad 模式：截图尺寸应覆盖整个 2x2 区域，否则会导致 quad 拼图整体缩小。
  if (layoutChoice.value === "quad") {
    const union = getUnionRect([
      view1Ref.value,
      view2Ref.value,
      view3Ref.value,
      view4Ref.value,
    ]);
    if (union) return union;
  }

  const el = pick();
  const rect = el?.getBoundingClientRect?.();
  const w = rect?.width ? Math.round(rect.width * dpr) : 1024;
  const h = rect?.height ? Math.round(rect.height * dpr) : 1024;
  return {
    width: Math.max(1, w),
    height: Math.max(1, h),
  };
}

async function confirmScreenshotCapture() {
  if (!sessionId.value) {
    ElMessage.warning("请先打开一个影像");
    return;
  }
  const folderPath = props.panelData?.folderPath;
  if (!folderPath) {
    ElMessage.error("无法确定影像目录");
    return;
  }
  // Vue refs/arrays may be Proxies; IPC arguments must be structured-cloneable.
  const selection = Array.isArray(screenshotSelection.value)
    ? Array.from(screenshotSelection.value).map((v) => String(v))
    : [];
  if (selection.length === 0) {
    ElMessage.warning("请至少选择一个视图");
    return;
  }

  if (!window.visualizationApi?.captureAPRScreenshots) {
    ElMessage.error("截图功能未初始化（captureAPRScreenshots 不可用）");
    return;
  }

  try {
    const { width, height } = getScreenshotRequestSize();
    const r = await window.visualizationApi.captureAPRScreenshots(
      sessionId.value,
      String(folderPath),
      selection,
      { ...viewHwnds.value },
      width,
      height
    );
    if (r?.success) {
      const successCount = r.files?.length ?? 0;
      const errorCount = r.errors?.length ?? 0;

      if (errorCount === 0) {
        showScreenshotStatus({
          show: true,
          type: "success",
          message: `✅ 截图完成！共 ${successCount} 个视图，保存在 ${r.outputDir}`,
        });
      } else {
        const failedViews = r.errors?.map((e: any) => e.view).join(", ") || "";
        const errorDetails =
          r.errors?.map((e: any) => `${e.view}: ${e.error}`) || [];
        showScreenshotStatus({
          show: true,
          type: "error",
          message: `⚠️ 部分截图失败：${failedViews}（成功 ${successCount} 个）`,
          details: errorDetails,
        });
      }
      showScreenshotPanel.value = false;
    } else {
      showScreenshotStatus({
        show: true,
        type: "error",
        message: `❌ 截图失败: ${r?.error || "未知错误"}`,
      });
    }
  } catch (e: any) {
    console.error("[screenshot] capture failed:", e);
    showScreenshotStatus({
      show: true,
      type: "error",
      message: `❌ 截图异常: ${e?.message ?? String(e)}`,
    });
  }
}

async function resetViewAll() {
  if (!isLoaded.value || !sessionId.value) return;
  if (!window.visualizationApi?.resetView) {
    console.warn("[Viewer] visualizationApi.resetView not available");
    return;
  }

  const windowIds = [
    `${sessionId.value}_axial`,
    `${sessionId.value}_coronal`,
    `${sessionId.value}_sagittal`,
    `${sessionId.value}_3d`,
  ];

  try {
    await Promise.all(
      windowIds.map((id) => window.visualizationApi.resetView(id))
    );
  } catch (e) {
    console.warn("[Viewer] resetView failed:", e);
  }

  // Reset UI state (progress + rotation) to defaults.
  suppressAprUiWatch = true;
  aprProgress.value = [50, 50, 50] as any;
  aprRotate.value = [0, 0, 0] as any;
  queueMicrotask(() => {
    suppressAprUiWatch = false;
  });

  // Keep 3D mode consistent with the toggle.
  try {
    await window.visualizationApi.set3DOrthogonalMode(
      `${sessionId.value}_3d`,
      fixedZeroRotation.value
    );
  } catch {
    // ignore
  }
}

// 为所有窗口设置工具类型
async function setAllWindowsToolType(toolType: number) {
  if (!sessionId.value) return;

  const windowIds = [
    `${sessionId.value}_axial`,
    `${sessionId.value}_coronal`,
    `${sessionId.value}_sagittal`,
    `${sessionId.value}_3d`,
  ];

  await Promise.all(
    windowIds.map((id) =>
      window.visualizationApi.setWindowToolType(id, toolType)
    )
  );
}

// 根据当前布局更新所有原生窗口位置与大小
async function updateNativeWindowLayouts() {
  if (!isLoaded.value || !sessionId.value) return;

  const OFFSCREEN_X = -20000;
  const OFFSCREEN_Y = -20000;
  const OFFSCREEN_W = 1;
  const OFFSCREEN_H = 1;

  const views = [
    { ref: view1Ref, id: `${sessionId.value}_axial` },
    { ref: view2Ref, id: `${sessionId.value}_coronal` },
    { ref: view3Ref, id: `${sessionId.value}_sagittal` },
    { ref: view4Ref, id: `${sessionId.value}_3d` },
  ];

  const raiseVisibleWindows = async (visibleIds: string[]) => {
    // Throttle to avoid excessive native calls during continuous resize.
    const now = Date.now();
    if (now - lastRaiseAt < 250) return;
    lastRaiseAt = now;

    try {
      await Promise.all(
        visibleIds.map(async (id) => {
          await window.visualizationApi.raiseWindow?.(id);
          await window.visualizationApi.refreshWindowZOrder?.(id);
        })
      );
    } catch {
      // ignore
    }
  };

  // Quad: normal 2x2 layout.
  if (layoutChoice.value === "quad") {
    await Promise.all(
      views.map(async ({ ref, id }) => {
        const metrics = computeRelativeRect(ref.value);
        if (!metrics) return;
        const { x, y, width, height } = metrics;
        await window.visualizationApi.resizeWindow(id, x, y, width, height);
      })
    );

    // After resize, ensure z-order is sane so native windows receive mouse.
    await raiseVisibleWindows(views.map((v) => v.id));
    return;
  }

  // Single: selected view gets full area; others moved offscreen to avoid overlap/flicker.
  const selectedIndex =
    layoutChoice.value === "view1"
      ? 0
      : layoutChoice.value === "view2"
      ? 1
      : layoutChoice.value === "view3"
      ? 2
      : 3;
  const selected = views[selectedIndex];
  const selectedMetrics = computeRelativeRect(selected.ref.value);

  await Promise.all(
    views.map(async ({ ref, id }, idx) => {
      if (idx === selectedIndex && selectedMetrics) {
        const { x, y, width, height } = selectedMetrics;
        await window.visualizationApi.resizeWindow(id, x, y, width, height);
      } else {
        await window.visualizationApi.resizeWindow(
          id,
          OFFSCREEN_X,
          OFFSCREEN_Y,
          OFFSCREEN_W,
          OFFSCREEN_H
        );
      }
    })
  );

  // After resize, ensure selected window stays on top.
  await raiseVisibleWindows([views[selectedIndex].id]);
}

// 嵌入原生窗口到 div 中（使用 Win32 API SetParent）
async function embedWindow(container: HTMLDivElement, windowId: string) {
  const metrics = computeRelativeRect(container);
  if (!metrics) return;

  const { x, y, width, height } = metrics;

  try {
    await window.visualizationApi.embedWindow(windowId, x, y, width, height);
  } catch (error) {
    console.error("[图像浏览] 嵌入窗口失败:", error);
  }
}

// 监听 APR 进度变化，更新中心点（使用防抖避免频繁更新）
let aprUpdateTimer: NodeJS.Timeout | null = null;
watch(
  aprProgress,
  async (newProgress) => {
    if (!sessionId.value || !isLoaded.value) return;
    if (suppressAprUiWatch) return;
    lastUiCenterUpdateAt = Date.now();

    // 防抖：延迟更新，避免滑块拖动时频繁触发
    if (aprUpdateTimer) {
      clearTimeout(aprUpdateTimer);
    }

    aprUpdateTimer = setTimeout(async () => {
      // 将进度 [0-100] 映射到体数据实际坐标
      const x = (newProgress[0] / 100) * volumeWidth.value;
      const y = (newProgress[1] / 100) * volumeHeight.value;
      const z = (newProgress[2] / 100) * volumeDepth.value;

      try {
        await window.visualizationApi.updateCenter(sessionId.value, x, y, z);
      } catch (error) {
        console.error("[图像浏览] 更新中心点失败:", error);
      }
    }, 10);
  },
  { deep: true, immediate: false }
);

// === APR 旋转逻辑（重写）：
// 1) UI 滑条是写入源（绝对角度），通过 @input/@change 主动调用 native
// 2) 轮询只从 axial APR 读回旋转（不读 3d），避免 3d/窗口状态返回 0 导致回写“回弹”
// 3) 用户拖动滑条时，轮询不允许覆盖 UI（直到松手）
const aprUiRotating = ref(false);
let aprRotateCommitTimer: number | null = null;

function commitAprRotationNow() {
  if (!sessionId.value || !isLoaded.value) return;
  if (!window.visualizationApi?.updateRotation) return;
  if (fixedZeroRotation.value) return;

  lastUiRotationUpdateAt = Date.now();
  const [x, y, z] = aprRotate.value;

  void window.visualizationApi
    .updateRotation(sessionId.value, x, y, z)
    .catch((error: unknown) =>
      console.error("[图像浏览] 更新旋转失败:", error)
    );
}

function scheduleAprRotationCommit() {
  if (aprRotateCommitTimer != null) {
    window.clearTimeout(aprRotateCommitTimer);
  }
  // ElSlider @input 触发很频繁：做一个非常短的节流即可
  aprRotateCommitTimer = window.setTimeout(() => {
    aprRotateCommitTimer = null;
    commitAprRotationNow();
  }, 10);
}

function onAprRotateInput() {
  if (suppressAprUiWatch) return;
  if (fixedZeroRotation.value) {
    // 正交切片模式：强制归零
    suppressAprUiWatch = true;
    aprRotate.value = [0, 0, 0] as any;
    suppressAprUiWatch = false;
    return;
  }
  aprUiRotating.value = true;
  scheduleAprRotationCommit();
}

function onAprRotateChange() {
  if (suppressAprUiWatch) return;
  aprUiRotating.value = false;
  commitAprRotationNow();
}

// “显示正交切片”开启时，立即将 native 旋转归零
watch(
  fixedZeroRotation,
  async (enabled) => {
    if (!sessionId.value || !isLoaded.value) return;

    // Switch 3D mode for the 3D window.
    try {
      await window.visualizationApi.set3DOrthogonalMode(
        `${sessionId.value}_3d`,
        enabled
      );
    } catch (error) {
      console.error("[图像浏览] 切换3D正交模式失败:", error);
    }

    if (!enabled) return;

    suppressAprUiWatch = true;
    aprRotate.value = [0, 0, 0] as any;
    suppressAprUiWatch = false;

    lastUiRotationUpdateAt = Date.now();
    try {
      await window.visualizationApi.updateRotation(sessionId.value, 0, 0, 0);
    } catch (error) {
      console.error("[图像浏览] 归零旋转失败:", error);
    }
  },
  { immediate: false }
);

// 监听 MIP/MinIP 变化
// MIP/MinIP 逻辑：mipValue > 0 时使用MIP模式，minipValue > 0 时使用MinIP模式
// 两者同时 > 0 时优先使用MIP
let projectionModeTimer: NodeJS.Timeout | null = null;
watch(
  [mipValue, minipValue],
  async ([mip, minip]) => {
    if (!sessionId.value || !isLoaded.value) return;
    if (suppressAprUiWatch) return;

    if (projectionModeTimer) {
      clearTimeout(projectionModeTimer);
    }

    projectionModeTimer = setTimeout(async () => {
      try {
        let mode = 0; // Normal
        let thickness = 10.0;

        if (mip > 0) {
          mode = 1; // MIP
          thickness = mip; // mipValue 0-100 代表厚度（体素数）
        } else if (minip > 0) {
          mode = 2; // MinIP
          thickness = minip;
        }

        if (window.visualizationApi?.setProjectionMode) {
          await window.visualizationApi.setProjectionMode(
            sessionId.value,
            mode,
            thickness
          );
        }
      } catch (error) {
        console.error("[图像浏览] 更新投影模式失败:", error);
      }
    }, 50);
  },
  { deep: true, immediate: false }
);

// 鼠标事件处理（转发到GLFW窗口）
function handleMouseDown(event: MouseEvent) {
  event.stopPropagation();
}

let lastRaiseAt = 0;
async function ensureNativeWindowOnTop(event: MouseEvent) {
  if (!sessionId.value || !isLoaded.value) return;
  const target = event.currentTarget as HTMLDivElement | null;
  if (!target) return;

  let windowId: string | null = null;
  if (target === view1Ref.value) windowId = `${sessionId.value}_axial`;
  else if (target === view2Ref.value) windowId = `${sessionId.value}_coronal`;
  else if (target === view3Ref.value) windowId = `${sessionId.value}_sagittal`;
  else if (target === view4Ref.value) windowId = `${sessionId.value}_3d`;

  if (!windowId) return;

  // Throttle to avoid spamming native calls on noisy hover transitions.
  const now = Date.now();
  if (now - lastRaiseAt < 250) return;
  lastRaiseAt = now;

  try {
    await window.visualizationApi.raiseWindow?.(windowId);
    await window.visualizationApi.refreshWindowZOrder?.(windowId);
  } catch {
    // ignore
  }
}

function handleMouseMove(event: MouseEvent) {
  event.stopPropagation();
}

function handleMouseUp(event: MouseEvent) {
  event.stopPropagation();
}

function handleWheel(event: WheelEvent) {
  event.stopPropagation();
  event.preventDefault();
}

onMounted(async () => {
  // 组件首次挂载时也尝试恢复窗口（避免从其它 Tab 切回时残留 hidden/disabled 状态）
  try {
    await window.visualizationApi?.showAllWindows?.();
  } catch {
    // ignore
  }

  if (props.panelData) {
    loadAPRViews(props.panelData.folderPath);
  }

  // 监听独立 dialog 窗口的返回结果（GeneralModule dialogs）
  if (window.nativeBridge?.dialog?.onResult) {
    window.nativeBridge.dialog.onResult(async (result: any) => {
      if (
        result?.sessionId &&
        sessionId.value &&
        result.sessionId !== sessionId.value
      ) {
        return;
      }

      // 处理截图对话框结果
      const isScreenshotDialog =
        result?.dialogType === "screenshot" ||
        (result?.action === "capture" &&
          Array.isArray(result?.data?.selection));

      if (isScreenshotDialog) {
        if (result.action === "capture") {
          const folderPath = props.panelData?.folderPath;
          if (!folderPath) {
            ElMessage.error("无法确定影像目录");
            return;
          }

          const selection = Array.isArray(result?.data?.selection)
            ? result.data.selection
            : [];

          try {
            const r = await window.visualizationApi?.captureAPRScreenshots?.(
              sessionId.value,
              folderPath,
              selection,
              { ...viewHwnds.value }
            );
            if (r?.success) {
              ElMessage.success(`截图成功，保存在 ${r.outputDir}`);
              if (Array.isArray(r?.errors) && r.errors.length > 0) {
                const first = r.errors[0];
                ElMessage.warning(
                  `部分视图截图失败：${first?.view ?? ""} ${first?.error ?? ""}`
                );
              }
            } else {
              ElMessage.error(r?.error || "截图失败");
            }
          } catch (e: any) {
            console.error("[screenshot] capture failed:", e);
            ElMessage.error(e?.message ?? String(e));
          }
        }
        return;
      }

      // 处理裁切设置对话框结果
      if (result?.dialogType === "cropsettings") {
        if (result.action === "apply" && result?.data) {
          try {
            const data = result.data;

            // 设置裁切形状
            await window.visualizationApi.setCropShape(data.shape);

            // 如果是圆柱体，设置方向
            if (data.shape === 2) {
              await window.visualizationApi.setCropCylinderDirection(
                data.direction
              );
            }

            // 按尺寸设置裁切框（dialog 返回单位已转换为像素）
            const aprState = await window.visualizationApi.getAPRState(
              sessionId.value,
              "axial"
            );

            const diameter = Number(data.diameter ?? 0);
            const cylinderHeight = Number(data.cylinderHeight ?? 0);

            let sizeX = Number(data.sizeX ?? 0);
            let sizeY = Number(data.sizeY ?? 0);
            let sizeZ = Number(data.sizeZ ?? 0);

            if (data.shape === 1) {
              // Sphere: use diameter for all axes.
              const d = diameter > 0 ? diameter : Math.min(sizeX, sizeY, sizeZ);
              sizeX = d;
              sizeY = d;
              sizeZ = d;
            } else if (data.shape === 2) {
              // Cylinder: map height to selected axis; diameter to the other two.
              const d = diameter > 0 ? diameter : Math.min(sizeX, sizeY);
              const h = cylinderHeight > 0 ? cylinderHeight : sizeZ;
              const dir = Number(data.direction ?? 0);
              if (dir === 0) {
                // Z axis
                sizeX = d;
                sizeY = d;
                sizeZ = h;
              } else if (dir === 1) {
                // Y axis
                sizeX = d;
                sizeY = h;
                sizeZ = d;
              } else {
                // X axis
                sizeX = h;
                sizeY = d;
                sizeZ = d;
              }
            }

            if (aprState && sizeX > 0 && sizeY > 0 && sizeZ > 0) {
              await window.visualizationApi.setCropBoxSize(
                sessionId.value,
                sizeX,
                sizeY,
                sizeZ,
                aprState.volumeWidth,
                aprState.volumeHeight,
                aprState.volumeDepth
              );
            }

            // 刷新视图
            await window.visualizationApi.invalidateAllWindows();

            const shapeNames = ["立方体", "球体", "圆柱体"];
            ElMessage.success(
              `裁切设置已更新: ${shapeNames[data.shape] || "未知形状"}`
            );
          } catch (error) {
            console.warn("[图像浏览] 裁切设置应用失败:", error);
            ElMessage.error("裁切设置应用失败");
          }
        }
        return;
      }
    });
  }

  // 监听窗口大小变化，调整嵌入的Win32窗口
  let resizeTimer: NodeJS.Timeout | null = null;
  const resizeObserver = new ResizeObserver(() => {
    if (!isLoaded.value || !sessionId.value) return;

    // 防抖：避免频繁resize
    if (resizeTimer) clearTimeout(resizeTimer);

    resizeTimer = setTimeout(() => {
      updateNativeWindowLayouts();
    }, 120);
  });

  // 观察所有视图容器
  if (view1Ref.value) resizeObserver.observe(view1Ref.value);
  if (view2Ref.value) resizeObserver.observe(view2Ref.value);
  if (view3Ref.value) resizeObserver.observe(view3Ref.value);
  if (view4Ref.value) resizeObserver.observe(view4Ref.value);

  // Electron窗口尺寸/位置变化时，重新同步原生窗口
  const boundsChangeListener = () => {
    updateNativeWindowLayouts();
  };
  window.ipcRenderer?.on(
    "electron-window-bounds-changed",
    boundsChangeListener
  );

  watch(layoutChoice, async () => {
    // Wait for DOM to reflow before measuring.
    await nextTick();
    await updateNativeWindowLayouts();
  });

  // 保存observer以便cleanup
  (onUnmounted as any)(() => {
    resizeObserver.disconnect();
    window.ipcRenderer?.off(
      "electron-window-bounds-changed",
      boundsChangeListener
    );
  });

  // 轮询 native APR 状态，用于同步进度条/旋转（POC：20Hz）
  const startAprStatePolling = () => {
    if (aprStatePollTimer != null) return;

    aprStatePollTimer = window.setInterval(async () => {
      if (!isLoaded.value || !sessionId.value) return;
      if (!window.visualizationApi?.getAPRState) return;

      // 避免与用户拖动滑块互相打架：UI最近刚改过就先不覆盖
      const now = Date.now();
      if (
        now - lastUiCenterUpdateAt < 150 ||
        now - lastUiRotationUpdateAt < 150
      ) {
        return;
      }

      try {
        // Single source of truth for polling: axial APR only.
        // NOTE: Do NOT poll "3d" here; it may refer to a window/camera state and can return 0,
        // which previously caused the UI to write back 0 and snap rotation.
        const stAxial = await window.visualizationApi.getAPRState(
          sessionId.value,
          "axial"
        );

        if (!stAxial?.success) return;
        if (
          stAxial.centerX == null ||
          stAxial.centerY == null ||
          stAxial.centerZ == null
        ) {
          return;
        }

        // 正交切片模式：如果 native 被交互旋转了，强制拉回 0
        if (fixedZeroRotation.value) {
          const nonZeroNative =
            Math.abs((stAxial as any).rotX) > 0.01 ||
            Math.abs((stAxial as any).rotY) > 0.01 ||
            Math.abs((stAxial as any).rotZ) > 0.01;
          if (nonZeroNative) {
            lastUiRotationUpdateAt = Date.now();
            try {
              await window.visualizationApi.updateRotation(
                sessionId.value,
                0,
                0,
                0
              );
            } catch (error) {
              console.error("[图像浏览] 强制正交旋转失败:", error);
            }
          }
        }

        // native coords -> [0..100] progress
        const px = (stAxial.centerX / volumeWidth.value) * 100;
        const py = (stAxial.centerY / volumeHeight.value) * 100;
        const pz = (stAxial.centerZ / volumeDepth.value) * 100;

        const clamp01 = (v: number) => Math.max(0, Math.min(100, v));
        const nextProgress = [clamp01(px), clamp01(py), clamp01(pz)];
        const nextRotate = fixedZeroRotation.value
          ? ([0, 0, 0] as any)
          : ([
              (stAxial as any).rotX,
              (stAxial as any).rotY,
              (stAxial as any).rotZ,
            ] as any);

        // 只有明显变化才写回，降低抖动
        const diff = (a: number, b: number) => Math.abs(a - b);
        const progressChanged =
          diff(nextProgress[0], aprProgress.value[0]) > 0.2 ||
          diff(nextProgress[1], aprProgress.value[1]) > 0.2 ||
          diff(nextProgress[2], aprProgress.value[2]) > 0.2;
        const rotateChanged =
          diff(nextRotate[0], aprRotate.value[0]) > 0.2 ||
          diff(nextRotate[1], aprRotate.value[1]) > 0.2 ||
          diff(nextRotate[2], aprRotate.value[2]) > 0.2;

        // 同步裁切框状态（从Native获取并更新UI参数）- 放在进度/旋转检查之前
        if (showCropBox.value && now - lastCropUiUpdateAt > 200) {
          try {
            const cropState = await window.visualizationApi.getCropSettings(
              sessionId.value
            );
            if (cropState && cropState.cropBox) {
              const box = cropState.cropBox;
              const sizeXPx = Math.abs(box.xEnd - box.xStart);
              const sizeYPx = Math.abs(box.yEnd - box.yStart);
              const sizeZPx = Math.abs(box.zEnd - box.zStart);

              suppressCropUiWatch = true;
              try {
                // 根据当前形状更新对应参数
                if (cropShape.value === 0) {
                  // 立方体
                  if (cropUnitMode.value === "mm") {
                    cropBoxSizeX.value =
                      Math.round(sizeXPx * volumeSpacing.value.x * 10) / 10;
                    cropBoxSizeY.value =
                      Math.round(sizeYPx * volumeSpacing.value.y * 10) / 10;
                    cropBoxSizeZ.value =
                      Math.round(sizeZPx * volumeSpacing.value.z * 10) / 10;
                  } else {
                    cropBoxSizeX.value = Math.round(sizeXPx);
                    cropBoxSizeY.value = Math.round(sizeYPx);
                    cropBoxSizeZ.value = Math.round(sizeZPx);
                  }
                } else if (cropShape.value === 1) {
                  // 球体：取最小尺寸的一半作为半径
                  const radiusPx = Math.min(sizeXPx, sizeYPx, sizeZPx) / 2;
                  if (cropUnitMode.value === "mm") {
                    const minSpacing = Math.min(
                      volumeSpacing.value.x,
                      volumeSpacing.value.y,
                      volumeSpacing.value.z
                    );
                    cropSphereRadius.value =
                      Math.round(radiusPx * minSpacing * 10) / 10;
                  } else {
                    cropSphereRadius.value = Math.round(radiusPx);
                  }
                } else if (cropShape.value >= 2 && cropShape.value <= 4) {
                  // 圆柱：根据方向计算半径和高度
                  let radiusPx: number, heightPx: number;
                  if (cropShape.value === 2) {
                    // 轴向圆柱（沿Z轴）
                    radiusPx = Math.min(sizeXPx, sizeYPx) / 2;
                    heightPx = sizeZPx;
                  } else if (cropShape.value === 3) {
                    // 冠状圆柱（沿Y轴）
                    radiusPx = Math.min(sizeXPx, sizeZPx) / 2;
                    heightPx = sizeYPx;
                  } else {
                    // 矢状圆柱（沿X轴）
                    radiusPx = Math.min(sizeYPx, sizeZPx) / 2;
                    heightPx = sizeXPx;
                  }
                  if (cropUnitMode.value === "mm") {
                    const minSpacing = Math.min(
                      volumeSpacing.value.x,
                      volumeSpacing.value.y,
                      volumeSpacing.value.z
                    );
                    cropCylinderRadius.value =
                      Math.round(radiusPx * minSpacing * 10) / 10;
                    cropCylinderHeight.value =
                      Math.round(heightPx * minSpacing * 10) / 10;
                  } else {
                    cropCylinderRadius.value = Math.round(radiusPx);
                    cropCylinderHeight.value = Math.round(heightPx);
                  }
                }
                await nextTick();
              } finally {
                suppressCropUiWatch = false;
              }
            }
          } catch {
            // ignore crop sync errors
          }
        }

        if (!progressChanged && !rotateChanged) return;

        // 用户正在拖动旋转滑条时，绝不允许轮询覆盖 UI
        if (aprUiRotating.value) {
          if (progressChanged) {
            suppressAprUiWatch = true;
            try {
              aprProgress.value = nextProgress as any;
              await nextTick();
            } finally {
              suppressAprUiWatch = false;
            }
          }
          return;
        }

        suppressAprUiWatch = true;
        try {
          if (progressChanged) aprProgress.value = nextProgress as any;
          if (rotateChanged) aprRotate.value = nextRotate as any;
          // Wait for watchers to flush while suppression is active.
          await nextTick();
        } finally {
          suppressAprUiWatch = false;
        }
      } catch {
        // ignore polling errors
      }
    }, 50);
  };

  startAprStatePolling();
});

// 清理资源
onUnmounted(() => {
  void stopRenderLoop();

  if (aprStatePollTimer != null) {
    window.clearInterval(aprStatePollTimer);
    aprStatePollTimer = null;
  }

  if (sessionId.value) {
    window.visualizationApi
      .destroyAPR(sessionId.value)
      .catch((error: unknown) =>
        console.error("[图像浏览] 清理APR失败:", error)
      );
  }
});
</script>

<template>
  <div class="viewer-page">
    <!-- 左侧控制面板 -->
    <aside class="left-panel">
      <ElCard class="control-card" shadow="hover">
        <!-- 窗口切换（置顶） -->
        <div class="section">
          <div class="section-title">窗口切换</div>
          <div class="btn-row">
            <ElRadioGroup v-model="layoutChoice" size="small">
              <ElRadioButton label="quad">全部</ElRadioButton>
              <ElRadioButton label="view1">轴位</ElRadioButton>
              <ElRadioButton label="view2">冠状位</ElRadioButton>
              <ElRadioButton label="view3">矢状位</ElRadioButton>
              <ElRadioButton label="view4">3D</ElRadioButton>
            </ElRadioGroup>
          </div>
        </div>

        <!-- 显示区 -->
        <div class="section">
          <div class="section-title">显示</div>
          <div class="btn-row">
            <ElButton
              size="large"
              :type="showCrosshair ? 'success' : 'primary'"
              :icon="View"
              @click="toggleCrosshair"
            >
              显示定位线
            </ElButton>
            <ElButton
              size="large"
              :type="showCropBox ? 'success' : 'primary'"
              :icon="Crop"
              @click="toggleCropBox"
            >
              显示裁切框
            </ElButton>
          </div>
          <!-- 裁切框形状设置（内联显示，showCropBox时可见） -->
          <div v-if="showCropBox" class="crop-settings-inline">
            <div class="crop-setting-row">
              <span class="crop-label">形状:</span>
              <ElSelect v-model="cropShape" size="small" style="width: 110px">
                <ElOption :value="0" label="立方体" />
                <ElOption :value="1" label="球体" />
                <ElOption :value="2" label="轴向圆柱" />
                <ElOption :value="3" label="冠状圆柱" />
                <ElOption :value="4" label="矢状圆柱" />
              </ElSelect>
              <span class="crop-label" style="margin-left: 8px">单位:</span>
              <ElSwitch
                v-model="cropUnitMode"
                active-value="mm"
                inactive-value="pixel"
                active-text="mm"
                inactive-text="像素"
                size="small"
              />
            </div>
            <!-- 立方体尺寸 -->
            <div v-if="cropShape === 0" class="crop-setting-row">
              <span class="crop-label">长(X):</span>
              <input
                type="number"
                v-model.number="cropBoxSizeX"
                :min="1"
                class="crop-input"
              />
              <span class="crop-label">宽(Y):</span>
              <input
                type="number"
                v-model.number="cropBoxSizeY"
                :min="1"
                class="crop-input"
              />
              <span class="crop-label">高(Z):</span>
              <input
                type="number"
                v-model.number="cropBoxSizeZ"
                :min="1"
                class="crop-input"
              />
              <span class="crop-unit">{{
                cropUnitMode === "mm" ? "mm" : "px"
              }}</span>
            </div>
            <!-- 球体半径 -->
            <div v-if="cropShape === 1" class="crop-setting-row">
              <span class="crop-label">半径:</span>
              <input
                type="number"
                v-model.number="cropSphereRadius"
                :min="1"
                class="crop-input"
              />
              <span class="crop-unit">{{
                cropUnitMode === "mm" ? "mm" : "px"
              }}</span>
            </div>
            <!-- 圆柱参数 -->
            <div
              v-if="cropShape >= 2 && cropShape <= 4"
              class="crop-setting-row"
            >
              <span class="crop-label">半径:</span>
              <input
                type="number"
                v-model.number="cropCylinderRadius"
                :min="1"
                class="crop-input"
              />
              <span class="crop-label">高度:</span>
              <input
                type="number"
                v-model.number="cropCylinderHeight"
                :min="1"
                class="crop-input"
              />
              <span class="crop-unit">{{
                cropUnitMode === "mm" ? "mm" : "px"
              }}</span>
            </div>
            <!-- 执行裁切按钮 -->
            <div class="crop-setting-row" style="margin-top: 8px">
              <ElButton
                size="small"
                type="warning"
                :icon="Scissor"
                @click="executeCrop"
              >
                执行裁切
              </ElButton>
            </div>
          </div>
        </div>

        <!-- APR进度滑动条 -->
        <div class="section">
          <div class="section-title">APR 三面进度</div>
          <div class="slider-row">
            <ElTooltip content="轴向面进度" placement="top"
              ><ElSlider
                v-model="aprProgress[0]"
                :min="0"
                :max="100"
                :show-tooltip="true"
            /></ElTooltip>
            <ElTooltip content="冠状面进度" placement="top"
              ><ElSlider
                v-model="aprProgress[1]"
                :min="0"
                :max="100"
                :show-tooltip="true"
            /></ElTooltip>
            <ElTooltip content="矢状面进度" placement="top"
              ><ElSlider
                v-model="aprProgress[2]"
                :min="0"
                :max="100"
                :show-tooltip="true"
            /></ElTooltip>
          </div>
        </div>

        <!-- APR旋转滑动条 -->
        <div class="section">
          <div class="section-title">APR 旋转</div>
          <div class="slider-row">
            <ElTooltip content="X轴旋转" placement="top"
              ><ElSlider
                v-model="aprRotate[0]"
                :min="-180"
                :max="180"
                :show-tooltip="true"
                @input="onAprRotateInput"
                @change="onAprRotateChange"
            /></ElTooltip>
            <ElTooltip content="Y轴旋转" placement="top"
              ><ElSlider
                v-model="aprRotate[1]"
                :min="-180"
                :max="180"
                :show-tooltip="true"
                @input="onAprRotateInput"
                @change="onAprRotateChange"
            /></ElTooltip>
            <ElTooltip content="Z轴旋转" placement="top"
              ><ElSlider
                v-model="aprRotate[2]"
                :min="-180"
                :max="180"
                :show-tooltip="true"
                @input="onAprRotateInput"
                @change="onAprRotateChange"
            /></ElTooltip>
          </div>
        </div>

        <!-- MIP/MINIP滑动条 -->
        <div class="section">
          <div class="section-title">MIP / MINIP</div>
          <div class="slider-row">
            <ElTooltip content="MIP厚度（最大10层）" placement="top"
              ><ElSlider
                v-model="mipValue"
                :min="0"
                :max="10"
                :show-tooltip="true"
            /></ElTooltip>
            <ElTooltip content="MINIP厚度（最大10层）" placement="top"
              ><ElSlider
                v-model="minipValue"
                :min="0"
                :max="10"
                :show-tooltip="true"
            /></ElTooltip>
          </div>
        </div>

        <!-- 测量工具 -->
        <div class="section">
          <div class="section-title">测量工具</div>
          <div class="btn-row">
            <ElButton
              size="small"
              :type="currentToolType === 1 ? 'success' : 'primary'"
              :icon="EditPen"
              @click="setMeasurementTool(1)"
            >
              直线
            </ElButton>
            <ElButton
              size="small"
              :type="currentToolType === 2 ? 'success' : 'primary'"
              :icon="Connection"
              @click="setMeasurementTool(2)"
            >
              角度
            </ElButton>
            <ElButton
              size="small"
              :type="currentToolType === 3 ? 'success' : 'primary'"
              :icon="Crop"
              @click="setMeasurementTool(3)"
            >
              方形
            </ElButton>
            <ElButton
              size="small"
              :type="currentToolType === 4 ? 'success' : 'primary'"
              :icon="Plus"
              @click="setMeasurementTool(4)"
            >
              圆形
            </ElButton>
          </div>
          <div class="btn-row">
            <ElButton
              size="small"
              :type="currentToolType === 5 ? 'success' : 'primary'"
              :icon="EditPen"
              @click="setMeasurementTool(5)"
            >
              贝塞尔
            </ElButton>
            <ElButton
              size="small"
              :type="currentToolType === 6 ? 'success' : 'primary'"
              :icon="EditPen"
              @click="setMeasurementTool(6)"
            >
              任意曲线
            </ElButton>
            <ElButton
              size="small"
              type="primary"
              :icon="View"
              @click="openMeasurementsDialog"
            >
              测量列表
            </ElButton>
          </div>
        </div>

        <!-- 其他区 -->
        <div class="section">
          <div class="section-title">其他</div>
          <div class="btn-row">
            <ElButton
              type="primary"
              :icon="Picture"
              @click="openScreenshotDialog"
            >
              截图
            </ElButton>
            <ElButton
              type="primary"
              :icon="View"
              @click="toggleWindowLevelPanel"
            >
              窗宽窗位
            </ElButton>
            <ElButton type="primary" :icon="Refresh" @click="resetViewAll">
              重置
            </ElButton>
          </div>

          <!-- 截图（内联确认面板） -->
          <div v-if="showScreenshotPanel" class="crop-settings-inline">
            <div class="crop-setting-row" style="align-items: center">
              <span class="crop-label">视图:</span>
              <ElCheckboxGroup v-model="screenshotSelection">
                <ElCheckbox label="quad">四视图(2×2)</ElCheckbox>
                <ElCheckbox label="axial">轴位</ElCheckbox>
                <ElCheckbox label="coronal">冠状位</ElCheckbox>
                <ElCheckbox label="sagittal">矢状位</ElCheckbox>
                <ElCheckbox label="3d">3D</ElCheckbox>
              </ElCheckboxGroup>
              <ElButton
                size="small"
                type="primary"
                @click="confirmScreenshotCapture"
              >
                确认截图
              </ElButton>
            </div>
          </div>

          <!-- 截图状态显示 -->
          <div
            v-if="screenshotStatus.show"
            class="screenshot-status-panel"
            :class="`status-${screenshotStatus.type}`"
          >
            <div class="status-message">
              {{ screenshotStatus.message }}
            </div>
            <div
              v-if="
                screenshotStatus.details && screenshotStatus.details.length > 0
              "
              class="status-details"
            >
              <div
                v-for="detail in screenshotStatus.details"
                :key="detail"
                class="status-detail-item"
              >
                {{ detail }}
              </div>
            </div>
            <ElButton
              size="small"
              type="text"
              @click="screenshotStatus.show = false"
              style="margin-left: auto"
            >
              ×
            </ElButton>
          </div>

          <!-- 窗宽窗位（内联显示） -->
          <div v-if="showWindowLevelPanel" class="crop-settings-inline">
            <div class="crop-setting-row">
              <span class="crop-label">窗宽:</span>
              <input
                type="text"
                inputmode="numeric"
                v-model="windowWidthInput"
                class="crop-input"
                @input="
                  windowWidthInput = sanitizeIntegerString(
                    windowWidthInput,
                    false
                  )
                "
              />
              <span class="crop-label">窗位:</span>
              <input
                type="text"
                inputmode="numeric"
                v-model="windowLevelInput"
                class="crop-input"
                @input="
                  windowLevelInput = sanitizeIntegerString(
                    windowLevelInput,
                    true
                  )
                "
              />
              <ElButton
                size="small"
                type="primary"
                @click="applyWindowLevelAll"
              >
                设定
              </ElButton>
            </div>
          </div>
        </div>
      </ElCard>
    </aside>

    <!-- 右侧显示面板：2x2 网格 -->
    <main class="right-panel">
      <div class="grid2x2" v-if="layoutChoice === 'quad'">
        <ElCard class="view-cell" shadow="hover">
          <div
            ref="view1Ref"
            class="view-container"
            @mouseenter="ensureNativeWindowOnTop"
            @mousedown="handleMouseDown"
            @mousemove="handleMouseMove"
            @mouseup="handleMouseUp"
            @wheel="handleWheel"
          ></div>
          <div v-if="!isLoaded" class="view-placeholder">轴位视图</div>
        </ElCard>
        <ElCard class="view-cell" shadow="hover">
          <div
            ref="view2Ref"
            class="view-container"
            @mouseenter="ensureNativeWindowOnTop"
            @mousedown="handleMouseDown"
            @mousemove="handleMouseMove"
            @mouseup="handleMouseUp"
            @wheel="handleWheel"
          ></div>
          <div v-if="!isLoaded" class="view-placeholder">冠状位视图</div>
        </ElCard>
        <ElCard class="view-cell" shadow="hover">
          <div
            ref="view3Ref"
            class="view-container"
            @mouseenter="ensureNativeWindowOnTop"
            @mousedown="handleMouseDown"
            @mousemove="handleMouseMove"
            @mouseup="handleMouseUp"
            @wheel="handleWheel"
          ></div>
          <div v-if="!isLoaded" class="view-placeholder">矢状位视图</div>
        </ElCard>
        <ElCard class="view-cell" shadow="hover">
          <div
            ref="view4Ref"
            class="view-container"
            @mouseenter="ensureNativeWindowOnTop"
            @mousedown="handleMouseDown"
            @mousemove="handleMouseMove"
            @mouseup="handleMouseUp"
            @wheel="handleWheel"
          ></div>
          <div v-if="!isLoaded" class="view-placeholder">3D 视图</div>
        </ElCard>
      </div>

      <div class="grid1x1" v-else>
        <ElCard
          v-if="layoutChoice === 'view1'"
          class="view-cell"
          shadow="hover"
        >
          <div
            ref="view1Ref"
            class="view-container"
            @mouseenter="ensureNativeWindowOnTop"
            @mousedown="handleMouseDown"
            @mousemove="handleMouseMove"
            @mouseup="handleMouseUp"
            @wheel="handleWheel"
          ></div>
          <div v-if="!isLoaded" class="view-placeholder">轴位视图</div>
        </ElCard>

        <ElCard
          v-else-if="layoutChoice === 'view2'"
          class="view-cell"
          shadow="hover"
        >
          <div
            ref="view2Ref"
            class="view-container"
            @mouseenter="ensureNativeWindowOnTop"
            @mousedown="handleMouseDown"
            @mousemove="handleMouseMove"
            @mouseup="handleMouseUp"
            @wheel="handleWheel"
          ></div>
          <div v-if="!isLoaded" class="view-placeholder">冠状位视图</div>
        </ElCard>

        <ElCard
          v-else-if="layoutChoice === 'view3'"
          class="view-cell"
          shadow="hover"
        >
          <div
            ref="view3Ref"
            class="view-container"
            @mouseenter="ensureNativeWindowOnTop"
            @mousedown="handleMouseDown"
            @mousemove="handleMouseMove"
            @mouseup="handleMouseUp"
            @wheel="handleWheel"
          ></div>
          <div v-if="!isLoaded" class="view-placeholder">矢状位视图</div>
        </ElCard>

        <ElCard v-else class="view-cell" shadow="hover">
          <div
            ref="view4Ref"
            class="view-container"
            @mouseenter="ensureNativeWindowOnTop"
            @mousedown="handleMouseDown"
            @mousemove="handleMouseMove"
            @mouseup="handleMouseUp"
            @wheel="handleWheel"
          ></div>
        </ElCard>
      </div>
    </main>
  </div>
</template>

<!-- layout styles are shared globally in src/style.css -->

<style scoped>
.screenshot-status-panel {
  padding: 12px 16px;
  margin: 8px 0;
  border-radius: 6px;
  border: 1px solid;
  display: flex;
  align-items: flex-start;
  gap: 12px;
  font-size: 13px;
  line-height: 1.4;
}

.status-success {
  background-color: #f0f9ff;
  border-color: #67c23a;
  color: #529b2e;
}

.status-error {
  background-color: #fef2f2;
  border-color: #f56c6c;
  color: #c45656;
}

.status-info {
  background-color: #f4f4f5;
  border-color: #909399;
  color: #606266;
}

.status-message {
  flex: 1;
  font-weight: 500;
}

.status-details {
  display: flex;
  flex-direction: column;
  gap: 4px;
  font-size: 12px;
  opacity: 0.8;
}

.status-detail-item {
  padding: 2px 6px;
  background: rgba(0, 0, 0, 0.05);
  border-radius: 4px;
}
</style>
