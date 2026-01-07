# ConsoleDllTest 集成到 hiscan-analyzer

## ✅ 编译成功！

ConsoleDllTest 的 DLL 已经成功编译为 Node.js native addon，可以在 Electron/Vue 中使用。

## 📁 文件位置

```
hiscan-analyzer/
  native/
    console-dll/               # Native addon 项目
      build/Release/
        console_dll_addon.node  # 编译好的 addon
        *.dll                   # 所有依赖的 DLL
      src/
        addon.cpp              # 主入口
        dicom_wrapper.cpp      # DICOM 功能封装
        image_processing_wrapper.cpp  # 图像处理封装
      binding.gyp              # 编译配置
      index.js                 # JavaScript 导出
      test.js                  # 测试脚本
      README.md               # 详细文档
```

## 🚀 快速开始

### 1. 在 Node.js 中使用

```javascript
const { DicomVolume } = require('./native/console-dll');

const volume = new DicomVolume();
volume.loadFromFolder('D:/DICOM_DATA/patient001');

const dims = volume.getDimensions();
console.log(`Size: ${dims.width}x${dims.height}x${dims.depth}`);

const thumbnail = volume.generateThumbnail(256);
// thumbnail 是 Buffer，包含 256x256 RGBA 像素数据
```

### 2. 在 Electron Main Process 中使用

在 `electron/main.ts` 中：

```typescript
import { ipcMain, dialog } from 'electron';
import path from 'path';

// 动态加载 native addon
const consoleDll = require(path.join(__dirname, '../native/console-dll'));
const { DicomVolume } = consoleDll;

let currentVolume: any = null;

// 注册 IPC 处理器
ipcMain.handle('dicom:select-folder', async () => {
  const result = await dialog.showOpenDialog({
    properties: ['openDirectory']
  });
  
  if (result.canceled) return null;
  return result.filePaths[0];
});

ipcMain.handle('dicom:load-series', async (event, folderPath: string) => {
  try {
    currentVolume = new DicomVolume();
    currentVolume.loadFromFolder(folderPath);
    
    const dims = currentVolume.getDimensions();
    const spacing = currentVolume.getSpacing();
    const thumbnail = currentVolume.generateThumbnail(256);
    
    return {
      success: true,
      dimensions: dims,
      spacing: spacing,
      thumbnail: thumbnail.toString('base64')
    };
  } catch (error) {
    return {
      success: false,
      error: error.message
    };
  }
});

ipcMain.handle('dicom:get-slice', async (event, sliceIndex: number) => {
  if (!currentVolume) {
    throw new Error('No volume loaded');
  }
  
  const sliceData = currentVolume.getSlice(sliceIndex);
  return sliceData;
});

ipcMain.handle('dicom:cleanup', async () => {
  currentVolume = null;
});
```

### 3. 在 Preload Script 中暴露 API

在 `electron/preload.ts` 中：

```typescript
import { contextBridge, ipcRenderer } from 'electron';

contextBridge.exposeInMainWorld('dicomApi', {
  selectFolder: () => ipcRenderer.invoke('dicom:select-folder'),
  loadSeries: (folderPath: string) => ipcRenderer.invoke('dicom:load-series', folderPath),
  getSlice: (index: number) => ipcRenderer.invoke('dicom:get-slice', index),
  cleanup: () => ipcRenderer.invoke('dicom:cleanup')
});
```

### 4. 在 Vue 组件中使用

