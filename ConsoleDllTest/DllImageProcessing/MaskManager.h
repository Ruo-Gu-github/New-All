#pragma once

#include <vector>
#include <string>
#include <memory>
#include <cstdint>

// Mask数据结构
struct MaskData {
    int width = 0;
    int height = 0;
    int depth = 0;
    std::vector<uint8_t> data;  // 二进制数据，0或255
    
    // 元数据
    std::string name;           // Mask名称
    float color[4] = {1.0f, 0.0f, 0.0f, 0.5f};  // RGBA颜色（用于显示）
    float minThreshold = 0.0f;  // 生成时的最小阈值
    float maxThreshold = 0.0f;  // 生成时的最大阈值
    bool visible = true;        // 是否可见
    float opacity = 0.5f;       // 不透明度
    
    // 统计信息
    int voxelCount = 0;         // 体素数量
    float centerX = 0.0f;       // 质心X
    float centerY = 0.0f;       // 质心Y
    float centerZ = 0.0f;       // 质心Z
    
    MaskData() = default;
    MaskData(int w, int h, int d) 
        : width(w), height(h), depth(d) {
        data.resize(w * h * d, 0);
    }
    
    // 获取体素值（边界安全）
    uint8_t Get(int x, int y, int z) const {
        if (x < 0 || x >= width || y < 0 || y >= height || z < 0 || z >= depth)
            return 0;
        return data[z * width * height + y * width + x];
    }
    
    // 设置体素值
    void Set(int x, int y, int z, uint8_t value) {
        if (x < 0 || x >= width || y < 0 || y >= height || z < 0 || z >= depth)
            return;
        data[z * width * height + y * width + x] = value;
    }
    
    // 更新统计信息
    void UpdateStatistics();
};

// Mask管理器
class MaskManager {
public:
    MaskManager() = default;
    ~MaskManager() = default;
    
    // ==================== Mask创建 ====================
    
    /// 从阈值生成Mask
    int CreateFromThreshold(const uint16_t* volumeData, int width, int height, int depth,
                           float minThreshold, float maxThreshold,
                           const char* name = "Mask");
    
    /// 创建空白Mask
    int CreateEmpty(int width, int height, int depth, const char* name = "Empty Mask");
    
    /// 从现有Mask复制
    int Clone(int maskIndex);
    
    // ==================== Mask管理 ====================
    
    /// 获取Mask数量
    int GetMaskCount() const { return static_cast<int>(masks_.size()); }
    
    /// 获取Mask指针
    MaskData* GetMask(int index);
    const MaskData* GetMask(int index) const;
    
    /// 删除Mask
    bool DeleteMask(int index);
    
    /// 清空所有Mask
    void Clear();
    
    /// 设置Mask名称
    void SetMaskName(int index, const char* name);
    
    /// 设置Mask颜色
    void SetMaskColor(int index, float r, float g, float b, float a);
    
    /// 设置Mask可见性
    void SetMaskVisible(int index, bool visible);
    
    /// 设置Mask不透明度
    void SetMaskOpacity(int index, float opacity);
    
    // ==================== 布尔运算 ====================
    
    /// 并集（A ∪ B）
    int Union(int indexA, int indexB, const char* name = "Union");
    
    /// 交集（A ∩ B）
    int Intersection(int indexA, int indexB, const char* name = "Intersection");
    
    /// 差集（A - B）
    int Difference(int indexA, int indexB, const char* name = "Difference");
    
    /// 异或（A ⊕ B）
    int XOR(int indexA, int indexB, const char* name = "XOR");
    
    /// 反转
    int Invert(int index, const char* name = "Inverted");
    
    // ==================== 形态学操作 ====================
    
    /// 膨胀
    int Dilate(int index, int radius = 1, const char* name = "Dilated");
    
    /// 腐蚀
    int Erode(int index, int radius = 1, const char* name = "Eroded");
    
    /// 开运算（先腐蚀后膨胀）
    int Opening(int index, int radius = 1, const char* name = "Opened");
    
    /// 闭运算（先膨胀后腐蚀）
    int Closing(int index, int radius = 1, const char* name = "Closed");
    
    /// 形态学梯度（膨胀-腐蚀）
    int MorphologicalGradient(int index, int radius = 1, const char* name = "Gradient");
    
    // ==================== 连通域操作 ====================
    
