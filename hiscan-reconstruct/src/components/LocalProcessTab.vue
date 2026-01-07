<script setup>
import { ref } from 'vue';

const localTasks = [
  { key: 'correction', label: '本地矫正' },
  { key: 'reconstruct', label: '本地重建' },
  { key: 'denoise', label: '本地降噪' }
];

const progress = ref({ correction: 0, reconstruct: 0, denoise: 0 });
const running = ref({ correction: false, reconstruct: false, denoise: false });

function startTask(taskKey) {
  running.value[taskKey] = true;
  progress.value[taskKey] = 0;
  // 模拟进度
  const timer = setInterval(() => {
    if (progress.value[taskKey] < 100) {
      progress.value[taskKey] += Math.floor(Math.random() * 10 + 5);
      if (progress.value[taskKey] > 100) progress.value[taskKey] = 100;
    } else {
      running.value[taskKey] = false;
      clearInterval(timer);
    }
  }, 400);
}
</script>

<template>
  <div class="tab-content">
    <h2>本地处理</h2>
    <p class="desc">用于本地文件的矫正、重建、降噪操作，适用于参数微调或服务器不可用场景。</p>
    <div class="local-task-list">
      <div v-for="task in localTasks" :key="task.key" class="local-task-item">
        <div class="task-label">{{ task.label }}</div>
        <button class="btn primary" :disabled="running[task.key]" @click="startTask(task.key)">
          {{ running[task.key] ? '处理中...' : '开始' + task.label }}
        </button>
        <div class="progress-bar">
          <div class="progress-fill" :style="{ width: progress[task.key] + '%' }"></div>
        </div>
        <div class="progress-text">{{ progress[task.key] }}%</div>
      </div>
    </div>
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
.desc {
  font-size: 14px;
  color: #ffe4aa;
  margin-bottom: 18px;
}
.local-task-list {
  display: flex;
  gap: 32px;
}
.local-task-item {
  background: rgba(60, 38, 0, 0.92);
  border-radius: 14px;
  padding: 24px 32px;
  box-shadow: 0 8px 24px rgba(25, 15, 0, 0.35);
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  min-width: 220px;
}
.task-label {
  font-size: 18px;
  font-weight: 600;
  margin-bottom: 12px;
  color: #ffd88c;
}
.btn.primary {
  background: #ffb300;
  color: #222;
  border: none;
  border-radius: 6px;
  padding: 8px 24px;
  font-size: 15px;
  font-weight: 600;
  margin-bottom: 16px;
  cursor: pointer;
}
.btn.primary:disabled {
  background: #ffe066;
  color: #aaa;
  cursor: not-allowed;
}
.progress-bar {
  width: 180px;
  height: 12px;
  background: rgba(255, 224, 102, 0.18);
  border-radius: 6px;
  overflow: hidden;
  margin-bottom: 8px;
}
.progress-fill {
  height: 100%;
  background: linear-gradient(90deg, #ffb300 0%, #ff7043 100%);
  transition: width 0.3s;
}
.progress-text {
  font-size: 13px;
  color: #fffbe6;
  font-weight: 600;
}
</style>
