<template>
  <div class="sample-shoot-layout">
    <div class="side-column left-column">
      <section class="panel panel-basic">
        <div class="panel-header">
          <h2>基本信息</h2>
          <p>当前计划的扫描参数</p>
        </div>
        <div class="info-lines">
          <div class="info-line">
            <span class="info-label">组织/单位</span>
            <span class="info-value">{{ baseInfo.organization || '—' }}</span>
          </div>
          <div class="info-line">
            <span class="info-label">样品名称</span>
            <span class="info-value">{{ baseInfo.sampleName || '—' }}</span>
          </div>
          <div class="info-line">
            <span class="info-label">样品类型</span>
            <span class="info-value">{{ baseInfo.type || '—' }}</span>
          </div>
          <div class="info-line">
            <span class="info-label">电压/电流</span>
            <span class="info-value">{{ baseInfo.power || '—' }}</span>
          </div>
          <div class="info-line">
            <span class="info-label">FOV</span>
            <span class="info-value">{{ baseInfo.fov || '—' }}</span>
          </div>
          <div class="info-line">
            <span class="info-label">Binning</span>
            <span class="info-value">{{ baseInfo.binning || '—' }}</span>
          </div>
          <div class="info-line">
            <span class="info-label">协议</span>
            <span class="info-value">{{ baseInfo.protocol || '—' }}</span>
          </div>
        </div>
        <div class="info-actions">
          <el-button type="primary" size="small" @click="openNewScanDialog">新建扫描</el-button>
        </div>
      </section>

      <section class="panel panel-mechanical">
        <div class="panel-header">
          <h2>机械控制</h2>
          <p>转台和平移平台快速调整</p>
        </div>
        <div class="motion-table">
          <div class="motion-row">
            <div class="motion-table">
              <div class="motion-row">
                <div class="row-label">转台</div>
                <div class="row-controls">
                  <el-button-group size="small">
                    <el-button type="primary" plain @click="spinTurn('cw')">顺时针</el-button>
                    <el-button @click="resetTurn">复位</el-button>
                    <el-button type="primary" plain @click="spinTurn('ccw')">逆时针</el-button>
                  </el-button-group>
                </div>
              </div>

              <div
                v-for="axis in stageAxes"
                :key="axis.axis"
                class="motion-row"
                :class="{ disabled: !axis.active }"
              >
                <div class="row-label">{{ axis.axis }} 轴</div>
                <div class="row-controls">
                  <el-button-group size="small">
                    <el-button
                      type="primary"
                      plain
                      :disabled="!axis.active"
                      @click="moveAxis(axis, 'forward')"
                    >前进</el-button>
                    <el-button :disabled="!axis.active" @click="resetAxis(axis)">复位</el-button>
                    <el-button
                      type="primary"
                      plain
                      :disabled="!axis.active"
                      @click="moveAxis(axis, 'backward')"
                    >后退</el-button>
                  </el-button-group>
                </div>
              </div>
            </div>
          </div>
        </div>
      </section>

      <section class="panel panel-segments">
        <div class="panel-header">
          <h2>扫描控制</h2>
          <p>接段计划与执行记录</p>
        </div>
        <div class="segment-actions">
          <el-button size="small" type="primary" plain @click="handleSegmentScan">新增接段</el-button>
          <el-button size="small" type="success" plain @click="openScanDialog">开始扫描</el-button>
        </div>
      </section>
    </div>

    <div class="center-panel view-panel">
      <header class="view-header">
        <div>
          <div class="view-title">实时预览</div>
          <div class="view-subtitle">同步显示当前扫描画面</div>
        </div>
        <div class="view-actions">
          <el-button size="small" type="primary" plain :loading="dicomLoading" @click="triggerDicomSelect">加载 DICOM</el-button>
          <el-button size="small" type="danger" plain :disabled="!dicomLoaded" @click="clearDicom">清空</el-button>
          <el-button size="small" @click="toggleOverlay">{{ overlayEnabled ? '隐藏辅助线' : '显示辅助线' }}</el-button>
          <el-button size="small" plain type="primary">截图</el-button>
          <el-button size="small" plain type="success">导出</el-button>
        </div>
      </header>
      <div class="view-screen">
        <input
          ref="dicomFileInputRef"
          class="dicom-file-input"
          type="file"
          multiple
          accept=".dcm,.dicom"
          @change="handleDicomFileChange"
        />
        <div class="view-placeholder">
          <img v-if="dicomPreview" :src="dicomPreview" alt="DICOM预览" class="preview-image" />
          <div v-else class="view-empty">尚未加载 DICOM 图像</div>
          <span class="overlay-label" v-if="overlayEnabled">辅助线已开启</span>
        </div>
      </div>
      <div class="dicom-summary" v-if="dicomInfo">
        <div class="summary-item"><span class="summary-label">样品</span><span class="summary-value">{{ dicomInfo.sampleName }}</span></div>
        <div class="summary-item"><span class="summary-label">图像尺寸</span><span class="summary-value">{{ dicomInfo.width }} × {{ dicomInfo.height }}</span></div>
        <div class="summary-item"><span class="summary-label">切片数</span><span class="summary-value">{{ dicomInfo.sliceCount }}</span></div>
        <div class="summary-item"><span class="summary-label">像素间距</span><span class="summary-value">{{ formatPixelSpacing(dicomInfo.pixelSpacing) }}</span></div>
      </div>
      <div class="scan-console">
        <div class="console-block">
          <div class="console-label">缩放</div>
          <div class="console-value">{{ viewState.zoom.toFixed(1) }}x</div>
        </div>
        <div class="console-block">
          <div class="console-label">曝光</div>
          <div class="console-value">{{ viewState.exposure.toFixed(0) }} ms</div>
        </div>
        <div class="console-block">
          <div class="console-label">帧率</div>
          <div class="console-value">{{ viewState.fps.toFixed(1) }} fps</div>
        </div>
        <div class="console-actions">
          <el-button type="primary" size="small" plain @click="openScanDialog">开始扫描</el-button>
          <el-button type="danger" size="small" plain>紧急停止</el-button>
        </div>
      </div>
    </div>

    <div class="side-column right-column">
      <section class="panel panel-status" :class="`status-${sourceStatusClass}`">
        <div class="status-chip">{{ sourceStatus }}</div>
        <p class="status-description">{{ sourceStatusDescription }}</p>
        <div class="status-inline">
          <span class="status-time">更新 {{ lastStatusUpdate }}</span>
          <el-button
            v-if="sourceActionLabel"
            :type="sourceActionType"
            plain
            size="small"
            @click="handleSourceAction"
          >{{ sourceActionLabel }}</el-button>
          <span v-else class="status-placeholder">无可执行操作</span>
        </div>
      </section>

      <section class="panel panel-monitor">
        <div class="panel-header">
          <h2>内部监控</h2>
          <p>实时查看舱内摄像头</p>
        </div>
        <div class="monitor-window">
          <div class="monitor-placeholder">摄像头画面预留区域</div>
        </div>
      </section>

      <section class="panel panel-quick">
        <div class="panel-header">
          <h2>常用功能</h2>
        </div>
        <div class="quick-actions">
          <el-button type="warning" plain @click="handleQuickReset">一键复位</el-button>
          <el-button type="success" plain @click="handleDeviceCheck">设备检查</el-button>
          <el-button type="primary" plain @click="openCommandExecutor">命令执行</el-button>
        </div>
      </section>
    </div>

    <div class="log-panel">
      <div class="log-content">
        <span>{{ logMsg }}</span>
        <div class="log-controls">
          <span>最新记录</span>
          <el-tag size="small" type="info">运行正常</el-tag>
        </div>
      </div>
    </div>

    <el-dialog
      v-model="newScanDialogVisible"
      title="新建扫描"
      width="480px"
      :close-on-click-modal="false"
    >
      <el-form :model="newScanForm" label-width="90px" class="scan-form">
        <el-form-item label="组织/单位">
          <el-input v-model="newScanForm.organization" placeholder="组织/单位" clearable />
        </el-form-item>
        <el-form-item label="样品名称">
          <el-input v-model="newScanForm.sampleName" placeholder="样品名称" clearable />
        </el-form-item>
        <el-form-item label="样品类型">
          <el-select
            v-model="newScanForm.type"
            filterable
            allow-create
            default-first-option
            placeholder="样品类型"
            clearable
          >
            <el-option v-for="item in typeOptions" :key="item" :label="item" :value="item" />
          </el-select>
        </el-form-item>
        <el-form-item label="电压/电流">
          <el-select v-model="newScanForm.power" placeholder="电压 / 电流" clearable>
            <el-option v-for="item in powerOptions" :key="item" :label="item" :value="item" />
          </el-select>
        </el-form-item>
        <el-form-item label="FOV">
          <el-select v-model="newScanForm.fov" placeholder="FOV" clearable>
            <el-option v-for="item in fovOptions" :key="item" :label="item" :value="item" />
          </el-select>
        </el-form-item>
        <el-form-item label="Binning">
          <el-select v-model="newScanForm.binning" placeholder="Binning" clearable>
            <el-option v-for="item in binningOptions" :key="item" :label="item" :value="item" />
          </el-select>
        </el-form-item>
        <el-form-item label="协议">
          <el-select v-model="newScanForm.protocol" placeholder="协议" clearable>
            <el-option v-for="item in protocolOptions" :key="item" :label="item" :value="item" />
          </el-select>
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="closeNewScanDialog">取消</el-button>
        <el-button
          type="primary"
          :disabled="!newScanForm.organization || !newScanForm.sampleName"
          @click="confirmNewScan"
        >保存</el-button>
      </template>
    </el-dialog>

    <el-dialog v-model="scanDialogVisible" title="开始扫描" width="420px" :close-on-click-modal="false">
      <div class="scan-summary-text">
        <p>确认使用当前参数开始扫描：</p>
        <ul>
          <li>样品：{{ baseInfo.sampleName || '未命名样本' }}</li>
          <li>类型：{{ baseInfo.type || '未填写' }}</li>
          <li>协议：{{ baseInfo.protocol || '未选择' }}</li>
        </ul>
      </div>
      <el-form :model="scanForm" label-width="80px" class="scan-form">
        <el-form-item label="样本名">
          <el-input v-model="scanForm.sampleName" placeholder="请输入样本名称" />
        </el-form-item>
        <el-form-item label="备注">
          <el-input v-model="scanForm.remark" type="textarea" rows="2" placeholder="可选" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="scanDialogVisible = false">取消</el-button>
        <el-button type="primary" :disabled="!scanForm.sampleName" @click="confirmScan">确定</el-button>
      </template>
    </el-dialog>

    <CommandExecutorDialog v-model="commandDialogVisible" />
  </div>
