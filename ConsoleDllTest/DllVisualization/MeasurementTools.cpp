#include "pch.h"
#include "MeasurementTools.h"
#include <cstdio>
#include <fstream>
#include <sstream>

// NOTE: This implementation provides a working baseline for the system:
// - Full class declarations are in MeasurementTools.h
// - Here we implement basic behavior for MeasurementTool, LineTool and ToolManager
// - Serialization is a simple text format (one tool per block). Extend to JSON later if needed.

// ------------------ MeasurementTool base implementations ------------------
std::string MeasurementTool::serialize() const {
    std::ostringstream ss;
    ss << "Type=" << (int)m_type << "\n";
    ss << "Name=" << m_name << "\n";
    ss << "SliceDirection=" << m_location.sliceDirection << "\n";
    ss << "SliceIndex=" << m_location.sliceIndex << "\n";
    ss << "Center=" << m_location.center.x << "," << m_location.center.y << "," << m_location.center.z << "\n";
    ss << "Rotation=" << m_location.rotationX << "," << m_location.rotationY << "," << m_location.rotationZ << "\n";
    ss << "IsAPR=" << (m_location.isAPR ? 1 : 0) << "\n";
    ss << "Points2DCount=" << m_points.size() << "\n";
    for (const auto &p : m_points) {
        ss << p.x << "," << p.y << "\n";
    }
    ss << "WorldPointsCount=" << m_worldPoints.size() << "\n";
    for (const auto &p : m_worldPoints) {
        ss << p.x << "," << p.y << "," << p.z << "\n";
    }
    ss << "---END---\n";
    return ss.str();
}

bool MeasurementTool::deserialize(const std::string& data) {
    // A forgiving parser for the simple format above. It will populate name, location and points.
    std::istringstream ss(data);
    std::string line;
    m_points.clear();
    m_worldPoints.clear();
    while (std::getline(ss, line)) {
        if (line.empty()) continue;
        if (line == "---END---") break;
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + 1);
        if (key == "Name") m_name = val;
        else if (key == "SliceDirection") m_location.sliceDirection = std::stoi(val);
        else if (key == "SliceIndex") m_location.sliceIndex = std::stoi(val);
        else if (key == "Center") {
            float x=0,y=0,z=0; sscanf_s(val.c_str(), "%f,%f,%f", &x,&y,&z);
            m_location.center = Point3D(x,y,z);
        } else if (key == "Rotation") {
            sscanf_s(val.c_str(), "%f,%f,%f", &m_location.rotationX, &m_location.rotationY, &m_location.rotationZ);
        } else if (key == "IsAPR") {
            m_location.isAPR = (std::stoi(val) != 0);
        } else if (key == "Points2DCount") {
            int cnt = std::stoi(val);
            for (int i=0;i<cnt;i++){
                std::string pline;
                if (!std::getline(ss, pline)) break;
                float x=0,y=0; sscanf_s(pline.c_str(), "%f,%f", &x,&y);
                m_points.emplace_back(x,y);
            }
        } else if (key == "WorldPointsCount") {
            int cnt = std::stoi(val);
            for (int i=0;i<cnt;i++){
                std::string pline;
                if (!std::getline(ss, pline)) break;
                float x=0,y=0,z=0; sscanf_s(pline.c_str(), "%f,%f,%f", &x,&y,&z);
                m_worldPoints.emplace_back(x,y,z);
            }
        }
    }
    // If we deserialized points, consider it complete
    if (!m_points.empty() || !m_worldPoints.empty()) m_isComplete = true;
    return true;
}

// ------------------ LineTool implementation ------------------

bool LineTool::handleMouseEvent(MouseEventType eventType, MouseButton button, const Point2D& screenPos, const Point3D& worldPos) {
    // Basic interaction model:
    // - Left mouse down: add a point (start)
    // - Mouse move while not complete: update last point for live feedback
    // - Left mouse up: finalize second point and mark complete
    if (button != MOUSE_LEFT) return false;
    if (eventType == MOUSE_DOWN) {
        m_points.push_back(screenPos);
        m_worldPoints.push_back(worldPos);
        m_isActive = true;
        m_isComplete = false;
        return true;
    } else if (eventType == MOUSE_MOVE) {
        if (m_isActive && !m_points.empty()) {
            // update last point for live preview
            if (m_points.size() == 1) {
                // keep a temporary second point
                if (m_points.size() < 2) m_points.push_back(screenPos);
                else m_points.back() = screenPos;
                if (m_worldPoints.size() < 2) m_worldPoints.push_back(worldPos);
                else m_worldPoints.back() = worldPos;
            } else {
                m_points.back() = screenPos;
                m_worldPoints.back() = worldPos;
            }
            return true;
        }
    } else if (eventType == MOUSE_UP) {
        if (m_isActive) {
            // finalize
            if (m_points.size() == 1) {
                m_points.push_back(screenPos);
                m_worldPoints.push_back(worldPos);
            } else {
                m_points.back() = screenPos;
                m_worldPoints.back() = worldPos;
            }
            m_isComplete = true;
            m_isActive = false;
            return true;
        }
    }
    return false;
}

