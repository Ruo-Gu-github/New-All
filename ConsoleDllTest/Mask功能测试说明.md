# Mask 功能测试说明

## 功能概述

完整的医学图像Mask管理系统，支持基于MPR体数据的分割、标注和分析操作。

## 核心功能

### 1. Mask创建
- **阈值分割**: 从体数据中根据HU值范围创建Mask
- **空白Mask**: 创建指定尺寸的空Mask
- **克隆Mask**: 复制现有Mask

### 2. 布尔运算
- **并集 (Union)**: A ∪ B
- **交集 (Intersection)**: A ∩ B  
- **差集 (Difference)**: A - B
- **异或 (XOR)**: A ⊕ B
- **反转 (Invert)**: ~A

### 3. 形态学操作
- **膨胀 (Dilate)**: 扩展Mask边界，使用3D球形结构元素
- **腐蚀 (Erode)**: 收缩Mask边界，去除小噪点
- **开运算 (Opening)**: 先腐蚀后膨胀，去除小突出
- **闭运算 (Closing)**: 先膨胀后腐蚀，填充小孔洞
- **形态学梯度**: 膨胀 - 腐蚀，提取边界

### 4. 连通域分析
- **FloodFill**: 从种子点开始的6-连通区域填充
- **连通域分解**: 将Mask分解为独立的连通域
- **过滤小区域**: 移除体素数量小于阈值的连通域
- **保留最大区域**: 只保留最大的连通域
- **填充孔洞**: 填充Mask内部的空洞

### 5. ROI绘制工具
- **矩形**: 在指定切片绘制矩形ROI（填充/边框）
- **圆形**: 在指定切片绘制圆形ROI
- **多边形**: 自定义多边形ROI，支持扫描线填充
- **画笔**: 圆形画笔涂抹
- **线段**: Bresenham直线绘制算法

### 6. 测量与统计
- **体积计算**: 统计Mask内的体素数量
- **质心计算**: 计算Mask的几何中心
- **边界框**: 计算Mask的最小外接立方体
- **边界提取**: 提取Mask的外表面
- **直方图统计**: 计算Mask内体数据的直方图分布

### 7. 导入导出
- **保存为二进制文件**: 包含元数据（名称、颜色、阈值）
- **从文件加载**: 恢复完整的Mask数据
- **未来支持**: STL格式导出（3D打印）

## 测试命令

在主程序中输入 `mask-mpr` 启动完整测试流程：

```bash
mask-mpr
```

## 测试流程

### 自动测试步骤

1. **加载DICOM数据**
   - 从指定路径加载DICOM系列
   - 创建3D体数据Volume

2. **创建骨组织Mask**
   - 阈值范围: 200-3000 HU
   - 自动分配颜色
   - 统计体素数量和质心
   - 计算直方图（256bins）
   - 显示Top 5直方图bin

3. **创建软组织Mask**
   - 阈值范围: 0-200 HU
   - 统计体素数量

4. **布尔运算测试**
   - 骨组织 ∪ 软组织
   - 骨组织 - 软组织
   - 显示结果体素数量

5. **形态学操作测试**
   - 腐蚀（半径2）：去除小噪点
   - 膨胀（半径2）：扩展边界
   - 比较前后体素数量变化

6. **连通域分析测试**
   - 分解所有连通域
   - 显示前5个连通域信息
   - 提取最大连通域

7. **导出导入测试**
   - 保存Mask到 `bone_mask.bin`
   - 从文件重新加载
   - 验证数据完整性

8. **可视化（可选）**
   - 询问是否打开MPR窗口
   - 在MPR窗口中叠加显示Mask

## API使用示例

### C++ API

