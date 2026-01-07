<template>
  <div class="image-manager">
    <!-- 顶部操作：两个 primary 按钮，置于圆角卡片中 -->
    <ElCard class="control-card" shadow="hover">
      <div class="toolbar">
        <ElButton
          class="accent-btn"
          size="large"
          :icon="UploadFilled"
          @click="selectAndLoadFolder"
          :loading="loading"
        >
          加载图像
        </ElButton>
        <ElButton
          class="accent-btn"
          size="large"
          :icon="DeleteFilled"
          :disabled="!panels.length"
          @click="clearPanels"
        >
          清空图像
        </ElButton>
      </div>
    </ElCard>

    <!-- 其余内容：也放入一个圆角卡片中，占据剩余空间 -->
    <ElCard class="content-card" shadow="hover">
      <div v-if="panels.length" class="panel-list">
        <div v-for="panel in panels" :key="panel.id" style="position: relative">
          <ElCard
            class="image-panel"
            shadow="hover"
            :class="{ selected: selectedPanels.includes(panel.id) }"
            @click="handlePanelClick(panel, $event)"
            @contextmenu="handlePanelRightClick(panel, $event)"
          >
            <div class="panel-body">
              <div class="panel-thumb">
                <img
                  v-if="panel.thumbnail"
                  :src="panel.thumbnail"
                  alt="DICOM预览"
                />
                <div v-else class="thumb-placeholder">
                  {{ panel.sampleName }}
                </div>
              </div>
              <div class="panel-info">
                <div class="info-row">
                  <span class="info-label">样本名</span>
                  <span class="info-value">{{ panel.patientName }}</span>
                </div>
                <div class="info-row">
                  <span class="info-label">图像数</span>
                  <span class="info-value">{{ panel.imageCount }}</span>
                </div>
                <div class="info-row">
                  <span class="info-label">图像尺寸</span>
                  <span class="info-value">{{
                    formatDimensions(panel.width, panel.height)
                  }}</span>
                </div>
                <div class="info-row">
                  <span class="info-label">分辨率</span>
                  <span class="info-value">{{ panel.resolution }}</span>
                </div>
              </div>
            </div>
          </ElCard>
        </div>
      </div>
      <div v-else class="empty-state">
        <ElEmpty description="暂无图像数据" />
      </div>
      <!-- 右键菜单 -->
      <div
        v-if="contextMenuVisible"
        class="context-menu"
        :style="{ left: contextMenuX + 'px', top: contextMenuY + 'px' }"
        @click.stop
      >
        <ul>
          <li @click="handleMenuAction('view')">图像浏览</li>
          <li @click="handleMenuAction('roi')">ROI编辑</li>
          <li @click="handleMenuAction('3d')">3维重建</li>
          <li @click="handleMenuAction('bone')">骨骼分析</li>
          <li @click="handleMenuAction('vessel')">血管分析</li>
          <li @click="handleMenuAction('fat')">脂肪分析</li>
          <li
            v-if="canJumpFromFileManager('lung')"
            @click="handleMenuAction('lung')"
          >
            肺容积分析（4D）
          </li>
          <!-- 可继续扩展 -->
        </ul>
      </div>
    </ElCard>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from "vue";
import { ElMessage, ElButton, ElCard, ElEmpty } from "element-plus";
import { UploadFilled, DeleteFilled } from "@element-plus/icons-vue";

// 定义 emit
const emit = defineEmits<{
  switchTab: [tabName: string, panelData: ImagePanel];
}>();

const props = defineProps<{
  // Per-tab flags from App.vue controlling whether a tab can be opened via
  // the file manager context menu.
  jumpableTabs?: Record<string, boolean>;
}>();

interface ImagePanel {
  id: string;
  sampleName: string;
  imageCount: number;
  width: number;
  height: number;
  resolution: string;
  thumbnail: string;
  folderPath: string;
  // DICOM Tags
  patientName?: string;
  patientID?: string;
  studyDate?: string;
  modality?: string;
  seriesDescription?: string;

