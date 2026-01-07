<script setup lang="ts">
import "element-plus/dist/index.css";
import { ref, watch, onMounted, onUnmounted, nextTick } from "vue";
import {
  ElButton,
  ElCard,
  ElCheckboxGroup,
  ElTable,
  ElTableColumn,
  ElSwitch,
  ElTooltip,
  ElDialog,
  ElSlider,
  ElRadioGroup,
  ElRadioButton,
  ElRadio,
  ElSelect,
  ElOption,
  ElInputNumber,
  ElColorPicker,
  ElInput,
} from "element-plus";
import { Crop, Plus, EditPen, Connection, View } from "@element-plus/icons-vue";
import { pickMaskColor16 } from "../utils/maskPalette";

// 禁用会遮挡原生 HWND 的 toast（避免闪屏）
const ElMessage = {
  success: (_msg?: any) => {},
  info: (_msg?: any) => {},
  warning: (_msg?: any) => {},
  error: (_msg?: any) => {},
} as const;

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

// UI状态
// 3D 默认使用 Raycast（体渲染），因此默认不显示正交切片
const fixedZeroRotation = ref(false);
const FALLBACK_WINDOW_WIDTH = 4096;
const FALLBACK_WINDOW_LEVEL = 2048;
const showCrosshair = ref(true); // 是否显示定位线
const currentToolType = ref(0); // 当前工具类型：0=定位线, 1=Line, 2=Angle, 3=Rect, 4=Circle, 5=Bezier

// 所有对话框改为独立窗口，不再需要 visible 状态

// Save mask dialog state
const maskSaveName = ref("");

// Screenshot dialog state
const screenshotSelection = ref<string[]>([
  "axial",
  "coronal",
  "sagittal",
  "3d",
  "mpr+3d",
]);

// Current folder path
const currentFolderPath = ref<string>("");

// Selected rows
const selectedRegion2D = ref<any>(null);
const selectedRegion3D = ref<any>(null);

// Color picker state
const selectedColor = ref("#ff0000");
const selectedOpacity = ref(100);

// RigMark dialog state (仅用于独立窗口的初始值)
const rigMarkMin = ref(0);
const rigMarkMax = ref(255);
const rigMarkColor = ref("#ff0000"); // 预览颜色
const histogramData = ref<number[]>([]); // 直方图数据
const histogramMinValue = ref(0); // CT值最小值
const histogramMaxValue = ref(255); // CT值最大值

// Morphology dialog state
const morphologyOperation = ref("dilate"); // dilate, erode, open, close
const morphologyKernel = ref(3);
const morphologyIterations = ref(1);

// Boolean dialog state
const booleanOperation = ref("union"); // union, intersect, subtract
const booleanVarA = ref("");
const booleanVarB = ref("");

// 3D Config state
const selected2DMaskFor3D = ref("");

// ROI/Region 控制表格数据（初始为空，通过添加/加载填充）
const region2DList = ref<
  Array<{
    maskId?: number;
    color: string;
    visible: boolean;
    min: number;
    max: number;
    name?: string;
  }>
>([]);

const region3DList = ref<
  Array<{
    color: string;
    visible: boolean;
    opacity: number;
    sourceMask2D?: string;
  }>
>([]);

// APR 视图引用（4个div容器）
const view1Ref = ref<HTMLDivElement | null>(null);
const view2Ref = ref<HTMLDivElement | null>(null);
const view3Ref = ref<HTMLDivElement | null>(null);
const view4Ref = ref<HTMLDivElement | null>(null);

type RoiLayoutChoice = "quad" | "view1" | "view2" | "view3" | "view4";
const layoutChoice = ref<RoiLayoutChoice>("quad");

// 直方图画布引用
const histogramCanvas = ref<HTMLCanvasElement | null>(null);

// APR Session ID（用于标识此次会话）
const sessionId = ref<string>("");
const isLoaded = ref(false);

// Native window IDs returned by CreateMPRViews (do not assume suffixes).
const nativeWinAxialId = ref<string>("");
const nativeWinCoronalId = ref<string>("");
const nativeWinSagittalId = ref<string>("");
const nativeWin3dId = ref<string>("");

function getWin3dId(): string {
  return (
    nativeWin3dId.value || (sessionId.value ? `${sessionId.value}_3d` : "")
  );
}

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
      await loadMPRViews(next.folderPath);
    }
  },
  { immediate: true }
);

