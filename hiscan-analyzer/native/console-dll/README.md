# ConsoleDllTest Node.js Native Addon

这个项目将 ConsoleDllTest 的 C++ DLL 封装为 Node.js native addon，使其可以在 Electron/Node.js 环境中使用。

## 架构

```
JavaScript (Vue/Electron)
    ↓
Node.js Addon (C++ N-API)
    ↓
ConsoleDllTest DLLs (DllDicom.dll, DllImageProcessing.dll, etc.)
```

## 编译步骤

### 前置要求

1. **Visual Studio 2019 或更高版本** (需要 C++ 开发工具)
2. **Node.js** (推荐 v18 或更高)
3. **node-gyp**:
   ```cmd
   npm install -g node-gyp
   ```

### 编译

```cmd
cd hiscan-analyzer/native/console-dll
npm install
npm run build
```

编译成功后会生成：
- `build/Release/console_dll_addon.node` - Native addon
- `build/Release/*.dll` - 自动复制的 DLL 文件

## 使用示例

### JavaScript 端

```javascript
const { DicomVolume } = require('./native/console-dll');

// 创建 DICOM Volume
const volume = new DicomVolume();

// 加载 DICOM 序列
try {
  volume.loadFromFolder('D:/DICOM_DATA/patient001');
  
  // 获取尺寸
  const dims = volume.getDimensions();
  console.log(`Volume size: ${dims.width}x${dims.height}x${dims.depth}`);
  
  // 获取体素间距
  const spacing = volume.getSpacing();
  console.log(`Spacing: ${spacing.x}, ${spacing.y}, ${spacing.z}`);
  
  // 生成缩略图 (256x256 RGBA buffer)
  const thumbnail = volume.generateThumbnail(256);
  
  // 获取指定切片数据
  const sliceData = volume.getSlice(100); // Buffer of short[]
  
} catch (error) {
  console.error('Error:', error.message);
}
```

### Electron Main Process

```javascript
const { ipcMain } = require('electron');
const { DicomVolume } = require('./native/console-dll');

let currentVolume = null;

ipcMain.handle('dicom:load', async (event, folderPath) => {
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

ipcMain.handle('dicom:get-slice', async (event, sliceIndex) => {
  if (!currentVolume) {
    throw new Error('No volume loaded');
  }
  
  const sliceData = currentVolume.getSlice(sliceIndex);
  return sliceData;
});
```

### Vue 组件

```vue
<template>
  <div>
    <button @click="loadDicom">加载 DICOM</button>
    <img v-if="thumbnailUrl" :src="thumbnailUrl" />
  </div>
</template>

<script setup>
import { ref } from 'vue';

const thumbnailUrl = ref('');

async function loadDicom() {
  const result = await window.electron.invoke('dicom:load', 'D:/DICOM_DATA/patient001');
  
  if (result.success) {
    thumbnailUrl.value = `data:image/png;base64,${result.thumbnail}`;
    console.log('Dimensions:', result.dimensions);
    console.log('Spacing:', result.spacing);
  } else {
    console.error('Failed to load:', result.error);
  }
}
</script>
```

## API 参考

### DicomVolume

#### 构造函数
```javascript
const volume = new DicomVolume();
```

#### 方法

- **loadFromFolder(folderPath: string): boolean**
  - 从文件夹加载 DICOM 序列
  - 参数：`folderPath` - DICOM 文件所在文件夹路径
  - 返回：成功返回 `true`，失败抛出异常

- **getDimensions(): { width, height, depth }**
  - 获取体数据尺寸
  - 返回：包含 `width`, `height`, `depth` 的对象

- **getSpacing(): { x, y, z }**
  - 获取体素间距
  - 返回：包含 `x`, `y`, `z` 的对象（单位：mm）

- **getData(): Buffer**
  - 获取完整体数据
  - 返回：包含 short[] 数组的 Buffer

- **getSlice(index: number): Buffer**
  - 获取指定切片的数据
  - 参数：`index` - 切片索引 (0 到 depth-1)
  - 返回：包含 short[] 数组的 Buffer

- **generateThumbnail(size?: number): Buffer**
  - 生成缩略图
  - 参数：`size` - 缩略图尺寸（默认 256）
  - 返回：包含 RGBA 像素数据的 Buffer

### MaskManager (TODO)

将来会包含 Mask 编辑、形态学操作等功能。

### ROIManager (TODO)

将来会包含 ROI 选择、统计分析等功能。

## 故障排查

### 编译错误

1. **找不到 DllDicom.lib**
   - 确保 ConsoleDllTest 已经编译
   - 检查 `binding.gyp` 中的路径是否正确

2. **LNK2019 未解析的外部符号**
   - 确保 DLL 项目导出了所需的函数
   - 检查 `DicomApi.h` 中的 `DICOM_API` 宏定义

3. **运行时找不到 DLL**
   - 确保 DLL 文件在 `build/Release/` 目录下
   - 或将 DLL 路径添加到系统 PATH

### 运行时错误

1. **Module did not self-register**
   - Node.js 版本不兼容，重新编译
   - 使用 `node-gyp rebuild` 重新编译

2. **加载 DICOM 失败**
   - 检查文件夹路径是否正确
   - 确保文件夹包含有效的 DICOM 文件
   - 查看 `Dicom_GetLastError()` 返回的错误信息

## 性能优化

1. **避免频繁创建 Volume 对象** - 复用已创建的对象
2. **使用 SharedArrayBuffer** - 大数据传递时考虑零拷贝
3. **异步处理** - 在 Worker 线程中处理耗时操作
4. **缓存切片数据** - 避免重复调用 `getSlice()`

## 下一步

- [ ] 添加 ImageProcessing API（Mask、ROI）
- [ ] 添加 Visualization API（MPR、APR、3D）
- [ ] 支持异步操作（使用 N-API AsyncWorker）
- [ ] 添加进度回调
- [ ] 完善错误处理
