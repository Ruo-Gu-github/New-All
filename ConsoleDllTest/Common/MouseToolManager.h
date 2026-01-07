#pragma once

#include <memory>
#include <vector>
#include <string>
#include <functional>

/// 鼠标工具基类
class MouseTool {
public:
    virtual ~MouseTool() = default;

    /// 处理鼠标按下事件
    /// @return true表示事件被处理
    virtual bool OnMouseDown(int x, int y, int button) = 0;

    /// 处理鼠标移动事件
    virtual bool OnMouseMove(int x, int y) = 0;

    /// 处理鼠标释放事件
    virtual bool OnMouseUp(int x, int y, int button) = 0;

    /// 渲染工具（如预览线、圆圈等）
    virtual void Render() = 0;

    /// 获取工具名称
    virtual const char* GetName() const = 0;

    /// 工具是否激活
    bool IsActive() const { return active_; }
    void SetActive(bool active) { active_ = active; }

protected:
    bool active_ = false;
};

/// 测量工具（直线距离）
class MeasureTool : public MouseTool {
public:
    bool OnMouseDown(int x, int y, int button) override;
    bool OnMouseMove(int x, int y) override;
    bool OnMouseUp(int x, int y, int button) override;
    void Render() override;
    const char* GetName() const override { return "Measure"; }

private:
    bool measuring_ = false;
    int startX_ = 0, startY_ = 0;
    int endX_ = 0, endY_ = 0;
};

/// 画笔工具（Mask编辑）
class BrushTool : public MouseTool {
public:
    bool OnMouseDown(int x, int y, int button) override;
    bool OnMouseMove(int x, int y) override;
    bool OnMouseUp(int x, int y, int button) override;
    void Render() override;
    const char* GetName() const override { return "Brush"; }

    void SetBrushSize(int size) { brushSize_ = size; }
    int GetBrushSize() const { return brushSize_; }

private:
    bool drawing_ = false;
    int brushSize_ = 10;
    std::vector<std::pair<int, int>> currentStroke_;
};

/// 橡皮擦工具
class EraserTool : public MouseTool {
public:
    bool OnMouseDown(int x, int y, int button) override;
    bool OnMouseMove(int x, int y) override;
    bool OnMouseUp(int x, int y, int button) override;
    void Render() override;
    const char* GetName() const override { return "Eraser"; }

    void SetEraserSize(int size) { eraserSize_ = size; }

private:
    bool erasing_ = false;
    int eraserSize_ = 10;
};

/// ROI矩形工具
class ROIRectangleTool : public MouseTool {
public:
    bool OnMouseDown(int x, int y, int button) override;
    bool OnMouseMove(int x, int y) override;
    bool OnMouseUp(int x, int y, int button) override;
    void Render() override;
    const char* GetName() const override { return "ROI Rectangle"; }

private:
    bool drawing_ = false;
    int startX_ = 0, startY_ = 0;
    int endX_ = 0, endY_ = 0;
};

/// 鼠标工具管理器 - 统一管理所有鼠标工具
class MouseToolManager {
public:
    MouseToolManager();
    ~MouseToolManager();

    /// 工具类型枚举
    enum ToolType {
        TOOL_NONE = 0,
        TOOL_PAN,           // 平移
        TOOL_ZOOM,          // 缩放
        TOOL_WINDOW_LEVEL,  // 窗宽窗位
        TOOL_MEASURE,       // 测量
        TOOL_BRUSH,         // 画笔
        TOOL_ERASER,        // 橡皮擦
        TOOL_ROI_RECT,      // 矩形ROI
        TOOL_ROI_CIRCLE,    // 圆形ROI
        TOOL_ANGLE          // 角度测量
    };

    /// 设置当前激活的工具
    void SetActiveTool(ToolType type);

    /// 获取当前工具类型
    ToolType GetActiveTool() const { return currentTool_; }

    /// 分发鼠标事件到当前工具
    bool DispatchMouseDown(int x, int y, int button);
    bool DispatchMouseMove(int x, int y);
    bool DispatchMouseUp(int x, int y, int button);

    /// 渲染所有激活的工具
    void RenderAll();

    /// 设置画笔大小
    void SetBrushSize(int size);

    /// 获取画笔大小
    int GetBrushSize() const;

    /// 清空所有工具状态
    void ClearAll();

private:
    ToolType currentTool_ = TOOL_NONE;
    std::vector<std::unique_ptr<MouseTool>> tools_;

    // 工具实例
    std::unique_ptr<MeasureTool> measureTool_;
    std::unique_ptr<BrushTool> brushTool_;
    std::unique_ptr<EraserTool> eraserTool_;
    std::unique_ptr<ROIRectangleTool> roiRectTool_;
};
