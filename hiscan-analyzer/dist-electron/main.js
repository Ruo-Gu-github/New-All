var __defProp = Object.defineProperty;
var __defNormalProp = (obj, key, value) => key in obj ? __defProp(obj, key, { enumerable: true, configurable: true, writable: true, value }) : obj[key] = value;
var __publicField = (obj, key, value) => __defNormalProp(obj, typeof key !== "symbol" ? key + "" : key, value);
import { app, Menu, BrowserWindow, ipcMain, dialog, nativeImage, webContents } from "electron";
import fs from "node:fs";
import path from "node:path";
import { createRequire } from "node:module";
app.disableHardwareAcceleration();
try {
  const tempBase = process.env.TEMP || process.env.TMP;
  if (tempBase) {
    const userDataDir = path.join(tempBase, "hiscan-analyzer-userdata");
    app.setPath("userData", userDataDir);
    app.setPath("cache", path.join(userDataDir, "Cache"));
    console.log("[app] userData/cache:", {
      userData: userDataDir,
      cache: path.join(userDataDir, "Cache")
    });
  }
} catch (e) {
  console.warn("[app] Failed to set temp userData/cache:", e);
}
function fileUrlToPlatformPath(urlStr) {
  const url = new URL(urlStr);
  let pathname = decodeURIComponent(url.pathname);
  if (process.platform === "win32") {
    if (pathname.startsWith("/")) pathname = pathname.slice(1);
    pathname = pathname.replaceAll("/", "\\");
  }
  return pathname;
}
const __filename$1 = fileUrlToPlatformPath(import.meta.url);
const __dirname$1 = path.dirname(__filename$1);
process.env.APP_ROOT = path.join(__dirname$1, "..");
const VITE_DEV_SERVER_URL = process.env["VITE_DEV_SERVER_URL"];
const MAIN_DIST = path.join(process.env.APP_ROOT, "dist-electron");
const RENDERER_DIST = path.join(process.env.APP_ROOT, "dist");
const IS_DEV = !app.isPackaged && !!VITE_DEV_SERVER_URL;
process.env.VITE_PUBLIC = VITE_DEV_SERVER_URL ? path.join(process.env.APP_ROOT, "public") : RENDERER_DIST;
let win = null;
const windowLabWindows = /* @__PURE__ */ new Map();
class DirectAddonAdapter {
  constructor(addon) {
    __publicField(this, "listeners", /* @__PURE__ */ new Set());
    this.addon = addon;
  }
  onRestarted(cb) {
    this.listeners.add(cb);
    return () => this.listeners.delete(cb);
  }
  invoke(method, ...args) {
    return new Promise((resolve, reject) => {
      try {
        const fn = this.addon[method];
        if (typeof fn !== "function") {
          reject({ message: `Method ${method} not found on addon` });
          return;
        }
        const result = fn.apply(this.addon, args);
        resolve(result);
      } catch (e) {
        reject({ message: (e == null ? void 0 : e.message) ?? String(e) });
      }
    });
  }
  invokeWithProgress(method, onProgress, ...args) {
    return new Promise((resolve, reject) => {
      try {
        const fn = this.addon[method];
        if (typeof fn !== "function") {
          reject({ message: `Method ${method} not found on addon` });
          return;
        }
        const result = fn.apply(this.addon, [...args, onProgress]);
        resolve(result);
      } catch (e) {
        reject({ message: (e == null ? void 0 : e.message) ?? String(e) });
      }
    });
  }
  dispose() {
  }
}
function loadConsoleDllAddon() {
  const require2 = createRequire(import.meta.url);
  let printedProbeError = false;
  const addonPath = (() => {
    if (!IS_DEV) {
      return path.join(
        process.resourcesPath,
        "native",
        "console_dll_addon.node"
      );
    }
    const baseRoots = [
      process.env.APP_ROOT,
      path.join(__dirname$1, ".."),
      (() => {
        try {
          return app.getAppPath();
        } catch {
          return void 0;
        }
      })(),
      process.cwd()
    ].filter(Boolean);
    const uniqueRoots = Array.from(new Set(baseRoots));
    const candidates = uniqueRoots.flatMap((root) => [
      path.join(
        root,
        "native",
        "console-dll",
        "build",
        "Release",
        "console_dll_addon.node"
      ),
      path.join(
        root,
        "native",
        "console-dll",
        "build",
        "Debug",
        "console_dll_addon.node"
      )
    ]);
    const found = candidates.find((candidate) => {
      try {
        fs.accessSync(candidate, fs.constants.R_OK);
        return true;
      } catch (e) {
        if (!printedProbeError) {
          printedProbeError = true;
          console.warn("[viz] Addon probe failed:", {
            candidate,
            code: e == null ? void 0 : e.code,
            error: (e == null ? void 0 : e.message) ?? String(e)
          });
        }
        return false;
      }
    });
    if (!found) {
      console.warn("[viz] Addon not found. Roots:", uniqueRoots);
      console.warn("[viz] Tried:", candidates);
      return null;
    }
    return found;
  })();
  if (!addonPath) return null;
  console.log("[viz] Addon path:", addonPath);
  try {
    const dllDir = path.dirname(addonPath);
    const currentPath = process.env.PATH || "";
    if (!currentPath.split(";").includes(dllDir)) {
      process.env.PATH = `${dllDir};${currentPath}`;
    }
  } catch {
  }
  try {
    return require2(addonPath);
  } catch (e) {
    console.warn("[viz] Failed to load addon:", addonPath, e);
    return null;
  }
}
const managedSeries = /* @__PURE__ */ new Map();
let rendererRestartInFlight = false;
function registerDicomHandlers(nativeHost) {
  ipcMain.removeHandler("dicom:select-folder");
  ipcMain.removeHandler("dicom:scan-folder");
  ipcMain.removeHandler("dicom:get-all-series");
  ipcMain.removeHandler("dicom:remove-series");
  ipcMain.removeHandler("dicom:cleanup");
  ipcMain.removeHandler("app:select-directories");
  ipcMain.removeHandler("app:select-his4d-file");
  ipcMain.removeHandler("app:save-his4d-file");
  ipcMain.removeHandler("app:save-stl-file");
  ipcMain.removeHandler("app:restart-renderer");
  ipcMain.handle("app:restart-renderer", async () => {
    try {
      if (rendererRestartInFlight) {
        return {
          success: false,
          error: "Renderer restart already in progress"
        };
      }
      rendererRestartInFlight = true;
      try {
        if (nativeHost) {
          await nativeHost.invoke("stopRenderLoop").catch(() => {
          });
          await nativeHost.invoke("hideAllWindows").catch(() => {
          });
          await nativeHost.invoke("destroyAllWindows").catch(() => {
          });
        }
      } catch {
      }
      if (win && !win.isDestroyed()) {
        win.reload();
        setTimeout(() => {
          rendererRestartInFlight = false;
        }, 1500);
        return { success: true };
      }
      rendererRestartInFlight = false;
      return { success: false, error: "Main window not available" };
    } catch (e) {
      rendererRestartInFlight = false;
      return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
    }
  });
  ipcMain.handle("dicom:select-folder", async () => {
    const result = await dialog.showOpenDialog({
      properties: ["openDirectory"]
    });
    if (result.canceled) return null;
    return result.filePaths[0] ?? null;
  });
  ipcMain.handle("app:select-directories", async () => {
    const result = await dialog.showOpenDialog({
      properties: ["openDirectory", "multiSelections"]
    });
    if (result.canceled) return null;
    return result.filePaths ?? [];
  });
  ipcMain.handle("app:select-his4d-file", async () => {
    var _a;
    const result = await dialog.showOpenDialog({
      properties: ["openFile"],
      filters: [{ name: "HIS4D", extensions: ["his4d"] }]
    });
    if (result.canceled) return null;
    return ((_a = result.filePaths) == null ? void 0 : _a[0]) ?? null;
  });
  ipcMain.handle("app:save-his4d-file", async () => {
    const result = await dialog.showSaveDialog({
      filters: [{ name: "HIS4D", extensions: ["his4d"] }],
      defaultPath: "cine.his4d"
    });
    if (result.canceled) return null;
    return result.filePath ?? null;
  });
  ipcMain.handle("app:save-stl-file", async (_event, defaultName) => {
    const result = await dialog.showSaveDialog({
      filters: [{ name: "STL", extensions: ["stl"] }],
      defaultPath: defaultName && defaultName.trim() ? defaultName : "mask.stl"
    });
    if (result.canceled) return null;
    return result.filePath ?? null;
  });
  ipcMain.handle("app:open-apr-test-window", async () => {
    try {
      createAprTestWindow();
      return { success: true };
    } catch (e) {
      return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
    }
  });
  ipcMain.handle("dicom:scan-folder", async (_event, folderPath) => {
    var _a;
    try {
      if (!nativeHost) {
        return { success: false, error: "DICOM API not available" };
      }
      const info = await nativeHost.invoke("loadDicomFolderInfo", folderPath);
      const fileCount = Number((info == null ? void 0 : info.fileCount) ?? 0);
      const width = Number((info == null ? void 0 : info.width) ?? 0);
      const height = Number((info == null ? void 0 : info.height) ?? 0);
      const tags = (info == null ? void 0 : info.tags) ?? {};
      const validation = (info == null ? void 0 : info.validation) ?? void 0;
      const windowWidth = typeof tags.windowWidth === "number" ? Number(tags.windowWidth) : void 0;
      const windowLevel = typeof tags.windowCenter === "number" ? Number(tags.windowCenter) : void 0;
      const thumbnailBase64 = ((_a = tryToBuffer(info == null ? void 0 : info.thumbnail)) == null ? void 0 : _a.toString("base64")) ?? "";
      const seriesId = makeId("series");
      const seriesInfo = {
        id: seriesId,
        folderPath,
        seriesName: path.basename(folderPath),
        fileCount,
        width,
        height,
        thumbnail: thumbnailBase64,
        patientName: tags.patientName,
        patientID: tags.patientID,
        studyDate: tags.studyDate,
        modality: tags.modality,
        seriesDescription: tags.seriesDescription,
        windowWidth,
        windowLevel
      };
      managedSeries.set(seriesId, seriesInfo);
      return {
        success: true,
        series: seriesInfo,
        validation
      };
    } catch (error) {
      console.error("[dicom:scan-folder] Error:", error);
      return { success: false, error: (error == null ? void 0 : error.message) ?? String(error) };
    }
  });
  ipcMain.handle("dicom:get-all-series", async () => {
    return Array.from(managedSeries.values());
  });
  ipcMain.handle("dicom:remove-series", async (_event, seriesId) => {
    managedSeries.delete(seriesId);
    return { success: true };
  });
  ipcMain.handle("dicom:cleanup", async () => {
    managedSeries.clear();
    return { success: true };
  });
}
function loadOpenGLAddon() {
  const require2 = createRequire(import.meta.url);
  const appRoot = process.env.APP_ROOT ?? process.cwd();
  const addonPath = IS_DEV ? path.join(
    appRoot,
    "native",
    "opengl-child",
    "build",
    "Release",
    "opengl_child.node"
  ) : path.join(process.resourcesPath, "native", "opengl_child.node");
  try {
    return require2(addonPath);
  } catch (e) {
    console.warn("[ogl] Failed to load addon:", addonPath, e);
    return null;
  }
}
function createMainWindow() {
  win = new BrowserWindow({
    width: 1600,
    height: 1e3,
    frame: false,
    autoHideMenuBar: true,
    webPreferences: {
      preload: path.join(MAIN_DIST, "preload.mjs"),
      contextIsolation: true,
      nodeIntegration: false
    }
  });
  win.setMenuBarVisibility(false);
  if (IS_DEV && VITE_DEV_SERVER_URL) {
    win.loadURL(VITE_DEV_SERVER_URL);
    win.webContents.openDevTools({ mode: "detach" });
  } else {
    win.loadFile(path.join(RENDERER_DIST, "index.html"));
  }
  win.on("closed", () => {
    win = null;
  });
  return win;
}
function createAprTestWindow() {
  const w = new BrowserWindow({
    width: 1100,
    height: 760,
    autoHideMenuBar: true,
    title: "APR Test",
    webPreferences: {
      preload: path.join(MAIN_DIST, "preload.mjs"),
      contextIsolation: true,
      nodeIntegration: false
    }
  });
  const html = `<!doctype html>
  <html lang="zh-CN">
    <head>
      <meta charset="utf-8" />
      <meta name="viewport" content="width=device-width, initial-scale=1" />
      <title>APR Test</title>
      <style>
        html,body{height:100%;margin:0;}
        body{font-family:system-ui,-apple-system,Segoe UI,Roboto,Arial,"Microsoft YaHei";}
        #bar{height:44px;display:flex;gap:8px;align-items:center;padding:0 10px;border-bottom:1px solid rgba(255,255,255,0.08);background:#0e141c;}
        #bar button{height:30px;padding:0 10px;border-radius:6px;border:1px solid rgba(255,255,255,0.14);background:rgba(255,255,255,0.06);color:#e6eef7;cursor:pointer;}
        #bar button:disabled{opacity:0.45;cursor:default;}
        #bar .path{flex:1;min-width:120px;opacity:0.9;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;}
        #bar .status{min-width:240px;opacity:0.85;font-size:12px;}
        #viewport{position:absolute;left:0;right:0;top:44px;bottom:0;background:#101820;}
        #hint{position:absolute;left:12px;top:56px;right:12px;pointer-events:none;opacity:0.75;font-size:12px;}
        a{color:#9fd1ff;}
      </style>
    </head>
    <body>
      <div id="bar">
        <button id="pick">选择DICOM文件夹</button>
        <div id="path" class="path">未选择</div>
        <button id="create" disabled>createAPR</button>
        <button id="embed" disabled>embedAxial</button>
        <button id="show" disabled>showAll</button>
        <button id="raise" disabled>raiseAll</button>
        <button id="refresh" disabled>refreshZ</button>
        <button id="start" disabled>startLoop</button>
        <button id="stop" disabled>stopLoop</button>
        <div id="status" class="status">ready</div>
      </div>
      <div id="viewport"></div>
      <div id="hint">说明：这个窗口只用于测试 APR 嵌入 HWND 在失焦/切回时是否被遮挡。你可以 Alt+Tab 让它失焦，看 viewport 里的窗口是否消失。</div>
      <script>
        const $ = (id) => document.getElementById(id);
        const statusEl = $('status');
        const pathEl = $('path');
        const viewport = $('viewport');

        let folderPath = null;
        let sessionId = 'aprtest-' + Math.random().toString(16).slice(2, 8);
        let windowIdAxial = null;

        function setStatus(msg){ statusEl.textContent = msg; }

        async function embedAxial(){
          if (!windowIdAxial) return;
          const r = viewport.getBoundingClientRect();
          const x = Math.round(r.left);
          const y = Math.round(r.top);
          const w = Math.max(1, Math.round(r.width));
          const h = Math.max(1, Math.round(r.height));
          await window.visualizationApi.embedWindow(windowIdAxial, x, y, w, h);
        }

        const ro = new ResizeObserver(() => {
          // keep embedded rect in sync
          if (!windowIdAxial) return;
          embedAxial().catch(()=>{});
        });
        ro.observe(viewport);

        window.addEventListener('focus', () => {
          if (!windowIdAxial) return;
          window.visualizationApi.raiseAllWindows?.();
          window.visualizationApi.refreshAllWindowsZOrder?.();
          setStatus('focus -> raise+refresh');
        });
        window.addEventListener('blur', () => {
          if (!windowIdAxial) return;
          if (document.hidden) { setStatus('blur(hidden)'); return; }
          window.visualizationApi.raiseAllWindows?.();
          window.visualizationApi.refreshAllWindowsZOrder?.();
          setStatus('blur -> raise+refresh');
        });

        $('pick').onclick = async () => {
          folderPath = await window.dicomApi?.selectFolder?.();
          pathEl.textContent = folderPath || '未选择';
          $('create').disabled = !folderPath;
          setStatus(folderPath ? 'folder selected' : 'cancelled');
        };

        $('create').onclick = async () => {
          if (!folderPath) return;
          setStatus('creating APR...');
          const res = await window.visualizationApi.createAPR(sessionId, folderPath);
          if (!res || res.success === false) {
            setStatus('createAPR failed');
            console.error(res);
            return;
          }
          windowIdAxial = res.windowIdAxial || (sessionId + '_axial');
          $('embed').disabled = false;
          $('show').disabled = false;
          $('raise').disabled = false;
          $('refresh').disabled = false;
          $('start').disabled = false;
          $('stop').disabled = false;
          setStatus('APR created: ' + windowIdAxial);
        };

        $('embed').onclick = async () => {
          if (!windowIdAxial) return;
          await embedAxial();
          setStatus('embedded');
        };
        $('show').onclick = async () => {
          await window.visualizationApi.showAllWindows?.();
          setStatus('showAllWindows');
        };
        $('raise').onclick = async () => {
          await window.visualizationApi.raiseAllWindows?.();
          setStatus('raiseAllWindows');
        };
        $('refresh').onclick = async () => {
          await window.visualizationApi.refreshAllWindowsZOrder?.();
          setStatus('refreshAllWindowsZOrder');
        };
        $('start').onclick = async () => {
          await window.visualizationApi.startRenderLoop?.(60);
          setStatus('startRenderLoop(60)');
        };
        $('stop').onclick = async () => {
          await window.visualizationApi.stopRenderLoop?.();
          setStatus('stopRenderLoop');
        };
      <\/script>
    </body>
  </html>`;
  w.loadURL(`data:text/html;charset=utf-8,${encodeURIComponent(html)}`);
  w.setMenuBarVisibility(false);
  return w;
}
function makeId(prefix) {
  return `${prefix}-${Date.now().toString(36)}-${Math.random().toString(16).slice(2, 8)}`;
}
function tryToBuffer(value) {
  try {
    if (!value) return null;
    if (Buffer.isBuffer(value)) return value;
    if (ArrayBuffer.isView(value)) {
      const view = value;
      return Buffer.from(
        new Uint8Array(view.buffer, view.byteOffset, view.byteLength)
      );
    }
    if (value instanceof ArrayBuffer) {
      return Buffer.from(new Uint8Array(value));
    }
    const anyVal = value;
    if (anyVal && typeof anyVal === "object" && Array.isArray(anyVal.data) && (anyVal.type === "Buffer" || anyVal.type === "Uint8Array")) {
      return Buffer.from(anyVal.data);
    }
    if (anyVal && typeof anyVal === "object" && typeof anyVal.length === "number") {
      try {
        return Buffer.from(anyVal);
      } catch {
        return null;
      }
    }
    return null;
  } catch {
    return null;
  }
}
function tryParseHwnd(value) {
  try {
    if (typeof value === "bigint") return value;
    if (typeof value === "number") {
      if (!Number.isFinite(value)) return null;
      return BigInt.asUintN(64, BigInt(Math.trunc(value)));
    }
    if (typeof value === "string") {
      const s = value.trim();
      if (!s) return null;
      return BigInt(s);
    }
    const buf = tryToBuffer(value);
    if (buf) {
      if (buf.length >= 8) return buf.readBigUInt64LE(0);
      if (buf.length >= 4) return BigInt(buf.readUInt32LE(0));
      return null;
    }
    return null;
  } catch {
    return null;
  }
}
function tryParseHwndFromPayload(payload) {
  return tryParseHwnd(payload == null ? void 0 : payload.hwnd);
}
function loadRendererForWindow(target, opts) {
  if (IS_DEV && VITE_DEV_SERVER_URL) {
    const url = new URL(VITE_DEV_SERVER_URL);
    if (opts == null ? void 0 : opts.tab) url.searchParams.set("tab", opts.tab);
    url.searchParams.set("autocreate", "1");
    target.loadURL(url.toString());
    return;
  }
  const searchParams = new URLSearchParams();
  if (opts == null ? void 0 : opts.tab) searchParams.set("tab", opts.tab);
  searchParams.set("autocreate", "1");
  const search = searchParams.toString();
  target.loadFile(path.join(RENDERER_DIST, "index.html"), {
    search: search ? `?${search}` : void 0
  });
}
function createWindowLabWindow(type) {
  const id = makeId(type);
  const title = type === "opengl" ? `OpenGL Window (${id})` : `Normal Window (${id})`;
  const w = new BrowserWindow({
    width: type === "opengl" ? 1e3 : 800,
    height: type === "opengl" ? 700 : 600,
    title,
    frame: false,
    autoHideMenuBar: true,
    webPreferences: {
      preload: path.join(MAIN_DIST, "preload.mjs"),
      contextIsolation: true,
      nodeIntegration: false
    }
  });
  w.setMenuBarVisibility(false);
  if (type === "opengl") {
    loadRendererForWindow(w, { tab: "openglTest" });
  } else {
    const html = `<!doctype html>
<html><head><meta charset="utf-8" />
<title>${title}</title>
<style>
  body{font-family:system-ui,Segoe UI,Arial;margin:0}
  .drag{height:36px;display:flex;align-items:center;justify-content:space-between;padding:0 12px;background:#0b2233;color:#d1f6ff;-webkit-app-region:drag;user-select:none}
  .btn{height:28px;margin-left:8px;-webkit-app-region:no-drag;cursor:pointer}
  .content{padding:16px}
  code{background:#eee;padding:2px 4px;border-radius:4px}
</style>
</head>
<body>
  <div class="drag">
    <div>${title}</div>
    <div>
      <button class="btn" onclick="window.close()">Close</button>
    </div>
  </div>
  <div class="content">
    <p>This is a plain Electron <code>BrowserWindow</code> for window action testing.</p>
    <p>Try: move / resize / minimize / maximize / hide / show.</p>
  </div>
</body></html>`;
    w.loadURL(`data:text/html;charset=utf-8,${encodeURIComponent(html)}`);
  }
  const info = { id, type, title };
  windowLabWindows.set(id, { info, win: w });
  w.on("closed", () => {
    windowLabWindows.delete(id);
  });
  return info;
}
function registerWindowControlHandlers() {
  ipcMain.handle("window-controls:minimize", (event) => {
    const w = BrowserWindow.fromWebContents(event.sender) ?? win;
    w == null ? void 0 : w.minimize();
  });
  ipcMain.handle("window-controls:toggle-maximize", (event) => {
    const w = BrowserWindow.fromWebContents(event.sender) ?? win;
    if (!w) return;
    if (w.isMaximized()) w.restore();
    else w.maximize();
  });
  ipcMain.handle("window-controls:close", (event) => {
    const w = BrowserWindow.fromWebContents(event.sender) ?? win;
    w == null ? void 0 : w.close();
  });
}
function registerWindowLabHandlers() {
  ipcMain.handle(
    "winlab:create",
    (_event, payload) => {
      try {
        const info = createWindowLabWindow(payload.type);
        return { success: true, window: info };
      } catch (e) {
        return { success: false, error: (e == null ? void 0 : e.message) ?? "winlab:create failed" };
      }
    }
  );
  ipcMain.handle("winlab:list", () => {
    const list = Array.from(windowLabWindows.values()).map(({ info, win: win2 }) => {
      const bounds = win2.getBounds();
      return {
        ...info,
        bounds,
        isVisible: win2.isVisible(),
        isMinimized: win2.isMinimized(),
        isMaximized: win2.isMaximized(),
        isDestroyed: win2.isDestroyed()
      };
    });
    return { success: true, windows: list };
  });
  ipcMain.handle(
    "winlab:action",
    (_event, payload) => {
      const entry = windowLabWindows.get(payload.id);
      if (!entry) return { success: false, error: "window not found" };
      const w = entry.win;
      try {
        switch (payload.action) {
          case "show":
            w.show();
            break;
          case "hide":
            w.hide();
            break;
          case "minimize":
            w.minimize();
            break;
          case "maximize":
            w.maximize();
            break;
          case "restore":
            w.restore();
            break;
          case "focus":
            w.focus();
            break;
          case "center":
            w.center();
            break;
          case "setBounds":
            if (!payload.bounds)
              return { success: false, error: "missing bounds" };
            w.setBounds(payload.bounds);
            break;
          case "close":
            w.close();
            break;
          default:
            return { success: false, error: "unknown action" };
        }
        return { success: true };
      } catch (e) {
        return { success: false, error: (e == null ? void 0 : e.message) ?? "winlab:action failed" };
      }
    }
  );
}
function registerOpenGLHandlers(addon) {
  const hwndsByWebContentsId = /* @__PURE__ */ new Map();
  function reassertVisibleForWebContentsId(webContentsId) {
    if (!addon) return;
    const set = hwndsByWebContentsId.get(webContentsId);
    if (!set || set.size === 0) return;
    for (const hwnd of set) {
      try {
        addon.setVisible(hwnd, true);
      } catch {
      }
    }
  }
  function getContentOriginDip(sender) {
    const bw = BrowserWindow.fromWebContents(sender);
    if (!bw) return null;
    const cb = bw.getContentBounds();
    return { x: cb.x, y: cb.y };
  }
  function destroyForWebContentsId(webContentsId) {
    const set = hwndsByWebContentsId.get(webContentsId);
    if (!set || set.size === 0) return;
    hwndsByWebContentsId.delete(webContentsId);
    for (const hwnd of set) {
      try {
        addon == null ? void 0 : addon.destroy(hwnd);
      } catch (e) {
        console.warn("[ogl] destroy failed", e);
      }
    }
  }
  app.on("web-contents-created", (_e, contents) => {
    contents.on("destroyed", () => destroyForWebContentsId(contents.id));
  });
  app.on("browser-window-focus", (_e, bw) => {
    try {
      if (bw.isMinimized()) return;
      reassertVisibleForWebContentsId(bw.webContents.id);
    } catch {
    }
  });
  app.on("browser-window-blur", (_e, bw) => {
    try {
      if (bw.isMinimized()) return;
      reassertVisibleForWebContentsId(bw.webContents.id);
    } catch {
    }
  });
  ipcMain.handle(
    "ogl:create",
    (event, payload) => {
      var _a, _b;
      if (!addon) return { success: false, error: "OpenGL addon not loaded" };
      const bw = BrowserWindow.fromWebContents(event.sender);
      if (!bw) return { success: false, error: "BrowserWindow not found" };
      const parent = ((_b = (_a = event.sender).getNativeWindowHandle) == null ? void 0 : _b.call(_a)) ?? bw.getNativeWindowHandle();
      const hwnd = addon.createChildWindow(
        parent,
        payload.x,
        payload.y,
        payload.width,
        payload.height,
        !!payload.transparentInput
      );
      const set = hwndsByWebContentsId.get(event.sender.id) ?? /* @__PURE__ */ new Set();
      set.add(hwnd);
      hwndsByWebContentsId.set(event.sender.id, set);
      return { success: true, hwnd: hwnd.toString() };
    }
  );
  ipcMain.handle(
    "ogl:create-client",
    (event, payload) => {
      var _a, _b;
      if (!addon) return { success: false, error: "OpenGL addon not loaded" };
      const bw = BrowserWindow.fromWebContents(event.sender);
      if (!bw) return { success: false, error: "BrowserWindow not found" };
      const origin = getContentOriginDip(event.sender);
      if (!origin) return { success: false, error: "content origin not found" };
      const parent = ((_b = (_a = event.sender).getNativeWindowHandle) == null ? void 0 : _b.call(_a)) ?? bw.getNativeWindowHandle();
      const x = Math.round(origin.x + payload.left);
      const y = Math.round(origin.y + payload.top);
      const w = Math.max(1, Math.round(payload.width));
      const h = Math.max(1, Math.round(payload.height));
      const hwnd = addon.createChildWindow(
        parent,
        x,
        y,
        w,
        h,
        !!payload.transparentInput
      );
      const set = hwndsByWebContentsId.get(event.sender.id) ?? /* @__PURE__ */ new Set();
      set.add(hwnd);
      hwndsByWebContentsId.set(event.sender.id, set);
      return { success: true, hwnd: hwnd.toString() };
    }
  );
  ipcMain.handle("ogl:status", () => {
    return {
      success: true,
      addonLoaded: !!addon,
      note: !!addon ? "addon loaded" : "addon not loaded (check build:native:ogl and path)"
    };
  });
  ipcMain.handle(
    "ogl:standalone-open",
    (event, payload) => {
      if (!addon) return { success: false, error: "OpenGL addon not loaded" };
      const w = Math.max(100, Math.floor((payload == null ? void 0 : payload.width) ?? 900));
      const h = Math.max(100, Math.floor((payload == null ? void 0 : payload.height) ?? 650));
      const hwnd = addon.createStandaloneWindow(w, h);
      const set = hwndsByWebContentsId.get(event.sender.id) ?? /* @__PURE__ */ new Set();
      set.add(hwnd);
      hwndsByWebContentsId.set(event.sender.id, set);
      return { success: true, hwnd: hwnd.toString() };
    }
  );
  ipcMain.handle(
    "ogl:set-rect",
    (_event, payload) => {
      if (!addon) return { success: false, error: "OpenGL addon not loaded" };
      const hwnd = tryParseHwndFromPayload(payload);
      if (!hwnd) return { success: false, error: "invalid hwnd" };
      const ok = addon.setRect(
        hwnd,
        payload.x,
        payload.y,
        payload.width,
        payload.height
      );
      return { success: !!ok };
    }
  );
  ipcMain.handle(
    "ogl:set-rect-client",
    (event, payload) => {
      if (!addon) return { success: false, error: "OpenGL addon not loaded" };
      const origin = getContentOriginDip(event.sender);
      if (!origin) return { success: false, error: "content origin not found" };
      const hwnd = tryParseHwndFromPayload(payload);
      if (!hwnd) return { success: false, error: "invalid hwnd" };
      const x = Math.round(origin.x + payload.left);
      const y = Math.round(origin.y + payload.top);
      const w = Math.max(1, Math.round(payload.width));
      const h = Math.max(1, Math.round(payload.height));
      const ok = addon.setRect(hwnd, x, y, w, h);
      return { success: !!ok };
    }
  );
  ipcMain.handle(
    "ogl:set-transparent-input",
    (_event, payload) => {
      if (!addon) return { success: false, error: "OpenGL addon not loaded" };
      const hwnd = tryParseHwndFromPayload(payload);
      if (!hwnd) return { success: false, error: "invalid hwnd" };
      const ok = addon.setTransparentInput(hwnd, !!payload.enabled);
      return { success: !!ok };
    }
  );
  ipcMain.handle(
    "ogl:set-visible",
    (_event, payload) => {
      if (!addon) return { success: false, error: "OpenGL addon not loaded" };
      const hwnd = tryParseHwndFromPayload(payload);
      if (!hwnd) return { success: false, error: "invalid hwnd" };
      const ok = addon.setVisible(hwnd, !!payload.visible);
      return { success: !!ok };
    }
  );
  ipcMain.handle("ogl:get-window-info", (_event, payload) => {
    var _a;
    if (!addon) return { success: false, error: "OpenGL addon not loaded" };
    const hwnd = tryParseHwndFromPayload(payload);
    if (!hwnd) return { success: false, error: "invalid hwnd" };
    try {
      const info = (_a = addon.getWindowInfo) == null ? void 0 : _a.call(addon, hwnd);
      return { success: true, info };
    } catch (e) {
      return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
    }
  });
  ipcMain.handle("ogl:destroy", (event, payload) => {
    if (!addon) return { success: false, error: "OpenGL addon not loaded" };
    const hwnd = tryParseHwndFromPayload(payload);
    if (!hwnd) return { success: false, error: "invalid hwnd" };
    addon.destroy(hwnd);
    const set = hwndsByWebContentsId.get(event.sender.id);
    if (set) {
      set.delete(hwnd);
      if (set.size === 0) hwndsByWebContentsId.delete(event.sender.id);
    }
    return { success: true };
  });
}
function registerPopupHandlers() {
  const popups = /* @__PURE__ */ new Map();
  const windows = /* @__PURE__ */ new Map();
  ipcMain.handle(
    "browserwindow:open",
    (event, payload) => {
      const id = makeId("win");
      const title = (payload == null ? void 0 : payload.title) ?? `Window (${id})`;
      const showInactive = (payload == null ? void 0 : payload.showInactive) ?? true;
      const modal = (payload == null ? void 0 : payload.modal) ?? false;
      const parent = BrowserWindow.fromWebContents(event.sender) ?? void 0;
      const w = new BrowserWindow({
        width: (payload == null ? void 0 : payload.width) ?? 720,
        height: (payload == null ? void 0 : payload.height) ?? 520,
        title,
        // Keep the same frameless style as existing popups (custom titlebar).
        frame: false,
        autoHideMenuBar: true,
        // Treat as a normal window (shows in taskbar) unless caller explicitly wants modal.
        skipTaskbar: false,
        modal,
        parent: modal ? parent : void 0,
        show: false,
        webPreferences: {
          preload: path.join(MAIN_DIST, "preload.mjs"),
          contextIsolation: true,
          nodeIntegration: false
        }
      });
      w.setMenuBarVisibility(false);
      const html = `<!doctype html>
<html><head><meta charset="utf-8" />
<title>${title}</title>
<style>
  body{margin:0;font-family:system-ui,Segoe UI,Arial;background:linear-gradient(180deg,#071a29 0%,#040d1a 100%);color:#dbf6ff}
  .titlebar{width:100%;height:36px;display:flex;align-items:center;justify-content:space-between;padding:0;-webkit-app-region:drag;user-select:none;color:#d1f6ff}
  .title{font-size:1.02rem;color:#f2fdff;font-weight:600;letter-spacing:1px;padding-left:12px}
  .window-controls{display:flex;gap:2px;-webkit-app-region:no-drag;height:100%;padding-right:4px}
  .window-controls button{width:44px;height:100%;background:transparent;color:#c7f4ff;border:none;font-size:1rem;border-radius:4px;cursor:pointer;transition:background .2s;display:flex;align-items:center;justify-content:center;padding:0;appearance:none}
  .window-controls button:hover{background:rgba(86,209,243,.22)}
  .window-controls button:last-child:hover{background:#e53935;color:#fff}
  .content{padding:12px}
  #log{margin-top:10px;white-space:pre-wrap;font-size:12px;opacity:.9}
</style>
</head>
<body>
  <div class="titlebar" data-drag-region>
    <span class="title">${title}</span>
    <div class="window-controls">
      <button type="button" onclick="window.ipcRenderer?.invoke?.('window-controls:minimize')" title="最小化">&#x2015;</button>
      <button type="button" onclick="window.ipcRenderer?.invoke?.('window-controls:toggle-maximize')" title="最大化/还原">&#x25A1;</button>
      <button type="button" onclick="window.ipcRenderer?.invoke?.('window-controls:close')" title="关闭">&#x2715;</button>
    </div>
  </div>
  <div class="content">
    <div>BrowserWindow (non-popup) test window.</div>
    <div id="log"></div>
  </div>
  <script>
    const logEl = document.getElementById('log');
    function log(msg){ logEl.textContent = msg + "
" + (logEl.textContent || ""); }
    ['mousedown','mouseup','mousemove','wheel','click','dblclick','contextmenu'].forEach((t)=>{
      window.addEventListener(t,(e)=>{
        log(String(t) + ' x=' + e.clientX + ' y=' + e.clientY + ' button=' + e.button + ' deltaY=' + (e.deltaY || 0));
      }, {passive:false});
    });
  <\/script>
</body></html>`;
      w.loadURL(`data:text/html;charset=utf-8,${encodeURIComponent(html)}`);
      windows.set(id, w);
      w.on("closed", () => windows.delete(id));
      if (showInactive) {
        w.once("ready-to-show", () => {
          if (!w.isDestroyed()) w.showInactive();
        });
      } else {
        w.once("ready-to-show", () => {
          if (!w.isDestroyed()) w.show();
        });
      }
      return { success: true, id };
    }
  );
  ipcMain.handle(
    "popup:open",
    (event, payload) => {
      const parent = BrowserWindow.fromWebContents(event.sender) ?? void 0;
      const id = makeId("popup");
      const title = (payload == null ? void 0 : payload.title) ?? `Popup (${id})`;
      const alwaysOnTop = (payload == null ? void 0 : payload.alwaysOnTop) ?? true;
      const modal = (payload == null ? void 0 : payload.modal) ?? false;
      const w = new BrowserWindow({
        width: (payload == null ? void 0 : payload.width) ?? 520,
        height: (payload == null ? void 0 : payload.height) ?? 360,
        title,
        frame: false,
        autoHideMenuBar: true,
        skipTaskbar: true,
        modal,
        alwaysOnTop,
        parent,
        webPreferences: {
          preload: path.join(MAIN_DIST, "preload.mjs"),
          contextIsolation: true,
          nodeIntegration: false
        }
      });
      w.setMenuBarVisibility(false);
      if (alwaysOnTop) {
        w.setAlwaysOnTop(true, "pop-up-menu");
        w.moveTop();
      }
      const html = `<!doctype html>
<html><head><meta charset="utf-8" />
<title>${title}</title>
<style>
  body{margin:0;font-family:system-ui,Segoe UI,Arial;background:linear-gradient(180deg,#071a29 0%,#040d1a 100%);color:#dbf6ff}
  .titlebar{width:100%;height:36px;display:flex;align-items:center;justify-content:space-between;padding:0;-webkit-app-region:drag;user-select:none;color:#d1f6ff}
  .title{font-size:1.02rem;color:#f2fdff;font-weight:600;letter-spacing:1px;padding-left:12px}
  .window-controls{display:flex;gap:2px;-webkit-app-region:no-drag;height:100%;padding-right:4px}
  .window-controls button{width:44px;height:100%;background:transparent;color:#c7f4ff;border:none;font-size:1rem;border-radius:4px;cursor:pointer;transition:background .2s;display:flex;align-items:center;justify-content:center;padding:0;appearance:none}
  .window-controls button:hover{background:rgba(86,209,243,.22)}
  .window-controls button:last-child:hover{background:#e53935;color:#fff}
  .content{padding:12px}
  #log{margin-top:10px;white-space:pre-wrap;font-size:12px;opacity:.9}
</style>
</head>
<body>
  <div class="titlebar" data-drag-region>
    <span class="title">${title}</span>
    <div class="window-controls">
      <button type="button" onclick="window.ipcRenderer?.invoke?.('window-controls:minimize')" title="最小化">&#x2015;</button>
      <button type="button" onclick="window.ipcRenderer?.invoke?.('window-controls:toggle-maximize')" title="最大化/还原">&#x25A1;</button>
      <button type="button" onclick="window.ipcRenderer?.invoke?.('window-controls:close')" title="关闭">&#x2715;</button>
    </div>
  </div>
  <div class="content">
    <div>Click/drag here to test mouse focus & routing.</div>
    <div id="log"></div>
  </div>
  <script>
    const logEl = document.getElementById('log');
    function log(msg){ logEl.textContent = msg + "
" + (logEl.textContent || ""); }
    ['mousedown','mouseup','mousemove','wheel','click','dblclick','contextmenu'].forEach((t)=>{
      window.addEventListener(t,(e)=>{
        log(String(t) + ' x=' + e.clientX + ' y=' + e.clientY + ' button=' + e.button + ' deltaY=' + (e.deltaY || 0));
      }, {passive:false});
    });
  <\/script>
</body></html>`;
      w.loadURL(`data:text/html;charset=utf-8,${encodeURIComponent(html)}`);
      popups.set(id, w);
      w.on("closed", () => popups.delete(id));
      return { success: true, id };
    }
  );
}
function registerDialogHandlers() {
  const dialogContextsByWebContentsId = /* @__PURE__ */ new Map();
  const confirmContextsByWebContentsId = /* @__PURE__ */ new Map();
  ipcMain.removeHandler("dialog:open");
  ipcMain.removeHandler("dialog:confirm");
  ipcMain.removeAllListeners("dialog:close");
  ipcMain.removeAllListeners("dialog:ready");
  ipcMain.removeAllListeners("dialog-confirm:response");
  ipcMain.handle(
    "dialog:open",
    async (event, dialogType, params) => {
      const parentWebContentsId = event.sender.id;
      const dialogConfig = {
        rigmark: {
          title: "阈值分割",
          width: 650,
          height: 750,
          modal: true,
          htmlFile: "dialog-rigmark.html"
        },
        cropsettings: {
          title: "裁切设置",
          width: 650,
          height: 720,
          modal: false,
          htmlFile: "dialog-cropsettings.html"
        },
        morphology: {
          title: "形态学操作",
          width: 450,
          height: 500,
          modal: true,
          htmlFile: "dialog-morphology.html"
        },
        boolean: {
          title: "布尔操作",
          width: 500,
          height: 550,
          modal: true,
          htmlFile: "dialog-boolean.html"
        },
        config3d: {
          title: "添加3D掩膜",
          width: 450,
          height: 400,
          modal: true,
          htmlFile: "dialog-config3d.html"
        },
        stats: {
          title: "掩膜信息",
          width: 550,
          height: 600,
          modal: true,
          htmlFile: "dialog-stats.html"
        },
        colorpicker: {
          title: "选择颜色",
          width: 550,
          height: 500,
          modal: true,
          htmlFile: "dialog-colorpicker.html"
        },
        opacity: {
          title: "设置透明度",
          width: 450,
          height: 400,
          modal: true,
          htmlFile: "dialog-opacity.html"
        },
        roiedit: {
          title: "ROI编辑",
          width: 650,
          height: 500,
          modal: false,
          htmlFile: "dialog-roiedit.html"
        },
        savemask: {
          title: "保存Mask",
          width: 500,
          height: 400,
          modal: true,
          htmlFile: "dialog-savemask.html"
        },
        screenshot: {
          title: "选择截图视图",
          width: 550,
          height: 500,
          modal: true,
          htmlFile: "dialog-screenshot.html"
        },
        measurements: {
          title: "测量结果",
          width: 920,
          height: 620,
          modal: false,
          htmlFile: "dialog-measurements.html"
        },
        measurementChart: {
          title: "测量图表",
          width: 920,
          height: 620,
          modal: false,
          htmlFile: "dialog-measurement-chart.html"
        },
        lighting: {
          title: "光照",
          width: 520,
          height: 360,
          modal: true,
          htmlFile: "dialog-lighting.html"
        },
        transferfunction: {
          title: "传递函数",
          width: 760,
          height: 560,
          modal: true,
          htmlFile: "dialog-transferfunction.html"
        }
      };
      const config = dialogConfig[dialogType] ?? {
        title: "Dialog",
        width: 600,
        height: 600,
        modal: true,
        htmlFile: "dialog.html"
      };
      const parent = BrowserWindow.fromWebContents(event.sender) ?? win ?? void 0;
      const dialogWindow = new BrowserWindow({
        width: config.width,
        height: config.height,
        parent,
        modal: config.modal,
        show: false,
        frame: false,
        transparent: false,
        backgroundColor: "#ffffff",
        title: config.title,
        alwaysOnTop: true,
        resizable: false,
        minimizable: false,
        maximizable: false,
        webPreferences: {
          preload: path.join(MAIN_DIST, "preload.mjs"),
          nodeIntegration: false,
          contextIsolation: true,
          // Let preload detect this is a dialog window and apply consistent styling.
          additionalArguments: ["--hiscan-window=dialog"]
        }
      });
      const dialogWebContentsId = dialogWindow.webContents.id;
      const ctx = {
        dialogType,
        params,
        parentWebContentsId,
        initSent: false
      };
      dialogContextsByWebContentsId.set(dialogWebContentsId, ctx);
      try {
        if (IS_DEV && VITE_DEV_SERVER_URL) {
          const url = `${VITE_DEV_SERVER_URL}${config.htmlFile}`;
          await dialogWindow.loadURL(url);
          dialogWindow.webContents.once("did-finish-load", () => {
            const c = dialogContextsByWebContentsId.get(dialogWebContentsId);
            if (c && !c.initSent) {
              c.initSent = true;
              dialogWindow.webContents.send("dialog:init-data", c.params);
            }
          });
        } else {
          const filePath = path.join(RENDERER_DIST, config.htmlFile);
          await dialogWindow.loadFile(filePath);
          dialogWindow.webContents.once("did-finish-load", () => {
            const c = dialogContextsByWebContentsId.get(dialogWebContentsId);
            if (c && !c.initSent) {
              c.initSent = true;
              dialogWindow.webContents.send("dialog:init-data", c.params);
            }
          });
        }
        try {
          const parentWc = parent == null ? void 0 : parent.webContents;
          parentWc == null ? void 0 : parentWc.send("dialog:opened", {
            webContentsId: dialogWebContentsId,
            dialogType: ctx.dialogType
          });
        } catch {
        }
        dialogWindow.show();
        dialogWindow.focus();
      } catch (error) {
        if (dialogWindow && !dialogWindow.isDestroyed()) dialogWindow.close();
        dialogContextsByWebContentsId.delete(dialogWebContentsId);
        return { success: false, error: `Failed to load dialog: ${error}` };
      }
      dialogWindow.on("closed", () => {
        const c = dialogContextsByWebContentsId.get(dialogWebContentsId);
        dialogContextsByWebContentsId.delete(dialogWebContentsId);
        const parentWc = (parent == null ? void 0 : parent.webContents) ?? ((c == null ? void 0 : c.parentWebContentsId) != null ? webContents.fromId(c.parentWebContentsId) : null);
        parentWc == null ? void 0 : parentWc.send("dialog:closed", {
          webContentsId: dialogWebContentsId,
          dialogType: c == null ? void 0 : c.dialogType
        });
      });
      return { success: true, webContentsId: dialogWebContentsId };
    }
  );
  const escapeHtml = (s) => s.replaceAll("&", "&amp;").replaceAll("<", "&lt;").replaceAll(">", "&gt;").replaceAll('"', "&quot;").replaceAll("'", "&#39;");
  ipcMain.handle(
    "dialog:confirm",
    async (event, payload) => {
      const title = (payload == null ? void 0 : payload.title) ?? "确认";
      const message = (payload == null ? void 0 : payload.message) ?? "";
      const confirmButtonText = (payload == null ? void 0 : payload.confirmButtonText) ?? "确定";
      const cancelButtonText = (payload == null ? void 0 : payload.cancelButtonText) ?? "取消";
      const showInactive = (payload == null ? void 0 : payload.showInactive) ?? true;
      const parent = BrowserWindow.fromWebContents(event.sender) ?? win ?? void 0;
      const w = new BrowserWindow({
        width: (payload == null ? void 0 : payload.width) ?? 520,
        height: (payload == null ? void 0 : payload.height) ?? 240,
        parent,
        // Non-modal by default to avoid forced focus switching with OpenGL child HWND.
        modal: false,
        show: false,
        frame: false,
        transparent: false,
        backgroundColor: "#ffffff",
        title,
        resizable: false,
        minimizable: false,
        maximizable: false,
        autoHideMenuBar: true,
        webPreferences: {
          preload: path.join(MAIN_DIST, "preload.mjs"),
          nodeIntegration: false,
          contextIsolation: true,
          additionalArguments: ["--hiscan-window=dialog"]
        }
      });
      const wcId = w.webContents.id;
      const ok = await new Promise((resolve) => {
        confirmContextsByWebContentsId.set(wcId, { resolve, resolved: false });
        w.on("closed", () => {
          const ctx = confirmContextsByWebContentsId.get(wcId);
          confirmContextsByWebContentsId.delete(wcId);
          if (ctx && !ctx.resolved) {
            ctx.resolved = true;
            ctx.resolve(false);
          }
        });
      });
      if (w.isDestroyed()) return { success: true, ok: false };
      const messageHtml = escapeHtml(message).replaceAll("\n", "<br/>");
      const html = `<!doctype html>
<html><head><meta charset="utf-8" />
<title>${escapeHtml(title)}</title>
<style>
  body{margin:0;font-family:system-ui,Segoe UI,Arial;background:linear-gradient(180deg,#071a29 0%,#040d1a 100%);color:#dbf6ff}
  .titlebar{width:100%;height:36px;display:flex;align-items:center;justify-content:space-between;padding:0;-webkit-app-region:drag;user-select:none;color:#d1f6ff}
  .title{font-size:1.02rem;color:#f2fdff;font-weight:600;letter-spacing:1px;padding-left:12px}
  .window-controls{display:flex;gap:2px;-webkit-app-region:no-drag;height:100%;padding-right:4px}
  .window-controls button{width:44px;height:100%;background:transparent;color:#c7f4ff;border:none;font-size:1rem;border-radius:4px;cursor:pointer;transition:background .2s;display:flex;align-items:center;justify-content:center;padding:0;appearance:none}
  .window-controls button:hover{background:rgba(86,209,243,.22)}
  .window-controls button:last-child:hover{background:#e53935;color:#fff}
  .content{padding:14px 14px 10px 14px}
  .msg{font-size:14px;line-height:1.5;white-space:normal;word-break:break-word}
  .btns{display:flex;gap:10px;justify-content:flex-end;padding:10px 14px 14px 14px}
  .btn{min-width:96px;height:32px;border-radius:6px;border:1px solid rgba(199,244,255,.35);background:rgba(255,255,255,.06);color:#dbf6ff;cursor:pointer}
  .btn:hover{background:rgba(255,255,255,.10)}
  .btn.primary{border-color:rgba(86,209,243,.55);background:rgba(86,209,243,.20)}
  .btn.primary:hover{background:rgba(86,209,243,.28)}
</style>
</head>
<body>
  <div class="titlebar" data-drag-region>
    <span class="title">${escapeHtml(title)}</span>
    <div class="window-controls">
      <button type="button" onclick="window.ipcRenderer?.invoke?.('window-controls:close')" title="关闭">&#x2715;</button>
    </div>
  </div>
  <div class="content">
    <div class="msg">${messageHtml}</div>
  </div>
  <div class="btns">
    <button class="btn" type="button" onclick="window.ipcRenderer?.send?.('dialog-confirm:response', { ok: false })">${escapeHtml(
        cancelButtonText
      )}</button>
    <button class="btn primary" type="button" onclick="window.ipcRenderer?.send?.('dialog-confirm:response', { ok: true })">${escapeHtml(
        confirmButtonText
      )}</button>
  </div>
</body></html>`;
      w.loadURL(`data:text/html;charset=utf-8,${encodeURIComponent(html)}`);
      w.once("ready-to-show", () => {
        if (w.isDestroyed()) return;
        if (showInactive) w.showInactive();
        else w.show();
      });
      return { success: true, ok };
    }
  );
  ipcMain.on("dialog-confirm:response", (event, payload) => {
    const wcId = event.sender.id;
    const ctx = confirmContextsByWebContentsId.get(wcId);
    if (!ctx || ctx.resolved) return;
    ctx.resolved = true;
    confirmContextsByWebContentsId.delete(wcId);
    ctx.resolve(!!(payload == null ? void 0 : payload.ok));
    const dialogWin = BrowserWindow.fromWebContents(event.sender);
    if (dialogWin && !dialogWin.isDestroyed()) dialogWin.close();
  });
  ipcMain.on("dialog:ready", (event) => {
    const c = dialogContextsByWebContentsId.get(event.sender.id);
    if (!c) return;
    if (c.initSent) return;
    c.initSent = true;
    event.sender.send("dialog:init-data", c.params);
  });
  ipcMain.on("dialog:close", (event, result) => {
    var _a;
    const dialogWin = BrowserWindow.fromWebContents(event.sender);
    if (!dialogWin || dialogWin.isDestroyed()) return;
    const c = dialogContextsByWebContentsId.get(event.sender.id);
    const parentWc = (c == null ? void 0 : c.parentWebContentsId) != null ? webContents.fromId(c.parentWebContentsId) : null;
    const taggedResult = {
      ...result,
      dialogType: c == null ? void 0 : c.dialogType,
      sessionId: (_a = c == null ? void 0 : c.params) == null ? void 0 : _a.sessionId
    };
    parentWc == null ? void 0 : parentWc.send("dialog:result", taggedResult);
    dialogWin.close();
  });
}
function registerVisualizationHandlers(nativeHost) {
  ipcMain.handle(
    "viz:create-apr",
    async (event, sessionId, folderPath) => {
      try {
        if (!nativeHost) {
          return { success: false, error: "Visualization API not available" };
        }
        const progressCallback = (progress, message) => {
          event.sender.send("viz:progress", { progress, message });
        };
        return nativeHost.invokeWithProgress(
          "createAPRViews",
          progressCallback,
          sessionId,
          folderPath
        );
      } catch (e) {
        console.error("[viz:create-apr] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:create-mpr",
    async (event, sessionId, folderPath) => {
      try {
        if (!nativeHost) {
          return { success: false, error: "Visualization API not available" };
        }
        const progressCallback = (progress, message) => {
          event.sender.send("viz:progress", { progress, message });
        };
        return nativeHost.invokeWithProgress(
          "createMPRViews",
          progressCallback,
          sessionId,
          folderPath
        );
      } catch (e) {
        console.error("[viz:create-mpr] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:render-apr-slice",
    async (_event, sessionId, viewName, width, height) => {
      try {
        if (!nativeHost) {
          return { success: false, error: "Visualization API not available" };
        }
        return nativeHost.invoke(
          "renderAPRSlice",
          sessionId,
          viewName,
          width,
          height
        );
      } catch (e) {
        console.error("[viz:render-apr-slice] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:capture-apr-screenshots",
    async (_event, sessionId, folderPath, selection, width, height) => {
      var _a;
      try {
        if (!nativeHost) {
          return { success: false, error: "Visualization API not available" };
        }
        if (!sessionId) {
          return { success: false, error: "Missing sessionId" };
        }
        if (!folderPath) {
          return { success: false, error: "Missing folderPath" };
        }
        const outDir = path.join(folderPath, "ScreenCapture");
        await fs.promises.mkdir(outDir, { recursive: true });
        const reqW = Number.isFinite(width) && width > 0 ? width : 1024;
        const reqH = Number.isFinite(height) && height > 0 ? height : 1024;
        const expandSelection = (sel) => {
          const raw = Array.isArray(sel) ? sel : [];
          const out = [];
          for (const v of raw) {
            if (v === "quad") {
              out.push("quad");
              continue;
            }
            if (v === "mpr+3d") {
              out.push("axial", "coronal", "sagittal", "3d");
            } else {
              out.push(v);
            }
          }
          return out.filter((v, i) => out.indexOf(v) === i);
        };
        const views = expandSelection(selection);
        if (views.length === 0) {
          return { success: false, error: "No views selected" };
        }
        const safeTimestamp = (/* @__PURE__ */ new Date()).toISOString().replaceAll(":", "-").replaceAll(".", "-");
        const savedFiles = [];
        const errors = [];
        const rgbaToBgraInPlace = (buf) => {
          for (let i = 0; i + 3 < buf.length; i += 4) {
            const r = buf[i];
            buf[i] = buf[i + 2];
            buf[i + 2] = r;
          }
        };
        const renderViewToBgra = async (viewName, w, h) => {
          try {
            const windowId = viewName;
            console.log(
              `[Screenshot] Using windowId: ${windowId} for viewName: ${viewName}`
            );
            const r = await nativeHost.invoke(
              "renderAPRSlice",
              sessionId,
              windowId,
              w,
              h
            );
            if (!(r == null ? void 0 : r.success)) {
              return {
                ok: false,
                error: (r == null ? void 0 : r.error) || `renderAPRSlice failed for ${viewName}`
              };
            }
            const outW = Number((r == null ? void 0 : r.width) ?? w);
            const outH = Number((r == null ? void 0 : r.height) ?? h);
            const pixelData = r == null ? void 0 : r.pixelData;
            if (!pixelData || !Number.isFinite(outW) || !Number.isFinite(outH) || outW <= 0 || outH <= 0) {
              return {
                ok: false,
                error: `Invalid render result for ${viewName}: width=${outW}, height=${outH}, hasPixelData=${!!pixelData}`
              };
            }
            const rgba = Buffer.isBuffer(pixelData) ? Buffer.from(pixelData) : Buffer.from(pixelData);
            const expectedSize = outW * outH * 4;
            if (rgba.length !== expectedSize) {
              return {
                ok: false,
                error: `Pixel data size mismatch for ${viewName}: expected ${expectedSize} (${outW}x${outH}x4), got ${rgba.length}`
              };
            }
            rgbaToBgraInPlace(rgba);
            return { ok: true, bgra: rgba, width: outW, height: outH };
          } catch (e) {
            const errorMsg = (e == null ? void 0 : e.message) || (typeof e === "object" ? JSON.stringify(e) : String(e));
            console.error(
              `[Screenshot] renderViewToBgra failed for ${viewName}:`,
              e
            );
            return { ok: false, error: errorMsg };
          }
        };
        const saveBgraAsPng = async (bgra, w, h, nameForFile) => {
          const img = nativeImage.createFromBitmap(bgra, {
            width: w,
            height: h
          });
          const png = img.toPNG();
          const fileName = `${safeTimestamp}_${nameForFile}.png`;
          const filePath = path.join(outDir, fileName);
          await fs.promises.writeFile(filePath, png);
          savedFiles.push(filePath);
        };
        const viewsSet = new Set(views);
        if (viewsSet.has("quad")) {
          console.log(`[Screenshot] Starting quad render: ${reqW}x${reqH}`);
          const tileW = Math.max(1, Math.floor(reqW / 2));
          const tileH = Math.max(1, Math.floor(reqH / 2));
          const order = [
            { key: "axial", x: 0, y: 0 },
            { key: "coronal", x: 1, y: 0 },
            { key: "sagittal", x: 0, y: 1 },
            { key: "3d", x: 1, y: 1 }
          ];
          const canvasW = tileW * 2;
          const canvasH = tileH * 2;
          const out = Buffer.alloc(canvasW * canvasH * 4);
          const rowBytes = tileW * 4;
          let quadOk = true;
          let successfulTiles = 0;
          for (const item of order) {
            console.log(
              `[Screenshot] Rendering quad tile: ${item.key} (${tileW}x${tileH})`
            );
            const rr = await renderViewToBgra(item.key, tileW, tileH);
            if (!rr.ok) {
              console.error(
                `[Screenshot] Quad tile ${item.key} failed: ${rr.error}`
              );
              errors.push({ view: `quad:${item.key}`, error: rr.error });
              quadOk = false;
              continue;
            }
            if (rr.width !== tileW || rr.height !== tileH) {
              const sizeError = `Unexpected size ${rr.width}x${rr.height} for ${item.key} (expected ${tileW}x${tileH})`;
              console.error(`[Screenshot] ${sizeError}`);
              errors.push({
                view: `quad:${item.key}`,
                error: sizeError
              });
              quadOk = false;
              continue;
            }
            for (let y = 0; y < tileH; y++) {
              const srcStart = y * rowBytes;
              const dstRow = (item.y * tileH + y) * canvasW * 4;
              const dstStart = dstRow + item.x * rowBytes;
              rr.bgra.copy(out, dstStart, srcStart, srcStart + rowBytes);
            }
            successfulTiles++;
          }
          console.log(
            `[Screenshot] Quad completed: ${successfulTiles}/4 tiles successful, quadOk=${quadOk}`
          );
          if (quadOk || successfulTiles >= 2) {
            await saveBgraAsPng(out, canvasW, canvasH, "quad");
            if (!quadOk) {
              console.log(
                `[Screenshot] Saved partial quad with ${successfulTiles}/4 tiles`
              );
            }
          } else {
            console.error(
              `[Screenshot] Quad completely failed: only ${successfulTiles}/4 tiles successful`
            );
          }
        }
        for (const viewName of views) {
          if (viewName === "quad") continue;
          console.log(
            `[Screenshot] Rendering single view: ${viewName} (${reqW}x${reqH})`
          );
          const rr = await renderViewToBgra(viewName, reqW, reqH);
          if (!rr.ok) {
            console.error(
              `[Screenshot] Single view ${viewName} failed: ${rr.error}`
            );
            errors.push({ view: viewName, error: rr.error });
            continue;
          }
          await saveBgraAsPng(rr.bgra, rr.width, rr.height, viewName);
          console.log(
            `[Screenshot] Single view ${viewName} saved successfully`
          );
        }
        if (savedFiles.length === 0) {
          return {
            success: false,
            error: ((_a = errors[0]) == null ? void 0 : _a.error) || "截图失败",
            outputDir: outDir,
            errors
          };
        }
        return { success: true, outputDir: outDir, files: savedFiles, errors };
      } catch (e) {
        console.error("[viz:capture-apr-screenshots] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:update-center",
    async (_event, sessionId, x, y, z) => {
      try {
        if (!nativeHost) {
          return { success: false, error: "Visualization API not available" };
        }
        return nativeHost.invoke("updateAPRCenter", sessionId, x, y, z);
      } catch (e) {
        console.error("[viz:update-center] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:update-rotation",
    async (_event, sessionId, angleX, angleY, angleZ) => {
      try {
        if (!nativeHost) {
          return { success: false, error: "Visualization API not available" };
        }
        return nativeHost.invoke(
          "updateAPRRotation",
          sessionId,
          angleX,
          angleY,
          angleZ
        );
      } catch (e) {
        console.error("[viz:update-rotation] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:set-crosshair-visible",
    async (_event, sessionId, visible) => {
      try {
        if (!nativeHost) {
          return { success: false, error: "Visualization API not available" };
        }
        await nativeHost.invoke("setCrosshairVisible", sessionId, visible);
        return { success: true };
      } catch (e) {
        console.error("[viz:set-crosshair-visible] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:get-apr-state",
    async (_event, sessionId, viewOrWindowId) => {
      try {
        if (!nativeHost) {
          return { success: false, error: "Visualization API not available" };
        }
        return nativeHost.invoke("getAPRState", sessionId, viewOrWindowId);
      } catch (e) {
        console.error("[viz:get-apr-state] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:set-window-level",
    async (_event, sessionId, windowWidth, windowLevel) => {
      try {
        if (!nativeHost) {
          return { success: false, error: "Visualization API not available" };
        }
        await nativeHost.invoke(
          "setSessionWindowLevel",
          sessionId,
          windowWidth,
          windowLevel
        );
        return { success: true };
      } catch (e) {
        console.error("[viz:set-window-level] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle("viz:get-window-level", async (_event, sessionId) => {
    try {
      if (!nativeHost) {
        return { success: false, error: "Visualization API not available" };
      }
      const st = await nativeHost.invoke("getSessionWindowLevel", sessionId);
      return { success: true, ...st };
    } catch (e) {
      console.error("[viz:get-window-level] Error:", e);
      return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
    }
  });
  ipcMain.handle(
    "viz:set-projection-mode",
    async (_event, sessionId, mode, thickness) => {
      try {
        if (!nativeHost) {
          return { success: false, error: "Visualization API not available" };
        }
        await nativeHost.invoke(
          "setSessionProjectionMode",
          sessionId,
          mode,
          thickness
        );
        return { success: true };
      } catch (e) {
        console.error("[viz:set-projection-mode] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:get-projection-mode",
    async (_event, sessionId) => {
      try {
        if (!nativeHost) {
          return { success: false, error: "Visualization API not available" };
        }
        const st = await nativeHost.invoke(
          "getSessionProjectionMode",
          sessionId
        );
        return { success: true, ...st };
      } catch (e) {
        console.error("[viz:get-projection-mode] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle("viz:crop-volume", async (_event, sessionId) => {
    try {
      if (!nativeHost) {
        return { success: false, error: "Visualization API not available" };
      }
      return nativeHost.invoke("cropVolume", sessionId);
    } catch (e) {
      console.error("[viz:crop-volume] Error:", e);
      return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
    }
  });
  ipcMain.handle(
    "viz:apply-cropped-volume",
    async (_event, sessionId) => {
      try {
        if (!nativeHost) {
          return { success: false, error: "Visualization API not available" };
        }
        return nativeHost.invoke("applyCroppedVolumeToSession", sessionId);
      } catch (e) {
        console.error("[viz:apply-cropped-volume] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle("viz:set-crop-shape", async (_event, shape) => {
    try {
      if (!nativeHost) {
        return { success: false, error: "Visualization API not available" };
      }
      await nativeHost.invoke("setAPRCropShape", shape);
      return { success: true };
    } catch (e) {
      console.error("[viz:set-crop-shape] Error:", e);
      return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
    }
  });
  ipcMain.handle("viz:get-crop-shape", async () => {
    try {
      if (!nativeHost) return 0;
      return await nativeHost.invoke("getAPRCropShape") ?? 0;
    } catch (e) {
      console.error("[viz:get-crop-shape] Error:", e);
      return 0;
    }
  });
  ipcMain.handle(
    "viz:set-crop-cylinder-direction",
    async (_event, direction) => {
      try {
        if (!nativeHost) {
          return { success: false, error: "Visualization API not available" };
        }
        await nativeHost.invoke("setAPRCropCylinderDirection", direction);
        return { success: true };
      } catch (e) {
        console.error("[viz:set-crop-cylinder-direction] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle("viz:get-crop-cylinder-direction", async () => {
    try {
      if (!nativeHost) return 0;
      return await nativeHost.invoke("getAPRCropCylinderDirection") ?? 0;
    } catch (e) {
      console.error("[viz:get-crop-cylinder-direction] Error:", e);
      return 0;
    }
  });
  ipcMain.handle(
    "viz:set-crop-box-size",
    async (_event, sizeX, sizeY, sizeZ, volumeWidth, volumeHeight, volumeDepth) => {
      try {
        if (!nativeHost) {
          return { success: false, error: "Visualization API not available" };
        }
        await nativeHost.invoke(
          "setAPRCropBoxSize",
          sizeX,
          sizeY,
          sizeZ,
          volumeWidth,
          volumeHeight,
          volumeDepth
        );
        return { success: true };
      } catch (e) {
        console.error("[viz:set-crop-box-size] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle("viz:get-crop-settings", async () => {
    try {
      if (!nativeHost) {
        return { shape: 0, cylinderDirection: 0, enabled: false, cropBox: {} };
      }
      return await nativeHost.invoke("getAPRCropSettings") ?? {
        shape: 0,
        cylinderDirection: 0,
        enabled: false,
        cropBox: {}
      };
    } catch (e) {
      console.error("[viz:get-crop-settings] Error:", e);
      return { shape: 0, cylinderDirection: 0, enabled: false, cropBox: {} };
    }
  });
  ipcMain.handle("viz:process-window-events", async () => {
    try {
      if (!nativeHost) return;
      await nativeHost.invoke("processWindowEvents");
    } catch (e) {
      console.error("[viz:process-window-events] Error:", e);
    }
  });
  ipcMain.handle("viz:destroy-apr", async (_event, sessionId) => {
    try {
      if (!nativeHost) {
        return { success: false, error: "Visualization API not available" };
      }
      return nativeHost.invoke("destroyAPRViews", sessionId);
    } catch (e) {
      console.error("[viz:destroy-apr] Error:", e);
      return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
    }
  });
  ipcMain.handle("viz:destroy-mpr", async (_event, sessionId) => {
    try {
      if (!nativeHost) {
        return { success: false, error: "Visualization API not available" };
      }
      return nativeHost.invoke("destroyMPRViews", sessionId);
    } catch (e) {
      console.error("[viz:destroy-mpr] Error:", e);
      return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
    }
  });
  ipcMain.handle(
    "viz:embed-window",
    async (event, windowIdOrHwnd, x, y, width, height) => {
      var _a, _b;
      try {
        if (!nativeHost) {
          return { success: false, error: "Window or API not available" };
        }
        const bw = BrowserWindow.fromWebContents(event.sender);
        if (!bw) return { success: false, error: "BrowserWindow not found" };
        const parent = ((_b = (_a = event.sender).getNativeWindowHandle) == null ? void 0 : _b.call(_a)) ?? bw.getNativeWindowHandle();
        await nativeHost.invoke(
          "embedWindow",
          windowIdOrHwnd,
          parent,
          x,
          y,
          width,
          height
        );
        return { success: true };
      } catch (e) {
        console.error("[viz:embed-window] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle("viz:render-all-views", async () => {
    try {
      if (!nativeHost) return;
      await nativeHost.invoke("renderAllViews");
    } catch (e) {
      console.error("[viz:render-all-views] Error:", e);
    }
  });
  ipcMain.handle("viz:invalidate-all-windows", async () => {
    try {
      if (!nativeHost) return;
      await nativeHost.invoke("invalidateAllWindows");
    } catch (e) {
      console.error("[viz:invalidate-all-windows] Error:", e);
    }
  });
  ipcMain.handle("viz:invalidate-window", async (_event, windowId) => {
    try {
      if (!nativeHost) return;
      await nativeHost.invoke("invalidateWindow", windowId);
    } catch (e) {
      console.error("[viz:invalidate-window] Error:", e);
    }
  });
  ipcMain.handle("viz:reset-view", async (_event, windowId) => {
    try {
      if (!nativeHost) return false;
      return await nativeHost.invoke("resetView", windowId) ?? false;
    } catch (e) {
      console.error("[viz:reset-view] Error:", e);
      return false;
    }
  });
  ipcMain.handle(
    "viz:set-3d-orthogonal-mode",
    async (_event, windowId, enabled) => {
      try {
        if (!nativeHost) return false;
        return await nativeHost.invoke("set3DOrthogonalMode", windowId, enabled) ?? false;
      } catch (e) {
        console.error("[viz:set-3d-orthogonal-mode] Error:", e);
        return false;
      }
    }
  );
  ipcMain.handle(
    "viz:set-3d-vram-optimized",
    async (_event, windowId, enabled) => {
      try {
        console.log("[viz:set-3d-vram-optimized]", { windowId, enabled });
        if (!nativeHost) return false;
        return await nativeHost.invoke("set3DVramOptimized", windowId, enabled) ?? false;
      } catch (e) {
        console.error("[viz:set-3d-vram-optimized] Error:", e);
        return false;
      }
    }
  );
  ipcMain.handle(
    "viz:set-3d-mask-iso-surface",
    async (_event, windowId, enabled) => {
      try {
        if (!nativeHost) return false;
        return await nativeHost.invoke("set3DMaskIsoSurface", windowId, enabled) ?? false;
      } catch (e) {
        console.error("[viz:set-3d-mask-iso-surface] Error:", e);
        return false;
      }
    }
  );
  ipcMain.handle(
    "viz:set-3d-light-parameters",
    async (_event, windowId, ambient, diffuse, specular) => {
      try {
        if (!nativeHost) return false;
        return await nativeHost.invoke(
          "set3DLightParameters",
          windowId,
          ambient,
          diffuse,
          specular
        ) ?? false;
      } catch (e) {
        console.error("[viz:set-3d-light-parameters] Error:", e);
        return false;
      }
    }
  );
  ipcMain.handle(
    "viz:set-3d-transfer-function",
    async (_event, windowId, points) => {
      try {
        if (!nativeHost) return false;
        return await nativeHost.invoke(
          "set3DTransferFunction",
          windowId,
          points
        ) ?? false;
      } catch (e) {
        console.error("[viz:set-3d-transfer-function] Error:", e);
        return false;
      }
    }
  );
  ipcMain.handle("viz:get-gpu-info", async (_event, windowId) => {
    try {
      if (!nativeHost) {
        return { success: false, error: "Visualization API not available" };
      }
      return nativeHost.invoke("getGpuInfo", windowId);
    } catch (e) {
      console.error("[viz:get-gpu-info] Error:", e);
      return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
    }
  });
  ipcMain.handle(
    "viz:pack-his4d",
    async (_event, outputPath, folders, timestampsMs) => {
      try {
        if (!nativeHost) {
          return { success: false, error: "Visualization API not available" };
        }
        return nativeHost.invoke(
          "packHis4dFromFolders",
          outputPath,
          folders,
          timestampsMs
        );
      } catch (e) {
        console.error("[viz:pack-his4d] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:create-apr-his4d",
    async (_event, sessionId, his4dPath) => {
      try {
        if (!nativeHost) {
          return { success: false, error: "Visualization API not available" };
        }
        return nativeHost.invoke(
          "createAPRViewsFromHis4d",
          sessionId,
          his4dPath
        );
      } catch (e) {
        console.error("[viz:create-apr-his4d] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:his4d-set-frame",
    async (_event, sessionId, frameIndex) => {
      try {
        if (!nativeHost) {
          return { success: false, error: "Visualization API not available" };
        }
        return nativeHost.invoke("his4dSetFrame", sessionId, frameIndex);
      } catch (e) {
        console.error("[viz:his4d-set-frame] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:his4d-get-session-info",
    async (_event, sessionId) => {
      try {
        if (!nativeHost) {
          return { success: false, error: "Visualization API not available" };
        }
        return nativeHost.invoke("his4dGetSessionInfo", sessionId);
      } catch (e) {
        console.error("[viz:his4d-get-session-info] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle("viz:destroy-all-3d-windows", async () => {
    try {
      if (!nativeHost) return;
      await nativeHost.invoke("destroyAll3DWindows");
    } catch (e) {
      console.error("[viz:destroy-all-3d-windows] Error:", e);
    }
  });
  ipcMain.handle("viz:hide-all-windows", async () => {
    try {
      if (!nativeHost) return;
      await nativeHost.invoke("hideAllWindows");
    } catch (e) {
      console.error("[viz:hide-all-windows] Error:", e);
    }
  });
  ipcMain.handle("viz:show-all-windows", async () => {
    try {
      if (!nativeHost) return;
      await nativeHost.invoke("showAllWindows");
    } catch (e) {
      console.error("[viz:show-all-windows] Error:", e);
    }
  });
  ipcMain.handle("viz:destroy-all-windows", async () => {
    try {
      if (!nativeHost) return;
      await nativeHost.invoke("destroyAllWindows");
    } catch (e) {
      console.error("[viz:destroy-all-windows] Error:", e);
    }
  });
  ipcMain.handle(
    "viz:resize-window",
    async (event, windowId, x, y, width, height) => {
      var _a, _b;
      try {
        if (!nativeHost) return false;
        const bw = BrowserWindow.fromWebContents(event.sender);
        if (!bw) return false;
        const parent = ((_b = (_a = event.sender).getNativeWindowHandle) == null ? void 0 : _b.call(_a)) ?? bw.getNativeWindowHandle();
        await nativeHost.invoke(
          "embedWindow",
          windowId,
          parent,
          x,
          y,
          width,
          height
        );
        return true;
      } catch (e) {
        console.error("[viz:resize-window] Error:", e);
        return false;
      }
    }
  );
  let isRenderLoopRunning = false;
  let renderLoopTargetFps;
  let warnedMissingRaiseWindow = false;
  const getErrorMessage = (e) => {
    const anyErr = e;
    return (anyErr == null ? void 0 : anyErr.message) ?? String(e);
  };
  const isAddonMissingMethodError = (e, method) => {
    const msg = getErrorMessage(e);
    return msg.includes(`Method ${method} not found on addon`);
  };
  ipcMain.handle("viz:start-render-loop", async (_event, targetFPS) => {
    try {
      if (!nativeHost) return false;
      if (isRenderLoopRunning) {
        if (renderLoopTargetFps === void 0 || renderLoopTargetFps === targetFPS) {
          renderLoopTargetFps = targetFPS;
          return true;
        }
        try {
          await nativeHost.invoke("stopRenderLoop");
        } catch {
        }
      }
      const ok = await nativeHost.invoke("startRenderLoop", targetFPS) ?? false;
      isRenderLoopRunning = !!ok;
      if (ok) renderLoopTargetFps = targetFPS;
      return ok;
    } catch (e) {
      const msg = getErrorMessage(e);
      if (msg.includes("Render loop already running")) {
        isRenderLoopRunning = true;
        renderLoopTargetFps = targetFPS;
        return true;
      }
      console.error("[viz:start-render-loop] Error:", e);
      return false;
    }
  });
  ipcMain.handle("viz:stop-render-loop", async () => {
    try {
      if (!nativeHost) return;
      await nativeHost.invoke("stopRenderLoop");
      isRenderLoopRunning = false;
      renderLoopTargetFps = void 0;
    } catch (e) {
      console.error("[viz:stop-render-loop] Error:", e);
    }
  });
  ipcMain.handle("viz:refresh-all-windows-zorder", async () => {
    try {
      if (!nativeHost) return;
      await nativeHost.invoke("refreshAllWindowsZOrder");
    } catch (e) {
      console.error("[viz:refresh-all-windows-zorder] Error:", e);
    }
  });
  ipcMain.handle(
    "viz:refresh-window-zorder",
    async (_event, windowId) => {
      try {
        if (!nativeHost) return false;
        return await nativeHost.invoke("refreshWindowZOrder", windowId) ?? false;
      } catch (e) {
        console.error("[viz:refresh-window-zorder] Error:", e);
        return false;
      }
    }
  );
  ipcMain.handle("viz:raise-all-windows", async () => {
    try {
      if (!nativeHost) return;
      await nativeHost.invoke("raiseAllWindows");
    } catch (e) {
      console.error("[viz:raise-all-windows] Error:", e);
    }
  });
  ipcMain.handle("viz:raise-window", async (_event, windowId) => {
    try {
      if (!nativeHost) return false;
      return await nativeHost.invoke("raiseWindow", windowId) ?? false;
    } catch (e) {
      if (isAddonMissingMethodError(e, "raiseWindow")) {
        if (!warnedMissingRaiseWindow) {
          warnedMissingRaiseWindow = true;
          console.warn(
            "[viz:raise-window] raiseWindow missing on addon; falling back to z-order refresh. (Rebuild native addon to fix.)"
          );
        }
        try {
          await nativeHost.invoke("refreshWindowZOrder", windowId).catch(() => {
          });
          await nativeHost.invoke("refreshAllWindowsZOrder").catch(() => {
          });
        } catch {
        }
        return true;
      }
      console.error("[viz:raise-window] Error:", e);
      return false;
    }
  });
  app.on("browser-window-focus", (_e, bw) => {
    try {
      if (bw.isMinimized()) return;
      void (nativeHost == null ? void 0 : nativeHost.invoke("refreshAllWindowsZOrder"));
    } catch {
    }
  });
  app.on("browser-window-blur", (_e, bw) => {
    try {
      if (bw.isMinimized()) return;
      void (nativeHost == null ? void 0 : nativeHost.invoke("refreshAllWindowsZOrder"));
    } catch {
    }
  });
  ipcMain.handle(
    "viz:set-window-tool-type",
    async (_event, windowId, toolType) => {
      try {
        if (!nativeHost) return false;
        return await nativeHost.invoke("setWindowToolType", windowId, toolType) ?? false;
      } catch (e) {
        console.error("[viz:set-window-tool-type] Error:", e);
        return false;
      }
    }
  );
  ipcMain.handle(
    "viz:set-window-crop-box-visible",
    async (_event, windowId, visible) => {
      try {
        if (!nativeHost) return false;
        return await nativeHost.invoke(
          "setWindowCropBoxVisible",
          windowId,
          visible
        ) ?? false;
      } catch (e) {
        console.error("[viz:set-window-crop-box-visible] Error:", e);
        return false;
      }
    }
  );
  ipcMain.handle("viz:enable-apr-crop-box", async (_event, enable) => {
    try {
      if (!nativeHost) return false;
      await nativeHost.invoke("enableAPRCropBox", enable);
      return true;
    } catch (e) {
      console.error("[viz:enable-apr-crop-box] Error:", e);
      return false;
    }
  });
  ipcMain.handle(
    "viz:set-apr-crop-box",
    async (_event, width, height, depth) => {
      try {
        if (!nativeHost) return false;
        await nativeHost.invoke("setAPRCropBox", width, height, depth);
        return true;
      } catch (e) {
        console.error("[viz:set-apr-crop-box] Error:", e);
        return false;
      }
    }
  );
  ipcMain.handle(
    "viz:set-apr-crop-box-range",
    async (_event, xStart, xEnd, yStart, yEnd, zStart, zEnd) => {
      try {
        if (!nativeHost) return false;
        await nativeHost.invoke(
          "setAPRCropBoxRange",
          xStart,
          xEnd,
          yStart,
          yEnd,
          zStart,
          zEnd
        );
        return true;
      } catch (e) {
        console.error("[viz:set-apr-crop-box-range] Error:", e);
        return false;
      }
    }
  );
  ipcMain.handle("viz:get-apr-crop-box", async () => {
    try {
      if (!nativeHost) return {};
      return await nativeHost.invoke("getAPRCropBox") ?? {};
    } catch (e) {
      console.error("[viz:get-apr-crop-box] Error:", e);
      return {};
    }
  });
  ipcMain.handle("viz:is-apr-crop-box-enabled", async () => {
    try {
      if (!nativeHost) return false;
      return await nativeHost.invoke("isAPRCropBoxEnabled") ?? false;
    } catch (e) {
      console.error("[viz:is-apr-crop-box-enabled] Error:", e);
      return false;
    }
  });
  ipcMain.handle("viz:get-completed-measurements", async () => {
    try {
      if (!nativeHost) return [];
      return await nativeHost.invoke("getCompletedMeasurements") ?? [];
    } catch (e) {
      console.error("[viz:get-completed-measurements] Error:", e);
      return [];
    }
  });
  ipcMain.handle(
    "viz:delete-measurement",
    async (_event, measurementId) => {
      try {
        if (!nativeHost) return false;
        return await nativeHost.invoke("deleteMeasurement", measurementId) ?? false;
      } catch (e) {
        console.error("[viz:delete-measurement] Error:", e);
        return false;
      }
    }
  );
  ipcMain.handle(
    "viz:get-measurement-profile",
    async (_event, sessionId, measurementId, maxPoints) => {
      try {
        if (!nativeHost)
          return { success: false, error: "Native host unavailable" };
        return await nativeHost.invoke(
          "getMeasurementProfile",
          sessionId,
          measurementId,
          maxPoints
        );
      } catch (e) {
        console.error("[viz:get-measurement-profile] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:get-measurement-region-histogram",
    async (_event, sessionId, measurementId) => {
      try {
        if (!nativeHost) {
          return { success: false, error: "Native host unavailable" };
        }
        return await nativeHost.invoke(
          "getMeasurementRegionHistogram",
          sessionId,
          measurementId
        );
      } catch (e) {
        console.error("[viz:get-measurement-region-histogram] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:update-mpr-center",
    async (_event, sessionId, x, y, z) => {
      try {
        if (!nativeHost)
          return { success: false, error: "Native host unavailable" };
        return await nativeHost.invoke("updateMPRCenter", sessionId, x, y, z);
      } catch (e) {
        console.error("[viz:update-mpr-center] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:get-volume-histogram",
    async (_event, sessionId) => {
      try {
        if (!nativeHost)
          return { success: false, error: "Native host unavailable" };
        return await nativeHost.invoke("getVolumeHistogram", sessionId);
      } catch (e) {
        console.error("[viz:get-volume-histogram] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:get-mask-statistics",
    async (_event, sessionId, maskId) => {
      try {
        if (!nativeHost)
          return { success: false, error: "Native host unavailable" };
        return await nativeHost.invoke("getMaskStatistics", sessionId, maskId);
      } catch (e) {
        console.error("[viz:get-mask-statistics] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:get-volume-spacing",
    async (_event, sessionId) => {
      try {
        if (!nativeHost)
          return { success: false, error: "Native host unavailable" };
        return await nativeHost.invoke("getVolumeSpacing", sessionId);
      } catch (e) {
        console.error("[viz:get-volume-spacing] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:calculate-bone-metrics",
    async (_event, sessionId, maskId, roiMaskId) => {
      try {
        if (!nativeHost)
          return { success: false, error: "Native host unavailable" };
        return await nativeHost.invoke(
          "calculateBoneMetrics",
          sessionId,
          maskId,
          roiMaskId
        );
      } catch (e) {
        console.error("[viz:calculate-bone-metrics] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:update-preview-mask",
    async (_event, sessionId, minT, maxT, hexColor) => {
      try {
        if (!nativeHost)
          return { success: false, error: "Native host unavailable" };
        return await nativeHost.invoke(
          "updatePreviewMask",
          sessionId,
          minT,
          maxT,
          hexColor
        );
      } catch (e) {
        console.error("[viz:update-preview-mask] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:clear-preview-mask",
    async (_event, sessionId) => {
      try {
        if (!nativeHost)
          return { success: false, error: "Native host unavailable" };
        return await nativeHost.invoke("clearPreviewMask", sessionId);
      } catch (e) {
        console.error("[viz:clear-preview-mask] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:create-mask-from-threshold",
    async (_event, sessionId, minT, maxT, hexColor, maskName) => {
      try {
        if (!nativeHost)
          return { success: false, error: "Native host unavailable" };
        return await nativeHost.invoke(
          "createMaskFromThreshold",
          sessionId,
          minT,
          maxT,
          hexColor,
          maskName
        );
      } catch (e) {
        console.error("[viz:create-mask-from-threshold] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:delete-mask",
    async (_event, sessionId, maskId) => {
      try {
        if (!nativeHost)
          return { success: false, error: "Native host unavailable" };
        return await nativeHost.invoke("deleteMask", sessionId, maskId);
      } catch (e) {
        console.error("[viz:delete-mask] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:save-masks",
    async (_event, sessionId, folderPath, maskName) => {
      try {
        if (!nativeHost)
          return { success: false, error: "Native host unavailable" };
        return await nativeHost.invoke(
          "saveMasks",
          sessionId,
          folderPath,
          maskName
        );
      } catch (e) {
        console.error("[viz:save-masks] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:load-masks",
    async (_event, sessionId, folderPath) => {
      try {
        if (!nativeHost)
          return { success: false, error: "Native host unavailable" };
        return await nativeHost.invoke("loadMasks", sessionId, folderPath);
      } catch (e) {
        console.error("[viz:load-masks] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:export-mask-to-stl",
    async (_event, sessionId, maskId, filepath, step) => {
      try {
        if (!nativeHost)
          return { success: false, error: "Native host unavailable" };
        return await nativeHost.invoke(
          "exportMaskToStl",
          sessionId,
          maskId,
          filepath,
          step
        );
      } catch (e) {
        console.error("[viz:export-mask-to-stl] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:mask-morphology-2d",
    async (_event, sessionId, maskId, operation, kernelSize) => {
      try {
        if (!nativeHost)
          return { success: false, error: "Native host unavailable" };
        return await nativeHost.invoke(
          "maskMorphology2D",
          sessionId,
          maskId,
          operation,
          kernelSize
        );
      } catch (e) {
        console.error("[viz:mask-morphology-2d] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:mask-morphology-3d",
    async (_event, sessionId, maskId, operation, kernelSize) => {
      try {
        if (!nativeHost)
          return { success: false, error: "Native host unavailable" };
        return await nativeHost.invoke(
          "maskMorphology3D",
          sessionId,
          maskId,
          operation,
          kernelSize
        );
      } catch (e) {
        console.error("[viz:mask-morphology-3d] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:mask-boolean",
    async (_event, sessionId, maskIdA, maskIdB, operation, name, color) => {
      try {
        if (!nativeHost)
          return { success: false, error: "Native host unavailable" };
        return await nativeHost.invoke(
          "maskBoolean",
          sessionId,
          maskIdA,
          maskIdB,
          operation,
          name,
          color
        );
      } catch (e) {
        console.error("[viz:mask-boolean] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:mask-inverse",
    async (_event, sessionId, maskId) => {
      try {
        if (!nativeHost)
          return { success: false, error: "Native host unavailable" };
        return await nativeHost.invoke("maskInverse", sessionId, maskId);
      } catch (e) {
        console.error("[viz:mask-inverse] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:fat-analyze-separate-fat",
    async (_event, sessionId, maskId, lowThreshold, highThreshold) => {
      try {
        if (!nativeHost)
          return { success: false, error: "Native host unavailable" };
        const result = await nativeHost.invoke(
          "fatAnalyzeSeparateFat",
          sessionId,
          maskId,
          lowThreshold,
          highThreshold
        );
        return { success: true, ...result };
      } catch (e) {
        console.error("[viz:fat-analyze-separate-fat] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:fat-analyze-separate-lung",
    async (_event, sessionId, maskId) => {
      try {
        if (!nativeHost)
          return { success: false, error: "Native host unavailable" };
        const result = await nativeHost.invoke(
          "fatAnalyzeSeparateLung",
          sessionId,
          maskId
        );
        return { success: true, ...result };
      } catch (e) {
        console.error("[viz:fat-analyze-separate-lung] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:vascular-analyze-compute",
    async (_event, sessionId, arg1, arg2) => {
      try {
        if (!nativeHost)
          return { success: false, error: "Native host unavailable" };
        const result = await nativeHost.invoke(
          "vascularAnalyzeCompute",
          sessionId,
          arg1,
          arg2
        );
        return { success: true, ...result };
      } catch (e) {
        console.error("[viz:vascular-analyze-compute] Error:", e);
        return { success: false, error: (e == null ? void 0 : e.message) ?? String(e) };
      }
    }
  );
  ipcMain.handle(
    "viz:vascular-filter-keep-largest",
    async (_event, maskBuffer, w, h, d) => {
      try {
        if (!nativeHost)
          return { success: false, error: "Native host unavailable" };
        const res = await nativeHost.invoke(
          "vascularFilterKeepLargest",
          maskBuffer,
          w,
          h,
          d
        );
        return { success: !!res };
      } catch (e) {
        console.error("[viz:vascular-filter-keep-largest] Error:", e);
        return { success: false, error: e.message };
      }
    }
  );
}
app.whenReady().then(() => {
  Menu.setApplicationMenu(null);
  const addon = loadOpenGLAddon();
  const consoleDllAddon = loadConsoleDllAddon();
  const nativeHost = consoleDllAddon ? new DirectAddonAdapter(consoleDllAddon) : null;
  registerWindowControlHandlers();
  registerWindowLabHandlers();
  registerOpenGLHandlers(addon);
  registerDicomHandlers(nativeHost);
  registerVisualizationHandlers(nativeHost);
  registerPopupHandlers();
  registerDialogHandlers();
  createMainWindow();
  app.on("activate", () => {
    if (BrowserWindow.getAllWindows().length === 0) createMainWindow();
  });
});
app.on("window-all-closed", () => {
  if (process.platform !== "darwin") app.quit();
});
export {
  MAIN_DIST,
  RENDERER_DIST,
  VITE_DEV_SERVER_URL
};
