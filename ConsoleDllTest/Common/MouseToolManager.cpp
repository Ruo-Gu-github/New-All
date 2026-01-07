#include "MouseToolManager.h"
#include <algorithm>

// ============================================================================
// MouseTool 基类实现
// ============================================================================

MouseTool::MouseTool(const std::string& name) 
    : m_name(name), m_isActive(false) {
}

MouseTool::~MouseTool() {
}

void MouseTool::SetActive(bool active) {
    m_isActive = active;
    if (!active) {
        OnDeactivate();
    }
}

void MouseTool::OnDeactivate() {
    // 默认实现：清理临时状态
}

// ============================================================================
// MeasureTool - 测量工具
// ============================================================================

MeasureTool::MeasureTool() 
    : MouseTool("Measure"), 
      m_measuring(false),
      m_startX(0), m_startY(0),
      m_endX(0), m_endY(0) {
}

bool MeasureTool::OnMouseDown(int x, int y, int button) {
    if (button == 0) {  // 左键
        m_measuring = true;
        m_startX = x;
        m_startY = y;
        m_endX = x;
        m_endY = y;
        return true;
    }
    return false;
}

bool MeasureTool::OnMouseMove(int x, int y) {
    if (m_measuring) {
        m_endX = x;
        m_endY = y;
        return true;
    }
    return false;
}

bool MeasureTool::OnMouseUp(int x, int y, int button) {
    if (button == 0 && m_measuring) {
        m_endX = x;
        m_endY = y;
        m_measuring = false;
        
        // 计算距离并保存测量结果
        double dx = m_endX - m_startX;
        double dy = m_endY - m_startY;
        double distance = std::sqrt(dx * dx + dy * dy);
        
        m_measurements.push_back({
            m_startX, m_startY,
            m_endX, m_endY,
            distance
        });
        
        return true;
    }
    return false;
}

void MeasureTool::Render() {
    // 渲染所有已保存的测量线
    for (const auto& measurement : m_measurements) {
        RenderLine(measurement.startX, measurement.startY,
                   measurement.endX, measurement.endY);
        RenderText(measurement.endX, measurement.endY,
                   std::to_string(measurement.distance) + " pixels");
    }
    
    // 渲染当前正在测量的线
    if (m_measuring) {
        RenderLine(m_startX, m_startY, m_endX, m_endY);
    }
}

void MeasureTool::RenderLine(int x1, int y1, int x2, int y2) {
    // TODO: 实际的OpenGL渲染代码
    // glBegin(GL_LINES);
    // glVertex2i(x1, y1);
    // glVertex2i(x2, y2);
    // glEnd();
}

void MeasureTool::RenderText(int x, int y, const std::string& text) {
    // TODO: 文字渲染
}

const std::vector<MeasureTool::Measurement>& MeasureTool::GetMeasurements() const {
    return m_measurements;
}

void MeasureTool::ClearMeasurements() {
    m_measurements.clear();
}

// ============================================================================
// BrushTool - 画笔工具
// ============================================================================

BrushTool::BrushTool() 
    : MouseTool("Brush"),
      m_drawing(false),
      m_brushSize(5),
      m_brushValue(255) {
}

bool BrushTool::OnMouseDown(int x, int y, int button) {
    if (button == 0) {
        m_drawing = true;
        DrawAt(x, y);
        return true;
    }
    return false;
}

bool BrushTool::OnMouseMove(int x, int y) {
    if (m_drawing) {
        DrawAt(x, y);
        return true;
    }
    return false;
}

bool BrushTool::OnMouseUp(int x, int y, int button) {
    if (button == 0 && m_drawing) {
        m_drawing = false;
        return true;
    }
    return false;
}

void BrushTool::Render() {
    // 画笔工具一般直接修改数据，不需要单独渲染
}

void BrushTool::SetBrushSize(int size) {
    m_brushSize = std::max(1, size);
}

void BrushTool::SetBrushValue(int value) {
    m_brushValue = std::clamp(value, 0, 255);
}

void BrushTool::DrawAt(int x, int y) {
    // TODO: 实际的绘制逻辑
    // 需要访问mask数据并修改像素值
    // 例如: maskManager->SetPixel(x, y, m_brushValue, m_brushSize);
}

// ============================================================================
// EraserTool - 橡皮擦工具
// ============================================================================

EraserTool::EraserTool() 
    : MouseTool("Eraser"),
      m_erasing(false),
      m_eraserSize(10) {
}

bool EraserTool::OnMouseDown(int x, int y, int button) {
    if (button == 0) {
        m_erasing = true;
        EraseAt(x, y);
        return true;
    }
    return false;
}

bool EraserTool::OnMouseMove(int x, int y) {
    if (m_erasing) {
        EraseAt(x, y);
        return true;
    }
    return false;
}

bool EraserTool::OnMouseUp(int x, int y, int button) {
    if (button == 0 && m_erasing) {
        m_erasing = false;
        return true;
    }
    return false;
}

void EraserTool::Render() {
    // 橡皮擦工具直接修改数据，不需要单独渲染
}

