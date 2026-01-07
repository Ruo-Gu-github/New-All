<template>
  <div class="dialog-container">
    <div class="dialog-content">
      <div class="form-item">
        <label class="form-label">最小值 ({{ rigMarkMin }})</label>
        <el-slider
          v-model="rigMarkMin"
          :min="histogramMinValue"
          :max="histogramMaxValue"
          show-input
          @change="updatePreview"
        />
      </div>

      <div class="form-item">
        <label class="form-label">最大值 ({{ rigMarkMax }})</label>
        <el-slider
          v-model="rigMarkMax"
          :min="histogramMinValue"
          :max="histogramMaxValue"
          show-input
          @change="updatePreview"
        />
      </div>

      <div class="form-item">
        <label class="form-label">直方图预览（Y轴为对数尺度）</label>
        <canvas ref="histogramCanvas" class="histogram-canvas"></canvas>
      </div>

      <div class="form-item">
        <label class="form-label">掩膜颜色</label>
        <el-color-picker
          v-model="previewColor"
          @change="updatePreview"
          show-alpha
        />
      </div>
    </div>

    <div class="dialog-footer">
      <el-button @click="cancel">取消</el-button>
      <el-button type="primary" @click="apply">应用</el-button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, watch, nextTick } from "vue";
import { ElSlider, ElColorPicker, ElButton, ElMessage } from "element-plus";
import "element-plus/dist/index.css";

const rigMarkMin = ref(0);
const rigMarkMax = ref(100);
const histogramMinValue = ref(0);
const histogramMaxValue = ref(255);
const previewColor = ref("#ffff00");
const histogramCanvas = ref<HTMLCanvasElement | null>(null);
const histogramData = ref<number[]>([]);
let sessionId = "";

// 接收来自主窗口的初始数据
onMounted(async () => {
  console.log(
    "[RigMarkDialog] Mounted, window.electronAPI:",
    window.electronAPI
  );

  // 从 URL 参数获取 sessionId
  const params = new URLSearchParams(window.location.search);
  sessionId = params.get("sessionId") || "";

  console.log("[RigMarkDialog] SessionId:", sessionId);

  if (!sessionId) {
    ElMessage.error("SessionId 未提供");
    return;
  }

  if (!window.electronAPI) {
    ElMessage.error("electronAPI 未注入（无法加载直方图）");
    return;
  }

  try {
    // 获取直方图数据
    const result = await window.electronAPI.invoke(
      "viz:get-volume-histogram",
      sessionId
    );
    console.log("[RigMarkDialog] Histogram result:", result);

    if (result && result.data) {
      histogramData.value = result.data;
      histogramMinValue.value = result.minValue || 0;
      histogramMaxValue.value = result.maxValue || 255;
      rigMarkMin.value = histogramMinValue.value;
      rigMarkMax.value = histogramMaxValue.value;

      await nextTick();
      drawHistogram();
    } else {
      ElMessage.error("获取直方图数据失败");
    }
  } catch (error: any) {
    console.error("[RigMarkDialog] Error loading histogram:", error);
    ElMessage.error(`加载直方图失败: ${error.message}`);
  }
});

function drawHistogram() {
  if (!histogramCanvas.value || !histogramData.value.length) {
    console.warn(
      "[RigMarkDialog] Cannot draw histogram, canvas or data missing"
    );
    return;
  }

  const canvas = histogramCanvas.value;

  // 设置 canvas 实际分辨率
  const dpr = window.devicePixelRatio || 1;
  const rect = canvas.getBoundingClientRect();
  canvas.width = rect.width * dpr;
  canvas.height = rect.height * dpr;

  const ctx = canvas.getContext("2d");
  if (!ctx) return;

  ctx.scale(dpr, dpr);

  const width = rect.width;
  const height = rect.height;
  const data = histogramData.value;

  console.log(
    "[RigMarkDialog] Drawing histogram, data length:",
    data.length,
    "canvas size:",
    width,
    "x",
    height
  );

  ctx.clearRect(0, 0, width, height);

  // 对数转换
  const logData = data.map((v) => (v > 0 ? Math.log10(v + 1) : 0));
  const maxLog = Math.max(...logData);

  if (maxLog === 0) {
    console.warn("[RigMarkDialog] Max log value is 0");
    return;
  }

  // 绘制直方图
  const barWidth = width / data.length;
  ctx.fillStyle = "#409eff";

  logData.forEach((value, index) => {
    const barHeight = (value / maxLog) * height;
    const x = index * barWidth;
    const y = height - barHeight;
    ctx.fillRect(x, y, Math.max(barWidth - 1, 1), barHeight);
  });

  // 绘制阈值线
  const range = histogramMaxValue.value - histogramMinValue.value;
  if (range === 0) return;

  const minX = ((rigMarkMin.value - histogramMinValue.value) / range) * width;
  const maxX = ((rigMarkMax.value - histogramMinValue.value) / range) * width;

  ctx.strokeStyle = "#f56c6c";
  ctx.lineWidth = 2;
  ctx.setLineDash([5, 5]);

  ctx.beginPath();
  ctx.moveTo(minX, 0);
  ctx.lineTo(minX, height);
  ctx.stroke();

  ctx.beginPath();
  ctx.moveTo(maxX, 0);
  ctx.lineTo(maxX, height);
  ctx.stroke();

  ctx.setLineDash([]);
}

watch([rigMarkMin, rigMarkMax], () => {
  drawHistogram();
});

async function updatePreview() {
  if (!sessionId) return;

  if (!window.electronAPI) {
    console.warn("[RigMarkDialog] electronAPI missing");
    return;
  }

  try {
    await window.electronAPI.invoke(
      "viz:update-preview-mask",
      sessionId,
      rigMarkMin.value,
      rigMarkMax.value,
      previewColor.value
    );
    drawHistogram();
  } catch (error) {
    console.error("[RigMarkDialog] Update preview error:", error);
  }
}

function cancel() {
  window.electronAPI?.send("dialog:close", { action: "cancel" });
}

function apply() {
  window.electronAPI?.send("dialog:close", {
    action: "apply",
    data: {
      min: rigMarkMin.value,
      max: rigMarkMax.value,
      color: previewColor.value,
    },
  });
}
</script>

<style scoped>
.dialog-container {
  width: 100%;
  min-height: 100vh;
  display: flex;
  flex-direction: column;
  background: #ffffff;
  padding: 20px;
  box-sizing: border-box;
}

.dialog-content {
  flex: 1;
  overflow-y: auto;
}

.form-item {
  margin-bottom: 24px;
}

.form-label {
  display: block;
  margin-bottom: 12px;
  color: #606266;
  font-size: 14px;
  font-weight: 500;
}

.histogram-canvas {
  width: 100%;
  height: 200px;
  border: 1px solid #dcdfe6;
  border-radius: 4px;
  background: #fafafa;
}

.dialog-footer {
  padding-top: 16px;
  border-top: 1px solid #e4e7ed;
  display: flex;
  justify-content: flex-end;
  gap: 12px;
  margin-top: 20px;
}
</style>
