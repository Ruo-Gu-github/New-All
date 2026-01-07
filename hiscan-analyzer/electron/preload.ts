import { ipcRenderer, contextBridge } from "electron";

function isDialogWindow() {
  try {
    return (
      Array.isArray(process.argv) &&
      process.argv.includes("--hiscan-window=dialog")
    );
  } catch {
    return false;
  }
}

function injectDialogTheme() {
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
  document.head?.appendChild(style);
}

try {
  if (isDialogWindow()) {
    window.addEventListener("DOMContentLoaded", injectDialogTheme, {
      once: true,
    });
  }
} catch {
  // ignore
}

// --------- Expose some API to the Renderer process ---------
contextBridge.exposeInMainWorld("ipcRenderer", {
  on(...args: Parameters<typeof ipcRenderer.on>) {
    const [channel, listener] = args;
    return ipcRenderer.on(channel, (event, ...args) =>
      listener(event, ...args)
    );
  },
  off(...args: Parameters<typeof ipcRenderer.off>) {
    const [channel, ...omit] = args;
    return ipcRenderer.off(channel, ...omit);
  },
  send(...args: Parameters<typeof ipcRenderer.send>) {
    const [channel, ...omit] = args;
    return ipcRenderer.send(channel, ...omit);
  },
  invoke(...args: Parameters<typeof ipcRenderer.invoke>) {
    const [channel, ...omit] = args;
    return ipcRenderer.invoke(channel, ...omit);
  },
});

// --------- DICOM API (window.dicomApi) ---------
contextBridge.exposeInMainWorld("dicomApi", {
  selectFolder: () => ipcRenderer.invoke("dicom:select-folder"),
  scanFolder: (folderPath: string) =>
    ipcRenderer.invoke("dicom:scan-folder", folderPath),
  getAllSeries: () => ipcRenderer.invoke("dicom:get-all-series"),
  removeSeries: (seriesId: string) =>
    ipcRenderer.invoke("dicom:remove-series", seriesId),
  cleanup: () => ipcRenderer.invoke("dicom:cleanup"),
});