  // Default window/level (from DICOM if available)
  windowWidth?: number;
  windowLevel?: number;
}

const panels = ref<ImagePanel[]>([]);
const loading = ref(false);
const selectedPanels = ref<string[]>([]);
const contextMenuPanelId = ref<string | null>(null);
const contextMenuVisible = ref(false);
const contextMenuX = ref(0);
const contextMenuY = ref(0);

function canJumpFromFileManager(tabKey: string) {
  const map = props.jumpableTabs;
  if (!map) return true;
  return !!map[tabKey];
}

// 在组件挂载时检查 API 和加载已有序列
onMounted(async () => {
  console.log("[AnalyzerImageManagerTab] Mounted");
  console.log("[AnalyzerImageManagerTab] window.dicomApi:", window.dicomApi);

  if (!window.dicomApi) {
    console.error("[AnalyzerImageManagerTab] dicomApi not found!");
    return;
  }

  // 加载已有的序列列表
  try {
    const allSeries = await window.dicomApi.getAllSeries();
    panels.value = allSeries.map((s: any) => ({
      id: s.id,
      sampleName: s.seriesName,
      imageCount: s.fileCount,
      width: s.width || 0,
      height: s.height || 0,
      resolution: formatResolution(s.width, s.height),
      thumbnail: buildThumbnailDataUrl(s.thumbnail),
      folderPath: s.folderPath,
      patientName: s.patientName,
      patientID: s.patientID,
      studyDate: s.studyDate,
      modality: s.modality,
      seriesDescription: s.seriesDescription,
      windowWidth: s.windowWidth,
      windowLevel: s.windowLevel,
    }));
  } catch (error) {
    console.error("[AnalyzerImageManagerTab] Failed to load series:", error);
  }
});

function inferThumbnailMimeFromBase64(base64: string): string {
  // PNG header: 89 50 4E 47 => iVBORw0KGgo
  if (base64.startsWith("iVBORw0KGgo")) return "image/png";
  // JPEG header: FF D8 FF => /9j/
  if (base64.startsWith("/9j/")) return "image/jpeg";
  // BMP header: 42 4D => Qk
  if (base64.startsWith("Qk")) return "image/bmp";
  return "image/png";
}

function buildThumbnailDataUrl(base64?: string): string {
  if (!base64 || typeof base64 !== "string") return "";
  const trimmed = base64.trim();
  if (!trimmed) return "";
  // If backend already returned a data URL, keep it.
  if (trimmed.startsWith("data:")) return trimmed;
  const mime = inferThumbnailMimeFromBase64(trimmed);
  return `data:${mime};base64,${trimmed}`;
}

function formatResolution(
  widthOrSpacingX?: number,
  heightOrSpacingY?: number
): string {
  if (!widthOrSpacingX || !heightOrSpacingY) return "未知";
  // 如果值很小（< 10），认为是spacing；否则认为是像素尺寸
  if (widthOrSpacingX < 10) {
    return `${widthOrSpacingX.toFixed(3)} mm × ${heightOrSpacingY.toFixed(
      3
    )} mm`;
  } else {
    const pixelSpacing = 0.372;
    const physicalWidth = (widthOrSpacingX * pixelSpacing).toFixed(2);
    const physicalHeight = (heightOrSpacingY * pixelSpacing).toFixed(2);
    return `${physicalWidth} × ${physicalHeight} mm`;
  }
}

function formatDimensions(width: number, height: number): string {
  if (!width || !height) return "未知";
  return `${width} × ${height}`;
}

