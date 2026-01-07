<template>
  <div class="machine-config-layout">
    <div class="column column-left">
      <section class="panel panel-mechanical">
        <div class="panel-header">
          <h2>机械状态</h2>
          <p>
            当前共有 {{ translationStages.length }} 台平移台，在线
            <strong>{{ connectedStageCount }}</strong> 台
          </p>
        </div>

        <div class="module-grid">
          <div class="module-card" :class="accentClass(turntable.status)">
            <div class="module-head">
              <div>
                <h3>{{ turntable.name }}</h3>
                <span class="module-sub">主轴转台模块</span>
              </div>
              <el-tag :type="statusTagType(turntable.status)" effect="dark" size="small">
                {{ statusLabel(turntable.status) }}
              </el-tag>
            </div>
            <div class="module-body">
              <el-select
                v-model="turntable.selectedDll"
                size="small"
                class="dll-select"
                placeholder="选择驱动"
                @change="handleDllChange(turntable, $event)"
              >
                <el-option v-for="dll in turntable.dllOptions" :key="dll" :label="dll" :value="dll" />
              </el-select>
              <div class="module-meta">
                <span>上次更新：{{ turntable.lastUpdate }}</span>
              </div>
              <el-button-group class="module-actions">
                <el-button
                  size="small"
                  plain
                  type="success"
                  :loading="turntable.busy"
                  @click="handleModuleAction(turntable, 'connect')"
                >连接</el-button>
                <el-button
                  size="small"
                  plain
                  type="warning"
                  :loading="turntable.busy"
                  @click="handleModuleAction(turntable, 'initialize')"
                >初始化</el-button>
                <el-button
                  size="small"
                  plain
                  type="danger"
                  :loading="turntable.busy"
                  @click="handleModuleAction(turntable, 'disconnect')"
                >断连</el-button>
              </el-button-group>
            </div>
          </div>
        </div>

        <div class="stage-table">
          <div class="stage-header">
            <h3>平移台列表</h3>
            <el-tag type="info" size="small">支持批量扩展</el-tag>
          </div>
          <el-table :data="translationStages" height="320" size="small" border>
            <el-table-column prop="alias" label="标识" width="120" />
            <el-table-column prop="axis" label="轴向" width="90" />
            <el-table-column label="状态" width="110">
              <template #default="{ row }">
                <el-tag :type="statusTagType(row.status)" effect="light" size="small">
                  {{ statusLabel(row.status) }}
                </el-tag>
              </template>
            </el-table-column>
            <el-table-column label="驱动 DLL" min-width="180">
              <template #default="{ row }">
                <el-select
                  v-model="row.selectedDll"
                  size="small"
                  class="stage-select"
                  placeholder="选择驱动"
                  @change="handleStageDllChange(row, $event)"
                >
                  <el-option v-for="dll in stageDllOptions" :key="dll" :label="dll" :value="dll" />
                </el-select>
              </template>
            </el-table-column>
            <el-table-column label="操作" width="210">
              <template #default="{ row }">
                <el-button-group>
                  <el-button
                    size="small"
                    plain
                    type="success"
                    :loading="row.busy"
                    @click="handleStageAction(row, 'connect')"
                  >连接</el-button>
                  <el-button
                    size="small"
                    plain
                    type="warning"
                    :loading="row.busy"
                    @click="handleStageAction(row, 'initialize')"
                  >初始化</el-button>
                  <el-button
                    size="small"
                    plain
                    type="danger"
                    :loading="row.busy"
                    @click="handleStageAction(row, 'disconnect')"
                  >断连</el-button>
                </el-button-group>
              </template>
            </el-table-column>
            <el-table-column prop="lastUpdate" label="更新时间" width="160" />
          </el-table>
        </div>
      </section>
    </div>

    <div class="column column-right">
      <section class="panel panel-systems">
        <div class="panel-header">
          <h2>探测系统</h2>
          <p>探测器与射线源的运行状态与驱动配置</p>
        </div>
        <div class="module-grid">
          <div class="module-card" :class="accentClass(detector.status)">
            <div class="module-head">
              <div>
                <h3>{{ detector.name }}</h3>
                <span class="module-sub">平板探测器阵列</span>
              </div>
              <el-tag :type="statusTagType(detector.status)" effect="dark" size="small">
                {{ statusLabel(detector.status) }}
              </el-tag>
            </div>
            <div class="module-body">
              <el-select
                v-model="detector.selectedDll"
                size="small"
                class="dll-select"
                placeholder="选择驱动"
                @change="handleDllChange(detector, $event)"
              >
                <el-option v-for="dll in detector.dllOptions" :key="dll" :label="dll" :value="dll" />
              </el-select>
              <div class="module-meta">
                <span>曝光配置：{{ detector.exposure }}</span>
                <span>上次更新：{{ detector.lastUpdate }}</span>
              </div>
              <el-button-group class="module-actions">
                <el-button
                  size="small"
                  plain
                  type="success"
                  :loading="detector.busy"
                  @click="handleModuleAction(detector, 'connect')"
                >连接</el-button>
                <el-button
                  size="small"
                  plain
                  type="warning"
                  :loading="detector.busy"
                  @click="handleModuleAction(detector, 'initialize')"
                >初始化</el-button>
                <el-button
                  size="small"
                  plain
                  type="danger"
                  :loading="detector.busy"
                  @click="handleModuleAction(detector, 'disconnect')"
                >断连</el-button>
              </el-button-group>
            </div>
          </div>

          <div class="module-card" :class="accentClass(xraySource.status)">
            <div class="module-head">
              <div>
                <h3>{{ xraySource.name }}</h3>
                <span class="module-sub">高压射线源控制模块</span>
              </div>
              <el-tag :type="statusTagType(xraySource.status)" effect="dark" size="small">
                {{ statusLabel(xraySource.status) }}
              </el-tag>
            </div>
            <div class="module-body">
              <el-select
                v-model="xraySource.selectedDll"
                size="small"
                class="dll-select"
                placeholder="选择驱动"
                @change="handleDllChange(xraySource, $event)"
              >
                <el-option v-for="dll in xraySource.dllOptions" :key="dll" :label="dll" :value="dll" />
              </el-select>
              <div class="module-meta">
                <span>当前功率：{{ xraySource.output }}</span>
                <span>上次更新：{{ xraySource.lastUpdate }}</span>
              </div>
              <el-button-group class="module-actions">
                <el-button
                  size="small"
                  plain
                  type="success"
                  :loading="xraySource.busy"
                  @click="handleModuleAction(xraySource, 'connect')"
                >连接</el-button>
                <el-button
                  size="small"
                  plain
                  type="warning"
                  :loading="xraySource.busy"
                  @click="handleModuleAction(xraySource, 'initialize')"
                >初始化</el-button>
                <el-button
                  size="small"
                  plain
                  type="danger"
                  :loading="xraySource.busy"
                  @click="handleModuleAction(xraySource, 'disconnect')"
                >断连</el-button>
              </el-button-group>
            </div>
          </div>
        </div>
      </section>

      <section class="panel panel-cameras">
        <div class="panel-header">
          <h2>摄像头状态</h2>
          <p>多个监控位切换与控制</p>
        </div>
        <el-tabs v-model="activeCamera" stretch class="camera-tabs">
          <el-tab-pane v-for="camera in cameras" :key="camera.id" :label="camera.alias" :name="camera.id">
            <div class="camera-card" :class="accentClass(camera.status)">
              <div class="camera-head">
                <el-tag :type="statusTagType(camera.status)" effect="dark" size="small">
                  {{ statusLabel(camera.status) }}
                </el-tag>
                <span class="camera-ip">{{ camera.ip }}</span>
                <span class="camera-meta">分辨率 {{ camera.resolution }}</span>
              </div>
              <div class="camera-body">
                <p>{{ camera.description }}</p>
                <div class="camera-footer">
                  <span>上次更新：{{ camera.lastUpdate }}</span>
                  <el-button-group>
                    <el-button
                      size="small"
                      plain
                      type="success"
                      :loading="camera.busy"
                      @click="handleCameraAction(camera, 'connect')"
                    >连接</el-button>
                    <el-button
                      size="small"
                      plain
                      type="warning"
                      :loading="camera.busy"
                      @click="handleCameraAction(camera, 'initialize')"
                    >初始化</el-button>
                    <el-button
                      size="small"
                      plain
                      type="danger"
                      :loading="camera.busy"
                      @click="handleCameraAction(camera, 'disconnect')"
                    >断连</el-button>
                  </el-button-group>
                </div>
              </div>
            </div>
          </el-tab-pane>
        </el-tabs>
      </section>
    </div>
  </div>
