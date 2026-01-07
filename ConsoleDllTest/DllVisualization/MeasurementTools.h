#pragma once

#include <vector>
#include <string>
#include <memory>
#include <map>
#include <cmath>

// 点结构
struct Point2D {
    float x, y;
    Point2D() : x(0), y(0) {}
    Point2D(float _x, float _y) : x(_x), y(_y) {}
};

struct Point3D {
    float x, y, z;
    Point3D() : x(0), y(0), z(0) {}
    Point3D(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
};

// 标记位置信息（记录标记所在的切片、方向、旋转等）
struct MarkLocation {
    Point3D center;                // 世界坐标中心点
    int sliceDirection;            // 切片方向 (0=轴向, 1=冠状, 2=矢状)
    int sliceIndex;                // 切片索引
    float rotationX;               // APR 旋转角度 X
    float rotationY;               // APR 旋转角度 Y
    float rotationZ;               // APR 旋转角度 Z
    bool isAPR;                    // 是否是 APR 视图
    
    MarkLocation() : center(), sliceDirection(0), sliceIndex(0),
                     rotationX(0), rotationY(0), rotationZ(0), isAPR(false) {}
};

// Profile 数据（用于直线和曲线的灰度剖面）
struct ProfileData {
    std::vector<float> distances;   // 从起点开始的距离
    std::vector<float> values;      // 对应位置的 CT 值
    
    void clear() {
        distances.clear();
        values.clear();
    }
};

// 测量结果基类
class ToolResult {
public:
    enum ResultType {
        RESULT_NONE = 0,
        RESULT_DISTANCE,        // 距离
        RESULT_ANGLE,           // 角度
        RESULT_AREA,            // 面积
        RESULT_STATISTICS,      // 统计（平均值、方差等）
        RESULT_PROFILE          // Profile 图数据
    };

    virtual ~ToolResult() = default;
    virtual ResultType getType() const = 0;
    virtual std::string toString() const = 0;
};

// 距离结果
class DistanceResult : public ToolResult {
public:
    float distance;
    Point3D startPoint;
    Point3D endPoint;
    
    DistanceResult() : distance(0.0f) {}
    
    ResultType getType() const override { return RESULT_DISTANCE; }
    
    std::string toString() const override {
        char buffer[256];
        sprintf_s(buffer, sizeof(buffer), "Distance: %.2f mm", distance);
        return std::string(buffer);
    }
};

// 角度结果
class AngleResult : public ToolResult {
public:
    float angleDegrees;
    Point3D vertex;
    Point3D point1;
    Point3D point2;
    
    AngleResult() : angleDegrees(0.0f) {}
    
    ResultType getType() const override { return RESULT_ANGLE; }
    
    std::string toString() const override {
        char buffer[256];
        sprintf_s(buffer, sizeof(buffer), "Angle: %.2f degrees", angleDegrees);
        return std::string(buffer);
    }
};

// 面积和统计结果
class AreaStatisticsResult : public ToolResult {
public:
    float area;           // 面积（平方毫米）
    float meanValue;      // 平均 CT 值
    float stdDev;         // 标准差
    float minValue;       // 最小值
    float maxValue;       // 最大值
    int pixelCount;       // 像素数量
    
    AreaStatisticsResult() : area(0), meanValue(0), stdDev(0), 
                             minValue(0), maxValue(0), pixelCount(0) {}
    
    ResultType getType() const override { return RESULT_STATISTICS; }
    
    std::string toString() const override {
        char buffer[512];
        sprintf_s(buffer, sizeof(buffer), "Area: %.2f mm2\nMean: %.2f HU\nStdDev: %.2f\nMin: %.2f\nMax: %.2f\nPixels: %d",
                area, meanValue, stdDev, minValue, maxValue, pixelCount);
        return std::string(buffer);
    }
};

// Profile 结果
class ProfileResult : public ToolResult {
public:
    ProfileData profile;
    
    ResultType getType() const override { return RESULT_PROFILE; }
    