```cpp
// 创建MaskManager
MaskManagerHandle manager = MaskManager_Create();

// 从阈值创建Mask
int maskIdx = MaskManager_CreateFromThreshold(
    manager, volume, minThreshold, maxThreshold, "MaskName");

// 获取统计信息
int voxels = MaskManager_CalculateVolume(manager, maskIdx);
float cx, cy, cz;
MaskManager_CalculateCentroid(manager, maskIdx, &cx, &cy, &cz);

// 计算直方图
int histogram[256];
uint16_t minVal, maxVal;
MaskManager_CalculateHistogram(manager, maskIdx, volumeData, 
    width, height, depth, histogram, 256, &minVal, &maxVal);

// 形态学操作
int erodedIdx = MaskManager_Erode(manager, maskIdx, radius, "Eroded");
int dilatedIdx = MaskManager_Dilate(manager, maskIdx, radius, "Dilated");

// 布尔运算
int unionIdx = MaskManager_Union(manager, maskA, maskB, "Union");
int diffIdx = MaskManager_Difference(manager, maskA, maskB, "Diff");

// 连通域分析
int largestIdx = MaskManager_KeepLargestRegion(manager, maskIdx, "Largest");

// 保存和加载
MaskManager_SaveToFile(manager, maskIdx, "mask.bin");
int loadedIdx = MaskManager_LoadFromFile(manager, "mask.bin");

// 清理
MaskManager_Destroy(manager);
```

## 数据结构

### MaskData
```cpp
struct MaskData {
    // 尺寸
    int width, height, depth;
    
    // 二进制数据 (0或255)
    std::vector<uint8_t> data;
    
    // 元数据
    std::string name;
    float color[4];  // RGBA
    float minThreshold, maxThreshold;
    bool visible;
    float opacity;
    
    // 统计信息
    int voxelCount;
    float centerX, centerY, centerZ;
};
```

## 算法说明

### 3D球形结构元素
形态学操作使用球形结构元素：
```
if (dx² + dy² + dz² ≤ radius²) → 包含在结构元素内
```

### 6-连通BFS
FloodFill和连通域分析使用6-连通（上下左右前后）：
```cpp
const int dx[] = {1, -1, 0, 0, 0, 0};
const int dy[] = {0, 0, 1, -1, 0, 0};
const int dz[] = {0, 0, 0, 0, 1, -1};
```

### 自动颜色分配
使用黄金角（137.5°）在HSV色彩空间均匀分布颜色：
```cpp
float hue = fmod(maskCount * 137.5f, 360.0f);
HSV(hue, 0.7, 0.9) → RGB
```

### 直方图统计
只统计Mask内的体素数据：
```cpp
for each voxel in mask:
    if mask[voxel] > 0:
        bin = (volumeData[voxel] - minVal) * (numBins-1) / range
        histogram[bin]++
```

## 性能优化

1. **并行处理**: 可使用OpenMP加速大数据处理
2. **稀疏存储**: 对于稀疏Mask可优化存储方式
3. **缓存统计**: UpdateStatistics()缓存常用统计量
4. **智能指针**: 使用shared_ptr自动管理内存

## 已知限制

1. Mask数据存储为uint8_t（0或255），占用空间较大
2. 连通域分析对大数据集可能较慢
3. 形态学操作的球形结构元素计算密集

## 未来扩展

1. **区域生长**: 基于体数据相似度的智能分割
2. **骨架化**: 提取Mask的拓扑骨架
3. **平滑边界**: 对Mask边界进行平滑处理
4. **STL导出**: 支持导出为3D打印格式
5. **GPU加速**: 使用CUDA加速形态学和连通域操作
6. **交互式绘制**: 在可视化窗口中实时绘制ROI

## 文件说明

- `MaskManager.h`: Mask管理器类声明
- `MaskManager.cpp`: 基础管理和布尔运算实现
- `MaskOperations.cpp`: 形态学、连通域、ROI绘制实现
- `MaskManagerApi.cpp`: C API包装层
- `ImageProcessingApi.h`: 完整的C API声明

## 编译配置

确保项目包含以下文件：
```xml
<ClInclude Include="MaskManager.h" />
<ClCompile Include="MaskManager.cpp" />
<ClCompile Include="MaskManagerApi.cpp" />
<ClCompile Include="MaskOperations.cpp" />
```

## 测试数据要求

- DICOM系列路径: `D:\DICOM\gutoushuju`
- 数据类型: 16位无符号整数 (uint16_t)
- 建议尺寸: 512x512xN (N为层数)
- HU值范围: -1024 ~ 3071
