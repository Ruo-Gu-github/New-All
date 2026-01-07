# Hiscan Analyzer

Electron + Vue 桌面端影像分析套件，提供 DICOM 载入、ROI 编辑等多页签工作流。本项目依赖一个 Node-API 原生插件通过 VTK 读取 DICOM，需使用 **VTK 9.5.x Shared Library + /MD (Release)** 的构建产物。

## 环境要求

- Node.js 20.x、npm 10.x
- MSVC / Windows SDK（在 Windows 平台上构建 Native 插件）
- 已编译好的 VTK（`BUILD_SHARED_LIBS=ON`、`CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL`、`CMAKE_BUILD_TYPE=Release`）

## 初始化步骤

```bash
npm install
npm run build:native:dicom
npm run dev
```

## 让 VTK DLL 在运行时可见

原生插件通过导入库（`.lib`）动态链接 VTK，在运行时需要能找到对应的 `.dll`。项目在 `electron/preload.ts` 中会尝试将以下目录加入 `PATH`：

1. `VTK_BIN_DIR` 或 `VTK_BIN_PATH` 环境变量指向的目录。
2. `APP_ROOT/vtk-bin`（可在项目根目录下创建 `vtk-bin`，复制需要的 VTK DLL）。
3. `APP_ROOT/../vtk/VTK-9.5.2/build/bin/Release`（与本仓库平级放置 `vtk/VTK-9.5.2` 的默认构建路径）。

> 建议：将发布所需的 VTK DLL 复制到 `hiscan-analyzer/vtk-bin` 中，并在打包流程里把该目录作为额外资源带入。

若仍提示缺少 DLL，可检查：

- `process.env.PATH` 是否包含对应目录；
- 复制的 DLL 是否与构建时使用的 VTK 版本一致；
- 是否混用了 Debug/Release 版本。

## Electron Builder 打包提示

当前 `electron-builder.json5` 仅包含构建产物目录。为了在安装包中携带 VTK DLL 与原生插件，需要在 `extraResources` 中加入：

```json5
{
   "extraResources": [
      {
         "from": "native/dicom/build/Release/dicom_native.node",
         "to": "resources/native/dicom"
      },
      {
         "from": "vtk-bin",
         "to": "resources/vtk-bin"
      }
   ]
}
```

同时确保运行时的 `VTK_BIN_DIR` 指向 `resources/vtk-bin`（预加载脚本已将该目录作为候选，无需额外配置）。

## 常用脚本

| 命令 | 说明 |
| --- | --- |
| `npm run dev` | 启动 Vite + Electron 双端开发环境 |
| `npm run build` | 构建前端、Electron 主进程并打包安装器 |
| `npm run build:native:dicom` | 使用 node-gyp 重建 DICOM 原生插件 |

## 目录结构速览

```
hiscan-analyzer/
├─ electron/            # Electron 主进程 & 预加载脚本
├─ native/dicom/        # Node-API 插件源码，负责 VTK DICOM 解析
├─ src/                 # Vue 3 前端代码
├─ vtk-bin/             # (可选) 运行期复制的 VTK DLL
└─ electron-builder.json5
```

## 参考

- [VTK Windows Build Instructions](https://vtk.org/Wiki/VTK/Configure_and_Build#Windows)
- [node-addon-api](https://github.com/nodejs/node-addon-api)
- [electron-builder extraResources](https://www.electron.build/configuration/contents#extraresources)
