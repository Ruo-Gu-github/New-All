<template>
  <div class="calibration-container">
    <section class="panel control-panel">
      <header class="panel-header">
        <h2>参数校准</h2>
        <p>配置校准参数并输出矫正结果</p>
      </header>

      <div class="panel-body">
        <div class="form-row">
          <span class="row-label">计算模式</span>
          <el-radio-group v-model="calibrationMode" size="small">
            <el-radio-button label="single">单小球计算</el-radio-button>
            <el-radio-button label="eight">八小球计算</el-radio-button>
          </el-radio-group>
        </div>

        <div class="form-row">
          <span class="row-label">校正图像</span>
          <el-upload
            class="upload-box"
            accept=".dcm,.dicom,.tif,.tiff,.png"
            :file-list="calibrationFiles"
            :auto-upload="false"
            :limit="1"
            @change="handleCalibrationFileChange"
            @remove="handleCalibrationFileRemove"
          >
            <el-button size="small" type="primary" plain>加载矫正图像</el-button>
          </el-upload>
        </div>

        <div class="form-row slider-row">
          <span class="row-label">二值化</span>
          <el-slider v-model="calibrationBinary" :min="0" :max="255" :step="1" />
          <span class="slider-value">{{ calibrationBinary }}</span>
        </div>

        <div class="form-row">
          <el-button
            type="success"
            size="small"
            :loading="calibrationLoading"
            :disabled="!calibrationFiles.length"
            @click="handleCalibrationCompute"
          >计算</el-button>
          <el-button size="small" plain @click="resetCalibration" :disabled="!calibrationFiles.length">重置</el-button>
        </div>

        <p class="panel-hint">提示：加载 DICOM 文件后，可在右侧视图执行缩放、拖动与 ROI 标记操作。</p>
      </div>
    </section>

    <section class="panel preview-panel">
      <header class="panel-header">
        <h2>DICOM 预览</h2>
        <p v-if="dicomLoaded">已加载校正影像，可进行交互操作</p>
        <p v-else>请先加载待校正的 DICOM 或影像文件</p>
      </header>
      <div class="preview-stage" :class="{ 'preview-stage--loaded': dicomLoaded }">
        <div v-if="dicomLoaded" class="preview-placeholder">DICOM 图像预览区域</div>
        <div v-else class="preview-placeholder preview-placeholder--empty">
          预览区域：等待加载影像文件
        </div>
      </div>
      <ul class="interaction-hints">
        <li>滚轮：缩放图像</li>
        <li>右键拖动：平移视图</li>
        <li>Shift + 左键：框选 ROI</li>
      </ul>
    </section>
  </div>
</template>

<script setup>
import { ref } from 'vue';
import { ElMessage } from 'element-plus';

const calibrationMode = ref('single');
const calibrationBinary = ref(128);
const calibrationFiles = ref([]);
const calibrationLoading = ref(false);
const dicomLoaded = ref(false);

function handleCalibrationFileChange(uploadFile, uploadFiles) {
  calibrationFiles.value = uploadFiles;
  dicomLoaded.value = uploadFiles.length > 0;
  if (uploadFile?.name) {
    ElMessage.success(`已加载矫正图像：${uploadFile.name}`);
  }
}

function handleCalibrationFileRemove() {
  calibrationFiles.value = [];
  dicomLoaded.value = false;
  ElMessage.info('已移除矫正图像');
}

function handleCalibrationCompute() {
  if (!calibrationFiles.value.length || calibrationLoading.value) return;
  calibrationLoading.value = true;
  ElMessage.info('正在计算校准参数...');
  setTimeout(() => {
    calibrationLoading.value = false;
    const modeLabel = calibrationMode.value === 'single' ? '单小球' : '八小球';
    ElMessage.success(`${modeLabel}校准完成，结果已生成`);
  }, 1600);
}

function resetCalibration() {
  calibrationFiles.value = [];
  dicomLoaded.value = false;
  calibrationBinary.value = 128;
  ElMessage.success('已重置校准配置');
}
</script>

<style scoped>
.calibration-container {
  width: 100%;
  height: 100%;
  display: grid;
  grid-template-columns: minmax(300px, 380px) minmax(0, 1fr);
  gap: 18px;
  padding: 20px 24px;
  box-sizing: border-box;
  background: linear-gradient(180deg, #0e183e 0%, #070d26 100%);;
  color: #e8edff;
}

.panel {
  background: rgba(19, 30, 72, 0.92);
  border: 1px solid rgba(94, 118, 210, 0.32);
  border-radius: 16px;
  padding: 18px 20px;
  box-shadow: 0 16px 32px rgba(5, 12, 38, 0.45);
  display: flex;
  flex-direction: column;
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
  color: #f4f7ff;
}

.panel-header p {
  margin: 0;
  font-size: 12px;
  color: #a6b7ff;
}

.panel-body {
  display: flex;
  flex-direction: column;
  gap: 14px;
}

.form-row {
  display: flex;
  align-items: center;
  gap: 12px;
  flex-wrap: wrap;
}

.row-label {
  min-width: 80px;
  font-size: 13px;
  color: #c5d1ff;
}

.slider-row {
  align-items: center;
}

.slider-row :deep(.el-slider) {
  flex: 1;
  margin-right: 8px;
}

.slider-value {
  font-size: 12px;
  color: #9fb6ff;
  min-width: 34px;
}

.panel-hint {
  margin: 8px 0 0;
  font-size: 12px;
  color: #96abff;
}

.upload-box :deep(.el-upload-list) {
  display: none;
}

.preview-stage {
  flex: 1;
  border-radius: 14px;
  border: 1px dashed rgba(114, 138, 228, 0.45);
  background: rgba(12, 21, 56, 0.88);
  display: flex;
  align-items: center;
  justify-content: center;
  transition: border-color 0.3s ease, background 0.3s ease;
}

.preview-stage--loaded {
  border-style: solid;
  border-color: rgba(110, 230, 190, 0.6);
  background: rgba(16, 27, 68, 0.92);
}

.preview-placeholder {
  font-size: 14px;
  color: #d3dbff;
  padding: 24px;
  text-align: center;
}

.preview-placeholder--empty {
  color: #8e9fde;
}

.interaction-hints {
  margin: 12px 0 0;
  padding-left: 20px;
  font-size: 12px;
  color: #a8b9ff;
  display: flex;
  flex-direction: column;
  gap: 4px;
}

@media (max-width: 1320px) {
  .calibration-container {
    grid-template-columns: 1fr;
  }
}
</style>
