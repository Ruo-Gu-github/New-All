<script setup lang="ts">
import { ref, onMounted, onUnmounted, computed, watch } from "vue";
import { ElProgress } from "element-plus";
import { useWindowControls } from "./services/nativeBridge";
import AnalyzerImageManagerTab from "./components/AnalyzerImageManagerTab.vue";
import AnalyzerViewerTab from "./components/AnalyzerViewerTab.vue";
import AnalyzerRoiTab from "./components/AnalyzerRoiTab.vue";
import AnalyzerReconTab from "./components/AnalyzerReconTab.vue";
import AnalyzerSkeletalTab from "./components/AnalyzerSkeletalTab.vue";
import AnalyzerVascularTab from "./components/AnalyzerVascularTab.vue";
import AnalyzerFatTab from "./components/AnalyzerFatTab.vue";
import AnalyzerLungTab from "./components/AnalyzerLungTab.vue";

const windowControls = useWindowControls();

// 全局加载进度
const globalLoading = ref(false);
const loadingProgress = ref(0);
const loadingMessage = ref("");

// Native crash/restart recovery UX
const nativeRestartedAt = ref<number | null>(null);
const reloadKey = ref(0);

// Temporary safety switches:
// - User request: do NOT restore previous UI/session state on startup.
// - Keep persistence off to avoid re-entering a broken state after exit/relaunch.
const ENABLE_SESSION_RESTORE = false;
const ENABLE_SESSION_PERSIST = false;

function showNativeRecovery() {
  nativeRestartedAt.value = Date.now();
}

function hideNativeRecovery() {
  nativeRestartedAt.value = null;
}

async function restartRenderer() {
  try {
    await (window as any).nativeBridge?.app?.restartRenderer?.();
  } catch (e) {
    console.warn("[App] restartRenderer failed:", e);
  }
}

function reloadCurrentSession() {
  // Trigger tabs to reload even if folderPath is unchanged.
  reloadKey.value += 1;
  hideNativeRecovery();
}

const canReloadSession = computed(() => {
  return !!selectedPanelData.value?.folderPath && activeTab.value !== "lung";
});

// 进度条颜色
const progressColor = computed(() => {
  if (loadingProgress.value < 30) return "#67C23A";
  if (loadingProgress.value < 70) return "#409EFF";
  return "#0BCDD4";
});

// 监听加载进度
onMounted(() => {
  if (window.visualizationApi) {
    window.visualizationApi.onProgress(
      (data: { progress: number; message: string }) => {
        globalLoading.value = true;
        loadingProgress.value = data.progress;
        loadingMessage.value = data.message;

        if (data.progress >= 100) {
          setTimeout(() => {
            globalLoading.value = false;
          }, 500);
        }
      }
    );
  }
});

onUnmounted(() => {
  if (window.visualizationApi) {
    window.visualizationApi.offProgress();
  }
});

type AnalyzerTabKey =
  | "imageManager"
  | "viewer"
  | "roi"
  | "recon3d"
  | "skeletal"
  | "vascular"
  | "fat"
  | "lung";

interface AnalyzerImageRecord {
  id: string;
  sampleName: string;
  resolution: string;
  imageCount: number;
  size: string;
  current: string;
  voltage: string;
  thumbnail: string;
  files: File[];
}

type TabConfig = {
  key: AnalyzerTabKey;
  label: string;
  // Whether this tab supports jumping from the Image Manager context menu.
  // Some tabs have their own data-loading flow and should not be opened this way.
  canJumpFromFileManager: boolean;
};

const tabs: TabConfig[] = [
  { key: "imageManager", label: "图像管理", canJumpFromFileManager: false },
  { key: "viewer", label: "图像浏览", canJumpFromFileManager: true },
  { key: "roi", label: "ROI编辑", canJumpFromFileManager: true },
  { key: "recon3d", label: "3维重建", canJumpFromFileManager: true },
  { key: "skeletal", label: "骨骼分析", canJumpFromFileManager: true },
  { key: "vascular", label: "血管分析", canJumpFromFileManager: true },
  { key: "fat", label: "脂肪分析", canJumpFromFileManager: true },
  { key: "lung", label: "肺容积分析（4D）", canJumpFromFileManager: false },
];

