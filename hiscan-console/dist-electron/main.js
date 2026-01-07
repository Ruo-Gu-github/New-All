import { app, BrowserWindow, ipcMain } from "electron";
import { createRequire } from "node:module";
import { fileURLToPath } from "node:url";
import path from "node:path";
const require2 = createRequire(import.meta.url);
const __dirname = path.dirname(fileURLToPath(import.meta.url));
process.env.APP_ROOT = path.join(__dirname, "..");
const VITE_DEV_SERVER_URL = process.env["VITE_DEV_SERVER_URL"];
const MAIN_DIST = path.join(process.env.APP_ROOT, "dist-electron");
const RENDERER_DIST = path.join(process.env.APP_ROOT, "dist");
process.env.VITE_PUBLIC = VITE_DEV_SERVER_URL ? path.join(process.env.APP_ROOT, "public") : RENDERER_DIST;
let win;
const PREVIEW_CHANNELS = {
  createWindow: "preview:create-window",
  getBuffer: "preview:get-buffer"
};
const DICOM_CHANNELS = {
  loadSeries: "dicom:load-series"
};
let nativePreview = null;
let nativeDicom = null;
const WINDOW_CONTROL_CHANNELS = {
  minimize: "window-controls:minimize",
  toggleMaximize: "window-controls:toggle-maximize",
  close: "window-controls:close"
};
function loadNativeModules() {
  try {
    const nativePath = path.join(process.env.APP_ROOT, "native", "hello", "build", "Release", "hello.node");
    nativePreview = require2(nativePath);
  } catch (error) {
    console.error("[main] Failed to load native preview add-on", error);
    nativePreview = null;
  }
  try {
    const dicomModulePath = path.join(process.env.APP_ROOT, "native", "dicom");
    nativeDicom = require2(dicomModulePath);
  } catch (error) {
    console.error("[main] Failed to load native dicom add-on", error);
    nativeDicom = null;
  }
}
function registerWindowControlHandlers() {
  ipcMain.removeHandler(WINDOW_CONTROL_CHANNELS.minimize);
  ipcMain.removeHandler(WINDOW_CONTROL_CHANNELS.toggleMaximize);
  ipcMain.removeHandler(WINDOW_CONTROL_CHANNELS.close);
  ipcMain.handle(WINDOW_CONTROL_CHANNELS.minimize, () => {
    win == null ? void 0 : win.minimize();
  });
  ipcMain.handle(WINDOW_CONTROL_CHANNELS.toggleMaximize, () => {
    if (!win) return;
    if (win.isMaximized()) {
      win.restore();
    } else {
      win.maximize();
    }
  });
  ipcMain.handle(WINDOW_CONTROL_CHANNELS.close, () => {
    win == null ? void 0 : win.close();
  });
}
function registerNativeHandlers() {
  ipcMain.removeHandler(PREVIEW_CHANNELS.createWindow);
  ipcMain.removeHandler(PREVIEW_CHANNELS.getBuffer);
  ipcMain.removeHandler(DICOM_CHANNELS.loadSeries);
  ipcMain.handle(PREVIEW_CHANNELS.createWindow, (_event, config) => {
    var _a;
    return ((_a = nativePreview == null ? void 0 : nativePreview.createPreviewWindow) == null ? void 0 : _a.call(nativePreview, config)) ?? "native module unavailable";
  });
  ipcMain.handle(PREVIEW_CHANNELS.getBuffer, () => {
    var _a;
    return ((_a = nativePreview == null ? void 0 : nativePreview.getPreviewBuffer) == null ? void 0 : _a.call(nativePreview)) ?? null;
  });
  ipcMain.handle(DICOM_CHANNELS.loadSeries, (_event, payload) => {
    var _a;
    return ((_a = nativeDicom == null ? void 0 : nativeDicom.loadSeries) == null ? void 0 : _a.call(nativeDicom, payload)) ?? null;
  });
}
function createWindow() {
  win = new BrowserWindow({
    width: 1600,
    height: 1020,
    minWidth: 1600,
    minHeight: 1020,
    frame: false,
    titleBarStyle: "hidden",
    backgroundColor: "#101820",
    icon: path.join(process.env.VITE_PUBLIC, "electron-vite.svg"),
    webPreferences: {
      preload: path.join(__dirname, "preload.mjs")
    }
  });
  win.on("closed", () => {
    win = null;
  });
  win.webContents.on("did-finish-load", () => {
    win == null ? void 0 : win.webContents.send("main-process-message", (/* @__PURE__ */ new Date()).toLocaleString());
  });
  if (VITE_DEV_SERVER_URL) {
    win.loadURL(VITE_DEV_SERVER_URL);
  } else {
    win.loadFile(path.join(RENDERER_DIST, "index.html"));
  }
}
app.on("window-all-closed", () => {
  if (process.platform !== "darwin") {
    app.quit();
    win = null;
  }
});
app.on("activate", () => {
  if (BrowserWindow.getAllWindows().length === 0) {
    createWindow();
  }
});
app.whenReady().then(() => {
  loadNativeModules();
  registerWindowControlHandlers();
  registerNativeHandlers();
  createWindow();
});
export {
  MAIN_DIST,
  RENDERER_DIST,
  VITE_DEV_SERVER_URL
};
