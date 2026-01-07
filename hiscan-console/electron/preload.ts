import { ipcRenderer, contextBridge } from 'electron'

// --------- Expose some API to the Renderer process ---------
contextBridge.exposeInMainWorld('ipcRenderer', {
  on(...args: Parameters<typeof ipcRenderer.on>) {
    const [channel, listener] = args
    return ipcRenderer.on(channel, (event, ...args) => listener(event, ...args))
  },
  off(...args: Parameters<typeof ipcRenderer.off>) {
    const [channel, ...omit] = args
    return ipcRenderer.off(channel, ...omit)
  },
  send(...args: Parameters<typeof ipcRenderer.send>) {
    const [channel, ...omit] = args
    return ipcRenderer.send(channel, ...omit)
  },
  invoke(...args: Parameters<typeof ipcRenderer.invoke>) {
    const [channel, ...omit] = args
    return ipcRenderer.invoke(channel, ...omit)
  },

  // You can expose other APTs you need here.
  // ...
})

contextBridge.exposeInMainWorld('nativeBridge', {
  windowControls: {
    minimize: () => ipcRenderer.invoke('window-controls:minimize'),
    toggleMaximize: () => ipcRenderer.invoke('window-controls:toggle-maximize'),
    close: () => ipcRenderer.invoke('window-controls:close'),
  },
  preview: {
    createWindow: (options?: { width?: number; height?: number; text?: string }) => ipcRenderer.invoke('preview:create-window', options),
    getPreviewBuffer: () => ipcRenderer.invoke('preview:get-buffer'),
  },
  dicom: {
    loadSeries: (payload: { files: Array<{ path?: string; name?: string }> }) => ipcRenderer.invoke('dicom:load-series', payload),
  },
  env: {
    platform: process.platform,
    versions: process.versions,
  },
})