</template>
<script setup>
import { computed, reactive, ref } from 'vue';
import CommandExecutorDialog from './CommandExecutorDialog.vue';
import { ElMessage } from 'element-plus';
import { loadDicomSeries } from '../services/dicomLoader';

import config from '../config/scan-options.json';
// command definitions are handled inside CommandExecutorDialog now

const baseInfo = reactive({
  organization: '',
  sampleName: '',
  type: '',
  power: '',
  fov: '',
  binning: '',
  protocol: '',
});

const newScanDialogVisible = ref(false);
const newScanForm = reactive({
  organization: '',
  sampleName: '',
  type: '',
  power: '',
  fov: '',
  binning: '',
  protocol: '',
});

const typeOptions = ref(config.scanOptions.typeOptions);
const powerOptions = ref(config.scanOptions.powerOptions);
const fovOptions = ref(config.scanOptions.fovOptions);
const binningOptions = ref(config.scanOptions.binningOptions);
const protocolOptions = ref(config.scanOptions.protocolOptions);

function resetNewScanForm() {
  newScanForm.organization = '';
  newScanForm.sampleName = '';
  newScanForm.type = '';
  newScanForm.power = '';
  newScanForm.fov = '';
  newScanForm.binning = '';
  newScanForm.protocol = '';
}

