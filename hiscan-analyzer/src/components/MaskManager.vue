<template>
  <div class="mask-manager">
    <ElCard class="manager-card" shadow="hover">
      <template #header>
        <div class="header-row">
          <span>Mask 管理器</span>
          <ElButton size="small" type="primary" @click="addMask">新建 Mask</ElButton>
        </div>
      </template>

      <!-- Mask 列表 -->
      <div class="mask-list">
        <div v-for="(mask, index) in masks" :key="mask.id" class="mask-item">
          <ElCheckbox v-model="mask.visible" @change="onVisibilityChange(index)">
            <span class="mask-name">{{ mask.name }}</span>
          </ElCheckbox>
          
          <div class="mask-controls">
            <!-- 颜色选择器 -->
            <el-color-picker 
              v-model="mask.color" 
              size="small"
              @change="onColorChange(index)"
              show-alpha
            />
            
            <!-- 操作按钮 -->
            <ElButton size="small" @click="cloneMask(index)" title="克隆">
              <DocumentCopy />
            </ElButton>
            <ElButton size="small" @click="deleteMask(index)" type="danger" title="删除">
              <Delete />
            </ElButton>
          </div>
        </div>
      </div>

      <ElDivider />

      <!-- Boolean 操作区 -->
      <div class="boolean-ops">
        <div class="section-title">布尔运算</div>
        <div class="select-row">
          <ElSelect v-model="selectedMaskA" placeholder="选择 Mask A" size="small">
            <ElOption v-for="(mask, idx) in masks" :key="idx" :label="mask.name" :value="idx" />
          </ElSelect>
          <ElSelect v-model="selectedMaskB" placeholder="选择 Mask B" size="small">
            <ElOption v-for="(mask, idx) in masks" :key="idx" :label="mask.name" :value="idx" />
          </ElSelect>
        </div>
        <div class="btn-row">
          <ElButton size="small" @click="booleanOp('union')">并集 (A ∪ B)</ElButton>
          <ElButton size="small" @click="booleanOp('intersection')">交集 (A ∩ B)</ElButton>
          <ElButton size="small" @click="booleanOp('difference')">差集 (A - B)</ElButton>
          <ElButton size="small" @click="booleanOp('xor')">异或 (A ⊕ B)</ElButton>
        </div>
      </div>

      <ElDivider />

      <!-- 连通域操作 -->
      <div class="connected-components">
        <div class="section-title">连通域</div>
        <div class="btn-row">
          <ElButton size="small" @click="enableFloodFill">点击选择连通域</ElButton>
          <ElButton size="small" @click="keepLargestRegion">保留最大连通域</ElButton>
          <ElButton size="small" @click="removeSmallRegions">移除小区域</ElButton>
        </div>
      </div>

      <ElDivider />

      <!-- 形态学操作 -->
      <div class="morphology-ops">
        <div class="section-title">形态学</div>
        <div class="slider-row">
          <span>半径:</span>
          <ElSlider v-model="morphRadius" :min="1" :max="10" show-input size="small" />
        </div>
        <div class="btn-row">
          <ElButton size="small" @click="morphOp('dilate')">膨胀</ElButton>
          <ElButton size="small" @click="morphOp('erode')">腐蚀</ElButton>
          <ElButton size="small" @click="morphOp('opening')">开运算</ElButton>
          <ElButton size="small" @click="morphOp('closing')">闭运算</ElButton>
        </div>
      </div>

      <ElDivider />

      <!-- ROI 绘制工具 -->
      <div class="roi-tools">
        <div class="section-title">ROI 绘制</div>
        <div class="btn-row">
          <ElButton 
            size="small" 
            :type="currentTool === 'rectangle' ? 'primary' : ''" 
            @click="selectTool('rectangle')"
          >
            矩形
          </ElButton>
          <ElButton 
            size="small" 
            :type="currentTool === 'circle' ? 'primary' : ''" 
            @click="selectTool('circle')"
          >
            圆形
          </ElButton>
          <ElButton 
            size="small" 
            :type="currentTool === 'polygon' ? 'primary' : ''" 
            @click="selectTool('polygon')"
          >
            多边形
          </ElButton>
          <ElButton 
            size="small" 
            :type="currentTool === 'brush' ? 'primary' : ''" 
            @click="selectTool('brush')"
          >
            画笔
          </ElButton>
        </div>
        <div class="slider-row" v-if="currentTool === 'brush'">
          <span>画笔大小:</span>
          <ElSlider v-model="brushSize" :min="1" :max="50" show-input size="small" />
        </div>
      </div>

      <ElDivider />

      <!-- 阈值分割 -->
      <div class="threshold-section">
        <div class="section-title">阈值分割</div>
        <div class="slider-row">
          <span>最小值:</span>
          <ElSlider v-model="thresholdMin" :min="0" :max="4095" show-input size="small" />
        </div>
        <div class="slider-row">
          <span>最大值:</span>
          <ElSlider v-model="thresholdMax" :min="0" :max="4095" show-input size="small" />
        </div>
        <ElButton size="small" type="primary" @click="createFromThreshold">创建 Mask</ElButton>
      </div>
    </ElCard>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive } from 'vue';
