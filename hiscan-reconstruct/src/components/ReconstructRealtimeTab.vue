<script setup>
import { computed, onBeforeUnmount, onMounted, reactive, ref } from "vue";

const now = ref(new Date());
const clockDisplay = computed(() =>
  now.value.toLocaleTimeString("zh-CN", { hour12: false })
);
const dateDisplay = computed(() =>
  new Intl.DateTimeFormat("zh-CN", {
    year: "numeric",
    month: "long",
    day: "numeric",
    weekday: "long",
  }).format(now.value)
);

const clockStart = Date.now();
const uptimeDisplay = computed(() => {
  const elapsed = Date.now() - clockStart;
  const totalSeconds = Math.floor(elapsed / 1000);
  const hours = String(Math.floor(totalSeconds / 3600)).padStart(2, "0");
  const minutes = String(Math.floor((totalSeconds % 3600) / 60)).padStart(2, "0");
  const seconds = String(totalSeconds % 60).padStart(2, "0");
  return `${hours}:${minutes}:${seconds}`;
});

const fileTransferLogs = ref([
  { time: "10:11:04", level: "info", message: "Batch-02 重建参数已同步至调度服务" },
  { time: "10:11:31", level: "info", message: "Batch-02 原始投影 2.4GB 开始传输" },
  { time: "10:12:02", level: "success", message: "Batch-02 原始投影传输完成，用时 31s" },
  { time: "10:12:05", level: "info", message: "Batch-02 校准阶段开始，预计耗时 1m20s" },
  { time: "10:12:58", level: "warn", message: "Batch-03 等待重建资源，排队序号 #2" },
  { time: "10:13:14", level: "success", message: "Batch-01 降噪阶段完成，侧写写入完毕" },
  { time: "10:13:44", level: "info", message: "Batch-02 体素重建启动，GPU0/1 已锁定" },
  { time: "10:14:21", level: "info", message: "Batch-02 降噪参数已下发，等待资源" },
  { time: "10:15:02", level: "warn", message: "Batch-03 校准阶段排队时间超过 2 分钟" },
]);

const batches = ref([
  {
    id: "Batch-01",
    sample: "肝脏批次 A",
    steps: [
      { name: "数据传输", status: "完成", progress: 100, count: "720 / 720" },
      { name: "几何校准", status: "完成", progress: 100, count: "64 / 64" },
      { name: "体素重建", status: "完成", progress: 100, count: "1600 / 1600" },
      { name: "降噪处理", status: "完成", progress: 100, count: "1600 / 1600" },
    ],
  },
  {
    id: "Batch-02",
    sample: "胰腺病理",
    steps: [
      { name: "数据传输", status: "完成", progress: 100, count: "720 / 720" },
      { name: "几何校准", status: "完成", progress: 100, count: "64 / 64" },
      { name: "体素重建", status: "进行中", progress: 64, count: "1024 / 1600" },
      { name: "降噪处理", status: "排队中", progress: 12, count: "待分配" },
    ],
  },
  {
    id: "Batch-03",
    sample: "脊柱复查",
    steps: [
      { name: "数据传输", status: "完成", progress: 100, count: "540 / 540" },
      { name: "几何校准", status: "排队中", progress: 28, count: "18 / 64" },
      { name: "体素重建", status: "等待", progress: 0, count: "0 / 1600" },
      { name: "降噪处理", status: "等待", progress: 0, count: "0 / 1600" },
    ],
  },
  {
    id: "Batch-04",
    sample: "科研样本-02",
    steps: [
      { name: "数据传输", status: "排队中", progress: 18, count: "132 / 720" },
      { name: "几何校准", status: "等待", progress: 0, count: "0 / 64" },
      { name: "体素重建", status: "等待", progress: 0, count: "0 / 1600" },
      { name: "降噪处理", status: "等待", progress: 0, count: "0 / 1600" },
    ],
  },
  {
    id: "Batch-05",
    sample: "全身体检",
    steps: [
      { name: "数据传输", status: "等待", progress: 0, count: "0 / 720" },
      { name: "几何校准", status: "等待", progress: 0, count: "0 / 64" },
      { name: "体素重建", status: "等待", progress: 0, count: "0 / 1600" },
      { name: "降噪处理", status: "等待", progress: 0, count: "0 / 1600" },
    ],
  },
]);