void LineTool::calculateResult(void* volumeData, int width, int height, int depth, float spacingX, float spacingY, float spacingZ) {
    if (m_worldPoints.size() < 2) return;
    const Point3D &a = m_worldPoints[0];
    const Point3D &b = m_worldPoints[1];
    float dx = (b.x - a.x);
    float dy = (b.y - a.y);
    float dz = (b.z - a.z);
    // If world units are voxel indices, convert using spacing; else assume mm already. We'll multiply by spacing here
    float sx = dx * spacingX;
    float sy = dy * spacingY;
    float sz = dz * spacingZ;
    float dist = std::sqrt(sx*sx + sy*sy + sz*sz);
    auto res = std::make_shared<DistanceResult>();
    res->distance = dist;
    res->startPoint = a;
    res->endPoint = b;
    m_result = res;

    // Profile sampling would require interpretation of volumeData. Provide a placeholder for now.
    // TODO: if volumeData is a known format (e.g., int16* array), sample along the line and fill ProfileResult
}

void LineTool::render() {
    // Rendering depends on the host's GL/UI. We provide no-op stub here.
    // The host should read getPoints()/getWorldPoints() and draw accordingly.
}

// ------------------ AngleTool stubs ------------------
bool AngleTool::handleMouseEvent(MouseEventType eventType, MouseButton button, const Point2D& screenPos, const Point3D& worldPos) {
    // Basic skeleton: collect three points and compute angle in calculateResult
    if (button != MOUSE_LEFT) return false;
    if (eventType == MOUSE_DOWN) {
        m_points.push_back(screenPos);
        m_worldPoints.push_back(worldPos);
        if (m_worldPoints.size() >= 3) {
            m_isComplete = true;
        }
        return true;
    }
    return false;
}
void AngleTool::calculateResult(void* volumeData, int width, int height, int depth, float spacingX, float spacingY, float spacingZ) {
    if (m_worldPoints.size() < 3) return;
    Point3D v = m_worldPoints[1]; // vertex
    Point3D p1 = m_worldPoints[0];
    Point3D p2 = m_worldPoints[2];
    float ax = p1.x - v.x, ay = p1.y - v.y, az = p1.z - v.z;
    float bx = p2.x - v.x, by = p2.y - v.y, bz = p2.z - v.z;
    // scale by spacing
    ax *= spacingX; ay *= spacingY; az *= spacingZ;
    bx *= spacingX; by *= spacingY; bz *= spacingZ;
    float adotb = ax*bx + ay*by + az*bz;
    float alen = std::sqrt(ax*ax + ay*ay + az*az);
    float blen = std::sqrt(bx*bx + by*by + bz*bz);
    float cosv = 1.0f;
    if (alen > 1e-6f && blen > 1e-6f) cosv = adotb / (alen * blen);
    if (cosv < -1.0f) cosv = -1.0f; if (cosv > 1.0f) cosv = 1.0f;
    float angleDeg = std::acos(cosv) * 180.0f / 3.14159265358979f;
    auto res = std::make_shared<AngleResult>();
    res->angleDegrees = angleDeg;
    res->vertex = v; res->point1 = p1; res->point2 = p2;
    m_result = res;
}
void AngleTool::render() {}

// ------------------ Bezier / ClosedBezier / Rectangle / Circle / Freehand stubs ------------------
bool BezierTool::handleMouseEvent(MouseEventType, MouseButton, const Point2D&, const Point3D&) { return false; }
void BezierTool::calculateResult(void*, int,int,int,float,float,float) {}
void BezierTool::render() {}
void BezierTool::generateCurve() {}
Point2D BezierTool::calculateBezierPoint(float) { return Point2D(); }

bool ClosedBezierTool::handleMouseEvent(MouseEventType, MouseButton, const Point2D&, const Point3D&) { return false; }
void ClosedBezierTool::calculateResult(void*, int,int,int,float,float,float) {}
void ClosedBezierTool::render() {}
void ClosedBezierTool::generateCurve() {}
Point2D ClosedBezierTool::calculateBezierPoint(float) { return Point2D(); }