function openNewScanDialog() {
  resetNewScanForm();
  newScanDialogVisible.value = true;
}

function closeNewScanDialog() {
  newScanDialogVisible.value = false;
  resetNewScanForm();
}

function confirmNewScan() {
  baseInfo.organization = newScanForm.organization;
  baseInfo.sampleName = newScanForm.sampleName;
  baseInfo.type = newScanForm.type;
  baseInfo.power = newScanForm.power;
  baseInfo.fov = newScanForm.fov;
  baseInfo.binning = newScanForm.binning;
  baseInfo.protocol = newScanForm.protocol;

  if (newScanForm.type && !typeOptions.value.includes(newScanForm.type)) {
    typeOptions.value.push(newScanForm.type);
  }

  logMsg.value = `已更新扫描信息：${baseInfo.sampleName || '未命名样本'}`;
  ElMessage.success('扫描基本信息已更新');
  newScanDialogVisible.value = false;
  resetNewScanForm();
}

const turntable = reactive({
  angle: 0,
});

function spinTurn(direction) {
  const step = 1.5;
  if (direction === 'cw') {
    turntable.angle = Number((turntable.angle + step).toFixed(1));
    logMsg.value = '转台顺时针微调';
  } else if (direction === 'ccw') {
    turntable.angle = Number((turntable.angle - step).toFixed(1));
    logMsg.value = '转台逆时针微调';
  }
}