</template>

<script setup>
import { computed, onBeforeUnmount, reactive, ref } from 'vue';
import { ElMessage } from 'element-plus';

const statusMeta = {
  connected: { label: '已连接', type: 'success', accent: 'accent-connected' },
  disconnected: { label: '未连接', type: 'danger', accent: 'accent-disconnected' },
  initializing: { label: '初始化中', type: 'warning', accent: 'accent-initializing' },
  standby: { label: '待机', type: 'info', accent: 'accent-standby' },
  streaming: { label: '推流中', type: 'success', accent: 'accent-connected' },
  offline: { label: '离线', type: 'danger', accent: 'accent-disconnected' },
  calibrating: { label: '校准中', type: 'warning', accent: 'accent-initializing' },
};

const stageDllOptions = [
  'HiScanStage.dll',
  'PrecisionSlide.dll',
  'LegacyStage32.dll',
  'MicroShiftPlus.dll',
];

const turntable = reactive({
  id: 'turntable',
  name: '主转台',
  status: 'connected',
  selectedDll: 'HiSpin-Pro.dll',
  dllOptions: ['HiSpin-Pro.dll', 'PrecisionRotor.dll', 'LegacySpin32.dll'],
  lastUpdate: createTimestamp(),
  busy: false,
});

const translationStages = reactive([
  {
    id: 'stage-x',
    alias: '平移台 1',
    axis: 'X',
    status: 'connected',
    selectedDll: stageDllOptions[0],
    lastUpdate: createTimestamp(),
    busy: false,
  },
  {
    id: 'stage-y',
    alias: '平移台 2',
    axis: 'Y',
    status: 'disconnected',
    selectedDll: stageDllOptions[1],
    lastUpdate: createTimestamp(),
    busy: false,
  },
  {
    id: 'stage-z',
    alias: '平移台 3',
    axis: 'Z',
    status: 'connected',
    selectedDll: stageDllOptions[2],
    lastUpdate: createTimestamp(),
    busy: false,
  },
]);

