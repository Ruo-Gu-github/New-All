<script setup>
import { ref } from "vue";
import { useWindowControls } from "./services/nativeBridge";
import ReconstructRecordsTab from "./components/ReconstructRecordsTab.vue";
import ReconstructRealtimeTab from "./components/ReconstructRealtimeTab.vue";
import LocalProcessTab from "./components/LocalProcessTab.vue";
import ReconstructServiceTab from "./components/ReconstructServiceTab.vue";

const windowControls = useWindowControls();

const tabs = [
  { key: "records", label: "任务记录" },
  { key: "realtime", label: "实时监控" },
  { key: "local", label: "本地处理" },
  { key: "service", label: "服务配置" },
];

const activeTab = ref("records");

const minimizeWindow = async () => {
  await windowControls.minimize();
};

const toggleMaximizeWindow = async () => {
  await windowControls.toggleMaximize();
};

const closeWindow = async () => {
  await windowControls.close();
};
</script>

<template>
  <div class="main-bg reconstruct-bg">
    <div class="console-app">
      <div class="titlebar" data-drag-region>
        <span class="title">Hiscan Reconstruct</span>
        <div class="window-controls">
          <button type="button" @click.stop="minimizeWindow" title="最小化">&#x2015;</button>
          <button type="button" @click.stop="toggleMaximizeWindow" title="最大化/还原">&#x25A1;</button>
          <button type="button" @click.stop="closeWindow" title="关闭">&#x2715;</button>
        </div>
      </div>

      <div class="tabs">
        <button
          v-for="tab in tabs"
          :key="tab.key"
          :class="{ active: activeTab === tab.key }"
          @click="activeTab = tab.key"
        >
          {{ tab.label }}
        </button>
      </div>

      <div class="tab-content-area">
        <ReconstructRecordsTab v-if="activeTab === 'records'" />
        <ReconstructRealtimeTab v-else-if="activeTab === 'realtime'" />
        <LocalProcessTab v-else-if="activeTab === 'local'" />
        <ReconstructServiceTab v-else />
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
</style>

