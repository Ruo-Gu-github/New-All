<script setup lang="ts">
import "element-plus/dist/index.css";
import { nextTick, onMounted, onUnmounted, ref, watch } from "vue";
import {
  ElButton,
  ElCard,
  ElSwitch,
  ElTable,
  ElTableColumn,
} from "element-plus";

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
  } | null;
  // Increment to force reload even when folderPath is unchanged.
  reloadKey?: number;
}>();

// Selected row
const selectedRegion3D = ref<any>(null);

// 2D mask 列表（用于 3D mask 的创建配置对话框）
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

// 3D Region 控制表格数据（与 ROI 编辑一致）
const region3DList = ref<
  Array<{
    color: string;
    visible: boolean;
    opacity: number;
    sourceMask2D?: string;
  }>
>([]);

// 3D 视图容器
const view3DRef = ref<HTMLDivElement | null>(null);
const isLoaded = ref(false);
const sessionId = ref<string>("");
const windowId3D = ref<string>("");
let isLoading = false;

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
      await loadRecon3DView(next.folderPath);
    }
  },
  { immediate: true }
);

async function stopRenderLoop() {
  if (!window.visualizationApi?.stopRenderLoop) return;
  try {
    await window.visualizationApi.stopRenderLoop();
  } catch (error) {
    console.error("[3维重建] 停止渲染循环失败:", error);
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

async function embed3DWindow() {
  if (!view3DRef.value || !windowId3D.value) return;
  const metrics = computeRelativeRect(view3DRef.value);
  if (!metrics) return;

  const { x, y, width, height } = metrics;
  try {
    await window.visualizationApi.embedWindow(
      windowId3D.value,
      x,
      y,
      width,
      height
    );
  } catch (error) {
    console.error("[3维重建] 嵌入3D窗口失败:", error);
  }
}

async function updateNative3DLayout() {
  if (!isLoaded.value || !windowId3D.value) return;
  const metrics = computeRelativeRect(view3DRef.value);
  if (!metrics) return;
  const { x, y, width, height } = metrics;
  try {
    await window.visualizationApi.resizeWindow(
      windowId3D.value,
      x,
      y,
      width,
      height
    );
  } catch (error) {
    console.error("[3维重建] resizeWindow失败:", error);
  }
}

async function loadRecon3DView(folderPath: string) {
  if (isLoading) return;
  isLoading = true;

  try {
    console.log("[3维重建] 加载 3D 视图:", folderPath);

    await stopRenderLoop();
    if (sessionId.value) {
      await window.visualizationApi.destroyAPR(sessionId.value);
      sessionId.value = "";
      windowId3D.value = "";
      isLoaded.value = false;
    }

    sessionId.value = `recon_${Date.now()}_${Math.random()
      .toString(36)
      .substr(2, 9)}`;

    const result = await window.visualizationApi.createAPR(
      sessionId.value,
      folderPath
    );
    if (!result.success) {
      throw new Error(result.error || "创建 APR(3D) 失败");
    }

    windowId3D.value =
      result.windowId3D || result.hwnd3D || `${sessionId.value}_3d`;

    // NOTE:
    // `visualizationApi.loadMasks()` 会弹出 Windows 文件选择对话框。
    // 3D 重建渲染应当仅依赖 DICOM 体数据，mask 作为可选叠加，
    // 因此这里不要在进入 tab 时自动触发“加载 mask 文件”。
    region2DList.value = [];

    await nextTick();
    await embed3DWindow();

    // Ensure we get a first paint immediately after embedding.
    try {
      await window.visualizationApi?.invalidateWindow?.(windowId3D.value);
    } catch {
      // ignore
    }

    // GPU diagnostics (no UI): helps confirm discrete GPU vs iGPU/Basic Render.
    try {
      const gpu = await window.visualizationApi?.getGpuInfo?.(windowId3D.value);
      if (gpu?.success) {
        console.log(
          "[3维重建] GL GPU:",
          gpu.vendor,
          "|",
          gpu.renderer,
          "|",
          gpu.version
        );
      } else if (gpu?.error) {
        console.warn("[3维重建] getGpuInfo failed:", gpu.error);
      }
    } catch (e) {
      console.warn("[3维重建] getGpuInfo exception:", e);
    }

    isLoaded.value = true;
    await window.visualizationApi.startRenderLoop(60);
  } catch (error: any) {
    console.error("[3维重建] 加载失败:", error);
  } finally {
    isLoading = false;
  }
}

// Handler functions for dialogs and deletion（与 ROI 编辑一致）
async function deleteSelectedRegion3D() {
  if (!selectedRegion3D.value) {
    return;
  }

  const index = region3DList.value.indexOf(selectedRegion3D.value);
  if (index === -1) return;

  try {
    // TODO: 调用C++ API删除3D mask

    region3DList.value.splice(index, 1);
    selectedRegion3D.value = null;
  } catch (error) {
    console.error("[3维重建] 删除3D mask失败:", error);
  }
}

// 打开3D配置对话框
async function openConfig3DDialog() {
  if (!sessionId.value) {
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
  }
}

// 打开颜色选择对话框（3D）
async function openColorPicker3D() {
  if (!selectedRegion3D.value) {
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

// Keyboard event handler for Delete key
function handleTableKeydown(event: KeyboardEvent) {
  if (event.key === "Delete" || event.key === "Del") {
    if (selectedRegion3D.value) {
      deleteSelectedRegion3D();
    }
  }
}

onMounted(() => {
  // 监听窗口大小变化，调整嵌入的Win32窗口
  let resizeTimer: NodeJS.Timeout | null = null;
  const resizeObserver = new ResizeObserver(() => {
    if (!isLoaded.value || !sessionId.value) return;

    if (resizeTimer) clearTimeout(resizeTimer);
    resizeTimer = setTimeout(() => {
      updateNative3DLayout();
    }, 100);
  });

  if (view3DRef.value) resizeObserver.observe(view3DRef.value);

  const boundsChangeListener = () => {
    void updateNative3DLayout();
  };
  window.ipcRenderer?.on(
    "electron-window-bounds-changed",
    boundsChangeListener
  );

  // 监听独立 dialog 窗口的返回结果（与 ROI 编辑一致）
  if (window.nativeBridge?.dialog?.onResult) {
    window.nativeBridge.dialog.onResult(async (result: any) => {
      if (
        result?.sessionId &&
        sessionId.value &&
        result.sessionId !== sessionId.value
      ) {
        return;
      }

      if (result?.dialogType === "config3d") {
        if (result.action === "apply") {
          const enabled = !!result?.data?.vramOptimized;
          const windowId = sessionId.value ? `${sessionId.value}_3d` : null;
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
      }
    });
  }

  (onUnmounted as any)(() => {
    resizeObserver.disconnect();
    window.ipcRenderer?.off(
      "electron-window-bounds-changed",
      boundsChangeListener
    );

    if (window.nativeBridge?.dialog?.offResult) {
      window.nativeBridge.dialog.offResult();
    }
  });
});

onUnmounted(() => {
  void stopRenderLoop();
  if (sessionId.value) {
    window.visualizationApi
      .destroyAPR(sessionId.value)
      .catch((error: unknown) =>
        console.error("[3维重建] 清理资源失败:", error)
      );
  }
});
</script>

<template>
  <div class="viewer-page">
    <!-- 左侧控制面板 -->
    <aside class="left-panel">
      <ElCard class="control-card" shadow="hover">
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
        <div class="section">
          <div class="btn-row">
            <ElButton size="small" type="primary">传递函数</ElButton>
            <ElButton size="small" type="primary">3D工具</ElButton>
            <ElButton size="small" type="primary">光照工具</ElButton>
            <ElButton size="small" type="primary">3D截图</ElButton>
          </div>
        </div>
      </ElCard>
    </aside>

    <!-- 右侧显示面板：单个3D视图区 -->
    <main class="right-panel">
      <ElCard class="view-cell" shadow="hover">
        <div ref="view3DRef" class="view-container"></div>
        <div v-if="!isLoaded" class="view-placeholder">
          3D 视图（用于mask渲染）
        </div>
      </ElCard>
    </main>
  </div>
</template>

<!-- layout styles are shared globally in src/style.css -->

<style scoped>
/* 复用 ROI 页的表格风格，保证“3D掩膜区域”观感一致 */
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

.roi-table :deep(.el-table__body tr.current-row > td) {
  background: rgba(64, 158, 255, 0.12) !important;
}

.view-container {
  width: 100%;
  height: 100%;
}
</style>