void EraserTool::SetEraserSize(int size) {
    m_eraserSize = std::max(1, size);
}

void EraserTool::EraseAt(int x, int y) {
    // TODO: 实际的擦除逻辑
    // maskManager->SetPixel(x, y, 0, m_eraserSize);
}

// ============================================================================
// ROIRectangleTool - ROI矩形工具
// ============================================================================

ROIRectangleTool::ROIRectangleTool() 
    : MouseTool("ROI Rectangle"),
      m_drawing(false),
      m_startX(0), m_startY(0),
      m_endX(0), m_endY(0) {
}

bool ROIRectangleTool::OnMouseDown(int x, int y, int button) {
    if (button == 0) {
        m_drawing = true;
        m_startX = x;
        m_startY = y;
        m_endX = x;
        m_endY = y;
        return true;
    }
    return false;
}

bool ROIRectangleTool::OnMouseMove(int x, int y) {
    if (m_drawing) {
        m_endX = x;
        m_endY = y;
        return true;
    }
    return false;
}

bool ROIRectangleTool::OnMouseUp(int x, int y, int button) {
    if (button == 0 && m_drawing) {
        m_endX = x;
        m_endY = y;
        m_drawing = false;
        
        // 保存ROI区域
        int x1 = std::min(m_startX, m_endX);
        int y1 = std::min(m_startY, m_endY);
        int x2 = std::max(m_startX, m_endX);
        int y2 = std::max(m_startY, m_endY);
        
        m_rois.push_back({ x1, y1, x2 - x1, y2 - y1 });
        
        return true;
    }
    return false;
}

void ROIRectangleTool::Render() {
    // 渲染所有已保存的ROI
    for (const auto& roi : m_rois) {
        RenderRectangle(roi.x, roi.y, roi.width, roi.height);
    }
    
    // 渲染当前正在绘制的矩形
    if (m_drawing) {
        int x = std::min(m_startX, m_endX);
        int y = std::min(m_startY, m_endY);
        int w = std::abs(m_endX - m_startX);
        int h = std::abs(m_endY - m_startY);
        RenderRectangle(x, y, w, h);
    }
}

void ROIRectangleTool::RenderRectangle(int x, int y, int width, int height) {
    // TODO: 实际的OpenGL渲染代码
    // glBegin(GL_LINE_LOOP);
    // glVertex2i(x, y);
    // glVertex2i(x + width, y);
    // glVertex2i(x + width, y + height);
    // glVertex2i(x, y + height);
    // glEnd();
}

const std::vector<ROIRectangleTool::ROI>& ROIRectangleTool::GetROIs() const {
    return m_rois;
}

void ROIRectangleTool::ClearROIs() {
    m_rois.clear();
}

// ============================================================================
// MouseToolManager - 工具管理器
// ============================================================================

MouseToolManager::MouseToolManager() 
    : m_activeTool(TOOL_NONE) {
    // 创建所有工具实例
    m_tools[TOOL_MEASURE] = std::make_unique<MeasureTool>();
    m_tools[TOOL_BRUSH] = std::make_unique<BrushTool>();
    m_tools[TOOL_ERASER] = std::make_unique<EraserTool>();
    m_tools[TOOL_ROI_RECT] = std::make_unique<ROIRectangleTool>();
}

MouseToolManager::~MouseToolManager() {
}

void MouseToolManager::SetActiveTool(ToolType type) {
    // 停用当前工具
    if (m_activeTool != TOOL_NONE && m_tools.count(m_activeTool) > 0) {
        m_tools[m_activeTool]->SetActive(false);
    }
    
    // 激活新工具
    m_activeTool = type;
    if (type != TOOL_NONE && m_tools.count(type) > 0) {
        m_tools[type]->SetActive(true);
    }
}

ToolType MouseToolManager::GetActiveTool() const {
    return m_activeTool;
}

bool MouseToolManager::DispatchMouseDown(int x, int y, int button) {
    if (m_activeTool != TOOL_NONE && m_tools.count(m_activeTool) > 0) {
        return m_tools[m_activeTool]->OnMouseDown(x, y, button);
    }
    return false;
}

bool MouseToolManager::DispatchMouseMove(int x, int y) {
    if (m_activeTool != TOOL_NONE && m_tools.count(m_activeTool) > 0) {
        return m_tools[m_activeTool]->OnMouseMove(x, y);
    }
    return false;
}

bool MouseToolManager::DispatchMouseUp(int x, int y, int button) {
    if (m_activeTool != TOOL_NONE && m_tools.count(m_activeTool) > 0) {
        return m_tools[m_activeTool]->OnMouseUp(x, y, button);
    }
    return false;
}

void MouseToolManager::RenderAll() {
    // 渲染所有工具的可视化内容
    for (auto& pair : m_tools) {
        pair.second->Render();
    }
}

MouseTool* MouseToolManager::GetTool(ToolType type) {
    if (m_tools.count(type) > 0) {
        return m_tools[type].get();
    }
    return nullptr;
}
