#include "BridgeAPI.h"
#include "WindowManager.h"
#include "MouseToolManager.h"
#include "EncodingUtils.h"
#include <sstream>
#include <iomanip>
#include <mutex>
#include <chrono>

// ============================================================================
// BridgeContext - 单例管理所有Bridge状态
// ============================================================================

class BridgeContext {
public:
    static BridgeContext& Instance() {
        static BridgeContext instance;
        return instance;
    }

    // 窗口管理
    std::unique_ptr<WindowManager> windowManager;
    
    // 工具管理
    std::unique_ptr<MouseToolManager> toolManager;
    
    // Volume句柄映射 (简化实现，实际应使用map)
    void* currentVolume = nullptr;
    
    // MPR渲染器句柄
    void* currentMPR = nullptr;
    
    // 状态信息
    struct Status {
        double fps = 0.0;
        int mouseX = 0;
        int mouseY = 0;
        int currentSlice = 0;
        int windowWidth = 400;
        int windowLevel = 40;
        double zoom = 1.0;
        int panX = 0;
        int panY = 0;
        std::string activeTool = "none";
    } status;
    
    // 线程安全
    std::mutex mutex;
    
    // FPS计算
    std::chrono::steady_clock::time_point lastFrameTime;
    int frameCount = 0;
    
private:
    BridgeContext() {
        toolManager = std::make_unique<MouseToolManager>();
        lastFrameTime = std::chrono::steady_clock::now();
    }
};

// ============================================================================
// 内部辅助函数
// ============================================================================

namespace {
    // JSON转义
    std::string JsonEscape(const std::string& str) {
        std::ostringstream oss;
        for (char c : str) {
            switch (c) {
                case '"':  oss << "\\\""; break;
                case '\\': oss << "\\\\"; break;
                case '\b': oss << "\\b"; break;
                case '\f': oss << "\\f"; break;
                case '\n': oss << "\\n"; break;
                case '\r': oss << "\\r"; break;
                case '\t': oss << "\\t"; break;
                default:   oss << c; break;
            }
        }
        return oss.str();
    }
    
    // 工具类型转字符串
    const char* ToolTypeToString(ToolType type) {
        switch (type) {
            case TOOL_MEASURE: return "measure";
            case TOOL_BRUSH: return "brush";
            case TOOL_ERASER: return "eraser";
            case TOOL_ROI_RECT: return "roi_rectangle";
            case TOOL_ROI_ELLIPSE: return "roi_ellipse";
            case TOOL_WINDOWING: return "windowing";
            case TOOL_PAN: return "pan";
            case TOOL_ZOOM: return "zoom";
            case TOOL_ROTATE: return "rotate";
            default: return "none";
        }
    }
}

// ============================================================================
// 窗口管理API
// ============================================================================