    std::string toString() const override {
        char buffer[256];
        sprintf_s(buffer, sizeof(buffer), "Profile: %zu points", profile.distances.size());
        return std::string(buffer);
    }
};

// 鼠标事件类型
enum MouseEventType {
    MOUSE_DOWN,
    MOUSE_MOVE,
    MOUSE_UP,
    MOUSE_DOUBLE_CLICK
};

// 鼠标按钮
enum MouseButton {
    MOUSE_LEFT = 0,
    MOUSE_RIGHT = 1,
    MOUSE_MIDDLE = 2
};

// 测量工具类型
enum ToolType {
    TOOL_NONE = 0,
    TOOL_LINE,              // 直线
    TOOL_ANGLE,             // 角度
    TOOL_BEZIER,            // 贝塞尔曲线
    TOOL_CLOSED_BEZIER,     // 封闭贝塞尔曲线
    TOOL_RECTANGLE,         // 矩形
    TOOL_CIRCLE,            // 圆形
    TOOL_FREEHAND,          // 任意封闭曲线
    TOOL_WINDOW_LEVEL,      // 窗宽窗位
    TOOL_SLICE_SWITCH       // 切片切换
};

// 测量工具基类
class MeasurementTool {
protected:
    ToolType m_type;
    std::vector<Point2D> m_points;      // 2D 屏幕坐标点
    std::vector<Point3D> m_worldPoints; // 3D 世界坐标点
    bool m_isComplete;
    bool m_isActive;
    MarkLocation m_location;            // 标记位置信息
    std::shared_ptr<ToolResult> m_result;
    std::string m_name;                 // 标记名称

public:
    MeasurementTool(ToolType type) 
        : m_type(type), m_isComplete(false), m_isActive(false) {}
    
    virtual ~MeasurementTool() = default;

    // 处理鼠标事件
    virtual bool handleMouseEvent(MouseEventType eventType, MouseButton button, 
                                   const Point2D& screenPos, const Point3D& worldPos) = 0;
    
    // 计算测量结果
    virtual void calculateResult(void* volumeData, int width, int height, int depth,
                                  float spacingX, float spacingY, float spacingZ) = 0;
    
    // 渲染工具（绘制到屏幕）
    virtual void render() = 0;
    
    // 重置工具
    virtual void reset() {
        m_points.clear();
        m_worldPoints.clear();
        m_isComplete = false;
        m_result.reset();
    }
    
    // Getter 方法
    ToolType getType() const { return m_type; }
    bool isComplete() const { return m_isComplete; }
    bool isActive() const { return m_isActive; }
    void setActive(bool active) { m_isActive = active; }
    
    const std::vector<Point2D>& getPoints() const { return m_points; }
    const std::vector<Point3D>& getWorldPoints() const { return m_worldPoints; }
    
    std::shared_ptr<ToolResult> getResult() const { return m_result; }
    
    const MarkLocation& getLocation() const { return m_location; }
    void setLocation(const MarkLocation& loc) { m_location = loc; }
    
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    
    // 序列化和反序列化
    virtual std::string serialize() const;
    virtual bool deserialize(const std::string& data);
};

// 直线工具
class LineTool : public MeasurementTool {
public:
    LineTool() : MeasurementTool(TOOL_LINE) {}
    
    bool handleMouseEvent(MouseEventType eventType, MouseButton button, 
                         const Point2D& screenPos, const Point3D& worldPos) override;
    void calculateResult(void* volumeData, int width, int height, int depth,
                        float spacingX, float spacingY, float spacingZ) override;
    void render() override;
};

// 角度工具（需要三个点：两条线的交点和两个端点）
class AngleTool : public MeasurementTool {
public:
    AngleTool() : MeasurementTool(TOOL_ANGLE) {}
    
    bool handleMouseEvent(MouseEventType eventType, MouseButton button, 
                         const Point2D& screenPos, const Point3D& worldPos) override;
    void calculateResult(void* volumeData, int width, int height, int depth,
                        float spacingX, float spacingY, float spacingZ) override;
    void render() override;
};

// 贝塞尔曲线工具
class BezierTool : public MeasurementTool {
private:
    std::vector<Point2D> m_curvePoints; // 插值后的曲线点
    
public:
    BezierTool() : MeasurementTool(TOOL_BEZIER) {}
    
    bool handleMouseEvent(MouseEventType eventType, MouseButton button, 
                         const Point2D& screenPos, const Point3D& worldPos) override;
    void calculateResult(void* volumeData, int width, int height, int depth,
                        float spacingX, float spacingY, float spacingZ) override;
    void render() override;
    
private:
    void generateCurve();
    Point2D calculateBezierPoint(float t);
};

// 封闭贝塞尔曲线工具
class ClosedBezierTool : public MeasurementTool {
private:
    std::vector<Point2D> m_curvePoints;
    
public:
    ClosedBezierTool() : MeasurementTool(TOOL_CLOSED_BEZIER) {}
    
    bool handleMouseEvent(MouseEventType eventType, MouseButton button, 
                         const Point2D& screenPos, const Point3D& worldPos) override;
    void calculateResult(void* volumeData, int width, int height, int depth,
                        float spacingX, float spacingY, float spacingZ) override;
    void render() override;
    
private:
    void generateCurve();
    Point2D calculateBezierPoint(float t);
};

// 矩形工具
class RectangleTool : public MeasurementTool {
public:
    RectangleTool() : MeasurementTool(TOOL_RECTANGLE) {}
    
