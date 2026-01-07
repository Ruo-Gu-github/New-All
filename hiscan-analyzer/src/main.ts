import { createApp } from "vue";
import "./native-stub";
import "./style.css";
import App from "./App.vue";

// Disable Element Plus toast messages globally.
// These overlays can occlude native child HWND (OpenGL) regions on Windows and cause flicker.
import { ElMessage } from "element-plus";

type ElMessageReturn = { close?: () => void };
const noopMsg = (_msg?: any): ElMessageReturn => ({ close: () => {} });
try {
  (ElMessage as any).success = noopMsg;
  (ElMessage as any).info = noopMsg;
  (ElMessage as any).warning = noopMsg;
  (ElMessage as any).error = noopMsg;
} catch {
  // ignore
}

createApp(App)
  .mount("#app")
  .$nextTick(() => {
    // Use contextBridge
    window.ipcRenderer?.on?.(
      "main-process-message",
      (_event: any, message: any) => {
        console.log(message);
      }
    );
  });