const activeBatches = computed(() =>
  batches.value.filter((batch) =>
    batch.steps.some((step) => step.progress < 100)
  )
);

const systemMetrics = reactive({
  gpu: [62, 48], // 支持多张显卡，示例为2张
  vram: [48, 36],
  cpu: 37,
  memory: 52,
  disk: 71,
  diskFree: 12, // 剩余磁盘空间（GB），用于不足提示
  network: 12,
});

const systemMetricEntries = computed(() => {
  const entries = [];
  systemMetrics.gpu.forEach((val, idx) => {
    entries.push({ key: `gpu${idx}`, label: `显卡${idx}占用`, value: val });
  });
  systemMetrics.vram.forEach((val, idx) => {
    entries.push({ key: `vram${idx}`, label: `显存${idx}占用`, value: val });
  });
  entries.push({ key: "cpu", label: "CPU 占用", value: systemMetrics.cpu });
  entries.push({ key: "memory", label: "内存占用", value: systemMetrics.memory });
  entries.push({ key: "disk", label: "磁盘吞吐", value: systemMetrics.disk });
  entries.push({ key: "network", label: "网络占用", value: systemMetrics.network });
  return entries;
});
function chipBarClass(val) {
  if (val >= 80) return "bar-red";
  if (val >= 60) return "bar-yellow";
  return "bar-green";
}
const stepColors = ["#ffb300", "#ff7043", "#42a5f5", "#ab47bc"];
const statusLegend = [
  { label: "数据传输", color: stepColors[0] },
  { label: "几何校准", color: stepColors[1] },
  { label: "体素重建", color: stepColors[2] },
  { label: "降噪处理", color: stepColors[3] },
];

const metricTimer = ref();
const clockTimer = ref();

onMounted(() => {
  clockTimer.value = window.setInterval(() => {
    now.value = new Date();
  }, 1000);

  metricTimer.value = window.setInterval(() => {
    systemMetrics.gpu = systemMetrics.gpu.map(val => Math.min(99, Math.max(12, val + randomDelta())));
    systemMetrics.vram = systemMetrics.vram.map(val => Math.min(99, Math.max(10, val + randomDelta())));
    systemMetrics.cpu = Math.min(99, Math.max(8, systemMetrics.cpu + randomDelta()));
    systemMetrics.memory = Math.min(99, Math.max(15, systemMetrics.memory + randomDelta()));
    systemMetrics.disk = Math.min(99, Math.max(20, systemMetrics.disk + randomDelta()));
    systemMetrics.diskFree = Math.max(0, systemMetrics.diskFree - Math.round(Math.random()));
    systemMetrics.network = Math.min(99, Math.max(4, systemMetrics.network + randomDelta()));
  }, 3000);
});

onBeforeUnmount(() => {
  if (clockTimer.value) {
    clearInterval(clockTimer.value);
  }
  if (metricTimer.value) {
    clearInterval(metricTimer.value);
  }
});

function randomDelta() {
  return Math.round((Math.random() - 0.5) * 8);
}

function logClass(level) {
  switch (level) {
    case "success":
      return "log-success";
    case "warn":
      return "log-warn";
    case "error":
      return "log-error";
    default:
      return "log-info";
  }
}

function stepFillStyle(step, index) {
  const color = stepColors[index % stepColors.length];
  const width = Math.max(0, Math.min(step.progress, 100));
  let opacity = 0.85;
  if (step.status === "进行中") {
    opacity = 1;
  } else if (step.status === "排队中") {
    opacity = 0.6;
  } else if (step.status === "等待") {
    opacity = 0.35;
  }
  return {
    width: `${width}%`,
    background: color,
    opacity,
  };
}

function batchStatusLabel(batch) {
  const running = batch.steps.find((step) => step.status === "进行中");
  if (running) {
    return `${running.name}进行中`;
  }
  const queued = batch.steps.find((step) => step.status === "排队中");
  if (queued) {
    return `排队 ${queued.name}`;
  }
  const pending = batch.steps.find((step) => step.status === "等待");
  if (pending) {
    return `等待 ${pending.name}`;
  }
  return "批次全部完成";
}