import { 
  ElCard, ElButton, ElCheckbox, ElSelect, ElOption, 
  ElDivider, ElSlider, ElColorPicker, ElMessage 
} from 'element-plus';
import { Delete, DocumentCopy } from '@element-plus/icons-vue';

// Mask 数据结构
interface MaskItem {
  id: number;
  name: string;
  visible: boolean;
  color: string;
  handle: any; // Native handle
}

// 状态
const masks = ref<MaskItem[]>([]);
let nextId = 1;

// Boolean 操作选择
const selectedMaskA = ref<number | null>(null);
const selectedMaskB = ref<number | null>(null);

// 形态学半径
const morphRadius = ref(3);

// ROI 工具
const currentTool = ref<string | null>(null);
const brushSize = ref(10);

// 阈值分割
const thresholdMin = ref(0);
const thresholdMax = ref(1000);

// FloodFill 模式
const floodFillMode = ref(false);

// ==================== Mask 管理 ====================
function addMask() {
  const newMask: MaskItem = {
    id: nextId++,
    name: `Mask ${masks.value.length + 1}`,
    visible: true,
    color: 'rgba(255, 0, 0, 0.5)',
    handle: null // TODO: 调用 Native API 创建
  };
  
  // TODO: 调用 Native API
  // newMask.handle = window.nativeApi.maskManager.createEmpty(512, 512, 512, newMask.name);
  
  masks.value.push(newMask);
  ElMessage.success(`已创建 ${newMask.name}`);
}

function cloneMask(index: number) {
  const original = masks.value[index];
  const cloned: MaskItem = {
    id: nextId++,
    name: `${original.name} (副本)`,
    visible: original.visible,
    color: original.color,
    handle: null // TODO: 调用 Native API 克隆
  };
  
  // TODO: 调用 Native API
  // cloned.handle = window.nativeApi.maskManager.clone(original.handle);
  
  masks.value.push(cloned);
  ElMessage.success(`已克隆 ${original.name}`);
}

function deleteMask(index: number) {
  const mask = masks.value[index];
  
  // TODO: 调用 Native API 销毁
  // window.nativeApi.maskManager.destroy(mask.handle);
  
  masks.value.splice(index, 1);
  ElMessage.info(`已删除 ${mask.name}`);
}

function onVisibilityChange(index: number) {
  const mask = masks.value[index];
  
  // TODO: 调用 Native API 设置可见性
  // window.nativeApi.maskManager.setVisible(mask.handle, mask.visible);
  
  // 触发视图更新
  console.log(`Mask ${mask.name} visibility: ${mask.visible}`);
}

function onColorChange(index: number) {
  const mask = masks.value[index];
  
  // TODO: 解析颜色并调用 Native API
  // const rgba = parseColor(mask.color);
  // window.nativeApi.maskManager.setColor(mask.handle, rgba.r, rgba.g, rgba.b, rgba.a);
  
  console.log(`Mask ${mask.name} color: ${mask.color}`);
}

// ==================== Boolean 操作 ====================
function booleanOp(op: string) {
  if (selectedMaskA.value === null || selectedMaskB.value === null) {
    ElMessage.warning('请选择两个 Mask');
    return;
  }
  
  const maskA = masks.value[selectedMaskA.value];
  const maskB = masks.value[selectedMaskB.value];
  
  let resultName = '';
  switch (op) {
    case 'union':
      resultName = `${maskA.name} ∪ ${maskB.name}`;
      // TODO: window.nativeApi.maskManager.union(maskA.handle, maskB.handle, resultName);
      break;
    case 'intersection':
      resultName = `${maskA.name} ∩ ${maskB.name}`;
      // TODO: window.nativeApi.maskManager.intersection(maskA.handle, maskB.handle, resultName);
      break;
    case 'difference':
      resultName = `${maskA.name} - ${maskB.name}`;
      // TODO: window.nativeApi.maskManager.difference(maskA.handle, maskB.handle, resultName);
      break;
    case 'xor':
      resultName = `${maskA.name} ⊕ ${maskB.name}`;
      // TODO: window.nativeApi.maskManager.xor(maskA.handle, maskB.handle, resultName);
      break;
  }
  
  ElMessage.success(`已创建: ${resultName}`);
  
  // TODO: 将结果添加到 masks 列表
}