async function selectAndLoadFolder() {
  try {
    loading.value = true;

    if (!window.dicomApi) {
      ElMessage.error("DICOM API 未加载");
      loading.value = false;
      return;
    }

    // 打开新文件夹前释放所有原生窗口和体数据，避免占用资源
    try {
      await window.visualizationApi?.stopRenderLoop?.();
    } catch (error) {
      console.error("[selectAndLoadFolder] stopRenderLoop failed:", error);
    }
    try {
      await window.visualizationApi?.destroyAllWindows?.();
    } catch (error) {
      console.error("[selectAndLoadFolder] destroyAllWindows failed:", error);
    }

    // 选择文件夹
    const folderPath = await window.dicomApi.selectFolder();

    if (!folderPath) {
      ElMessage.info("未选择文件夹");
      loading.value = false;
      return;
    }

    console.log("[selectAndLoadFolder] 扫描文件夹:", folderPath);

    // 扫描文件夹（只读第一个文件）
    const result = await window.dicomApi.scanFolder(folderPath);

    console.log("[selectAndLoadFolder] 扫描结果:", result);

    if (!result || !result.success) {
      ElMessage.error(`扫描失败: ${result?.error || "无效的 DICOM 数据"}`);
      loading.value = false;
      return;
    }

    const series = result.series;

    // 创建面板
    const panel: ImagePanel = {
      id: series.id,
      sampleName: series.seriesDescription || series.seriesName || "未命名",
      imageCount: series.fileCount || 0,
      width: series.width,
      height: series.height,
      resolution: formatResolution(series.width, series.height),
      thumbnail: buildThumbnailDataUrl(series.thumbnail),
      folderPath: series.folderPath,
      patientName: series.patientName,
      patientID: series.patientID,
      studyDate: series.studyDate,
      modality: series.modality,
      seriesDescription: series.seriesDescription,
      windowWidth: series.windowWidth,
      windowLevel: series.windowLevel,
    };

    console.log("[selectAndLoadFolder] 创建的面板:", panel);

    panels.value.push(panel);
    ElMessage.success(
      `成功加载: ${panel.sampleName} (${panel.imageCount} 张图像)`
    );
  } catch (error: any) {
    console.error("选择文件夹失败:", error);
    ElMessage.error(`操作失败: ${error.message || "未知错误"}`);
  } finally {
    loading.value = false;
  }
}

function clearPanels() {
  panels.value = [];
  void window.dicomApi?.cleanup?.();
  ElMessage.info("已清空图像列表");
}

function handlePanelClick(panel: ImagePanel, event: MouseEvent) {
  if (event.ctrlKey || event.metaKey) {
    // 多选
    if (selectedPanels.value.includes(panel.id)) {
      selectedPanels.value = selectedPanels.value.filter(
        (id) => id !== panel.id
      );
    } else {
      selectedPanels.value.push(panel.id);
    }
  } else if (event.shiftKey) {
    // Shift 多选（简单实现：全选）
    selectedPanels.value = panels.value.map((p) => p.id);
  } else {
    // 单选
    selectedPanels.value = [panel.id];
  }
}

function handlePanelRightClick(panel: ImagePanel, event: MouseEvent) {
  event.preventDefault();
  contextMenuPanelId.value = panel.id;
  contextMenuVisible.value = true;
  contextMenuX.value = event.clientX;
  contextMenuY.value = event.clientY;
}

function handleMenuAction(action: string) {
  contextMenuVisible.value = false;
  let tabName = "";
  if (action === "view") {
    tabName = "viewer";
  } else if (action === "roi") {
    tabName = "roi";
  } else if (action === "3d") {
    tabName = "reconstruct";
  } else if (action === "bone") {
    tabName = "skeletal";
  } else if (action === "vessel") {
    tabName = "vascular";
  } else if (action === "fat") {
    tabName = "fat";
  } else if (action === "lung") {
    tabName = "lung";
  }

  if (
    tabName &&
    !canJumpFromFileManager(tabName === "reconstruct" ? "recon3d" : tabName)
  ) {
    return;
  }

  if (tabName && contextMenuPanelId.value) {
    // 找到对应的 panel 数据
    const panel = panels.value.find((p) => p.id === contextMenuPanelId.value);
    if (panel) {
      // 通过 emit 通知父组件切换 tab，传递完整的 panel 数据（包含路径）
      emit("switchTab", tabName, panel);
    }
  }
}
</script>

<style scoped>
.image-manager {
  display: flex;
  flex-direction: column;
  gap: 16px;
  width: 100%;
  height: 100%;
}

