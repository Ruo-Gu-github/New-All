/// <reference types="vite/client" />

declare module "*.vue" {
  import type { DefineComponent } from "vue";
  const component: DefineComponent<
    Record<string, unknown>,
    Record<string, unknown>,
    any
  >;
  export default component;
}

// UI-only mode: these APIs may be injected by Electron preload OR stubbed in `src/native-stub.ts`.
interface Window {
  ipcRenderer?: {
    on: (channel: string, listener: (...args: any[]) => void) => void;
    off: (channel: string, listener?: (...args: any[]) => void) => void;
    send: (channel: string, ...args: any[]) => void;
    invoke: (channel: string, ...args: any[]) => Promise<any>;
  };
  electronAPI?: {
    invoke: (channel: string, ...args: any[]) => Promise<any>;
    send: (channel: string, ...args: any[]) => void;
    on: (channel: string, callback: (...args: any[]) => void) => void;
    off: (channel: string, callback: (...args: any[]) => void) => void;
  };
  dicomApi?: any;
  visualizationApi?: any;
  offscreenApi?: any;
  nativeBridge?: any;
  nativeApi?: any;
}
