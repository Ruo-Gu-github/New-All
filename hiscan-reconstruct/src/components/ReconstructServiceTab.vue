<script setup>
import { reactive, ref } from "vue";

const resourceStats = ref([
  { label: "GPU 占用", value: "87%", helper: "温度 72℃" },
  { label: "显存使用", value: "21.3 GB", helper: "峰值 24 GB" },
  { label: "磁盘吞吐", value: "1.2 GB/s", helper: "缓存稳定" },
]);

const serviceConfig = reactive({
  gpuReconstruct: 0,
  gpuDenoise: 1,
  savePath: '',
});

// 配置保存为JSON
function saveServiceConfig() {
  const configJson = JSON.stringify(serviceConfig);
  // 实际项目可写入文件，这里仅演示
  window.localStorage.setItem('serviceConfig', configJson);
  alert('配置已保存');
}

// 启动时验证配置有效性（示例：路径和显卡）
function validateServiceConfig() {
  // 假设本机有2张显卡，路径必须非空
  const availableGpus = [0, 1];
  let errorMsg = '';
  if (!availableGpus.includes(serviceConfig.gpuReconstruct)) {
    errorMsg += `重建GPU${serviceConfig.gpuReconstruct} 不存在。\n`;
  }
  if (!availableGpus.includes(serviceConfig.gpuDenoise)) {
    errorMsg += `降噪GPU${serviceConfig.gpuDenoise} 不存在。\n`;
  }
  if (!serviceConfig.savePath) {
    errorMsg += '重建文件保存路径未设置。\n';
  }
  if (errorMsg) {
    alert('服务配置有误：\n' + errorMsg + '请前往服务配置Tab修正。');
    // 跳转逻辑由父组件/路由控制
    // emit('switchTab', 'service')
  }
}

// 启动时调用（实际应在App入口或父组件）
// validateServiceConfig();
</script>

<template>
  <div class="tab-content">
    <div class="panel-card">
      <header>
        <h2>服务配置</h2>
        <p>重建与降噪GPU分配、文件保存路径</p>
      </header>
      <div class="form-stack">
        <label>重建GPU分配
          <select v-model.number="serviceConfig.gpuReconstruct">
            <option :value="0">显卡0</option>
            <option :value="1">显卡1</option>
          </select>
        </label>
        <label>降噪GPU分配
          <select v-model.number="serviceConfig.gpuDenoise">
            <option :value="0">显卡0</option>
            <option :value="1">显卡1</option>
          </select>
        </label>
        <label>重建文件保存路径
          <input v-model="serviceConfig.savePath" type="text" placeholder="请输入保存路径" />
        </label>
        <button class="btn primary" @click="saveServiceConfig">保存配置</button>
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
  color: #ffeac0;
  display: flex;
  flex-direction: column;
}
</style>