const jumpableTabsFromFileManager = computed(() => {
  const map: Record<string, boolean> = {};
  for (const t of tabs) {
    map[t.key] = !!t.canJumpFromFileManager;
  }
  return map;
});

const activeTab = ref<AnalyzerTabKey>("imageManager");
const lastSelectedRecord = ref<AnalyzerImageRecord | null>(null);
const selectedPanelData = ref<any>(null);

const tabsWith3D: AnalyzerTabKey[] = [
  "viewer",
  "roi",
  "recon3d",
  "skeletal",
  "vascular",
  "fat",
  "lung",
];

function is3DTab(t: AnalyzerTabKey) {
  return tabsWith3D.includes(t);
}

async function preSwitchNativeWindows(nextTab: AnalyzerTabKey) {
  const currentTab = activeTab.value;
  const leaving3D = is3DTab(currentTab) && !is3DTab(nextTab);
  const switchingBetween3D =
    is3DTab(currentTab) && is3DTab(nextTab) && currentTab !== nextTab;

  if (!(leaving3D || switchingBetween3D)) return;
  if (!window.visualizationApi) return;

  // Stop rendering first to avoid drawing during unmount/embed/layout changes.
  try {
    await window.visualizationApi.stopRenderLoop?.();
  } catch (error) {
    console.warn("[App] stopRenderLoop (pre-switch) failed:", error);
  }

  // If leaving 3D pages entirely, hide HWNDs before switching to prevent transient flashes.
  if (leaving3D) {
    try {
      await window.visualizationApi.hideAllWindows?.();
    } catch (error) {
      console.warn("[App] hideAllWindows (pre-switch) failed:", error);
    }
  }
}

let renderResumeTimer: number | null = null;
let lastRenderPauseReason: "resize" | "blur" | "hidden" | null = null;

async function pauseRenderLoop(reason: "resize" | "blur" | "hidden") {
  lastRenderPauseReason = reason;
  if (renderResumeTimer != null) {
    window.clearTimeout(renderResumeTimer);
    renderResumeTimer = null;
  }
  try {
    await window.visualizationApi?.stopRenderLoop?.();
  } catch (error) {
    console.warn("[App] stopRenderLoop failed:", error);
  }
}

function scheduleResumeRenderLoop(delayMs = 180) {
  if (renderResumeTimer != null) {
    window.clearTimeout(renderResumeTimer);
  }
  renderResumeTimer = window.setTimeout(async () => {
    renderResumeTimer = null;
    // Only resume when a 3D tab is active and we are not in a loading transition.
    if (globalLoading.value) return;
    if (!is3DTab(activeTab.value)) return;
    try {
      await window.visualizationApi?.startRenderLoop?.(60);
    } catch (error) {
      // Safe to ignore: a tab may not have created/embedded windows yet.
      console.debug("[App] startRenderLoop skipped/failed:", error);
    } finally {
      lastRenderPauseReason = null;
    }
  }, delayMs);
}

async function ensureSelectedPanelData() {
  if (selectedPanelData.value?.folderPath) return;
  try {
    const allSeries = await window.dicomApi?.getAllSeries?.();
    const last =
      Array.isArray(allSeries) && allSeries.length > 0
        ? allSeries[allSeries.length - 1]
        : null;
    if (!last?.folderPath) return;
    selectedPanelData.value = {
      id: last.id,
      folderPath: last.folderPath,
      patientName: last.patientName,
      imageCount: last.fileCount ?? 0,
      width: last.width ?? 0,
      height: last.height ?? 0,
      windowWidth:
        typeof last.windowWidth === "number"
          ? Number(last.windowWidth)
          : undefined,
      windowLevel:
        typeof last.windowLevel === "number"
          ? Number(last.windowLevel)
          : undefined,
      modality: last.modality,
      seriesDescription: last.seriesDescription,
      studyDate: last.studyDate,
    };
  } catch {
    // ignore
  }
}

