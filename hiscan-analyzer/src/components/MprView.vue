<template>
  <div class="mpr-view-container">
    <!-- MPR 图像显示（底层） -->
    <canvas 
      ref="mprCanvas" 
      class="mpr-canvas"
      @mousedown="onMouseDown"
      @mousemove="onMouseMove"
      @mouseup="onMouseUp"
      @wheel="onWheel"
    />
    
    <!-- Mask 叠加层（上层，半透明） -->
    <canvas 
      ref="maskCanvas" 
      class="mask-overlay"
    />
    
    <!-- ROI 绘制层（最上层） -->
    <canvas 
      ref="roiCanvas" 
      class="roi-overlay"
      @mousedown="onRoiMouseDown"
      @mousemove="onRoiMouseMove"
      @mouseup="onRoiMouseUp"
    />

    <!-- 信息叠加 -->
    <div class="info-overlay">
      <div class="info-text">{{ viewInfo }}</div>
      <div class="info-text">WW: {{ windowWidth }} WL: {{ windowLevel }}</div>
      <div class="info-text" v-if="currentPixelValue !== null">
        Value: {{ currentPixelValue }}
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, watch, computed } from 'vue';

interface MaskData {
  visible: boolean;
  color: string; // rgba(r, g, b, a)
  data: Uint8Array;
  width: number;
  height: number;
}

// Props
const props = defineProps<{
  viewType: 'axial' | 'coronal' | 'sagittal'; // 轴向、冠状、矢状
  sliceIndex: number; // 当前切片索引
  volumeData?: Uint16Array; // 体数据
  masks?: MaskData[]; // Mask 列表
  windowWidth?: number;
  windowLevel?: number;
  drawMode?: 'none' | 'rectangle' | 'circle' | 'polygon' | 'brush';
  brushSize?: number;
}>();

// Emits
const emit = defineEmits<{
  (e: 'click', x: number, y: number, z: number): void;
  (e: 'draw', roi: any): void;
}>();

// Refs
const mprCanvas = ref<HTMLCanvasElement | null>(null);
const maskCanvas = ref<HTMLCanvasElement | null>(null);
const roiCanvas = ref<HTMLCanvasElement | null>(null);

// 状态
const windowWidth = ref(props.windowWidth || 400);
const windowLevel = ref(props.windowLevel || 40);
const currentPixelValue = ref<number | null>(null);
const viewInfo = computed(() => {
  const typeMap = {
    axial: '轴向面 (Axial)',
    coronal: '冠状面 (Coronal)',
    sagittal: '矢状面 (Sagittal)'
  };
  return `${typeMap[props.viewType]} - Slice ${props.sliceIndex}`;
});

// 鼠标状态
const isDrawing = ref(false);
const drawStartPos = ref({ x: 0, y: 0 });
const drawCurrentPos = ref({ x: 0, y: 0 });

// ==================== MPR 渲染 ====================
function renderMPR() {
  if (!mprCanvas.value || !props.volumeData) return;
  
  const ctx = mprCanvas.value.getContext('2d');
  if (!ctx) return;
  
  const width = mprCanvas.value.width;
  const height = mprCanvas.value.height;
  
  // TODO: 从 volumeData 提取当前切片并应用窗宽窗位
  // 这里需要根据 viewType 和 sliceIndex 从 3D 体数据中提取 2D 切片
  // 然后应用窗宽窗位转换为灰度图像
  
  const imageData = ctx.createImageData(width, height);
  
  // 示例：填充灰色背景（实际应从体数据提取）
  for (let i = 0; i < imageData.data.length; i += 4) {
    const gray = 50; // TODO: 计算实际像素值
    imageData.data[i] = gray;     // R
    imageData.data[i + 1] = gray; // G
    imageData.data[i + 2] = gray; // B
    imageData.data[i + 3] = 255;  // A
  }
  
  ctx.putImageData(imageData, 0, 0);
}

// ==================== Mask 渲染 ====================
function renderMasks() {
  if (!maskCanvas.value || !props.masks) return;
  
  const ctx = maskCanvas.value.getContext('2d');
  if (!ctx) return;
  
  const width = maskCanvas.value.width;
  const height = maskCanvas.value.height;
  
  // 清空画布
  ctx.clearRect(0, 0, width, height);
  
  // 遍历所有可见的 Mask
  for (const mask of props.masks) {
    if (!mask.visible || !mask.data) continue;
    
    // 创建临时 ImageData
    const imageData = ctx.createImageData(width, height);
    
    // 解析颜色
    const rgba = parseColor(mask.color);
    
    // 将 Mask 数据转换为 RGBA
    for (let i = 0; i < mask.data.length; i++) {
      if (mask.data[i] > 0) {
        const idx = i * 4;
        imageData.data[idx] = rgba.r;
        imageData.data[idx + 1] = rgba.g;
        imageData.data[idx + 2] = rgba.b;
        imageData.data[idx + 3] = rgba.a;
      }
    }
    
    // 绘制到画布
    ctx.putImageData(imageData, 0, 0);
  }
}