function resetTurn() {
  turntable.angle = 0;
  logMsg.value = '转台已复位';
}

const stageAxes = reactive([
  { axis: 'X', active: true, position: 0, step: 0.2 },
  { axis: 'Y', active: true, position: 0, step: 0.2 },
  { axis: 'Z', active: false, position: 0, step: 0.2 },
]);

function moveAxis(axis, direction) {
  if (!axis.active) return;
  const delta = direction === 'forward' ? axis.step : -axis.step;
  axis.position = Number((axis.position + delta).toFixed(2));
  logMsg.value = `${axis.axis} 轴${direction === 'forward' ? '前进' : '后退'}微调`;
}

function resetAxis(axis) {
  if (!axis.active) return;
  axis.position = 0;
  logMsg.value = `${axis.axis} 轴已复位`;
}

const segmentSequence = ref(3);

function handleSegmentScan() {
  segmentSequence.value += 1;
  logMsg.value = `已创建新的接段任务 S${segmentSequence.value}`;
  ElMessage.info('已创建新的接段任务');
}

const overlayEnabled = ref(false);
const viewState = reactive({
  zoom: 1.0,
  exposure: 60,
  fps: 24,
});

const dicomInfo = ref(null);
const dicomPreview = ref('');
const dicomLoading = ref(false);
const dicomFileInputRef = ref(null);

const dicomLoaded = computed(() => Boolean(dicomInfo.value));

function triggerDicomSelect() {
  dicomFileInputRef.value?.click();
}

function resetDicomInput() {
  if (dicomFileInputRef.value) {
    dicomFileInputRef.value.value = '';
  }
}

async function handleDicomFileChange(event) {
  const input = event.target;
  const files = input && input.files ? Array.from(input.files) : [];
  if (!files.length) {
    resetDicomInput();
    return;
  }
  dicomLoading.value = true;
  try {
    const series = await loadDicomSeries(files);
    dicomInfo.value = series;
    dicomPreview.value = series.thumbnail;
    if (series.sampleName) {
      baseInfo.sampleName = series.sampleName;
    }
    logMsg.value = `已加载 DICOM 序列：${series.sampleName}`;
    ElMessage.success('DICOM 序列加载完成');
  } catch (error) {
    console.error('[SampleShoot] 加载 DICOM 失败', error);
    dicomInfo.value = null;
    dicomPreview.value = '';
  } finally {
    dicomLoading.value = false;
    resetDicomInput();
  }
}

function clearDicom() {
  dicomInfo.value = null;
  dicomPreview.value = '';
  resetDicomInput();
  logMsg.value = '已清空 DICOM 预览';
  ElMessage.info('已清空 DICOM 数据');
}

function formatPixelSpacing(spacing) {
  if (!spacing || spacing.length < 2) return '未知';
  const [sx, sy] = spacing;
  if (!Number.isFinite(sx) || !Number.isFinite(sy)) {
    return '未知';
  }
  return `${sx.toFixed(3)} mm × ${sy.toFixed(3)} mm`;
}

function toggleOverlay() {
  overlayEnabled.value = !overlayEnabled.value;
}

const scanDialogVisible = ref(false);
const scanForm = reactive({
  sampleName: '',
  remark: '',
});

function openScanDialog() {
  scanForm.sampleName = baseInfo.sampleName;
  scanForm.remark = '';
  scanDialogVisible.value = true;
}

function confirmScan() {
  scanDialogVisible.value = false;
  logMsg.value = `开始扫描：${scanForm.sampleName}`;
  ElMessage.success(`已开始扫描：${scanForm.sampleName}`);
}

const sourceStatus = ref('未连接');
const lastStatusUpdate = ref(new Date().toLocaleString());