// 加载 MPR 视图（ROI 编辑：正交切片）
async function loadMPRViews(folderPath: string) {
  // 防止重复加载
  if (isLoading) return;

  isLoading = true;

  try {
    console.log("[ROI编辑] 加载 MPR 视图:", folderPath);

    // 保存当前文件夹路径
    currentFolderPath.value = folderPath;

    // 停止旧的渲染循环
    await stopRenderLoop();

    // 清理旧会话
    if (sessionId.value) {
      await window.visualizationApi.destroyMPR(sessionId.value);
      sessionId.value = "";
      isLoaded.value = false;
    }

    // 创建新会话 ID（使用 roi_ 前缀区分）
    sessionId.value = `roi_${Date.now()}_${Math.random()
      .toString(36)
      .substr(2, 9)}`;

    // 创建 MPR 视图（4个窗口：axial, coronal, sagittal, 3d）
    const result = await window.visualizationApi.createMPR(
      sessionId.value,
      folderPath
    );

    if (!result.success) {
      throw new Error(result.error || "创建 MPR 视图失败");
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
      }
    }

    // 获取体数据直方图（用于阈值分割）
    try {
      const histogram = await window.visualizationApi.getVolumeHistogram(
        sessionId.value
      );
      if (histogram && histogram.data && histogram.data.length > 0) {
        histogramData.value = histogram.data;
        histogramMinValue.value = histogram.minValue ?? -1024;
        histogramMaxValue.value = histogram.maxValue ?? 3071;
        rigMarkMin.value = histogramMinValue.value;
        rigMarkMax.value = histogramMaxValue.value;
        console.log(
          `[ROI] 直方图加载成功: 范围 ${histogramMinValue.value} ~ ${histogramMaxValue.value}, 数据点 ${histogramData.value.length}`
        );
      }
    } catch (error) {
      console.warn("[ROI] 获取直方图失败:", error);
    }

    // 嵌入原生窗口到 div 中（Win32 child HWND，宿主消息循环驱动）
    if (
      (result.windowIdAxial || result.hwndAxial) &&
      (result.windowIdCoronal || result.hwndCoronal) &&
      (result.windowIdSagittal || result.hwndSagittal) &&
      (result.windowId3D || result.hwnd3D)
    ) {
      nativeWinAxialId.value = result.windowIdAxial ?? String(result.hwndAxial);
      nativeWinCoronalId.value =
        result.windowIdCoronal ?? String(result.hwndCoronal);
      nativeWinSagittalId.value =
        result.windowIdSagittal ?? String(result.hwndSagittal);
      nativeWin3dId.value = result.windowId3D ?? String(result.hwnd3D);

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

      await embedWindow(view1Ref.value, nativeWinAxialId.value);
      await embedWindow(view2Ref.value, nativeWinCoronalId.value);
      await embedWindow(view3Ref.value, nativeWinSagittalId.value);
      await embedWindow(view4Ref.value, nativeWin3dId.value);
    }

    isLoaded.value = true;
    console.log("[ROI编辑] APR 视图创建成功");

    // 打开 ROI/MPR 时优先使用 DICOM 默认窗宽窗位
    try {
      const ww = props.panelData?.windowWidth;
      const wl = props.panelData?.windowLevel;
      if (typeof ww === "number" && typeof wl === "number" && ww > 0) {
        await window.visualizationApi.setWindowLevel(sessionId.value, ww, wl);
      } else {
        const st = await window.visualizationApi.getWindowLevel(
          sessionId.value
        );
        const nativeWw = st?.success ? st.windowWidth : undefined;
        const nativeWl = st?.success ? st.windowLevel : undefined;
        if (
          !(
            typeof nativeWw === "number" &&
            nativeWw > 0 &&
            typeof nativeWl === "number"
          )
        ) {
          await window.visualizationApi.setWindowLevel(
            sessionId.value,
            FALLBACK_WINDOW_WIDTH,
            FALLBACK_WINDOW_LEVEL
          );
        }
      }
    } catch (error) {
      console.warn("[ROI编辑] 应用窗宽窗位失败:", error);
    }

    // 正交切片 3D 显示开关（默认开启）
    try {
      await window.visualizationApi.set3DOrthogonalMode(
        getWin3dId(),
        fixedZeroRotation.value
      );
    } catch (error) {
      console.error("[ROI编辑] 设置3D正交模式失败:", error);
    }

    // App Tab 切换会 hideAllWindows()，native 会 SW_HIDE + EnableWindow(FALSE)
    // 必须在 embed/layout 完成后显式 show/enable，否则会出现白屏且无鼠标消息。
    try {
      await updateNativeWindowLayouts();
      await window.visualizationApi.showAllWindows?.();
    } catch (error) {
      console.warn("[ROI编辑] showAllWindows/layout sync failed:", error);
    }

    // 启动固定帧率渲染循环（60fps）
    await window.visualizationApi.startRenderLoop(60);
  } catch (error: any) {
    console.error("[ROI编辑] 加载APR失败:", error);
  } finally {
    isLoading = false;
  }
}

onMounted(async () => {
  // 组件首次挂载时也尝试恢复窗口（避免从其它 Tab 切回时残留 hidden 状态）
  try {
    if (window.visualizationApi?.showAllWindows) {
      await window.visualizationApi.showAllWindows();
    }
  } catch {
    // ignore
  }
});

// 停止渲染循环
async function stopRenderLoop() {
  if (!window.visualizationApi?.stopRenderLoop) return;
  try {
    await window.visualizationApi.stopRenderLoop();
  } catch (error) {
    console.error("[ROI编辑] 停止渲染循环失败:", error);
  }
}

// 统一计算相对于 Electron WebContents 客户区的坐标（CSS px / DIP）
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
      console.error("[ROI编辑] setCrosshairVisible失败:", error);
    }
  }

  if (showCrosshair.value) {
    // 切换到定位线工具
    currentToolType.value = 0;
    await setAllWindowsToolType(0);
  }
}

