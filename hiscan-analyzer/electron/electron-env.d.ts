/// <reference types="vite-plugin-electron/electron-env" />

declare namespace NodeJS {
  interface ProcessEnv {
    /**
     * The built directory structure
     *
     * ```tree
     * ├─┬─┬ dist
     * │ │ └── index.html
     * │ │
     * │ ├─┬ dist-electron
     * │ │ ├── main.js
     * │ │ └── preload.js
     * │
     * ```
     */
    APP_ROOT: string;
    /** /dist/ or /public/ */
    VITE_PUBLIC: string;
  }
}

// Used in Renderer process, expose in `preload.ts`
interface Window {
  ipcRenderer: import("electron").IpcRenderer;
  visualizationApi: {
    createAPR: (
      sessionId: string,
      folderPath: string
    ) => Promise<{
      success: boolean;
      error?: string;
      width?: number;
      height?: number;
      depth?: number;
      centerX?: number;
      centerY?: number;
      centerZ?: number;
      // Preferred stable IDs for embed/resize APIs
      windowIdAxial?: string;
      windowIdCoronal?: string;
      windowIdSagittal?: string;
      windowId3D?: string;
      // Legacy native HWNDs returned as strings (avoid JS Number precision loss)
      hwndAxial?: string;
      hwndCoronal?: string;
      hwndSagittal?: string;
      hwnd3D?: string;
    }>;
    renderAPRSlice: (
      sessionId: string,
      viewName: string,
      width: number,
      height: number
    ) => Promise<any>;
    updateCenter: (
      sessionId: string,
      x: number,
      y: number,
      z: number
    ) => Promise<void>;
    updateRotation: (
      sessionId: string,
      angleX: number,
      angleY: number,
      angleZ: number
    ) => Promise<void>;
    getAPRState: (sessionId: string, viewOrWindowId?: string) => Promise<{
      success: boolean;
      error?: string;
      centerX?: number;
      centerY?: number;
      centerZ?: number;
      rotX?: number;
      rotY?: number;
      rotZ?: number;
      zoom?: number;
    }>;

      setWindowLevel: (
        sessionId: string,
        windowWidth: number,
        windowLevel: number
      ) => Promise<{ success: boolean; error?: string }>;
      getWindowLevel: (sessionId: string) => Promise<
        | { success: true; windowWidth?: number; windowLevel?: number }
        | { success: false; error?: string }
      >;
    processWindowEvents: () => Promise<void>;
    embedWindow: (
      windowId: string,
      x: number,
      y: number,
      width: number,
      height: number
    ) => Promise<void>;
        set3DVramOptimized: (windowId: string, enabled: boolean) => Promise<boolean>;
        set3DOrthogonalMode: (
    invalidateAllWindows: () => Promise<void>;
    invalidateWindow: (windowId: string) => Promise<void>;
    set3DOrthogonalMode: (
      windowId: string,
      enabled: boolean
    ) => Promise<boolean>;
    getGpuInfo: (windowId: string) => Promise<{
      success: boolean;
      error?: string;
      vendor?: string;
      renderer?: string;
      version?: string;
    }>;

    // HIS4D (4D cine)
    packHis4dFromFolders: (
      outputPath: string,
      folders: string[],
      timestampsMs?: number[]
    ) => Promise<{
      success: boolean;
      error?: string;
      outputPath?: string;
      width?: number;
      height?: number;
      depth?: number;
      frameCount?: number;
    }>;
    createAPRFromHis4d: (
      sessionId: string,
      his4dPath: string
    ) => Promise<{
      success: boolean;
      error?: string;
      sessionId?: string;
      his4dPath?: string;
      width?: number;
      height?: number;
      depth?: number;
      frameCount?: number;
      timestampsMs?: number[];
      windowIdAxial?: string;
      windowIdCoronal?: string;
      windowIdSagittal?: string;
      windowId3D?: string;
      hwndAxial?: string;
      hwndCoronal?: string;
      hwndSagittal?: string;
      hwnd3D?: string;
    }>;
    his4dSetFrame: (
      sessionId: string,
      frameIndex: number
    ) => Promise<{ success: boolean; error?: string; frameIndex?: number }>;
    his4dGetSessionInfo: (sessionId: string) => Promise<{
      success: boolean;
      error?: string;
      width?: number;
      height?: number;
      depth?: number;
      frameCount?: number;
      currentFrame?: number;
    }>;
    hideAllWindows: () => Promise<void>;
    showAllWindows: () => Promise<void>;
    destroyAll3DWindows: () => Promise<void>;
    destroyAllWindows: () => Promise<void>;
    resizeWindow: (
      windowId: string,
      x: number,
      y: number,
      width: number,
      height: number
    ) => Promise<boolean>;
    startRenderLoop: (targetFPS?: number) => Promise<boolean>;
    stopRenderLoop: () => Promise<void>;
    setWindowToolType: (windowId: string, toolType: number) => Promise<boolean>;
    enableAPRCropBox: (enable: boolean) => Promise<void>;
    setAPRCropBox: (
      width: number,
      height: number,
      depth: number
    ) => Promise<void>;
      setAPRCropBoxRange: (
        xStart: number,
        xEnd: number,
        yStart: number,
        yEnd: number,
        zStart: number,
        zEnd: number
      ) => Promise<{ success: boolean; error?: string }>;
      getAPRCropBox: () => Promise<
        | { success: true; xStart: number; xEnd: number; yStart: number; yEnd: number; zStart: number; zEnd: number }
        | { success: false; error: string }
      >;
    isAPRCropBoxEnabled: () => Promise<boolean>;
    // 裁切设置 API
    cropVolume: (sessionId: string) => Promise<{
      success: boolean;
      error?: string;
      width?: number;
      height?: number;
      depth?: number;
      spacingX?: number;
      spacingY?: number;
      spacingZ?: number;
    }>;
    applyCroppedVolume: (sessionId: string) => Promise<{
      success: boolean;
      error?: string;
      width?: number;
      height?: number;
      depth?: number;
    }>;
    setCropShape: (shape: number) => Promise<{ success: boolean; error?: string }>;
    getCropShape: () => Promise<number>;
    setCropCylinderDirection: (direction: number) => Promise<{ success: boolean; error?: string }>;
    getCropCylinderDirection: () => Promise<number>;
    setCropBoxSize: (
      sizeX: number,
      sizeY: number,
      sizeZ: number,
      volumeWidth: number,
      volumeHeight: number,
      volumeDepth: number
    ) => Promise<{ success: boolean; error?: string }>;
    getCropSettings: () => Promise<{
      shape: number;
      cylinderDirection: number;
      enabled: boolean;
      cropBox: {
        xStart?: number;
        xEnd?: number;
        yStart?: number;
        yEnd?: number;
        zStart?: number;
        zEnd?: number;
      };
    }>;
    getCompletedMeasurements: () => Promise<
      Array<{
        sessionId: string;
        id: number;
        toolType: number;
        result: number;
        isAPR: boolean;
        sliceDirection: number;
        sliceIndex: number;
        centerX: number;
        centerY: number;
        centerZ: number;
        rotX: number;
        rotY: number;
        rotZ: number;
      }>
    >;
    getMeasurementProfile: (
      sessionId: string,
      measurementId: number,
      maxPoints?: number
    ) => Promise<
      | {
          axis: number[];
          values: number[];
        }
      | { success: false; error?: string }
    >;
    updateMPRCenter: (
      sessionId: string,
      x: number,
      y: number,
      z: number
    ) => Promise<{ success: boolean; error?: string }>;
    destroyAPR: (sessionId: string) => Promise<void>;
    onProgress: (
      callback: (data: { progress: number; message: string }) => void
    ) => void;
    offProgress: () => void;
    // Mask相关API
    getVolumeHistogram: (sessionId: string) => Promise<{
      data: number[];
      minValue: number;
      maxValue: number;
    }>;

    getMaskStatistics: (sessionId: string, maskId: number) => Promise<{
      histogram: number[];
      minValue: number;
      maxValue: number;
      mean: number;
      stdDev: number;
      count: number;
      volumeMm3: number;
    }>;
    updatePreviewMask: (
      sessionId: string,
      minThreshold: number,
      maxThreshold: number,
      hexColor: string
    ) => Promise<void>;
    clearPreviewMask: (sessionId: string) => Promise<void>;
    createMaskFromThreshold: (
      sessionId: string,
      minThreshold: number,
      maxThreshold: number,
      hexColor: string,
      maskName: string
    ) => Promise<{
      success: boolean;
      maskId?: number;
      error?: string;
    }>;

    createEmptyMask: (
      sessionId: string,
      hexColor: string,
      maskName: string
    ) => Promise<{
      success: boolean;
      maskId?: number;
      error?: string;
    }>;

    selectMaskForEditing: (
      sessionId: string,
      maskId: number
    ) => Promise<{ success: boolean; error?: string }>;

    setMaskTool: (maskTool: number) => Promise<{ success: boolean; error?: string }>;
    getMaskTool: () => Promise<{ success: boolean; maskTool?: number; error?: string }>;

    setMaskBrushRadius: (
      radius: number
    ) => Promise<{ success: boolean; error?: string }>;
    getMaskBrushRadius: () => Promise<{ success: boolean; radius?: number; error?: string }>;
    deleteMask: (sessionId: string, maskId: number) => Promise<void>;
    saveMasks: (
      sessionId: string,
      folderPath: string,
      maskName: string
    ) => Promise<{
      success: boolean;
      error?: string;
      filePath?: string;
    }>;
    loadMasks: (
      sessionId: string,
      folderPath: string
    ) => Promise<{
      success: boolean;
      cancelled?: boolean;
      error?: string;
      masks?: Array<{
        maskId: number;
        name: string;
        color: string;
        visible: boolean;
        minThreshold: number;
        maxThreshold: number;
      }>;
    }>;

    // Morphology and Boolean Operations
    maskMorphology2D: (
      sessionId: string,
      maskId: number,
      operation: number,
      kernelSize: number
    ) => Promise<{ success: boolean; error?: string }>;
    maskMorphology3D: (
      sessionId: string,
      maskId: number,
      operation: number,
      kernelSize: number
    ) => Promise<{ success: boolean; error?: string }>;
    maskBoolean: (
      sessionId: string,
      maskIdA: number,
      maskIdB: number,
      operation: number,
      name?: string,
      color?: string
    ) => Promise<{ success: boolean; newMaskId?: number; error?: string }>;
    maskInverse: (
      sessionId: string,
      maskId: number
    ) => Promise<{ success: boolean; error?: string }>;
  };
  nativeBridge?: {
    windowControls?: {
      minimize: () => Promise<void>;
      toggleMaximize: () => Promise<void>;
      close: () => Promise<void>;
    };
    dicom?: {
      loadSeries: (payload: {
        files: Array<{ name: string; path?: string; data?: Uint8Array }>;
      }) => Promise<
        | {
            sampleName?: string;
            width?: number;
            height?: number;
            pixelSpacing?: [number, number];
            sliceCount?: number;
            thumbnail?: string;
          }
        | unknown
      >;
    };
    dialog?: {
      open: (
        dialogType: string,
        params: any
      ) => Promise<{ success: boolean; error?: string }>;
      onResult: (callback: (result: any) => void) => void;
      offResult: () => void;
    };
    native?: Record<string, unknown>;
    env?: Record<string, unknown>;
  };
  offscreenApi: {
    // 创建和销毁
    createOffscreenAPR: (
      sessionId: string,
      folderPath: string,
      width: number,
      height: number
    ) => Promise<{
      success: boolean;
      error?: string;
    }>;
    destroyOffscreenAPR: (sessionId: string) => Promise<{
      success: boolean;
      error?: string;
    }>;
    // 配置
    setSliceDirection: (
      sessionId: string,
      direction: number
    ) => Promise<{ success: boolean }>;
    setCenter: (
      sessionId: string,
      x: number,
      y: number,
      z: number
    ) => Promise<{ success: boolean }>;
    // 交互
    handleMouseEvent: (
      sessionId: string,
      eventType: string,
      button: number,
      x: number,
      y: number,
      deltaX: number,
      deltaY: number,
      modifiers: number
    ) => Promise<{ success: boolean }>;
    // 渲染
    render: (sessionId: string) => Promise<{ success: boolean }>;
    getFrame: (sessionId: string) => Promise<{
      pixelData: Buffer;
      width: number;
      height: number;
      timestamp: number;
    } | null>;
    hasNewFrame: (sessionId: string) => Promise<boolean>;
    // 多视图同步
    linkCenters: (sessionIds: string[]) => Promise<{ success: boolean }>;
    // 事件
    onFrameReady: (callback: (data: { sessionId: string }) => void) => void;
    offFrameReady: (callback: (data: { sessionId: string }) => void) => void;
  };
  electronAPI?: {
    invoke: (channel: string, ...args: any[]) => Promise<any>;
    send: (channel: string, ...args: any[]) => void;
    on: (channel: string, callback: (...args: any[]) => void) => void;
    off: (channel: string, callback: (...args: any[]) => void) => void;
  };
}