const sourceStatusMap = {
  未连接: {
    class: 'disconnected',
    description: '射线源未连接，请检查通讯和电源',
  },
  未预热: {
    class: 'idle',
    description: '射线源待机，建议先执行预热',
    actionLabel: '预热',
    actionType: 'warning',
    next: '预热中',
  },
  预热中: {
    class: 'heating',
    description: '射线源正在预热，请勿靠近舱体',
    actionLabel: '关闭',
    actionType: 'danger',
    next: '射线源关闭',
  },
  射线源开启: {
    class: 'active',
    description: '射线源开启，确保安全联锁生效',
    actionLabel: '关闭',
    actionType: 'danger',
    next: '射线源关闭',
  },
  射线源关闭: {
    class: 'inactive',
    description: '射线源关闭，可安全进入舱体',
    actionLabel: '打开',
    actionType: 'primary',
    next: '射线源开启',
  },
  舱门开启: {
    class: 'door',
    description: '舱门打开，确认联锁状态',
  },
};

const sourceStatusClass = computed(() => sourceStatusMap[sourceStatus.value]?.class ?? 'disconnected');
const sourceStatusDescription = computed(() => sourceStatusMap[sourceStatus.value]?.description ?? '');
const sourceActionLabel = computed(() => sourceStatusMap[sourceStatus.value]?.actionLabel ?? '');
const sourceActionType = computed(() => sourceStatusMap[sourceStatus.value]?.actionType ?? 'primary');

function updateSourceStatus(nextStatus) {
  if (!nextStatus) return;
  sourceStatus.value = nextStatus;
  lastStatusUpdate.value = new Date().toLocaleString();
}

function handleSourceAction() {
  const meta = sourceStatusMap[sourceStatus.value];
  if (!meta?.next) return;
  updateSourceStatus(meta.next);
  ElMessage.success(`射线源状态变更：${meta.next}`);
}

function handleQuickReset() {
  resetTurn();
  stageAxes.forEach((axis) => {
    axis.position = 0;
    axis.targetInput = '';
  });
  logMsg.value = '系统已执行一键复位';
  ElMessage.success('设备已执行一键复位');
}

function handleDeviceCheck() {
  logMsg.value = '正在执行设备检查...';
  ElMessage.info('正在执行设备检查...');
}

const commandDialogVisible = ref(false);

function openCommandExecutor() {
  commandDialogVisible.value = true;
}


const logMsg = ref('系统日志：启动成功');
</script>

<style scoped>
/* 覆盖 Element Plus 弹窗根节点背景色 */

/* 常用功能按钮自动换行和宽度统一 */
.quick-actions {
  display: flex;
  flex-wrap: wrap;
  gap: 12px;
  justify-content: start; 
  align-items: center;
}

.quick-actions :deep(.el-button) {
  min-width: 100px;
  margin-left: 0 !important;
}

.sample-shoot-layout {
  width: 100%;
  height: 100%;
  display: grid;
  grid-template-columns: 300px minmax(560px, 1fr) 300px;
  gap: 16px;
  padding: 18px 24px 80px;
  box-sizing: border-box;
  position: relative;
  background: linear-gradient(180deg, #0e183e 0%, #070d26 100%);
  color: #e8edff;
}

.side-column {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.panel {
  background: rgba(21, 32, 70, 0.9);
  border: 1px solid rgba(92, 116, 209, 0.35);
  border-radius: 14px;
  padding: 14px 16px 16px;
  backdrop-filter: blur(10px);
  box-shadow: 0 12px 24px rgba(3, 10, 32, 0.45);
}

.panel-header {
  display: flex;
  flex-direction: column;
  gap: 2px;
  margin-bottom: 12px;
}

.panel-header h2 {
  margin: 0;
  font-size: 18px;
  font-weight: 600;
  color: #f0f4ff;
}

.panel-header p {
  margin: 0;
  font-size: 12px;
  color: #a6b7ff;
}

.info-lines {
  display: flex;
  flex-direction: column;
  gap: 6px;
  padding: 4px 0;
}

.info-line {
  display: grid;
  grid-template-columns: 92px 1fr;
  align-items: center;
  gap: 12px;
  padding: 4px 12px;
  border-radius: 8px;
  background: rgba(16, 28, 60, 0.65);
}

.info-label {
  font-size: 12px;
  color: #97a8e3;
  text-align: left;
}

.info-value {
  font-size: 14px;
  font-weight: 500;
  color: #f4f6ff;
  text-align: right;
}

.info-actions {
  margin-top: 12px;
  display: flex;
  gap: 10px;
}

.compact-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(160px, 1fr));
  gap: 8px 12px;
}