// --------- Visualization API (window.visualizationApi) ---------
contextBridge.exposeInMainWorld("visualizationApi", {
  createAPR: (sessionId: string, folderPath: string) =>
    ipcRenderer.invoke("viz:create-apr", sessionId, folderPath),

  createMPR: (sessionId: string, folderPath: string) =>
    ipcRenderer.invoke("viz:create-mpr", sessionId, folderPath),

  renderAPRSlice: (
    sessionId: string,
    viewName: string,
    width: number,
    height: number
  ) =>
    ipcRenderer.invoke(
      "viz:render-apr-slice",
      sessionId,
      viewName,
      width,
      height
    ),

  captureAPRScreenshots: (
    sessionId: string,
    folderPath: string,
    selection: string[],
    width?: number,
    height?: number
  ) =>
    ipcRenderer.invoke(
      "viz:capture-apr-screenshots",
      sessionId,
      folderPath,
      selection,
      width,
      height
    ),

  updateCenter: (sessionId: string, x: number, y: number, z: number) =>
    ipcRenderer.invoke("viz:update-center", sessionId, x, y, z),

  updateRotation: (
    sessionId: string,
    angleX: number,
    angleY: number,
    angleZ: number
  ) =>
    ipcRenderer.invoke(
      "viz:update-rotation",
      sessionId,
      angleX,
      angleY,
      angleZ
    ),

  setCrosshairVisible: (sessionId: string, visible: boolean) =>
    ipcRenderer.invoke("viz:set-crosshair-visible", sessionId, visible),

  getAPRState: (sessionId: string, viewOrWindowId?: string) =>
    ipcRenderer.invoke("viz:get-apr-state", sessionId, viewOrWindowId),

  setWindowLevel: (
    sessionId: string,
    windowWidth: number,
    windowLevel: number
  ) =>
    ipcRenderer.invoke(
      "viz:set-window-level",
      sessionId,
      windowWidth,
      windowLevel
    ),
  getWindowLevel: (sessionId: string) =>
    ipcRenderer.invoke("viz:get-window-level", sessionId),

  // MIP/MinIP projection mode (0=Normal, 1=MIP, 2=MinIP)
  setProjectionMode: (sessionId: string, mode: number, thickness: number) =>
    ipcRenderer.invoke("viz:set-projection-mode", sessionId, mode, thickness),
  getProjectionMode: (sessionId: string) =>
    ipcRenderer.invoke("viz:get-projection-mode", sessionId),

  // 裁切 API
  cropVolume: (sessionId: string) =>
    ipcRenderer.invoke("viz:crop-volume", sessionId),
  applyCroppedVolume: (sessionId: string) =>
    ipcRenderer.invoke("viz:apply-cropped-volume", sessionId),

  // 裁切设置 API
  setCropShape: (shape: number) =>
    ipcRenderer.invoke("viz:set-crop-shape", shape),
  getCropShape: () => ipcRenderer.invoke("viz:get-crop-shape"),
  setCropCylinderDirection: (direction: number) =>
    ipcRenderer.invoke("viz:set-crop-cylinder-direction", direction),
  getCropCylinderDirection: () =>
    ipcRenderer.invoke("viz:get-crop-cylinder-direction"),
  setCropBoxSize: (
    sizeX: number,
    sizeY: number,
    sizeZ: number,
    volumeWidth: number,
    volumeHeight: number,
    volumeDepth: number
  ) =>
    ipcRenderer.invoke(
      "viz:set-crop-box-size",
      sizeX,
      sizeY,
      sizeZ,
      volumeWidth,
      volumeHeight,
      volumeDepth
    ),
  getCropSettings: () => ipcRenderer.invoke("viz:get-crop-settings"),

  processWindowEvents: () => ipcRenderer.invoke("viz:process-window-events"),
  embedWindow: (
    windowId: string,
    x: number,
    y: number,
    width: number,
    height: number
  ) => ipcRenderer.invoke("viz:embed-window", windowId, x, y, width, height),
  renderAllViews: () => ipcRenderer.invoke("viz:render-all-views"),
  invalidateAllWindows: () => ipcRenderer.invoke("viz:invalidate-all-windows"),
  invalidateWindow: (windowId: string) =>
    ipcRenderer.invoke("viz:invalidate-window", windowId),
  resetView: (windowId: string) =>
    ipcRenderer.invoke("viz:reset-view", windowId),
  set3DOrthogonalMode: (windowId: string, enabled: boolean) =>
    ipcRenderer.invoke("viz:set-3d-orthogonal-mode", windowId, enabled),
  set3DMaskIsoSurface: (windowId: string, enabled: boolean) =>
    ipcRenderer.invoke("viz:set-3d-mask-iso-surface", windowId, enabled),
  set3DLightParameters: (
    windowId: string,
    ambient: number,
    diffuse: number,
    specular: number
  ) =>
    ipcRenderer.invoke(
      "viz:set-3d-light-parameters",
      windowId,
      ambient,
      diffuse,
      specular
    ),
  set3DTransferFunction: (
    windowId: string,
    points: Array<{ value: number; r: number; g: number; b: number; a: number }>
  ) => ipcRenderer.invoke("viz:set-3d-transfer-function", windowId, points),
  getGpuInfo: (windowId: string) =>
    ipcRenderer.invoke("viz:get-gpu-info", windowId),

  // HIS4D (4D cine)
  packHis4dFromFolders: (
    outputPath: string,
    folders: string[],
    timestampsMs?: number[]
  ) => ipcRenderer.invoke("viz:pack-his4d", outputPath, folders, timestampsMs),

  createAPRFromHis4d: (sessionId: string, his4dPath: string) =>
    ipcRenderer.invoke("viz:create-apr-his4d", sessionId, his4dPath),

  his4dSetFrame: (sessionId: string, frameIndex: number) =>
    ipcRenderer.invoke("viz:his4d-set-frame", sessionId, frameIndex),

  his4dGetSessionInfo: (sessionId: string) =>
    ipcRenderer.invoke("viz:his4d-get-session-info", sessionId),
  hideAllWindows: () => ipcRenderer.invoke("viz:hide-all-windows"),
  showAllWindows: () => ipcRenderer.invoke("viz:show-all-windows"),
  destroyAll3DWindows: () => ipcRenderer.invoke("viz:destroy-all-3d-windows"),
  destroyAllWindows: () => ipcRenderer.invoke("viz:destroy-all-windows"),
  resizeWindow: (
    windowId: string,
    x: number,
    y: number,
    width: number,
    height: number
  ) => ipcRenderer.invoke("viz:resize-window", windowId, x, y, width, height),
  startRenderLoop: (targetFPS = 60) =>
    ipcRenderer.invoke("viz:start-render-loop", targetFPS),
  stopRenderLoop: () => ipcRenderer.invoke("viz:stop-render-loop"),
  refreshAllWindowsZOrder: () =>
    ipcRenderer.invoke("viz:refresh-all-windows-zorder"),
  refreshWindowZOrder: (windowId: string) =>
    ipcRenderer.invoke("viz:refresh-window-zorder", windowId),
  raiseAllWindows: () => ipcRenderer.invoke("viz:raise-all-windows"),
  raiseWindow: (windowId: string) =>
    ipcRenderer.invoke("viz:raise-window", windowId),
  setWindowToolType: (windowId: string, toolType: number) =>
    ipcRenderer.invoke("viz:set-window-tool-type", windowId, toolType),
  setWindowCropBoxVisible: (windowId: string, visible: boolean) =>
    ipcRenderer.invoke("viz:set-window-crop-box-visible", windowId, visible),
  enableAPRCropBox: (enable: boolean) =>
    ipcRenderer.invoke("viz:enable-apr-crop-box", enable),
  setAPRCropBox: (width: number, height: number, depth: number) =>
    ipcRenderer.invoke("viz:set-apr-crop-box", width, height, depth),
  setAPRCropBoxRange: (
    xStart: number,
    xEnd: number,
    yStart: number,
    yEnd: number,
    zStart: number,
    zEnd: number
  ) =>
    ipcRenderer.invoke(
      "viz:set-apr-crop-box-range",
      xStart,
      xEnd,
      yStart,
      yEnd,
      zStart,
      zEnd
    ),
  getAPRCropBox: () => ipcRenderer.invoke("viz:get-apr-crop-box"),
  isAPRCropBoxEnabled: () => ipcRenderer.invoke("viz:is-apr-crop-box-enabled"),

  getCompletedMeasurements: () =>
    ipcRenderer.invoke("viz:get-completed-measurements"),
  deleteMeasurement: (measurementId: number) =>
    ipcRenderer.invoke("viz:delete-measurement", measurementId),
  getMeasurementProfile: (
    sessionId: string,
    measurementId: number,
    maxPoints?: number
  ) =>
    ipcRenderer.invoke(
      "viz:get-measurement-profile",
      sessionId,
      measurementId,
      maxPoints
    ),
  getMeasurementRegionHistogram: (sessionId: string, measurementId: number) =>
    ipcRenderer.invoke(
      "viz:get-measurement-region-histogram",
      sessionId,
      measurementId
    ),
  updateMPRCenter: (sessionId: string, x: number, y: number, z: number) =>
    ipcRenderer.invoke("viz:update-mpr-center", sessionId, x, y, z),
  destroyAPR: (sessionId: string) =>
    ipcRenderer.invoke("viz:destroy-apr", sessionId),

  destroyMPR: (sessionId: string) =>
    ipcRenderer.invoke("viz:destroy-mpr", sessionId),

  onProgress: (
    callback: (data: { progress: number; message: string }) => void
  ) => {
    ipcRenderer.on("viz:progress", (_event, payload) => callback(payload));
  },
  offProgress: () => {
    ipcRenderer.removeAllListeners("viz:progress");
  },

  getVolumeHistogram: (sessionId: string) =>
    ipcRenderer.invoke("viz:get-volume-histogram", sessionId),

  getMaskStatistics: (sessionId: string, maskId: number) =>
    ipcRenderer.invoke("viz:get-mask-statistics", sessionId, maskId),

  getVolumeSpacing: (sessionId: string) =>
    ipcRenderer.invoke("viz:get-volume-spacing", sessionId),

  calculateBoneMetrics: (
    sessionId: string,
    maskId: number,
    roiMaskId?: number
  ) =>
    ipcRenderer.invoke(
      "viz:calculate-bone-metrics",
      sessionId,
      maskId,
      roiMaskId
    ),

  updatePreviewMask: (
    sessionId: string,
    minThreshold: number,
    maxThreshold: number,
    hexColor: string
  ) =>
    ipcRenderer.invoke(
      "viz:update-preview-mask",
      sessionId,
      minThreshold,
      maxThreshold,
      hexColor
    ),
  clearPreviewMask: (sessionId: string) =>
    ipcRenderer.invoke("viz:clear-preview-mask", sessionId),
  createMaskFromThreshold: (
    sessionId: string,
    minThreshold: number,
    maxThreshold: number,
    hexColor: string,
    maskName: string
  ) =>
    ipcRenderer.invoke(
      "viz:create-mask-from-threshold",
      sessionId,
      minThreshold,
      maxThreshold,
      hexColor,
      maskName
    ),

  createEmptyMask: (sessionId: string, hexColor: string, maskName: string) =>
    ipcRenderer.invoke("viz:create-empty-mask", sessionId, hexColor, maskName),

  selectMaskForEditing: (sessionId: string, maskId: number) =>
    ipcRenderer.invoke("viz:select-mask-for-editing", sessionId, maskId),

  setMaskTool: (maskTool: number) =>
    ipcRenderer.invoke("viz:set-mask-tool", maskTool),

  getMaskTool: () => ipcRenderer.invoke("viz:get-mask-tool"),

  setMaskBrushRadius: (radius: number) =>
    ipcRenderer.invoke("viz:set-mask-brush-radius", radius),

  getMaskBrushRadius: () => ipcRenderer.invoke("viz:get-mask-brush-radius"),
  deleteMask: (sessionId: string, maskId: number) =>
    ipcRenderer.invoke("viz:delete-mask", sessionId, maskId),
  saveMasks: (sessionId: string, folderPath: string, maskName: string) =>
    ipcRenderer.invoke("viz:save-masks", sessionId, folderPath, maskName),
  loadMasks: (sessionId: string, folderPath: string) =>
    ipcRenderer.invoke("viz:load-masks", sessionId, folderPath),

  exportMaskToStl: (
    sessionId: string,
    maskId: number,
    filepath: string,
    step: number
  ) =>
    ipcRenderer.invoke(
      "viz:export-mask-to-stl",
      sessionId,
      maskId,
      filepath,
      step
    ),

  // Morphology and Boolean Operations
  maskMorphology2D: (
    sessionId: string,
    maskId: number,
    operation: number,
    kernelSize: number
  ) =>
    ipcRenderer.invoke(
      "viz:mask-morphology-2d",
      sessionId,
      maskId,
      operation,
      kernelSize
    ),
  maskMorphology3D: (
    sessionId: string,
    maskId: number,
    operation: number,
    kernelSize: number
  ) =>
    ipcRenderer.invoke(
      "viz:mask-morphology-3d",
      sessionId,
      maskId,
      operation,
      kernelSize
    ),
  maskBoolean: (
    sessionId: string,
    maskIdA: number,
    maskIdB: number,
    operation: number,
    name?: string,
    color?: string
  ) =>
    ipcRenderer.invoke(
      "viz:mask-boolean",
      sessionId,
      maskIdA,
      maskIdB,
      operation,
      name,
      color
    ),
  maskInverse: (sessionId: string, maskId: number) =>
    ipcRenderer.invoke("viz:mask-inverse", sessionId, maskId),

  // Fat Analysis
  fatAnalyzeSeparateFat: (
    sessionId: string,
    maskId: number,
    lowThreshold: number,
    highThreshold: number
  ) =>
    ipcRenderer.invoke(
      "viz:fat-analyze-separate-fat",
      sessionId,
      maskId,
      lowThreshold,
      highThreshold
    ),
  fatAnalyzeSeparateLung: (sessionId: string, maskId: number) =>
    ipcRenderer.invoke("viz:fat-analyze-separate-lung", sessionId, maskId),

  // Vascular Analysis
  vascularAnalyzeCompute: (
    sessionId: string,
    arg1: number | Uint8Array,
    arg2?: number
  ) =>
    ipcRenderer.invoke("viz:vascular-analyze-compute", sessionId, arg1, arg2),

  vascularFilterKeepLargest: (
    maskBuffer: Uint8Array,
    w: number,
    h: number,
    d: number
  ) =>
    ipcRenderer.invoke("viz:vascular-filter-keep-largest", maskBuffer, w, h, d),

  set3DVramOptimized: (windowId: string, enabled: boolean) =>
    ipcRenderer.invoke("viz:set-3d-vram-optimized", windowId, enabled),
});