const detector = reactive({
  id: 'detector',
  name: 'CT 探测器',
  status: 'connected',
  selectedDll: 'HiScanDetector.dll',
  dllOptions: ['HiScanDetector.dll', 'EdgePanelX.dll', 'LegacyFlatPanel.dll'],
  exposure: '0.45 s / 帧',
  lastUpdate: createTimestamp(),
  busy: false,
});

const xraySource = reactive({
  id: 'xray',
  name: '射线源',
  status: 'standby',
  selectedDll: 'HiBeam-Core.dll',
  dllOptions: ['HiBeam-Core.dll', 'PrecisionPhoton.dll', 'LegacyTube.dll'],
  output: '40 kV / 180 µA',
  lastUpdate: createTimestamp(),
  busy: false,
});

const cameras = reactive([
  {
    id: 'cam-1',
    alias: '舱体顶部',
    status: 'streaming',
    ip: '192.168.5.21',
    resolution: '1080p @ 30fps',
    description: '俯视图，查看转台整体运动。',
    lastUpdate: createTimestamp(),
    busy: false,
  },
  {
    id: 'cam-2',
    alias: '舱体侧面',
    status: 'offline',
    ip: '192.168.5.22',
    resolution: '720p @ 30fps',
    description: '侧面监控样品夹持情况。',
    lastUpdate: createTimestamp(),
    busy: false,
  },
  {
    id: 'cam-3',
    alias: '门禁监控',
    status: 'connected',
    ip: '192.168.5.29',
    resolution: '1080p @ 60fps',
    description: '门禁安全摄像头，检测舱门状态。',
    lastUpdate: createTimestamp(),
    busy: false,
  },
]);

