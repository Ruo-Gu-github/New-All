import { app, BrowserWindow, ipcMain } from 'electron'
import { createRequire } from 'node:module'
import { fileURLToPath } from 'node:url'
import path from 'node:path'

const require = createRequire(import.meta.url)
const __dirname = path.dirname(fileURLToPath(import.meta.url))

// The built directory structure
//
// ├─┬─┬ dist
// │ │ └── index.html
// │ │
// │ ├─┬ dist-electron
// │ │ ├── main.js
// │ │ └── preload.mjs
// │
process.env.APP_ROOT = path.join(__dirname, '..')

// 🚧 Use ['ENV_NAME'] avoid vite:define plugin - Vite@2.x
export const VITE_DEV_SERVER_URL = process.env['VITE_DEV_SERVER_URL']
export const MAIN_DIST = path.join(process.env.APP_ROOT, 'dist-electron')
export const RENDERER_DIST = path.join(process.env.APP_ROOT, 'dist')

process.env.VITE_PUBLIC = VITE_DEV_SERVER_URL ? path.join(process.env.APP_ROOT, 'public') : RENDERER_DIST

let win: BrowserWindow | null

const WINDOW_CONTROL_CHANNELS = {
  minimize: 'window-controls:minimize',
  toggleMaximize: 'window-controls:toggle-maximize',
  close: 'window-controls:close',
} as const

function registerWindowControlHandlers() {
  ipcMain.removeHandler(WINDOW_CONTROL_CHANNELS.minimize)
  ipcMain.removeHandler(WINDOW_CONTROL_CHANNELS.toggleMaximize)
  ipcMain.removeHandler(WINDOW_CONTROL_CHANNELS.close)

  ipcMain.handle(WINDOW_CONTROL_CHANNELS.minimize, () => {
    win?.minimize()
  })

  ipcMain.handle(WINDOW_CONTROL_CHANNELS.toggleMaximize, () => {
    if (!win) return
    if (win.isMaximized()) {
      win.restore()
    } else {
      win.maximize()
    }
  })

  ipcMain.handle(WINDOW_CONTROL_CHANNELS.close, () => {
    win?.close()
  })
}

function createWindow() {
  win = new BrowserWindow({
    width: 1600,
    height: 900,
    minWidth: 1600,
    minHeight: 900,
    frame: false,
    titleBarStyle: 'hidden',
    backgroundColor: '#101820',
    icon: path.join(process.env.VITE_PUBLIC, 'electron-vite.svg'),
    webPreferences: {
      preload: path.join(__dirname, 'preload.mjs'),
    },
  })

  win.on('closed', () => {
    win = null
  })

  // Test active push message to Renderer-process.
  win.webContents.on('did-finish-load', () => {
    win?.webContents.send('main-process-message', (new Date).toLocaleString())
  })

  if (VITE_DEV_SERVER_URL) {
    win.loadURL(VITE_DEV_SERVER_URL)
  } else {
    // win.loadFile('dist/index.html')
    win.loadFile(path.join(RENDERER_DIST, 'index.html'))
  }
}

// Quit when all windows are closed, except on macOS. There, it's common
// for applications and their menu bar to stay active until the user quits
// explicitly with Cmd + Q.
app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit()
    win = null
  }
})

app.on('activate', () => {
  // On OS X it's common to re-create a window in the app when the
  // dock icon is clicked and there are no other windows open.
  if (BrowserWindow.getAllWindows().length === 0) {
    createWindow()
  }
})

app.whenReady().then(() => {
  registerWindowControlHandlers()
  createWindow()
})