```vue
<template>
  <div class="dicom-viewer">
    <el-button @click="loadDicom">加载 DICOM</el-button>
    
    <div v-if="seriesInfo">
      <p>尺寸: {{ seriesInfo.dimensions.width }} x {{ seriesInfo.dimensions.height }} x {{ seriesInfo.dimensions.depth }}</p>
      <p>间距: {{ seriesInfo.spacing.x.toFixed(2) }} x {{ seriesInfo.spacing.y.toFixed(2) }} x {{ seriesInfo.spacing.z.toFixed(2) }} mm</p>
      
      <img v-if="thumbnailUrl" :src="thumbnailUrl" alt="缩略图" />
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue';

const seriesInfo = ref<any>(null);
const thumbnailUrl = ref('');

async function loadDicom() {
  try {
    // 选择文件夹
    const folderPath = await (window as any).dicomApi.selectFolder();
    if (!folderPath) return;
    
    // 加载 DICOM
    const result = await (window as any).dicomApi.loadSeries(folderPath);
    
    if (result.success) {
      seriesInfo.value = result;
      thumbnailUrl.value = `data:image/png;base64,${result.thumbnail}`;
    } else {
      console.error('加载失败:', result.error);
    }
  } catch (error) {
    console.error('错误:', error);
  }
}
</script>
```

## 📚 API 文档

### DicomVolume 类

#### 构造函数
```javascript
const volume = new DicomVolume();
```

#### 方法

##### loadFromFolder(folderPath)
加载 DICOM 序列

- **参数**: `folderPath` (string) - DICOM 文件夹路径
- **返回**: boolean
- **异常**: 加载失败时抛出异常

##### getDimensions()
获取体数据尺寸

- **返回**: `{ width: number, height: number, depth: number }`

##### getSpacing()
获取体素间距

- **返回**: `{ x: number, y: number, z: number }` (单位: mm)

##### getData()
获取完整体数据

- **返回**: Buffer (short[] 数组)

##### getSlice(index)
获取指定切片

- **参数**: `index` (number) - 切片索引 (0 到 depth-1)
- **返回**: Buffer (short[] 数组)

##### generateThumbnail(size?)
生成缩略图

- **参数**: `size` (number, 可选) - 缩略图尺寸，默认 256
- **返回**: Buffer (RGBA 像素数据)

## 🔧 重新编译

如果修改了 ConsoleDllTest 的 DLL，需要重新编译 addon：

```bash
cd native/console-dll
npm run build
```

## 📝 类型定义

建议创建 TypeScript 类型定义：

```typescript
// types/console-dll.d.ts
declare module 'console-dll-addon' {
  export class DicomVolume {
    constructor();
    loadFromFolder(folderPath: string): boolean;
    getDimensions(): { width: number; height: number; depth: number };
    getSpacing(): { x: number; y: number; z: number };
    getData(): Buffer;
    getSlice(index: number): Buffer;
    generateThumbnail(size?: number): Buffer;
  }
  
  export class MaskManager {
    // TODO: 添加方法定义
  }
  
  export class ROIManager {
    // TODO: 添加方法定义
  }
}

// Window API 扩展
interface Window {
  dicomApi: {
    selectFolder(): Promise<string | null>;
    loadSeries(folderPath: string): Promise<{
      success: boolean;
      dimensions?: { width: number; height: number; depth: number };
      spacing?: { x: number; y: number; z: number };
      thumbnail?: string;
      error?: string;
    }>;
    getSlice(index: number): Promise<Buffer>;
    cleanup(): Promise<void>;
  };
}
```

## ⚠️ 注意事项

1. **DLL 依赖**: 所有 DLL 文件必须在 `build/Release/` 目录下
2. **路径问题**: Electron 打包时需要确保 addon 和 DLL 被正确包含
3. **内存管理**: 使用完 DicomVolume 后应该手动清理
4. **线程安全**: Native addon 在主线程运行，耗时操作会阻塞 UI

## 🎯 下一步

- [ ] 在实际项目中集成到 AnalyzerImageManagerTab.vue
- [ ] 添加 ImageProcessing API（Mask、ROI）
- [ ] 添加 Visualization API（MPR、APR、3D）
- [ ] 实现异步操作（避免阻塞 UI）
- [ ] 添加进度回调
- [ ] 完善错误处理

## 🐛 故障排查

### 问题：Module did not self-register
**解决**: Node.js 版本不匹配，重新编译 addon

### 问题：找不到 DLL
**解决**: 确保所有 DLL 在 `build/Release/` 目录，或添加到系统 PATH

### 问题：加载 DICOM 失败
**解决**: 检查文件夹路径、DICOM 文件有效性、查看错误信息
