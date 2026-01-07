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
    APP_ROOT: string
    /** /dist/ or /public/ */
    VITE_PUBLIC: string
  }
}

// Used in Renderer process, expose in `preload.ts`
interface Window {
  ipcRenderer: import('electron').IpcRenderer
  nativeBridge?: {
    windowControls?: {
      minimize: () => Promise<void>
      toggleMaximize: () => Promise<void>
      close: () => Promise<void>
    }
    preview?: {
      createWindow: (options?: { width?: number; height?: number; text?: string }) => Promise<string>
      getPreviewBuffer?: () => Promise<{
        width: number
        height: number
        data: Uint8Array | ArrayBuffer | ArrayBufferView | number[]
        text?: string
      }>
    }
    dicom?: {
      loadSeries: (payload: { files: Array<{ path?: string; name?: string }> }) => Promise<{
        sampleName?: string
        width?: number
        height?: number
        sliceCount?: number
        pixelSpacing?: [number, number]
        thumbnail?: string
      } | null>
    }
    native?: Record<string, unknown>
    env?: Record<string, unknown>
  }
}