async function onTabClick(nextTab: AnalyzerTabKey) {
  // Avoid switching tabs while native is still loading/creating textures.
  if (globalLoading.value) return;

  await preSwitchNativeWindows(nextTab);

  // For 3D/analysis tabs: reuse the last loaded series automatically.
  // NOTE: lung tab is an independent module and must NOT reuse cached volume/series.
  if (nextTab !== "imageManager" && nextTab !== "lung") {
    await ensureSelectedPanelData();
  }

  activeTab.value = nextTab;
}

// 支持通过 URL 参数直接打开指定 tab（用于多窗口测试）
onMounted(() => {
  try {
    const params = new URLSearchParams(window.location.search);
    const tab = params.get("tab") as AnalyzerTabKey | null;
    if (tab && tabs.some((t) => t.key === tab)) {
      activeTab.value = tab;
    }
  } catch {
    // ignore
  }

  // Restore last session context (minimal): tab + selected series folderPath.
  // Disabled (temporary): do NOT restore state on startup.
  if (ENABLE_SESSION_RESTORE) {
    try {
      const params = new URLSearchParams(window.location.search);
      const hasTabParam = !!params.get("tab");
      if (!hasTabParam) {
        const savedTab = localStorage.getItem("hiscan.lastTab");
        const savedPanel = localStorage.getItem("hiscan.lastPanelData");
        if (savedPanel) {
          const parsed = JSON.parse(savedPanel);
          if (parsed?.folderPath) {
            selectedPanelData.value = parsed;
          }
        }
        if (
          savedTab &&
          tabs.some((t) => t.key === savedTab) &&
          savedTab !== "imageManager" &&
          savedTab !== "lung" &&
          selectedPanelData.value?.folderPath
        ) {
          activeTab.value = savedTab as AnalyzerTabKey;
        }
      }
    } catch {
      // ignore
    }
  }

  // Reduce flicker during maximize/restore/resize/minimize:
  // pause render loop while the Electron window is actively changing bounds.
  const boundsListener = () => {
    void pauseRenderLoop("resize");
    // 320ms 匹配 Windows maximize/restore 动画时长 (约 300-400ms)
    scheduleResumeRenderLoop(320);
  };
  // Some tabs also listen to this event to re-embed windows; we only pause/resume.
  (window as any).ipcRenderer?.on?.(
    "electron-window-bounds-changed",
    boundsListener
  );

  // Fallback: also pause during resize events.
  // On some systems, bounds-changed IPC may miss certain resize/drag sequences.
  const onResize = () => {
    void pauseRenderLoop("resize");
    // Debounce resume while resizing.
    scheduleResumeRenderLoop(220);
  };
  window.addEventListener("resize", onResize);

  // Minimize often triggers blur/visibility changes; pause render loop to avoid black-frame flashes.
  const openDialogWebContentsIds = new Set<number>();
  const onDialogOpened = (payload?: any) => {
    const id = Number(payload?.webContentsId);
    if (Number.isFinite(id) && id > 0) openDialogWebContentsIds.add(id);

    // If the main window already blurred and paused the loop, undo it.
    // This is common when the dialog steals focus immediately.
    if (lastRenderPauseReason === "blur") {
      scheduleResumeRenderLoop(0);
    }
  };
  const onDialogClosed = (payload?: any) => {
    const id = Number(payload?.webContentsId);
    if (Number.isFinite(id) && id > 0) openDialogWebContentsIds.delete(id);
    else openDialogWebContentsIds.clear();
  };
  (window as any).ipcRenderer?.on?.("dialog:opened", onDialogOpened);
  (window as any).ipcRenderer?.on?.("dialog:closed", onDialogClosed);

  const onBlur = () => {
    if (document.hidden) {
      void pauseRenderLoop("hidden");
      return;
    }

    try {
      if (is3DTab(activeTab.value)) {
        void window.visualizationApi?.raiseAllWindows?.();
        void window.visualizationApi?.refreshAllWindowsZOrder?.();
      }
    } catch {
      // ignore
    }
    // IMPORTANT:
    // Do NOT stop the render loop on a plain blur.
    // Users frequently click outside the app or operate multiple windows; stopping the loop
    // can cause embedded native HWNDs to become hidden or lose mouse routing.
  };
  const onFocus = () => {
    try {
      if (is3DTab(activeTab.value)) {
        void window.visualizationApi?.raiseAllWindows?.();
        void window.visualizationApi?.refreshAllWindowsZOrder?.();
      }
    } catch {
      // ignore
    }
    scheduleResumeRenderLoop(120);
  };
  window.addEventListener("blur", onBlur);
  window.addEventListener("focus", onFocus);

  const onVisibility = () => {
    if (document.hidden) {
      void pauseRenderLoop("hidden");
    } else {
      try {
        if (is3DTab(activeTab.value)) {
          void window.visualizationApi?.raiseAllWindows?.();
          void window.visualizationApi?.refreshAllWindowsZOrder?.();
        }
      } catch {
        // ignore
      }
      scheduleResumeRenderLoop(120);
    }
  };
  document.addEventListener("visibilitychange", onVisibility);

  // Native host restarted/crashed and recovered: prompt user actions.
  let handlingNativeRestart = false;
  let lastNativeRestartAt = 0;

  const onNativeRestarted = () => {
    const now = Date.now();
    console.warn("[App] native:restarted");

    // Guard against crash-loops (event storms) which can freeze the UI.
    if (handlingNativeRestart) return;
    if (now - lastNativeRestartAt < 1500) return;
    handlingNativeRestart = true;
    lastNativeRestartAt = now;

    // Make UI responsive again: stop render loop, hide native windows (best-effort).
    try {
      globalLoading.value = false;
    } catch {
      // ignore
    }
    try {
      void window.visualizationApi?.stopRenderLoop?.();
      void window.visualizationApi?.hideAllWindows?.();
    } catch {
      // ignore
    }

    // Do not try to restore session; keep things simple and allow user to exit.
    activeTab.value = "imageManager";
    selectedPanelData.value = null;
    lastSelectedRecord.value = null;

    showNativeRecovery();
    setTimeout(() => {
      handlingNativeRestart = false;
    }, 800);
  };
  (window as any).ipcRenderer?.on?.("native:restarted", onNativeRestarted);

  onUnmounted(() => {
    (window as any).ipcRenderer?.off?.(
      "electron-window-bounds-changed",
      boundsListener
    );
    window.removeEventListener("resize", onResize);
    window.removeEventListener("blur", onBlur);
    window.removeEventListener("focus", onFocus);
    document.removeEventListener("visibilitychange", onVisibility);
    (window as any).ipcRenderer?.off?.("dialog:opened", onDialogOpened);
    (window as any).ipcRenderer?.off?.("dialog:closed", onDialogClosed);
    (window as any).ipcRenderer?.off?.("native:restarted", onNativeRestarted);
  });
});