.new-scan-actions {
  margin-top: 12px;
  display: flex;
  justify-content: flex-end;
  gap: 10px;
}

.motion-table {
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.motion-row {
  display: grid;
  grid-template-columns: 48px 1fr;
  align-items: center;
  gap: 6px;
  padding: 8px 8px;
  border: 1px solid rgba(100, 124, 214, 0.28);
  border-radius: 10px;
  background: rgba(14, 26, 62, 0.9);
}

.motion-row.disabled {
  opacity: 0.55;
}

.row-label {
  font-size: 14px;
  font-weight: 600;
  color: #e0e6ff;
}

.row-controls {
  display: flex;
  justify-content: flex-start;
}

.row-controls :deep(.el-button-group) {
  display: inline-flex;
}

.row-controls :deep(.el-button) {
  min-width: 60px;
}

.segment-actions {
  display: flex;
  justify-content: flex-start;
  gap: 10px;
}

.center-panel {
  background: rgba(19, 31, 72, 0.92);
  border: 1px solid rgba(102, 127, 223, 0.32);
  border-radius: 18px;
  padding: 18px 20px;
  display: flex;
  flex-direction: column;
  gap: 16px;
  box-shadow: 0 18px 32px rgba(3, 12, 40, 0.55);
}

.view-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.view-title {
  font-size: 20px;
  font-weight: 600;
  color: #f7f9ff;
}

.view-subtitle {
  font-size: 12px;
  color: #b9c4ff;
}

.view-actions {
  display: flex;
  gap: 10px;
}

.view-screen {
  flex: 1;
  background: radial-gradient(circle at center, rgba(87, 120, 206, 0.4), rgba(12, 23, 55, 0.95));
  border-radius: 16px;
  display: flex;
  align-items: center;
  justify-content: center;
  border: 1px solid rgba(108, 132, 223, 0.25);
  position: relative;
}

.view-placeholder {
  text-align: center;
  font-size: 16px;
  color: #dfe7ff;
  display: flex;
  flex-direction: column;
  gap: 8px;
  flex: 1;
  justify-content: center;
  align-items: center;
  width: 100%;
  height: 100%;
  position: relative;
}

.dicom-file-input {
  display: none;
}

.preview-image {
  max-width: 100%;
  max-height: 100%;
  border-radius: 12px;
  box-shadow: 0 12px 24px rgba(6, 12, 32, 0.55);
  border: 1px solid rgba(140, 178, 255, 0.35);
}

.view-empty {
  font-size: 14px;
  color: rgba(213, 224, 255, 0.8);
  letter-spacing: 0.5px;
  padding: 12px 18px;
  border-radius: 12px;
  background: rgba(16, 34, 72, 0.78);
  border: 1px dashed rgba(116, 148, 230, 0.45);
}

.overlay-label {
  padding: 4px 10px;
  border-radius: 999px;
  background: rgba(84, 123, 255, 0.75);
  font-size: 12px;
  position: absolute;
  left: 16px;
  top: 16px;
}

.dicom-summary {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(160px, 1fr));
  gap: 10px 14px;
  padding: 12px 16px;
  border-radius: 12px;
  background: rgba(15, 30, 70, 0.78);
  border: 1px solid rgba(120, 146, 238, 0.28);
}