function batchFocusStyle(batch) {
  // Find the running step
  const running = batch.steps.find((step) => step.status === '进行中');
  if (running) {
    // Find the index of the running step
    const idx = batch.steps.findIndex((step) => step === running);
    // Use the stepColors for background
    return {
      background: stepColors[idx % stepColors.length],
      color: '#fff',
      border: `1px solid ${stepColors[idx % stepColors.length]}`,
    };
  }
  // If queued, use the queued step color
  const queued = batch.steps.find((step) => step.status === '排队中');
  if (queued) {
    const idx = batch.steps.findIndex((step) => step === queued);
    return {
      background: stepColors[idx % stepColors.length],
      color: '#fff',
      border: `1px solid ${stepColors[idx % stepColors.length]}`,
    };
  }
  // If pending, use the pending step color
  const pending = batch.steps.find((step) => step.status === '等待');
  if (pending) {
    const idx = batch.steps.findIndex((step) => step === pending);
    return {
      background: stepColors[idx % stepColors.length],
      color: '#fff',
      border: `1px solid ${stepColors[idx % stepColors.length]}`,
    };
  }
  // Default style
  return {
    background: 'rgba(60,38,0,0.16)',
    color: '#fff',
    border: '1px solid rgba(255,200,120,0.28)',
  };
}
</script>

<template>
  <div class="tab-content">
    <div class="content-layout">
      <div class="left-column">
        <section class="clock-card">
          <div class="clock-time">{{ clockDisplay }}</div>
          <div class="clock-date">{{ dateDisplay }}</div>
          <div class="clock-uptime">调度运行时长 {{ uptimeDisplay }}</div>
        </section>

        <section class="log-card">
          <header class="section-title">
            <h2>文件传输与任务日志</h2>
            <span>记录数据同步、校准、重建、降噪节点的状态</span>
          </header>
          <ul class="log-list">
            <li v-for="log in fileTransferLogs" :key="log.time + log.message" :class="['log-entry', logClass(log.level)]">
              <span class="log-time">{{ log.time }}</span>
              <span class="log-message">{{ log.message }}</span>
            </li>
          </ul>
        </section>

      </div>

      <div class="right-column">
        <section class="steps-card">
          <header class="section-title card-title">
            <div>
              <h2>活跃批次进度</h2>
              <span>展示处理中批次的阶段占比</span>
            </div>
            <div class="status-legend">
              <div v-for="item in statusLegend" :key="item.label" class="legend-item">
                <span class="legend-dot" :style="{ background: item.color }" />
                <span>{{ item.label }}</span>
              </div>
            </div>
          </header>
          <div class="batch-list">
            <div v-for="batch in activeBatches" :key="batch.id" class="batch-row">
              <div class="batch-meta">
                <div class="batch-title">
                        <span class="batch-id">{{ batch.id }}</span>
                        <span class="batch-name">{{ batch.sample }}</span>
                      </div>
                      <div
                        class="batch-focus"
                        :style="batchFocusStyle(batch)"
                      >
                        {{ batchStatusLabel(batch) }}
                      </div>
              </div>
              <div class="step-stack">
                <div
                  v-for="(step, index) in batch.steps"
                  :key="step.name"
                  class="step-track"
                >
                  <div class="step-header">
                    <span>{{ step.name }}</span>
                    <span class="step-count">{{ step.count }}</span>
                  </div>
                  <span class="step-fill" :style="stepFillStyle(step, index)" />
                </div>
              </div>
            </div>
          </div>
        </section>
      </div>
    </div>

    <footer class="system-bar-flat-wrapper">
      <div class="system-bar-flat">
        <div
          v-for="metric in systemMetricEntries"
          :key="metric.key"
          class="system-chip-flat"
          :class="chipBarClass(metric.value)"
        >
          <span class="chip-label-flat">{{ metric.label }}</span>
          <span class="chip-value-flat">{{ metric.value }}%</span>
        </div>
        <div
          v-if="systemMetrics.diskFree <= 10"
          class="system-chip-flat bar-red"
          style="font-weight:bold;"
        >
          <span class="chip-label-flat">磁盘空间不足</span>
          <span class="chip-value-flat">剩余 {{ systemMetrics.diskFree }}GB</span>
        </div>
      </div>
    </footer>
  </div>
</template>