// Persist minimal session context.
watch(
  [activeTab, selectedPanelData],
  ([tab, panel]) => {
    if (!ENABLE_SESSION_PERSIST) return;
    try {
      localStorage.setItem("hiscan.lastTab", tab);
      if (panel?.folderPath) {
        localStorage.setItem("hiscan.lastPanelData", JSON.stringify(panel));
      }
    } catch {
      // ignore
    }
  },
  { deep: true }
);

// 监听 tab 切换，使用隐藏/显示窗口优化性能
watch(activeTab, async (newTab, oldTab) => {
  // 离开3D相关页面：隐藏窗口（保留体数据和APR）
  if (oldTab && is3DTab(oldTab) && !is3DTab(newTab)) {
    console.log(`[App] Leaving ${oldTab}, hiding windows`);
    try {
      // Fallback: if tab switch didn't go through onTabClick/handleSwitchTab.
      await window.visualizationApi?.stopRenderLoop?.();
      await window.visualizationApi?.hideAllWindows?.();
    } catch (error) {
      console.error("[App] hideAllWindows failed:", error);
    }
  }

  // 返回3D相关页面：显示窗口
  if (newTab && is3DTab(newTab) && oldTab && !is3DTab(oldTab)) {
    // Do NOT call showAllWindows here.
    // Showing native HWNDs before the target tab finishes creating/embedding can cause flicker.
  }
});