.summary-item {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.summary-label {
  font-size: 12px;
  color: rgba(189, 204, 255, 0.75);
}

.summary-value {
  font-size: 16px;
  font-weight: 600;
  color: #f4f7ff;
}

.scan-console {
  display: grid;
  grid-template-columns: repeat(3, minmax(120px, 1fr)) auto;
  gap: 12px;
  padding: 12px 16px;
  border: 1px solid rgba(104, 130, 230, 0.35);
  border-radius: 14px;
  background: rgba(14, 27, 66, 0.85);
}

.console-block {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.console-label {
  font-size: 12px;
  color: #b2c1ff;
}

.console-value {
  font-size: 18px;
  font-weight: 600;
  color: #f7f9ff;
}

.console-actions {
  display: flex;
  align-items: center;
  gap: 8px;
}


.panel-status {
  display: flex;
  flex-direction: column;
  gap: 10px;
  padding: 16px;
  border-radius: 16px;
  border: 1px solid transparent;
  box-shadow: inset 0 0 0 1px rgba(255, 255, 255, 0.04);
}

.panel-status.status-disconnected {
  background: linear-gradient(135deg, #1f1f24 0%, #060608 100%);
  border-color: rgba(142, 151, 168, 0.25);
}

.panel-status.status-idle {
  background: linear-gradient(135deg, #253250 0%, #1a2140 100%);
  border-color: rgba(118, 145, 238, 0.3);
}

.panel-status.status-heating {
  background: linear-gradient(135deg, #6a1a1a 0%, #c62828 100%);
  border-color: rgba(255, 170, 136, 0.4);
}

.panel-status.status-active {
  background: linear-gradient(135deg, #7a1111 0%, #ff3b3b 100%);
  border-color: rgba(255, 197, 189, 0.45);
}

.panel-status.status-inactive {
  background: linear-gradient(135deg, #0f5127 0%, #10a74d 100%);
  border-color: rgba(144, 236, 181, 0.35);
}

.panel-status.status-door {
  background: linear-gradient(135deg, #7f5a00 0%, #ffbe20 100%);
  border-color: rgba(255, 233, 174, 0.45);
}

.status-chip {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  padding: 4px 14px;
  border-radius: 999px;
  background: rgba(0, 0, 0, 0.25);
  font-size: 14px;
  font-weight: 600;
  color: #ffffff;
}

.status-description {
  margin: 0;
  font-size: 13px;
  color: rgba(236, 241, 255, 0.85);
}

.status-inline {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

.status-time {
  font-size: 12px;
  color: rgba(240, 244, 255, 0.75);
}

.status-placeholder {
  font-size: 12px;
  color: rgba(240, 244, 255, 0.8);
}

.monitor-window {
  height: 180px;
  border-radius: 12px;
  border: 1px dashed rgba(150, 170, 255, 0.5);
  background: rgba(11, 19, 44, 0.6);
  display: flex;
  align-items: center;
  justify-content: center;
}

.monitor-placeholder {
  color: rgba(203, 214, 255, 0.85);
  font-size: 15px;
}


.log-panel {
  position: absolute;
  left: 24px;
  right: 24px;
  bottom: 20px;
  border-radius: 14px;
  background: rgba(9, 17, 44, 0.92);
  border: 1px solid rgba(86, 110, 210, 0.35);
  padding: 10px 18px;
  box-sizing: border-box;
  display: flex;
  align-items: center;
  color: #f6f8ff;
}

.log-content {
  width: 100%;
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.log-controls {
  display: flex;
  align-items: center;
  gap: 12px;
  font-size: 12px;
  color: #c5d2ff;
}

.log-slider {
  width: 160px;
}

.panel :deep(.el-form-item__label) {
  color: #dbe4ff;
}

.panel :deep(.el-input__wrapper),
.panel :deep(.el-select .el-input__wrapper),
.panel :deep(.el-input-number .el-input__wrapper) {
  background-color: rgba(17, 28, 68, 0.88);
  border-color: rgba(102, 128, 224, 0.45);
}

.panel :deep(.el-input__inner),
.panel :deep(.el-select__selected-item),
.panel :deep(.el-input-number__decrease),
.panel :deep(.el-input-number__increase) {
  color: #f5f7ff;
}

.panel :deep(.el-switch__label) {
  color: #cbd5ff;
}

.panel :deep(.el-button.is-plain) {
  background-color: rgba(18, 34, 78, 0.85);
  border-color: rgba(87, 119, 214, 0.5);
  color: #dfe6ff;
}

.panel :deep(.el-descriptions__body) {
  background: #eef3ff;
}

.panel :deep(.el-descriptions__label) {
  width: 100px;
  color: #1c2c58;
}

.panel :deep(.el-dialog__body) {
  background: rgba(14, 22, 52, 0.98);
}

.panel :deep(.el-dialog__header) {
  color: #e8edff;
}

/* 抽离后，命令执行器的样式已放到 CommandExecutorDialog.vue 中 */

@media (max-width: 1480px) {
  .sample-shoot-layout {
    grid-template-columns: 1fr;
    padding-bottom: 160px;
  }

  .side-column,
  .center-panel {
    width: 100%;
  }

  .log-panel {
    left: 16px;
    right: 16px;
  }
}
</style>