// ==================== 连通域操作 ====================
function enableFloodFill() {
  floodFillMode.value = true;
  ElMessage.info('请在视图中点击选择连通域');
  
  // TODO: 监听 MPR 视图的鼠标点击事件
  // 当用户点击时，获取坐标并调用 FloodFill
}

function keepLargestRegion() {
  if (masks.value.length === 0) {
    ElMessage.warning('没有可用的 Mask');
    return;
  }
  
  // TODO: 对当前选中的 Mask 执行保留最大连通域
  ElMessage.success('已保留最大连通域');
}

function removeSmallRegions() {
  if (masks.value.length === 0) {
    ElMessage.warning('没有可用的 Mask');
    return;
  }
  
  // TODO: 移除小于指定体素数的连通域
  ElMessage.success('已移除小区域');
}

// ==================== 形态学操作 ====================
function morphOp(op: string) {
  if (masks.value.length === 0) {
    ElMessage.warning('没有可用的 Mask');
    return;
  }
  
  // TODO: 对当前选中的 Mask 执行形态学操作
  console.log(`执行 ${op}，半径: ${morphRadius.value}`);
  ElMessage.success(`已执行 ${op} 操作`);
}

// ==================== ROI 绘制 ====================
function selectTool(tool: string) {
  currentTool.value = tool;
  ElMessage.info(`已选择工具: ${tool}`);
  
  // TODO: 设置 MPR 视图的绘制模式
}

// ==================== 阈值分割 ====================
function createFromThreshold() {
  const newMask: MaskItem = {
    id: nextId++,
    name: `阈值 ${thresholdMin.value}-${thresholdMax.value}`,
    visible: true,
    color: 'rgba(0, 255, 0, 0.5)',
    handle: null
  };
  
  // TODO: 调用 Native API 从阈值创建 Mask
  // newMask.handle = window.nativeApi.maskManager.createFromThreshold(
  //   volumeHandle, thresholdMin.value, thresholdMax.value, newMask.name
  // );
  
  masks.value.push(newMask);
  ElMessage.success(`已创建阈值 Mask: ${newMask.name}`);
}

// ==================== 对外暴露 ====================
// 当 MPR 视图接收到鼠标点击时调用
function onMprClick(x: number, y: number, z: number, maskIndex: number) {
  if (!floodFillMode.value) return;
  
  const mask = masks.value[maskIndex];
  
  // TODO: 调用 FloodFill
  // const newHandle = window.nativeApi.maskManager.floodFill(mask.handle, x, y, z, `${mask.name}_FloodFill`);
  
  ElMessage.success(`已在 (${x}, ${y}, ${z}) 执行 FloodFill`);
  floodFillMode.value = false;
}

// 导出接口供父组件使用
defineExpose({
  onMprClick,
  masks
});
</script>

<style scoped>
.mask-manager {
  width: 100%;
  height: 100%;
}

.manager-card {
  height: 100%;
  background: rgba(8, 25, 44, 0.9);
  border: 1px solid rgba(11, 205, 212, 0.25);
  border-radius: 8px;
}

.header-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.mask-list {
  display: flex;
  flex-direction: column;
  gap: 8px;
  max-height: 200px;
  overflow-y: auto;
}

.mask-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 8px;
  background: rgba(255, 255, 255, 0.05);
  border-radius: 4px;
}

.mask-name {
  font-size: 14px;
  margin-left: 8px;
}

.mask-controls {
  display: flex;
  gap: 8px;
  align-items: center;
}

.section-title {
  font-size: 13px;
  font-weight: bold;
  color: rgba(168, 219, 255, 0.9);
  margin-bottom: 8px;
}

.select-row {
  display: flex;
  gap: 8px;
  margin-bottom: 8px;
}

.btn-row {
  display: flex;
  flex-wrap: wrap;
  gap: 6px;
}

.slider-row {
  display: flex;
  align-items: center;
  gap: 12px;
  margin-bottom: 8px;
}

.slider-row span {
  min-width: 70px;
  font-size: 12px;
}

.threshold-section {
  margin-top: 8px;
}
</style>
