"use strict";
const electron = require("electron");
function isDialogWindow() {
  try {
    return Array.isArray(process.argv) && process.argv.includes("--hiscan-window=dialog");
  } catch {
    return false;
  }
}
function injectDialogTheme() {
  var _a;
  if (!isDialogWindow()) return;
  if (typeof document === "undefined") return;
  const css = `
    html,body{height:100%;}
    body{
    }
    /* Common dialog layout */
    .custom-titlebar{
      height: 36px !important;
      background: rgba(8, 25, 44, 0.35) !important;
      border-bottom: 1px solid rgba(11, 205, 212, 0.18) !important;
      border-radius: 0 !important;
    }
    .titlebar-title{
      color: #f2fdff !important;
      font-weight: 600 !important;
      letter-spacing: 0.5px !important;
    }
    .titlebar-close{
      color: rgba(219, 246, 255, 0.78) !important;
    }
    .titlebar-close:hover{
      background: rgba(11, 205, 212, 0.22) !important;
      color: #dbf6ff !important;
    }
    /* Neutralize white panels from legacy dialog styles */
    .panel,.panel-hd,.panel-bd,canvas,#app{
      background: transparent !important;
    }
    .panel,.panel-hd,.footer{
      border-color: rgba(11, 205, 212, 0.18) !important;
    }
  `;
  const style = document.createElement("style");
  style.setAttribute("data-hiscan-theme", "dialog");
  style.textContent = css;
  (_a = document.head) == null ? void 0 : _a.appendChild(style);
}
try {
  if (isDialogWindow()) {
    window.addEventListener("DOMContentLoaded", injectDialogTheme, {
      once: true
    });
  }
} catch {
}
electron.contextBridge.exposeInMainWorld("ipcRenderer", {
  on(...args) {
    const [channel, listener] = args;
    return electron.ipcRenderer.on(
      channel,
      (event, ...args2) => listener(event, ...args2)
    );
  },
  off(...args) {
    const [channel, ...omit] = args;
    return electron.ipcRenderer.off(channel, ...omit);
  },
  send(...args) {
    const [channel, ...omit] = args;
    return electron.ipcRenderer.send(channel, ...omit);
  },
  invoke(...args) {
    const [channel, ...omit] = args;
    return electron.ipcRenderer.invoke(channel, ...omit);
  }
});
electron.contextBridge.exposeInMainWorld("dicomApi", {
  selectFolder: () => electron.ipcRenderer.invoke("dicom:select-folder"),
  scanFolder: (folderPath) => electron.ipcRenderer.invoke("dicom:scan-folder", folderPath),
  getAllSeries: () => electron.ipcRenderer.invoke("dicom:get-all-series"),
  removeSeries: (seriesId) => electron.ipcRenderer.invoke("dicom:remove-series", seriesId),
  cleanup: () => electron.ipcRenderer.invoke("dicom:cleanup")
});
electron.contextBridge.exposeInMainWorld("visualizationApi", {
  createAPR: (sessionId, folderPath) => electron.ipcRenderer.invoke("viz:create-apr", sessionId, folderPath),
  createMPR: (sessionId, folderPath) => electron.ipcRenderer.invoke("viz:create-mpr", sessionId, folderPath),
  renderAPRSlice: (sessionId, viewName, width, height) => electron.ipcRenderer.invoke(
    "viz:render-apr-slice",
    sessionId,
    viewName,
    width,
    height
  ),
  captureAPRScreenshots: (sessionId, folderPath, selection, windowHwndsOrWidth, width, height) => electron.ipcRenderer.invoke(
    "viz:capture-apr-screenshots",
    sessionId,
    folderPath,
    selection,
    windowHwndsOrWidth,
    width,
    height
  ),
  updateCenter: (sessionId, x, y, z) => electron.ipcRenderer.invoke("viz:update-center", sessionId, x, y, z),
  updateRotation: (sessionId, angleX, angleY, angleZ) => electron.ipcRenderer.invoke(
    "viz:update-rotation",
    sessionId,
    angleX,
    angleY,
    angleZ
  ),
  setCrosshairVisible: (sessionId, visible) => electron.ipcRenderer.invoke("viz:set-crosshair-visible", sessionId, visible),
  getAPRState: (sessionId, viewOrWindowId) => electron.ipcRenderer.invoke("viz:get-apr-state", sessionId, viewOrWindowId),
  setWindowLevel: (sessionId, windowWidth, windowLevel) => electron.ipcRenderer.invoke(
    "viz:set-window-level",
    sessionId,
    windowWidth,
    windowLevel
  ),
  getWindowLevel: (sessionId) => electron.ipcRenderer.invoke("viz:get-window-level", sessionId),
  // MIP/MinIP projection mode (0=Normal, 1=MIP, 2=MinIP)
  setProjectionMode: (sessionId, mode, thickness) => electron.ipcRenderer.invoke("viz:set-projection-mode", sessionId, mode, thickness),
  getProjectionMode: (sessionId) => electron.ipcRenderer.invoke("viz:get-projection-mode", sessionId),
  // 裁切 API
  cropVolume: (sessionId) => electron.ipcRenderer.invoke("viz:crop-volume", sessionId),
  applyCroppedVolume: (sessionId) => electron.ipcRenderer.invoke("viz:apply-cropped-volume", sessionId),
  // 裁切设置 API
  setCropShape: (shape) => electron.ipcRenderer.invoke("viz:set-crop-shape", shape),
  getCropShape: () => electron.ipcRenderer.invoke("viz:get-crop-shape"),
  setCropCylinderDirection: (direction) => electron.ipcRenderer.invoke("viz:set-crop-cylinder-direction", direction),
  getCropCylinderDirection: () => electron.ipcRenderer.invoke("viz:get-crop-cylinder-direction"),
  setCropBoxSize: (sessionId, sizeX, sizeY, sizeZ, volumeWidth, volumeHeight, volumeDepth) => electron.ipcRenderer.invoke(
    "viz:set-crop-box-size",
    sessionId,
    sizeX,
    sizeY,
    sizeZ,
    volumeWidth,
    volumeHeight,
    volumeDepth
  ),
  getCropSettings: (sessionId) => electron.ipcRenderer.invoke("viz:get-crop-settings", sessionId),
  processWindowEvents: () => electron.ipcRenderer.invoke("viz:process-window-events"),
  embedWindow: (windowId, x, y, width, height) => electron.ipcRenderer.invoke("viz:embed-window", windowId, x, y, width, height),
  renderAllViews: () => electron.ipcRenderer.invoke("viz:render-all-views"),
  invalidateAllWindows: () => electron.ipcRenderer.invoke("viz:invalidate-all-windows"),
  invalidateWindow: (windowId) => electron.ipcRenderer.invoke("viz:invalidate-window", windowId),
  resetView: (windowId) => electron.ipcRenderer.invoke("viz:reset-view", windowId),
  set3DOrthogonalMode: (windowId, enabled) => electron.ipcRenderer.invoke("viz:set-3d-orthogonal-mode", windowId, enabled),
  set3DMaskIsoSurface: (windowId, enabled) => electron.ipcRenderer.invoke("viz:set-3d-mask-iso-surface", windowId, enabled),
  set3DLightParameters: (windowId, ambient, diffuse, specular) => electron.ipcRenderer.invoke(
    "viz:set-3d-light-parameters",
    windowId,
    ambient,
    diffuse,
    specular
  ),
  set3DTransferFunction: (windowId, points) => electron.ipcRenderer.invoke("viz:set-3d-transfer-function", windowId, points),
  getGpuInfo: (windowId) => electron.ipcRenderer.invoke("viz:get-gpu-info", windowId),
  // HIS4D (4D cine)
  packHis4dFromFolders: (outputPath, folders, timestampsMs) => electron.ipcRenderer.invoke("viz:pack-his4d", outputPath, folders, timestampsMs),
  createAPRFromHis4d: (sessionId, his4dPath) => electron.ipcRenderer.invoke("viz:create-apr-his4d", sessionId, his4dPath),
  his4dSetFrame: (sessionId, frameIndex) => electron.ipcRenderer.invoke("viz:his4d-set-frame", sessionId, frameIndex),
  his4dGetSessionInfo: (sessionId) => electron.ipcRenderer.invoke("viz:his4d-get-session-info", sessionId),
  hideAllWindows: () => electron.ipcRenderer.invoke("viz:hide-all-windows"),
  showAllWindows: () => electron.ipcRenderer.invoke("viz:show-all-windows"),
  destroyAll3DWindows: () => electron.ipcRenderer.invoke("viz:destroy-all-3d-windows"),
  destroyAllWindows: () => electron.ipcRenderer.invoke("viz:destroy-all-windows"),
  resizeWindow: (windowId, x, y, width, height) => electron.ipcRenderer.invoke("viz:resize-window", windowId, x, y, width, height),
  startRenderLoop: (targetFPS = 60) => electron.ipcRenderer.invoke("viz:start-render-loop", targetFPS),
  stopRenderLoop: () => electron.ipcRenderer.invoke("viz:stop-render-loop"),
  refreshAllWindowsZOrder: () => electron.ipcRenderer.invoke("viz:refresh-all-windows-zorder"),
  refreshWindowZOrder: (windowId) => electron.ipcRenderer.invoke("viz:refresh-window-zorder", windowId),
  raiseAllWindows: () => electron.ipcRenderer.invoke("viz:raise-all-windows"),
  raiseWindow: (windowId) => electron.ipcRenderer.invoke("viz:raise-window", windowId),
  setWindowToolType: (windowId, toolType) => electron.ipcRenderer.invoke("viz:set-window-tool-type", windowId, toolType),
  setWindowCropBoxVisible: (windowId, visible) => electron.ipcRenderer.invoke("viz:set-window-crop-box-visible", windowId, visible),
  enableAPRCropBox: (sessionId, enable) => electron.ipcRenderer.invoke("viz:enable-apr-crop-box", sessionId, enable),
  setAPRCropBox: (sessionId, width, height, depth) => electron.ipcRenderer.invoke("viz:set-apr-crop-box", sessionId, width, height, depth),
  setAPRCropBoxRange: (sessionId, xStart, xEnd, yStart, yEnd, zStart, zEnd) => electron.ipcRenderer.invoke(
    "viz:set-apr-crop-box-range",
    sessionId,
    xStart,
    xEnd,
    yStart,
    yEnd,
    zStart,
    zEnd
  ),
  getAPRCropBox: (sessionId) => electron.ipcRenderer.invoke("viz:get-apr-crop-box", sessionId),
  isAPRCropBoxEnabled: (sessionId) => electron.ipcRenderer.invoke("viz:is-apr-crop-box-enabled", sessionId),
  getCompletedMeasurements: () => electron.ipcRenderer.invoke("viz:get-completed-measurements"),
  deleteMeasurement: (measurementId) => electron.ipcRenderer.invoke("viz:delete-measurement", measurementId),
  getMeasurementProfile: (sessionId, measurementId, maxPoints) => electron.ipcRenderer.invoke(
    "viz:get-measurement-profile",
    sessionId,
    measurementId,
    maxPoints
  ),
  getMeasurementRegionHistogram: (sessionId, measurementId) => electron.ipcRenderer.invoke(
    "viz:get-measurement-region-histogram",
    sessionId,
    measurementId
  ),
  updateMPRCenter: (sessionId, x, y, z) => electron.ipcRenderer.invoke("viz:update-mpr-center", sessionId, x, y, z),
  destroyAPR: (sessionId) => electron.ipcRenderer.invoke("viz:destroy-apr", sessionId),
  destroyMPR: (sessionId) => electron.ipcRenderer.invoke("viz:destroy-mpr", sessionId),
  onProgress: (callback) => {
    electron.ipcRenderer.on("viz:progress", (_event, payload) => callback(payload));
  },
  offProgress: () => {
    electron.ipcRenderer.removeAllListeners("viz:progress");
  },
  getVolumeHistogram: (sessionId) => electron.ipcRenderer.invoke("viz:get-volume-histogram", sessionId),
  getMaskStatistics: (sessionId, maskId) => electron.ipcRenderer.invoke("viz:get-mask-statistics", sessionId, maskId),
  getVolumeSpacing: (sessionId) => electron.ipcRenderer.invoke("viz:get-volume-spacing", sessionId),
  calculateBoneMetrics: (sessionId, maskId, roiMaskId) => electron.ipcRenderer.invoke(
    "viz:calculate-bone-metrics",
    sessionId,
    maskId,
    roiMaskId
  ),
  updatePreviewMask: (sessionId, minThreshold, maxThreshold, hexColor) => electron.ipcRenderer.invoke(
    "viz:update-preview-mask",
    sessionId,
    minThreshold,
    maxThreshold,
    hexColor
  ),
  clearPreviewMask: (sessionId) => electron.ipcRenderer.invoke("viz:clear-preview-mask", sessionId),
  createMaskFromThreshold: (sessionId, minThreshold, maxThreshold, hexColor, maskName) => electron.ipcRenderer.invoke(
    "viz:create-mask-from-threshold",
    sessionId,
    minThreshold,
    maxThreshold,
    hexColor,
    maskName
  ),
  createEmptyMask: (sessionId, hexColor, maskName) => electron.ipcRenderer.invoke("viz:create-empty-mask", sessionId, hexColor, maskName),
  selectMaskForEditing: (sessionId, maskId) => electron.ipcRenderer.invoke("viz:select-mask-for-editing", sessionId, maskId),
  setMaskTool: (maskTool) => electron.ipcRenderer.invoke("viz:set-mask-tool", maskTool),
  getMaskTool: () => electron.ipcRenderer.invoke("viz:get-mask-tool"),
  setMaskBrushRadius: (radius) => electron.ipcRenderer.invoke("viz:set-mask-brush-radius", radius),
  getMaskBrushRadius: () => electron.ipcRenderer.invoke("viz:get-mask-brush-radius"),
  deleteMask: (sessionId, maskId) => electron.ipcRenderer.invoke("viz:delete-mask", sessionId, maskId),
  saveMasks: (sessionId, folderPath, maskName) => electron.ipcRenderer.invoke("viz:save-masks", sessionId, folderPath, maskName),
  loadMasks: (sessionId, folderPath) => electron.ipcRenderer.invoke("viz:load-masks", sessionId, folderPath),
  exportMaskToStl: (sessionId, maskId, filepath, step) => electron.ipcRenderer.invoke(
    "viz:export-mask-to-stl",
    sessionId,
    maskId,
    filepath,
    step
  ),
  // Morphology and Boolean Operations
  maskMorphology2D: (sessionId, maskId, operation, kernelSize) => electron.ipcRenderer.invoke(
    "viz:mask-morphology-2d",
    sessionId,
    maskId,
    operation,
    kernelSize
  ),
  maskMorphology3D: (sessionId, maskId, operation, kernelSize) => electron.ipcRenderer.invoke(
    "viz:mask-morphology-3d",
    sessionId,
    maskId,
    operation,
    kernelSize
  ),
  maskBoolean: (sessionId, maskIdA, maskIdB, operation, name, color) => electron.ipcRenderer.invoke(
    "viz:mask-boolean",
    sessionId,
    maskIdA,
    maskIdB,
    operation,
    name,
    color
  ),
  maskInverse: (sessionId, maskId) => electron.ipcRenderer.invoke("viz:mask-inverse", sessionId, maskId),
  // Fat Analysis
  fatAnalyzeSeparateFat: (sessionId, maskId, lowThreshold, highThreshold) => electron.ipcRenderer.invoke(
    "viz:fat-analyze-separate-fat",
    sessionId,
    maskId,
    lowThreshold,
    highThreshold
  ),
  fatAnalyzeSeparateLung: (sessionId, maskId) => electron.ipcRenderer.invoke("viz:fat-analyze-separate-lung", sessionId, maskId),
  // Vascular Analysis
  vascularAnalyzeCompute: (sessionId, arg1, arg2) => electron.ipcRenderer.invoke("viz:vascular-analyze-compute", sessionId, arg1, arg2),
  vascularFilterKeepLargest: (maskBuffer, w, h, d) => electron.ipcRenderer.invoke("viz:vascular-filter-keep-largest", maskBuffer, w, h, d),
  set3DVramOptimized: (windowId, enabled) => electron.ipcRenderer.invoke("viz:set-3d-vram-optimized", windowId, enabled)
});
electron.contextBridge.exposeInMainWorld("electronAPI", {
  invoke: (channel, ...args) => electron.ipcRenderer.invoke(channel, ...args),
  send: (channel, ...args) => electron.ipcRenderer.send(channel, ...args),
  on: (channel, callback) => {
    electron.ipcRenderer.on(channel, (_event, ...args) => callback(...args));
  },
  off: (channel, callback) => {
    electron.ipcRenderer.removeListener(channel, callback);
  }
});
electron.contextBridge.exposeInMainWorld("nativeBridge", {
  windowControls: {
    minimize: () => electron.ipcRenderer.invoke("window-controls:minimize"),
    toggleMaximize: () => electron.ipcRenderer.invoke("window-controls:toggle-maximize"),
    close: () => electron.ipcRenderer.invoke("window-controls:close")
  },
  env: {
    platform: process.platform,
    versions: process.versions
  },
  // Dialog API
  dialog: {
    open: (dialogType, params) => electron.ipcRenderer.invoke("dialog:open", dialogType, params),
    confirm: (payload) => electron.ipcRenderer.invoke("dialog:confirm", payload),
    onResult: (callback) => {
      electron.ipcRenderer.on("dialog:result", (_event, result) => callback(result));
    },
    offResult: () => {
      electron.ipcRenderer.removeAllListeners("dialog:result");
    }
  },
  app: {
    restartRenderer: () => electron.ipcRenderer.invoke("app:restart-renderer"),
    openAprTestWindow: () => electron.ipcRenderer.invoke("app:open-apr-test-window")
  }
});
