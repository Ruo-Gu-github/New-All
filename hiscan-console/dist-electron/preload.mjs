"use strict";
const electron = require("electron");
electron.contextBridge.exposeInMainWorld("ipcRenderer", {
  on(...args) {
    const [channel, listener] = args;
    return electron.ipcRenderer.on(channel, (event, ...args2) => listener(event, ...args2));
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
  // You can expose other APTs you need here.
  // ...
});
electron.contextBridge.exposeInMainWorld("nativeBridge", {
  windowControls: {
    minimize: () => electron.ipcRenderer.invoke("window-controls:minimize"),
    toggleMaximize: () => electron.ipcRenderer.invoke("window-controls:toggle-maximize"),
    close: () => electron.ipcRenderer.invoke("window-controls:close")
  },
  preview: {
    createWindow: (options) => electron.ipcRenderer.invoke("preview:create-window", options),
    getPreviewBuffer: () => electron.ipcRenderer.invoke("preview:get-buffer")
  },
  dicom: {
    loadSeries: (payload) => electron.ipcRenderer.invoke("dicom:load-series", payload)
  },
  env: {
    platform: process.platform,
    versions: process.versions
  }
});