.control-card,
.content-card {
  border-radius: 12px;
  background: rgba(8, 25, 44, 0.8);
  border: 1px solid rgba(11, 205, 212, 0.25);
}

/* 控制卡片内边距与布局 */
.control-card :deep(.el-card__body) {
  padding: 12px 16px;
}

.content-card {
  flex: 1;
  display: flex;
  min-height: 0; /* 允许内部滚动 */
}

.content-card :deep(.el-card__body) {
  padding: 12px;
  display: flex;
  flex-direction: column;
  height: 100%;
}

.toolbar {
  display: flex;
  align-items: center;
  gap: 12px;
}

/* 与 tabs 色系统一的自定义按钮（去掉 primary） */
.control-card :deep(.accent-btn) {
  background: rgba(0, 142, 177, 0.68);
  color: #f5fbff; /* 更高对比度的文字颜色 */
  border: 1px solid rgba(53, 209, 240, 0.4);
  border-radius: 6px; /* 更接近普通按钮 */
  box-shadow: 0 1px 2px rgba(0, 120, 160, 0.18);
  padding: 8px 12px; /* 更紧凑 */
  min-height: 36px;
  font-weight: 600;
  display: inline-flex;
  align-items: center;
  gap: 8px;
}
.control-card :deep(.accent-btn .el-icon) {
  font-size: 16px; /* 常规按钮图标尺寸 */
  color: inherit;
}
.control-card :deep(.accent-btn .el-icon svg) {
  width: 16px;
  height: 16px;
}
.control-card :deep(.accent-btn:hover) {
  background: rgba(0, 142, 177, 0.78);
  border-color: rgba(53, 209, 240, 0.55);
}
.control-card :deep(.accent-btn.is-disabled),
.control-card :deep(.accent-btn:disabled) {
  opacity: 0.6;
  filter: grayscale(10%);
}

.file-input {
  display: none;
}

.panel-list {
  display: flex;
  flex-wrap: wrap;
  gap: 16px;
  overflow-y: auto;
  padding-right: 4px;
  align-content: flex-start;
}

.image-panel {
  width: 400px;
  height: 180px;
  flex-shrink: 0;
  border-radius: 12px;
  background: rgba(8, 25, 44, 0.8);
  border: 1px solid rgba(11, 205, 212, 0.25);
}

.image-panel :deep(.el-card__body) {
  height: 100%;
  padding: 16px;
}

.panel-body {
  display: flex;
  gap: 16px;
  align-items: stretch;
}

.panel-thumb {
  width: 140px;
  display: flex;
  align-items: center;
  justify-content: center;
}

.panel-thumb img {
  width: 100%;
  aspect-ratio: 1 / 1;
  object-fit: cover;
  border-radius: 10px;
  border: 1px solid rgba(11, 205, 212, 0.45);
  background: #061120;
}

.thumb-placeholder {
  width: 100%;
  aspect-ratio: 1 / 1;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 10px;
  border: 1px dashed rgba(11, 205, 212, 0.35);
  color: rgba(168, 219, 255, 0.65);
  background: rgba(6, 20, 36, 0.65);
  font-size: 14px;
}

.panel-info {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 10px;
  justify-content: center;
}

.info-row {
  display: flex;
  align-items: center;
  gap: 12px;
  font-size: 14px;
}

.info-label {
  width: 78px;
  color: rgba(168, 219, 255, 0.7);
}

.info-value {
  color: #f3f8ff;
  flex: 1;
  word-break: break-all;
}

.image-panel.selected {
  border: 2px solid #00bfff;
  box-shadow: 0 0 0 2px #00bfff33;
}
.context-menu {
  position: fixed;
  z-index: 9999;
  background: #1a2a3a;
  border-radius: 8px;
  box-shadow: 0 2px 8px #0006;
  min-width: 120px;
  padding: 6px 0;
  color: #fff;
}
.context-menu ul {
  list-style: none;
  margin: 0;
  padding: 0;
}
.context-menu li {
  padding: 8px 18px;
  cursor: pointer;
  font-size: 15px;
}
.context-menu li:hover {
  background: #00bfff33;
}
</style>
