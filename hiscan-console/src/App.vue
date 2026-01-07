<script setup>
import { ref } from 'vue';
import RecordManager from './components/RecordManager.vue';
import SampleShoot from './components/SampleShoot.vue';
import MachineConfig from './components/MachineConfig.vue';
import ParameterCalibration from './components/ParameterCalibration.vue';
import { useWindowControls } from './services/nativeBridge';

const windowControls = useWindowControls();
const tab = ref(0);

async function minimize() {
  await windowControls.minimize();
}
async function toggleMaximize() {
  await windowControls.toggleMaximize();
}
async function closeWindow() {
  await windowControls.close();
}
</script>

<template>
  <div class="main-bg">
    <div class="console-app">
      <div class="titlebar" data-drag-region>
        <span class="title">Hiscan Console</span>
        <div class="window-controls">
          <button type="button" @click.stop="minimize" title="最小化">&#x2015;</button>
          <button type="button" @click.stop="toggleMaximize" title="最大化/还原">&#x25A1;</button>
          <button type="button" @click.stop="closeWindow" title="关闭">&#x2715;</button>
        </div>
      </div>
      <div class="tabs">
        <button :class="{active: tab === 0}" @click="tab = 0">拍摄记录管理</button>
        <button :class="{active: tab === 1}" @click="tab = 1">样品拍摄</button>
        <button :class="{active: tab === 2}" @click="tab = 2">机器配置</button>
        <button :class="{active: tab === 3}" @click="tab = 3">参数校准</button>
      </div>
      <div class="tab-content-area">
        <RecordManager v-if="tab === 0" />
        <SampleShoot v-if="tab === 1" />
        <MachineConfig v-if="tab === 2" />
        <ParameterCalibration v-if="tab === 3" />
      </div>
    </div>
  </div>
</template>

<style>
html,
body,
#app {
  margin: 0;
  padding: 0;
  height: 100%;
  width: 100%;
  overflow: hidden;
}

/* 全局覆盖命令执行器弹窗背景色 */
.command-dialog {
  background: linear-gradient(180deg, #101d42 0%, #081230 100%);
  border-radius: 14px;
  width: 1080px;
  height: 800px;
  max-width: 90vw;
  max-height: 84vh;
  display: flex;
  flex-direction: column;
}

/* 全局覆盖所有弹窗标题颜色，确保生效 */
.command-dialog .el-dialog__title {
  color: #f0f0f0 !important;
}

</style>

<style scoped>
.main-bg {
  min-height: 100vh;
  background: linear-gradient(180deg, #101d42 0%, #081230 100%);
  width: 100%;
  height: 100%;
}
.console-app {
  width: 100%;
  height: 100%;
  display: flex;
  flex-direction: column;
}
.titlebar {
  width: 100%;
  height: 36px;
  border-bottom: none;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0;
  -webkit-app-region: drag;
  user-select: none;
}
.titlebar .title {
  font-size: 1.1rem;
  color: #fff;
  font-weight: 600;
  letter-spacing: 1px;
  padding-left: 12px;
}
.window-controls {
  display: flex;
  gap: 2px;
  -webkit-app-region: no-drag;
  height: 100%;
  padding-right: 4px;
}
.window-controls button {
  width: 44px;
  height: 100%;
  background: transparent;
  color: #fff;
  border: none;
  font-size: 1rem;
  border-radius: 4px;
  cursor: pointer;
  transition: background 0.2s;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 0;
  appearance: none;
}
.window-controls button:hover {
  background: #3949ab;
}
.window-controls button:last-child:hover {
  background: #e53935;
  color: #fff;
}
.tabs {
  display: flex;
  gap: 16px;
  margin-bottom: 0;
  border-bottom: none;
  padding: 0 32px;
  width: 100%;
  box-sizing: border-box;
  height: 52px;
  align-items: center;
}
.tabs button {
  background: transparent;
  color: #e8edff;
  border: none;
  padding: 8px 24px;
  font-size: 1.1rem;
  border-radius: 8px 8px 0 0;
  cursor: pointer;
  font-weight: 500;
  transition: background 0.2s, color 0.2s;
}

.tabs button.active {
  background: rgba(36, 49, 110, 0.85);
  color: #fff;
  box-shadow: 0 2px 8px 0 rgba(16,29,66,0.08);
  border: 1.5px solid rgba(92,116,209,0.18);
  color: #fff;
}
.tab-content-area {
  flex: 1;
  width: 100%;
  height: calc(100% - 92px);
  background: #212a4a;
  border-radius: 0 0 12px 12px;
  color: #fff;
  overflow: auto;
  display: flex;
  flex-direction: column;
  padding: 0;
  box-sizing: border-box;
}
</style>



