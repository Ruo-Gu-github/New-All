type IpcListener = (...args: any[]) => void;

function ensure<T>(key: keyof Window, value: T) {
  const w = window as any;
  if (w[key] == null) w[key] = value;
}

function notAvailable(method: string) {
  return async (..._args: any[]) => ({
    success: false,
    error: `Native backend disabled: ${method}`,
  });
}

// ---- ipcRenderer stub (so Vite web dev doesn't crash) ----
ensure("ipcRenderer" as any, {
  on: (_channel: string, _listener: IpcListener) => {},
  off: (_channel: string, _listener?: IpcListener) => {},
  send: (_channel: string, ..._args: any[]) => {},
  invoke: async (_channel: string, ..._args: any[]) => ({
    success: false,
    error: `Native backend disabled: ${_channel}`,
  }),
});

// ---- electronAPI stub (used by dialogs) ----
ensure("electronAPI" as any, {
  invoke: async (channel: string, ..._args: any[]) => ({
    success: false,
    error: `Native backend disabled: ${channel}`,
  }),
  send: (_channel: string, ..._args: any[]) => {},
  on: (_channel: string, _callback: (...args: any[]) => void) => {},
  off: (_channel: string, _callback: (...args: any[]) => void) => {},
});

// ---- dicomApi stub ----
ensure("dicomApi" as any, {
  selectFolder: async () => null,
  scanFolder: notAvailable("dicomApi.scanFolder"),
  getAllSeries: async () => [],
  removeSeries: async (_seriesId: string) => ({ success: true }),
  loadSeries: notAvailable("dicomApi.loadSeries"),
  getSlice: notAvailable("dicomApi.getSlice"),
  cleanup: async () => ({ success: true }),
});