// 切换测量工具（1=Line, 2=Angle, 3=Rect, 4=Circle, 5=Bezier）
async function setMeasurementTool(toolType: number) {
  // 关闭定位线
  showCrosshair.value = false;
  if (sessionId.value) {
    try {
      await window.visualizationApi.setCrosshairVisible(sessionId.value, false);
    } catch (error) {
      console.error("[ROI编辑] setCrosshairVisible失败:", error);
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
    "任意曲线",
  ];
  console.log("[ROI编辑] 切换工具:", toolNames[toolType] || "未知工具");
}

// 为所有窗口设置工具类型（避免死锁：使用Promise.all并行执行）
async function setAllWindowsToolType(toolType: number) {
  if (!sessionId.value) return;

  const windowIds = [
    nativeWinAxialId.value || `${sessionId.value}_axial`,
    nativeWinCoronalId.value || `${sessionId.value}_coronal`,
    nativeWinSagittalId.value || `${sessionId.value}_sagittal`,
    getWin3dId(),
  ].filter(Boolean);

  try {
    await Promise.all(
      windowIds.map((id) =>
        window.visualizationApi.setWindowToolType(id, toolType)
      )
    );
  } catch (error) {
    console.error("[ROI编辑] 设置工具类型失败:", error);
  }
}

// ROI 3D：切换是否显示“正交切片 3D”（三平面）
async function toggleOrthogonal3D() {
  fixedZeroRotation.value = !fixedZeroRotation.value;
  if (!sessionId.value || !isLoaded.value) return;
  try {
    await window.visualizationApi.set3DOrthogonalMode(
      getWin3dId(),
      fixedZeroRotation.value
    );
  } catch (error) {
    console.error("[ROI编辑] 切换3D正交模式失败:", error);
  }
}

// 根据当前布局更新所有原生窗口位置与大小
async function updateNativeWindowLayouts() {
  if (!isLoaded.value || !sessionId.value) return;

  const isVisible = (id: string) => {
    if (layoutChoice.value === "quad") return true;
    if (layoutChoice.value === "view1") return id.endsWith("_axial");
    if (layoutChoice.value === "view2") return id.endsWith("_coronal");
    if (layoutChoice.value === "view3") return id.endsWith("_sagittal");
    if (layoutChoice.value === "view4")
      return id === getWin3dId() || id.endsWith("_3d") || id.endsWith("_roi3d");
    return true;
  };

  const views = [
    { ref: view1Ref, id: nativeWinAxialId.value || `${sessionId.value}_axial` },
    {
      ref: view2Ref,
      id: nativeWinCoronalId.value || `${sessionId.value}_coronal`,
    },
    {
      ref: view3Ref,
      id: nativeWinSagittalId.value || `${sessionId.value}_sagittal`,
    },
    { ref: view4Ref, id: getWin3dId() },
  ];

  await Promise.all(
    views.map(async ({ ref, id }) => {
      if (!isVisible(id)) {
        await window.visualizationApi.resizeWindow(id, -10000, -10000, 10, 10);
        return;
      }

      const metrics = computeRelativeRect(ref.value);
      if (!metrics) return;
      const { x, y, width, height } = metrics;
      await window.visualizationApi.resizeWindow(id, x, y, width, height);
    })
  );
}

watch(layoutChoice, async () => {
  await nextTick();
  await updateNativeWindowLayouts();
});

// 嵌入原生窗口到 div 中（使用 Win32 API SetParent）
async function embedWindow(container: HTMLDivElement, windowId: string) {
  const metrics = computeRelativeRect(container);
  if (!metrics) return;

  const { x, y, width, height } = metrics;

  try {
    await window.visualizationApi.embedWindow(windowId, x, y, width, height);
  } catch (error) {
    console.error("[ROI编辑] 窗口嵌入失败:", error);
  }
}

// 鼠标事件处理（转发到GLFW窗口，避免事件被阻止）
function handleMouseDown(event: MouseEvent) {
  event.stopPropagation();
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

onMounted(() => {
  if (props.panelData) {
    loadMPRViews(props.panelData.folderPath);
  }

  // 监听独立 dialog 窗口的返回结果
  if (window.nativeBridge?.dialog?.onResult) {
    window.nativeBridge.dialog.onResult(async (result: any) => {
      console.log("[Dialog Result]", result);
      if (
        result?.sessionId &&
        sessionId.value &&
        result.sessionId !== sessionId.value
      ) {
        return;
      }

      if (result?.dialogType === "rigmark") {
        if (result.action === "apply" && result.data) {
          await applyRigMarkFromDialog(result.data);
        } else if (result.action === "cancel") {
          await closeRigMarkDialog();
        }
        return;
      }

      if (result?.dialogType === "config3d") {
        if (result.action === "apply") {
          const enabled = !!result?.data?.vramOptimized;
          const windowId = getWin3dId() || null;
          console.log("[config3d] Apply VRAM optimized:", {
            enabled,
            windowId,
          });
          if (
            windowId &&
            (window as any).visualizationApi?.set3DVramOptimized
          ) {
            await (window as any).visualizationApi.set3DVramOptimized(
              windowId,
              enabled
            );
          }
        }
        return;
      }

      // 处理形态学操作结果
      if (result?.dialogType === "morphology") {
        if (result.action === "apply" && result.data) {
          await applyMorphologyFromDialog(result.data);
        }
        return;
      }

      // 处理布尔运算结果
      if (result?.dialogType === "boolean") {
        if (result.action === "apply" && result.data) {
          await applyBooleanFromDialog(result.data);
        }
        return;
      }
    });
  }

  // 监听对话框关闭事件，重新激活 MPR 窗口
  const dialogClosedListener = () => {
    console.log("[Dialog Closed] Reactivating MPR windows");
    if (isLoaded.value && sessionId.value) {
      // 多重策略确保窗口重新激活：
      // 1. 立即更新布局
      updateNativeWindowLayouts();

      // 2. 延迟后再次更新并触发鼠标事件
      setTimeout(async () => {
        await updateNativeWindowLayouts();

        // 3. 主动点击第一个视图区域来激活原生窗口
        if (view1Ref.value) {
          view1Ref.value.focus();

          // 触发鼠标进入事件
          const mouseEnterEvent = new MouseEvent("mouseenter", {
            bubbles: true,
            cancelable: true,
            view: window,
          });
          view1Ref.value.dispatchEvent(mouseEnterEvent);

          console.log(
            "[Dialog Closed] Reactivated windows and triggered focus"
          );
        }
      }, 200);
    }
  };
  window.ipcRenderer?.on("dialog:closed", dialogClosedListener);

  // 监听窗口大小变化，调整嵌入的Win32窗口
  let resizeTimer: NodeJS.Timeout | null = null;
  const resizeObserver = new ResizeObserver(() => {
    if (!isLoaded.value || !sessionId.value) return;

    // 防抖：避免频繁resize
    if (resizeTimer) clearTimeout(resizeTimer);

    resizeTimer = setTimeout(() => {
      updateNativeWindowLayouts();
    }, 100); // 100ms 防抖
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

  // 保存observer以便cleanup
  (onUnmounted as any)(() => {
    resizeObserver.disconnect();
    window.ipcRenderer?.off(
      "electron-window-bounds-changed",
      boundsChangeListener
    );
    window.ipcRenderer?.off("dialog:closed", dialogClosedListener);
    // 清理 dialog 监听器
    if (window.nativeBridge?.dialog?.offResult) {
      window.nativeBridge.dialog.offResult();
    }
  });
});

// Handler functions for dialogs and deletion
async function deleteSelectedRegion2D() {
  if (!selectedRegion2D.value) {
    ElMessage.warning("请先选择要删除的2D区域");
    return;
  }

  const index = region2DList.value.indexOf(selectedRegion2D.value);
  if (index === -1) return;

  const maskId = selectedRegion2D.value.maskId;

  try {
    // 调用C++ API删除mask vector中的项
    if (sessionId.value && maskId !== undefined) {
      await (window as any).visualizationApi?.deleteMask?.(
        sessionId.value,
        maskId
      );
    }

    // 从列表中删除
    region2DList.value.splice(index, 1);
    selectedRegion2D.value = null;

    ElMessage.success("2D区域删除成功");
  } catch (error) {
    console.error("[ROI] 删除mask失败:", error);
    ElMessage.error("删除mask失败");
  }
}

async function deleteSelectedRegion3D() {
  if (!selectedRegion3D.value) {
    ElMessage.warning("请先选择要删除的3D区域");
    return;
  }

  const index = region3DList.value.indexOf(selectedRegion3D.value);
  if (index === -1) return;

  try {
    // TODO: 调用C++ API删除3D mask

    region3DList.value.splice(index, 1);
    selectedRegion3D.value = null;
    ElMessage.success("3D区域删除成功");
  } catch (error) {
    console.error("[ROI] 删除3D mask失败:", error);
    ElMessage.error("删除3D mask失败");
  }
}

async function saveMask() {
  if (!maskSaveName.value.trim()) {
    ElMessage.warning("请输入mask名称");
    return;
  }

  if (!sessionId.value || !currentFolderPath.value) {
    ElMessage.error("未加载图像数据");
    return;
  }

  try {
    // 调用C++ API保存mask到文件夹
    // masks文件夹会在C++侧创建
    const result = await (window as any).visualizationApi?.saveMasks?.(
      sessionId.value,
      currentFolderPath.value,
      maskSaveName.value.trim()
    );

    if (result?.success) {
      ElMessage.success(`Mask已保存: ${maskSaveName.value}`);
    } else {
      throw new Error(result?.error || "保存失败");
    }
  } catch (error: any) {
    console.error("[ROI] 保存mask失败:", error);
    ElMessage.error(`保存mask失败: ${error.message}`);
  }
}

// 加载mask
async function loadMask() {
  if (!currentFolderPath.value) {
    ElMessage.error("未加载图像数据");
    return;
  }

  try {
    // 调用C++ API打开文件选择对话框
    const result = await (window as any).visualizationApi?.loadMasks?.(
      sessionId.value,
      currentFolderPath.value
    );

    if (result?.success && result.masks && result.masks.length > 0) {
      // 将加载的masks添加到列表
      for (const mask of result.masks) {
        region2DList.value.push({
          maskId: mask.maskId,
          color: mask.color,
          visible: mask.visible ?? true,
          min: mask.minThreshold,
          max: mask.maxThreshold,
          name: mask.name,
        });
      }

      ElMessage.success(`已加载 ${result.masks.length} 个mask`);
    } else if (result?.cancelled) {
      // 用户取消选择
      return;
    } else {
      throw new Error(result?.error || "加载失败");
    }
  } catch (error: any) {
    console.error("[ROI] 加载mask失败:", error);
    ElMessage.error(`加载mask失败: ${error.message}`);
  }
}

// 实时预览mask（阈值调节时调用）
async function updateMaskPreview() {
  if (!sessionId.value) return;

  try {
    // 调用C++ API创建临时预览mask
    await window.visualizationApi.updatePreviewMask(
      sessionId.value,
      rigMarkMin.value,
      rigMarkMax.value,
      rigMarkColor.value
    );
  } catch (error) {
    console.error("[ROI] 更新mask预览失败:", error);
  }
}

// 绘制直方图
function drawHistogram() {
  if (!histogramCanvas.value || histogramData.value.length === 0) return;

  const canvas = histogramCanvas.value;
  const ctx = canvas.getContext("2d");
  if (!ctx) return;

  const width = canvas.width;
  const height = canvas.height;
  const padding = 40;
  const chartWidth = width - padding * 2;
  const chartHeight = height - padding * 2;

  // 清空画布
  ctx.clearRect(0, 0, width, height);
  ctx.fillStyle = "#f5f7fa";
  ctx.fillRect(0, 0, width, height);

  // 计算对数值（避免log(0)）
  const logData = histogramData.value.map((val) =>
    val > 0 ? Math.log10(val + 1) : 0
  );
  const maxLogValue = Math.max(...logData);

  if (maxLogValue === 0) return;

  // 绘制直方图柱子
  const barWidth = chartWidth / histogramData.value.length;

  for (let i = 0; i < histogramData.value.length; i++) {
    const normalizedHeight = (logData[i] / maxLogValue) * chartHeight;
    const x = padding + i * barWidth;
    const y = padding + chartHeight - normalizedHeight;

    // 计算当前bin对应的CT值
    const ctValue =
      histogramMinValue.value +
      (i / histogramData.value.length) *
        (histogramMaxValue.value - histogramMinValue.value);

    // 判断是否在选中范围内
    const inRange = ctValue >= rigMarkMin.value && ctValue <= rigMarkMax.value;

    // 设置颜色
    if (inRange) {
      ctx.fillStyle = rigMarkColor.value + "80"; // 半透明
    } else {
      ctx.fillStyle = "#909399";
    }

    ctx.fillRect(x, y, barWidth, normalizedHeight);
  }

  // 绘制坐标轴
  ctx.strokeStyle = "#606266";
  ctx.lineWidth = 2;
  ctx.beginPath();
  ctx.moveTo(padding, padding);
  ctx.lineTo(padding, padding + chartHeight);
  ctx.lineTo(padding + chartWidth, padding + chartHeight);
  ctx.stroke();

  // 绘制标签
  ctx.fillStyle = "#606266";
  ctx.font = "12px Arial";
  ctx.textAlign = "center";

  // X轴标签
  ctx.fillText(String(histogramMinValue.value), padding, height - 10);
  ctx.fillText(
    String(histogramMaxValue.value),
    padding + chartWidth,
    height - 10
  );
  ctx.fillText("CT值", width / 2, height - 10);

  // Y轴标签
  ctx.save();
  ctx.translate(15, height / 2);
  ctx.rotate(-Math.PI / 2);
  ctx.fillText("像素数量 (log)", 0, 0);
  ctx.restore();
}

// 打开RigMark对话框时设置预览颜色并绘制直方图
async function openRigMarkDialog() {
  if (!sessionId.value) {
    ElMessage.error("未加载APR视图");
    return;
  }

  console.log(
    "[openRigMarkDialog] Opening dialog with sessionId:",
    sessionId.value
  );

  try {
    // 先获取直方图数据
    const histogramResult = await window.visualizationApi.getVolumeHistogram(
      sessionId.value
    );
    console.log("[openRigMarkDialog] Histogram result:", histogramResult);

    if (!histogramResult || !histogramResult.data) {
      ElMessage.error("获取直方图数据失败");
      return;
    }

    // 使用独立的 BrowserWindow，传入直方图数据
    if (window.nativeBridge?.dialog?.open) {
      const defaultColor = pickMaskColor16(region2DList.value.length);
      const result = await window.nativeBridge.dialog.open("rigmark", {
        sessionId: sessionId.value,
        dialogType: "rigmark",
        histogramData: histogramResult.data,
        minValue: histogramResult.minValue,
        maxValue: histogramResult.maxValue,
        defaultColor,
      });

      console.log("[openRigMarkDialog] Dialog open result:", result);

      if (!result.success) {
        ElMessage.error(result.error || "打开对话框失败");
      }
    } else {
      ElMessage.error("Dialog API 未初始化");
      console.error("[openRigMarkDialog] Dialog API not available");
    }
  } catch (error: any) {
    console.error("[openRigMarkDialog] Error:", error);
    ElMessage.error(`打开对话框失败: ${error.message}`);
  }
}

// 关闭RigMark对话框并清除预览（由独立窗口调用）
async function closeRigMarkDialog() {
  if (sessionId.value && (window as any).visualizationApi?.clearPreviewMask) {
    try {
      await (window as any).visualizationApi.clearPreviewMask(sessionId.value);
    } catch (error) {
      console.error("[ROI] 清除预览mask失败:", error);
    }
  }
}

// 从独立窗口接收应用结果
async function applyRigMarkFromDialog(data: {
  min: number;
  max: number;
  color: string;
}) {
  if (!sessionId.value) {
    ElMessage.error("未加载APR视图");
    return;
  }

  try {
    const defaultColor = pickMaskColor16(region2DList.value.length);
    const color = data?.color || defaultColor;

    // 调用C++ DLL创建mask并添加到mask vector
    const result = await window.visualizationApi.createMaskFromThreshold(
      sessionId.value,
      data.min,
      data.max,
      color,
      `Mask_${region2DList.value.length + 1}`
    );

    if (result.success && result.maskId !== undefined) {
      // 添加到2D区域列表
      const newRegion = {
        maskId: result.maskId,
        color,
        visible: true,
        min: data.min,
        max: data.max,
      };
      region2DList.value.push(newRegion);

      // 清除预览mask
      await window.visualizationApi.clearPreviewMask(sessionId.value);

      ElMessage.success("阈值分割应用成功");
    } else {
      throw new Error(result.error || "创建mask失败");
    }
  } catch (error: any) {
    console.error("[ROI] 阈值分割失败:", error);
    ElMessage.error(`阈值分割失败: ${error.message}`);
  }
}

// 从形态学对话框接收结果并应用
async function applyMorphologyFromDialog(data: {
  operation: string;
  kernel: number;
  iterations: number;
}) {
  if (!sessionId.value) {
    ElMessage.error("未加载APR视图");
    return;
  }

  const selectedMask = selectedRegion2D.value;
  if (!selectedMask) {
    ElMessage.warning("请先选择一个掩膜");
    return;
  }

  try {
    // 映射操作类型到枚举值
    const opMap: Record<string, number> = {
      dilate: 0, // MORPH_DILATE
      erode: 1, // MORPH_ERODE
      open: 2, // MORPH_OPEN
      close: 3, // MORPH_CLOSE
    };
    const opCode = opMap[data.operation] ?? 0;

    // kernel: 3/5/7/9(对话框语义为核尺寸)，iterations: 重复次数
    const kernelSize = Math.max(1, Number(data.kernel) || 3);
    const iterations = Math.max(1, Number(data.iterations) || 1);

    // 固定 kernel 选项: 3/5/7/9。
    // 对 box 核的 dilate/erode：重复迭代等价于一次更大半径（Minkowski sum）。
    // 对 open/close：不做合并，保持逐次执行以保证结果一致。
    const isDilateOrErode =
      data.operation === "dilate" || data.operation === "erode";

    if (isDilateOrErode && iterations > 1) {
      const baseRadius = Math.floor((kernelSize - 1) / 2);
      const effectiveRadius = baseRadius * iterations;
      const effectiveKernelSize = effectiveRadius * 2 + 1;

      const result = await window.visualizationApi.maskMorphology3D(
        sessionId.value,
        selectedMask.maskId,
        opCode,
        effectiveKernelSize
      );
      if (!result?.success) {
        throw new Error(result?.error || "形态学操作失败");
      }
    } else {
      for (let i = 0; i < iterations; i++) {
        const result = await window.visualizationApi.maskMorphology3D(
          sessionId.value,
          selectedMask.maskId,
          opCode,
          kernelSize
        );
        if (!result?.success) {
          throw new Error(result?.error || "形态学操作失败");
        }
      }
    }

    ElMessage.success(`形态学操作 (${data.operation}) 应用成功`);
  } catch (error: any) {
    console.error("[ROI] 形态学操作失败:", error);
    ElMessage.error(`形态学操作失败: ${error.message}`);
  }
}

// 从布尔运算对话框接收结果并应用
async function applyBooleanFromDialog(data: {
  operation: string;
  maskIdA: number;
  maskIdB: number;
  name?: string;
  color?: string;
}) {
  if (!sessionId.value) {
    ElMessage.error("未加载APR视图");
    return;
  }

  try {
    // 映射操作类型到枚举值
    const opMap: Record<string, number> = {
      union: 0, // BOOL_UNION
      intersect: 1, // BOOL_INTERSECT
      subtract: 2, // BOOL_SUBTRACT
    };
    const opCode = opMap[data.operation] ?? 0;

    const defaultColor = pickMaskColor16(region2DList.value.length);
    const name = data.name || `Boolean_${region2DList.value.length + 1}`;
    const color = data.color || defaultColor;

    // 调用 Native API
    const result = await window.visualizationApi.maskBoolean(
      sessionId.value,
      data.maskIdA,
      data.maskIdB,
      opCode,
      name,
      color
    );

    if (result.success && result.newMaskId !== undefined) {
      // 添加新mask到列表
      const newRegion = {
        maskId: result.newMaskId,
        color,
        visible: true,
        min: 0,
        max: 0,
      };
      region2DList.value.push(newRegion);

      ElMessage.success(`布尔运算 (${data.operation}) 应用成功`);
    } else {
      throw new Error(result.error || "布尔运算失败");
    }
  } catch (error: any) {
    console.error("[ROI] 布尔运算失败:", error);
    ElMessage.error(`布尔运算失败: ${error.message}`);
  }
}

// 打开 ROI 编辑对话框（非模态）
async function openRoiEditDialog() {
  if (!sessionId.value) {
    ElMessage.error("请先加载APR视图");
    return;
  }

  console.log(
    "[openRoiEditDialog] Opening dialog with sessionId:",
    sessionId.value
  );

  try {
    if (window.nativeBridge?.dialog?.open) {
      const result = await window.nativeBridge.dialog.open("roiedit", {
        sessionId: sessionId.value,
      });

      console.log("[openRoiEditDialog] Dialog open result:", result);

      if (!result.success) {
        ElMessage.error(result.error || "打开对话框失败");
      }
    } else {
      ElMessage.error("Dialog API 未初始化");
      console.error("[openRoiEditDialog] Dialog API not available");
    }
  } catch (error: any) {
    console.error("[openRoiEditDialog] Error:", error);
    ElMessage.error(`打开对话框失败: ${error.message}`);
  }
}

// 打开形态学操作对话框
async function openMorphologyDialog() {
  if (!sessionId.value) {
    ElMessage.error("请先加载APR视图");
    return;
  }

  try {
    if (window.nativeBridge?.dialog?.open) {
      await window.nativeBridge.dialog.open("morphology", {
        sessionId: sessionId.value,
      });
    }
  } catch (error: any) {
    console.error("[openMorphologyDialog] Error:", error);
    ElMessage.error(`打开对话框失败: ${error.message}`);
  }
}

// 打开布尔操作对话框
async function openBooleanDialog() {
  if (!sessionId.value) {
    ElMessage.error("请先加载APR视图");
    return;
  }

  try {
    if (window.nativeBridge?.dialog?.open) {
      await window.nativeBridge.dialog.open("boolean", {
        sessionId: sessionId.value,
        maskList: region2DList.value,
      });
    }
  } catch (error: any) {
    console.error("[openBooleanDialog] Error:", error);
    ElMessage.error(`打开对话框失败: ${error.message}`);
  }
}

// 打开3D配置对话框
async function openConfig3DDialog() {
  if (!sessionId.value) {
    ElMessage.error("请先加载APR视图");
    return;
  }

  try {
    if (window.nativeBridge?.dialog?.open) {
      await window.nativeBridge.dialog.open("config3d", {
        sessionId: sessionId.value,
        maskList: region2DList.value,
      });
    }
  } catch (error: any) {
    console.error("[openConfig3DDialog] Error:", error);
    ElMessage.error(`打开对话框失败: ${error.message}`);
  }
}

// 打开统计信息对话框
async function openStatsDialog() {
  const mask = selectedRegion2D.value || selectedRegion3D.value;
  if (!mask) {
    ElMessage.warning("请先选择一个掩膜");
    return;
  }

  try {
    if (window.nativeBridge?.dialog?.open) {
      await window.nativeBridge.dialog.open("stats", {
        sessionId: sessionId.value,
        maskId: selectedRegion2D.value?.maskId,
        maskData: {
          ...mask,
          is3D: !!selectedRegion3D.value,
        },
      });
    }
  } catch (error: any) {
    console.error("[openStatsDialog] Error:", error);
    ElMessage.error(`打开对话框失败: ${error.message}`);
  }
}

// 打开颜色选择对话框（2D）
async function openColorPicker2D() {
  if (!selectedRegion2D.value) {
    ElMessage.warning("请先选择一个2D掩膜");
    return;
  }

  try {
    if (window.nativeBridge?.dialog?.open) {
      await window.nativeBridge.dialog.open("colorpicker", {
        currentColor: selectedRegion2D.value.color,
        targetType: "2d",
        targetIndex: region2DList.value.indexOf(selectedRegion2D.value),
      });
    }
  } catch (error: any) {
    console.error("[openColorPicker2D] Error:", error);
    ElMessage.error(`打开对话框失败: ${error.message}`);
  }
}

// 打开颜色选择对话框（3D）
async function openColorPicker3D() {
  if (!selectedRegion3D.value) {
    ElMessage.warning("请先选择一个3D掩膜");
    return;
  }

  try {
    if (window.nativeBridge?.dialog?.open) {
      await window.nativeBridge.dialog.open("colorpicker", {
        currentColor: selectedRegion3D.value.color,
        targetType: "3d",
        targetIndex: region3DList.value.indexOf(selectedRegion3D.value),
      });
    }
  } catch (error: any) {
    console.error("[openColorPicker3D] Error:", error);
    ElMessage.error(`打开对话框失败: ${error.message}`);
  }
}

// 打开透明度对话框
async function openOpacityDialog() {
  if (!selectedRegion3D.value) {
    ElMessage.warning("请先选择一个3D掩膜");
    return;
  }

  try {
    if (window.nativeBridge?.dialog?.open) {
      await window.nativeBridge.dialog.open("opacity", {
        opacity: selectedRegion3D.value.opacity,
        color: selectedRegion3D.value.color,
        targetIndex: region3DList.value.indexOf(selectedRegion3D.value),
      });
    }
  } catch (error: any) {
    console.error("[openOpacityDialog] Error:", error);
    ElMessage.error(`打开对话框失败: ${error.message}`);
  }
}

// 打开保存Mask对话框
async function openSaveMaskDialog() {
  if (!sessionId.value) {
    ElMessage.error("请先加载APR视图");
    return;
  }

  try {
    if (window.nativeBridge?.dialog?.open) {
      await window.nativeBridge.dialog.open("savemask", {
        sessionId: sessionId.value,
      });
    }
  } catch (error: any) {
    console.error("[openSaveMaskDialog] Error:", error);
    ElMessage.error(`打开对话框失败: ${error.message}`);
  }
}

// 打开截图对话框
async function openScreenshotDialog() {
  if (!sessionId.value) {
    ElMessage.error("请先加载APR视图");
    return;
  }

  try {
    if (window.nativeBridge?.dialog?.open) {
      await window.nativeBridge.dialog.open("screenshot", {
        sessionId: sessionId.value,
      });
    }
  } catch (error: any) {
    console.error("[openScreenshotDialog] Error:", error);
    ElMessage.error(`打开对话框失败: ${error.message}`);
  }
}

function applyMorphology() {
  // TODO: Call C++ DLL mask API
  ElMessage.success(`形态学操作应用成功: ${morphologyOperation.value}`);
}

function applyBoolean() {
  // TODO: Call C++ DLL mask API
  ElMessage.success(`布尔操作应用成功: ${booleanOperation.value}`);
}

function invertMask() {
  if (!selectedRegion2D.value) {
    ElMessage.warning("请先选择要反色的2D掩膜");
    return;
  }
  // TODO: Call C++ DLL mask API to invert mask
  ElMessage.success("掩膜反色成功");
}

function connectedComponents() {
  if (!selectedRegion2D.value) {
    ElMessage.warning("请先选择要处理的2D掩膜");
    return;
  }
  // TODO: Call C++ DLL mask API to find connected components
  ElMessage.success("连通域分析完成");
}

function captureScreenshot() {
  if (screenshotSelection.value.length === 0) {
    ElMessage.warning("请至少选择一个视图进行截图");
    return;
  }

  // TODO: Call C++ DLL API to capture selected views
  const viewNames = screenshotSelection.value
    .map((view) => {
      switch (view) {
        case "axial":
          return "轴位";
        case "coronal":
          return "冠状位";
        case "sagittal":
          return "矢状位";
        case "3d":
          return "3D";
        case "mpr+3d":
          return "MPR+3D";
        default:
          return view;
      }
    })
    .join("、");

  ElMessage.success(`截图已保存: ${viewNames}`);
}

function apply3DConfig() {
  if (!selected2DMaskFor3D.value) {
    ElMessage.warning("请选择一个2D掩膜");
    return;
  }

  // 找到选中的2D mask
  const mask2DIndex = parseInt(
    selected2DMaskFor3D.value.replace("region_", "")
  );
  const mask2D = region2DList.value[mask2DIndex];

  if (!mask2D) {
    ElMessage.error("选中的2D掩膜不存在");
    return;
  }

  // TODO: Call C++ DLL mask API to create 3D mask from 2D
  const newRegion = {
    color: mask2D.color,
    visible: true,
    opacity: 80,
    sourceMask2D: selected2DMaskFor3D.value,
  };
  region3DList.value.push(newRegion);
  selected2DMaskFor3D.value = "";
  ElMessage.success("3D掩膜创建成功");
}

// Keyboard event handler for Delete key
function handleTableKeydown(event: KeyboardEvent) {
  if (event.key === "Delete" || event.key === "Del") {
    if (selectedRegion2D.value) {
      deleteSelectedRegion2D();
    } else if (selectedRegion3D.value) {
      deleteSelectedRegion3D();
    }
  }
}

// 清理资源
onUnmounted(() => {
  void stopRenderLoop();

  if (sessionId.value) {
    window.visualizationApi
      .destroyMPR(sessionId.value)
      .catch((error: unknown) =>
        console.error("[ROI编辑] 清理资源失败:", error)
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

        <!-- 显示功能区 -->
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
              :type="fixedZeroRotation ? 'success' : 'primary'"
              @click="toggleOrthogonal3D"
            >
              显示正交切片
            </ElButton>
          </div>
        </div>

        <!-- 2D Region Control Plane -->
        <div class="section">
          <div class="section-title">2D 掩膜编辑区域</div>
          <ElTable
            :data="region2DList"
            size="small"
            border
            class="roi-table"
            highlight-current-row
            @current-change="(row) => (selectedRegion2D = row)"
            @keydown="handleTableKeydown"
            tabindex="0"
          >
            <ElTableColumn prop="color" label="颜色" width="80">
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
            <ElTableColumn prop="visible" label="显示" width="80">
              <template #default="scope">
                <ElSwitch v-model="scope.row.visible" size="small" />
              </template>
            </ElTableColumn>
            <ElTableColumn prop="min" label="最小值" />
            <ElTableColumn prop="max" label="最大值" />
          </ElTable>
          <div class="btn-row">
            <ElButton size="small" type="primary" @click="openRigMarkDialog"
              >添加</ElButton
            >
            <ElButton
              size="small"
              type="primary"
              @click="deleteSelectedRegion2D"
              >删除</ElButton
            >
            <ElButton size="small" type="primary" @click="openColorPicker2D"
              >颜色</ElButton
            >
            <ElButton size="small" type="primary" @click="openSaveMaskDialog"
              >保存</ElButton
            >
            <ElButton size="small" type="primary" @click="loadMask"
              >加载</ElButton
            >
            <ElButton size="small" type="primary" @click="openStatsDialog"
              >信息</ElButton
            >
          </div>
        </div>
        <!-- 2D 掩膜操作区域 -->
        <div class="section">
          <div class="section-title">2D 掩膜操作区域</div>
          <div class="btn-very-big-row">
            <ElButton size="large" type="primary" @click="openMorphologyDialog"
              >形态学操作</ElButton
            >
            <ElButton size="large" type="primary" @click="openBooleanDialog"
              >Boolean操作</ElButton
            >
            <ElButton size="large" type="primary" @click="invertMask"
              >反色</ElButton
            >
            <ElButton size="large" type="primary" @click="connectedComponents"
              >连通域</ElButton
            >
            <ElButton size="large" type="primary" @click="openRoiEditDialog"
              >ROI编辑</ElButton
            >
            <ElButton size="large" type="primary" @click="openScreenshotDialog"
              >截图</ElButton
            >
          </div>
        </div>

        <!-- 3D Region Control Plane -->
        <div class="section">
          <div class="section-title">3D 掩膜编辑区域</div>
          <ElTable
            :data="region3DList"
            size="small"
            border
            class="roi-table"
            highlight-current-row
            @current-change="(row) => (selectedRegion3D = row)"
            @keydown="handleTableKeydown"
            tabindex="0"
          >
            <ElTableColumn prop="color" label="颜色" width="80">
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
            <ElTableColumn prop="visible" label="显示" width="80">
              <template #default="scope">
                <ElSwitch v-model="scope.row.visible" size="small" />
              </template>
            </ElTableColumn>
            <ElTableColumn prop="opacity" label="透明度">
              <template #default="scope">
                <span>{{ scope.row.opacity }}%</span>
              </template>
            </ElTableColumn>
          </ElTable>
          <div class="btn-row">
            <ElButton size="small" type="primary" @click="openConfig3DDialog"
              >添加</ElButton
            >
            <ElButton
              size="small"
              type="primary"
              @click="deleteSelectedRegion3D"
              >删除</ElButton
            >
            <ElButton size="small" type="primary" @click="openColorPicker3D"
              >颜色</ElButton
            >
            <ElButton size="small" type="primary" @click="openOpacityDialog"
              >透明</ElButton
            >
          </div>
        </div>

        <!-- 测量工具区（新增）-->
        <div class="section">
          <div class="section-title">测量工具</div>
          <div class="btn-row">
            <ElTooltip content="直线测量" placement="top">
              <ElButton
                size="small"
                :type="currentToolType === 1 ? 'success' : 'primary'"
                :icon="EditPen"
                @click="setMeasurementTool(1)"
              >
                直线
              </ElButton>
            </ElTooltip>
            <ElTooltip content="角度测量" placement="top">
              <ElButton
                size="small"
                :type="currentToolType === 2 ? 'success' : 'primary'"
                :icon="Connection"
                @click="setMeasurementTool(2)"
              >
                角度
              </ElButton>
            </ElTooltip>
            <ElTooltip content="矩形测量" placement="top">
              <ElButton
                size="small"
                :type="currentToolType === 3 ? 'success' : 'primary'"
                :icon="Crop"
                @click="setMeasurementTool(3)"
              >
                方形
              </ElButton>
            </ElTooltip>
            <ElTooltip content="圆形测量" placement="top">
              <ElButton
                size="small"
                :type="currentToolType === 4 ? 'success' : 'primary'"
                :icon="Plus"
                @click="setMeasurementTool(4)"
              >
                圆形
              </ElButton>
            </ElTooltip>
          </div>
          <div class="btn-row">
            <ElTooltip content="贝塞尔曲线" placement="top">
              <ElButton
                size="small"
                :type="currentToolType === 5 ? 'success' : 'primary'"
                :icon="EditPen"
                @click="setMeasurementTool(5)"
              >
                贝塞尔
              </ElButton>
            </ElTooltip>
            <ElTooltip content="任意曲线" placement="top">
              <ElButton
                size="small"
                :type="currentToolType === 6 ? 'success' : 'primary'"
                :icon="EditPen"
                @click="setMeasurementTool(6)"
              >
                任意曲线
              </ElButton>
            </ElTooltip>
          </div>
        </div>
      </ElCard>
    </aside>

    <!-- 右侧显示面板：2x2 网格，显示APR视图 -->
    <main class="right-panel">
      <div class="grid2x2" v-if="layoutChoice === 'quad'">
        <ElCard class="view-cell" shadow="hover">
          <div
            ref="view1Ref"
            class="view-container"
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
            @mousedown="handleMouseDown"
            @mousemove="handleMouseMove"
            @mouseup="handleMouseUp"
            @wheel="handleWheel"
          ></div>
          <div v-if="!isLoaded" class="view-placeholder">
            3D 视图（用于mask渲染）
          </div>
        </ElCard>
      </div>

      <div class="grid1x1" v-else>
        <ElCard
          class="view-cell"
          shadow="hover"
          v-if="layoutChoice === 'view1'"
        >
          <div
            ref="view1Ref"
            class="view-container"
            @mousedown="handleMouseDown"
            @mousemove="handleMouseMove"
            @mouseup="handleMouseUp"
            @wheel="handleWheel"
          ></div>
          <div v-if="!isLoaded" class="view-placeholder">轴位视图</div>
        </ElCard>

        <ElCard
          class="view-cell"
          shadow="hover"
          v-else-if="layoutChoice === 'view2'"
        >
          <div
            ref="view2Ref"
            class="view-container"
            @mousedown="handleMouseDown"
            @mousemove="handleMouseMove"
            @mouseup="handleMouseUp"
            @wheel="handleWheel"
          ></div>
          <div v-if="!isLoaded" class="view-placeholder">冠状位视图</div>
        </ElCard>

        <ElCard
          class="view-cell"
          shadow="hover"
          v-else-if="layoutChoice === 'view3'"
        >
          <div
            ref="view3Ref"
            class="view-container"
            @mousedown="handleMouseDown"
            @mousemove="handleMouseMove"
            @mouseup="handleMouseUp"
            @wheel="handleWheel"
          ></div>
          <div v-if="!isLoaded" class="view-placeholder">矢状位视图</div>
        </ElCard>

        <ElCard class="view-cell" shadow="hover" v-else>
          <div
            ref="view4Ref"
            class="view-container"
            @mousedown="handleMouseDown"
            @mousemove="handleMouseMove"
            @mouseup="handleMouseUp"
            @wheel="handleWheel"
          ></div>
          <div v-if="!isLoaded" class="view-placeholder">
            3D 视图（用于mask渲染）
          </div>
        </ElCard>
      </div>
    </main>
  </div>

  <!-- RigMark Dialog (阈值分割) -->
  <!-- 所有对话框已改为独立窗口，ElDialog 已移除 -->
</template>

<style scoped>
/* ROI/Region 控制表格采用浅色风格，参考console工程Record页面 */
/* ROI/Region 控制表格宽度增大，表头一行不换行 */
.roi-table {
  margin-bottom: 10px;
  border-radius: 10px;
  overflow: hidden;
  background: rgba(246, 250, 255, 0.98);
  box-shadow: 0 2px 8px rgba(0, 120, 160, 0.1);
  --el-table-bg-color: rgba(246, 250, 255, 0.98);
  --el-table-border-color: rgba(180, 210, 240, 0.32);
  min-width: 320px;
  max-width: 100%;
}
.roi-table :deep(.el-table__header th) {
  background: linear-gradient(180deg, #e6f2ff 0%, #d3e6fa 100%);
  color: #1a355a;
  font-size: 13px;
  font-weight: 600;
  border-bottom: none !important;
  border-top: none !important;
  border-left: none !important;
  border-right: none !important;
  letter-spacing: 0.2px;
  white-space: nowrap;
}
.roi-table :deep(.el-table__header th) {
  background: linear-gradient(180deg, #e6f2ff 0%, #d3e6fa 100%);
  color: #1a355a;
  font-size: 13px;
  font-weight: 600;
  border-bottom: none !important;
  border-top: none !important;
  border-left: none !important;
  border-right: none !important;
  letter-spacing: 0.2px;
}
.roi-table :deep(.el-table__row) {
  background: transparent;
}
.roi-table :deep(td) {
  border-bottom: 1px solid rgba(180, 210, 240, 0.18);
}
.roi-table :deep(.el-table__cell) {
  padding: 5px 10px;
  color: #1a355a;
  font-size: 13px;
}
/* 表格高亮行样式 */
.roi-table :deep(.el-table__body tr.current-row > td) {
  background-color: rgba(30, 200, 255, 0.25) !important;
  border-left: 3px solid #1ec8ff !important;
}
.roi-table :deep(.el-table__body tr:hover > td) {
  background-color: rgba(30, 200, 255, 0.12) !important;
}
.roi-table :deep(.el-switch) {
  --el-switch-on-color: #1ec8ff;
  --el-switch-off-color: #b6cbe0;
}

.btn-very-big-row .el-button {
  display: inline-block;
  margin-top: 10px;
  margin-left: 6px;
  width: calc(50% - 12px);
}

.control-card :deep(.el-checkbox) {
  margin-left: 8px;
  color: #b6eaff;
  font-size: 13px;
}

/* 强制 ElDialog 在最顶层，覆盖原生窗口 */
:deep(.el-dialog__wrapper) {
  z-index: 999999 !important;
}

:deep(.el-overlay) {
  z-index: 999998 !important;
}
</style>