<style scoped>
.main-bg {
  min-height: 100vh;
  width: 100%;
  background: linear-gradient(180deg, #261b00 0%, #120b02 100%);
  display: flex;
  justify-content: center;
  align-items: stretch;
  color: #ffe9b0;
}

.console-app {
  width: 100%;
  height: 100vh;
  display: flex;
  flex-direction: column;
}

.titlebar {
  width: 100%;
  height: 36px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0;
  -webkit-app-region: drag;
  user-select: none;
  color: #ffefc6;
}

.title {
  font-size: 1.1rem;
  color: #fffbeb;
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
  color: #ffeaba;
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
  background: rgba(255, 233, 162, 0.28);
}

.window-controls button:last-child:hover {
  background: #ef6c00;
  color: #fffef5;
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
  color: rgba(255, 245, 200, 0.78);
  border: none;
  padding: 8px 24px;
  font-size: 1.1rem;
  border-radius: 8px 8px 0 0;
  cursor: pointer;
  font-weight: 500;
  transition: background 0.2s, color 0.2s;
}

.tabs button.active {
  background: rgba(78, 58, 9, 0.68);
  color: #ffe082;
  box-shadow: 0 2px 6px rgba(24, 18, 0, 0.22);
  border: 1.5px solid rgba(255, 213, 79, 0.28);
}

.tab-content-area {
  flex: 1;
  width: 100%;
  min-height: calc(100% - 84px);
  color: #ffe9b0;
  overflow: auto;
  display: flex;
  flex-direction: column;
  box-sizing: border-box;
  gap: 16px;
}

:deep(.panel-grid) {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
  gap: 18px;
  width: 100%;
  min-height: 220px;
}

:deep(.panel-card) {
  background: rgba(37, 25, 0, 0.86);
  border: 1px solid rgba(255, 213, 79, 0.35);
  border-radius: 16px;
  padding: 18px 20px;
  display: flex;
  flex-direction: column;
  gap: 16px;
  min-height: 200px;
  box-shadow: 0 12px 24px rgba(255, 193, 7, 0.18);
}

:deep(.panel-card header h2) {
  margin: 0;
  font-size: 16px;
  font-weight: 600;
  color: #ffd95b;
}

:deep(.panel-card header p) {
  margin: 6px 0 0;
  color: rgba(255, 217, 91, 0.78);
  font-size: 12px;
}

:deep(.panel-card.accent) {
  background: linear-gradient(135deg, rgba(255, 213, 79, 0.26), rgba(255, 171, 0, 0.2));
  border-color: rgba(255, 213, 79, 0.55);
}

:deep(.metric-row) {
  display: flex;
  gap: 16px;
  flex-wrap: wrap;
}

:deep(.metric-chip) {
  min-width: 120px;
  background: rgba(18, 12, 0, 0.78);
  border-radius: 12px;
  padding: 12px 14px;
  display: flex;
  flex-direction: column;
  gap: 4px;
}

:deep(.metric-value) {
  font-size: 18px;
  font-weight: 600;
  color: #ffdd6b;
}

:deep(.metric-label) {
  font-size: 12px;
  color: rgba(255, 217, 91, 0.85);
}

:deep(.metric-helper) {
  font-size: 11px;
  color: rgba(255, 217, 91, 0.72);
}

:deep(.history-list) {
  list-style: none;
  margin: 0;
  padding: 0;
  display: flex;
  flex-direction: column;
  gap: 10px;
  color: #ffe9b0;
  font-size: 13px;
}

:deep(.history-time) {
  min-width: 72px;
  font-weight: 600;
  color: #ffd54f;
}

:deep(.history-detail) {
  color: rgba(255, 236, 179, 0.86);
}

:deep(.form-stack) {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

:deep(.form-stack label) {
  display: flex;
  flex-direction: column;
  gap: 6px;
  font-size: 12px;
  color: rgba(255, 217, 91, 0.88);
}

:deep(.form-stack input),
:deep(.form-stack select) {
  padding: 10px 12px;
  border-radius: 12px;
  border: 1px solid rgba(255, 204, 64, 0.45);
  background: rgba(18, 13, 0, 0.7);
  color: #ffe9b0;
}

:deep(.switch-row) {
  display: flex;
  align-items: center;
  gap: 8px;
}

:deep(.switch-row input) {
  width: 18px;
  height: 18px;
}

:deep(.btn) {
  border: none;
  border-radius: 999px;
  padding: 9px 20px;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  transition: transform 0.15s ease, box-shadow 0.15s ease;
}

:deep(.btn.primary) {
  background: linear-gradient(135deg, rgba(255, 213, 79, 0.92), rgba(255, 171, 0, 0.92));
  color: #2b1d00;
  box-shadow: 0 12px 24px rgba(255, 193, 7, 0.25);
}

:deep(.btn.primary:hover) {
  transform: translateY(-1px);
}

:deep(.btn.outline) {
  background: rgba(33, 24, 0, 0.58);
  color: #ffd95b;
  border: 1px solid rgba(255, 213, 79, 0.45);
}

:deep(.action-row) {
  display: flex;
  gap: 10px;
}

:deep(.status-list) {
  display: flex;
  flex-direction: column;
  gap: 6px;
  font-size: 12px;
}

:deep(.status-list .error) {
  color: #ff8a80;
}

:deep(.pipeline-list) {
  list-style: none;
  margin: 0;
  padding: 0;
  display: flex;
  flex-direction: column;
  gap: 12px;
}

:deep(.pipeline-meta) {
  display: flex;
  justify-content: space-between;
  color: rgba(255, 217, 91, 0.85);
  font-size: 13px;
}

:deep(.pipeline-name) {
  font-weight: 600;
}

:deep(.pipeline-status) {
  color: rgba(255, 227, 130, 0.9);
}

:deep(.progress-bar) {
  height: 6px;
  background: rgba(255, 213, 79, 0.2);
  border-radius: 999px;
  overflow: hidden;
}

:deep(.progress) {
  height: 100%;
  background: linear-gradient(90deg, rgba(255, 193, 7, 0.95), rgba(255, 215, 64, 0.95));
}

@media (max-width: 1200px) {
  .tabs {
    padding: 0 16px;
  }
  .tab-content-area {
    padding: 20px;
  }
}

@media (max-width: 768px) {
  .panel-grid {
    grid-template-columns: 1fr;
  }
  .action-row {
    flex-direction: column;
  }
}
</style>