    bool handleMouseEvent(MouseEventType eventType, MouseButton button, 
                         const Point2D& screenPos, const Point3D& worldPos) override;
    void calculateResult(void* volumeData, int width, int height, int depth,
                        float spacingX, float spacingY, float spacingZ) override;
    void render() override;
};

// 圆形工具
class CircleTool : public MeasurementTool {
private:
    float m_radius;
    Point2D m_center;
    
public:
    CircleTool() : MeasurementTool(TOOL_CIRCLE), m_radius(0) {}
    
    bool handleMouseEvent(MouseEventType eventType, MouseButton button, 
                         const Point2D& screenPos, const Point3D& worldPos) override;
    void calculateResult(void* volumeData, int width, int height, int depth,
                        float spacingX, float spacingY, float spacingZ) override;
    void render() override;
};

// 任意封闭曲线工具（自由绘制）
class FreehandTool : public MeasurementTool {
public:
    FreehandTool() : MeasurementTool(TOOL_FREEHAND) {}
    
    bool handleMouseEvent(MouseEventType eventType, MouseButton button, 
                         const Point2D& screenPos, const Point3D& worldPos) override;
    void calculateResult(void* volumeData, int width, int height, int depth,
                        float spacingX, float spacingY, float spacingZ) override;
    void render() override;
};

// 窗宽窗位工具
class WindowLevelTool : public MeasurementTool {
private:
    Point2D m_startPos;
    float m_windowWidth;
    float m_windowCenter;
    
public:
    WindowLevelTool() : MeasurementTool(TOOL_WINDOW_LEVEL), 
                        m_windowWidth(400), m_windowCenter(40) {}
    
    bool handleMouseEvent(MouseEventType eventType, MouseButton button, 
                         const Point2D& screenPos, const Point3D& worldPos) override;
    void calculateResult(void* volumeData, int width, int height, int depth,
                        float spacingX, float spacingY, float spacingZ) override;
    void render() override;
    
    float getWindowWidth() const { return m_windowWidth; }
    float getWindowCenter() const { return m_windowCenter; }
    void setWindowLevel(float width, float center) {
        m_windowWidth = width;
        m_windowCenter = center;
    }
};

// 切片切换工具
class SliceSwitchTool : public MeasurementTool {
private:
    Point2D m_startPos;
    int m_startSlice;
    
public:
    SliceSwitchTool() : MeasurementTool(TOOL_SLICE_SWITCH), m_startSlice(0) {}
    
    bool handleMouseEvent(MouseEventType eventType, MouseButton button, 
                         const Point2D& screenPos, const Point3D& worldPos) override;
    void calculateResult(void* volumeData, int width, int height, int depth,
                        float spacingX, float spacingY, float spacingZ) override;
    void render() override;
    
    void setStartSlice(int slice) { m_startSlice = slice; }
};

// 工具管理器
class ToolManager {
private:
    std::shared_ptr<MeasurementTool> m_currentTool;
    std::vector<std::shared_ptr<MeasurementTool>> m_completedTools;
    ToolType m_selectedToolType;
    
    // 体数据信息（用于计算）
    void* m_volumeData;
    int m_width, m_height, m_depth;
    float m_spacingX, m_spacingY, m_spacingZ;
    
    // 当前视图信息
    MarkLocation m_currentLocation;
    
public:
    ToolManager();
    ~ToolManager();
    
    // 设置体数据
    void setVolumeData(void* data, int width, int height, int depth,
                      float spacingX, float spacingY, float spacingZ);
    
    // 设置当前位置信息
    void setCurrentLocation(const MarkLocation& loc) { m_currentLocation = loc; }
    const MarkLocation& getCurrentLocation() const { return m_currentLocation; }
    
    // 切换工具
    void selectTool(ToolType type);
    ToolType getCurrentToolType() const { return m_selectedToolType; }
    
    // 鼠标事件处理
    bool handleMouseEvent(MouseEventType eventType, MouseButton button, 
                         const Point2D& screenPos, const Point3D& worldPos);
    
    // 渲染所有工具
    void renderAll();
    
    // 获取已完成的工具
    const std::vector<std::shared_ptr<MeasurementTool>>& getCompletedTools() const {
        return m_completedTools;
    }
    
    // 清除所有工具
    void clearAll();
    
    // 删除指定工具
    void deleteTool(int index);
    
    // 保存和加载标记
    bool saveMarksToFile(const std::string& filename);
    bool loadMarksFromFile(const std::string& filename);
    
    // 导出结果到控制台
    void printResults();
};