// ---- visualizationApi stub ----
ensure("visualizationApi" as any, {
  createAPR: notAvailable("visualizationApi.createAPR"),
  createMPR: notAvailable("visualizationApi.createMPR"),
  onProgress: (
    _callback: (data: { progress: number; message: string }) => void
  ) => {},
  offProgress: () => {},

  renderAPRSlice: notAvailable("visualizationApi.renderAPRSlice"),

  captureAPRScreenshots: notAvailable("visualizationApi.captureAPRScreenshots"),
  updateCenter: async () => {},
  updateRotation: async () => {},
  setWindowLevel: async () => ({
    success: false,
    error: "Native backend disabled",
  }),
  getWindowLevel: async () => ({
    success: false,
    error: "Native backend disabled",
  }),
  setCrosshairVisible: async () => {},
  processWindowEvents: async () => {},
  embedWindow: async () => {},
  renderAllViews: async () => {},
  invalidateAllWindows: async () => {},
  invalidateWindow: async () => {},
  resetView: async () => false,
  set3DOrthogonalMode: async () => true,
  set3DMaskIsoSurface: async () => false,
  set3DVramOptimized: async () => true,
  destroyAll3DWindows: async () => {},
  destroyAllWindows: async () => {},
  hideAllWindows: async () => {},
  showAllWindows: async () => {},
  resizeWindow: async () => true,
  startRenderLoop: async () => true,
  stopRenderLoop: async () => {},
  setWindowToolType: async () => true,
  setWindowCropBoxVisible: async () => true,
  enableAPRCropBox: async () => {},
  setAPRCropBox: async () => {},
  setAPRCropBoxRange: async () => ({
    success: false,
    error: "Native backend disabled",
  }),
  getAPRCropBox: async () => ({
    success: false,
    error: "Native backend disabled",
  }),
  isAPRCropBoxEnabled: async () => false,
  setCropShape: async () => ({ success: true }),
  setCropCylinderDirection: async () => ({ success: true }),
  setCropBoxSize: async () => {},
  getCropSettings: async () => ({
    shape: 0,
    cylinderDirection: 0,
    cropBox: {
      xStart: 0,
      xEnd: 100,
      yStart: 0,
      yEnd: 100,
      zStart: 0,
      zEnd: 100,
    },
    enabled: false,
  }),
  getCompletedMeasurements: async () => [],
  deleteMeasurement: async () => false,
  getMeasurementProfile: async () => ({ axis: [], values: [] }),
  getMeasurementRegionHistogram: async () => ({
    success: false,
    error: "Native backend disabled",
    data: [],
    minValue: 0,
    maxValue: 0,
  }),
  updateMPRCenter: async () => ({
    success: false,
    error: "Native backend disabled",
  }),
  destroyAPR: async () => {},
  destroyMPR: async () => {},

  getVolumeSpacing: async () => ({ spacingX: 1, spacingY: 1, spacingZ: 1 }),
  calculateBoneMetrics: async (
    _sessionId?: string,
    _maskId?: number,
    _roiMaskId?: number
  ) => ({
    success: false,
    error: "Native backend disabled",
  }),

  getVolumeHistogram: async () => ({ data: [], minValue: 0, maxValue: 0 }),
  getMaskStatistics: async () => ({
    histogram: Array.from({ length: 256 }, () => 0),
    minValue: 0,
    maxValue: 0,
    mean: 0,
    stdDev: 0,
    count: 0,
    volumeMm3: 0,
  }),
  updatePreviewMask: async () => {},
  clearPreviewMask: async () => {},
  createMaskFromThreshold: async () => ({
    success: false,
    error: "Native backend disabled",
  }),
  createEmptyMask: async () => ({
    success: false,
    error: "Native backend disabled",
  }),
  selectMaskForEditing: async () => ({
    success: false,
    error: "Native backend disabled",
  }),
  setMaskTool: async () => ({
    success: false,
    error: "Native backend disabled",
  }),
  getMaskTool: async () => ({
    success: false,
    error: "Native backend disabled",
  }),
  setMaskBrushRadius: async () => ({
    success: false,
    error: "Native backend disabled",
  }),
  getMaskBrushRadius: async () => ({
    success: false,
    error: "Native backend disabled",
  }),
  deleteMask: async () => {},
  saveMasks: async () => ({ success: false, error: "Native backend disabled" }),
  loadMasks: async () => ({ success: false, error: "Native backend disabled" }),

  exportMaskToStl: async () => ({
    success: false,
    error: "Native backend disabled",
  }),

  // Morphology and Boolean Operations
  maskMorphology2D: async () => ({
    success: false,
    error: "Native backend disabled",
  }),
  maskMorphology3D: async () => ({
    success: false,
    error: "Native backend disabled",
  }),
  maskBoolean: async () => ({
    success: false,
    error: "Native backend disabled",
  }),
  maskInverse: async () => ({
    success: false,
    error: "Native backend disabled",
  }),
});

// ---- offscreenApi stub ----
ensure("offscreenApi" as any, {
  createOffscreenAPR: notAvailable("offscreenApi.createOffscreenAPR"),
  destroyOffscreenAPR: notAvailable("offscreenApi.destroyOffscreenAPR"),
  setSliceDirection: async () => ({ success: false }),
  setCenter: async () => ({ success: false }),
  handleMouseEvent: async () => ({ success: false }),
  render: async () => ({ success: false }),
  getFrame: async () => null,
  hasNewFrame: async () => false,
  linkCenters: async () => ({ success: false }),
  onFrameReady: (_callback: (sessionId: string) => void) => {},
  offFrameReady: () => {},
});

// ---- nativeBridge stub (window controls + dialog wiring) ----
ensure("nativeBridge" as any, {
  windowControls: {
    minimize: async () => {},
    toggleMaximize: async () => {},
    close: async () => {},
  },
  dialog: {
    open: async (_dialogType: string, _params: any) => ({
      success: false,
      error: "Native backend disabled",
    }),
    onResult: (_callback: (result: any) => void) => {},
    offResult: () => {},
  },
  env: {
    platform: "web",
    versions: {},
  },
});