<style scoped>
.tab-content {
  width: 100%;
  height: 100%;
  padding: 20px 24px 16px;
  box-sizing: border-box;
  background: linear-gradient(180deg, rgba(46, 32, 0, 0.96) 0%, rgba(22, 14, 0, 0.96) 100%);
  color: #ffeac0;
  display: flex;
  flex-direction: column;
}

.content-layout {
  display: flex;
  gap: 16px;
  flex: 1;
  min-height: 0;
}

.left-column {
  flex: 0 0 36%;
  display: flex;
  flex-direction: column;
  gap: 12px;
  min-width: 360px;
  max-width: 440px;
}

.right-column {
  flex: 1;
  display: flex;
  flex-direction: column;
  min-height: 0;
}

.clock-card {
  background: rgba(60, 38, 0, 0.92);
  border: 1px solid rgba(255, 204, 64, 0.4);
  border-radius: 14px;
  padding: 18px 24px;
  box-shadow: 0 12px 28px rgba(25, 15, 0, 0.55);
  backdrop-filter: blur(10px);
}

.clock-time {
  font-size: 46px;
  font-weight: 600;
  letter-spacing: 2px;
  color: #ffda7f;
}

.clock-date {
  margin-top: 6px;
  font-size: 16px;
  color: rgba(255, 238, 194, 0.9);
}

.clock-uptime {
  margin-top: 10px;
  font-size: 14px;
  color: rgba(255, 219, 148, 0.8);
}

.log-card {
  flex: 1;
  display: flex;
  flex-direction: column;
  background: rgba(60, 38, 0, 0.88);
  border: 1px solid rgba(255, 204, 64, 0.35);
  border-radius: 14px;
  padding: 16px 20px;
  box-shadow: 0 16px 36px rgba(25, 14, 0, 0.55);
  backdrop-filter: blur(12px);
  min-height: 0;
}

.section-title h2 {
  font-size: 18px;
  font-weight: 600;
  margin: 0;
  color: #ffe4aa;
}

.section-title span {
  font-size: 12.5px;
  color: rgba(255, 227, 179, 0.7);
}