// ==================== ROI 绘制 ====================
function renderROI() {
  if (!roiCanvas.value || props.drawMode === 'none') return;
  
  const ctx = roiCanvas.value.getContext('2d');
  if (!ctx) return;
  
  const width = roiCanvas.value.width;
  const height = roiCanvas.value.height;
  
  ctx.clearRect(0, 0, width, height);
  
  if (!isDrawing.value) return;
  
  ctx.strokeStyle = 'rgba(255, 255, 0, 0.8)';
  ctx.lineWidth = 2;
  
  switch (props.drawMode) {
    case 'rectangle':
      ctx.strokeRect(
        drawStartPos.value.x,
        drawStartPos.value.y,
        drawCurrentPos.value.x - drawStartPos.value.x,
        drawCurrentPos.value.y - drawStartPos.value.y
      );
      break;
      
    case 'circle':
      const radius = Math.sqrt(
        Math.pow(drawCurrentPos.value.x - drawStartPos.value.x, 2) +
        Math.pow(drawCurrentPos.value.y - drawStartPos.value.y, 2)
      );
      ctx.beginPath();
      ctx.arc(drawStartPos.value.x, drawStartPos.value.y, radius, 0, Math.PI * 2);
      ctx.stroke();
      break;
      
    case 'brush':
      // 画笔实时绘制
      ctx.fillStyle = 'rgba(255, 255, 0, 0.5)';
      ctx.beginPath();
      ctx.arc(drawCurrentPos.value.x, drawCurrentPos.value.y, props.brushSize || 10, 0, Math.PI * 2);
      ctx.fill();
      break;
  }
}

// ==================== 鼠标事件 ====================
function onMouseDown(e: MouseEvent) {
  const rect = (e.target as HTMLCanvasElement).getBoundingClientRect();
  const x = Math.floor(e.clientX - rect.left);
  const y = Math.floor(e.clientY - rect.top);
  
  // 如果是点击模式（FloodFill），发送点击事件
  emit('click', x, y, props.sliceIndex);
}

function onMouseMove(e: MouseEvent) {
  const rect = (e.target as HTMLCanvasElement).getBoundingClientRect();
  const x = Math.floor(e.clientX - rect.left);
  const y = Math.floor(e.clientY - rect.top);
  
  // TODO: 从 volumeData 获取当前像素值
  currentPixelValue.value = x + y; // Placeholder
}

function onMouseUp(_e: MouseEvent) {
  // 鼠标松开
}

function onWheel(e: WheelEvent) {
  // TODO: 滚轮调整窗宽窗位或切片索引
  e.preventDefault();
}

// ROI 绘制鼠标事件
function onRoiMouseDown(e: MouseEvent) {
  if (props.drawMode === 'none') return;
  
  const rect = (e.target as HTMLCanvasElement).getBoundingClientRect();
  const x = Math.floor(e.clientX - rect.left);
  const y = Math.floor(e.clientY - rect.top);
  
  isDrawing.value = true;
  drawStartPos.value = { x, y };
  drawCurrentPos.value = { x, y };
}

function onRoiMouseMove(e: MouseEvent) {
  if (!isDrawing.value) return;
  
  const rect = (e.target as HTMLCanvasElement).getBoundingClientRect();
  const x = Math.floor(e.clientX - rect.left);
  const y = Math.floor(e.clientY - rect.top);
  
  drawCurrentPos.value = { x, y };
  renderROI();
}

function onRoiMouseUp(_e: MouseEvent) {
  if (!isDrawing.value) return;
  
  isDrawing.value = false;
  
  // 发送绘制完成事件
  const roi = {
    type: props.drawMode,
    start: drawStartPos.value,
    end: drawCurrentPos.value,
    slice: props.sliceIndex
  };
  
  emit('draw', roi);
  
  // 清空 ROI 画布
  const ctx = roiCanvas.value?.getContext('2d');
  if (ctx) {
    ctx.clearRect(0, 0, roiCanvas.value!.width, roiCanvas.value!.height);
  }
}

// ==================== 工具函数 ====================
function parseColor(colorStr: string): { r: number, g: number, b: number, a: number } {
  // 解析 rgba(r, g, b, a) 格式
  const match = colorStr.match(/rgba?\((\d+),\s*(\d+),\s*(\d+)(?:,\s*([\d.]+))?\)/);
  if (match) {
    return {
      r: parseInt(match[1]),
      g: parseInt(match[2]),
      b: parseInt(match[3]),
      a: match[4] ? Math.floor(parseFloat(match[4]) * 255) : 255
    };
  }
  return { r: 255, g: 0, b: 0, a: 128 };
}

// ==================== 生命周期 ====================
onMounted(() => {
  // 初始化 Canvas 尺寸
  if (mprCanvas.value) {
    mprCanvas.value.width = 512;
    mprCanvas.value.height = 512;
  }
  if (maskCanvas.value) {
    maskCanvas.value.width = 512;
    maskCanvas.value.height = 512;
  }
  if (roiCanvas.value) {
    roiCanvas.value.width = 512;
    roiCanvas.value.height = 512;
  }
  
  renderMPR();
  renderMasks();
});

// 监听 props 变化
watch(() => props.sliceIndex, () => {
  renderMPR();
  renderMasks();
});

watch(() => props.masks, () => {
  renderMasks();
}, { deep: true });

watch(() => props.drawMode, () => {
  // 清空 ROI 画布
  const ctx = roiCanvas.value?.getContext('2d');
  if (ctx) {
    ctx.clearRect(0, 0, roiCanvas.value!.width, roiCanvas.value!.height);
  }
});
</script>

<style scoped>
.mpr-view-container {
  position: relative;
  width: 100%;
  height: 100%;
  background: #000;
}

.mpr-canvas,
.mask-overlay,
.roi-overlay {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
}

.mpr-canvas {
  z-index: 1;
}

.mask-overlay {
  z-index: 2;
  pointer-events: none; /* Mask 不响应鼠标事件 */
}

.roi-overlay {
  z-index: 3;
}

.info-overlay {
  position: absolute;
  top: 10px;
  left: 10px;
  z-index: 4;
  pointer-events: none;
  color: #fff;
  font-size: 12px;
  text-shadow: 0 0 3px #000;
}

.info-text {
  margin-bottom: 4px;
}
</style>