extern "C" {

BRIDGE_API void* Bridge_CreateWindow(int width, int height, void* parentHWND) {
    try {
        auto& ctx = BridgeContext::Instance();
        std::lock_guard<std::mutex> lock(ctx.mutex);
        
        ctx.windowManager = std::make_unique<WindowManager>();
        
        // 设置鼠标回调
        ctx.windowManager->SetMouseCallback([](int x, int y, int button, int action) {
            auto& ctx = BridgeContext::Instance();
            ctx.status.mouseX = x;
            ctx.status.mouseY = y;
            
            // 分发到工具管理器
            if (action == 0) {  // Down
                ctx.toolManager->DispatchMouseDown(x, y, button);
            } else if (action == 1) {  // Move
                ctx.toolManager->DispatchMouseMove(x, y);
            } else if (action == 2) {  // Up
                ctx.toolManager->DispatchMouseUp(x, y, button);
            }
        });
        
        // 创建窗口
        if (ctx.windowManager->CreateWindow(width, height, L"MedicalViewer", (HWND)parentHWND)) {
            return ctx.windowManager.get();
        }
        
        return nullptr;
    } catch (...) {
        return nullptr;
    }
}

BRIDGE_API void Bridge_DestroyWindow(void* windowHandle) {
    try {
        auto& ctx = BridgeContext::Instance();
        std::lock_guard<std::mutex> lock(ctx.mutex);
        ctx.windowManager.reset();
    } catch (...) {
    }
}

BRIDGE_API bool Bridge_ProcessMessages(void* windowHandle) {
    try {
        auto& ctx = BridgeContext::Instance();
        if (ctx.windowManager) {
            return ctx.windowManager->ProcessMessages();
        }
    } catch (...) {
    }
    return false;
}

// ============================================================================
// DICOM加载API
// ============================================================================

BRIDGE_API VolumeHandle Bridge_LoadDicomSeries(const char* pathUtf8, ProgressCallback callback) {
    try {
        auto& ctx = BridgeContext::Instance();
        std::lock_guard<std::mutex> lock(ctx.mutex);
        
        // 转换UTF-8路径
        std::string utf8Path = pathUtf8;
        std::wstring widePath = EncodingUtils::Utf8ToWide(utf8Path);
        
        // 调用DllDicom的加载函数
        // TODO: 实际调用Dicom_Volume_LoadFromDicomSeries
        // void* volume = Dicom_Volume_LoadFromDicomSeries(...);
        
        // 临时返回假数据用于测试
        ctx.currentVolume = (void*)0x12345678;
        
        if (callback) {
            callback(100.0, "加载完成");
        }
        
        return ctx.currentVolume;
    } catch (...) {
        return nullptr;
    }
}

BRIDGE_API void Bridge_UnloadVolume(VolumeHandle volume) {
    try {
        auto& ctx = BridgeContext::Instance();
        std::lock_guard<std::mutex> lock(ctx.mutex);
        
        // TODO: 调用DllDicom的释放函数
        // Dicom_Volume_Free(volume);
        
        if (ctx.currentVolume == volume) {
            ctx.currentVolume = nullptr;
        }
    } catch (...) {
    }
}

BRIDGE_API const char* Bridge_GetVolumeInfo(VolumeHandle volume) {
    try {
        // TODO: 从volume获取实际信息
        static std::string json;
        std::ostringstream oss;
        oss << "{"
            << "\"width\": 512,"
            << "\"height\": 512,"
            << "\"depth\": 300,"
            << "\"spacingX\": 0.5,"
            << "\"spacingY\": 0.5,"
            << "\"spacingZ\": 1.0,"
            << "\"dataType\": \"uint16\""
            << "}";
        json = oss.str();
        return json.c_str();
    } catch (...) {
        return "{}";
    }
}

// ============================================================================
// MPR渲染API
// ============================================================================

BRIDGE_API MPRHandle Bridge_CreateMPRRenderer(VolumeHandle volume, int direction) {
    try {
        auto& ctx = BridgeContext::Instance();
        std::lock_guard<std::mutex> lock(ctx.mutex);
        
        // TODO: 调用DllVisualization创建MPR渲染器
        // void* mpr = Visualization_MPR_Create(volume, direction);
        
        ctx.currentMPR = (void*)0x87654321;
        return ctx.currentMPR;
    } catch (...) {
        return nullptr;
    }
}

BRIDGE_API void Bridge_DestroyMPRRenderer(MPRHandle mpr) {
    try {
        auto& ctx = BridgeContext::Instance();
        std::lock_guard<std::mutex> lock(ctx.mutex);
        
        // TODO: 调用DllVisualization释放
        // Visualization_MPR_Destroy(mpr);
        
        if (ctx.currentMPR == mpr) {
            ctx.currentMPR = nullptr;
        }
    } catch (...) {
    }
}

BRIDGE_API FrameBuffer* Bridge_RenderMPR(MPRHandle mpr) {
    try {
        auto& ctx = BridgeContext::Instance();
        
        // 更新FPS
        auto now = std::chrono::steady_clock::now();
        ctx.frameCount++;
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - ctx.lastFrameTime).count();
        if (elapsed >= 1000) {
            ctx.status.fps = ctx.frameCount * 1000.0 / elapsed;
            ctx.frameCount = 0;
            ctx.lastFrameTime = now;
        }
        
        // TODO: 实际的离屏渲染
        // FrameBuffer* fb = Visualization_MPR_RenderOffscreen(mpr);
        
        // 临时返回假数据
        static FrameBuffer fb;
        fb.width = 512;
        fb.height = 512;
        fb.channels = 4;
        fb.pixels = nullptr;  // 实际应该是RGBA数据
        return &fb;
    } catch (...) {
        return nullptr;
    }
}