const activeCamera = ref(cameras[0]?.id ?? '');

const timers = new Set();

const connectedStageCount = computed(() =>
  translationStages.filter((stage) => stage.status === 'connected').length
);

function statusLabel(statusKey) {
  return statusMeta[statusKey]?.label ?? statusKey;
}

function statusTagType(statusKey) {
  return statusMeta[statusKey]?.type ?? 'info';
}

function accentClass(statusKey) {
  return statusMeta[statusKey]?.accent ?? 'accent-disconnected';
}

function createTimestamp() {
  return new Date().toLocaleString();
}

function updateStatus(target, statusKey) {
  target.status = statusKey;
  target.lastUpdate = createTimestamp();
}

function labelOf(target) {
  return target.alias || target.name;
}

function schedule(callback, delay = 420) {
  const timer = setTimeout(() => {
    callback();
    timers.delete(timer);
  }, delay);
  timers.add(timer);
}

function handleModuleAction(target, action) {
  if (target.busy) return;
  const name = labelOf(target);
  if (action === 'connect') {
    target.busy = true;
    schedule(() => {
      updateStatus(target, 'connected');
      target.busy = false;
      ElMessage.success(`${name} 已连接`);
    });
  } else if (action === 'disconnect') {
    target.busy = true;
    schedule(() => {
      updateStatus(target, 'disconnected');
      target.busy = false;
      ElMessage.warning(`${name} 已断连`);
    });
  } else if (action === 'initialize') {
    target.busy = true;
    updateStatus(target, 'initializing');
    ElMessage.info(`${name} 开始初始化...`);
    schedule(() => {
      updateStatus(target, 'connected');
      target.busy = false;
      ElMessage.success(`${name} 初始化完成`);
    }, 1400);
  }
}

function handleStageAction(stage, action) {
  handleModuleAction(stage, action);
}

function handleCameraAction(camera, action) {
  if (action === 'connect') {
    handleModuleAction(camera, 'connect');
    schedule(() => {
      updateStatus(camera, 'streaming');
    }, 450);
  } else if (action === 'disconnect') {
    handleModuleAction(camera, 'disconnect');
    schedule(() => {
      updateStatus(camera, 'offline');
    }, 450);
  } else if (action === 'initialize') {
    camera.busy = true;
    updateStatus(camera, 'calibrating');
    ElMessage.info(`${labelOf(camera)} 开始校准...`);
    schedule(() => {
      updateStatus(camera, 'streaming');
      camera.busy = false;
      ElMessage.success(`${labelOf(camera)} 校准完成`);
    }, 1500);
  }
}

function handleDllChange(target, dllValue) {
  target.selectedDll = dllValue;
  target.lastUpdate = createTimestamp();
  ElMessage.success(`${labelOf(target)} 驱动切换为 ${dllValue}`);
}

function handleStageDllChange(stage, dllValue) {
  stage.selectedDll = dllValue;
  stage.lastUpdate = createTimestamp();
  ElMessage.success(`${stage.alias} 驱动切换为 ${dllValue}`);
}

onBeforeUnmount(() => {
  timers.forEach((timer) => clearTimeout(timer));
  timers.clear();
});
</script>