bool RectangleTool::handleMouseEvent(MouseEventType, MouseButton, const Point2D&, const Point3D&) { return false; }
void RectangleTool::calculateResult(void*, int,int,int,float,float,float) {}
void RectangleTool::render() {}

bool CircleTool::handleMouseEvent(MouseEventType, MouseButton, const Point2D&, const Point3D&) { return false; }
void CircleTool::calculateResult(void*, int,int,int,float,float,float) {}
void CircleTool::render() {}

bool FreehandTool::handleMouseEvent(MouseEventType eventType, MouseButton button, const Point2D& screenPos, const Point3D& worldPos) {
    // Stub for freehand drawing - to be implemented
    return false;
}
void FreehandTool::calculateResult(void*, int,int,int,float,float,float) {}
void FreehandTool::render() {}

// ------------------ WindowLevelTool stub ------------------
bool WindowLevelTool::handleMouseEvent(MouseEventType eventType, MouseButton button, const Point2D& screenPos, const Point3D& worldPos) {
    // Typical behavior: mouse drag adjusts window/level. We'll provide a minimal behavior.
    if (button != MOUSE_RIGHT && button != MOUSE_LEFT) return false;
    if (eventType == MOUSE_DOWN) {
        m_startPos = screenPos;
        m_isActive = true;
        return true;
    } else if (eventType == MOUSE_MOVE) {
        if (!m_isActive) return false;
        float dx = screenPos.x - m_startPos.x;
        float dy = screenPos.y - m_startPos.y;
        // change window/level proportional to drag
        m_windowWidth += dx * 1.0f;
        m_windowCenter += -dy * 1.0f;
        m_startPos = screenPos;
        return true;
    } else if (eventType == MOUSE_UP) {
        m_isActive = false;
        m_isComplete = true;
        return true;
    }
    return false;
}
void WindowLevelTool::calculateResult(void*, int,int,int,float,float,float) {}
void WindowLevelTool::render() {}

// ------------------ SliceSwitchTool stub ------------------
bool SliceSwitchTool::handleMouseEvent(MouseEventType eventType, MouseButton button, const Point2D& screenPos, const Point3D& worldPos) {
    if (button != MOUSE_MIDDLE && button != MOUSE_LEFT) return false;
    if (eventType == MOUSE_DOWN) {
        m_startPos = screenPos;
        m_isActive = true;
        return true;
    } else if (eventType == MOUSE_MOVE) {
        // host should interpret movement to change slice. We only mark active here.
        return m_isActive;
    } else if (eventType == MOUSE_UP) {
        m_isActive = false;
        m_isComplete = true;
        return true;
    }
    return false;
}
void SliceSwitchTool::calculateResult(void*, int,int,int,float,float,float) {}
void SliceSwitchTool::render() {}

// ------------------ ToolManager implementation ------------------
ToolManager::ToolManager()
    : m_currentTool(nullptr), m_selectedToolType(TOOL_NONE),
      m_volumeData(nullptr), m_width(0), m_height(0), m_depth(0), m_spacingX(1.0f), m_spacingY(1.0f), m_spacingZ(1.0f) {}

ToolManager::~ToolManager() {}

void ToolManager::setVolumeData(void* data, int width, int height, int depth, float spacingX, float spacingY, float spacingZ) {
    m_volumeData = data;
    m_width = width; m_height = height; m_depth = depth;
    m_spacingX = spacingX; m_spacingY = spacingY; m_spacingZ = spacingZ;
}

void ToolManager::selectTool(ToolType type) {
    m_selectedToolType = type;
    m_currentTool.reset();
    switch (type) {
        case TOOL_LINE: m_currentTool = std::make_shared<LineTool>(); break;
        case TOOL_ANGLE: m_currentTool = std::make_shared<AngleTool>(); break;
        case TOOL_BEZIER: m_currentTool = std::make_shared<BezierTool>(); break;
        case TOOL_CLOSED_BEZIER: m_currentTool = std::make_shared<ClosedBezierTool>(); break;
        case TOOL_RECTANGLE: m_currentTool = std::make_shared<RectangleTool>(); break;
        case TOOL_CIRCLE: m_currentTool = std::make_shared<CircleTool>(); break;
        case TOOL_FREEHAND: m_currentTool = std::make_shared<FreehandTool>(); break;
        case TOOL_WINDOW_LEVEL: m_currentTool = std::make_shared<WindowLevelTool>(); break;
        case TOOL_SLICE_SWITCH: m_currentTool = std::make_shared<SliceSwitchTool>(); break;
        default: m_currentTool = nullptr; break;
    }
    if (m_currentTool) {
        m_currentTool->setLocation(m_currentLocation);
    }
}