BRIDGE_API void Bridge_SetMPRSliceIndex(MPRHandle mpr, int index) {
    try {
        auto& ctx = BridgeContext::Instance();
        std::lock_guard<std::mutex> lock(ctx.mutex);
        
        ctx.status.currentSlice = index;
        
        // TODO: 调用DllVisualization设置切片
        // Visualization_MPR_SetSliceIndex(mpr, index);
    } catch (...) {
    }
}

BRIDGE_API void Bridge_SetMPRWindow(MPRHandle mpr, int windowWidth, int windowLevel) {
    try {
        auto& ctx = BridgeContext::Instance();
        std::lock_guard<std::mutex> lock(ctx.mutex);
        
        ctx.status.windowWidth = windowWidth;
        ctx.status.windowLevel = windowLevel;
        
        // TODO: 调用DllVisualization设置窗宽窗位
        // Visualization_MPR_SetWindow(mpr, windowWidth, windowLevel);
    } catch (...) {
    }
}

BRIDGE_API void Bridge_SetMPRZoom(MPRHandle mpr, double zoom) {
    try {
        auto& ctx = BridgeContext::Instance();
        std::lock_guard<std::mutex> lock(ctx.mutex);
        
        ctx.status.zoom = zoom;
        
        // TODO: 调用DllVisualization设置缩放
        // Visualization_MPR_SetZoom(mpr, zoom);
    } catch (...) {
    }
}

BRIDGE_API void Bridge_SetMPRPan(MPRHandle mpr, int panX, int panY) {
    try {
        auto& ctx = BridgeContext::Instance();
        std::lock_guard<std::mutex> lock(ctx.mutex);
        
        ctx.status.panX = panX;
        ctx.status.panY = panY;
        
        // TODO: 调用DllVisualization设置平移
        // Visualization_MPR_SetPan(mpr, panX, panY);
    } catch (...) {
    }
}

// ============================================================================
// 鼠标工具API
// ============================================================================

BRIDGE_API void Bridge_SetMouseTool(int toolType) {
    try {
        auto& ctx = BridgeContext::Instance();
        std::lock_guard<std::mutex> lock(ctx.mutex);
        
        ctx.toolManager->SetActiveTool((ToolType)toolType);
        ctx.status.activeTool = ToolTypeToString((ToolType)toolType);
    } catch (...) {
    }
}

BRIDGE_API int Bridge_GetMouseTool() {
    try {
        auto& ctx = BridgeContext::Instance();
        std::lock_guard<std::mutex> lock(ctx.mutex);
        
        return (int)ctx.toolManager->GetActiveTool();
    } catch (...) {
        return (int)TOOL_NONE;
    }
}

BRIDGE_API void Bridge_SendMouseEvent(int x, int y, int button, int action) {
    try {
        auto& ctx = BridgeContext::Instance();
        ctx.status.mouseX = x;
        ctx.status.mouseY = y;
        
        if (action == 0) {  // Down
            ctx.toolManager->DispatchMouseDown(x, y, button);
        } else if (action == 1) {  // Move
            ctx.toolManager->DispatchMouseMove(x, y);
        } else if (action == 2) {  // Up
            ctx.toolManager->DispatchMouseUp(x, y, button);
        }
    } catch (...) {
    }
}

