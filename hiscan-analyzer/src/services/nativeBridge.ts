type WindowControls = {
  minimize: () => Promise<void>;
  toggleMaximize: () => Promise<void>;
  close: () => Promise<void>;
};

type NativePayload = ArrayBuffer | ArrayBufferView | string;

type NativeModuleApi = {
  runJob: (payload: NativePayload, onProgress: (percent: number) => void) => Promise<unknown>;
  getRecords: (dbPath?: string) => Promise<unknown>;
};

type NativeBridgeShape = {
  windowControls?: Partial<WindowControls>;
  native?: Partial<NativeModuleApi>;
  env?: Record<string, unknown>;
};

const bridge: NativeBridgeShape = typeof window !== 'undefined'
  ? (window as typeof window & { nativeBridge?: NativeBridgeShape }).nativeBridge ?? {}
  : {};

function fallbackPromise(): Promise<void> {
  return Promise.resolve();
}

export function useWindowControls(): WindowControls {
  const controls = bridge.windowControls ?? {};
  return {
    minimize: () => (controls.minimize?.() ?? fallbackPromise()),
    toggleMaximize: () => (controls.toggleMaximize?.() ?? fallbackPromise()),
    close: () => (controls.close?.() ?? fallbackPromise()),
  };
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
    console.warn('[nativeBridge] getRecords unavailable');
    return [];
  }
  return bridge.native.getRecords(dbPath);
}