    /// FloodFill（从种子点开始）
    int FloodFill(int index, int seedX, int seedY, int seedZ, const char* name = "FloodFill");
    
    /// FloodFill（多个种子点）
    int FloodFillMulti(int index, const int* seedsX, const int* seedsY, const int* seedsZ, 
                      int numSeeds, const char* name = "FloodFill");
    
    /// 获取所有连通域（返回每个连通域作为单独的Mask）
    std::vector<int> GetConnectedComponents(int index);
    
    /// 移除小连通域
    int RemoveSmallRegions(int index, int minVoxels, const char* name = "Filtered");
    
    /// 保留最大连通域
    int KeepLargestRegion(int index, const char* name = "Largest");
    
    /// 填充孔洞
    int FillHoles(int index, const char* name = "Filled");
    
    // ==================== ROI绘制 ====================
    
    /// 在指定切片上绘制矩形
    void DrawRectangle(int index, int z, int x1, int y1, int x2, int y2, bool fill = true);
    
    /// 在指定切片上绘制圆形
    void DrawCircle(int index, int z, int cx, int cy, int radius, bool fill = true);
    
    /// 在指定切片上绘制多边形
    void DrawPolygon(int index, int z, const int* pointsX, const int* pointsY, 
                    int numPoints, bool fill = true);
    
    /// 在指定切片上绘制画笔笔触
    void DrawBrush(int index, int z, int x, int y, int radius);
    
    /// 在指定切片上绘制圆形区域（用于2D画笔工具）
    /// sliceDir: 0=Axial(z), 1=Coronal(y), 2=Sagittal(x)
    void DrawCircle2D(int index, int sliceDir, int sliceIndex, float cx, float cy, float radius);
    
    /// 在指定切片上擦除圆形区域（用于2D橡皮擦工具）
    void EraseCircle2D(int index, int sliceDir, int sliceIndex, float cx, float cy, float radius);
    
    /// 在3D空间中绘制球形画笔（用于实时编辑）
    void DrawBrushSphere3D(int index, float cx, float cy, float cz, float radius);
    
    /// 在3D空间中擦除球形区域（橡皮擦）
    void EraseSphere3D(int index, float cx, float cy, float cz, float radius);
    
    /// 在指定切片上绘制线段
    void DrawLine(int index, int z, int x1, int y1, int x2, int y2);
    
    /// 从ROI生成3D Mask（插值中间层）
    int InterpolateFromROI(int width, int height, int depth, 
                          const std::vector<int>& keySlices,
                          const char* name = "Interpolated");
    
    // ==================== 测量与分析 ====================
    
    /// 计算体积（体素数量）
    int CalculateVolume(int index);
    
    /// 计算质心
    void CalculateCentroid(int index, float* cx, float* cy, float* cz);
    
    /// 计算边界框
    void CalculateBoundingBox(int index, int* minX, int* minY, int* minZ,
                             int* maxX, int* maxY, int* maxZ);
    
    /// 提取边界
    int ExtractBoundary(int index, const char* name = "Boundary");
    
    /// 计算Mask内体数据的直方图
    void CalculateHistogram(int index, const uint16_t* volumeData, int width, int height, int depth,
                           int* histogram, int numBins, uint16_t* minValue, uint16_t* maxValue);
    
    // ==================== 高级操作 ====================
    
    /// 区域生长（基于体数据的相似度）
    int RegionGrowing(const uint16_t* volumeData, int width, int height, int depth,
                     int seedX, int seedY, int seedZ, 
                     float threshold, const char* name = "RegionGrown");
    
    /// 平滑边界
    int SmoothBoundary(int index, int iterations = 1, const char* name = "Smoothed");
    
    /// 骨架化
    int Skeletonize(int index, const char* name = "Skeleton");
    
    // ==================== 导入导出 ====================
    
    /// 保存为二进制文件
    bool SaveToFile(int index, const char* filepath);
    
    /// 从二进制文件加载
    int LoadFromFile(const char* filepath);
    
    /// 导出为STL（用于3D打印）
    bool ExportToSTL(int index, const char* filepath);
    
private:
    std::vector<std::shared_ptr<MaskData>> masks_;
    
    // 辅助函数
    bool IsValidIndex(int index) const;
    std::shared_ptr<MaskData> CreateMaskCopy(const MaskData& source);
};
