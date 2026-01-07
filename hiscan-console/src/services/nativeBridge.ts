type WindowControls = {
  minimize: () => Promise<void>;
  toggleMaximize: () => Promise<void>;
  close: () => Promise<void>;
};

type NativePayload = ArrayBuffer | ArrayBufferView | string;

type PreviewOptions = {
  width?: number;
  height?: number;
  text?: string;
};

type PreviewBuffer = {
  width: number;
  height: number;
  data: Uint8Array;
  text?: string;
};

type DicomSeriesResult = {
  sampleName?: string;
  width?: number;
  height?: number;
  sliceCount?: number;
  pixelSpacing?: [number, number];
  thumbnail?: string;
};

type NativeApi = {
  runJob?: (payload: NativePayload, onProgress: (percent: number) => void) => Promise<unknown>;
  getRecords?: (dbPath?: string) => Promise<unknown>;
  invokeHardware?: (command: string, payload?: unknown) => Promise<unknown>;
};

type NativeBridgeShape = {
  windowControls?: Partial<WindowControls>;
  native?: NativeApi;
  preview?: {
    createWindow?: (options?: PreviewOptions) => Promise<string>;
    getPreviewBuffer?: () => Promise<{
      width: number;
      height: number;
      data: Uint8Array | ArrayBuffer | ArrayBufferView | number[];
      text?: string;
    }>;
  };
  dicom?: {
    loadSeries?: (payload: { files: Array<{ path?: string; name?: string }> }) => Promise<DicomSeriesResult | null>;
  };
  env?: Record<string, unknown>;
};

const bridge: NativeBridgeShape = typeof window !== 'undefined'
  ? (window as typeof window & { nativeBridge?: NativeBridgeShape }).nativeBridge ?? {}
  : {};

function noopPromise(): Promise<void> {
  return Promise.resolve();
}

export function useWindowControls(): WindowControls {
  const controls = bridge.windowControls ?? {};
  return {
    minimize: () => (controls.minimize?.() ?? noopPromise()),
    toggleMaximize: () => (controls.toggleMaximize?.() ?? noopPromise()),
    close: () => (controls.close?.() ?? noopPromise()),
  };
}

export async function invokeHardware(command: string, payload: unknown = {}): Promise<unknown> {
  if (!bridge.native?.invokeHardware) {
    console.warn('[nativeBridge] invokeHardware unavailable');
    return { ack: false, command, payload };
  }
  return bridge.native.invokeHardware(command, payload);
}

export async function runNativeJob(payload: NativePayload, onProgress: (percent: number) => void = () => {}): Promise<unknown> {
  if (!bridge.native?.runJob) {
    console.warn('[nativeBridge] runJob unavailable');
    return { status: 'unavailable' };
  }
  return bridge.native.runJob(payload, onProgress);
}

export async function getNativeRecords(dbPath = ''): Promise<unknown> {
  if (!bridge.native?.getRecords) {
    return [];
  }
  return bridge.native.getRecords(dbPath);
}

// 获取GDI预览像素buffer（BGRA Uint8Array）
async function invokeNativePreviewCreate(options: PreviewOptions): Promise<string> {
  if (!bridge.preview?.createWindow) {
    console.warn('[nativeBridge] preview.createWindow unavailable');
    return '';
  }
  try {
    return await bridge.preview.createWindow(options);
  } catch (error) {
    console.error('[nativeBridge] preview.createWindow failed', error);
    return '';
  }
}

export async function getNativePreviewBuffer(): Promise<PreviewBuffer | null> {
  const getter = bridge.preview?.getPreviewBuffer;
  if (!getter) {
    console.warn('[nativeBridge] preview.getPreviewBuffer unavailable');
    return null;
  }
  try {
    const payload = await getter();
    if (!payload) return null;
    const width = typeof payload.width === 'number' ? payload.width : 0;
    const height = typeof payload.height === 'number' ? payload.height : 0;
    let raw = payload.data;
    let data: Uint8Array | null = null;
    if (raw instanceof Uint8Array) {
      data = raw;
    } else if (raw instanceof ArrayBuffer) {
      data = new Uint8Array(raw);
    } else if (ArrayBuffer.isView(raw)) {
      const view = raw as ArrayBufferView;
      data = new Uint8Array(view.buffer, view.byteOffset, view.byteLength);
    } else if (Array.isArray(raw)) {
      data = new Uint8Array(raw as number[]);
    }
    if (!data) {
      console.warn('[nativeBridge] getPreviewBuffer returned unsupported data type');
      return null;
    }
    return {
      width,
      height,
      data,
      text: typeof payload.text === 'string' ? payload.text : undefined,
    };
  } catch (error) {
    console.error('[nativeBridge] getPreviewBuffer failed', error);
    return null;
  }
}

export async function createNativePreviewWindow(options: PreviewOptions = {}): Promise<string> {
  return invokeNativePreviewCreate(options);
}