BRIDGE_API void Bridge_RenderTools(MPRHandle mpr) {
    try {
        auto& ctx = BridgeContext::Instance();
        ctx.toolManager->RenderAll();
    } catch (...) {
    }
}

// ============================================================================
// 状态查询API
// ============================================================================

BRIDGE_API const char* Bridge_GetRenderStatus() {
    try {
        auto& ctx = BridgeContext::Instance();
        std::lock_guard<std::mutex> lock(ctx.mutex);
        
        static std::string json;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "{"
            << "\"fps\": " << ctx.status.fps << ","
            << "\"mousePosition\": {\"x\": " << ctx.status.mouseX << ", \"y\": " << ctx.status.mouseY << "},"
            << "\"currentSlice\": " << ctx.status.currentSlice << ","
            << "\"windowWidth\": " << ctx.status.windowWidth << ","
            << "\"windowLevel\": " << ctx.status.windowLevel << ","
            << "\"zoom\": " << ctx.status.zoom << ","
            << "\"pan\": {\"x\": " << ctx.status.panX << ", \"y\": " << ctx.status.panY << "},"
            << "\"activeTool\": \"" << ctx.status.activeTool << "\""
            << "}";
        json = oss.str();
        return json.c_str();
    } catch (...) {
        return "{}";
    }
}

BRIDGE_API const char* Bridge_GetMeasurements() {
    try {
        auto& ctx = BridgeContext::Instance();
        std::lock_guard<std::mutex> lock(ctx.mutex);
        
        // 获取测量工具的数据
        auto* measureTool = dynamic_cast<MeasureTool*>(ctx.toolManager->GetTool(TOOL_MEASURE));
        if (!measureTool) {
            return "[]";
        }
        
        const auto& measurements = measureTool->GetMeasurements();
        
        static std::string json;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "[";
        for (size_t i = 0; i < measurements.size(); ++i) {
            if (i > 0) oss << ",";
            const auto& m = measurements[i];
            oss << "{"
                << "\"id\": " << i << ","
                << "\"type\": \"distance\","
                << "\"value\": " << m.distance << ","
                << "\"unit\": \"pixels\","
                << "\"points\": [[" << m.startX << "," << m.startY << "],["
                << m.endX << "," << m.endY << "]]"
                << "}";
        }
        oss << "]";
        json = oss.str();
        return json.c_str();
    } catch (...) {
        return "[]";
    }
}

BRIDGE_API const char* Bridge_GetROIs() {
    try {
        auto& ctx = BridgeContext::Instance();
        std::lock_guard<std::mutex> lock(ctx.mutex);
        
        // 获取ROI工具的数据
        auto* roiTool = dynamic_cast<ROIRectangleTool*>(ctx.toolManager->GetTool(TOOL_ROI_RECT));
        if (!roiTool) {
            return "[]";
        }
        
        const auto& rois = roiTool->GetROIs();
        
        static std::string json;
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < rois.size(); ++i) {
            if (i > 0) oss << ",";
            const auto& r = rois[i];
            oss << "{"
                << "\"id\": " << i << ","
                << "\"type\": \"rectangle\","
                << "\"x\": " << r.x << ","
                << "\"y\": " << r.y << ","
                << "\"width\": " << r.width << ","
                << "\"height\": " << r.height
                << "}";
        }
        oss << "]";
        json = oss.str();
        return json.c_str();
    } catch (...) {
        return "[]";
    }
}

// ============================================================================
// 初始化与清理
// ============================================================================

BRIDGE_API bool Bridge_Initialize() {
    try {
        // 初始化各个DLL模块
        // TODO: 调用各DLL的初始化函数
        return true;
    } catch (...) {
        return false;
    }
}

BRIDGE_API void Bridge_Shutdown() {
    try {
        auto& ctx = BridgeContext::Instance();
        std::lock_guard<std::mutex> lock(ctx.mutex);
        
        ctx.windowManager.reset();
        ctx.toolManager.reset();
        
        // TODO: 调用各DLL的清理函数
    } catch (...) {
    }
}

} // extern "C"