<style scoped>
.machine-config-layout {
  width: 100%;
  height: 100%;
  display: grid;
  grid-template-columns: minmax(540px, 1.1fr) minmax(380px, 0.9fr);
  gap: 16px;
  padding: 18px 24px 28px;
  box-sizing: border-box;
  color: #e8edff;
  background: linear-gradient(180deg, #0e183e 0%, #070d26 100%);
}

.column {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.panel {
  background: rgba(21, 32, 72, 0.92);
  border: 1px solid rgba(92, 116, 209, 0.35);
  border-radius: 16px;
  padding: 18px 20px;
  box-shadow: 0 16px 32px rgba(4, 12, 36, 0.45);
  backdrop-filter: blur(12px);
}

.panel-header {
  display: flex;
  flex-direction: column;
  gap: 4px;
  margin-bottom: 16px;
}

.panel-header h2 {
  margin: 0;
  font-size: 20px;
  font-weight: 600;
  color: #f0f4ff;
}

.panel-header p {
  margin: 0;
  font-size: 12px;
  color: #a6b7ff;
}

.module-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
  gap: 14px;
}

.module-card {
  position: relative;
  border-radius: 14px;
  padding: 16px;
  background: rgba(17, 26, 62, 0.92);
  border: 1px solid rgba(102, 128, 224, 0.28);
  display: flex;
  flex-direction: column;
  gap: 12px;
  transition: border-color 0.3s ease, box-shadow 0.3s ease;
}

.module-head {
  display: flex;
  justify-content: space-between;
  align-items: flex-start;
}

.module-head h3 {
  margin: 0;
  font-size: 18px;
  font-weight: 600;
  color: #f6f8ff;
}

.module-sub {
  font-size: 12px;
  color: #aebaf8;
}

.module-body {
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.module-meta {
  display: flex;
  flex-direction: column;
  gap: 4px;
  font-size: 12px;
  color: #bec8ff;
}

.module-actions {
  display: flex;
  justify-content: flex-start;
}

.dll-select,
.stage-select {
  width: 100%;
}

.stage-table {
  margin-top: 18px;
}

.stage-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 10px;
}

.stage-header h3 {
  margin: 0;
  font-size: 16px;
  font-weight: 600;
}

.camera-card {
  border-radius: 14px;
  border: 1px solid rgba(102, 128, 224, 0.28);
  background: rgba(16, 26, 62, 0.88);
  padding: 16px 18px;
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.camera-head {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  align-items: center;
  font-size: 13px;
  color: #cbd5ff;
}

.camera-ip {
  font-family: 'JetBrains Mono', Consolas, monospace;
  font-size: 12px;
  padding: 2px 6px;
  border-radius: 6px;
  background: rgba(45, 59, 118, 0.65);
}

.camera-body {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.camera-body p {
  margin: 0;
  font-size: 13px;
  color: #d4dcff;
}

.camera-footer {
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-size: 12px;
  color: #c7d2ff;
  gap: 12px;
}

.camera-footer .el-button-group {
  flex-shrink: 0;
}

.accent-connected {
  border-color: rgba(96, 211, 163, 0.45);
  box-shadow: 0 0 0 1px rgba(96, 211, 163, 0.25);
}

.accent-disconnected {
  border-color: rgba(214, 92, 92, 0.45);
  box-shadow: 0 0 0 1px rgba(214, 92, 92, 0.25);
}

.accent-initializing {
  border-color: rgba(214, 167, 92, 0.5);
  box-shadow: 0 0 0 1px rgba(214, 167, 92, 0.3);
}

.accent-standby {
  border-color: rgba(114, 149, 241, 0.45);
  box-shadow: 0 0 0 1px rgba(114, 149, 241, 0.3);
}

.camera-tabs :deep(.el-tabs__header) {
  margin-bottom: 12px;
}

.camera-tabs :deep(.el-tabs__item) {
  color: #cdd6ff;
}

.camera-tabs :deep(.is-active) {
  color: #fff;
}

:deep(.el-table) {
  background: transparent;
  color: #e8edff;
}

:deep(.el-table__header-wrapper th) {
  background: rgba(19, 30, 70, 0.95);
  color: #e0e6ff;
}

:deep(.el-table__row) {
  background: rgba(14, 23, 58, 0.85);
}

:deep(.el-table--border .el-table__cell) {
  border-color: rgba(80, 102, 176, 0.35);
}

:deep(.el-button.is-plain) {
  background-color: rgba(18, 32, 80, 0.9);
  border-color: rgba(102, 128, 224, 0.45);
  color: #e3eaff;
}

:deep(.el-select .el-input__wrapper) {
  background: rgba(18, 32, 78, 0.85);
  border-color: rgba(102, 128, 224, 0.45);
  box-shadow: none;
}

:deep(.el-input__inner) {
  color: #f5f7ff;
}

:deep(.el-button-group .el-button + .el-button) {
  margin-left: 0;
}

@media (max-width: 1420px) {
  .machine-config-layout {
    grid-template-columns: 1fr;
    padding-bottom: 120px;
  }

  .camera-footer {
    flex-direction: column;
    align-items: flex-start;
  }
}
</style>