// 让已打开的 tab 保持挂载（避免频繁销毁重建带来的状态抖动）
const mountedTabFlags = ref<Record<string, boolean>>({});
watch(
  activeTab,
  (t) => {
    mountedTabFlags.value = { ...mountedTabFlags.value, [t]: true };
  },
  { immediate: true }
);

// 处理 tab 切换（支持两种调用方式）
async function handleSwitchTab(
  tabOrPayload:
    | AnalyzerTabKey
    | { tab: AnalyzerTabKey; record: AnalyzerImageRecord }
    | string,
  panelDataOrId?: any
) {
  if (typeof tabOrPayload === "string") {
    // 来自右键菜单：handleSwitchTab(tabName, panelData)
    const tabMap: Record<string, AnalyzerTabKey> = {
      viewer: "viewer",
      roi: "roi",
      reconstruct: "recon3d",
      skeletal: "skeletal",
      vascular: "vascular",
      fat: "fat",
      lung: "lung",
    };
    const next = tabMap[tabOrPayload] || (tabOrPayload as AnalyzerTabKey);
    if (!jumpableTabsFromFileManager.value[next]) {
      return;
    }

    await preSwitchNativeWindows(next);
    activeTab.value = next;
    selectedPanelData.value = panelDataOrId; // 保存完整的 panel 数据（包含路径）
  } else if (typeof tabOrPayload === "object" && "tab" in tabOrPayload) {
    // 来自其他地方：handleSwitchTab({ tab, record })

    await preSwitchNativeWindows(tabOrPayload.tab);
    activeTab.value = tabOrPayload.tab;
    lastSelectedRecord.value = tabOrPayload.record;
  }
}

const minimizeWindow = async () => {
  await windowControls.minimize();
};

const toggleMaximizeWindow = async () => {
  await windowControls.toggleMaximize();
};

const closeWindow = async () => {
  await windowControls.close();
};
</script>

<template>
  <div class="main-bg">
    <div class="console-app">
      <div class="titlebar" data-drag-region>
        <span class="title">Hiscan Analyzer</span>
        <div class="window-controls">
          <button type="button" @click.stop="minimizeWindow" title="最小化">
            &#x2015;
          </button>
          <button
            type="button"
            @click.stop="toggleMaximizeWindow"
            title="最大化/还原"
          >
            &#x25A1;
          </button>
          <button type="button" @click.stop="closeWindow" title="关闭">
            &#x2715;
          </button>
        </div>
      </div>

      <div class="tabs">
        <button
          v-for="tab in tabs"
          :key="tab.key"
          :class="{ active: activeTab === tab.key }"
          @click="onTabClick(tab.key)"
        >
          {{ tab.label }}
        </button>
      </div>

      <!-- Non-overlay loading indicator (do NOT cover native HWND area) -->
      <div v-if="globalLoading" class="global-loading-strip">
        <div class="global-loading-text">{{ loadingMessage }}</div>
        <el-progress
          class="global-loading-progress"
          :percentage="loadingProgress"
          :stroke-width="6"
          :color="progressColor"
        />
        <div class="global-loading-pct">{{ loadingProgress }}%</div>
      </div>

      <!-- Native host recovery strip (non-overlay) -->
      <div v-if="nativeRestartedAt" class="recovery-strip">
        <div class="recovery-text">
          渲染引擎已重启（native:restarted）。建议刷新以恢复会话。
        </div>
        <div class="recovery-actions">
          <button type="button" class="recovery-btn" @click="restartRenderer">
            重启渲染器
          </button>
          <button
            type="button"
            class="recovery-btn primary"
            :disabled="!canReloadSession"
            @click="reloadCurrentSession"
            title="重新创建当前 tab 的会话/窗口"
          >
            重新加载当前会话
          </button>
          <button
            type="button"
            class="recovery-btn ghost"
            @click="hideNativeRecovery"
          >
            忽略
          </button>
        </div>
      </div>

      <div class="tab-content-area">
        <AnalyzerImageManagerTab
          v-if="activeTab === 'imageManager'"
          :jumpable-tabs="jumpableTabsFromFileManager"
          @switch-tab="handleSwitchTab"
        />
        <AnalyzerViewerTab
          v-if="activeTab === 'viewer'"
          :panel-data="selectedPanelData"
          :reload-key="reloadKey"
          @switch-tab="handleSwitchTab"
        />
        <AnalyzerRoiTab
          v-if="activeTab === 'roi'"
          :panel-data="selectedPanelData"
          :reload-key="reloadKey"
        />
        <AnalyzerReconTab
          v-if="activeTab === 'recon3d'"
          :panel-data="selectedPanelData"
          :reload-key="reloadKey"
        />
        <AnalyzerSkeletalTab
          v-if="activeTab === 'skeletal'"
          :panel-data="selectedPanelData"
          :reload-key="reloadKey"
          @switch-tab="handleSwitchTab"
        />
        <AnalyzerVascularTab
          v-if="activeTab === 'vascular'"
          :panel-data="selectedPanelData"
          :reload-key="reloadKey"
        />
        <AnalyzerFatTab
          v-if="activeTab === 'fat'"
          :panel-data="selectedPanelData"
          :reload-key="reloadKey"
        />
        <AnalyzerLungTab v-if="activeTab === 'lung'" :panel-data="null" />
      </div>
    </div>
  </div>