// --------- Dialog API ---------
contextBridge.exposeInMainWorld("electronAPI", {
  invoke: (channel: string, ...args: any[]) =>
    ipcRenderer.invoke(channel, ...args),
  send: (channel: string, ...args: any[]) => ipcRenderer.send(channel, ...args),
  on: (channel: string, callback: (...args: any[]) => void) => {
    ipcRenderer.on(channel, (_event, ...args) => callback(...args));
  },
  off: (channel: string, callback: (...args: any[]) => void) => {
    ipcRenderer.removeListener(channel, callback);
  },
});

// --------- Unified Native Bridge ---------
contextBridge.exposeInMainWorld("nativeBridge", {
  windowControls: {
    minimize: () => ipcRenderer.invoke("window-controls:minimize"),
    toggleMaximize: () => ipcRenderer.invoke("window-controls:toggle-maximize"),
    close: () => ipcRenderer.invoke("window-controls:close"),
  },
  env: {
    platform: process.platform,
    versions: process.versions,
  },
  // Dialog API
  dialog: {
    open: (dialogType: string, params: any) =>
      ipcRenderer.invoke("dialog:open", dialogType, params),
    confirm: (payload: {
      title?: string;
      message: string;
      confirmButtonText?: string;
      cancelButtonText?: string;
      width?: number;
      height?: number;
      showInactive?: boolean;
    }) => ipcRenderer.invoke("dialog:confirm", payload),
    onResult: (callback: (result: any) => void) => {
      ipcRenderer.on("dialog:result", (_event, result) => callback(result));
    },
    offResult: () => {
      ipcRenderer.removeAllListeners("dialog:result");
    },
  },
  app: {
    restartRenderer: () => ipcRenderer.invoke("app:restart-renderer"),
    openAprTestWindow: () => ipcRenderer.invoke("app:open-apr-test-window"),
  },
});