bool ToolManager::handleMouseEvent(MouseEventType eventType, MouseButton button, const Point2D& screenPos, const Point3D& worldPos) {
    if (!m_currentTool) return false;
    bool handled = m_currentTool->handleMouseEvent(eventType, button, screenPos, worldPos);
    if (handled) {
        if (m_currentTool->isComplete()) {
            // finalize: compute result and move to completed list
            m_currentTool->calculateResult(m_volumeData, m_width, m_height, m_depth, m_spacingX, m_spacingY, m_spacingZ);
            m_completedTools.push_back(m_currentTool);
            // reset selection so next selectTool needs to be called or keep current tool for multi-measurement
            // For now we keep current tool instance so user can create multiple marks with same tool; create a fresh instance instead
            selectTool(m_selectedToolType);
        }
    }
    return handled;
}

void ToolManager::renderAll() {
    // Host renderer should call each tool's render method. We just iterate to allow extension.
    if (m_currentTool) m_currentTool->render();
    for (auto &t : m_completedTools) t->render();
}

void ToolManager::clearAll() {
    m_completedTools.clear();
    m_currentTool.reset();
}

void ToolManager::deleteTool(int index) {
    if (index < 0 || index >= (int)m_completedTools.size()) return;
    m_completedTools.erase(m_completedTools.begin() + index);
}

bool ToolManager::saveMarksToFile(const std::string& filename) {
    std::ofstream ofs(filename, std::ios::out | std::ios::trunc);
    if (!ofs) return false;
    // Simple format: each tool serialized in sequence
    for (const auto &t : m_completedTools) {
        ofs << t->serialize();
    }
    ofs.close();
    return true;
}

bool ToolManager::loadMarksFromFile(const std::string& filename) {
    std::ifstream ifs(filename, std::ios::in);
    if (!ifs) return false;
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ifs.close();
    // Split by blocks ending with ---END---
    size_t pos = 0;
    while (pos < content.size()) {
        size_t end = content.find("---END---", pos);
        if (end == std::string::npos) break;
        size_t blockEnd = end + strlen("---END---");
        std::string block = content.substr(pos, blockEnd - pos);
        // In block parse Type= to decide which tool to instantiate
        int type = TOOL_NONE;
        {
            size_t tpos = block.find("Type=");
            if (tpos != std::string::npos) {
                size_t eol = block.find('\n', tpos);
                std::string val = block.substr(tpos + 5, (eol == std::string::npos ? block.size() : eol) - (tpos+5));
                type = std::stoi(val);
            }
        }
        std::shared_ptr<MeasurementTool> tool;
        switch (type) {
            case TOOL_LINE: tool = std::make_shared<LineTool>(); break;
            case TOOL_ANGLE: tool = std::make_shared<AngleTool>(); break;
            case TOOL_BEZIER: tool = std::make_shared<BezierTool>(); break;
            case TOOL_CLOSED_BEZIER: tool = std::make_shared<ClosedBezierTool>(); break;
            case TOOL_RECTANGLE: tool = std::make_shared<RectangleTool>(); break;
            case TOOL_CIRCLE: tool = std::make_shared<CircleTool>(); break;
            case TOOL_FREEHAND: tool = std::make_shared<FreehandTool>(); break;
            case TOOL_WINDOW_LEVEL: tool = std::make_shared<WindowLevelTool>(); break;
            case TOOL_SLICE_SWITCH: tool = std::make_shared<SliceSwitchTool>(); break;
            default: tool = nullptr; break;
        }
        if (tool) {
            tool->deserialize(block);
            // recalc results for loaded marks
            tool->calculateResult(m_volumeData, m_width, m_height, m_depth, m_spacingX, m_spacingY, m_spacingZ);
            m_completedTools.push_back(tool);
        }
        pos = blockEnd;
    }
    return true;
}

void ToolManager::printResults() {
    printf("--- Measurement results (%zu) ---\n", m_completedTools.size());
    int idx=0;
    for (const auto &t : m_completedTools) {
        auto r = t->getResult();
        std::string s = (r ? r->toString() : std::string("(no result)"));
        printf("[%02d] ToolType=%d Name=%s\n%s\n", idx, (int)t->getType(), t->getName().c_str(), s.c_str());
        idx++;
    }
}

// End of file