</template>

<style>
html,
body,
#app {
  margin: 0;
  padding: 0;
  height: 100%;
  width: 100%;
  overflow: hidden;
}
</style>

<style scoped>
.main-bg {
  min-height: 100vh;
  width: 100%;
  background: linear-gradient(180deg, #071a29 0%, #040d1a 100%);
  display: flex;
  justify-content: center;
  align-items: stretch;
  color: #dbf6ff;
}

.console-app {
  width: 100%;
  height: 100vh;
  display: flex;
  flex-direction: column;
}

.global-loading-strip {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 6px 12px;
  border-top: 1px solid rgba(11, 205, 212, 0.18);
  border-bottom: 1px solid rgba(11, 205, 212, 0.18);
  background: rgba(8, 25, 44, 0.35);
}

.recovery-strip {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
  padding: 8px 12px;
  border-top: 1px solid rgba(11, 205, 212, 0.18);
  border-bottom: 1px solid rgba(11, 205, 212, 0.18);
  background: rgba(8, 25, 44, 0.55);
}

.recovery-text {
  font-size: 12px;
  opacity: 0.92;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.recovery-actions {
  display: flex;
  gap: 8px;
  flex: 0 0 auto;
}

.recovery-btn {
  height: 28px;
  padding: 0 10px;
  border-radius: 8px;
  border: 1px solid rgba(11, 205, 212, 0.28);
  background: rgba(4, 13, 26, 0.35);
  color: #dbf6ff;
  cursor: pointer;
}

.recovery-btn:hover {
  background: rgba(11, 205, 212, 0.18);
}

.recovery-btn.primary {
  background: rgba(11, 205, 212, 0.22);
  border-color: rgba(11, 205, 212, 0.45);
}

.recovery-btn.primary:hover {
  background: rgba(11, 205, 212, 0.3);
}

.recovery-btn.ghost {
  opacity: 0.85;
}

.recovery-btn:disabled {
  opacity: 0.45;
  cursor: not-allowed;
}

.global-loading-text {
  flex: 0 0 auto;
  font-size: 12px;
  opacity: 0.92;
  max-width: 420px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.global-loading-progress {
  flex: 1 1 auto;
}

.global-loading-pct {
  flex: 0 0 auto;
  font-size: 12px;
  opacity: 0.9;
  min-width: 44px;
  text-align: right;
}

.titlebar {
  width: 100%;
  height: 36px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0;
  -webkit-app-region: drag;
  user-select: none;
  color: #d1f6ff;
}

.title {
  font-size: 1.1rem;
  color: #f2fdff;
  font-weight: 600;
  letter-spacing: 1px;
  padding-left: 12px;
}

.window-controls {
  display: flex;
  gap: 2px;
  -webkit-app-region: no-drag;
  height: 100%;
  padding-right: 4px;
}

.window-controls button {
  width: 44px;
  height: 100%;
  background: transparent;
  color: #c7f4ff;
  border: none;
  font-size: 1rem;
  border-radius: 4px;
  cursor: pointer;
  transition: background 0.2s;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 0;
  appearance: none;
}

.window-controls button:hover {
  background: rgba(86, 209, 243, 0.22);
}

.window-controls button:last-child:hover {
  background: #e53935;
  color: #ffffff;
}

.tabs {
  display: flex;
  gap: 16px;
  padding: 0 32px;
  height: 52px;
  align-items: center;
}

.tabs button {
  background: transparent;
  color: rgba(172, 243, 255, 0.88);
  border: none;
  padding: 8px 24px;
  font-size: 1.05rem;
  border-radius: 8px 8px 0 0;
  cursor: pointer;
  font-weight: 500;
  transition: background 0.2s, color 0.2s;
}

.tabs button.active {
  background: rgba(0, 142, 177, 0.68);
  color: #041621;
  box-shadow: 0 2px 6px rgba(0, 120, 160, 0.28);
  border: 1.5px solid rgba(53, 209, 240, 0.35);
}

.tab-content-area {
  flex: 1;
  width: 100%;
  height: calc(100% - 92px);
  color: #d6f5ff;
  overflow-x: hidden;
  overflow-y: auto;
  scrollbar-gutter: stable;
  display: flex;
  flex-direction: column;
  padding: 24px 32px;
  box-sizing: border-box;
  background: rgba(8, 25, 44, 0.35);
}

:deep(.panel-grid) {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
  gap: 18px;
  width: 100%;
}

:deep(.panel-card) {
  background: rgba(10, 28, 55, 0.92);
  border: 1px solid rgba(0, 188, 212, 0.32);
  border-radius: 16px;
  padding: 18px 20px;
  display: flex;
  flex-direction: column;
  gap: 16px;
  min-height: 180px;
  box-shadow: 0 12px 24px rgba(0, 188, 212, 0.12);
}

:deep(.panel-card header h2) {
  margin: 0;
  font-size: 16px;
  font-weight: 600;
  color: #9cf0ff;
}

:deep(.panel-card header p) {
  margin: 6px 0 0;
  color: rgba(156, 238, 255, 0.78);
  font-size: 12px;
}

:deep(.panel-card.accent) {
  background: linear-gradient(
    135deg,
    rgba(0, 188, 212, 0.28),
    rgba(32, 120, 180, 0.22)
  );
  border-color: rgba(0, 188, 212, 0.6);
}

:deep(.metric-row) {
  display: flex;
  gap: 16px;
  flex-wrap: wrap;
}

:deep(.metric-chip) {
  min-width: 120px;
  background: rgba(3, 24, 46, 0.72);
  border-radius: 12px;
  padding: 12px 14px;
  display: flex;
  flex-direction: column;
  gap: 4px;
}

:deep(.metric-value) {
  font-size: 18px;
  font-weight: 600;
  color: #8bf2ff;
}

:deep(.metric-label) {
  font-size: 12px;
  color: rgba(178, 238, 255, 0.8);
}

:deep(.metric-helper) {
  font-size: 11px;
  color: rgba(172, 247, 255, 0.86);
}

:deep(.log-list) {
  list-style: none;
  margin: 0;
  padding: 0;
  display: flex;
  flex-direction: column;
  gap: 10px;
  color: rgba(164, 244, 255, 0.85);
  font-size: 13px;
}

:deep(.form-stack) {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

:deep(.form-stack label) {
  display: flex;
  flex-direction: column;
  gap: 6px;
  font-size: 12px;
  color: rgba(162, 241, 255, 0.82);
}

:deep(.form-stack input),
:deep(.form-stack select) {
  padding: 10px 12px;
  border-radius: 12px;
  border: 1px solid rgba(0, 188, 212, 0.4);
  background: rgba(6, 20, 40, 0.92);
  color: #dbf6ff;
}

:deep(.btn) {
  border: none;
  border-radius: 999px;
  padding: 9px 20px;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  transition: transform 0.15s ease, box-shadow 0.15s ease;
}

:deep(.btn.primary) {
  background: linear-gradient(
    135deg,
    rgba(0, 188, 212, 0.9),
    rgba(20, 154, 220, 0.9)
  );
  color: #041621;
  box-shadow: 0 12px 24px rgba(0, 188, 212, 0.3);
}

:deep(.btn.primary:hover) {
  transform: translateY(-1px);
}

:deep(.btn.outline) {
  background: rgba(0, 188, 212, 0.1);
  color: #8aefff;
  border: 1px solid rgba(0, 188, 212, 0.45);
}

:deep(.btn.block) {
  width: 100%;
}

:deep(.action-row) {
  display: flex;
  gap: 10px;
}

:deep(.status-list) {
  display: flex;
  flex-direction: column;
  gap: 6px;
  font-size: 12px;
}

:deep(.status-list .error) {
  color: #ff8a80;
}

:deep(.task-list) {
  list-style: none;
  margin: 0;
  padding: 0;
  display: flex;
  flex-direction: column;
  gap: 12px;
}

:deep(.task-name) {
  font-weight: 600;
}

:deep(.task-meta) {
  display: flex;
  justify-content: space-between;
  color: rgba(156, 238, 255, 0.82);
  font-size: 12px;
}

:deep(.flow-list),
:deep(.fat-list) {
  list-style: none;
  margin: 0;
  padding: 0;
  display: flex;
  flex-direction: column;
  gap: 12px;
}

:deep(.flow-name) {
  font-weight: 600;
}

:deep(.flow-value) {
  color: rgba(130, 242, 255, 0.94);
}

:deep(.flow-note) {
  font-size: 12px;
  color: rgba(156, 238, 255, 0.78);
}

:deep(.fat-region) {
  font-weight: 600;
}

:deep(.fat-value) {
  margin-left: auto;
  color: rgba(148, 240, 255, 0.92);
}

:deep(.fat-badge) {
  margin-left: 12px;
  padding: 2px 8px;
  border-radius: 999px;
  background: rgba(0, 188, 212, 0.16);
  border: 1px solid rgba(0, 188, 212, 0.4);
}

/* 全局加载进度条 */
.global-loading-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: rgba(4, 13, 26, 0.9);
  backdrop-filter: blur(8px);
  z-index: 9999;
  display: flex;
  align-items: center;
  justify-content: center;
}

.loading-container {
  width: 100%;
  max-width: 500px;
  padding: 0 40px;
}

.loading-content {
  background: linear-gradient(
    135deg,
    rgba(7, 26, 41, 0.95),
    rgba(8, 25, 44, 0.95)
  );
  border: 2px solid rgba(11, 205, 212, 0.4);
  border-radius: 16px;
  padding: 40px;
  box-shadow: 0 8px 32px rgba(11, 205, 212, 0.15);
}

.loading-title {
  font-size: 18px;
  font-weight: 600;
  color: #0bcdd4;
  text-align: center;
  margin-bottom: 24px;
  text-shadow: 0 2px 8px rgba(11, 205, 212, 0.3);
}

.loading-percentage {
  font-size: 14px;
  color: rgba(168, 219, 255, 0.8);
  text-align: center;
  margin-top: 12px;
  font-weight: 500;
}

.loading-content :deep(.el-progress__text) {
  display: none;
}

.loading-content :deep(.el-progress-bar__outer) {
  background-color: rgba(11, 205, 212, 0.1);
  border-radius: 6px;
}

.loading-content :deep(.el-progress-bar__inner) {
  border-radius: 6px;
  background: linear-gradient(90deg, #0bcdd4, #00bfff);
  box-shadow: 0 0 10px rgba(11, 205, 212, 0.5);
}

@media (max-width: 1200px) {
  .tabs {
    padding: 0 16px;
  }
  .tab-content-area {
    padding: 20px;
  }
}

@media (max-width: 768px) {
  :deep(.panel-grid) {
    grid-template-columns: 1fr;
  }
  :deep(.action-row) {
    flex-direction: column;
  }
  :deep(.btn.block) {
    width: 100%;
  }
}
</style>
