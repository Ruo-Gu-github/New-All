# 使用 ConsoleDllTest 的 DLL 集成方案

## 方案说明

直接使用 ConsoleDllTest 中已编译的 DLL，通过 `ffi-napi` 调用，**不需要**将其编译为 `.node` 文件。

### 为什么不用 node-addon-api？

1. **node-addon-api** 需要将 C++ 代码编译成 `.node` 文件（Node.js 原生模块）
2. 你已经有现成的 DLL，重新编译会很麻烦
3. **ffi-napi** 可以直接调用任何标准 Windows DLL，无需重新编译

## 所需依赖

```bash
npm install ffi-napi ref-napi --save
```

## DLL 路径配置

### 开发模式
```
ConsoleDllTest/Dlls/debug/bin/
  ├── DllDicom.dll
  ├── DllCore.dll
  ├── DllVisualization.dll
  ├── DllImageProcessing.dll
  └── ... (其他依赖 DLL)
```

### 生产模式（打包后）
```
resources/dlls/
  └── (所有 DLL 文件)
```

## 使用步骤

### 1. 安装依赖

```bash
cd hiscan-analyzer
npm install ffi-napi ref-napi --save
```

### 2. 测试 DLL 加载

```bash
node test-dll-load.cjs
```

修改测试脚本中的 DICOM 数据路径后运行，验证 DLL 是否正常工作。

### 3. 集成到 Electron

已创建的文件：
- `electron/dicomLoader.ts` - DICOM 加载封装
- `electron/main.ts` - IPC 处理（已更新）
- `electron/preload.ts` - API 暴露（已更新）
- `src/components/AnalyzerImageManagerTab.vue` - UI 组件（已更新）

### 4. 启动开发服务器

```bash
npm run dev
```

点击"加载图像"按钮，选择包含 DICOM 文件的文件夹。

## DLL 依赖问题

如果遇到 "找不到指定的模块" 错误，可能是缺少依赖 DLL：

### 方案 1: 复制所有依赖到同一目录

将这些 DLL 复制到 `ConsoleDllTest/Dlls/debug/bin/`:
- GDCM 相关 DLL (gdcmCommon.dll, gdcmDICT.dll 等)
- OpenCV DLL (opencv_world455d.dll)
- VTK DLL (如果用到)
- MSVC 运行库 (vcruntime140.dll, msvcp140.dll)

### 方案 2: 添加到系统 PATH

```javascript
// 在 electron/main.ts 中添加
process.env.PATH = `${dllPath};${process.env.PATH}`;
```

### 方案 3: 使用 Dependency Walker 检查

下载 Dependency Walker 查看你的 DLL 缺少哪些依赖。

## API 调用示例

### 加载 DICOM 序列

```typescript
// 在渲染进程中
const seriesInfo = await window.dicomApi.loadSeries(folderPath);

console.log(seriesInfo);
// {
//   folderPath: "D:/DICOM_DATA",
//   seriesName: "1.2.3.20211216.103835_0001",
//   fileCount: 1166,
//   width: 512,
//   height: 512,
//   depth: 1166,
//   spacingX: 1.0,
//   spacingY: 1.0,
//   spacingZ: 1.0,
//   thumbnail: "data:image/png;base64,..." // base64 缩略图
// }
```

### 获取切片数据

```typescript
const slice = await window.dicomApi.getSlice(100);

console.log(slice);
// {
//   width: 512,
//   height: 512,
//   data: [12, 45, 78, ...] // Uint16Array 转为数组
// }
```

## 注意事项

1. **路径问题**: Windows 路径需要使用 `\\` 或 `/`，避免使用单个 `\`
2. **内存管理**: 使用完 Volume 后记得调用 `Dicom_Volume_Destroy` 释放
3. **错误处理**: 所有 API 调用后检查返回值，失败时调用 `Dicom_GetLastError`
4. **数据传输**: 大数据（如完整体数据）不要通过 IPC 传输，只传切片或缩略图

## 打包配置

在 `electron-builder` 配置中添加 DLL 文件：

```json
{
  "extraResources": [
    {
      "from": "../ConsoleDllTest/Dlls/release/bin",
      "to": "dlls",
      "filter": ["*.dll"]
    }
  ]
}
```

## 故障排查

### DLL 加载失败

1. 检查 DLL 路径是否正确
2. 检查是否缺少依赖 DLL
3. 检查 DLL 是 x64 还是 x86（需要匹配 Node.js 版本）
4. 查看控制台错误信息

### DICOM 加载失败

1. 检查文件夹路径是否正确
2. 确认文件夹中有 `.dcm` 文件
3. 检查 DICOM 文件格式是否正确
4. 查看 `Dicom_GetLastError()` 的错误信息

### 缩略图不显示

1. 检查 base64 数据是否正确
2. 确认图像数据不为空
3. 检查缩放算法是否正确

## 下一步

- [ ] 测试 DLL 加载
- [ ] 安装 npm 依赖
- [ ] 运行测试脚本
- [ ] 启动开发服务器
- [ ] 测试加载 DICOM 图像
- [ ] 实现图像显示组件
- [ ] 集成其他 DLL (ImageProcessing, Visualization 等)