.log-list {
  list-style: none;
  padding: 0;
  margin: 16px 0 0;
  overflow-y: auto;
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.log-entry {
  display: flex;
  gap: 12px;
  padding: 9px 12px;
  border-radius: 10px;
  background: rgba(40, 25, 0, 0.85);
  border: 1px solid rgba(255, 210, 120, 0.25);
}

.log-time {
  font-family: "SFMono-Regular", Consolas, "Liberation Mono", monospace;
  font-size: 12.5px;
  color: rgba(255, 229, 170, 0.9);
  min-width: 66px;
}

.log-message {
  flex: 1;
  font-size: 13px;
  color: #ffecbe;
}

.log-info {
  border-color: rgba(255, 210, 120, 0.35);
}

.log-success {
  border-color: rgba(156, 214, 103, 0.55);
  background: rgba(42, 54, 12, 0.82);
}



.system-bar-flat-wrapper {
  width: 100%;
  margin-top: 16px;
  background: rgba(40, 25, 0, 0.82);
  border: 1.5px solid rgba(255, 204, 96, 0.35);
  border-radius: 6px;
  box-sizing: border-box;
  padding: 4px 8px;
  display: flex;
  flex-shrink: 0;
}

.system-bar-flat {
  display: flex;
  flex-direction: row;
  gap: 0;
  width: 100%;
  box-shadow: none;
  background: none;
  padding: 0;
  flex-shrink: 0;
}

 .system-chip-flat {
   display: flex;
   flex-direction: row;
   align-items: center;
   justify-content: flex-start;
   min-width: 100px;
   height: 26px;
   padding: 0 10px;
   margin-right: 8px;
   box-sizing: border-box;
   border-radius: 4px;
   border: none;
   font-size: 13px;
   font-weight: 600;
 }

 .chip-label-flat {
   font-size: 13px;
   color: #222;
   margin-right: 10px;
 }

 .chip-value-flat {
   font-size: 13px;
   color: #222;
   font-weight: 600;
 }

 .bar-green {
   background: rgba(126, 217, 87, 0.55);
 }
 .bar-yellow {
   background: rgba(255, 224, 102, 0.55);
 }
 .bar-red {
   background: rgba(255, 94, 94, 0.55);
 }

.steps-card {
  flex: 1;
  background: rgba(60, 38, 0, 0.9);
  border: 1px solid rgba(255, 204, 64, 0.35);
  border-radius: 14px;
  padding: 18px 24px;
  box-shadow: 0 16px 36px rgba(25, 15, 0, 0.55);
  display: flex;
  flex-direction: column;
  gap: 18px;
  backdrop-filter: blur(12px);
  min-height: 0;
}


.batch-list {
  flex: 1;
  overflow-y: auto;
  display: flex;
  flex-direction: column;
  gap: 4px;
  min-height: 0;
  padding-right: 4px;
}

.batch-row {
  height: 90px;
  display: grid;
  grid-template-columns: 200px 1fr;
  gap: 12px;
  align-items: center;
  padding: 6px 10px;
  border-radius: 12px;
  background: rgba(36, 20, 0, 0.68);
  border: 1px solid rgba(255, 200, 120, 0.25);
}

.batch-meta {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.batch-title {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.batch-id {
  font-size: 14px;
  font-weight: 600;
  color: #ffd88c;
}

.batch-name {
  font-size: 12.5px;
  color: rgba(255, 224, 175, 0.78);
}

.batch-focus {
  display: inline-flex;
  align-items: center;
  gap: 8px;
  font-size: 12px;
  color: rgba(255, 222, 170, 0.8);
  padding: 4px 10px;
  border-radius: 6px;
  background: rgba(255, 190, 100, 0.16);
  border: 1px solid rgba(255, 200, 120, 0.28);
  max-width: fit-content;
}

.step-stack {
  width: 100%;
  height: 100%;
  display: grid;
  grid-template-rows: repeat(4, 1fr);
  gap: 6px;
}

.step-track {
  position: relative;
  border-radius: 999px;
  background: rgba(255, 219, 165, 0.12);
  overflow: hidden;
}

.step-track:nth-child(1) {
  background: rgba(255, 179, 0, 0.18);
}

.step-track:nth-child(2) {
  background: rgba(255, 112, 67, 0.18);
}

.step-track:nth-child(3) {
  background: rgba(66, 165, 245, 0.18);
}

.step-track:nth-child(4) {
  background: rgba(171, 71, 188, 0.18);
}

.step-fill {
  position: absolute;
  top: 0;
  left: 0;
  bottom: 0;
  border-radius: inherit;
  transition: width 0.3s ease, opacity 0.3s ease;
  width: 0;
}

.step-header {
  position: absolute;
  inset: 0;
  z-index: 1;
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 0 10px;
  font-size: 12px;
  color: rgba(36, 20, 0, 0.85);
  font-weight: 600;
  mix-blend-mode: lighten;
}

.step-count {
  font-family: "SFMono-Regular", Consolas, "Liberation Mono", monospace;
  font-size: 11.5px;
}

.step-track:nth-child(1) .step-header,
.step-track:nth-child(1) .step-count {
  color: rgba(60, 36, 0, 0.82);
}

.step-track:nth-child(2) .step-header,
.step-track:nth-child(2) .step-count {
  color: rgba(68, 14, 0, 0.9);
}

.step-track:nth-child(3) .step-header,
.step-track:nth-child(3) .step-count {
  color: rgba(6, 32, 54, 0.9);
}

.step-track:nth-child(4) .step-header,
.step-track:nth-child(4) .step-count {
  color: rgba(48, 8, 54, 0.92);
}

.card-title {
  display: flex;
  justify-content: space-between;
  gap: 16px;
  align-items: flex-end;
}

.status-legend {
  display: flex;
  align-items: center;
  gap: 12px;
  flex-wrap: wrap;
}

.legend-item {
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 12px;
  color: rgba(255, 224, 170, 0.7);
}

.legend-dot {
  width: 12px;
  height: 12px;
  border-radius: 50%;
  border: 1px solid rgba(255, 235, 200, 0.4);
}

.log-list::-webkit-scrollbar,
.batch-list::-webkit-scrollbar {
  width: 8px;
}

.log-list::-webkit-scrollbar-thumb,
.batch-list::-webkit-scrollbar-thumb {
  border-radius: 999px;
  background: rgba(255, 200, 90, 0.45);
}
</style>

