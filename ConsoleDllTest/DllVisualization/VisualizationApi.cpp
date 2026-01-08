#include "pch.h"

#include "../Common/NativeInterfaces.h"
#include "../DllDicom/DicomApi.h"
#include "../Common/VolumeData.h"
#include "../DllImageProcessing/ImageProcessingApi.h"
#include "MeasurementTools.h"  // 锟斤拷锟斤拷Point2D锟饺讹拷锟斤拷
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <random>
#include <string>
#include <cctype>
#include <vector>
#include <iostream>
#include <sstream>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <deque>
#include <map>
#include <unordered_map>
#include <fstream>

#include "ImageBrowserOrthogonal3DRenderer.h"
#include "RoiOrthogonal3DRenderer.h"
#include "ReconstructionRaycast3DRenderer.h"

// JSON support (锟津单碉拷JSON锟斤拷锟叫伙拷锟斤拷锟斤拷使锟斤拷nlohmann/json)
#include <sstream>

// Base64 锟斤拷锟斤拷/锟斤拷锟诫（锟斤拷锟斤拷mask锟斤拷锟斤拷锟斤拷锟叫伙拷锟斤拷
#include <string>
#include <vector>

// vcpkg OpenGL/GLEW/GLFW3 锟斤拷态锟斤拷锟斤拷
#include <GL/glew.h>
#if !defined(_WIN32)
#include <GLFW/glfw3.h>
#else
#include <windowsx.h>  // GET_X_LPARAM, GET_Y_LPARAM锟饺猴拷
#include <commdlg.h>   // GetOpenFileName
#endif

// NanoVG for high-quality vector graphics (圆锟轿端点、圆锟斤拷锟斤拷锟斤拷)
#define NANOVG_GL3_IMPLEMENTATION
#include "../Dlls/include/nanovg.h"
#include "../Dlls/include/nanovg_gl.h"

// NOTE: We intentionally avoid GLFW on Windows to match Electron/Chromium's
// HWND-based hosting model. GLFW codepaths are kept for non-Windows builds.

// DICOM APIs come from ../DllDicom/DicomApi.h

// 前锟斤拷锟斤拷锟斤拷 MaskManager 锟斤拷
class MaskManager;

// 全锟街诧拷锟叫匡拷状态锟斤拷使锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟疥）
// 瑁佸垏褰㈢姸绫诲瀷
enum CropShapeType {
    CROP_SHAPE_BOX = 0,      // 绔嬫柟浣?
    CROP_SHAPE_SPHERE = 1,   // 鐞冧綋
    CROP_SHAPE_CYLINDER = 2  // 鍦嗘煴浣?
};

// 鍦嗘煴浣撴柟鍚?
enum CropCylinderDirection {
    CROP_CYLINDER_AXIAL = 0,    // 杞村悜 (娌縕杞?
    CROP_CYLINDER_CORONAL = 1,  // 鍐犵姸 (娌縔杞?
    CROP_CYLINDER_SAGITTAL = 2  // 鐭㈢姸 (娌縓杞?
};

struct GlobalAPRCropBox {
    bool enabled = false;
    
    // 浣撶礌鍧愭爣绯讳腑鐨?D瑁佸垏妗嗚寖鍥达紙鍖呭洿鐩掞紝閫傜敤浜庢墍鏈夊舰鐘讹級
    float xStart = 0.0f;
    float xEnd = 0.0f;
    float yStart = 0.0f;
    float yEnd = 0.0f;
    float zStart = 0.0f;
    float zEnd = 0.0f;
    
    // 褰㈢姸璁剧疆
    CropShapeType shape = CROP_SHAPE_BOX;
    CropCylinderDirection cylinderDirection = CROP_CYLINDER_AXIAL;
    
    // 鎷栧姩鐘舵€?
    bool isDragging = false;
    bool isDraggingBox = false;  // 鏄惁姝ｅ湪鎷栧姩鏁翠釜妗嗭紙Shift+鎷栧姩锛?
    int dragCorner = -1;   // 0=宸︿笂, 1=鍙充笂, 2=宸︿笅, 3=鍙充笅
    int dragEdge = -1;     // 0=涓? 1=涓? 2=宸? 3=鍙?
    int dragDirection = 0; // 鎷栧姩鎵€鍦ㄧ殑瑙嗗浘绫诲瀷 (0=Axial, 1=Coronal, 2=Sagittal)
    double dragStartMouseX = 0.0;  // 鎷栧姩寮€濮嬫椂鐨勯紶鏍囦綅缃紙灞忓箷鍧愭爣锛?
    double dragStartMouseY = 0.0;
    float dragStartXStart = 0.0f;  // 鎷栧姩寮€濮嬫椂鐨勬浣嶇疆锛堜綋绱犲潗鏍囷級
    float dragStartXEnd = 0.0f;
    float dragStartYStart = 0.0f;
    float dragStartYEnd = 0.0f;
    float dragStartZStart = 0.0f;
    float dragStartZEnd = 0.0f;
};

// DEPRECATED: Use TabSessionContext::cropBox instead
static GlobalAPRCropBox g_aprCropBox;

// 全锟街憋拷锟斤拷锟斤拷锟芥储锟斤拷锟揭伙拷尾锟斤拷械慕锟斤拷
// Per-session cropped APR storage (sessionId -> APRHandle)
static std::map<std::string, APRHandle> g_sessionCroppedAPRs;
// Legacy global pointer for backward compatibility
// DEPRECATED: Use TabSessionContext::croppedAPR instead
static APRHandle g_lastCroppedAPR = nullptr;

// 全锟街憋拷锟斤拷锟斤拷3D 锟斤拷锟斤拷锟斤拷图锟斤拷锟斤拷转锟角度猴拷锟斤拷锟斤拷
// DEPRECATED: Use TabSessionContext::rotX instead
static float g_3dRotX = 30.0f;
static float g_3dRotY = 45.0f;
static float g_3dZoom = 1.0f;  // 锟斤拷锟斤拷锟斤拷锟斤拷
static float g_3dPanX = 0.0f;
static float g_3dPanY = 0.0f;
static float g_3dRotMat[16] = {
    1,0,0,0,
    0,1,0,0,
    0,0,1,0,
    0,0,0,1
};

// ==================== 3D primitives state ====================
enum class Primitive3DTypeCpp {
    Cube = 1,
    Sphere = 2,
    Cylinder = 3,
};

struct Primitive3D {
    int id = 0;
    Primitive3DTypeCpp type = Primitive3DTypeCpp::Cube;
    // Parameters
    float p0 = 1.0f; // sizeX / radius
    float p1 = 1.0f; // sizeY / height
    float p2 = 1.0f; // sizeZ

    // Transform
    float tx = 0, ty = 0, tz = 0;
    float rx = 0, ry = 0, rz = 0;
    float sx = 1, sy = 1, sz = 1;

    // Appearance
    float r = 0.9f, g = 0.2f, b = 0.2f, a = 0.9f;
    bool visible = true;
};

struct Scene3DTransform {
    float tx = 0, ty = 0, tz = 0;
    float rx = 0, ry = 0, rz = 0;
    float sx = 1, sy = 1, sz = 1;
};
static bool g_3dRotating = false;  // 锟角凤拷锟斤拷锟斤拷锟揭硷拷锟斤拷转
static double g_3dLastMouseX = 0.0;
static double g_3dLastMouseY = 0.0;

// 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟皆诧拷值锟截诧拷锟斤拷
static uint16_t SampleVolume(const uint16_t* data, int width, int height, int depth,
                              float x, float y, float z) {
    // 锟竭斤拷锟斤拷
    if (x < 0 || x >= width - 1 || y < 0 || y >= height - 1 || z < 0 || z >= depth - 1) {
        return 0;
    }
    
    // 锟斤拷取锟斤拷锟斤拷锟斤拷锟街猴拷小锟斤拷锟斤拷锟斤拷
    int x0 = static_cast<int>(x);
    int y0 = static_cast<int>(y);
    int z0 = static_cast<int>(z);
    int x1 = x0 + 1;
    int y1 = y0 + 1;
    int z1 = z0 + 1;
    
    float fx = x - x0;
    float fy = y - y0;
    float fz = z - z0;
    
    // 锟斤拷取8锟斤拷锟斤拷锟斤拷锟街?
    size_t idx000 = static_cast<size_t>(z0) * width * height + y0 * width + x0;
    size_t idx001 = static_cast<size_t>(z0) * width * height + y0 * width + x1;
    size_t idx010 = static_cast<size_t>(z0) * width * height + y1 * width + x0;
    size_t idx011 = static_cast<size_t>(z0) * width * height + y1 * width + x1;
    size_t idx100 = static_cast<size_t>(z1) * width * height + y0 * width + x0;
    size_t idx101 = static_cast<size_t>(z1) * width * height + y0 * width + x1;
    size_t idx110 = static_cast<size_t>(z1) * width * height + y1 * width + x0;
    size_t idx111 = static_cast<size_t>(z1) * width * height + y1 * width + x1;
    
    float v000 = data[idx000];
    float v001 = data[idx001];
    float v010 = data[idx010];
    float v011 = data[idx011];
    float v100 = data[idx100];
    float v101 = data[idx101];
    float v110 = data[idx110];
    float v111 = data[idx111];
    
    // 锟斤拷锟斤拷锟皆诧拷值
    float v00 = v000 * (1 - fx) + v001 * fx;
    float v01 = v010 * (1 - fx) + v011 * fx;
    float v10 = v100 * (1 - fx) + v101 * fx;
    float v11 = v110 * (1 - fx) + v111 * fx;
    
    float v0 = v00 * (1 - fy) + v01 * fy;
    float v1 = v10 * (1 - fy) + v11 * fy;
    
    float result = v0 * (1 - fz) + v1 * fz;
    
    return static_cast<uint16_t>(result);
}

// 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷染锟斤拷锟斤拷锟斤拷锟斤拷锟节ｏ拷锟斤拷锟斤拷锟捷猴拷龋锟街э拷锟斤拷锟斤拷牛锟?
// crossX, crossY: 锟斤拷位锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷 [0, 1] 锟叫碉拷位锟斤拷
static void RenderTextureToWindow(GLuint textureID, int texWidth, int texHeight, float zoomFactor = 1.0f, float crossTexX = 0.5f, float crossTexY = 0.5f) {
    glClear(GL_COLOR_BUFFER_BIT);
    
    // 锟斤拷取锟斤拷锟节尺达拷
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    int winWidth = viewport[2];
    int winHeight = viewport[3];
    
    // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟捷猴拷锟?
    float texAspect = static_cast<float>(texWidth) / texHeight;
    float winAspect = static_cast<float>(winWidth) / winHeight;
    
    // 锟斤拷锟姐保锟斤拷锟捷猴拷鹊锟斤拷锟绞撅拷锟斤拷锟轿达拷锟斤拷锟绞憋拷锟?
    float baseLeft, baseRight, baseBottom, baseTop;
    if (texAspect > winAspect) {
        baseLeft = -1.0f;
        baseRight = 1.0f;
        float h = winAspect / texAspect;
        baseBottom = -h;
        baseTop = h;
    } else {
        baseBottom = -1.0f;
        baseTop = 1.0f;
        float w = texAspect / winAspect;
        baseLeft = -w;
        baseRight = w;
    }
    
    // 锟斤拷位锟斤拷锟斤拷锟斤拷幕锟秸硷拷锟轿伙拷茫锟轿达拷锟斤拷锟绞憋拷锟?
    float crossScreenX = baseLeft + (baseRight - baseLeft) * crossTexX;
    float crossScreenY = baseBottom + (baseTop - baseBottom) * crossTexY;
    
    const float z = std::max(1e-6f, zoomFactor);

    // 锟斤拷锟斤拷锟斤拷锟脚猴拷锟斤拷锟斤拷锟斤拷锟斤拷攴段?
    // 锟斤拷锟斤拷时锟斤拷锟斤拷位锟竭憋拷锟斤拷锟斤拷锟斤拷幕锟侥固讹拷位锟斤拷 (crossScreenX, crossScreenY)
    float texLeft, texRight, texBottom, texTop;
    
    if (z > 1.0f) {
        // 锟斤拷锟脚猴拷锟斤拷示锟斤拷锟斤拷锟斤拷锟斤拷围锟斤拷锟饺和高讹拷
        float displayTexWidth = 1.0f / z;
        float displayTexHeight = 1.0f / z;
        
        // 锟斤拷位锟斤拷锟斤拷锟?锟揭诧拷锟斤拷锟斤拷锟斤拷锟斤拷缺锟斤拷锟?
        float leftRatio = crossTexX;
        float rightRatio = 1.0f - crossTexX;
        float bottomRatio = crossTexY;
        float topRatio = 1.0f - crossTexY;
        
        // 锟斤拷锟斤拷锟斤拷示锟斤拷锟斤拷锟斤拷锟斤拷锟疥范围
        texLeft = crossTexX - displayTexWidth * leftRatio;
        texRight = crossTexX + displayTexWidth * rightRatio;
        texBottom = crossTexY - displayTexHeight * bottomRatio;
        texTop = crossTexY + displayTexHeight * topRatio;
        
        // 锟竭斤拷锟介，锟斤拷止锟斤拷锟斤拷 [0, 1]
        if (texLeft < 0.0f) {
            texRight += (0.0f - texLeft);
            texLeft = 0.0f;
        }
        if (texRight > 1.0f) {
            texLeft -= (texRight - 1.0f);
            texRight = 1.0f;
        }
        if (texBottom < 0.0f) {
            texTop += (0.0f - texBottom);
            texBottom = 0.0f;
        }
        if (texTop > 1.0f) {
            texBottom -= (texTop - 1.0f);
            texTop = 1.0f;
        }
        
        // 锟劫次硷拷椋凤拷锟斤拷诜锟轿э拷锟?
        if (texLeft < 0.0f) texLeft = 0.0f;
        if (texRight > 1.0f) texRight = 1.0f;
        if (texBottom < 0.0f) texBottom = 0.0f;
        if (texTop > 1.0f) texTop = 1.0f;
    } else {
        // z <= 1: 锟斤拷示锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷图锟斤拷锟斤拷
        texLeft = 0.0f;
        texRight = 1.0f;
        texBottom = 0.0f;
        texTop = 1.0f;
    }

    // z < 1: 锟斤拷锟斤拷锟斤拷锟绞撅拷锟斤拷锟斤拷冢锟斤拷瞥锟斤拷锟斤拷椋拷锟斤拷锟斤拷锟斤拷锟斤拷锟较?锟斤拷锟狡空硷拷一锟斤拷
    if (z < 1.0f) {
        baseLeft = crossScreenX + (baseLeft - crossScreenX) * z;
        baseRight = crossScreenX + (baseRight - crossScreenX) * z;
        baseBottom = crossScreenY + (baseBottom - crossScreenY) * z;
        baseTop = crossScreenY + (baseTop - crossScreenY) * z;
    }
    
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷模式为 REPLACE锟斤拷使锟斤拷锟斤拷锟斤拷锟斤拷色锟斤拷锟斤拷锟皆讹拷锟斤拷锟斤拷色锟斤拷
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    
    // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // 确锟斤拷使锟矫帮拷色锟斤拷锟斤拷锟斤拷色
    glColor3f(1.0f, 1.0f, 1.0f);
    
    // 锟斤拷锟斤拷锟侥憋拷锟轿ｏ拷使锟矫硷拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟疥）
    glBegin(GL_QUADS);
    glTexCoord2f(texLeft, texBottom);  glVertex2f(baseLeft, baseBottom);
    glTexCoord2f(texRight, texBottom); glVertex2f(baseRight, baseBottom);
    glTexCoord2f(texRight, texTop);    glVertex2f(baseRight, baseTop);
    glTexCoord2f(texLeft, texTop);     glVertex2f(baseLeft, baseTop);
    glEnd();
    
    glDisable(GL_TEXTURE_2D);
}

struct TexWindowMapping {
    float baseLeft = -1.0f;
    float baseRight = 1.0f;
    float baseBottom = -1.0f;
    float baseTop = 1.0f;
    float texLeft = 0.0f;
    float texRight = 1.0f;
    float texBottom = 0.0f;
    float texTop = 1.0f;
    bool valid = false;
};

static inline Point2D ImageNdcToScreenNdc(const TexWindowMapping& map, const Point2D& imageNdc) {
    // Convert full-image NDC (-1..1) to screen NDC, honoring both aspect-fit (base*)
    // and zoom/crop (tex*). This must mirror RenderTextureToWindow.
    Point2D out{0, 0};
    if (!map.valid) return out;

    const float du = (map.texRight - map.texLeft);
    const float dv = (map.texTop - map.texBottom);
    if (std::fabs(du) < 1e-6f || std::fabs(dv) < 1e-6f) return out;

    const float u = (imageNdc.x + 1.0f) * 0.5f;
    const float v = (imageNdc.y + 1.0f) * 0.5f;
    const float relX = (u - map.texLeft) / du;
    const float relY = (v - map.texBottom) / dv;

    out.x = map.baseLeft + relX * (map.baseRight - map.baseLeft);
    out.y = map.baseBottom + relY * (map.baseTop - map.baseBottom);
    return out;
}

static inline bool ScreenNdcToImageNdc(const TexWindowMapping& map, const Point2D& screenNdc, Point2D& outImageNdc, bool clampToImageRect) {
    if (!map.valid) return false;

    Point2D s = screenNdc;
    if (clampToImageRect) {
        s.x = std::max(map.baseLeft, std::min(map.baseRight, s.x));
        s.y = std::max(map.baseBottom, std::min(map.baseTop, s.y));
    } else {
        if (s.x < map.baseLeft || s.x > map.baseRight || s.y < map.baseBottom || s.y > map.baseTop) {
            return false;
        }
    }

    const float baseW = (map.baseRight - map.baseLeft);
    const float baseH = (map.baseTop - map.baseBottom);
    const float du = (map.texRight - map.texLeft);
    const float dv = (map.texTop - map.texBottom);
    if (std::fabs(baseW) < 1e-6f || std::fabs(baseH) < 1e-6f || std::fabs(du) < 1e-6f || std::fabs(dv) < 1e-6f) {
        return false;
    }

    const float relX = (s.x - map.baseLeft) / baseW;
    const float relY = (s.y - map.baseBottom) / baseH;

    const float u = map.texLeft + relX * du;
    const float v = map.texBottom + relY * dv;

    outImageNdc.x = u * 2.0f - 1.0f;
    outImageNdc.y = v * 2.0f - 1.0f;
    return true;
}

// Compute the exact mapping used by RenderTextureToWindow so overlays / hit-tests match the rendered image.
static TexWindowMapping ComputeTexWindowMapping(int texWidth, int texHeight, int windowWidth, int windowHeight,
                                                float zoomFactor, float crossTexX, float crossTexY) {
    TexWindowMapping m;
    if (texWidth <= 1 || texHeight <= 1 || windowWidth <= 0 || windowHeight <= 0) return m;

    const float texAspect = static_cast<float>(texWidth) / static_cast<float>(texHeight);
    const float winAspect = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);

    float baseLeft = -1.0f, baseRight = 1.0f, baseBottom = -1.0f, baseTop = 1.0f;
    if (texAspect > winAspect) {
        float h = winAspect / texAspect;
        baseBottom = -h;
        baseTop = h;
    } else {
        float w = texAspect / winAspect;
        baseLeft = -w;
        baseRight = w;
    }

    // Crosshair position in screen space (for z < 1 scaling)
    const float crossScreenX = baseLeft + (baseRight - baseLeft) * crossTexX;
    const float crossScreenY = baseBottom + (baseTop - baseBottom) * crossTexY;

    float texLeft = 0.0f, texRight = 1.0f, texBottom = 0.0f, texTop = 1.0f;
    const float z = std::max(1e-6f, zoomFactor);

    if (z > 1.0f) {
        const float displayTexWidth = 1.0f / z;
        const float displayTexHeight = 1.0f / z;

        const float leftRatio = crossTexX;
        const float rightRatio = 1.0f - crossTexX;
        const float bottomRatio = crossTexY;
        const float topRatio = 1.0f - crossTexY;

        texLeft = crossTexX - displayTexWidth * leftRatio;
        texRight = crossTexX + displayTexWidth * rightRatio;
        texBottom = crossTexY - displayTexHeight * bottomRatio;
        texTop = crossTexY + displayTexHeight * topRatio;

        if (texLeft < 0.0f) {
            texRight += (0.0f - texLeft);
            texLeft = 0.0f;
        }
        if (texRight > 1.0f) {
            texLeft -= (texRight - 1.0f);
            texRight = 1.0f;
        }
        if (texBottom < 0.0f) {
            texTop += (0.0f - texBottom);
            texBottom = 0.0f;
        }
        if (texTop > 1.0f) {
            texBottom -= (texTop - 1.0f);
            texTop = 1.0f;
        }

        if (texLeft < 0.0f) texLeft = 0.0f;
        if (texRight > 1.0f) texRight = 1.0f;
        if (texBottom < 0.0f) texBottom = 0.0f;
        if (texTop > 1.0f) texTop = 1.0f;
    }

    if (z < 1.0f) {
        baseLeft = crossScreenX + (baseLeft - crossScreenX) * z;
        baseRight = crossScreenX + (baseRight - crossScreenX) * z;
        baseBottom = crossScreenY + (baseBottom - crossScreenY) * z;
        baseTop = crossScreenY + (baseTop - crossScreenY) * z;
    }

    m.baseLeft = baseLeft;
    m.baseRight = baseRight;
    m.baseBottom = baseBottom;
    m.baseTop = baseTop;
    m.texLeft = texLeft;
    m.texRight = texRight;
    m.texBottom = texBottom;
    m.texTop = texTop;
    m.valid = true;
    return m;
}

// 锟斤拷染Mask锟斤拷锟接层（RGBA锟斤拷透锟斤拷锟斤拷锟斤拷锟斤拷
static void RenderMaskOverlay(const uint8_t* rgba, int texWidth, int texHeight, 
                              float zoomFactor = 1.0f, float crossTexX = 0.5f, float crossTexY = 0.5f) {
    // 生成一次性的覆盖纹理
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    
    // 上传RGBA数据
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // 开启混合，用alpha做透明
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    
    // 使用MODULATE模式支持透明度
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    
    // 根据当前视窗和缩放计算显示窗口（与 RenderTextureToWindow 保持一致）
    float texLeft = 0.0f, texRight = 1.0f, texBottom = 0.0f, texTop = 1.0f;
    float z = zoomFactor;
    
    if (z > 0 && z != 1.0f) {
        const float halfWidth = 0.5f / z;
        const float halfHeight = 0.5f / z;
        texLeft = crossTexX - halfWidth;
        texRight = crossTexX + halfWidth;
        texBottom = crossTexY - halfHeight;
        texTop = crossTexY + halfHeight;
        
        // 边缘裁剪
        if (texLeft < 0.0f) {
            texRight += (0.0f - texLeft);
            texLeft = 0.0f;
        }
        if (texRight > 1.0f) {
            texLeft -= (texRight - 1.0f);
            texRight = 1.0f;
        }
        if (texBottom < 0.0f) {
            texTop += (0.0f - texBottom);
            texBottom = 0.0f;
        }
        if (texTop > 1.0f) {
            texBottom -= (texTop - 1.0f);
            texTop = 1.0f;
        }
        
        if (texLeft < 0.0f) texLeft = 0.0f;
        if (texRight > 1.0f) texRight = 1.0f;
        if (texBottom < 0.0f) texBottom = 0.0f;
        if (texTop > 1.0f) texTop = 1.0f;
    }
    
    // 获取当前窗口大小并适配纵横比（与 RenderTextureToWindow 保持一致）
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    int windowWidth = viewport[2];
    int windowHeight = viewport[3];
    
    float texAspect = static_cast<float>(texWidth) / texHeight;
    float windowAspect = static_cast<float>(windowWidth) / windowHeight;
    
    float baseLeft = -1.0f, baseRight = 1.0f, baseBottom = -1.0f, baseTop = 1.0f;
    
    if (texAspect > windowAspect) {
        float scale = windowAspect / texAspect;
        baseBottom = -scale;
        baseTop = scale;
    } else {
        float scale = texAspect / windowAspect;
        baseLeft = -scale;
        baseRight = scale;
    }
    
    // z < 1: 缩放到 crosshair 位置，放大时用交叉点做锚点
    if (z < 1.0f) {
        const float crossScreenX = baseLeft + (baseRight - baseLeft) * crossTexX;
        const float crossScreenY = baseBottom + (baseTop - baseBottom) * crossTexY;
        baseLeft = crossScreenX + (baseLeft - crossScreenX) * z;
        baseRight = crossScreenX + (baseRight - crossScreenX) * z;
        baseBottom = crossScreenY + (baseBottom - crossScreenY) * z;
        baseTop = crossScreenY + (baseTop - crossScreenY) * z;
    }

    // 绘制四边形
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);  // 颜色统一，透明度来自纹理 alpha
    glBegin(GL_QUADS);
    glTexCoord2f(texLeft, texBottom);  glVertex2f(baseLeft, baseBottom);
    glTexCoord2f(texRight, texBottom); glVertex2f(baseRight, baseBottom);
    glTexCoord2f(texRight, texTop);    glVertex2f(baseRight, baseTop);
    glTexCoord2f(texLeft, texTop);     glVertex2f(baseLeft, baseTop);
    glEnd();
    
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    
    // 删除一次性纹理
    glDeleteTextures(1, &texID);
}

// 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟捷结构锟斤拷锟斤拷锟节革拷锟斤拷锟斤拷锟斤拷前锟斤拷锟藉）
// 使锟斤拷3D锟斤拷锟斤拷锟斤拷锟斤拷锟疥（锟斤拷锟斤拷锟斤拷锟疥）锟斤拷锟斤拷锟斤拷锟节诧拷同锟斤拷图锟斤拷锟斤拷锟斤拷锟铰憋拷锟斤拷一锟斤拷
struct MeasurementPoint {
    float x, y, z;  // 3D锟斤拷锟斤拷锟斤拷锟斤拷锟疥（锟斤拷锟斤拷锟斤拷锟疥，锟斤拷位锟斤拷mm锟斤拷
};

// 锟斤拷锟斤拷锟斤拷堑拇锟斤拷锟轿伙拷锟斤拷锟较拷锟斤拷锟铰硷拷锟斤拷母锟斤拷锟酵硷拷锟斤拷锟揭伙拷愦达拷锟斤拷锟?
struct MeasurementLocation {
    int sliceDirection;  // 锟斤拷片锟斤拷锟斤拷0=Axial(XY平锟斤拷), 1=Coronal(XZ平锟斤拷), 2=Sagittal(YZ平锟斤拷)
    int sliceIndex;      // 锟斤拷片锟斤拷锟斤拷锟斤拷锟节该凤拷锟斤拷锟较的诧拷锟斤拷锟斤拷
    float rotX, rotY, rotZ;  // APR锟斤拷转锟角度ｏ拷锟斤拷锟斤拷锟叫讹拷锟角凤拷同一锟接角ｏ拷
    bool isAPR;          // 锟角凤拷锟斤拷APR锟斤拷图
    float centerX, centerY, centerZ;  // 锟斤拷锟斤拷时锟斤拷锟斤拷图锟斤拷锟斤拷锟斤拷锟疥（锟斤拷锟节伙拷锟狡ｏ拷锟斤拷锟斤拷锟斤拷center锟戒化锟斤拷锟斤拷锟狡讹拷锟斤拷
    
    MeasurementLocation() : sliceDirection(0), sliceIndex(0), 
                           rotX(0), rotY(0), rotZ(0), isAPR(false),
                           centerX(0), centerY(0), centerZ(0) {}
};

struct CompletedMeasurement {
    std::string sessionId;
    int id;
    int toolType;
    std::vector<MeasurementPoint> points;
    float result;  // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷搿拷嵌取锟斤拷锟斤拷锟饺ｏ拷
    MeasurementLocation location;  // 锟斤拷堑拇锟斤拷锟轿伙拷锟?
};

// ==================== 锟斤拷锟斤拷锟斤拷锟姐辅锟斤拷锟斤拷锟斤拷 ====================
static float CalculateDistance(const MeasurementPoint& p1, const MeasurementPoint& p2) {
    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    float dz = p2.z - p1.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

static float CalculateAngle(const MeasurementPoint& p1, const MeasurementPoint& vertex, const MeasurementPoint& p2) {
    // 锟斤拷锟斤拷锟斤拷锟斤拷锟竭讹拷之锟斤拷慕嵌龋锟?D锟斤拷
    float dx1 = p1.x - vertex.x;
    float dy1 = p1.y - vertex.y;
    float dz1 = p1.z - vertex.z;
    float dx2 = p2.x - vertex.x;
    float dy2 = p2.y - vertex.y;
    float dz2 = p2.z - vertex.z;
    
    float dot = dx1 * dx2 + dy1 * dy2 + dz1 * dz2;
    float len1 = sqrtf(dx1 * dx1 + dy1 * dy1 + dz1 * dz1);
    float len2 = sqrtf(dx2 * dx2 + dy2 * dy2 + dz2 * dz2);
    
    if (len1 < 0.001f || len2 < 0.001f) return 0.0f;
    
    float cosAngle = dot / (len1 * len2);
    cosAngle = fmaxf(-1.0f, fminf(1.0f, cosAngle));  // 锟斤拷锟狡凤拷围
    return acosf(cosAngle) * 180.0f / 3.14159f;  // 转锟斤拷为锟斤拷锟斤拷
}

static float CalculateRectangleArea(const MeasurementPoint& p1, const MeasurementPoint& p2) {
    float width = fabsf(p2.x - p1.x);
    float height = fabsf(p2.y - p1.y);
    return width * height;
}

static float CalculateCircleArea(const MeasurementPoint& center, const MeasurementPoint& edge) {
    float radius = CalculateDistance(center, edge);
    return 3.14159f * radius * radius;
}

// 璁＄畻妞渾闈㈢Н锛氬熀浜庝袱涓瑙掔偣瀹氫箟鐨勫寘鍥寸洅锛屾寜鍒囩墖鏂瑰悜璁＄畻
static float CalculateEllipseAreaInPlane(const MeasurementPoint& p1, const MeasurementPoint& p2, int sliceDirection) {
    float rx, ry;
    switch (sliceDirection) {
        case 0: // Axial (XY骞抽潰)
            rx = fabsf(p2.x - p1.x) / 2.0f;
            ry = fabsf(p2.y - p1.y) / 2.0f;
            break;
        case 1: // Coronal (XZ骞抽潰)
            rx = fabsf(p2.x - p1.x) / 2.0f;
            ry = fabsf(p2.z - p1.z) / 2.0f;
            break;
        case 2: // Sagittal (YZ骞抽潰)
            rx = fabsf(p2.y - p1.y) / 2.0f;
            ry = fabsf(p2.z - p1.z) / 2.0f;
            break;
        default:
            rx = fabsf(p2.x - p1.x) / 2.0f;
            ry = fabsf(p2.y - p1.y) / 2.0f;
            break;
    }
    return 3.14159f * rx * ry;  // 妞渾闈㈢Н = 蟺 * rx * ry
}

static float CalculatePolylineLength(const std::vector<MeasurementPoint>& points) {
    if (points.size() < 2) return 0.0f;
    float sum = 0.0f;
    for (size_t i = 1; i < points.size(); ++i) {
        sum += CalculateDistance(points[i - 1], points[i]);
    }
    return sum;
}

static float CalculateRectangleAreaInPlane(const MeasurementPoint& p1, const MeasurementPoint& p2, int sliceDirection) {
    // sliceDirection: 0=XY, 1=XZ, 2=YZ
    switch (sliceDirection) {
        case 0: {
            float width = fabsf(p2.x - p1.x);
            float height = fabsf(p2.y - p1.y);
            return width * height;
        }
        case 1: {
            float width = fabsf(p2.x - p1.x);
            float height = fabsf(p2.z - p1.z);
            return width * height;
        }
        case 2: {
            float width = fabsf(p2.y - p1.y);
            float height = fabsf(p2.z - p1.z);
            return width * height;
        }
        default:
            return 0.0f;
    }
}

static float RecomputeCompletedMeasurementResult(const CompletedMeasurement& m) {
    switch (m.toolType) {
        case 1:
            return (m.points.size() >= 2) ? CalculateDistance(m.points[0], m.points[1]) : 0.0f;
        case 2:
            return (m.points.size() >= 3) ? CalculateAngle(m.points[0], m.points[1], m.points[2]) : 0.0f;
        case 3:
            return (m.points.size() >= 2) ? CalculateRectangleAreaInPlane(m.points[0], m.points[1], m.location.sliceDirection) : 0.0f;
        case 4:
            return (m.points.size() >= 2) ? CalculateEllipseAreaInPlane(m.points[0], m.points[1], m.location.sliceDirection) : 0.0f;
        case 5:
        case 6:
            return CalculatePolylineLength(m.points);
        default:
            return 0.0f;
    }
}

// 锟斤拷锟狡讹拷位锟竭ｏ拷CrossHair锟斤拷- 锟斤拷锟斤拷要锟斤拷锟斤拷锟斤拷锟接ｏ拷直锟斤拷使锟矫癸拷一锟斤拷锟斤拷锟斤拷
// crossX, crossY: 锟斤拷一锟斤拷锟斤拷锟斤拷 [-1, 1]锟斤拷(0,0) 锟斤拷示锟斤拷锟斤拷
static void DrawCrossHair(float crossX, float crossY, int sliceDirection, float inPlaneAngleDeg = 0.0f) {
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(1.0f);
    
    // 锟斤拷锟斤拷锟斤拷片锟斤拷锟斤拷确锟斤拷锟斤拷锟斤拷锟斤拷色锟斤拷锟斤拷锟斤拷医学影锟斤拷锟阶硷拷锟?
    // X锟斤拷=锟斤拷色, Y锟斤拷=锟斤拷色, Z锟斤拷=锟斤拷色
    // Axial (XY平锟斤拷): 锟斤拷锟斤拷=Y锟斤拷(锟斤拷), 锟斤拷锟斤拷=X锟斤拷(锟斤拷)
    // Coronal (XZ平锟斤拷): 锟斤拷锟斤拷=Z锟斤拷(锟斤拷), 锟斤拷锟斤拷=X锟斤拷(锟斤拷)
    // Sagittal (YZ平锟斤拷): 锟斤拷锟斤拷=Z锟斤拷(锟斤拷), 锟斤拷锟斤拷=Y锟斤拷(锟斤拷)
    
    const float theta = inPlaneAngleDeg * (3.14159265358979323846f / 180.0f);
    const float c = cosf(theta);
    const float s = sinf(theta);
    const float L = 10.0f;

    // Rotated basis directions in screen NDC.
    const float dirHx = c;
    const float dirHy = s;
    const float dirVx = -s;
    const float dirVy = c;

    // 锟斤拷锟斤拷水平锟竭ｏ拷锟斤拷锟津穿癸拷锟斤拷幕锟斤拷 (rotated with slice)
    glBegin(GL_LINES);
    if (sliceDirection == 0) {
        glColor4f(0.0f, 1.0f, 0.0f, 0.8f);  // Axial: 锟斤拷锟斤拷=Y锟斤拷=锟斤拷色
    } else {
        glColor4f(0.0f, 0.0f, 1.0f, 0.8f);  // Coronal/Sagittal: 锟斤拷锟斤拷=Z锟斤拷=锟斤拷色
    }
    glVertex2f(crossX - dirHx * L, crossY - dirHy * L);
    glVertex2f(crossX + dirHx * L, crossY + dirHy * L);
    glEnd();
    
    // 锟斤拷锟狡达拷直锟竭ｏ拷锟斤拷锟津穿癸拷锟斤拷幕锟斤拷 (rotated with slice)
    glBegin(GL_LINES);
    if (sliceDirection == 0 || sliceDirection == 1) {
        glColor4f(1.0f, 0.0f, 0.0f, 0.8f);  // Axial/Coronal: 锟斤拷锟斤拷=X锟斤拷=锟斤拷色
    } else {
        glColor4f(0.0f, 1.0f, 0.0f, 0.8f);  // Sagittal: 锟斤拷锟斤拷=Y锟斤拷=锟斤拷色
    }
    glVertex2f(crossX - dirVx * L, crossY - dirVy * L);
    glVertex2f(crossX + dirVx * L, crossY + dirVy * L);
    glEnd();
    
    glDisable(GL_BLEND);
}

static uint8_t DebugGlyph5x7(char c, int row) {
    // 5x7 bitmap, row: 0..6, bit4..bit0.
    // Only includes the characters we need for quick diagnostics.
    switch (c) {
        case 'F': {
            static const uint8_t g[7] = { 0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000 };
            return g[row];
        }
        case 'G': {
            static const uint8_t g[7] = { 0b01111, 0b10000, 0b10000, 0b10111, 0b10001, 0b10001, 0b01111 };
            return g[row];
        }
        case 'H': {
            static const uint8_t g[7] = { 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001, 0b10001 };
            return g[row];
        }
        case 'K': {
            static const uint8_t g[7] = { 0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001 };
            return g[row];
        }
        case 'L': {
            static const uint8_t g[7] = { 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111 };
            return g[row];
        }
        case 'N': {
            static const uint8_t g[7] = { 0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001, 0b10001 };
            return g[row];
        }
        case 'O': {
            static const uint8_t g[7] = { 0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110 };
            return g[row];
        }
        case 'U': {
            static const uint8_t g[7] = { 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110 };
            return g[row];
        }
        case 'D': {
            static const uint8_t g[7] = { 0b11110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11110 };
            return g[row];
        }
        case 'T': {
            static const uint8_t g[7] = { 0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100 };
            return g[row];
        }
        case 'E': {
            static const uint8_t g[7] = { 0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111 };
            return g[row];
        }
        case 'S': {
            static const uint8_t g[7] = { 0b01111, 0b10000, 0b10000, 0b01110, 0b00001, 0b00001, 0b11110 };
            return g[row];
        }
        case ' ': {
            return 0;
        }
        default:
            return 0;
    }
}

static void DrawDebugTextTopLeftNDC(const char* text) {
    if (!text || !text[0]) return;

    // Render in NDC space (-1..1). This is intentionally independent of NanoVG.
    const float startX = -0.98f;
    const float startY = 0.94f;
    const float cellW = 0.014f;
    const float cellH = 0.020f;
    const float charGap = cellW * 1.5f;

    glDisable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f, 0.0f, 1.0f, 0.95f); // bright magenta

    float penX = startX;
    float penY = startY;

    glBegin(GL_QUADS);
    for (const char* p = text; *p; ++p) {
        const char c = *p;
        if (c == '\n') {
            penX = startX;
            penY -= cellH * 9.0f;
            continue;
        }

        for (int row = 0; row < 7; ++row) {
            const uint8_t bits = DebugGlyph5x7(c, row);
            if (!bits) continue;
            for (int col = 0; col < 5; ++col) {
                const bool on = (bits & (1u << (4 - col))) != 0;
                if (!on) continue;

                const float x0 = penX + col * cellW;
                const float y0 = penY - row * cellH;
                const float x1 = x0 + cellW * 0.9f;
                const float y1 = y0 - cellH * 0.9f;

                glVertex2f(x0, y0);
                glVertex2f(x1, y0);
                glVertex2f(x1, y1);
                glVertex2f(x0, y1);
            }
        }

        penX += (5.0f * cellW) + charGap;
    }
    glEnd();

    glDisable(GL_BLEND);
}

// Forward declaration (implementation is near NanoVG helpers).
static void DrawNanoVGDebugTextTopLeft(const char* text);

#ifdef _DEBUG
#pragma comment(lib, "glew32d.lib")
#else
#pragma comment(lib, "glew32.lib")
#endif
#if !defined(_WIN32)
#pragma comment(lib, "glfw3dll.lib")
#endif
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GlU32.lib")


struct RendererContext
{
    bool initialized = false;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    float lastSlice = 0.0f;
    std::uint32_t frameCount = 0;
};

static thread_local std::string g_lastError;

// 全锟斤拷 MPR 锟斤拷锟侥碉拷锟斤拷锟斤拷锟斤拷锟斤拷诙锟斤拷锟酵纪拷锟斤拷锟?
struct GlobalMPRCenter {
    float x = 0, y = 0, z = 0;
    VolumeHandle volume = nullptr;
    std::vector<MPRHandle> linkedMPRs;  // 锟斤拷锟斤拷锟斤拷要同锟斤拷锟斤拷 MPR
};
// DEPRECATED: Use TabSessionContext::mprCenter instead
static GlobalMPRCenter g_globalMPRCenter;

// 全锟斤拷 APR 锟斤拷锟侥碉拷锟斤拷锟斤拷锟斤拷锟斤拷诙锟斤拷锟酵纪拷锟斤拷锟?
struct GlobalAPRCenter {
    float x = 0, y = 0, z = 0;
    VolumeHandle volume = nullptr;
    std::vector<APRHandle> linkedAPRs;  // 锟斤拷锟斤拷锟斤拷要同锟斤拷锟斤拷 APR
    // Unified world rotation shared by all linked APRs.
    // Authoritative representation is a column-major 4x4 rotation matrix.
    float rotMat[16] = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };
};
// DEPRECATED: Use TabSessionContext::aprCenter instead
static GlobalAPRCenter g_globalAPRCenter;

static void SetLastError(const std::string& message)
{
    g_lastError = message;
}

static NativeResult ValidateHandle(RendererHandle handle, RendererContext*& outContext)
{
    if (!handle)
    {
        SetLastError("Renderer handle is null");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    outContext = static_cast<RendererContext*>(handle);
    return NATIVE_OK;
}

extern "C" __declspec(dllexport) const char* Renderer_GetLastError()
{
    return g_lastError.c_str();
}

extern "C" __declspec(dllexport) RendererHandle Renderer_Create()
{
    try
    {
        RendererContext* context = new RendererContext();
        std::cout << "[Renderer DLL] Renderer_Create called" << std::endl;
        std::cout << "[Renderer DLL] Renderer_Create success" << std::endl;
        return static_cast<RendererHandle>(context);
    }
    catch (const std::bad_alloc&)
    {
        SetLastError("Failed to allocate renderer context");
        std::cout << "[Renderer DLL] Renderer_Create failed" << std::endl;
        return nullptr;
    }
}

extern "C" __declspec(dllexport) void Renderer_Destroy(RendererHandle handle)
{
    if (!handle)
    {
        return;
    }

    RendererContext* context = nullptr;
    if (ValidateHandle(handle, context) != NATIVE_OK)
    {
        return;
    }

    std::cout << "[Renderer DLL] Renderer_Destroy called" << std::endl;
    if (!handle)
    {
        std::cout << "[Renderer DLL] Renderer_Destroy: handle is nullptr" << std::endl;
        return;
    }

    delete context;
    std::cout << "[Renderer DLL] Renderer_Destroy success" << std::endl;
}

extern "C" __declspec(dllexport) NativeResult Renderer_Initialize(RendererHandle handle, std::uint32_t width, std::uint32_t height)
{
    RendererContext* context = nullptr;
    NativeResult result = ValidateHandle(handle, context);
    if (result != NATIVE_OK)
    {
        return result;
    }

    if (width == 0 || height == 0)
    {
        SetLastError("Renderer surface dimensions must be non-zero");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    // Windowing is handled by Win32/WGL on Windows; GLFW is only used on non-Windows.
#if !defined(_WIN32)
    if (!glfwInit())
    {
        SetLastError("GLFW initialization failed");
        return NATIVE_E_INTERNAL_ERROR;
    }
#endif

    context->width = width;
    context->height = height;
    context->initialized = true;
    context->lastSlice = 0.0f;
    context->frameCount = 0;

    return NATIVE_OK;
}

extern "C" __declspec(dllexport) NativeResult Renderer_RenderSlice(RendererHandle handle, float sliceLocation)
{
    RendererContext* context = nullptr;
    NativeResult result = ValidateHandle(handle, context);
    if (result != NATIVE_OK)
    {
        return result;
    }

    if (!context->initialized)
    {
        SetLastError("Renderer not initialized");
        return NATIVE_E_NOT_INITIALIZED;
    }

    if (!std::isfinite(sliceLocation))
    {
        SetLastError("Slice location is not finite");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    context->lastSlice = sliceLocation;
    context->frameCount += 1;

    return NATIVE_OK;
}

extern "C" __declspec(dllexport) NativeResult Renderer_RunSelfTest(RendererDiagnostics* diagnostics)
{
    if (diagnostics == nullptr)
    {
        SetLastError("Diagnostics pointer is null");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    // Create a temporary OpenGL context to verify GLEW/OpenGL wiring.
#if defined(_WIN32)
    // Win32/WGL self-test (no GLFW on Windows)
    WNDCLASSW wc{};
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"RendererSelfTestWGL";
    RegisterClassW(&wc); // ok if already registered

    HWND hwnd = CreateWindowW(wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW,
                              0, 0, 1, 1, nullptr, nullptr, wc.hInstance, nullptr);
    if (!hwnd) {
        SetLastError("DllRenderer: Win32 window creation failed");
        return NATIVE_E_INTERNAL_ERROR;
    }

    HDC hdc = GetDC(hwnd);
    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pf = ChoosePixelFormat(hdc, &pfd);
    if (!pf || !SetPixelFormat(hdc, pf, &pfd)) {
        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);
        SetLastError("DllRenderer: SetPixelFormat failed");
        return NATIVE_E_INTERNAL_ERROR;
    }

    HGLRC rc = wglCreateContext(hdc);
    if (!rc || !wglMakeCurrent(hdc, rc)) {
        if (rc) wglDeleteContext(rc);
        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);
        SetLastError("DllRenderer: wglCreateContext/wglMakeCurrent failed");
        return NATIVE_E_INTERNAL_ERROR;
    }

    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK) {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(rc);
        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);
        SetLastError("DllRenderer: GLEW init failed");
        return NATIVE_E_INTERNAL_ERROR;
    }

    const GLubyte* glVersion = glGetString(GL_VERSION);
    const GLubyte* glewVersion = glewGetString(GLEW_VERSION);
    std::ostringstream oss;
    oss << "DllRenderer: OpenGL=" << (glVersion ? (const char*)glVersion : "unknown")
        << ", GLEW=" << (glewVersion ? (const char*)glewVersion : "unknown");
    SetLastError(oss.str());

    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(rc);
    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
#else
    // Non-Windows self-test uses GLFW
    if (!glfwInit())
    {
        SetLastError("DllRenderer: GLFW init failed");
        return NATIVE_E_INTERNAL_ERROR;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(640, 480, "OffscreenTest", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        SetLastError("DllRenderer: GLFW window creation failed");
        return NATIVE_E_INTERNAL_ERROR;
    }

    glfwMakeContextCurrent(window);
    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK)
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        SetLastError("DllRenderer: GLEW init failed");
        return NATIVE_E_INTERNAL_ERROR;
    }

    const GLubyte* glVersion = glGetString(GL_VERSION);
    const GLubyte* glewVersion = glewGetString(GLEW_VERSION);
    std::ostringstream oss;
    oss << "DllRenderer: OpenGL=" << (glVersion ? (const char*)glVersion : "unknown")
        << ", GLEW=" << (glewVersion ? (const char*)glewVersion : "unknown")
        << ", GLFW=" << glfwGetVersionString();
    SetLastError(oss.str());

    glfwDestroyWindow(window);
    glfwTerminate();
#endif

    RendererHandle handle = Renderer_Create();
    if (!handle)
    {
        return NATIVE_E_INTERNAL_ERROR;
    }

    NativeResult initResult = Renderer_Initialize(handle, 640, 480);
    if (initResult != NATIVE_OK)
    {
        Renderer_Destroy(handle);
        return initResult;
    }

    for (int frame = 0; frame < 5; ++frame)
    {
        const float sliceLocation = 0.5f * static_cast<float>(frame);
        NativeResult renderResult = Renderer_RenderSlice(handle, sliceLocation);
        if (renderResult != NATIVE_OK)
        {
            Renderer_Destroy(handle);
            return renderResult;
        }
    }

    RendererContext* context = static_cast<RendererContext*>(handle);
    diagnostics->surfaceWidth = context->width;
    diagnostics->surfaceHeight = context->height;
    diagnostics->lastSliceLocation = context->lastSlice;
    diagnostics->renderedFrameCount = context->frameCount;

    Renderer_Destroy(handle);
    return NATIVE_OK;
}

// ==================== 锟斤拷锟斤拷锟斤拷MPR/APR/Volume3D 实锟斤拷 ====================
#include "VisualizationApi.h"

// 全锟街憋拷锟斤拷锟斤拷锟斤拷锟斤拷 OpenGL 锟斤拷锟斤拷锟侥癸拷锟斤拷
#if !defined(_WIN32)
static GLFWwindow* g_sharedContextWindow = nullptr;
#endif

// 前锟斤拷锟斤拷锟斤拷
// VolumeContext is defined in ../Common/VolumeData.h

// MPR/APR/Volume3D 锟斤拷锟斤拷锟侥结构
struct MPRContext {
    VolumeHandle volume = nullptr;
    float centerX = 0, centerY = 0, centerZ = 0;
    int sliceDirection = 1;  // 0=Axial, 1=Coronal, 2=Sagittal (默锟较癸拷状锟斤拷)
    bool showCrossHair = true;
    GLuint textureID = 0;  // OpenGL 锟斤拷锟斤拷 ID
    std::vector<uint8_t> displayBuffer;  // 锟斤拷示锟斤拷锟斤拷锟斤拷
    int sliceWidth = 0, sliceHeight = 0;
    float zoomFactor = 1.0f;  // 锟斤拷锟斤拷锟斤拷锟接ｏ拷1.0 = 原始锟斤拷小锟斤拷

    // Window/level in HU
    // Fallback when DICOM does not provide defaults: show full 12-bit CT range.
    float windowWidthHU = 4096.0f;
    float windowLevelHU = 2048.0f;

    // Optional: associated session id (UTF-8). Set by MPR_RegisterSessionVolume.
    std::string sessionId;
    
    // Mask 锟斤拷锟斤拷支锟斤拷
    struct MaskOverlay {
        MaskManagerHandle manager = nullptr;
        int maskIndex = -1;
        float r = 1.0f, g = 1.0f, b = 1.0f, a = 0.5f;  // RGBA 锟斤拷色
        bool visible = true;
        GLuint cachedTextureID = 0;  // 锟斤拷锟斤拷锟斤拷锟斤拷锟絀D锟斤拷锟斤拷锟斤拷锟脚伙拷锟斤拷
        int cachedSliceIndex = -1;   // 锟斤拷锟斤拷锟斤拷锟狡拷锟斤拷锟?
    };
    std::vector<MaskOverlay> maskOverlays;
    bool showAllMasks = true;
    
    // Mask锟洁辑锟酵癸拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷值锟街割）
    struct MaskData {
        int id = -1;
        std::string name;
        std::string color;  // #rrggbb锟斤拷式
        bool visible = true;
        float minThreshold = -1000.0f;
        float maxThreshold = 3000.0f;
        std::vector<uint8_t> data;  // mask锟斤拷锟捷ｏ拷锟斤拷volume同锟斤拷锟斤拷小锟斤拷
    };
    std::vector<MaskData> masks;  // 锟斤拷锟斤拷permanent masks
    MaskData* previewMask = nullptr;  // 锟斤拷时预锟斤拷mask

    // Revision counter for mask content/visibility changes.
    // Used by the 3D raycaster to avoid re-uploading mask volumes every frame.
    uint64_t maskRevision = 1;
    
    // 直锟斤拷图锟斤拷锟斤拷
    int histogram[256] = {0};
    int histogramMinValue = -1000;
    int histogramMaxValue = 3000;
    bool histogramCalculated = false;
};

// Session锟斤拷锟斤拷锟斤拷锟斤拷锟节存储MPR锟斤拷锟斤拷锟侥ｏ拷
struct SessionContext {
    std::string sessionId;
    MPRHandle mprHandle = nullptr;
    VolumeHandle volumeHandle = nullptr;
};

// 全锟斤拷Session映锟斤拷
static std::map<std::string, SessionContext> g_Sessions;

// ==================== Per-Tab Session Context ====================
// All data that belongs to a single tab/session should be stored here.
// This ensures complete isolation between tabs and avoids global state conflicts.

struct TabSessionContext {
    std::string sessionId;
    
    // ==================== Volume and Rendering ====================
    VolumeHandle volumeHandle = nullptr;
    MPRHandle mprHandle = nullptr;
    
    // All APR handles for this session (multiple views sharing same volume)
    std::vector<APRHandle> linkedAPRs;
    
    // Cropped volume result
    APRHandle croppedAPR = nullptr;
    
    // ==================== Crop Box State ====================
    GlobalAPRCropBox cropBox;
    
    // ==================== 3D View State ====================
    float rotX = 30.0f;
    float rotY = 45.0f;
    float zoom = 1.0f;
    float panX = 0.0f;
    float panY = 0.0f;
    float rotMat[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    bool rotating = false;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;
    
    // ==================== APR/MPR Center (shared by linked views) ====================
    GlobalAPRCenter aprCenter;
    GlobalMPRCenter mprCenter;
    
    // ==================== Measurement State ====================
    std::vector<CompletedMeasurement> completedMeasurements;
    std::mutex measurementsMutex;
    int nextMeasurementId = 1;
    int hoverMeasurementIndex = -1;
    int hoverPointIndex = -1;
    int dragMeasurementIndex = -1;
    int dragPointIndex = -1;
    bool isDraggingPoint = false;
    std::vector<MeasurementPoint> measurementPoints;
    MeasurementPoint currentMousePos = {0.0f, 0.0f, 0.0f};
    MeasurementLocation currentMeasurementLocation;
    bool isDrawing = false;
    int currentToolType = 0;  // 0=navigate, 1+=measurement tools
    int lastMeasurementTool = 1;
    bool shiftPressed = false;
    double lastClickTime = 0.0;
    
    // ==================== Mask Editing State ====================
    int currentMaskIndex = -1;
    float brushRadius = 5.0f;
    void* currentMaskManager = nullptr;
    int currentMaskTool = 1;  // 1=brush, 2=eraser, etc.
    std::string maskEditSessionId;
    int maskEditMaskId = -1;
    std::vector<Point2D> maskStrokePath;
    bool maskStrokeNeedsUpdate = false;
    
    // ==================== Windows belonging to this tab ====================
    std::vector<WindowHandle> windows;
    
    // ==================== 3D Primitives ====================
    std::vector<Primitive3D> primitives;
    int nextPrimitiveId = 1;
    Scene3DTransform sceneTransform;
    
    // ==================== Constructor/Destructor ====================
    TabSessionContext() {
        // Initialize rotation matrix to identity
        rotMat[0] = rotMat[5] = rotMat[10] = rotMat[15] = 1.0f;
        rotMat[1] = rotMat[2] = rotMat[3] = rotMat[4] = 0.0f;
        rotMat[6] = rotMat[7] = rotMat[8] = rotMat[9] = 0.0f;
        rotMat[11] = rotMat[12] = rotMat[13] = rotMat[14] = 0.0f;
    }
    
    ~TabSessionContext() {
        // Resources should be cleaned up via Session_Destroy before destruction
    }
};

// Global map of all tab sessions
static std::map<std::string, std::unique_ptr<TabSessionContext>> g_TabSessions;
static std::mutex g_TabSessionMutex;

// Helper to get or create a tab session context
static TabSessionContext* GetTabSession(const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(g_TabSessionMutex);
    auto it = g_TabSessions.find(sessionId);
    if (it != g_TabSessions.end()) {
        return it->second.get();
    }
    // Create new session
    auto ctx = std::make_unique<TabSessionContext>();
    ctx->sessionId = sessionId;
    TabSessionContext* ptr = ctx.get();
    g_TabSessions[sessionId] = std::move(ctx);
    return ptr;
}

// Helper to get existing tab session (returns nullptr if not found)
static TabSessionContext* FindTabSession(const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(g_TabSessionMutex);
    auto it = g_TabSessions.find(sessionId);
    if (it != g_TabSessions.end()) {
        return it->second.get();
    }
    return nullptr;
}

// Destroy a tab session and all its resources
static void DestroyTabSession(const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(g_TabSessionMutex);
    auto it = g_TabSessions.find(sessionId);
    if (it != g_TabSessions.end()) {
        g_TabSessions.erase(it);
    }
}

// ==================== Session State Access Helpers ====================
// These helpers provide easy access to session-specific state
// They first try TabSessionContext, then fall back to global variables

// Legacy globals used by session helpers. Keep definitions here so we only define once.
static int g_currentToolType = 0;
static std::vector<CompletedMeasurement> g_completedMeasurements;
static int g_nextMeasurementId = 1;
static int g_currentMaskIndex = -1;
static float g_brushRadius = 5.0f;
static int g_currentMaskTool = 1;
static bool g_isDrawing = false;

// Get crop box for a session
static GlobalAPRCropBox* GetSessionCropBox(const std::string& sessionId) {
    TabSessionContext* ctx = FindTabSession(sessionId);
    if (ctx) return &ctx->cropBox;
    return &g_aprCropBox;
}

// Get APR center for a session
static GlobalAPRCenter* GetSessionAPRCenter(const std::string& sessionId) {
    TabSessionContext* ctx = FindTabSession(sessionId);
    if (ctx) return &ctx->aprCenter;
    return &g_globalAPRCenter;
}

// Get MPR center for a session
static GlobalMPRCenter* GetSessionMPRCenter(const std::string& sessionId) {
    TabSessionContext* ctx = FindTabSession(sessionId);
    if (ctx) return &ctx->mprCenter;
    return &g_globalMPRCenter;
}

// Get 3D rotation matrix for a session
static float* GetSessionRotMat(const std::string& sessionId) {
    TabSessionContext* ctx = FindTabSession(sessionId);
    if (ctx) return ctx->rotMat;
    return g_3dRotMat;
}

// Get/Set 3D view state
static void GetSession3DState(const std::string& sessionId,
                               float* rotX, float* rotY, float* zoom, float* panX, float* panY) {
    TabSessionContext* ctx = FindTabSession(sessionId);
    if (ctx) {
        if (rotX) *rotX = ctx->rotX;
        if (rotY) *rotY = ctx->rotY;
        if (zoom) *zoom = ctx->zoom;
        if (panX) *panX = ctx->panX;
        if (panY) *panY = ctx->panY;
    } else {
        if (rotX) *rotX = g_3dRotX;
        if (rotY) *rotY = g_3dRotY;
        if (zoom) *zoom = g_3dZoom;
        if (panX) *panX = g_3dPanX;
        if (panY) *panY = g_3dPanY;
    }
}

static void SetSession3DState(const std::string& sessionId,
                               float rotX, float rotY, float zoom, float panX, float panY) {
    TabSessionContext* ctx = FindTabSession(sessionId);
    if (ctx) {
        ctx->rotX = rotX;
        ctx->rotY = rotY;
        ctx->zoom = zoom;
        ctx->panX = panX;
        ctx->panY = panY;
    }
    g_3dRotX = rotX; g_3dRotY = rotY; g_3dZoom = zoom;
    g_3dPanX = panX; g_3dPanY = panY;
}

// Get current tool type
static int GetSessionToolType(const std::string& sessionId) {
    TabSessionContext* ctx = FindTabSession(sessionId);
    return ctx ? ctx->currentToolType : g_currentToolType;
}

static void SetSessionToolType(const std::string& sessionId, int toolType) {
    TabSessionContext* ctx = FindTabSession(sessionId);
    if (ctx) ctx->currentToolType = toolType;
    g_currentToolType = toolType;
}

// Get measurement state
static std::vector<CompletedMeasurement>& GetSessionMeasurements(const std::string& sessionId) {
    TabSessionContext* ctx = FindTabSession(sessionId);
    return ctx ? ctx->completedMeasurements : g_completedMeasurements;
}

static int GetSessionNextMeasurementId(const std::string& sessionId) {
    TabSessionContext* ctx = FindTabSession(sessionId);
    return ctx ? ctx->nextMeasurementId++ : g_nextMeasurementId++;
}

// Mask editing state helpers
static int GetSessionMaskIndex(const std::string& sessionId) {
    TabSessionContext* ctx = FindTabSession(sessionId);
    return ctx ? ctx->currentMaskIndex : g_currentMaskIndex;
}

static void SetSessionMaskIndex(const std::string& sessionId, int maskIndex) {
    TabSessionContext* ctx = FindTabSession(sessionId);
    if (ctx) ctx->currentMaskIndex = maskIndex;
    g_currentMaskIndex = maskIndex;
}

static float GetSessionBrushRadius(const std::string& sessionId) {
    TabSessionContext* ctx = FindTabSession(sessionId);
    return ctx ? ctx->brushRadius : g_brushRadius;
}

static void SetSessionBrushRadius(const std::string& sessionId, float radius) {
    TabSessionContext* ctx = FindTabSession(sessionId);
    if (ctx) ctx->brushRadius = radius;
    g_brushRadius = radius;
}

static int GetSessionMaskTool(const std::string& sessionId) {
    TabSessionContext* ctx = FindTabSession(sessionId);
    return ctx ? ctx->currentMaskTool : g_currentMaskTool;
}

static void SetSessionMaskTool(const std::string& sessionId, int tool) {
    TabSessionContext* ctx = FindTabSession(sessionId);
    if (ctx) ctx->currentMaskTool = tool;
    g_currentMaskTool = tool;
}

// Drawing state helpers
static bool GetSessionIsDrawing(const std::string& sessionId) {
    TabSessionContext* ctx = FindTabSession(sessionId);
    return ctx ? ctx->isDrawing : g_isDrawing;
}

static void SetSessionIsDrawing(const std::string& sessionId, bool drawing) {
    TabSessionContext* ctx = FindTabSession(sessionId);
    if (ctx) ctx->isDrawing = drawing;
    g_isDrawing = drawing;
}


static std::mutex g_SessionMutex;

// 锟芥储锟斤拷锟叫达拷锟斤拷锟侥达拷锟节ｏ拷锟斤拷锟斤拷Window_InvalidateAll锟斤拷
static std::vector<WindowHandle> g_AllWindows;

// ==================== Minimal matrix helpers (OpenGL column-major) ====================
static void Mat4_Identity(float m[16]) {
    for (int i = 0; i < 16; ++i) m[i] = 0.0f;
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

// out = a * b (column-major)
static void Mat4_Mul(float out[16], const float a[16], const float b[16]) {
    float r[16];
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            r[col * 4 + row] =
                a[0 * 4 + row] * b[col * 4 + 0] +
                a[1 * 4 + row] * b[col * 4 + 1] +
                a[2 * 4 + row] * b[col * 4 + 2] +
                a[3 * 4 + row] * b[col * 4 + 3];
        }
    }
    std::memcpy(out, r, sizeof(float) * 16);
}

static void Mat4_Translation(float out[16], float tx, float ty, float tz) {
    Mat4_Identity(out);
    out[12] = tx;
    out[13] = ty;
    out[14] = tz;
}

static void Mat4_Scale(float out[16], float sx, float sy, float sz) {
    Mat4_Identity(out);
    out[0] = sx;
    out[5] = sy;
    out[10] = sz;
}

static void Mat4_RotationAxisAngle(float out[16], float ax, float ay, float az, float deg);

// Match OpenGL fixed pipeline order used elsewhere: M = T * Rx * Ry * Rz * S
static void Mat4_TRS(float out[16],
    float tx, float ty, float tz,
    float rxDeg, float ryDeg, float rzDeg,
    float sx, float sy, float sz
) {
    float t[16], rx[16], ry[16], rz[16], s[16];
    float tmp1[16], tmp2[16], tmp3[16];
    Mat4_Translation(t, tx, ty, tz);
    Mat4_RotationAxisAngle(rx, 1.0f, 0.0f, 0.0f, rxDeg);
    Mat4_RotationAxisAngle(ry, 0.0f, 1.0f, 0.0f, ryDeg);
    Mat4_RotationAxisAngle(rz, 0.0f, 0.0f, 1.0f, rzDeg);
    Mat4_Scale(s, sx, sy, sz);

    Mat4_Mul(tmp1, t, rx);
    Mat4_Mul(tmp2, tmp1, ry);
    Mat4_Mul(tmp3, tmp2, rz);
    Mat4_Mul(out, tmp3, s);
}

// Column-major OpenGL-style frustum matrix.
static void Mat4_Frustum(float out[16], float l, float r, float b, float t, float n, float f) {
    Mat4_Identity(out);
    out[0] = (2.0f * n) / (r - l);
    out[5] = (2.0f * n) / (t - b);
    out[8] = (r + l) / (r - l);
    out[9] = (t + b) / (t - b);
    out[10] = -(f + n) / (f - n);
    out[11] = -1.0f;
    out[14] = -(2.0f * f * n) / (f - n);
    out[15] = 0.0f;
}

static void Mat4_RotationAxisAngle(float out[16], float ax, float ay, float az, float deg) {
    const float len = std::sqrt(ax * ax + ay * ay + az * az);
    if (len <= 1e-6f) {
        Mat4_Identity(out);
        return;
    }

    ax /= len; ay /= len; az /= len;
    const float rad = deg * 3.14159265358979323846f / 180.0f;
    const float c = std::cos(rad);
    const float s = std::sin(rad);
    const float t = 1.0f - c;

    // Rodrigues' rotation formula (3x3) expanded into 4x4
    Mat4_Identity(out);
    out[0] = t * ax * ax + c;
    out[4] = t * ax * ay - s * az;
    out[8] = t * ax * az + s * ay;

    out[1] = t * ax * ay + s * az;
    out[5] = t * ay * ay + c;
    out[9] = t * ay * az - s * ax;

    out[2] = t * ax * az - s * ay;
    out[6] = t * ay * az + s * ax;
    out[10] = t * az * az + c;
}

static void Mat4_GetAxisX(const float m[16], float outAxis[3]) {
    outAxis[0] = m[0];
    outAxis[1] = m[1];
    outAxis[2] = m[2];
}

static void Mat4_GetAxisY(const float m[16], float outAxis[3]) {
    outAxis[0] = m[4];
    outAxis[1] = m[5];
    outAxis[2] = m[6];
}

static void Mat4_MulVec3_3x3(const float m[16], const float v[3], float outV[3]) {
    // out = R * v using the top-left 3x3 of a column-major mat4
    outV[0] = m[0] * v[0] + m[4] * v[1] + m[8] * v[2];
    outV[1] = m[1] * v[0] + m[5] * v[1] + m[9] * v[2];
    outV[2] = m[2] * v[0] + m[6] * v[1] + m[10] * v[2];
}

static bool Mat4_IsIdentity(const float m[16], float eps = 1e-4f) {
    static const float I[16] = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };
    for (int i = 0; i < 16; ++i) {
        if (std::fabs(m[i] - I[i]) > eps) return false;
    }
    return true;
}

static void Mat4_Orthonormalize3x3(float m[16]) {
    // Columns represent basis vectors (x,y,z). Re-orthonormalize to avoid drift.
    float x[3] = { m[0], m[1], m[2] };
    float y[3] = { m[4], m[5], m[6] };

    auto norm3 = [](float v[3]) {
        const float len = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
        if (len > 1e-6f) { v[0] /= len; v[1] /= len; v[2] /= len; }
    };
    auto dot3 = [](const float a[3], const float b[3]) {
        return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
    };
    auto cross3 = [](const float a[3], const float b[3], float out[3]) {
        out[0] = a[1]*b[2] - a[2]*b[1];
        out[1] = a[2]*b[0] - a[0]*b[2];
        out[2] = a[0]*b[1] - a[1]*b[0];
    };

    norm3(x);
    // y = y - proj_x(y)
    const float d = dot3(y, x);
    y[0] -= d * x[0];
    y[1] -= d * x[1];
    y[2] -= d * x[2];
    norm3(y);

    float z[3];
    cross3(x, y, z);
    norm3(z);

    m[0] = x[0]; m[1] = x[1]; m[2] = x[2];
    m[4] = y[0]; m[5] = y[1]; m[6] = y[2];
    m[8] = z[0]; m[9] = z[1]; m[10] = z[2];
    // Keep translation/last row untouched.
    m[3] = 0.0f; m[7] = 0.0f; m[11] = 0.0f;
    m[12] = 0.0f; m[13] = 0.0f; m[14] = 0.0f; m[15] = 1.0f;
}

// Extract Euler angles (degrees) from a column-major rotation matrix.
// Convention matches APR_SetRotation: R = Rz * Ry * Rx (applied to column vectors).
static void Mat4_ExtractEulerZYXDeg(const float m[16], float* outXDeg, float* outYDeg, float* outZDeg) {
    const float r00 = m[0];
    const float r01 = m[4];
    const float r02 = m[8];

    const float r10 = m[1];
    const float r11 = m[5];
    const float r12 = m[9];

    const float r20 = m[2];
    const float r21 = m[6];
    const float r22 = m[10];

    // For ZYX (yaw-pitch-roll): pitchY = asin(-r20), rollX = atan2(r21, r22), yawZ = atan2(r10, r00)
    float sy = -r20;
    if (sy < -1.0f) sy = -1.0f;
    if (sy > 1.0f) sy = 1.0f;

    const float y = std::asin(sy);
    const float cy = std::cos(y);

    float x = 0.0f;
    float z = 0.0f;

    if (std::fabs(cy) > 1e-6f) {
        x = std::atan2(r21, r22);
        z = std::atan2(r10, r00);
    } else {
        // Gimbal lock: choose roll=0 and solve yaw from remaining terms.
        // Works for pitch near +/- 90deg.
        (void)r02;
        (void)r12;
        x = 0.0f;
        z = std::atan2(-r01, r11);
    }

    const float rad2deg = 180.0f / 3.14159265358979323846f;
    if (outXDeg) *outXDeg = x * rad2deg;
    if (outYDeg) *outYDeg = y * rad2deg;
    if (outZDeg) *outZDeg = z * rad2deg;
}

static void Win32_ArcballVec(int width, int height, double x, double y, float outV[3]) {
    // Map screen to [-1,1]
    const float w = (width > 0) ? static_cast<float>(width) : 1.0f;
    const float h = (height > 0) ? static_cast<float>(height) : 1.0f;
    float nx = static_cast<float>((2.0 * x - w) / w);
    float ny = static_cast<float>((h - 2.0 * y) / h);

    // Project to sphere (arcball)
    const float len2 = nx * nx + ny * ny;
    float nz = 0.0f;
    if (len2 <= 1.0f) {
        nz = std::sqrt(std::max(0.0f, 1.0f - len2));
    } else {
        const float invLen = 1.0f / std::sqrt(len2);
        nx *= invLen;
        ny *= invLen;
        nz = 0.0f;
    }
    outV[0] = nx;
    outV[1] = ny;
    outV[2] = nz;
}

struct APRContext;
static void APR_RotateCrosshairInSlice(APRContext* ctx, int volW, int volH, int volD, float deltaDeg);

struct APRContext {
    VolumeHandle volume = nullptr;
    float centerX = 0, centerY = 0, centerZ = 0;
    int sliceDirection = 1;  // 0=Axial, 1=Coronal, 2=Sagittal (默锟较癸拷状锟斤拷)
    // World rotation of the volume (column-major 4x4). This is the source of truth.
    float rotMat[16] = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };
    bool showCrossHair = true;
    GLuint textureID = 0;
    std::vector<uint8_t> displayBuffer;
    int sliceWidth = 0, sliceHeight = 0;
    float zoomFactor = 1.0f;  // 锟斤拷锟斤拷锟斤拷锟接ｏ拷1.0 = 原始锟斤拷小锟斤拷
    bool orthogonal3DMode = false;  // 锟角凤拷锟斤拷锟斤拷3D锟斤拷锟斤拷模式

    // Window/level in HU
    // Fallback when DICOM does not provide defaults: show full 12-bit CT range.
    float windowWidthHU = 4096.0f;
    float windowLevelHU = 2048.0f;
    
    // MIP/MinIP projection mode
    // 0 = Normal (single slice), 1 = MIP (max intensity), 2 = MinIP (min intensity)
    int projectionMode = 0;
    float projectionThickness = 10.0f;  // Thickness in voxels for MIP/MinIP

    // Optional: bind to an MPR session (registered via MPR_RegisterSessionVolume)
    // so APR_Render can draw masks/preview masks from that session.
    std::string sessionId;
    
    // 锟斤拷位锟斤拷锟斤拷锟斤拷锟狡斤拷锟斤拷2D锟斤拷锟疥（锟斤拷锟斤拷锟斤拷锟斤拷平锟芥，未锟斤拷转锟斤拷锟斤拷系锟斤拷
    // 锟斤拷锟斤拷隙锟绞敝憋拷锟斤拷薷锟斤拷锟斤拷值锟斤拷锟斤拷染时通锟斤拷锟斤拷转锟斤拷锟斤拷转锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
    float crosshairU = 0.0f;  // 锟斤拷锟狡斤拷锟斤拷U锟斤拷锟疥（锟斤拷锟斤拷Axial锟斤拷X锟斤拷
    float crosshairV = 0.0f;  // 锟斤拷锟狡斤拷锟斤拷V锟斤拷锟疥（锟斤拷锟斤拷Axial锟斤拷Y锟斤拷
    
    // 锟斤拷锟叫猴拷锟斤拷锟斤拷锟斤拷荩锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟絥ullptr锟斤拷锟斤拷使锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷volume锟斤拷
    VolumeContext* croppedVolumeData = nullptr;
    bool ownsVolumeData = true;  // 锟角凤拷拥锟斤拷croppedVolumeData锟斤拷锟斤拷锟斤拷删锟斤拷时锟叫断ｏ拷
};

static void APR_RotateCrosshairInSlice(APRContext* ctx, int volW, int volH, int volD, float deltaDeg) {
    if (!ctx) return;
    if (volW <= 1 || volH <= 1 || volD <= 1) return;
    if (deltaDeg == 0.0f) return;

    int sliceW = 0;
    int sliceH = 0;
    float centerU = 0.0f;
    float centerV = 0.0f;

    if (ctx->sliceDirection == 0) {          // Axial: U=X, V=Y
        sliceW = volW;
        sliceH = volH;
        centerU = ctx->centerX;
        centerV = ctx->centerY;
    } else if (ctx->sliceDirection == 1) {   // Coronal: U=X, V=Z
        sliceW = volW;
        sliceH = volD;
        centerU = ctx->centerX;
        centerV = ctx->centerZ;
    } else {                                 // Sagittal: U=Y, V=Z
        sliceW = volH;
        sliceH = volD;
        centerU = ctx->centerY;
        centerV = ctx->centerZ;
    }

    if (sliceW <= 1 || sliceH <= 1) return;

    const float invWm1 = 1.0f / std::max(1.0f, (static_cast<float>(sliceW) - 1.0f));
    const float invHm1 = 1.0f / std::max(1.0f, (static_cast<float>(sliceH) - 1.0f));

    // Texture space: +Y is up (matches crossTexY = 1 - v/(H-1)).
    const float crossTexX = ctx->crosshairU * invWm1;
    const float crossTexY = 1.0f - (ctx->crosshairV * invHm1);
    const float centerTexX = centerU * invWm1;
    const float centerTexY = 1.0f - (centerV * invHm1);

    const float dx = crossTexX - centerTexX;
    const float dy = crossTexY - centerTexY;

    const float rad = deltaDeg * (3.14159265358979323846f / 180.0f);
    const float c = std::cos(rad);
    const float s = std::sin(rad);
    float ndx = c * dx - s * dy;
    float ndy = s * dx + c * dy;

    float newCrossTexX = centerTexX + ndx;
    float newCrossTexY = centerTexY + ndy;

    if (newCrossTexX < 0.0f) newCrossTexX = 0.0f;
    if (newCrossTexX > 1.0f) newCrossTexX = 1.0f;
    if (newCrossTexY < 0.0f) newCrossTexY = 0.0f;
    if (newCrossTexY > 1.0f) newCrossTexY = 1.0f;

    ctx->crosshairU = newCrossTexX * (static_cast<float>(sliceW) - 1.0f);
    ctx->crosshairV = (1.0f - newCrossTexY) * (static_cast<float>(sliceH) - 1.0f);
}

struct Volume3DContext {
    std::vector<VolumeHandle> volumes;
    TransferFunctionHandle transferFunction = nullptr;
    float ambient = 0.3f, diffuse = 0.6f, specular = 0.3f;
};

struct TransferFunctionContext {
    struct ControlPoint {
        float value, r, g, b, a;
    };
    std::vector<ControlPoint> points;
};

// 锟斤拷锟斤拷傻牟锟斤拷锟酵硷拷锟饺拷锟斤拷斜锟?
static std::mutex g_completedMeasurementsMutex;

static int g_hoverMeasurementIndex = -1;
static int g_hoverPointIndex = -1;
static int g_dragMeasurementIndex = -1;
static int g_dragPointIndex = -1;
static bool g_isDraggingPoint = false;

static std::vector<MeasurementPoint> g_measurementPoints;
static MeasurementPoint g_currentMousePos = {0.0f, 0.0f, 0.0f};  // 锟斤拷前锟斤拷锟轿伙拷茫锟?D锟斤拷锟斤拷锟斤拷锟疥）
static MeasurementLocation g_currentMeasurementLocation;  // 锟斤拷前锟斤拷锟节伙拷锟狡的诧拷锟斤拷锟斤拷堑锟轿伙拷锟斤拷锟较?
// g_currentToolType: 0=CrossHair(锟斤拷位锟斤拷), 1=Line, 2=Angle, 3=Rect, 4=Circle, 5=CatmullRom, 6=Freehand, 7=MaskEdit(Mask锟洁辑模式)
static int g_lastMeasurementTool = 1;  // 锟斤拷录锟斤拷一锟斤拷使锟矫的诧拷锟斤拷锟斤拷锟竭ｏ拷锟斤拷锟节帮拷0锟斤拷锟叫伙拷
static bool g_shiftPressed = false;  // Shift锟斤拷锟角凤拷锟铰ｏ拷锟斤拷锟斤拷约锟斤拷锟斤拷
static double g_lastClickTime = 0.0;  // 锟较次碉拷锟绞憋拷洌拷锟斤拷诩锟斤拷双锟斤拷锟斤拷

// Mask锟洁辑锟斤拷锟斤拷全锟街憋拷锟斤拷
static void* g_currentMaskManager = nullptr;  // 锟斤拷前使锟矫碉拷MaskManager锟斤拷锟斤拷锟节伙拷锟绞癸拷锟竭ｏ拷
// g_currentMaskTool: 1=Brush(锟斤拷锟斤拷), 2=Eraser(锟斤拷皮锟斤拷), 3=RectROI, 4=CircleROI, 5=PolygonROI, 6=FloodFill, 7=ConnectedComponent

// Session-permanent mask editing selection (sessionId + maskId)
static std::string g_currentSessionMaskEditSessionId;
static int g_currentSessionMaskId = -1;

static uint8_t* GetCurrentEditableMaskData(MPRContext* mprCtx, const char** outName) {
    if (!mprCtx || !mprCtx->volume) return nullptr;

    if (g_currentMaskManager != nullptr && g_currentMaskIndex >= 0) {
        if (outName) {
            *outName = MaskManager_GetName(g_currentMaskManager, g_currentMaskIndex);
        }
        const uint8_t* constMaskData = MaskManager_GetData(g_currentMaskManager, g_currentMaskIndex);
        return const_cast<uint8_t*>(constMaskData);
    }

    if (!g_currentSessionMaskEditSessionId.empty() && g_currentSessionMaskId >= 0 &&
        mprCtx->sessionId == g_currentSessionMaskEditSessionId) {

        auto it = std::find_if(mprCtx->masks.begin(), mprCtx->masks.end(),
            [&](const MPRContext::MaskData& m) { return m.id == g_currentSessionMaskId; });
        if (it == mprCtx->masks.end()) return nullptr;
        if (outName) *outName = it->name.c_str();
        if (it->data.empty()) return nullptr;
        return it->data.data();
    }

    return nullptr;
}

// Mask锟洁辑锟绞伙拷路锟斤拷锟斤拷锟斤拷锟斤拷预锟斤拷锟斤拷一锟斤拷锟斤拷栅锟今化ｏ拷
static std::vector<Point2D> g_maskStrokePath;  // 锟斤拷前锟绞伙拷锟斤拷2D锟斤拷锟斤拷锟斤拷锟斤拷路锟斤拷
static bool g_maskStrokeNeedsUpdate = false;   // 锟角凤拷锟斤拷要锟斤拷锟斤拷mask锟斤拷锟斤拷

// NanoVG锟斤拷锟斤拷锟侥ｏ拷锟斤拷锟节革拷锟斤拷锟斤拷圆锟轿端碉拷锟斤拷锟竭伙拷锟狡ｏ拷
static NVGcontext* g_nvgContext = nullptr;
static int g_nvgFontId = -1;
static HGLRC g_nvgGlrc = nullptr;

static void PrepareGLForNanoVGOverlay() {
    // After hide/show or switching tabs, GL state can differ (depth/scissor/blend).
    // Force a known-good 2D overlay state so NanoVG reliably shows HUD/markers.
    // Also ensure we draw to the default framebuffer and are not affected by
    // leftover shader/FBO/stencil state from other passes.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);

    glDisable(GL_STENCIL_TEST);
    glStencilMask(0xFFFFFFFF);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static void EnsureNanoVGReady() {
    // NanoVG GL backend stores GL objects; if the current HGLRC changes (common after
    // hide/show, tab switching, or multi-window context rebinding), those objects may
    // no longer be valid/visible. Detect and recreate NanoVG resources.
    const HGLRC currentGlrc = wglGetCurrentContext();
    if (g_nvgContext && currentGlrc && (!g_nvgGlrc || currentGlrc != g_nvgGlrc)) {
        nvgDeleteGL3(g_nvgContext);
        g_nvgContext = nullptr;
        g_nvgFontId = -1;
    }

    if (!g_nvgContext) {
        // Try with stencil strokes first, then fall back (some GL setups lack stencil).
        g_nvgContext = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
        if (!g_nvgContext) {
            g_nvgContext = nvgCreateGL3(NVG_ANTIALIAS);
        }
        g_nvgGlrc = currentGlrc;
    }

    if (g_nvgContext && g_nvgFontId < 0) {
        g_nvgFontId = nvgCreateFont(g_nvgContext, "ui", "C:\\Windows\\Fonts\\arial.ttf");
        if (g_nvgFontId < 0) {
            g_nvgFontId = nvgCreateFont(g_nvgContext, "ui", "C:\\Windows\\Fonts\\segoeui.ttf");
        }
    }
}

static void DrawNanoVGDebugTextTopLeft(const char* text) {
    if (!text || !text[0]) return;

    EnsureNanoVGReady();
    if (!g_nvgContext) return;

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    const int winWidth = viewport[2];
    const int winHeight = viewport[3];
    if (winWidth <= 0 || winHeight <= 0) return;

    PrepareGLForNanoVGOverlay();
    nvgBeginFrame(g_nvgContext, winWidth, winHeight, 1.0f);
    nvgSave(g_nvgContext);

    const float x = 12.0f;
    const float y = 12.0f + 24.0f; // 2nd line under the GL probe

    if (g_nvgFontId >= 0) {
        nvgFontSize(g_nvgContext, 16.0f);
        nvgFontFace(g_nvgContext, "ui");
        nvgTextAlign(g_nvgContext, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

        // Shadow then green text
        nvgFillColor(g_nvgContext, nvgRGBA(0, 0, 0, 200));
        nvgText(g_nvgContext, x + 1.0f, y + 1.0f, text, nullptr);
        nvgFillColor(g_nvgContext, nvgRGBA(0, 255, 0, 230));
        nvgText(g_nvgContext, x, y, text, nullptr);
    } else {
        // Font failed to load: still draw a visible indicator box.
        nvgBeginPath(g_nvgContext);
        nvgRect(g_nvgContext, x, y, 120.0f, 18.0f);
        nvgStrokeWidth(g_nvgContext, 2.0f);
        nvgStrokeColor(g_nvgContext, nvgRGBA(255, 0, 0, 220));
        nvgStroke(g_nvgContext);
    }

    nvgRestore(g_nvgContext);
    nvgEndFrame(g_nvgContext);
}

static void DrawSliceHeaderNVG_InFrame(int winWidth, int winHeight, const char* text) {
    if (!g_nvgContext || g_nvgFontId < 0) return;
    if (winWidth <= 0 || winHeight <= 0) return;
    if (!text || !text[0]) return;

    const float marginTop = 10.0f;
    const float fontSize = 14.0f;

    nvgSave(g_nvgContext);
    nvgFontSize(g_nvgContext, fontSize);
    nvgFontFace(g_nvgContext, "ui");
    nvgTextAlign(g_nvgContext, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);

    const float x = (float)winWidth * 0.5f;
    const float y = marginTop;

    // No background plate; draw text only.
    nvgFillColor(g_nvgContext, nvgRGBA(0, 0, 0, 180));
    nvgText(g_nvgContext, x + 1.0f, y + 1.0f, text, nullptr);
    nvgFillColor(g_nvgContext, nvgRGBA(255, 255, 255, 230));
    nvgText(g_nvgContext, x, y, text, nullptr);

    nvgRestore(g_nvgContext);
}

// 导出的辅助函数，供ImageBrowserOrthogonal3DRenderer使用
void DrawVerticalScaleBarNVG_InFrame(int winWidth, int winHeight, int sliceDirection, float zoomFactor, VolumeHandle volume) {
    if (!g_nvgContext) return;
    if (winWidth <= 0 || winHeight <= 0) return;
    if (!volume) return;

    float spacingX = 0.0f, spacingY = 0.0f, spacingZ = 0.0f;
    if (Dicom_Volume_GetSpacing(volume, &spacingX, &spacingY, &spacingZ) != NATIVE_OK) {
        return;
    }

    const float margin = 12.0f;
    const float lineH = 18.0f;
    const float topReserve = 10.0f + 16.0f + 8.0f;
    const float bottomReserve = margin + (lineH * 2.0f + 6.0f);
    const float desiredH = (float)winHeight * 0.8f;
    float barH = desiredH;
    const float maxH = (float)winHeight - topReserve - bottomReserve;
    if (maxH > 40.0f && barH > maxH) barH = maxH;
    if (barH < 40.0f) barH = 40.0f;
    float barY = topReserve;
    if (barY + barH > (float)winHeight - bottomReserve) {
        barY = (float)winHeight - bottomReserve - barH;
    }
    if (barY < margin) barY = margin;

    const float z = std::max(1e-6f, zoomFactor);
    float mmPerPx = 0.0f;
    if (sliceDirection == 0) {
        mmPerPx = spacingY / z;
    } else {
        mmPerPx = spacingZ / z;
    }
    if (!(mmPerPx > 0.0f)) return;

    const float totalMmF = barH * mmPerPx;
    int totalMmI = (int)floorf(totalMmF + 0.5f);
    if (totalMmI < 1) totalMmI = 1;

    char label[64];
    if (totalMmI >= 100) {
        snprintf(label, sizeof(label), "%.1f cm", (float)totalMmI / 10.0f);
    } else {
        snprintf(label, sizeof(label), "%d mm", totalMmI);
    }

    const float x = (float)winWidth - margin;

    nvgSave(g_nvgContext);
    nvgBeginPath(g_nvgContext);
    nvgMoveTo(g_nvgContext, x, barY);
    nvgLineTo(g_nvgContext, x, barY + barH);
    nvgStrokeWidth(g_nvgContext, 2.0f);
    nvgStrokeColor(g_nvgContext, nvgRGBA(255, 255, 255, 200));
    nvgStroke(g_nvgContext);

    const float pxPerMm = 1.0f / mmPerPx;
    int stepMm = 1;
    if (pxPerMm < 3.0f) {
        stepMm = 5;
    }
    const float tickShort = 6.0f;
    const float tickLong = 12.0f;

    nvgBeginPath(g_nvgContext);
    for (int mm = 0; mm <= totalMmI; mm += stepMm) {
        const float t = (float)mm / (float)totalMmI;
        const float y = barY + barH * t;
        const bool isMajor = (mm % 5) == 0;
        const float len = isMajor ? tickLong : tickShort;
        nvgMoveTo(g_nvgContext, x, y);
        nvgLineTo(g_nvgContext, x - len, y);
    }
    nvgStrokeWidth(g_nvgContext, 2.0f);
    nvgStrokeColor(g_nvgContext, nvgRGBA(255, 255, 255, 200));
    nvgStroke(g_nvgContext);

    if (g_nvgFontId >= 0) {
        nvgFontSize(g_nvgContext, 14.0f);
        nvgFontFace(g_nvgContext, "ui");
        nvgTextAlign(g_nvgContext, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
        nvgFillColor(g_nvgContext, nvgRGBA(0, 0, 0, 180));
        nvgText(g_nvgContext, x - 10.0f + 1.0f, barY + barH * 0.5f + 1.0f, label, nullptr);
        nvgFillColor(g_nvgContext, nvgRGBA(255, 255, 255, 230));
        nvgText(g_nvgContext, x - 10.0f, barY + barH * 0.5f, label, nullptr);
    }

    nvgRestore(g_nvgContext);
}

// 3D视图专用的水平比例尺（地图样式）
void DrawHorizontalScaleBarNVG_InFrame(int winWidth, int winHeight, float zoomFactor, VolumeHandle volume) {
    if (!g_nvgContext) return;
    if (winWidth <= 0 || winHeight <= 0) return;
    if (!volume) return;

    float spacingX = 0.0f, spacingY = 0.0f, spacingZ = 0.0f;
    if (Dicom_Volume_GetSpacing(volume, &spacingX, &spacingY, &spacingZ) != NATIVE_OK) {
        return;
    }

    // 使用最小的spacing
    float minSpacing = spacingX;
    if (spacingY < minSpacing) minSpacing = spacingY;
    if (spacingZ < minSpacing) minSpacing = spacingZ;

    const float margin = 12.0f;
    const float desiredBarWidth = 60.0f;
    
    const float z = std::max(1e-6f, zoomFactor);
    float mmPerPx = minSpacing / z;
    if (!(mmPerPx > 0.0f)) return;
    
    float totalMmF = desiredBarWidth * mmPerPx;
    
    int totalMmI;
    if (totalMmF <= 1.0f) {
        totalMmI = 1;
    } else if (totalMmF <= 2.0f) {
        totalMmI = 2;
    } else if (totalMmF <= 5.0f) {
        totalMmI = 5;
    } else if (totalMmF <= 10.0f) {
        totalMmI = 10;
    } else if (totalMmF <= 20.0f) {
        totalMmI = 20;
    } else if (totalMmF <= 50.0f) {
        totalMmI = 50;
    } else if (totalMmF <= 100.0f) {
        totalMmI = 100;
    } else if (totalMmF <= 200.0f) {
        totalMmI = 200;
    } else if (totalMmF <= 500.0f) {
        totalMmI = 500;
    } else {
        totalMmI = ((int)(totalMmF / 100.0f) + 1) * 100;
    }
    
    float barWidth = (float)totalMmI / mmPerPx;
    if (barWidth < 30.0f) barWidth = 30.0f;
    if (barWidth > 150.0f) barWidth = 150.0f;
    
    char label[64];
    if (totalMmI >= 100) {
        snprintf(label, sizeof(label), "%.1f cm", (float)totalMmI / 10.0f);
    } else {
        snprintf(label, sizeof(label), "%d mm", totalMmI);
    }
    
    const float barX = (float)winWidth - margin - barWidth;
    const float barY = (float)winHeight - margin - 30.0f;
    const float tickHeight = 8.0f;

    nvgSave(g_nvgContext);
    
    nvgBeginPath(g_nvgContext);
    nvgMoveTo(g_nvgContext, barX, barY);
    nvgLineTo(g_nvgContext, barX + barWidth, barY);
    nvgStrokeWidth(g_nvgContext, 2.0f);
    nvgStrokeColor(g_nvgContext, nvgRGBA(255, 255, 255, 200));
    nvgStroke(g_nvgContext);
    
    nvgBeginPath(g_nvgContext);
    nvgMoveTo(g_nvgContext, barX, barY - tickHeight);
    nvgLineTo(g_nvgContext, barX, barY);
    nvgStrokeWidth(g_nvgContext, 2.0f);
    nvgStrokeColor(g_nvgContext, nvgRGBA(255, 255, 255, 200));
    nvgStroke(g_nvgContext);
    
    nvgBeginPath(g_nvgContext);
    nvgMoveTo(g_nvgContext, barX + barWidth, barY - tickHeight);
    nvgLineTo(g_nvgContext, barX + barWidth, barY);
    nvgStrokeWidth(g_nvgContext, 2.0f);
    nvgStrokeColor(g_nvgContext, nvgRGBA(255, 255, 255, 200));
    nvgStroke(g_nvgContext);
    
    if (g_nvgFontId >= 0) {
        nvgFontSize(g_nvgContext, 12.0f);
        nvgFontFace(g_nvgContext, "ui");
        nvgTextAlign(g_nvgContext, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
        nvgFillColor(g_nvgContext, nvgRGBA(0, 0, 0, 180));
        nvgText(g_nvgContext, barX + barWidth * 0.5f + 1.0f, barY - tickHeight - 4.0f + 1.0f, label, nullptr);
        nvgFillColor(g_nvgContext, nvgRGBA(255, 255, 255, 230));
        nvgText(g_nvgContext, barX + barWidth * 0.5f, barY - tickHeight - 4.0f, label, nullptr);
    }

    nvgRestore(g_nvgContext);
}

// 包装函数，带完整的 NanoVG 帧管理（用于外部调用）
void DrawVerticalScaleBarNVG(int winWidth, int winHeight, int sliceDirection, float zoomFactor, VolumeHandle volume) {
    EnsureNanoVGReady();
    if (!g_nvgContext) return;
    
    PrepareGLForNanoVGOverlay();
    nvgBeginFrame(g_nvgContext, winWidth, winHeight, 1.0f);
    DrawVerticalScaleBarNVG_InFrame(winWidth, winHeight, sliceDirection, zoomFactor, volume);
    nvgEndFrame(g_nvgContext);
}

// 3D视图专用的水平比例尺包装函数
void DrawHorizontalScaleBarNVG(int winWidth, int winHeight, float zoomFactor, VolumeHandle volume) {
    EnsureNanoVGReady();
    if (!g_nvgContext) return;
    
    PrepareGLForNanoVGOverlay();
    nvgBeginFrame(g_nvgContext, winWidth, winHeight, 1.0f);
    DrawHorizontalScaleBarNVG_InFrame(winWidth, winHeight, zoomFactor, volume);
    nvgEndFrame(g_nvgContext);
}

// 在左下角绘制方向指示立方体（内容部分，需要在 nvgBeginFrame/EndFrame 之间调用）
static void DrawOrientationCubeNVG_InFrame(int winWidth, int winHeight, const float* camRot) {
    if (!g_nvgContext || winWidth <= 0 || winHeight <= 0) return;

    const float cubeSize = 60.0f;  // 立方体大小
    const float margin = 20.0f;    // 左下角边距
    const float centerX = margin + cubeSize * 0.5f;
    const float centerY = (float)winHeight - margin - cubeSize * 0.5f;
    
    nvgSave(g_nvgContext);
    
    // 定义立方体的8个顶点（局部坐标系，以原点为中心）
    float vertices[8][3] = {
        {-1.0f, -1.0f, -1.0f},  // 0: 左下后
        { 1.0f, -1.0f, -1.0f},  // 1: 右下后
        { 1.0f,  1.0f, -1.0f},  // 2: 右上后
        {-1.0f,  1.0f, -1.0f},  // 3: 左上后
        {-1.0f, -1.0f,  1.0f},  // 4: 左下前
        { 1.0f, -1.0f,  1.0f},  // 5: 右下前
        { 1.0f,  1.0f,  1.0f},  // 6: 右上前
        {-1.0f,  1.0f,  1.0f}   // 7: 左上前
    };
    
    // 应用相机旋转矩阵，变换顶点
    float transformedVerts[8][3];
    for (int i = 0; i < 8; ++i) {
        // 矩阵乘法：v' = R * v（OpenGL列优先矩阵）
        transformedVerts[i][0] = camRot[0] * vertices[i][0] + camRot[4] * vertices[i][1] + camRot[8]  * vertices[i][2];
        transformedVerts[i][1] = camRot[1] * vertices[i][0] + camRot[5] * vertices[i][1] + camRot[9]  * vertices[i][2];
        transformedVerts[i][2] = camRot[2] * vertices[i][0] + camRot[6] * vertices[i][1] + camRot[10] * vertices[i][2];
        
        // 投影到屏幕（正交投影，忽略Z，缩放到cubeSize范围）
        transformedVerts[i][0] = centerX + transformedVerts[i][0] * cubeSize * 0.4f;
        transformedVerts[i][1] = centerY - transformedVerts[i][1] * cubeSize * 0.4f;  // Y轴翻转
    }
    
    // 绘制立方体的12条边（红色）
    nvgStrokeWidth(g_nvgContext, 2.0f);
    nvgStrokeColor(g_nvgContext, nvgRGBA(255, 50, 50, 220));
    
    // 定义立方体的12条边（顶点索引对）
    int edges[12][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0},  // 后面
        {4, 5}, {5, 6}, {6, 7}, {7, 4},  // 前面
        {0, 4}, {1, 5}, {2, 6}, {3, 7}   // 连接前后
    };
    
    for (int i = 0; i < 12; ++i) {
        nvgBeginPath(g_nvgContext);
        nvgMoveTo(g_nvgContext, transformedVerts[edges[i][0]][0], transformedVerts[edges[i][0]][1]);
        nvgLineTo(g_nvgContext, transformedVerts[edges[i][1]][0], transformedVerts[edges[i][1]][1]);
        nvgStroke(g_nvgContext);
    }
    
    // 计算每个面的中心点和法向量，绘制标签
    // 只绘制面向观察者的三个面（Z > 0的面）
    if (g_nvgFontId >= 0) {
        nvgFontSize(g_nvgContext, 14.0f);
        nvgFontFace(g_nvgContext, "ui");
        nvgTextAlign(g_nvgContext, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        
        // 定义6个面：前、后、左、右、上、下
        // 每个面由4个顶点和一个标签定义
        struct Face {
            int verts[4];
            const char* label;
            float normalX, normalY, normalZ;  // 未变换的法向量
        };
        
        Face faces[] = {
            {{4, 5, 6, 7}, "A", 0.0f, 0.0f, 1.0f},   // 前面 (Z+) - Axial
            {{1, 0, 3, 2}, "A", 0.0f, 0.0f, -1.0f},  // 后面 (Z-)
            {{0, 4, 7, 3}, "S", -1.0f, 0.0f, 0.0f},  // 左面 (X-) - Sagittal
            {{5, 1, 2, 6}, "S", 1.0f, 0.0f, 0.0f},   // 右面 (X+)
            {{3, 7, 6, 2}, "C", 0.0f, 1.0f, 0.0f},   // 上面 (Y+) - Coronal
            {{4, 0, 1, 5}, "C", 0.0f, -1.0f, 0.0f}   // 下面 (Y-)
        };
        
        for (int i = 0; i < 6; ++i) {
            // 变换法向量
            float nx = camRot[0] * faces[i].normalX + camRot[4] * faces[i].normalY + camRot[8]  * faces[i].normalZ;
            float ny = camRot[1] * faces[i].normalX + camRot[5] * faces[i].normalY + camRot[9]  * faces[i].normalZ;
            float nz = camRot[2] * faces[i].normalX + camRot[6] * faces[i].normalY + camRot[10] * faces[i].normalZ;
            
            // 只绘制面向观察者的面（法向量Z分量 > 0）
            if (nz > 0.1f) {
                // 计算面的中心点（4个顶点的平均）
                float cx = 0.0f, cy = 0.0f;
                for (int j = 0; j < 4; ++j) {
                    cx += transformedVerts[faces[i].verts[j]][0];
                    cy += transformedVerts[faces[i].verts[j]][1];
                }
                cx *= 0.25f;
                cy *= 0.25f;
                
                // 绘制文字（黑色阴影 + 白色文字）
                nvgFillColor(g_nvgContext, nvgRGBA(0, 0, 0, 180));
                nvgText(g_nvgContext, cx + 1.0f, cy + 1.0f, faces[i].label, nullptr);
                
                nvgFillColor(g_nvgContext, nvgRGBA(255, 255, 255, 240));
                nvgText(g_nvgContext, cx, cy, faces[i].label, nullptr);
            }
        }
    }
    
    nvgRestore(g_nvgContext);
}

// 3D视图方向指示立方体包装函数
void DrawOrientationCubeNVG(int winWidth, int winHeight, const float* camRot) {
    EnsureNanoVGReady();
    if (!g_nvgContext) return;
    
    PrepareGLForNanoVGOverlay();
    nvgBeginFrame(g_nvgContext, winWidth, winHeight, 1.0f);
    DrawOrientationCubeNVG_InFrame(winWidth, winHeight, camRot);
    nvgEndFrame(g_nvgContext);
}

static void DrawWindowLevelHudNVG_InFrame(int winWidth, int winHeight, float windowWidthHU, float windowLevelHU) {
    if (!g_nvgContext) return;
    if (winWidth <= 0 || winHeight <= 0) return;

    const bool hasWwWl = (windowWidthHU > 0.0f);

    const float margin = 12.0f;
    const float textSize = 14.0f;
    const float lineH = 18.0f;
    const float barW = 16.0f;
    const float barGap = 8.0f;

    // 80% height bar like reference image.
    const float topReserve = 10.0f + 16.0f + 8.0f;              // slice header + breathing space
    const float bottomReserve = margin + (lineH * 2.0f + 6.0f);  // WW/WL text
    const float desiredH = (float)winHeight * 0.8f;
    float barH = desiredH;
    const float maxH = (float)winHeight - topReserve - bottomReserve;
    if (maxH > 40.0f && barH > maxH) barH = maxH;
    if (barH < 40.0f) barH = 40.0f;

    // Always draw the WW/WL UI. If parameters are missing/invalid, still render the bar/ticks
    // and show placeholder text (if font is available).
    const float vmin = hasWwWl ? (windowLevelHU - windowWidthHU * 0.5f) : -1.0f;
    const float vmax = hasWwWl ? (windowLevelHU + windowWidthHU * 0.5f) :  1.0f;

    // Place WW/WL at bottom-left; keep color bar above it on the left.
    const float textX = margin;
    const float textY2 = (float)winHeight - margin - lineH * 0.5f;            // WL line
    const float textY1 = (float)winHeight - margin - lineH - lineH * 0.5f;    // WW line

    float barX = margin;
    float barY = topReserve;
    if (barY + barH > (float)winHeight - bottomReserve) {
        barY = (float)winHeight - bottomReserve - barH;
    }
    if (barY < margin) barY = margin;

    nvgSave(g_nvgContext);

    // Grayscale bar: top white, bottom black
    nvgBeginPath(g_nvgContext);
    nvgRect(g_nvgContext, barX, barY, barW, barH);
    NVGpaint grad = nvgLinearGradient(
        g_nvgContext,
        barX, barY,
        barX, barY + barH,
        nvgRGBA(255, 255, 255, 220),
        nvgRGBA(0, 0, 0, 220)
    );
    nvgFillPaint(g_nvgContext, grad);
    nvgFill(g_nvgContext);
    nvgStrokeWidth(g_nvgContext, 1.0f);
    nvgStrokeColor(g_nvgContext, nvgRGBA(255, 255, 255, 160));
    nvgStroke(g_nvgContext);

    // Min/Max tick marks + labels
    nvgBeginPath(g_nvgContext);
    nvgMoveTo(g_nvgContext, barX + barW, barY + 1.0f);
    nvgLineTo(g_nvgContext, barX + barW + 6.0f, barY + 1.0f);
    nvgMoveTo(g_nvgContext, barX + barW, barY + barH - 1.0f);
    nvgLineTo(g_nvgContext, barX + barW + 6.0f, barY + barH - 1.0f);
    nvgStrokeColor(g_nvgContext, nvgRGBA(255, 255, 255, 180));
    nvgStrokeWidth(g_nvgContext, 1.0f);
    nvgStroke(g_nvgContext);

    if (g_nvgFontId >= 0) {
        nvgFontSize(g_nvgContext, textSize);
        nvgFontFace(g_nvgContext, "ui");
        nvgTextAlign(g_nvgContext, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

        char buf[64];
        if (hasWwWl) snprintf(buf, sizeof(buf), "%.0f", vmax);
        else snprintf(buf, sizeof(buf), "--");
        nvgFillColor(g_nvgContext, nvgRGBA(255, 255, 255, 230));
        nvgText(g_nvgContext, barX + barW + barGap, barY + 1.0f, buf, nullptr);

        if (hasWwWl) snprintf(buf, sizeof(buf), "%.0f", vmin);
        else snprintf(buf, sizeof(buf), "--");
        nvgFillColor(g_nvgContext, nvgRGBA(255, 255, 255, 230));
        nvgText(g_nvgContext, barX + barW + barGap, barY + barH - 1.0f, buf, nullptr);

        // WW/WL at bottom-left
        if (hasWwWl) snprintf(buf, sizeof(buf), "WW %.0f", windowWidthHU);
        else snprintf(buf, sizeof(buf), "WW --");
        nvgFillColor(g_nvgContext, nvgRGBA(255, 255, 255, 230));
        nvgText(g_nvgContext, textX, textY1, buf, nullptr);

        if (hasWwWl) snprintf(buf, sizeof(buf), "WL %.0f", windowLevelHU);
        else snprintf(buf, sizeof(buf), "WL --");
        nvgFillColor(g_nvgContext, nvgRGBA(255, 255, 255, 230));
        nvgText(g_nvgContext, textX, textY2, buf, nullptr);
    }

    nvgRestore(g_nvgContext);
}

// 直锟斤拷锟斤拷mask锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷圆锟轿ｏ拷锟斤拷锟斤拷锟杰ｏ拷锟斤拷锟斤拷API锟斤拷锟矫ｏ拷
static void DrawCircleToMaskBuffer(uint8_t* maskData, int width, int height, int depth,
                                   int sliceDir, int sliceIndex, float cx, float cy, float radius, bool erase) {
    if (!maskData) return;
    
    int radiusInt = (int)(radius + 0.5f);
    int minX = (int)(cx - radius);
    int maxX = (int)(cx + radius);
    int minY = (int)(cy - radius);
    int maxY = (int)(cy + radius);
    
    float radiusSq = radius * radius;
    
    for (int py = minY; py <= maxY; py++) {
        for (int px = minX; px <= maxX; px++) {
            float dx = px - cx;
            float dy = py - cy;
            float distSq = dx * dx + dy * dy;
            
            if (distSq <= radiusSq) {
                // 锟斤拷锟斤拷锟斤拷片锟斤拷锟斤拷映锟戒到3D锟斤拷锟斤拷
                int x, y, z;
                if (sliceDir == 0) {  // Axial (XY)
                    x = px; y = py; z = sliceIndex;
                } else if (sliceDir == 1) {  // Coronal (XZ)
                    x = px; y = sliceIndex; z = py;
                } else {  // Sagittal (YZ)
                    x = sliceIndex; y = px; z = py;
                }
                
                // 锟竭斤拷锟斤拷
                if (x >= 0 && x < width && y >= 0 && y < height && z >= 0 && z < depth) {
                    int index = z * width * height + y * width + x;
                    if (erase) {
                        maskData[index] = 0;  // 锟斤拷锟斤拷
                    } else {
                        maskData[index] = 255;  // 锟斤拷锟斤拷
                    }
                }
            }
        }
    }
}


// ==================== 锟斤拷锟斤拷转锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷前锟斤拷锟斤拷锟斤拷锟斤拷====================
// 实锟街斤拷锟斤拷VolumeContext锟斤拷锟斤拷之锟斤拷
static MeasurementPoint MPR_NDCToWorld(float ndcX, float ndcY, MPRContext* ctx);
static MeasurementPoint MPR_NDCToWorld_Fixed(float ndcX, float ndcY, MPRContext* ctx,
                                            float fixedCenterX, float fixedCenterY, float fixedCenterZ);
static Point2D MPR_WorldToNDC(const MeasurementPoint& world, MPRContext* ctx);
static Point2D MPR_WorldToNDC_Fixed(const MeasurementPoint& world, MPRContext* ctx, 
                                     float fixedCenterX, float fixedCenterY, float fixedCenterZ);
static MeasurementPoint APR_NDCToWorld(float ndcX, float ndcY, APRContext* ctx);
static MeasurementPoint APR_NDCToWorld_Fixed(float ndcX, float ndcY, APRContext* ctx,
                                            float fixedCenterX, float fixedCenterY, float fixedCenterZ);
static Point2D APR_WorldToNDC(const MeasurementPoint& world, APRContext* ctx);
static Point2D APR_WorldToNDC_Fixed(const MeasurementPoint& world, APRContext* ctx,
                                     float fixedCenterX, float fixedCenterY, float fixedCenterZ);

// Forward declaration: MPR session lookup (implementation lives near the MPR mask APIs)
static MPRContext* GetMPRContextFromSession(const char* sessionId);

struct WindowContext {
#ifdef _WIN32
    HWND hwnd = nullptr;           // Win32锟斤拷锟节撅拷锟?
    HDC hdc = nullptr;             // 锟借备锟斤拷锟斤拷锟斤拷
    HGLRC hglrc = nullptr;         // OpenGL锟斤拷染锟斤拷锟斤拷锟斤拷
    HWND parentHwnd = nullptr;     // 嵌锟斤拷母锟斤拷锟斤拷锟?
    RECT embeddedRect { 0, 0, 0, 0 }; // 锟节革拷锟斤拷锟斤拷锟叫碉拷位锟矫和尺达拷
#else
    GLFWwindow* window = nullptr;  // 锟斤拷Windows平台锟斤拷锟斤拷使锟斤拷GLFW
#endif
    std::string debugTitle;
    int width = 800, height = 600;
    void* boundRenderer = nullptr;
    int rendererType = 0;
    bool is3DView = false;         // 锟角凤拷锟斤拷3D锟斤拷锟斤拷锟斤拷图锟斤拷锟斤拷要锟斤拷锟斤拷锟斤拷染锟斤拷

    // Selects which 3D renderer implementation to use when is3DView==true.
    // 1=ImageBrowserOrthogonal3DRenderer, 2=RoiOrthogonal3DRenderer, 3=ReconstructionRaycast3DRenderer
    int threeDRendererKind = 1;
    bool threeDRendererKindExplicit = false;

    // 3D view mode: true=orthogonal tri-planar (APR_RenderOrthogonal3D), false=raycast volume
    bool orthogonal3DView = true;

    // 3D raycast VRAM optimization: when enabled, upload a downsampled 3D volume texture
    // to reduce GPU memory footprint (at the cost of visual fidelity).
    bool vramOptimized3D = false;

    // 3D raycast mask iso-surface mode: when enabled, use session masks as an iso-surface (binary mask -> iso=0.5)
    bool maskIsoSurface3D = false;

    // 3D raycast transfer function (256x1 RGBA8 LUT). If no custom LUT is set,
    // a default grayscale ramp is used.
    std::vector<unsigned char> transferLutRGBA3D;
    bool transferLutCustom3D = false;
    bool transferLutDirty3D = true;

    // 3D raycast lighting parameters (normalized 0..1). Used by mask iso-surface shading.
    float lightAmbient3D = 0.25f;
    float lightDiffuse3D = 0.75f;
    float lightSpecular3D = 0.0f;

    // Raycasting 3D renderer state (allocated lazily on first WM_PAINT)
    void* raycast3D = nullptr;

    // 3D primitive scene
    int nextPrimitiveId = 1;
    Scene3DTransform sceneTransform{};
    std::vector<Primitive3D> primitives;
    
    // 3D锟斤拷图锟斤拷要锟斤拷锟斤拷锟斤拷APR锟斤拷锟?
    void* aprAxial = nullptr;
    void* aprCoronal = nullptr;
    void* aprSagittal = nullptr;
    
    // 锟斤拷锟斤拷锟斤拷锟竭癸拷锟斤拷锟斤拷
    ToolManagerHandle toolManager = nullptr;
    ToolHandle activeTool = nullptr;
    ToolHandle tools[7] = { nullptr };  // tools[1-6] for measurement tools
    
    // 锟斤拷杲伙拷锟阶刺?
    double lastMouseX = 0, lastMouseY = 0;
    bool isDragging = false;
    bool isRightDragging = false;  // 锟揭硷拷锟斤拷拽锟斤拷锟节达拷锟斤拷锟斤拷位

    // Right-drag window/level for 2D views
    bool isWindowing = false;
    double windowingStartX = 0.0;
    double windowingStartY = 0.0;
    float windowingStartWW = 400.0f;
    float windowingStartWL = 40.0f;

    // 3D navigation state (per-window; used when is3DView=true)
    float viewRotX = 30.0f;
    float viewRotY = 45.0f;
    float viewZoom = 1.0f;
    float viewPanX = 0.0f;
    float viewPanY = 0.0f;
    float viewRotMat[16] = { 0 };  // accumulated rotation matrix (column-major)
    bool viewRotMatInitialized = false;
    bool isMiddleDragging = false;
    bool is3DRotating = false;
    bool is3DPanning = false;

    // Arcball rotation (3D)
    bool arcballActive = false;
    float arcballLast[3] = { 0.0f, 0.0f, 1.0f };

    // APR shift-rotate (in-plane Z rotation)
    bool isShiftRotateZ = false;
    double shiftRotateLastAngleDeg = 0.0;
    
    // 锟斤拷锟节可硷拷锟斤拷状态锟斤拷锟斤拷锟斤拷Tab锟叫伙拷时锟斤拷停锟斤拷染锟斤拷
    bool isHidden = false;

    // Standalone window behavior. When embedded into Electron (child HWND),
    // closing must be controlled by host, so we disable close handling.
    bool allowClose = true;
    
    // 锟斤拷染锟斤拷锟斤拷锟街撅拷锟斤拷锟斤拷锟斤拷馗锟斤拷锟絀nvalidateRect锟斤拷锟矫碉拷锟斤拷锟斤拷息锟斤拷锟叫堆伙拷锟斤拷
    std::atomic<bool> needsRedraw{ false };
    
    // 锟竭程帮拷全锟斤拷锟斤拷锟斤拷锟斤拷止锟斤拷锟竭筹拷同时锟斤拷锟斤拷displayBuffer锟斤拷锟斤拷锟捷ｏ拷
    std::mutex renderMutex;

    // Crop box visibility (per-window)
    bool cropBoxVisible = false;
};

NativeResult Window_SetCropBoxVisible(WindowHandle handle, bool visible) {
    if (!handle) return NATIVE_E_INVALID_ARGUMENT;
    auto ctx = static_cast<WindowContext*>(handle);
    if (!ctx) return NATIVE_E_INVALID_ARGUMENT;
    ctx->cropBoxVisible = visible;
    Window_Invalidate(handle);
    return NATIVE_OK;
}

static std::string GetSessionIdFromWindowContext(WindowContext* ctx) {
    if (!ctx || !ctx->boundRenderer) return std::string();
    if (ctx->rendererType == 1) {
        auto mprCtx = static_cast<MPRContext*>(ctx->boundRenderer);
        return mprCtx ? mprCtx->sessionId : std::string();
    }
    auto aprCtx = static_cast<APRContext*>(ctx->boundRenderer);
    return aprCtx ? aprCtx->sessionId : std::string();
}

static void Win32_SyncWindowLevelToSession(WindowContext* activeWin, float ww, float wl) {
    if (!activeWin || !activeWin->boundRenderer) return;
    if (!(ww > 0.0f)) return;

    const std::string sessionId = GetSessionIdFromWindowContext(activeWin);
    if (sessionId.empty()) return;

    for (auto winHandle : g_AllWindows) {
        auto win = static_cast<WindowContext*>(winHandle);
        if (!win || !win->boundRenderer) continue;
        if (GetSessionIdFromWindowContext(win) != sessionId) continue;

        if (win->rendererType == 0 || win->rendererType == 2) {
            auto aprCtx = static_cast<APRContext*>(win->boundRenderer);
            if (!aprCtx) continue;
            aprCtx->windowWidthHU = ww;
            aprCtx->windowLevelHU = wl;
        } else if (win->rendererType == 1) {
            auto mprCtx = static_cast<MPRContext*>(win->boundRenderer);
            if (!mprCtx) continue;
            mprCtx->windowWidthHU = ww;
            mprCtx->windowLevelHU = wl;
        } else {
            continue;
        }

#if defined(_WIN32)
        if (win->hwnd) {
            InvalidateRect(win->hwnd, nullptr, FALSE);
        }
#endif
    }
}

static void SafeCopySessionId(char outSessionId[64], const std::string& sessionId) {
    if (!outSessionId) return;
    outSessionId[0] = '\0';
    if (sessionId.empty()) return;
    strncpy_s(outSessionId, 64, sessionId.c_str(), 63);
    outSessionId[63] = '\0';
}

static Primitive3D* FindPrimitiveById(WindowContext* ctx, int primitiveId) {
    if (!ctx || primitiveId <= 0) return nullptr;
    for (auto& prim : ctx->primitives) {
        if (prim.id == primitiveId) return &prim;
    }
    return nullptr;
}

static void DrawCubeUnit() {
    // Cube centered at origin, side length 1.
    const float h = 0.5f;
    glBegin(GL_TRIANGLES);

    // +Z
    glNormal3f(0, 0, 1);
    glVertex3f(-h, -h, h); glVertex3f( h, -h, h); glVertex3f( h,  h, h);
    glVertex3f(-h, -h, h); glVertex3f( h,  h, h); glVertex3f(-h,  h, h);

    // -Z
    glNormal3f(0, 0, -1);
    glVertex3f( h, -h, -h); glVertex3f(-h, -h, -h); glVertex3f(-h,  h, -h);
    glVertex3f( h, -h, -h); glVertex3f(-h,  h, -h); glVertex3f( h,  h, -h);

    // -X
    glNormal3f(-1, 0, 0);
    glVertex3f(-h, -h, -h); glVertex3f(-h, -h,  h); glVertex3f(-h,  h,  h);
    glVertex3f(-h, -h, -h); glVertex3f(-h,  h,  h); glVertex3f(-h,  h, -h);

    // +X
    glNormal3f(1, 0, 0);
    glVertex3f( h, -h,  h); glVertex3f( h, -h, -h); glVertex3f( h,  h, -h);
    glVertex3f( h, -h,  h); glVertex3f( h,  h, -h); glVertex3f( h,  h,  h);

    // +Y
    glNormal3f(0, 1, 0);
    glVertex3f(-h,  h,  h); glVertex3f( h,  h,  h); glVertex3f( h,  h, -h);
    glVertex3f(-h,  h,  h); glVertex3f( h,  h, -h); glVertex3f(-h,  h, -h);

    // -Y
    glNormal3f(0, -1, 0);
    glVertex3f(-h, -h, -h); glVertex3f( h, -h, -h); glVertex3f( h, -h,  h);
    glVertex3f(-h, -h, -h); glVertex3f( h, -h,  h); glVertex3f(-h, -h,  h);

    glEnd();
}

static void DrawSphere(float radius, int slices = 24, int stacks = 16) {
    // Latitude-longitude triangulation, centered at origin.
    for (int y = 0; y < stacks; y++) {
        float v0 = (float)y / stacks;
        float v1 = (float)(y + 1) / stacks;
        float phi0 = (v0 - 0.5f) * 3.1415926f;
        float phi1 = (v1 - 0.5f) * 3.1415926f;

        float y0 = sinf(phi0);
        float y1 = sinf(phi1);
        float r0 = cosf(phi0);
        float r1 = cosf(phi1);

        glBegin(GL_TRIANGLE_STRIP);
        for (int x = 0; x <= slices; x++) {
            float u = (float)x / slices;
            float theta = u * 2.0f * 3.1415926f;
            float cx = cosf(theta);
            float cz = sinf(theta);

            float nx0 = cx * r0;
            float nz0 = cz * r0;
            float nx1 = cx * r1;
            float nz1 = cz * r1;

            glNormal3f(nx1, y1, nz1);
            glVertex3f(nx1 * radius, y1 * radius, nz1 * radius);
            glNormal3f(nx0, y0, nz0);
            glVertex3f(nx0 * radius, y0 * radius, nz0 * radius);
        }
        glEnd();
    }
}

static void DrawCylinder(float radius, float height, int slices = 24) {
    // Cylinder centered at origin, axis along Y.
    float hy = height * 0.5f;

    // Side
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= slices; i++) {
        float u = (float)i / slices;
        float theta = u * 2.0f * 3.1415926f;
        float x = cosf(theta);
        float z = sinf(theta);
        glNormal3f(x, 0, z);
        glVertex3f(x * radius,  hy, z * radius);
        glVertex3f(x * radius, -hy, z * radius);
    }
    glEnd();

    // Top cap
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, 1, 0);
    glVertex3f(0, hy, 0);
    for (int i = 0; i <= slices; i++) {
        float u = (float)i / slices;
        float theta = u * 2.0f * 3.1415926f;
        float x = cosf(theta);
        float z = sinf(theta);
        glVertex3f(x * radius, hy, z * radius);
    }
    glEnd();

    // Bottom cap
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, -1, 0);
    glVertex3f(0, -hy, 0);
    for (int i = 0; i <= slices; i++) {
        float u = (float)i / slices;
        float theta = -u * 2.0f * 3.1415926f;
        float x = cosf(theta);
        float z = sinf(theta);
        glVertex3f(x * radius, -hy, z * radius);
    }
    glEnd();
}

static void Render3DPrimitives(WindowContext* win) {
    if (!win) return;
    if (win->primitives.empty()) return;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_TEXTURE_2D);

    glPushMatrix();
    // Scene transform
    glTranslatef(win->sceneTransform.tx, win->sceneTransform.ty, win->sceneTransform.tz);
    glRotatef(win->sceneTransform.rx, 1, 0, 0);
    glRotatef(win->sceneTransform.ry, 0, 1, 0);
    glRotatef(win->sceneTransform.rz, 0, 0, 1);
    glScalef(win->sceneTransform.sx, win->sceneTransform.sy, win->sceneTransform.sz);

    // Simple lighting
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    GLfloat lightPos[4] = { 1.0f, 1.0f, 2.0f, 0.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    for (const auto& prim : win->primitives) {
        if (!prim.visible) continue;
        glPushMatrix();
        glTranslatef(prim.tx, prim.ty, prim.tz);
        glRotatef(prim.rx, 1, 0, 0);
        glRotatef(prim.ry, 0, 1, 0);
        glRotatef(prim.rz, 0, 0, 1);
        glScalef(prim.sx, prim.sy, prim.sz);

        glColor4f(prim.r, prim.g, prim.b, prim.a);

        switch (prim.type) {
            case Primitive3DTypeCpp::Cube:
                glScalef(prim.p0, prim.p1, prim.p2);
                DrawCubeUnit();
                break;
            case Primitive3DTypeCpp::Sphere:
                DrawSphere(prim.p0);
                break;
            case Primitive3DTypeCpp::Cylinder:
                DrawCylinder(prim.p0, prim.p1);
                break;
        }

        // Wireframe outline for readability
        glDisable(GL_LIGHTING);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(1.5f);
        glColor4f(1, 1, 1, 0.6f);
        switch (prim.type) {
            case Primitive3DTypeCpp::Cube:
                glPushMatrix();
                glScalef(prim.p0, prim.p1, prim.p2);
                DrawCubeUnit();
                glPopMatrix();
                break;
            case Primitive3DTypeCpp::Sphere:
                DrawSphere(prim.p0);
                break;
            case Primitive3DTypeCpp::Cylinder:
                DrawCylinder(prim.p0, prim.p1);
                break;
        }
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_LIGHTING);

        glPopMatrix();
    }

    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_LIGHT0);
    glDisable(GL_LIGHTING);
    glPopMatrix();
}

// ==================== Raycasting 3D (minimal implementation) ====================
// This replaces the old orthogonal-triplanar placeholder for the 3D window.
// It uses a simple two-pass volume raycasting technique:
// 1) Render backfaces into an offscreen texture storing exit points.
// 2) Render frontfaces and ray-march inside the volume using a transfer function.

struct Raycast3DContext {
    // GL resources
    GLuint volumeTex3D = 0;
    GLuint transferTex2D = 0;
    GLuint backfaceFbo = 0;
    GLuint backfaceTex2D = 0;
    GLuint backfaceDepthRb = 0;
    GLuint progBackface = 0;
    GLuint progRaycast = 0;

    // Optional 3D mask texture (binary/uint8).
    GLuint maskTex3D = 0;

    // Box mesh (unit cube in [-0.5,0.5])
    GLuint boxVbo = 0;
    int boxVertCount = 0;

    // Simple geometry program + meshes for overlay primitives
    GLuint progGeom = 0;
    GLuint cubeVbo = 0;
    int cubeVertCount = 0;
    GLuint sphereVbo = 0;
    int sphereVertCount = 0;
    GLuint cylinderVbo = 0;
    int cylinderVertCount = 0;

    // Cached state
    VolumeHandle volume = nullptr;
    int volW = 0, volH = 0, volD = 0;
    bool vramOptimized = false;

    // Cached mask upload state
    std::string maskSessionId;
    uint64_t maskRevision = 0;
    int maskW = 0, maskH = 0, maskD = 0;
    std::vector<uint8_t> maskCombined;

    int fboW = 0, fboH = 0;
    float boxW = 1.0f, boxH = 1.0f, boxD = 1.0f; // normalized box extents
};

static void Raycast3D_Destroy(Raycast3DContext* rc) {
    if (!rc) return;
    if (rc->progRaycast) glDeleteProgram(rc->progRaycast);
    if (rc->progBackface) glDeleteProgram(rc->progBackface);
    if (rc->progGeom) glDeleteProgram(rc->progGeom);
    if (rc->backfaceDepthRb) glDeleteRenderbuffers(1, &rc->backfaceDepthRb);
    if (rc->backfaceTex2D) glDeleteTextures(1, &rc->backfaceTex2D);
    if (rc->backfaceFbo) glDeleteFramebuffers(1, &rc->backfaceFbo);
    if (rc->transferTex2D) glDeleteTextures(1, &rc->transferTex2D);
    if (rc->maskTex3D) glDeleteTextures(1, &rc->maskTex3D);
    if (rc->volumeTex3D) glDeleteTextures(1, &rc->volumeTex3D);
    if (rc->boxVbo) glDeleteBuffers(1, &rc->boxVbo);
    if (rc->cubeVbo) glDeleteBuffers(1, &rc->cubeVbo);
    if (rc->sphereVbo) glDeleteBuffers(1, &rc->sphereVbo);
    if (rc->cylinderVbo) glDeleteBuffers(1, &rc->cylinderVbo);
    delete rc;
}

static void Raycast3D_EnsureUnitCubeVbo(GLuint* outVbo, int* outVertCount) {
    if (!outVbo || !outVertCount) return;
    if (*outVbo) return;

    // 12 triangles (36 verts). Unit cube in [-0.5, 0.5].
    static const float verts[] = {
        // +Z
        -0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
        -0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        // -Z
         0.5f,-0.5f,-0.5f, -0.5f,-0.5f,-0.5f, -0.5f, 0.5f,-0.5f,
         0.5f,-0.5f,-0.5f, -0.5f, 0.5f,-0.5f,  0.5f, 0.5f,-0.5f,
        // -X
        -0.5f,-0.5f,-0.5f, -0.5f,-0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        -0.5f,-0.5f,-0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f,
        // +X
         0.5f,-0.5f, 0.5f,  0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f,
         0.5f,-0.5f, 0.5f,  0.5f, 0.5f,-0.5f,  0.5f, 0.5f, 0.5f,
        // +Y
        -0.5f, 0.5f, 0.5f,  0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f,
        -0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f, -0.5f, 0.5f,-0.5f,
        // -Y
        -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,
        -0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f, -0.5f,-0.5f, 0.5f,
    };

    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)sizeof(verts), verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    *outVbo = vbo;
    *outVertCount = 36;
}

static void DrawVboPositions(GLuint prog, GLuint vbo, int vertCount) {
    if (!prog || !vbo || vertCount <= 0) return;
    GLint locPos = glGetAttribLocation(prog, "aPos");
    if (locPos < 0) return;

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray((GLuint)locPos);
    glVertexAttribPointer((GLuint)locPos, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glDrawArrays(GL_TRIANGLES, 0, vertCount);
    glDisableVertexAttribArray((GLuint)locPos);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static GLuint CompileShader(GLenum type, const char* source, std::string* outLog) {
    GLuint shader = glCreateShader(type);
    if (!shader) return 0;

    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok == GL_TRUE) {
        return shader;
    }

    GLint logLen = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
    std::string log;
    if (logLen > 1) {
        log.resize(static_cast<size_t>(logLen));
        GLsizei written = 0;
        glGetShaderInfoLog(shader, logLen, &written, log.data());
    }

    if (outLog) {
        *outLog = log;
    }

    glDeleteShader(shader);
    return 0;
}

static GLuint LinkProgram(GLuint vs, GLuint fs, std::string* outLog) {
    GLuint prog = glCreateProgram();
    if (!prog) return 0;
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint ok = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (ok == GL_TRUE) {
        return prog;
    }

    GLint logLen = 0;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLen);
    std::string log;
    if (logLen > 1) {
        log.resize(static_cast<size_t>(logLen));
        GLsizei written = 0;
        glGetProgramInfoLog(prog, logLen, &written, log.data());
    }

    if (outLog) {
        *outLog = log;
    }

    glDeleteProgram(prog);
    return 0;
}

static bool Raycast3D_EnsurePrograms(Raycast3DContext* rc) {
    if (!rc) return false;
    if (rc->progBackface && rc->progRaycast && rc->progGeom) return true;

    // GLSL 1.20 (OpenGL 2.1 era). We still use shaders for all 3D drawing.
    const char* vsSrc =
        "#version 120\n"
        "attribute vec3 aPos;\n"
        "uniform mat4 uMVP;\n"
        "uniform vec3 uBoxSize;\n"
        "varying vec3 vTex;\n"
        "void main(){\n"
        "  vec3 pos = aPos * uBoxSize;\n"
        "  vTex = aPos + vec3(0.5);\n"
        "  gl_Position = uMVP * vec4(pos, 1.0);\n"
        "}\n";

    const char* fsBackface =
        "#version 120\n"
        "varying vec3 vTex;\n"
        "void main(){\n"
        "  gl_FragColor = vec4(vTex, 1.0);\n"
        "}\n";

    const char* fsRaycast =
        "#version 120\n"
        "varying vec3 vTex;\n"
        "uniform vec3 uBoxSize;\n"
        "uniform sampler2D uExitPoints;\n"
        "uniform sampler3D uVolume;\n"
        "uniform sampler2D uTransfer;\n"
        "uniform sampler3D uMask;\n"
        "uniform vec2 uViewport;\n"
        "uniform float uStepSize;\n"
        "uniform float uDensity;\n"
        "uniform int uMaskIsoEnabled;\n"
        "uniform float uMaskIso;\n"
        "uniform vec4 uMaskColor;\n"
        "uniform vec3 uMaskTexel;\n"
        "uniform float uLightAmbient;\n"
        "uniform float uLightDiffuse;\n"
        "uniform float uLightSpecular;\n"
        "void main(){\n"
        "  vec2 uv = gl_FragCoord.xy / uViewport;\n"
        "  vec3 entry = vTex;\n"
        "  vec3 exitP = texture2D(uExitPoints, uv).xyz;\n"
        "  vec3 dir = exitP - entry;\n"
        "  float len = length(dir);\n"
        "  if (len < 1e-5) discard;\n"
        "  vec3 stepDir = dir / len;\n"
        "  vec3 step = stepDir * uStepSize;\n"
        "  int steps = int(len / uStepSize);\n"
        "  if (uMaskIsoEnabled != 0){\n"
        "    float prev = texture3D(uMask, entry).r;\n"
        "    vec3 pos = entry;\n"
        "    for (int i=0; i<4096; i++){\n"
        "      if (i >= steps) break;\n"
        "      if (pos.x < 0.0 || pos.x > 1.0 || pos.y < 0.0 || pos.y > 1.0 || pos.z < 0.0 || pos.z > 1.0) break;\n"
        "      float m = texture3D(uMask, pos).r;\n"
        "      if (prev < uMaskIso && m >= uMaskIso){\n"
        "        // Gradient-based normal estimate (mask is binary but filtered -> boundary gradient exists).\n"
        "        float gx = texture3D(uMask, pos + vec3(uMaskTexel.x, 0.0, 0.0)).r - texture3D(uMask, pos - vec3(uMaskTexel.x, 0.0, 0.0)).r;\n"
        "        float gy = texture3D(uMask, pos + vec3(0.0, uMaskTexel.y, 0.0)).r - texture3D(uMask, pos - vec3(0.0, uMaskTexel.y, 0.0)).r;\n"
        "        float gz = texture3D(uMask, pos + vec3(0.0, 0.0, uMaskTexel.z)).r - texture3D(uMask, pos - vec3(0.0, 0.0, uMaskTexel.z)).r;\n"
        "        vec3 n = normalize(vec3(gx, gy, gz));\n"
        "        vec3 l = normalize(vec3(0.4, 0.7, 0.2));\n"
        "        float diff = max(dot(n, l), 0.0);\n"
        "        float spec = pow(diff, 24.0);\n"
        "        vec3 rgb = uMaskColor.rgb * (uLightAmbient + uLightDiffuse * diff) + vec3(uLightSpecular * spec);\n"
        "        gl_FragColor = vec4(rgb, uMaskColor.a);\n"
        "        return;\n"
        "      }\n"
        "      prev = m;\n"
        "      pos += step;\n"
        "    }\n"
        "    discard;\n"
        "  }\n"
        "  vec4 acc = vec4(0.0);\n"
        "  vec3 pos = entry;\n"
        "  for (int i=0; i<2048; i++){\n"
        "    if (i >= steps) break;\n"
        "    if (pos.x < 0.0 || pos.x > 1.0 || pos.y < 0.0 || pos.y > 1.0 || pos.z < 0.0 || pos.z > 1.0) break;\n"
        "    float v = texture3D(uVolume, pos).r;\n"
        "    vec4 col = texture2D(uTransfer, vec2(v, 0.5));\n"
        "    col.a *= uDensity;\n"
        "    acc.rgb += (1.0 - acc.a) * col.a * col.rgb;\n"
        "    acc.a   += (1.0 - acc.a) * col.a;\n"
        "    if (acc.a > 0.97) break;\n"
        "    pos += step;\n"
        "  }\n"
        "  gl_FragColor = acc;\n"
        "}\n";

    const char* fsGeom =
        "#version 120\n"
        "uniform vec4 uColor;\n"
        "void main(){\n"
        "  gl_FragColor = uColor;\n"
        "}\n";

    std::string log;
    GLuint vs = CompileShader(GL_VERTEX_SHADER, vsSrc, &log);
    if (!vs) {
        printf("[Raycast3D] Vertex shader compile failed: %s\n", log.c_str());
        return false;
    }
    GLuint fsB = CompileShader(GL_FRAGMENT_SHADER, fsBackface, &log);
    if (!fsB) {
        printf("[Raycast3D] Backface fragment shader compile failed: %s\n", log.c_str());
        glDeleteShader(vs);
        return false;
    }
    GLuint fsR = CompileShader(GL_FRAGMENT_SHADER, fsRaycast, &log);
    if (!fsR) {
        printf("[Raycast3D] Raycast fragment shader compile failed: %s\n", log.c_str());
        glDeleteShader(vs);
        glDeleteShader(fsB);
        return false;
    }

    rc->progBackface = LinkProgram(vs, fsB, &log);
    if (!rc->progBackface) {
        printf("[Raycast3D] Backface program link failed: %s\n", log.c_str());
        glDeleteShader(vs);
        glDeleteShader(fsB);
        glDeleteShader(fsR);
        return false;
    }
    rc->progRaycast = LinkProgram(vs, fsR, &log);
    if (!rc->progRaycast) {
        printf("[Raycast3D] Raycast program link failed: %s\n", log.c_str());
        glDeleteProgram(rc->progBackface);
        rc->progBackface = 0;
        glDeleteShader(vs);
        glDeleteShader(fsB);
        glDeleteShader(fsR);
        return false;
    }

    GLuint fsG = CompileShader(GL_FRAGMENT_SHADER, fsGeom, &log);
    if (!fsG) {
        printf("[Raycast3D] Geom fragment shader compile failed: %s\n", log.c_str());
        glDeleteProgram(rc->progBackface);
        glDeleteProgram(rc->progRaycast);
        rc->progBackface = 0;
        rc->progRaycast = 0;
        glDeleteShader(vs);
        glDeleteShader(fsB);
        glDeleteShader(fsR);
        return false;
    }

    rc->progGeom = LinkProgram(vs, fsG, &log);
    if (!rc->progGeom) {
        printf("[Raycast3D] Geom program link failed: %s\n", log.c_str());
        glDeleteProgram(rc->progBackface);
        glDeleteProgram(rc->progRaycast);
        rc->progBackface = 0;
        rc->progRaycast = 0;
        glDeleteShader(vs);
        glDeleteShader(fsB);
        glDeleteShader(fsR);
        glDeleteShader(fsG);
        return false;
    }

    glDeleteShader(vs);
    glDeleteShader(fsB);
    glDeleteShader(fsR);
    glDeleteShader(fsG);
    return true;
}

static void EnsurePrimitiveMeshes(Raycast3DContext* rc) {
    if (!rc) return;
    // Cube mesh: reuse unit cube.
    Raycast3D_EnsureUnitCubeVbo(&rc->cubeVbo, &rc->cubeVertCount);

    // Sphere mesh (unit sphere radius 0.5)
    if (!rc->sphereVbo) {
        const int stacks = 16;
        const int slices = 24;
        std::vector<float> verts;
        verts.reserve((size_t)stacks * (size_t)slices * 6 * 3);
        for (int i = 0; i < stacks; i++) {
            const float v0 = (float)i / (float)stacks;
            const float v1 = (float)(i + 1) / (float)stacks;
            const float th0 = v0 * 3.14159265358979323846f;
            const float th1 = v1 * 3.14159265358979323846f;
            for (int j = 0; j < slices; j++) {
                const float u0 = (float)j / (float)slices;
                const float u1 = (float)(j + 1) / (float)slices;
                const float ph0 = u0 * 2.0f * 3.14159265358979323846f;
                const float ph1 = u1 * 2.0f * 3.14159265358979323846f;

                auto add = [&](float th, float ph) {
                    const float x = std::sin(th) * std::cos(ph);
                    const float y = std::cos(th);
                    const float z = std::sin(th) * std::sin(ph);
                    verts.push_back(0.5f * x);
                    verts.push_back(0.5f * y);
                    verts.push_back(0.5f * z);
                };

                // two triangles
                add(th0, ph0);
                add(th1, ph0);
                add(th1, ph1);

                add(th0, ph0);
                add(th1, ph1);
                add(th0, ph1);
            }
        }

        glGenBuffers(1, &rc->sphereVbo);
        glBindBuffer(GL_ARRAY_BUFFER, rc->sphereVbo);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(verts.size() * sizeof(float)), verts.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        rc->sphereVertCount = (int)(verts.size() / 3);
    }

    // Cylinder mesh (unit radius 0.5, height 1.0 along Y in [-0.5,0.5])
    if (!rc->cylinderVbo) {
        const int slices = 24;
        std::vector<float> verts;
        // sides: 2 triangles per slice
        verts.reserve((size_t)slices * 12 * 3);
        for (int i = 0; i < slices; i++) {
            const float a0 = (float)i / (float)slices * 2.0f * 3.14159265358979323846f;
            const float a1 = (float)(i + 1) / (float)slices * 2.0f * 3.14159265358979323846f;
            const float x0 = 0.5f * std::cos(a0);
            const float z0 = 0.5f * std::sin(a0);
            const float x1 = 0.5f * std::cos(a1);
            const float z1 = 0.5f * std::sin(a1);
            const float y0 = -0.5f;
            const float y1 = 0.5f;

            // tri 1
            verts.push_back(x0); verts.push_back(y0); verts.push_back(z0);
            verts.push_back(x0); verts.push_back(y1); verts.push_back(z0);
            verts.push_back(x1); verts.push_back(y1); verts.push_back(z1);
            // tri 2
            verts.push_back(x0); verts.push_back(y0); verts.push_back(z0);
            verts.push_back(x1); verts.push_back(y1); verts.push_back(z1);
            verts.push_back(x1); verts.push_back(y0); verts.push_back(z1);
        }
        // caps
        for (int i = 0; i < slices; i++) {
            const float a0 = (float)i / (float)slices * 2.0f * 3.14159265358979323846f;
            const float a1 = (float)(i + 1) / (float)slices * 2.0f * 3.14159265358979323846f;
            const float x0 = 0.5f * std::cos(a0);
            const float z0 = 0.5f * std::sin(a0);
            const float x1 = 0.5f * std::cos(a1);
            const float z1 = 0.5f * std::sin(a1);
            // top (+Y)
            verts.push_back(0.0f); verts.push_back(0.5f); verts.push_back(0.0f);
            verts.push_back(x1);   verts.push_back(0.5f); verts.push_back(z1);
            verts.push_back(x0);   verts.push_back(0.5f); verts.push_back(z0);
            // bottom (-Y)
            verts.push_back(0.0f); verts.push_back(-0.5f); verts.push_back(0.0f);
            verts.push_back(x0);   verts.push_back(-0.5f); verts.push_back(z0);
            verts.push_back(x1);   verts.push_back(-0.5f); verts.push_back(z1);
        }

        glGenBuffers(1, &rc->cylinderVbo);
        glBindBuffer(GL_ARRAY_BUFFER, rc->cylinderVbo);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(verts.size() * sizeof(float)), verts.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        rc->cylinderVertCount = (int)(verts.size() / 3);
    }
}

static void Render3DPrimitives(WindowContext* win, Raycast3DContext* rc, const float pv[16]) {
    if (!win || !rc || !rc->progGeom) return;
    if (win->primitives.empty()) return;

    EnsurePrimitiveMeshes(rc);
    glUseProgram(rc->progGeom);

    GLint locMvp = glGetUniformLocation(rc->progGeom, "uMVP");
    GLint locBox = glGetUniformLocation(rc->progGeom, "uBoxSize");
    GLint locCol = glGetUniformLocation(rc->progGeom, "uColor");
    if (locBox >= 0) glUniform3f(locBox, 1.0f, 1.0f, 1.0f);

    float scene[16];
    Mat4_TRS(scene,
        win->sceneTransform.tx, win->sceneTransform.ty, win->sceneTransform.tz,
        win->sceneTransform.rx, win->sceneTransform.ry, win->sceneTransform.rz,
        win->sceneTransform.sx, win->sceneTransform.sy, win->sceneTransform.sz);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (const auto& prim : win->primitives) {
        if (!prim.visible) continue;

        float primTRS[16];
        Mat4_TRS(primTRS, prim.tx, prim.ty, prim.tz, prim.rx, prim.ry, prim.rz, prim.sx, prim.sy, prim.sz);

        float shapeS[16];
        if (prim.type == Primitive3DTypeCpp::Cube) {
            Mat4_Scale(shapeS, prim.p0, prim.p1, prim.p2);
        }
        else if (prim.type == Primitive3DTypeCpp::Sphere) {
            const float s = prim.p0 * 2.0f; // unit sphere radius is 0.5
            Mat4_Scale(shapeS, s, s, s);
        }
        else { // Cylinder
            const float s = prim.p0 * 2.0f; // radius
            Mat4_Scale(shapeS, s, prim.p1, s); // height along Y
        }

        float model1[16];
        float model[16];
        Mat4_Mul(model1, scene, primTRS);
        Mat4_Mul(model, model1, shapeS);

        float mvp[16];
        Mat4_Mul(mvp, pv, model);

        if (locMvp >= 0) glUniformMatrix4fv(locMvp, 1, GL_FALSE, mvp);
        if (locCol >= 0) glUniform4f(locCol, prim.r, prim.g, prim.b, prim.a);

        switch (prim.type) {
            case Primitive3DTypeCpp::Cube:
                DrawVboPositions(rc->progGeom, rc->cubeVbo, rc->cubeVertCount);
                break;
            case Primitive3DTypeCpp::Sphere:
                DrawVboPositions(rc->progGeom, rc->sphereVbo, rc->sphereVertCount);
                break;
            case Primitive3DTypeCpp::Cylinder:
                DrawVboPositions(rc->progGeom, rc->cylinderVbo, rc->cylinderVertCount);
                break;
        }

        // Wireframe overlay
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        if (locCol >= 0) glUniform4f(locCol, 1.0f, 1.0f, 1.0f, 0.6f);
        switch (prim.type) {
            case Primitive3DTypeCpp::Cube:
                DrawVboPositions(rc->progGeom, rc->cubeVbo, rc->cubeVertCount);
                break;
            case Primitive3DTypeCpp::Sphere:
                DrawVboPositions(rc->progGeom, rc->sphereVbo, rc->sphereVertCount);
                break;
            case Primitive3DTypeCpp::Cylinder:
                DrawVboPositions(rc->progGeom, rc->cylinderVbo, rc->cylinderVertCount);
                break;
        }
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glUseProgram(0);
}

static bool Raycast3D_EnsureVolumeTexture(Raycast3DContext* rc, VolumeHandle volume, bool vramOptimized) {
    if (!rc || !volume) return false;
    int w = 0, h = 0, d = 0;
    if (Dicom_Volume_GetDimensions(volume, &w, &h, &d) != NATIVE_OK) return false;
    if (w <= 0 || h <= 0 || d <= 0) return false;

    if (rc->volumeTex3D && rc->volume == volume && rc->volW == w && rc->volH == h && rc->volD == d && rc->vramOptimized == vramOptimized) {
        return true;
    }

    // (Re)upload volume
    if (rc->volumeTex3D) {
        glDeleteTextures(1, &rc->volumeTex3D);
        rc->volumeTex3D = 0;
    }

    void* raw = Dicom_Volume_GetData(volume);
    if (!raw) {
        printf("[Raycast3D] Dicom_Volume_GetData returned NULL\n");
        return false;
    }

    // If VRAM-optimized, upload a downsampled copy (nearest-neighbor, factor=2).
    // This keeps the shader logic unchanged (still samples [0,1] texture coords).
    const uint16_t* src16 = static_cast<const uint16_t*>(raw);
    int texW = w;
    int texH = h;
    int texD = d;
    std::vector<uint16_t> downsampled;
    if (vramOptimized) {
        texW = (w + 1) / 2;
        texH = (h + 1) / 2;
        texD = (d + 1) / 2;
        const size_t texSize = (size_t)texW * (size_t)texH * (size_t)texD;
        downsampled.resize(texSize);

        for (int z = 0; z < texD; ++z) {
            const int srcZ = std::min(d - 1, z * 2);
            for (int y = 0; y < texH; ++y) {
                const int srcY = std::min(h - 1, y * 2);
                for (int x = 0; x < texW; ++x) {
                    const int srcX = std::min(w - 1, x * 2);
                    const size_t srcIdx = (size_t)srcZ * (size_t)w * (size_t)h + (size_t)srcY * (size_t)w + (size_t)srcX;
                    const size_t dstIdx = (size_t)z * (size_t)texW * (size_t)texH + (size_t)y * (size_t)texW + (size_t)x;
                    downsampled[dstIdx] = src16[srcIdx];
                }
            }
        }
    }

    glGenTextures(1, &rc->volumeTex3D);
    glBindTexture(GL_TEXTURE_3D, rc->volumeTex3D);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // Use LUMINANCE16 for broad compatibility with GLSL 1.20 + texture3D.
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage3D(
        GL_TEXTURE_3D,
        0,
        GL_LUMINANCE16,
        texW, texH, texD,
        0,
        GL_LUMINANCE,
        GL_UNSIGNED_SHORT,
        vramOptimized ? (const void*)downsampled.data() : (const void*)src16
    );

    rc->volume = volume;
    rc->volW = w;
    rc->volH = h;
    rc->volD = d;
    rc->vramOptimized = vramOptimized;

    // Normalized box extents (preserve volume aspect ratio)
    int maxDim = std::max(w, std::max(h, d));
    rc->boxW = (float)w / (float)maxDim;
    rc->boxH = (float)h / (float)maxDim;
    rc->boxD = (float)d / (float)maxDim;

    return true;
}

static float Clamp01f(float x) {
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

static void BuildDefaultTransferLut(std::vector<unsigned char>& outLut) {
    outLut.resize(256 * 4);
    for (int i = 0; i < 256; i++) {
        outLut[i * 4 + 0] = (unsigned char)i;
        outLut[i * 4 + 1] = (unsigned char)i;
        outLut[i * 4 + 2] = (unsigned char)i;
        outLut[i * 4 + 3] = (unsigned char)i;
    }
}

static void BuildTransferLutFromPoints(const float* pointsPacked, int pointCount, std::vector<unsigned char>& outLut) {
    struct Cp { float v, r, g, b, a; };
    std::vector<Cp> pts;
    pts.reserve((size_t)std::max(0, pointCount));

    if (!pointsPacked || pointCount <= 0) {
        BuildDefaultTransferLut(outLut);
        return;
    }

    for (int i = 0; i < pointCount; ++i) {
        const float* p = pointsPacked + i * 5;
        Cp cp;
        cp.v = Clamp01f(p[0]);
        cp.r = Clamp01f(p[1]);
        cp.g = Clamp01f(p[2]);
        cp.b = Clamp01f(p[3]);
        cp.a = Clamp01f(p[4]);
        pts.push_back(cp);
    }

    std::sort(pts.begin(), pts.end(), [](const Cp& a, const Cp& b) { return a.v < b.v; });

    outLut.resize(256 * 4);
    int seg = 0;
    for (int i = 0; i < 256; ++i) {
        const float v = (float)i / 255.0f;
        while (seg + 1 < (int)pts.size() && v > pts[seg + 1].v) {
            seg++;
        }

        const Cp c0 = pts[std::min(seg, (int)pts.size() - 1)];
        const Cp c1 = pts[std::min(seg + 1, (int)pts.size() - 1)];
        float t = 0.0f;
        if (c1.v > c0.v) {
            t = (v - c0.v) / (c1.v - c0.v);
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;
        }

        const float r = c0.r + (c1.r - c0.r) * t;
        const float g = c0.g + (c1.g - c0.g) * t;
        const float b = c0.b + (c1.b - c0.b) * t;
        const float a = c0.a + (c1.a - c0.a) * t;

        outLut[i * 4 + 0] = (unsigned char)std::round(Clamp01f(r) * 255.0f);
        outLut[i * 4 + 1] = (unsigned char)std::round(Clamp01f(g) * 255.0f);
        outLut[i * 4 + 2] = (unsigned char)std::round(Clamp01f(b) * 255.0f);
        outLut[i * 4 + 3] = (unsigned char)std::round(Clamp01f(a) * 255.0f);
    }
}

static void Raycast3D_EnsureTransferTexture(Raycast3DContext* rc, WindowContext* win) {
    if (!rc || !win) return;

    const bool needCreate = (rc->transferTex2D == 0);
    const bool needUpload = needCreate || win->transferLutDirty3D;
    if (!needUpload) return;

    std::vector<unsigned char> tf;
    if (win->transferLutCustom3D && win->transferLutRGBA3D.size() == 256 * 4) {
        tf = win->transferLutRGBA3D;
    } else {
        BuildDefaultTransferLut(tf);
    }

    if (needCreate) {
        glGenTextures(1, &rc->transferTex2D);
    }

    glBindTexture(GL_TEXTURE_2D, rc->transferTex2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, tf.data());

    win->transferLutDirty3D = false;
}

// Backward-compatible wrapper for old callsites.
static void Raycast3D_EnsureTransferTexture(Raycast3DContext* rc) {
    if (!rc) return;
    if (rc->transferTex2D) return;
    glGenTextures(1, &rc->transferTex2D);
    glBindTexture(GL_TEXTURE_2D, rc->transferTex2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    std::vector<unsigned char> tf;
    BuildDefaultTransferLut(tf);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, tf.data());
}

// Forward declaration for ParseHexColor (used by raycast mask iso-surface path).
static void ParseHexColor(const char* hexColor, float& r, float& g, float& b);

static bool Raycast3D_EnsureMaskTexture(
    Raycast3DContext* rc,
    const APRContext* axial,
    float* outMaskColorR,
    float* outMaskColorG,
    float* outMaskColorB,
    bool* outHasMask
) {
    if (outHasMask) *outHasMask = false;
    if (!rc || !axial || !axial->volume) return false;
    if (axial->sessionId.empty()) return false;

    MPRContext* mpr = GetMPRContextFromSession(axial->sessionId.c_str());
    if (!mpr || !mpr->volume) return false;

    int w = 0, h = 0, d = 0;
    if (Dicom_Volume_GetDimensions(mpr->volume, &w, &h, &d) != NATIVE_OK) return false;
    if (w <= 0 || h <= 0 || d <= 0) return false;

    // Determine if there is any visible mask data to show.
    const MPRContext::MaskData* firstVisible = nullptr;
    if (mpr->previewMask && mpr->previewMask->visible && !mpr->previewMask->data.empty()) {
        firstVisible = mpr->previewMask;
    }
    if (!firstVisible) {
        for (const auto& m : mpr->masks) {
            if (m.visible && !m.data.empty()) {
                firstVisible = &m;
                break;
            }
        }
    }
    if (!firstVisible) {
        // No mask to display.
        return true;
    }

    const bool needsRebuild =
        (rc->maskTex3D == 0) ||
        (rc->maskSessionId != axial->sessionId) ||
        (rc->maskRevision != mpr->maskRevision) ||
        (rc->maskW != w || rc->maskH != h || rc->maskD != d);

    if (needsRebuild) {
        const size_t total = static_cast<size_t>(w) * static_cast<size_t>(h) * static_cast<size_t>(d);
        rc->maskCombined.assign(total, 0);

        auto orMask = [&](const std::vector<uint8_t>& src) {
            if (src.size() != total) return;
            for (size_t i = 0; i < total; ++i) {
                if (src[i] > 0) rc->maskCombined[i] = 255;
            }
        };

        if (mpr->previewMask && mpr->previewMask->visible) {
            orMask(mpr->previewMask->data);
        }
        for (const auto& m : mpr->masks) {
            if (!m.visible) continue;
            orMask(m.data);
        }

        if (!rc->maskTex3D) {
            glGenTextures(1, &rc->maskTex3D);
        }
        glBindTexture(GL_TEXTURE_3D, rc->maskTex3D);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage3D(
            GL_TEXTURE_3D,
            0,
            GL_LUMINANCE8,
            w, h, d,
            0,
            GL_LUMINANCE,
            GL_UNSIGNED_BYTE,
            rc->maskCombined.data()
        );

        rc->maskSessionId = axial->sessionId;
        rc->maskRevision = mpr->maskRevision;
        rc->maskW = w;
        rc->maskH = h;
        rc->maskD = d;
    }

    if (outMaskColorR && outMaskColorG && outMaskColorB) {
        float r = 0.2f, g = 0.9f, b = 0.2f;
        ParseHexColor(firstVisible->color.c_str(), r, g, b);
        *outMaskColorR = r;
        *outMaskColorG = g;
        *outMaskColorB = b;
    }
    if (outHasMask) *outHasMask = true;
    return true;
}

static bool Raycast3D_EnsureBackfaceFbo(Raycast3DContext* rc, int w, int h) {
    if (!rc) return false;
    if (w <= 0 || h <= 0) return false;
    if (rc->backfaceFbo && rc->backfaceTex2D && rc->backfaceDepthRb && rc->fboW == w && rc->fboH == h) {
        return true;
    }

    if (rc->backfaceDepthRb) { glDeleteRenderbuffers(1, &rc->backfaceDepthRb); rc->backfaceDepthRb = 0; }
    if (rc->backfaceTex2D) { glDeleteTextures(1, &rc->backfaceTex2D); rc->backfaceTex2D = 0; }
    if (!rc->backfaceFbo) { glGenFramebuffers(1, &rc->backfaceFbo); }

    glBindFramebuffer(GL_FRAMEBUFFER, rc->backfaceFbo);

    glGenTextures(1, &rc->backfaceTex2D);
    glBindTexture(GL_TEXTURE_2D, rc->backfaceTex2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rc->backfaceTex2D, 0);

    glGenRenderbuffers(1, &rc->backfaceDepthRb);
    glBindRenderbuffer(GL_RENDERBUFFER, rc->backfaceDepthRb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rc->backfaceDepthRb);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        printf("[Raycast3D] Backface FBO incomplete: 0x%X\n", (unsigned int)status);
        return false;
    }

    rc->fboW = w;
    rc->fboH = h;
    return true;
}

void Raycast3D_RenderWindow(WindowContext* win) {
    if (!win || !win->is3DView || !win->aprAxial) return;
    std::lock_guard<std::mutex> lock(win->renderMutex);
    APRContext* axial = static_cast<APRContext*>(win->aprAxial);
    if (!axial || !axial->volume) return;

    Raycast3DContext* rc = static_cast<Raycast3DContext*>(win->raycast3D);
    if (!rc) {
        rc = new Raycast3DContext();
        win->raycast3D = rc;
    }

    if (!Raycast3D_EnsurePrograms(rc)) {
        return;
    }
    // Ensure a unit-cube mesh for the box.
    Raycast3D_EnsureUnitCubeVbo(&rc->boxVbo, &rc->boxVertCount);

    if (!Raycast3D_EnsureVolumeTexture(rc, axial->volume, win->vramOptimized3D)) {
        return;
    }
    Raycast3D_EnsureTransferTexture(rc, win);
    if (!Raycast3D_EnsureBackfaceFbo(rc, win->width, win->height)) {
        return;
    }

    glViewport(0, 0, win->width, win->height);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    // Build projection/view matrices on CPU (no fixed pipeline matrix stack).
    float proj[16];
    float viewT[16];
    float view[16];
    float pv[16];
    float mvp[16];

    float aspect = (win->height > 0) ? (float)win->width / (float)win->height : 1.0f;
    Mat4_Frustum(proj, -aspect * 0.1f, aspect * 0.1f, -0.1f, 0.1f, 0.2f, 10.0f);

    const float zoom = (win->viewZoom <= 0.0001f) ? 0.0001f : win->viewZoom;
    Mat4_Translation(viewT, win->viewPanX, win->viewPanY, -3.0f / zoom);
    if (!win->viewRotMatInitialized) {
        Mat4_Identity(win->viewRotMat);
        win->viewRotMatInitialized = true;
    }
    Mat4_Mul(view, viewT, win->viewRotMat); // match old: T then R
    Mat4_Mul(pv, proj, view);
    std::memcpy(mvp, pv, sizeof(float) * 16);

    // ---- Pass 1: backfaces -> exit points texture ----
    glBindFramebuffer(GL_FRAMEBUFFER, rc->backfaceFbo);
    glViewport(0, 0, rc->fboW, rc->fboH);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    glUseProgram(rc->progBackface);
    GLint locMvpB = glGetUniformLocation(rc->progBackface, "uMVP");
    if (locMvpB >= 0) glUniformMatrix4fv(locMvpB, 1, GL_FALSE, mvp);
    GLint locBox = glGetUniformLocation(rc->progBackface, "uBoxSize");
    if (locBox >= 0) glUniform3f(locBox, rc->boxW, rc->boxH, rc->boxD);
    DrawVboPositions(rc->progBackface, rc->boxVbo, rc->boxVertCount);
    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ---- Pass 2: frontfaces -> raycast ----
    glViewport(0, 0, win->width, win->height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glCullFace(GL_BACK);

    glUseProgram(rc->progRaycast);
    GLint locMvpR = glGetUniformLocation(rc->progRaycast, "uMVP");
    if (locMvpR >= 0) glUniformMatrix4fv(locMvpR, 1, GL_FALSE, mvp);
    GLint locBox2 = glGetUniformLocation(rc->progRaycast, "uBoxSize");
    if (locBox2 >= 0) glUniform3f(locBox2, rc->boxW, rc->boxH, rc->boxD);
    GLint locVp = glGetUniformLocation(rc->progRaycast, "uViewport");
    if (locVp >= 0) glUniform2f(locVp, (float)win->width, (float)win->height);
    GLint locStep = glGetUniformLocation(rc->progRaycast, "uStepSize");
    if (locStep >= 0) glUniform1f(locStep, 0.0035f);
    GLint locDen = glGetUniformLocation(rc->progRaycast, "uDensity");
    if (locDen >= 0) glUniform1f(locDen, 0.08f);

    // Mask iso-surface (optional)
    bool hasMask = false;
    float mr = 0.2f, mg = 0.9f, mb = 0.2f;
    if (win->maskIsoSurface3D) {
        Raycast3D_EnsureMaskTexture(rc, axial, &mr, &mg, &mb, &hasMask);
    }
    GLint locIsoEnabled = glGetUniformLocation(rc->progRaycast, "uMaskIsoEnabled");
    if (locIsoEnabled >= 0) glUniform1i(locIsoEnabled, (win->maskIsoSurface3D && hasMask) ? 1 : 0);
    GLint locIso = glGetUniformLocation(rc->progRaycast, "uMaskIso");
    if (locIso >= 0) glUniform1f(locIso, 0.5f);
    GLint locMaskColor = glGetUniformLocation(rc->progRaycast, "uMaskColor");
    if (locMaskColor >= 0) glUniform4f(locMaskColor, mr, mg, mb, 0.85f);
    GLint locMaskTexel = glGetUniformLocation(rc->progRaycast, "uMaskTexel");
    if (locMaskTexel >= 0) {
        const float tx = (rc->maskW > 0) ? (1.0f / (float)rc->maskW) : (1.0f / (float)rc->volW);
        const float ty = (rc->maskH > 0) ? (1.0f / (float)rc->maskH) : (1.0f / (float)rc->volH);
        const float tz = (rc->maskD > 0) ? (1.0f / (float)rc->maskD) : (1.0f / (float)rc->volD);
        glUniform3f(locMaskTexel, tx, ty, tz);
    }

    // Lighting parameters (used by mask iso-surface shading; safe to set always)
    GLint locLA = glGetUniformLocation(rc->progRaycast, "uLightAmbient");
    if (locLA >= 0) glUniform1f(locLA, win->lightAmbient3D);
    GLint locLD = glGetUniformLocation(rc->progRaycast, "uLightDiffuse");
    if (locLD >= 0) glUniform1f(locLD, win->lightDiffuse3D);
    GLint locLS = glGetUniformLocation(rc->progRaycast, "uLightSpecular");
    if (locLS >= 0) glUniform1f(locLS, win->lightSpecular3D);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, rc->backfaceTex2D);
    GLint locExit = glGetUniformLocation(rc->progRaycast, "uExitPoints");
    if (locExit >= 0) glUniform1i(locExit, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, rc->volumeTex3D);
    GLint locVol = glGetUniformLocation(rc->progRaycast, "uVolume");
    if (locVol >= 0) glUniform1i(locVol, 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, rc->transferTex2D);
    GLint locTf = glGetUniformLocation(rc->progRaycast, "uTransfer");
    if (locTf >= 0) glUniform1i(locTf, 2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_3D, rc->maskTex3D);
    GLint locMask = glGetUniformLocation(rc->progRaycast, "uMask");
    if (locMask >= 0) glUniform1i(locMask, 3);

    DrawVboPositions(rc->progRaycast, rc->boxVbo, rc->boxVertCount);

    glUseProgram(0);
    glDisable(GL_CULL_FACE);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_3D, 0);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Draw overlay primitives after volume (shader-based).
    Render3DPrimitives(win, rc, pv);

    // Debug overlay: show 3D navigation & key texture state for raycast path.
    {
        char dbg[512];
        std::snprintf(
            dbg,
            sizeof(dbg),
            "Ray3D viewZoom=%.4f pan=(%.3f,%.3f) vramOpt=%d iso=%d hasMask=%d tex(back/vol/tf/mask)=%u/%u/%u/%u fbo=%u size=%dx%d",
            win->viewZoom,
            win->viewPanX,
            win->viewPanY,
            win->vramOptimized3D ? 1 : 0,
            win->maskIsoSurface3D ? 1 : 0,
            hasMask ? 1 : 0,
            (unsigned int)rc->backfaceTex2D,
            (unsigned int)rc->volumeTex3D,
            (unsigned int)rc->transferTex2D,
            (unsigned int)rc->maskTex3D,
            (unsigned int)rc->backfaceFbo,
            win->width,
            win->height);
        DrawNanoVGDebugTextTopLeft(dbg);
    }
}

// ==================== Window3D Primitives API ====================

static int Window3D_AddPrimitiveInternal(WindowContext* ctx, Primitive3DTypeCpp type, float p0, float p1, float p2) {
    if (!ctx || !ctx->is3DView) return 0;
    Primitive3D prim;
    prim.id = ctx->nextPrimitiveId++;
    prim.type = type;
    prim.p0 = p0;
    prim.p1 = p1;
    prim.p2 = p2;
    // Default placement: near center
    prim.tx = 0.0f;
    prim.ty = 0.0f;
    prim.tz = 0.0f;
    ctx->primitives.push_back(prim);
    return prim.id;
}

VIZ_API int Window3D_AddCube(WindowHandle handle, float sizeX, float sizeY, float sizeZ) {
    if (!handle) return 0;
    auto ctx = static_cast<WindowContext*>(handle);
    if (sizeX <= 0 || sizeY <= 0 || sizeZ <= 0) return 0;
    std::lock_guard<std::mutex> lock(ctx->renderMutex);
    return Window3D_AddPrimitiveInternal(ctx, Primitive3DTypeCpp::Cube, sizeX, sizeY, sizeZ);
}

VIZ_API int Window3D_AddSphere(WindowHandle handle, float radius) {
    if (!handle) return 0;
    auto ctx = static_cast<WindowContext*>(handle);
    if (radius <= 0) return 0;
    std::lock_guard<std::mutex> lock(ctx->renderMutex);
    return Window3D_AddPrimitiveInternal(ctx, Primitive3DTypeCpp::Sphere, radius, 0.0f, 0.0f);
}

VIZ_API int Window3D_AddCylinder(WindowHandle handle, float radius, float height) {
    if (!handle) return 0;
    auto ctx = static_cast<WindowContext*>(handle);
    if (radius <= 0 || height <= 0) return 0;
    std::lock_guard<std::mutex> lock(ctx->renderMutex);
    return Window3D_AddPrimitiveInternal(ctx, Primitive3DTypeCpp::Cylinder, radius, height, 0.0f);
}

VIZ_API NativeResult Window3D_RemovePrimitive(WindowHandle handle, int primitiveId) {
    if (!handle || primitiveId <= 0) return NATIVE_E_INVALID_ARGUMENT;
    auto ctx = static_cast<WindowContext*>(handle);
    std::lock_guard<std::mutex> lock(ctx->renderMutex);
    auto& v = ctx->primitives;
    auto it = std::remove_if(v.begin(), v.end(), [&](const Primitive3D& p) { return p.id == primitiveId; });
    if (it == v.end()) return NATIVE_E_INVALID_ARGUMENT;
    v.erase(it, v.end());
    return NATIVE_OK;
}

VIZ_API void Window3D_ClearPrimitives(WindowHandle handle) {
    if (!handle) return;
    auto ctx = static_cast<WindowContext*>(handle);
    std::lock_guard<std::mutex> lock(ctx->renderMutex);
    ctx->primitives.clear();
}

VIZ_API NativeResult Window3D_SetPrimitiveTransform(
    WindowHandle handle,
    int primitiveId,
    float tx, float ty, float tz,
    float rxDeg, float ryDeg, float rzDeg,
    float sx, float sy, float sz
) {
    if (!handle || primitiveId <= 0) return NATIVE_E_INVALID_ARGUMENT;
    auto ctx = static_cast<WindowContext*>(handle);
    std::lock_guard<std::mutex> lock(ctx->renderMutex);
    Primitive3D* prim = FindPrimitiveById(ctx, primitiveId);
    if (!prim) return NATIVE_E_INVALID_ARGUMENT;
    prim->tx = tx; prim->ty = ty; prim->tz = tz;
    prim->rx = rxDeg; prim->ry = ryDeg; prim->rz = rzDeg;
    prim->sx = sx; prim->sy = sy; prim->sz = sz;
    return NATIVE_OK;
}

VIZ_API NativeResult Window3D_SetPrimitiveColor(WindowHandle handle, int primitiveId, float r, float g, float b, float a) {
    if (!handle || primitiveId <= 0) return NATIVE_E_INVALID_ARGUMENT;
    auto ctx = static_cast<WindowContext*>(handle);
    std::lock_guard<std::mutex> lock(ctx->renderMutex);
    Primitive3D* prim = FindPrimitiveById(ctx, primitiveId);
    if (!prim) return NATIVE_E_INVALID_ARGUMENT;
    prim->r = r; prim->g = g; prim->b = b; prim->a = a;
    return NATIVE_OK;
}

VIZ_API NativeResult Window3D_SetPrimitiveVisible(WindowHandle handle, int primitiveId, bool visible) {
    if (!handle || primitiveId <= 0) return NATIVE_E_INVALID_ARGUMENT;
    auto ctx = static_cast<WindowContext*>(handle);
    std::lock_guard<std::mutex> lock(ctx->renderMutex);
    Primitive3D* prim = FindPrimitiveById(ctx, primitiveId);
    if (!prim) return NATIVE_E_INVALID_ARGUMENT;
    prim->visible = visible;
    return NATIVE_OK;
}

VIZ_API NativeResult Window3D_SetSceneTransform(
    WindowHandle handle,
    float tx, float ty, float tz,
    float rxDeg, float ryDeg, float rzDeg,
    float sx, float sy, float sz
) {
    if (!handle) return NATIVE_E_INVALID_ARGUMENT;
    auto ctx = static_cast<WindowContext*>(handle);
    std::lock_guard<std::mutex> lock(ctx->renderMutex);
    ctx->sceneTransform.tx = tx;
    ctx->sceneTransform.ty = ty;
    ctx->sceneTransform.tz = tz;
    ctx->sceneTransform.rx = rxDeg;
    ctx->sceneTransform.ry = ryDeg;
    ctx->sceneTransform.rz = rzDeg;
    ctx->sceneTransform.sx = sx;
    ctx->sceneTransform.sy = sy;
    ctx->sceneTransform.sz = sz;
    return NATIVE_OK;
}

// 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷专锟矫结构锟斤拷锟斤拷锟斤拷使锟斤拷GLFW锟斤拷
struct OffscreenContext {
#if !defined(_WIN32)
    GLFWwindow* window = nullptr;
#else
    HWND hwnd = nullptr;
    HDC hdc = nullptr;
    HGLRC hglrc = nullptr;
#endif
    int width = 800, height = 600;
};

void Window_Invalidate(WindowHandle handle);

#ifdef _WIN32
// Win32锟斤拷锟斤拷锟斤拷锟斤拷
static const wchar_t* g_WindowClassName = L"VisualizationWindow";
static bool g_WindowClassRegistered = false;
static HGLRC g_SharedGLContext = nullptr;  // 锟斤拷锟斤拷锟斤拷OpenGL锟斤拷锟斤拷锟斤拷

// ==================== Win32 专锟矫革拷锟斤拷锟斤拷锟斤拷 ====================
static double GetWin32TimeSeconds() {
    static double invFreq = 0.0;
    static bool initialized = false;
    LARGE_INTEGER freq{};
    LARGE_INTEGER counter{};

    if (!initialized) {
        if (QueryPerformanceFrequency(&freq)) {
            invFreq = 1.0 / static_cast<double>(freq.QuadPart);
            initialized = true;
        } else {
            return 0.0;
        }
    }

    if (!QueryPerformanceCounter(&counter)) {
        return 0.0;
    }

    return static_cast<double>(counter.QuadPart) * invFreq;
}

static MeasurementPoint ConstrainSquareInPlane(const MeasurementPoint& start, const MeasurementPoint& end, int sliceDirection) {
    MeasurementPoint out = end;
    const float dx = end.x - start.x;
    const float dy = end.y - start.y;
    const float dz = end.z - start.z;

    if (sliceDirection == 0) { // Axial (XY)
        const float size = fmaxf(fabsf(dx), fabsf(dy));
        out.x = start.x + (dx >= 0 ? size : -size);
        out.y = start.y + (dy >= 0 ? size : -size);
    } else if (sliceDirection == 1) { // Coronal (XZ)
        const float size = fmaxf(fabsf(dx), fabsf(dz));
        out.x = start.x + (dx >= 0 ? size : -size);
        out.z = start.z + (dz >= 0 ? size : -size);
    } else { // Sagittal (YZ)
        const float size = fmaxf(fabsf(dy), fabsf(dz));
        out.y = start.y + (dy >= 0 ? size : -size);
        out.z = start.z + (dz >= 0 ? size : -size);
    }
    return out;
}

static bool Win32_TryGetActiveSliceMapping(WindowContext* ctx, TexWindowMapping& outMap) {
    outMap = TexWindowMapping{};
    if (!ctx || !ctx->boundRenderer || ctx->width <= 0 || ctx->height <= 0) return false;

    const bool isMPR = ctx->rendererType == 1;
    const bool isAPR = (ctx->rendererType == 0 || ctx->rendererType == 2);
    if (!isMPR && !isAPR) return false;

    int width = 0, height = 0, depth = 0;
    int sliceWidth = 0, sliceHeight = 0;
    float crossTexX = 0.5f, crossTexY = 0.5f;
    float zoomFactor = 1.0f;

    if (isMPR) {
        auto mprCtx = static_cast<MPRContext*>(ctx->boundRenderer);
        if (!mprCtx || !mprCtx->volume) return false;
        if (Dicom_Volume_GetDimensions(mprCtx->volume, &width, &height, &depth) != NATIVE_OK) return false;
        if (width <= 0 || height <= 0 || depth <= 0) return false;

        if (mprCtx->sliceDirection == 0) { sliceWidth = width; sliceHeight = height; }
        else if (mprCtx->sliceDirection == 1) { sliceWidth = width; sliceHeight = depth; }
        else { sliceWidth = height; sliceHeight = depth; }

        if (mprCtx->sliceDirection == 0) {
            crossTexX = mprCtx->centerX / std::max(1.0f, (width - 1.0f));
            crossTexY = 1.0f - (mprCtx->centerY / std::max(1.0f, (height - 1.0f)));
        } else if (mprCtx->sliceDirection == 1) {
            crossTexX = mprCtx->centerX / std::max(1.0f, (width - 1.0f));
            crossTexY = 1.0f - (mprCtx->centerZ / std::max(1.0f, (depth - 1.0f)));
        } else {
            crossTexX = mprCtx->centerY / std::max(1.0f, (height - 1.0f));
            crossTexY = 1.0f - (mprCtx->centerZ / std::max(1.0f, (depth - 1.0f)));
        }
        zoomFactor = mprCtx->zoomFactor;
    } else {
        auto aprCtx = static_cast<APRContext*>(ctx->boundRenderer);
        if (!aprCtx || !aprCtx->volume) return false;
        if (Dicom_Volume_GetDimensions(aprCtx->volume, &width, &height, &depth) != NATIVE_OK) return false;
        if (width <= 0 || height <= 0 || depth <= 0) return false;

        if (aprCtx->sliceDirection == 0) { sliceWidth = width; sliceHeight = height; }
        else if (aprCtx->sliceDirection == 1) { sliceWidth = width; sliceHeight = depth; }
        else { sliceWidth = height; sliceHeight = depth; }

        crossTexX = aprCtx->crosshairU / std::max(1.0f, (sliceWidth - 1.0f));
        crossTexY = 1.0f - (aprCtx->crosshairV / std::max(1.0f, (sliceHeight - 1.0f)));
        zoomFactor = aprCtx->zoomFactor;
    }

    outMap = ComputeTexWindowMapping(sliceWidth, sliceHeight, ctx->width, ctx->height, zoomFactor, crossTexX, crossTexY);
    return outMap.valid;
}

static bool Win32_MousePixelsToImageNdc(WindowContext* ctx, double mouseX, double mouseY, Point2D& outImageNdc, bool clampToImageRect) {
    if (!ctx || ctx->width <= 0 || ctx->height <= 0) return false;

    TexWindowMapping map;
    if (!Win32_TryGetActiveSliceMapping(ctx, map)) return false;

    const float screenNdcX = static_cast<float>(mouseX / ctx->width) * 2.0f - 1.0f;
    const float screenNdcY = 1.0f - static_cast<float>(mouseY / ctx->height) * 2.0f;
    Point2D screenNdc{screenNdcX, screenNdcY};

    return ScreenNdcToImageNdc(map, screenNdc, outImageNdc, clampToImageRect);
}

static bool Win32_HandleMeasurementMouseDown(WindowContext* ctx, double mouseX, double mouseY) {
    const bool isMPR = ctx && ctx->rendererType == 1;
    const bool isAPR = ctx && (ctx->rendererType == 0 || ctx->rendererType == 2);

    if (!ctx || !(isMPR || isAPR) || !ctx->boundRenderer || !ctx->activeTool) {
        return false;
    }

    if (g_currentToolType < 1 || g_currentToolType > 6) {
        return false;
    }

    if (ctx->width <= 0 || ctx->height <= 0) {
        return false;
    }

    // Mouse pixels -> screen NDC
    const float ndcX = static_cast<float>(mouseX / ctx->width) * 2.0f - 1.0f;
    const float ndcY = 1.0f - static_cast<float>(mouseY / ctx->height) * 2.0f;

    // Mouse pixels -> image/view NDC (matches RenderTextureToWindow mapping)
    Point2D imageNdc{0, 0};
    if (!Win32_MousePixelsToImageNdc(ctx, mouseX, mouseY, imageNdc, /*clampToImageRect*/ false)) {
        // Click in letterbox/pillarbox area -> ignore for measurement tools.
        return false;
    }

    TexWindowMapping measureMap;
    const bool hasMeasureMap = Win32_TryGetActiveSliceMapping(ctx, measureMap);

    // Prefer editing existing measurements when clicking on a control point.
    auto ShouldDisplayMeasurement = [ctx](const MeasurementLocation& loc) -> bool {
        if (!ctx || !ctx->boundRenderer) return false;

        if (ctx->rendererType == 1) {
            // MPR does not display APR measurements
            if (loc.isAPR) return false;
            auto mprCtx = static_cast<MPRContext*>(ctx->boundRenderer);
            if (loc.sliceDirection != mprCtx->sliceDirection) return false;

            int currentSliceIndex = 0;
            if (mprCtx->sliceDirection == 0) currentSliceIndex = (int)(mprCtx->centerZ + 0.5f);
            else if (mprCtx->sliceDirection == 1) currentSliceIndex = (int)(mprCtx->centerY + 0.5f);
            else currentSliceIndex = (int)(mprCtx->centerX + 0.5f);
            return loc.sliceIndex == currentSliceIndex;
        }

        // APR windows
        if (!loc.isAPR) return false;
        auto aprCtx = static_cast<APRContext*>(ctx->boundRenderer);
        if (loc.sliceDirection != aprCtx->sliceDirection) return false;

        int currentSliceIndex = 0;
        if (aprCtx->sliceDirection == 0) currentSliceIndex = (int)(aprCtx->centerZ + 0.5f);
        else if (aprCtx->sliceDirection == 1) currentSliceIndex = (int)(aprCtx->centerY + 0.5f);
        else currentSliceIndex = (int)(aprCtx->centerX + 0.5f);
        if (loc.sliceIndex != currentSliceIndex) return false;

        // Rotation gating: keep in sync with render filtering (currently uses rotZ tolerance)
        float curX = 0.0f, curY = 0.0f, curZ = 0.0f;
        Mat4_ExtractEulerZYXDeg(aprCtx->rotMat, &curX, &curY, &curZ);
        float rotDiff = fabsf(loc.rotZ - curZ);
        return rotDiff <= 1.0f;
    };

    auto HitTestControlPoint = [&](int& outMeasurementIndex, int& outPointIndex) -> bool {
        outMeasurementIndex = -1;
        outPointIndex = -1;

        const float radiusPx = 8.0f;
        float rNdc = 2.0f * radiusPx / (float)std::max(1, std::min(ctx->width, ctx->height));
        float r2 = rNdc * rNdc;
        float bestD2 = 1e9f;

        std::lock_guard<std::mutex> lock(g_completedMeasurementsMutex);
        for (size_t mi = 0; mi < g_completedMeasurements.size(); ++mi) {
            const auto& m = g_completedMeasurements[mi];
            if (m.toolType == 6) continue; // freehand has no control points
            if (!ShouldDisplayMeasurement(m.location)) continue;

            for (size_t pi = 0; pi < m.points.size(); ++pi) {
                Point2D pNdc{0, 0};
                if (ctx->rendererType == 1) {
                    auto mprCtx = static_cast<MPRContext*>(ctx->boundRenderer);
                    pNdc = MPR_WorldToNDC_Fixed(m.points[pi], mprCtx, m.location.centerX, m.location.centerY, m.location.centerZ);
                } else {
                    auto aprCtx = static_cast<APRContext*>(ctx->boundRenderer);
                    pNdc = APR_WorldToNDC_Fixed(m.points[pi], aprCtx, m.location.centerX, m.location.centerY, m.location.centerZ);
                }
                if (hasMeasureMap) {
                    pNdc = ImageNdcToScreenNdc(measureMap, pNdc);
                }
                float dx = pNdc.x - ndcX;
                float dy = pNdc.y - ndcY;
                float d2 = dx * dx + dy * dy;
                if (d2 <= r2 && d2 < bestD2) {
                    bestD2 = d2;
                    outMeasurementIndex = (int)mi;
                    outPointIndex = (int)pi;
                }
            }
        }

        return outMeasurementIndex >= 0 && outPointIndex >= 0;
    };

    int hitMi = -1;
    int hitPi = -1;
    if (HitTestControlPoint(hitMi, hitPi)) {
        g_hoverMeasurementIndex = hitMi;
        g_hoverPointIndex = hitPi;
        g_dragMeasurementIndex = hitMi;
        g_dragPointIndex = hitPi;
        g_isDraggingPoint = true;
        if (ctx->hwnd) {
            SetCapture(ctx->hwnd);
            InvalidateRect(ctx->hwnd, NULL, FALSE);
        }
        return true;
    }

    MeasurementPoint worldPos{};
    if (isMPR) {
        auto mprCtx = static_cast<MPRContext*>(ctx->boundRenderer);
        worldPos = MPR_NDCToWorld(imageNdc.x, imageNdc.y, mprCtx);
    } else if (isAPR) {
        auto aprCtx = static_cast<APRContext*>(ctx->boundRenderer);
        worldPos = APR_NDCToWorld(imageNdc.x, imageNdc.y, aprCtx);
    }

    double currentTime = GetWin32TimeSeconds();
    bool isDoubleClick = (currentTime - g_lastClickTime) < 0.3;
    g_lastClickTime = currentTime;

    if (g_measurementPoints.empty()) {
        g_currentMeasurementLocation = MeasurementLocation();

        if (isMPR) {
            auto mprCtx = static_cast<MPRContext*>(ctx->boundRenderer);
            g_currentMeasurementLocation.sliceDirection = mprCtx->sliceDirection;
            g_currentMeasurementLocation.isAPR = false;
            g_currentMeasurementLocation.centerX = mprCtx->centerX;
            g_currentMeasurementLocation.centerY = mprCtx->centerY;
            g_currentMeasurementLocation.centerZ = mprCtx->centerZ;

            if (mprCtx->sliceDirection == 0) {
                g_currentMeasurementLocation.sliceIndex = static_cast<int>(mprCtx->centerZ + 0.5f);
            } else if (mprCtx->sliceDirection == 1) {
                g_currentMeasurementLocation.sliceIndex = static_cast<int>(mprCtx->centerY + 0.5f);
            } else {
                g_currentMeasurementLocation.sliceIndex = static_cast<int>(mprCtx->centerX + 0.5f);
            }
        } else if (isAPR) {
            auto aprCtx = static_cast<APRContext*>(ctx->boundRenderer);
            g_currentMeasurementLocation.sliceDirection = aprCtx->sliceDirection;
            g_currentMeasurementLocation.isAPR = true;
            Mat4_ExtractEulerZYXDeg(aprCtx->rotMat,
                &g_currentMeasurementLocation.rotX,
                &g_currentMeasurementLocation.rotY,
                &g_currentMeasurementLocation.rotZ);
            g_currentMeasurementLocation.centerX = aprCtx->centerX;
            g_currentMeasurementLocation.centerY = aprCtx->centerY;
            g_currentMeasurementLocation.centerZ = aprCtx->centerZ;

            if (aprCtx->sliceDirection == 0) {
                g_currentMeasurementLocation.sliceIndex = static_cast<int>(aprCtx->centerZ + 0.5f);
            } else if (aprCtx->sliceDirection == 1) {
                g_currentMeasurementLocation.sliceIndex = static_cast<int>(aprCtx->centerY + 0.5f);
            } else {
                g_currentMeasurementLocation.sliceIndex = static_cast<int>(aprCtx->centerX + 0.5f);
            }
        }
    }

    switch (g_currentToolType) {
        case 1:
        case 3:
        case 4:
            g_isDrawing = true;
            g_measurementPoints.clear();
            g_measurementPoints.push_back(worldPos);
            g_currentMousePos = worldPos;
            break;
        case 2:  // 锟角度癸拷锟竭ｏ拷锟斤拷锟轿碉拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟?
            g_measurementPoints.push_back(worldPos);
            
            // 锟斤拷锟斤拷丫锟斤拷锟?锟斤拷锟姐，锟皆讹拷锟斤拷山嵌炔锟斤拷锟?
            if (g_measurementPoints.size() == 3) {
                // 锟斤拷锟斤拷嵌龋锟絧oint[0] - point[1] - point[2]锟斤拷point[1]锟角讹拷锟斤拷
                float angle = CalculateAngle(g_measurementPoints[0], g_measurementPoints[1], g_measurementPoints[2]);
                
                CompletedMeasurement measurement;
                measurement.sessionId = GetSessionIdFromWindowContext(ctx);
                measurement.id = g_nextMeasurementId++;
                measurement.toolType = 2;
                measurement.points = g_measurementPoints;
                measurement.result = angle;
                measurement.location = g_currentMeasurementLocation;
                {
                    std::lock_guard<std::mutex> lock(g_completedMeasurementsMutex);
                    g_completedMeasurements.push_back(measurement);
                }
                
                g_measurementPoints.clear();
            }
            break;
        case 5:
            if (isDoubleClick && g_measurementPoints.size() >= 2) {
                CompletedMeasurement measurement;
                measurement.sessionId = GetSessionIdFromWindowContext(ctx);
                measurement.id = g_nextMeasurementId++;
                measurement.toolType = 5;
                measurement.points = g_measurementPoints;
                measurement.result = CalculatePolylineLength(measurement.points);
                measurement.location = g_currentMeasurementLocation;
                {
                    std::lock_guard<std::mutex> lock(g_completedMeasurementsMutex);
                    g_completedMeasurements.push_back(measurement);
                }
                g_measurementPoints.clear();
            } else {
                g_measurementPoints.push_back(worldPos);
            }
            break;
        case 6:
            g_isDrawing = true;
            g_measurementPoints.clear();
            g_measurementPoints.push_back(worldPos);
            break;
        default:
            return false;
    }

    if (ctx->activeTool) {
        // Tool_* uses a simplified 2D path; keep feeding screen NDC so it stays consistent.
        Tool_AddPoint(ctx->activeTool, ndcX, ndcY);
    }

    return true;
}

static void Win32_HandleMeasurementMouseUp(WindowContext* ctx, double mouseX, double mouseY) {
    if (g_isDraggingPoint) {
        g_isDraggingPoint = false;
        g_dragMeasurementIndex = -1;
        g_dragPointIndex = -1;
        if (ctx && ctx->hwnd) {
            ReleaseCapture();
            InvalidateRect(ctx->hwnd, NULL, FALSE);
        }
        return;
    }
    if (!ctx || !(ctx->rendererType == 0 || ctx->rendererType == 1 || ctx->rendererType == 2) || !ctx->boundRenderer || !ctx->activeTool) {
        g_isDrawing = false;
        return;
    }

    if (!g_isDrawing || g_measurementPoints.empty()) {
        g_isDrawing = false;
        return;
    }

    if (ctx->width <= 0 || ctx->height <= 0) {
        g_isDrawing = false;
        return;
    }

    Point2D imageNdc{0, 0};
    if (!Win32_MousePixelsToImageNdc(ctx, mouseX, mouseY, imageNdc, /*clampToImageRect*/ true)) {
        g_measurementPoints.clear();
        g_isDrawing = false;
        return;
    }

    MeasurementPoint releasePos{};
    if (ctx->rendererType == 1) {
        auto mprCtx = static_cast<MPRContext*>(ctx->boundRenderer);
        releasePos = MPR_NDCToWorld(imageNdc.x, imageNdc.y, mprCtx);
    } else {
        auto aprCtx = static_cast<APRContext*>(ctx->boundRenderer);
        releasePos = APR_NDCToWorld(imageNdc.x, imageNdc.y, aprCtx);
    }

    switch (g_currentToolType) {
        case 1: {
            if (g_measurementPoints.size() >= 1) {
                g_measurementPoints.push_back(releasePos);
                float distance = CalculateDistance(g_measurementPoints[0], g_measurementPoints[1]);
                CompletedMeasurement measurement;
                measurement.sessionId = GetSessionIdFromWindowContext(ctx);
                measurement.id = g_nextMeasurementId++;
                measurement.toolType = 1;
                measurement.points = g_measurementPoints;
                measurement.result = distance;
                measurement.location = g_currentMeasurementLocation;
                {
                    std::lock_guard<std::mutex> lock(g_completedMeasurementsMutex);
                    g_completedMeasurements.push_back(measurement);
                }
            }
            break;
        }
        case 2: {
            // 锟角度癸拷锟斤拷锟斤拷mousedown时锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷拥悖拷锟斤拷锟斤拷锟斤拷锟绞憋拷远锟斤拷锟缴ｏ拷
            // mouseup锟斤拷锟斤拷要锟斤拷锟解处锟斤拷
            break;
        }
        case 3: {
            if (g_measurementPoints.size() >= 1) {
                MeasurementPoint endPos = releasePos;
                if (g_shiftPressed) {
                    endPos = ConstrainSquareInPlane(g_measurementPoints[0], endPos, g_currentMeasurementLocation.sliceDirection);
                }
                g_measurementPoints.push_back(endPos);
                float area = CalculateRectangleAreaInPlane(g_measurementPoints[0], g_measurementPoints[1], g_currentMeasurementLocation.sliceDirection);
                CompletedMeasurement measurement;
                measurement.sessionId = GetSessionIdFromWindowContext(ctx);
                measurement.id = g_nextMeasurementId++;
                measurement.toolType = 3;
                measurement.points = g_measurementPoints;
                measurement.result = area;
                measurement.location = g_currentMeasurementLocation;
                {
                    std::lock_guard<std::mutex> lock(g_completedMeasurementsMutex);
                    g_completedMeasurements.push_back(measurement);
                }
            }
            break;
        }
        case 4: {
            if (g_measurementPoints.size() >= 1) {
                MeasurementPoint startPos = g_measurementPoints[0];
                MeasurementPoint endPos = releasePos;
                if (g_shiftPressed) {
                    endPos = ConstrainSquareInPlane(startPos, endPos, g_currentMeasurementLocation.sliceDirection);
                }

                g_measurementPoints.push_back(endPos);
                float area = CalculateEllipseAreaInPlane(g_measurementPoints[0], g_measurementPoints[1], g_currentMeasurementLocation.sliceDirection);
                CompletedMeasurement measurement;
                measurement.sessionId = GetSessionIdFromWindowContext(ctx);
                measurement.id = g_nextMeasurementId++;
                measurement.toolType = 4;
                measurement.points = g_measurementPoints;
                measurement.result = area;
                measurement.location = g_currentMeasurementLocation;
                {
                    std::lock_guard<std::mutex> lock(g_completedMeasurementsMutex);
                    g_completedMeasurements.push_back(measurement);
                }
            }
            break;
        }
        case 6: {
            if (g_measurementPoints.size() >= 3) {
                printf("[Win32_Measurement] Freehand completed with %zu points\n", g_measurementPoints.size());
                CompletedMeasurement measurement;
                measurement.sessionId = GetSessionIdFromWindowContext(ctx);
                measurement.id = g_nextMeasurementId++;
                measurement.toolType = 6;
                measurement.points = g_measurementPoints;
                measurement.result = CalculatePolylineLength(measurement.points);
                measurement.location = g_currentMeasurementLocation;
                {
                    std::lock_guard<std::mutex> lock(g_completedMeasurementsMutex);
                    g_completedMeasurements.push_back(measurement);
                }
            }
            break;
        }
        default:
            break;
    }

    g_measurementPoints.clear();
    g_isDrawing = false;
}

static void Win32_UpdateMeasurementCursor(WindowContext* ctx, double mouseX, double mouseY) {
    const bool isMPR = ctx && ctx->rendererType == 1;
    const bool isAPR = ctx && (ctx->rendererType == 0 || ctx->rendererType == 2);
    
    if (!ctx || !(isMPR || isAPR) || !ctx->boundRenderer || !ctx->activeTool) {
        return;
    }
    if (g_currentToolType < 1 || g_currentToolType > 6) {
        return;
    }
    if (ctx->width <= 0 || ctx->height <= 0) {
        return;
    }

    const float ndcX = static_cast<float>(mouseX / ctx->width) * 2.0f - 1.0f;
    const float ndcY = 1.0f - static_cast<float>(mouseY / ctx->height) * 2.0f;

    TexWindowMapping measureMap;
    const bool hasMeasureMap = Win32_TryGetActiveSliceMapping(ctx, measureMap);

    // Update cursor world position using image/view NDC mapping.
    {
        Point2D imageNdc{0, 0};
        const bool clamp = g_isDrawing || g_isDraggingPoint;
        if (Win32_MousePixelsToImageNdc(ctx, mouseX, mouseY, imageNdc, clamp)) {
            if (isMPR) {
                auto mprCtx = static_cast<MPRContext*>(ctx->boundRenderer);
                g_currentMousePos = MPR_NDCToWorld(imageNdc.x, imageNdc.y, mprCtx);
            } else {
                auto aprCtx = static_cast<APRContext*>(ctx->boundRenderer);
                g_currentMousePos = APR_NDCToWorld(imageNdc.x, imageNdc.y, aprCtx);
            }
        }
    }

    auto ShouldDisplayMeasurement = [ctx](const MeasurementLocation& loc) -> bool {
        if (!ctx || !ctx->boundRenderer) return false;

        if (ctx->rendererType == 1) {
            if (loc.isAPR) return false;
            auto mprCtx = static_cast<MPRContext*>(ctx->boundRenderer);
            if (loc.sliceDirection != mprCtx->sliceDirection) return false;
            int currentSliceIndex = 0;
            if (mprCtx->sliceDirection == 0) currentSliceIndex = (int)(mprCtx->centerZ + 0.5f);
            else if (mprCtx->sliceDirection == 1) currentSliceIndex = (int)(mprCtx->centerY + 0.5f);
            else currentSliceIndex = (int)(mprCtx->centerX + 0.5f);
            return loc.sliceIndex == currentSliceIndex;
        }

        if (!loc.isAPR) return false;
        auto aprCtx = static_cast<APRContext*>(ctx->boundRenderer);
        if (loc.sliceDirection != aprCtx->sliceDirection) return false;
        int currentSliceIndex = 0;
        if (aprCtx->sliceDirection == 0) currentSliceIndex = (int)(aprCtx->centerZ + 0.5f);
        else if (aprCtx->sliceDirection == 1) currentSliceIndex = (int)(aprCtx->centerY + 0.5f);
        else currentSliceIndex = (int)(aprCtx->centerX + 0.5f);
        if (loc.sliceIndex != currentSliceIndex) return false;
        float curX = 0.0f, curY = 0.0f, curZ = 0.0f;
        Mat4_ExtractEulerZYXDeg(aprCtx->rotMat, &curX, &curY, &curZ);
        float rotDiff = fabsf(loc.rotZ - curZ);
        return rotDiff <= 1.0f;
    };

    // Drag-to-edit update
    {
        std::lock_guard<std::mutex> lock(g_completedMeasurementsMutex);
        if (g_isDraggingPoint && g_dragMeasurementIndex >= 0 && g_dragMeasurementIndex < (int)g_completedMeasurements.size()) {
            auto& m = g_completedMeasurements[(size_t)g_dragMeasurementIndex];
            if (g_dragPointIndex >= 0 && g_dragPointIndex < (int)m.points.size()) {
                MeasurementPoint newWorld{};
                Point2D imageNdc{0, 0};
                if (Win32_MousePixelsToImageNdc(ctx, mouseX, mouseY, imageNdc, /*clampToImageRect*/ true)) {
                    if (ctx->rendererType == 1) {
                        auto mprCtx = static_cast<MPRContext*>(ctx->boundRenderer);
                        newWorld = MPR_NDCToWorld_Fixed(imageNdc.x, imageNdc.y, mprCtx, m.location.centerX, m.location.centerY, m.location.centerZ);
                    } else {
                        auto aprCtx = static_cast<APRContext*>(ctx->boundRenderer);
                        newWorld = APR_NDCToWorld_Fixed(imageNdc.x, imageNdc.y, aprCtx, m.location.centerX, m.location.centerY, m.location.centerZ);
                    }
                } else {
                    return;
                }

                m.points[(size_t)g_dragPointIndex] = newWorld;
                m.result = RecomputeCompletedMeasurementResult(m);
                if (ctx->hwnd) {
                    InvalidateRect(ctx->hwnd, NULL, FALSE);
                }
            }
            return;
        }
    }

    // Hover hit-test update (only when not dragging)
    const float radiusPx = 8.0f;
    float rNdc = 2.0f * radiusPx / (float)std::max(1, std::min(ctx->width, ctx->height));
    float r2 = rNdc * rNdc;

    int bestMi = -1;
    int bestPi = -1;
    float bestD2 = 1e9f;

    {
        std::lock_guard<std::mutex> lock(g_completedMeasurementsMutex);
        for (size_t mi = 0; mi < g_completedMeasurements.size(); ++mi) {
            const auto& m = g_completedMeasurements[mi];
            if (m.toolType == 6) continue;
            if (!ShouldDisplayMeasurement(m.location)) continue;
            for (size_t pi = 0; pi < m.points.size(); ++pi) {
                Point2D pNdc{0, 0};
                if (ctx->rendererType == 1) {
                    auto mprCtx = static_cast<MPRContext*>(ctx->boundRenderer);
                    pNdc = MPR_WorldToNDC_Fixed(m.points[pi], mprCtx, m.location.centerX, m.location.centerY, m.location.centerZ);
                } else {
                    auto aprCtx = static_cast<APRContext*>(ctx->boundRenderer);
                    pNdc = APR_WorldToNDC_Fixed(m.points[pi], aprCtx, m.location.centerX, m.location.centerY, m.location.centerZ);
                }
                if (hasMeasureMap) {
                    pNdc = ImageNdcToScreenNdc(measureMap, pNdc);
                }
                float dx = pNdc.x - ndcX;
                float dy = pNdc.y - ndcY;
                float d2 = dx * dx + dy * dy;
                if (d2 <= r2 && d2 < bestD2) {
                    bestD2 = d2;
                    bestMi = (int)mi;
                    bestPi = (int)pi;
                }
            }
        }
    }

    if (bestMi != g_hoverMeasurementIndex || bestPi != g_hoverPointIndex) {
        g_hoverMeasurementIndex = bestMi;
        g_hoverPointIndex = bestPi;
        if (ctx->hwnd) {
            InvalidateRect(ctx->hwnd, NULL, FALSE);
        }
    }

    if (g_isDrawing && g_currentToolType == 6) {
        if (g_measurementPoints.empty() || CalculateDistance(g_measurementPoints.back(), g_currentMousePos) > 1.0f) {
            g_measurementPoints.push_back(g_currentMousePos);
        }
    }

    // 锟斤拷锟斤拷锟斤拷贫锟绞憋拷锟斤拷锟斤拷鼗锟斤拷锟斤拷锟绞撅拷嵌群捅锟斤拷锟斤拷锟皆わ拷锟?
    if ((g_currentToolType == 5 && !g_measurementPoints.empty()) ||
        (g_currentToolType == 2 && !g_measurementPoints.empty())) {
        if (ctx->hwnd) {
            InvalidateRect(ctx->hwnd, NULL, FALSE);
        }
    }
}

static bool Win32_HandleCropBoxMouseDown(WindowContext* ctx, double mouseX, double mouseY, bool shiftPressed) {
    // Accept rendererType 0 (APR) and 2 (APR offscreen)
    const bool isAPR = ctx && (ctx->rendererType == 0 || ctx->rendererType == 2);
    
    if (!ctx || !g_aprCropBox.enabled || !ctx->cropBoxVisible || !isAPR || !ctx->boundRenderer || g_currentToolType != 0) {
        return false;
    }

    auto aprCtx = static_cast<APRContext*>(ctx->boundRenderer);
    if (!aprCtx || !aprCtx->volume) {
        return false;
    }

    int width = 0, height = 0, depth = 0;
    if (Dicom_Volume_GetDimensions(aprCtx->volume, &width, &height, &depth) != NATIVE_OK || width <= 0 || height <= 0 || depth <= 0) {
        return false;
    }

    int sliceWidth = 0, sliceHeight = 0;
    float crossTexX = 0.5f, crossTexY = 0.5f;
    if (aprCtx->sliceDirection == 0) {
        sliceWidth = width;
        sliceHeight = height;
        crossTexX = aprCtx->crosshairU / std::max(1, (width - 1));
        crossTexY = 1.0f - (aprCtx->crosshairV / std::max(1, (height - 1)));
    } else if (aprCtx->sliceDirection == 1) {
        sliceWidth = width;
        sliceHeight = depth;
        crossTexX = aprCtx->crosshairU / std::max(1, (width - 1));
        crossTexY = 1.0f - (aprCtx->crosshairV / std::max(1, (depth - 1)));
    } else {
        sliceWidth = height;
        sliceHeight = depth;
        crossTexX = aprCtx->crosshairU / std::max(1, (height - 1));
        crossTexY = 1.0f - (aprCtx->crosshairV / std::max(1, (depth - 1)));
    }

    const int winWidth = std::max(1, ctx->width);
    const int winHeight = std::max(1, ctx->height);

    const TexWindowMapping map = ComputeTexWindowMapping(sliceWidth, sliceHeight, winWidth, winHeight, aprCtx->zoomFactor, crossTexX, crossTexY);
    if (!map.valid) return false;

    // Mouse (pixels, top-left origin) -> NDC
    float ndcX = static_cast<float>((mouseX / winWidth) * 2.0 - 1.0);
    float ndcY = static_cast<float>(1.0 - (mouseY / winHeight) * 2.0);

    // NDC -> texture UV -> image XY
    float u = map.texLeft + ((ndcX - map.baseLeft) / (map.baseRight - map.baseLeft)) * (map.texRight - map.texLeft);
    float v = map.texBottom + ((ndcY - map.baseBottom) / (map.baseTop - map.baseBottom)) * (map.texTop - map.texBottom);
    u = std::max(0.0f, std::min(1.0f, u));
    v = std::max(0.0f, std::min(1.0f, v));
    float imgX = u * (sliceWidth - 1);
    float imgY = (1.0f - v) * (sliceHeight - 1);

    float boxLeft = 0.0f, boxRight = 0.0f, boxTop = 0.0f, boxBottom = 0.0f;
    if (aprCtx->sliceDirection == 0) {
        boxLeft = g_aprCropBox.xStart;
        boxRight = g_aprCropBox.xEnd;
        boxTop = g_aprCropBox.yStart;
        boxBottom = g_aprCropBox.yEnd;
    } else if (aprCtx->sliceDirection == 1) {
        boxLeft = g_aprCropBox.xStart;
        boxRight = g_aprCropBox.xEnd;
        boxTop = g_aprCropBox.zStart;
        boxBottom = g_aprCropBox.zEnd;
    } else {
        boxLeft = g_aprCropBox.yStart;
        boxRight = g_aprCropBox.yEnd;
        boxTop = g_aprCropBox.zStart;
        boxBottom = g_aprCropBox.zEnd;
    }

    // Threshold is defined in image pixels.
    float threshold = 10.0f / std::max(1e-6f, aprCtx->zoomFactor);

    g_aprCropBox.dragCorner = -1;
    g_aprCropBox.dragEdge = -1;
    g_aprCropBox.isDraggingBox = false;

    if (std::abs(imgX - boxLeft) < threshold && std::abs(imgY - boxTop) < threshold) {
        g_aprCropBox.dragCorner = 0;
    } else if (std::abs(imgX - boxRight) < threshold && std::abs(imgY - boxTop) < threshold) {
        g_aprCropBox.dragCorner = 1;
    } else if (std::abs(imgX - boxLeft) < threshold && std::abs(imgY - boxBottom) < threshold) {
        g_aprCropBox.dragCorner = 2;
    } else if (std::abs(imgX - boxRight) < threshold && std::abs(imgY - boxBottom) < threshold) {
        g_aprCropBox.dragCorner = 3;
    } else {
        float midX = (boxLeft + boxRight) * 0.5f;
        float midY = (boxTop + boxBottom) * 0.5f;

        if (std::abs(imgX - midX) < threshold && std::abs(imgY - boxTop) < threshold) {
            g_aprCropBox.dragEdge = 0;
        } else if (std::abs(imgX - boxRight) < threshold && std::abs(imgY - midY) < threshold) {
            g_aprCropBox.dragEdge = 1;
        } else if (std::abs(imgX - midX) < threshold && std::abs(imgY - boxBottom) < threshold) {
            g_aprCropBox.dragEdge = 2;
        } else if (std::abs(imgX - boxLeft) < threshold && std::abs(imgY - midY) < threshold) {
            g_aprCropBox.dragEdge = 3;
        } else if (imgX >= boxLeft && imgX <= boxRight && imgY >= boxTop && imgY <= boxBottom) {
            // 锟斤拷锟斤拷诓锟斤拷锌锟斤拷诓锟?-> 平锟斤拷锟斤拷锟斤拷锟斤拷锟叫匡拷
            g_aprCropBox.isDraggingBox = true;
            g_aprCropBox.dragStartMouseX = mouseX;
            g_aprCropBox.dragStartMouseY = mouseY;
            g_aprCropBox.dragStartXStart = g_aprCropBox.xStart;
            g_aprCropBox.dragStartXEnd = g_aprCropBox.xEnd;
            g_aprCropBox.dragStartYStart = g_aprCropBox.yStart;
            g_aprCropBox.dragStartYEnd = g_aprCropBox.yEnd;
            g_aprCropBox.dragStartZStart = g_aprCropBox.zStart;
            g_aprCropBox.dragStartZEnd = g_aprCropBox.zEnd;
        }
    }

    if (g_aprCropBox.dragCorner == -1 && g_aprCropBox.dragEdge == -1 && !g_aprCropBox.isDraggingBox) {
        return false;
    }

    g_aprCropBox.dragDirection = aprCtx->sliceDirection;
    g_aprCropBox.isDragging = true;
    return true;
}

static void Win32_HandleCropBoxMouseMove(WindowContext* ctx, double mouseX, double mouseY) {
    // Accept rendererType 0 (APR) and 2 (APR offscreen)
    const bool isAPR = ctx && (ctx->rendererType == 0 || ctx->rendererType == 2);
    
    if (!ctx || !g_aprCropBox.isDragging || !ctx->cropBoxVisible || !isAPR || !ctx->boundRenderer) {
        return;
    }

    auto aprCtx = static_cast<APRContext*>(ctx->boundRenderer);
    if (!aprCtx || !aprCtx->volume) {
        return;
    }

    int width = 0, height = 0, depth = 0;
    if (Dicom_Volume_GetDimensions(aprCtx->volume, &width, &height, &depth) != NATIVE_OK || width <= 0 || height <= 0 || depth <= 0) {
        return;
    }

    int sliceWidth = 0, sliceHeight = 0;
    float crossTexX = 0.5f, crossTexY = 0.5f;
    if (g_aprCropBox.dragDirection == 0) {
        sliceWidth = width;
        sliceHeight = height;
        crossTexX = aprCtx->crosshairU / std::max(1, (width - 1));
        crossTexY = 1.0f - (aprCtx->crosshairV / std::max(1, (height - 1)));
    } else if (g_aprCropBox.dragDirection == 1) {
        sliceWidth = width;
        sliceHeight = depth;
        crossTexX = aprCtx->crosshairU / std::max(1, (width - 1));
        crossTexY = 1.0f - (aprCtx->crosshairV / std::max(1, (depth - 1)));
    } else {
        sliceWidth = height;
        sliceHeight = depth;
        crossTexX = aprCtx->crosshairU / std::max(1, (height - 1));
        crossTexY = 1.0f - (aprCtx->crosshairV / std::max(1, (depth - 1)));
    }

    const int winWidth = std::max(1, ctx->width);
    const int winHeight = std::max(1, ctx->height);
    const TexWindowMapping map = ComputeTexWindowMapping(sliceWidth, sliceHeight, winWidth, winHeight, aprCtx->zoomFactor, crossTexX, crossTexY);
    if (!map.valid) return;

    float ndcX = static_cast<float>((mouseX / winWidth) * 2.0 - 1.0);
    float ndcY = static_cast<float>(1.0 - (mouseY / winHeight) * 2.0);

    float u = map.texLeft + ((ndcX - map.baseLeft) / (map.baseRight - map.baseLeft)) * (map.texRight - map.texLeft);
    float v = map.texBottom + ((ndcY - map.baseBottom) / (map.baseTop - map.baseBottom)) * (map.texTop - map.texBottom);
    u = std::max(0.0f, std::min(1.0f, u));
    v = std::max(0.0f, std::min(1.0f, v));
    float imgX = u * (sliceWidth - 1);
    float imgY = (1.0f - v) * (sliceHeight - 1);

    if (g_aprCropBox.isDraggingBox) {
        // Delta in image pixels: compute from current image coords vs stored start mouse coords.
        // Use the same mapping to convert stored start mouse into start image coords.
        float startNdcX = static_cast<float>((g_aprCropBox.dragStartMouseX / winWidth) * 2.0 - 1.0);
        float startNdcY = static_cast<float>(1.0 - (g_aprCropBox.dragStartMouseY / winHeight) * 2.0);
        float startU = map.texLeft + ((startNdcX - map.baseLeft) / (map.baseRight - map.baseLeft)) * (map.texRight - map.texLeft);
        float startV = map.texBottom + ((startNdcY - map.baseBottom) / (map.baseTop - map.baseBottom)) * (map.texTop - map.texBottom);
        startU = std::max(0.0f, std::min(1.0f, startU));
        startV = std::max(0.0f, std::min(1.0f, startV));
        float startImgX = startU * (sliceWidth - 1);
        float startImgY = (1.0f - startV) * (sliceHeight - 1);

        float deltaX = imgX - startImgX;
        float deltaY = imgY - startImgY;

        if (g_aprCropBox.dragDirection == 0) {
            g_aprCropBox.xStart = g_aprCropBox.dragStartXStart + deltaX;
            g_aprCropBox.xEnd = g_aprCropBox.dragStartXEnd + deltaX;
            g_aprCropBox.yStart = g_aprCropBox.dragStartYStart + deltaY;
            g_aprCropBox.yEnd = g_aprCropBox.dragStartYEnd + deltaY;
        } else if (g_aprCropBox.dragDirection == 1) {
            g_aprCropBox.xStart = g_aprCropBox.dragStartXStart + deltaX;
            g_aprCropBox.xEnd = g_aprCropBox.dragStartXEnd + deltaX;
            g_aprCropBox.zStart = g_aprCropBox.dragStartZStart + deltaY;
            g_aprCropBox.zEnd = g_aprCropBox.dragStartZEnd + deltaY;
        } else {
            g_aprCropBox.yStart = g_aprCropBox.dragStartYStart + deltaX;
            g_aprCropBox.yEnd = g_aprCropBox.dragStartYEnd + deltaX;
            g_aprCropBox.zStart = g_aprCropBox.dragStartZStart + deltaY;
            g_aprCropBox.zEnd = g_aprCropBox.dragStartZEnd + deltaY;
        }
    } else if (g_aprCropBox.dragCorner >= 0 || g_aprCropBox.dragEdge >= 0) {
        // 获取当前中心和半径
        float centerX = (g_aprCropBox.xStart + g_aprCropBox.xEnd) * 0.5f;
        float centerY = (g_aprCropBox.yStart + g_aprCropBox.yEnd) * 0.5f;
        float centerZ = (g_aprCropBox.zStart + g_aprCropBox.zEnd) * 0.5f;
        float radiusX = (g_aprCropBox.xEnd - g_aprCropBox.xStart) * 0.5f;
        float radiusY = (g_aprCropBox.yEnd - g_aprCropBox.yStart) * 0.5f;
        float radiusZ = (g_aprCropBox.zEnd - g_aprCropBox.zStart) * 0.5f;
        
        if (g_aprCropBox.shape == CROP_SHAPE_SPHERE) {
            // 球体：拖拽任意点时保持球形，均匀缩放所有轴
            float newRadius = 0.0f;
            if (g_aprCropBox.dragDirection == 0) {
                // Axial视图 (XY平面)
                float dx = std::abs(imgX - centerX);
                float dy = std::abs(imgY - centerY);
                newRadius = std::max(dx, dy);
            } else if (g_aprCropBox.dragDirection == 1) {
                // Coronal视图 (XZ平面)
                float dx = std::abs(imgX - centerX);
                float dz = std::abs(imgY - centerZ);
                newRadius = std::max(dx, dz);
            } else {
                // Sagittal视图 (YZ平面)
                float dy = std::abs(imgX - centerY);
                float dz = std::abs(imgY - centerZ);
                newRadius = std::max(dy, dz);
            }
            newRadius = std::max(5.0f, newRadius);
            // 更新所有三个轴的范围
            g_aprCropBox.xStart = centerX - newRadius;
            g_aprCropBox.xEnd = centerX + newRadius;
            g_aprCropBox.yStart = centerY - newRadius;
            g_aprCropBox.yEnd = centerY + newRadius;
            g_aprCropBox.zStart = centerZ - newRadius;
            g_aprCropBox.zEnd = centerZ + newRadius;
        } else if (g_aprCropBox.shape == CROP_SHAPE_CYLINDER) {
            // 圆柱体：保持圆形截面，高度可独立调整
            int cylDir = static_cast<int>(g_aprCropBox.cylinderDirection);
            
            if (cylDir == 0) {
                // 轴向圆柱（沿Z轴）- XY是圆形截面
                if (g_aprCropBox.dragDirection == 0) {
                    // Axial视图：拖拽调整XY半径（保持圆形）
                    float dx = std::abs(imgX - centerX);
                    float dy = std::abs(imgY - centerY);
                    float newRadius = std::max(5.0f, std::max(dx, dy));
                    g_aprCropBox.xStart = centerX - newRadius;
                    g_aprCropBox.xEnd = centerX + newRadius;
                    g_aprCropBox.yStart = centerY - newRadius;
                    g_aprCropBox.yEnd = centerY + newRadius;
                } else if (g_aprCropBox.dragDirection == 1) {
                    // Coronal视图：拖拽X边调整半径，拖拽Z边调整高度
                    if (g_aprCropBox.dragEdge == 1 || g_aprCropBox.dragEdge == 3) {
                        float newRadius = std::max(5.0f, std::abs(imgX - centerX));
                        g_aprCropBox.xStart = centerX - newRadius;
                        g_aprCropBox.xEnd = centerX + newRadius;
                        g_aprCropBox.yStart = centerY - newRadius;
                        g_aprCropBox.yEnd = centerY + newRadius;
                    } else {
                        if (g_aprCropBox.dragEdge == 0 || g_aprCropBox.dragCorner == 0 || g_aprCropBox.dragCorner == 1) g_aprCropBox.zStart = imgY;
                        else g_aprCropBox.zEnd = imgY;
                    }
                } else {
                    // Sagittal视图：拖拽Y边调整半径，拖拽Z边调整高度
                    if (g_aprCropBox.dragEdge == 1 || g_aprCropBox.dragEdge == 3) {
                        float newRadius = std::max(5.0f, std::abs(imgX - centerY));
                        g_aprCropBox.xStart = centerX - newRadius;
                        g_aprCropBox.xEnd = centerX + newRadius;
                        g_aprCropBox.yStart = centerY - newRadius;
                        g_aprCropBox.yEnd = centerY + newRadius;
                    } else {
                        if (g_aprCropBox.dragEdge == 0 || g_aprCropBox.dragCorner == 0 || g_aprCropBox.dragCorner == 1) g_aprCropBox.zStart = imgY;
                        else g_aprCropBox.zEnd = imgY;
                    }
                }
            } else if (cylDir == 1) {
                // 冠状圆柱（沿Y轴）- XZ是圆形截面
                if (g_aprCropBox.dragDirection == 1) {
                    // Coronal视图：拖拽调整XZ半径（保持圆形）
                    float dx = std::abs(imgX - centerX);
                    float dz = std::abs(imgY - centerZ);
                    float newRadius = std::max(5.0f, std::max(dx, dz));
                    g_aprCropBox.xStart = centerX - newRadius;
                    g_aprCropBox.xEnd = centerX + newRadius;
                    g_aprCropBox.zStart = centerZ - newRadius;
                    g_aprCropBox.zEnd = centerZ + newRadius;
                } else if (g_aprCropBox.dragDirection == 0) {
                    // Axial视图：拖拽X边调整半径，拖拽Y边调整高度
                    if (g_aprCropBox.dragEdge == 1 || g_aprCropBox.dragEdge == 3) {
                        float newRadius = std::max(5.0f, std::abs(imgX - centerX));
                        g_aprCropBox.xStart = centerX - newRadius;
                        g_aprCropBox.xEnd = centerX + newRadius;
                        g_aprCropBox.zStart = centerZ - newRadius;
                        g_aprCropBox.zEnd = centerZ + newRadius;
                    } else {
                        if (g_aprCropBox.dragEdge == 0 || g_aprCropBox.dragCorner == 0 || g_aprCropBox.dragCorner == 1) g_aprCropBox.yStart = imgY;
                        else g_aprCropBox.yEnd = imgY;
                    }
                } else {
                    // Sagittal视图：拖拽Z边调整半径，拖拽Y边调整高度
                    if (g_aprCropBox.dragEdge == 0 || g_aprCropBox.dragEdge == 2) {
                        float newRadius = std::max(5.0f, std::abs(imgY - centerZ));
                        g_aprCropBox.xStart = centerX - newRadius;
                        g_aprCropBox.xEnd = centerX + newRadius;
                        g_aprCropBox.zStart = centerZ - newRadius;
                        g_aprCropBox.zEnd = centerZ + newRadius;
                    } else {
                        if (g_aprCropBox.dragEdge == 1 || g_aprCropBox.dragCorner == 1 || g_aprCropBox.dragCorner == 3) g_aprCropBox.yEnd = imgX;
                        else g_aprCropBox.yStart = imgX;
                    }
                }
            } else {
                // 矢状圆柱（沿X轴）- YZ是圆形截面
                if (g_aprCropBox.dragDirection == 2) {
                    // Sagittal视图：拖拽调整YZ半径（保持圆形）
                    float dy = std::abs(imgX - centerY);
                    float dz = std::abs(imgY - centerZ);
                    float newRadius = std::max(5.0f, std::max(dy, dz));
                    g_aprCropBox.yStart = centerY - newRadius;
                    g_aprCropBox.yEnd = centerY + newRadius;
                    g_aprCropBox.zStart = centerZ - newRadius;
                    g_aprCropBox.zEnd = centerZ + newRadius;
                } else if (g_aprCropBox.dragDirection == 0) {
                    // Axial视图：拖拽Y边调整半径，拖拽X边调整高度
                    if (g_aprCropBox.dragEdge == 0 || g_aprCropBox.dragEdge == 2) {
                        float newRadius = std::max(5.0f, std::abs(imgY - centerY));
                        g_aprCropBox.yStart = centerY - newRadius;
                        g_aprCropBox.yEnd = centerY + newRadius;
                        g_aprCropBox.zStart = centerZ - newRadius;
                        g_aprCropBox.zEnd = centerZ + newRadius;
                    } else {
                        if (g_aprCropBox.dragEdge == 1 || g_aprCropBox.dragCorner == 1 || g_aprCropBox.dragCorner == 3) g_aprCropBox.xEnd = imgX;
                        else g_aprCropBox.xStart = imgX;
                    }
                } else {
                    // Coronal视图：拖拽Z边调整半径，拖拽X边调整高度
                    if (g_aprCropBox.dragEdge == 0 || g_aprCropBox.dragEdge == 2) {
                        float newRadius = std::max(5.0f, std::abs(imgY - centerZ));
                        g_aprCropBox.yStart = centerY - newRadius;
                        g_aprCropBox.yEnd = centerY + newRadius;
                        g_aprCropBox.zStart = centerZ - newRadius;
                        g_aprCropBox.zEnd = centerZ + newRadius;
                    } else {
                        if (g_aprCropBox.dragEdge == 1 || g_aprCropBox.dragCorner == 1 || g_aprCropBox.dragCorner == 3) g_aprCropBox.xEnd = imgX;
                        else g_aprCropBox.xStart = imgX;
                    }
                }
            }
        } else {
            // 立方体：原有逻辑，独立调整各轴
            if (g_aprCropBox.dragCorner >= 0) {
                if (g_aprCropBox.dragDirection == 0) {
                    if (g_aprCropBox.dragCorner == 0) { g_aprCropBox.xStart = imgX; g_aprCropBox.yStart = imgY; }
                    else if (g_aprCropBox.dragCorner == 1) { g_aprCropBox.xEnd = imgX; g_aprCropBox.yStart = imgY; }
                    else if (g_aprCropBox.dragCorner == 2) { g_aprCropBox.xStart = imgX; g_aprCropBox.yEnd = imgY; }
                    else if (g_aprCropBox.dragCorner == 3) { g_aprCropBox.xEnd = imgX; g_aprCropBox.yEnd = imgY; }
                } else if (g_aprCropBox.dragDirection == 1) {
                    if (g_aprCropBox.dragCorner == 0) { g_aprCropBox.xStart = imgX; g_aprCropBox.zStart = imgY; }
                    else if (g_aprCropBox.dragCorner == 1) { g_aprCropBox.xEnd = imgX; g_aprCropBox.zStart = imgY; }
                    else if (g_aprCropBox.dragCorner == 2) { g_aprCropBox.xStart = imgX; g_aprCropBox.zEnd = imgY; }
                    else if (g_aprCropBox.dragCorner == 3) { g_aprCropBox.xEnd = imgX; g_aprCropBox.zEnd = imgY; }
                } else {
                    if (g_aprCropBox.dragCorner == 0) { g_aprCropBox.yStart = imgX; g_aprCropBox.zStart = imgY; }
                    else if (g_aprCropBox.dragCorner == 1) { g_aprCropBox.yEnd = imgX; g_aprCropBox.zStart = imgY; }
                    else if (g_aprCropBox.dragCorner == 2) { g_aprCropBox.yStart = imgX; g_aprCropBox.zEnd = imgY; }
                    else if (g_aprCropBox.dragCorner == 3) { g_aprCropBox.yEnd = imgX; g_aprCropBox.zEnd = imgY; }
                }
            } else if (g_aprCropBox.dragEdge >= 0) {
                if (g_aprCropBox.dragDirection == 0) {
                    if (g_aprCropBox.dragEdge == 0) g_aprCropBox.yStart = imgY;
                    else if (g_aprCropBox.dragEdge == 1) g_aprCropBox.xEnd = imgX;
                    else if (g_aprCropBox.dragEdge == 2) g_aprCropBox.yEnd = imgY;
                    else if (g_aprCropBox.dragEdge == 3) g_aprCropBox.xStart = imgX;
                } else if (g_aprCropBox.dragDirection == 1) {
                    if (g_aprCropBox.dragEdge == 0) g_aprCropBox.zStart = imgY;
                    else if (g_aprCropBox.dragEdge == 1) g_aprCropBox.xEnd = imgX;
                    else if (g_aprCropBox.dragEdge == 2) g_aprCropBox.zEnd = imgY;
                    else if (g_aprCropBox.dragEdge == 3) g_aprCropBox.xStart = imgX;
                } else {
                    if (g_aprCropBox.dragEdge == 0) g_aprCropBox.zStart = imgY;
                    else if (g_aprCropBox.dragEdge == 1) g_aprCropBox.yEnd = imgX;
                    else if (g_aprCropBox.dragEdge == 2) g_aprCropBox.zEnd = imgY;
                    else if (g_aprCropBox.dragEdge == 3) g_aprCropBox.yStart = imgX;
                }
            }
        }
    }

    if (g_aprCropBox.xStart > g_aprCropBox.xEnd) std::swap(g_aprCropBox.xStart, g_aprCropBox.xEnd);
    if (g_aprCropBox.yStart > g_aprCropBox.yEnd) std::swap(g_aprCropBox.yStart, g_aprCropBox.yEnd);
    if (g_aprCropBox.zStart > g_aprCropBox.zEnd) std::swap(g_aprCropBox.zStart, g_aprCropBox.zEnd);

    g_aprCropBox.xStart = std::max(0.0f, std::min(static_cast<float>(width - 1), g_aprCropBox.xStart));
    g_aprCropBox.xEnd = std::max(0.0f, std::min(static_cast<float>(width - 1), g_aprCropBox.xEnd));
    g_aprCropBox.yStart = std::max(0.0f, std::min(static_cast<float>(height - 1), g_aprCropBox.yStart));
    g_aprCropBox.yEnd = std::max(0.0f, std::min(static_cast<float>(height - 1), g_aprCropBox.yEnd));
    g_aprCropBox.zStart = std::max(0.0f, std::min(static_cast<float>(depth - 1), g_aprCropBox.zStart));
    g_aprCropBox.zEnd = std::max(0.0f, std::min(static_cast<float>(depth - 1), g_aprCropBox.zEnd));
}

static float Win32_AngleDeg(double x, double y, double cx, double cy) {
    const double dx = x - cx;
    const double dy = y - cy;
    // Screen coordinates: +y is down; use atan2(dy, dx) consistently.
    const double angleRad = std::atan2(dy, dx);
    return static_cast<float>(angleRad * (180.0 / 3.14159265358979323846));
}

// Win32锟斤拷息锟斤拷锟斤拷锟斤拷锟斤拷
static LRESULT CALLBACK Win32WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    WindowContext* ctx = reinterpret_cast<WindowContext*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    
    switch (msg) {
    case WM_CLOSE:
        // Embedded HWNDs must not close themselves.
        if (ctx && ctx->allowClose) {
            DestroyWindow(hwnd);
        }
        return 0;
        
    case WM_PAINT:
        // 锟斤拷榇帮拷锟斤拷欠锟斤拷锟斤拷兀锟斤拷锟斤拷氐拇锟斤拷锟斤拷锟斤拷锟斤拷锟饺?
        if (ctx && ctx->isHidden) {
            ValidateRect(hwnd, nullptr);
            ctx->needsRedraw.store(false);  // 锟斤拷锟斤拷锟截伙拷锟街?
            return 0;
        }
        
        // 锟叫伙拷锟斤拷锟斤拷前锟斤拷锟节碉拷OpenGL锟斤拷锟斤拷锟斤拷
        if (ctx && ctx->hglrc) {
            HDC hdc = ctx->hdc ? ctx->hdc : GetDC(hwnd);

            if (wglMakeCurrent(hdc, ctx->hglrc)) {
                glViewport(0, 0, ctx->width, ctx->height);
                if (ctx->boundRenderer) {
                    if (ctx->rendererType == 0 || ctx->rendererType == 2) {
                        // Crop box is a legacy global; gate visibility per-window so
                        // unrelated windows (e.g., ROI editor) don't show it.
                        const bool prevCropEnabled = g_aprCropBox.enabled;
                        g_aprCropBox.enabled = prevCropEnabled && ctx->cropBoxVisible;

                        APR_Render(static_cast<APRHandle>(ctx->boundRenderer));

                        g_aprCropBox.enabled = prevCropEnabled;
                    }
                    else if (ctx->rendererType == 1)
                        MPR_Render(static_cast<MPRHandle>(ctx->boundRenderer));
                }
                else if (ctx->is3DView && ctx->aprAxial) {
                    // Split into independent renderer classes/files.
                    if (ctx->threeDRendererKind == 2) {
                        // ROI editor 3D window: temporarily disable rendering.
                        // Keep the window black by clearing and swapping.
                        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    } else if (ctx->threeDRendererKind == 3) {
                        ReconstructionRaycast3DRenderer::Render(ctx);
                    } else {
                        ImageBrowserOrthogonal3DRenderer::Render(
                            static_cast<APRHandle>(ctx->aprAxial),
                            static_cast<APRHandle>(ctx->aprCoronal),
                            static_cast<APRHandle>(ctx->aprSagittal),
                            ctx->width,
                            ctx->height,
                            ctx->viewRotMat,
                            ctx->viewRotMatInitialized,
                            ctx->viewZoom,
                            ctx->viewPanX,
                            ctx->viewPanY,
                            ctx->cropBoxVisible
                        );
                    }
                }
                SwapBuffers(hdc);
                wglMakeCurrent(nullptr, nullptr);
            }

            if (!ctx->hdc) {  // 锟斤拷时锟斤拷玫锟揭拷头锟?
                ReleaseDC(hwnd, hdc);
            }

            ValidateRect(hwnd, nullptr);
            ctx->needsRedraw.store(false);  // 锟斤拷染锟斤拷桑锟斤拷锟斤拷帽锟街撅拷锟斤拷锟斤拷锟斤拷锟揭伙拷锟絀nvalidateRect
            return 0;
        }
        break;

    case WM_DESTROY:
        if (ctx && ctx->allowClose) {
            PostQuitMessage(0);
        }
        return 0;

    case WM_ERASEBKGND:
        // 杩斿洖1鍛婅瘔绯荤粺涓嶉渶瑕佹摝闄よ儗鏅紝鐢監penGL瀹屽叏鎺ョ缁樺埗
        // 杩欏彲浠ラ槻姝esize/maximize鏃剁殑榛戣壊/鐧借壊闂儊
        return 1;
        
    case WM_SIZE:
        if (ctx) {
            int newWidth = LOWORD(lParam);
            int newHeight = HIWORD(lParam);
            
            // 鍙洿鏂板昂瀵革紝涓嶅湪杩欓噷鍚屾璋冪敤 wglMakeCurrent
            // viewport 鏇存柊寤惰繜鍒?WM_PAINT锛岄伩鍏?resize 鏈熼棿鐨?GPU stall
            ctx->width = newWidth;
            ctx->height = newHeight;
            
            // 闅愯棌鐨勭獥鍙ｄ笉闇€瑕佽Е鍙戦噸缁?
            if (!ctx->isHidden) {
                InvalidateRect(hwnd, nullptr, FALSE);
            }
        }
        return 0;
        
    case WM_LBUTTONDOWN:
        if (ctx) {
            double xpos = static_cast<double>(GET_X_LPARAM(lParam));
            double ypos = static_cast<double>(GET_Y_LPARAM(lParam));

            g_shiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

            // 3D view: LMB drag = rotate (arcball), Shift+LMB = pan.
            // (Previously only RMB drag rotated, which feels like "mouse not working" for many users.)
            if (ctx->is3DView && !ctx->boundRenderer) {
                ctx->isDragging = true;
                ctx->is3DRotating = !g_shiftPressed;
                ctx->is3DPanning = g_shiftPressed;
                ctx->arcballActive = false;

                if (!ctx->viewRotMatInitialized) {
                    Mat4_Identity(ctx->viewRotMat);
                    ctx->viewRotMatInitialized = true;
                }

                if (!g_shiftPressed) {
                    ctx->arcballActive = true;
                    Win32_ArcballVec(ctx->width, ctx->height, xpos, ypos, ctx->arcballLast);
                }

                ctx->lastMouseX = xpos;
                ctx->lastMouseY = ypos;
                SetCapture(hwnd);
                return 0;
            }

            // 锟斤拷锟饺硷拷1: 锟斤拷锟斤拷锟斤拷锟斤拷
            if (g_currentToolType >= 1 && g_currentToolType <= 6) {
                if (Win32_HandleMeasurementMouseDown(ctx, xpos, ypos)) {
                    ctx->lastMouseX = xpos;
                    ctx->lastMouseY = ypos;
                    SetCapture(hwnd);
                    return 0;
                }
            }

            // 锟斤拷锟饺硷拷2: APR锟斤拷锟叫匡拷
            if (g_aprCropBox.enabled && ctx->cropBoxVisible && g_currentToolType == 0) {
                if (Win32_HandleCropBoxMouseDown(ctx, xpos, ypos, g_shiftPressed)) {
                    ctx->lastMouseX = xpos;
                    ctx->lastMouseY = ypos;
                    SetCapture(hwnd);
                    return 0;
                }
            }

            // 锟斤拷锟饺硷拷3: 默锟斤拷锟较讹拷锟斤拷锟狡讹拷锟斤拷位锟竭ｏ拷
            if (g_currentToolType == 0) {
                ctx->isDragging = true;
                ctx->lastMouseX = xpos;
                ctx->lastMouseY = ypos;

                // APR: Shift+drag rotates in-plane (Z) like legacy bone module.
                if (g_shiftPressed && ctx->boundRenderer && (ctx->rendererType == 0 || ctx->rendererType == 2)) {
                    ctx->isShiftRotateZ = true;
                    const double cx = (ctx->width > 0) ? (ctx->width * 0.5) : 0.0;
                    const double cy = (ctx->height > 0) ? (ctx->height * 0.5) : 0.0;
                    ctx->shiftRotateLastAngleDeg = Win32_AngleDeg(xpos, ypos, cx, cy);
                } else {
                    ctx->isShiftRotateZ = false;
                }

                SetCapture(hwnd);
            }
        }
        return 0;
        
    case WM_LBUTTONUP:
        if (ctx) {
            double xpos = static_cast<double>(GET_X_LPARAM(lParam));
            double ypos = static_cast<double>(GET_Y_LPARAM(lParam));

            // 3D view: end LMB interaction.
            if (ctx->is3DView && !ctx->boundRenderer) {
                ctx->isDragging = false;
                ctx->is3DRotating = false;
                ctx->is3DPanning = false;
                ctx->arcballActive = false;
                ReleaseCapture();
                return 0;
            }

            if (g_aprCropBox.isDragging) {
                g_aprCropBox.isDragging = false;
                g_aprCropBox.isDraggingBox = false;
                g_aprCropBox.dragCorner = -1;
                g_aprCropBox.dragEdge = -1;
            }

            if (g_currentToolType >= 1 && g_currentToolType <= 6) {
                Win32_HandleMeasurementMouseUp(ctx, xpos, ypos);
            }

            ctx->isDragging = false;
            ctx->isShiftRotateZ = false;
            ReleaseCapture();
        }
        return 0;
        
    case WM_RBUTTONDOWN:
        if (ctx) {
            double xpos = static_cast<double>(GET_X_LPARAM(lParam));
            double ypos = static_cast<double>(GET_Y_LPARAM(lParam));

            bool shiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

            // APR/MPR: RMB drag = window/level
            if (ctx->boundRenderer) {
                ctx->isRightDragging = true;
                ctx->isWindowing = true;
                ctx->windowingStartX = xpos;
                ctx->windowingStartY = ypos;
                ctx->lastMouseX = xpos;
                ctx->lastMouseY = ypos;

                float ww = 400.0f;
                float wl = 40.0f;
                if (ctx->rendererType == 0 || ctx->rendererType == 2) {
                    auto aprCtx = static_cast<APRContext*>(ctx->boundRenderer);
                    ww = aprCtx->windowWidthHU;
                    wl = aprCtx->windowLevelHU;
                } else if (ctx->rendererType == 1) {
                    auto mprCtx = static_cast<MPRContext*>(ctx->boundRenderer);
                    ww = mprCtx->windowWidthHU;
                    wl = mprCtx->windowLevelHU;
                }
                ctx->windowingStartWW = ww;
                ctx->windowingStartWL = wl;

                SetCapture(hwnd);
                return 0;
            }

            // Only 3D view: RMB rotate / Shift+RMB pan
            if (!ctx->boundRenderer) {
                if (ctx->is3DView) {
                    SetFocus(hwnd);
                }
                g_3dRotating = true;
                g_3dLastMouseX = xpos;
                g_3dLastMouseY = ypos;
                ctx->isRightDragging = true;
                ctx->is3DRotating = true;
                ctx->is3DPanning = false;

                // Arcball is used for 3D rotation (non-shift). Shift+RMB is panning.
                if (ctx->is3DView && !shiftPressed) {
                    if (!ctx->viewRotMatInitialized) {
                        Mat4_Identity(ctx->viewRotMat);
                        ctx->viewRotMatInitialized = true;
                    }
                    ctx->arcballActive = true;
                    Win32_ArcballVec(ctx->width, ctx->height, xpos, ypos, ctx->arcballLast);
                } else {
                    ctx->arcballActive = false;
                }
                ctx->lastMouseX = xpos;
                ctx->lastMouseY = ypos;
                SetCapture(hwnd);
            }
        }
        return 0;
        
    case WM_RBUTTONUP:
        if (ctx) {
            // APR/MPR
            if (ctx->boundRenderer) {
                if (ctx->isRightDragging) {
                    ctx->isRightDragging = false;
                    ctx->isWindowing = false;
                    ReleaseCapture();
                }
                return 0;
            }

            // Only 3D view
            if (!ctx->boundRenderer) {
                g_3dRotating = false;
                if (ctx->isRightDragging) {
                    ctx->isRightDragging = false;
                    ctx->is3DRotating = false;
                    ctx->is3DPanning = false;
                    ctx->arcballActive = false;
                    ReleaseCapture();
                }
            }
        }
        return 0;

    case WM_MBUTTONDOWN:
        if (ctx) {
            double xpos = static_cast<double>(GET_X_LPARAM(lParam));
            double ypos = static_cast<double>(GET_Y_LPARAM(lParam));

            if (ctx->is3DView) {
                SetFocus(hwnd);
                ctx->isMiddleDragging = true;
                ctx->is3DPanning = true;
                ctx->lastMouseX = xpos;
                ctx->lastMouseY = ypos;
                SetCapture(hwnd);
            }
        }
        return 0;

    case WM_MBUTTONUP:
        if (ctx) {
            if (ctx->is3DView && ctx->isMiddleDragging) {
                ctx->isMiddleDragging = false;
                ctx->is3DPanning = false;
                ReleaseCapture();
            }
        }
        return 0;
        
    case WM_MOUSEMOVE:
        if (ctx) {
            double xpos = static_cast<double>(GET_X_LPARAM(lParam));
            double ypos = static_cast<double>(GET_Y_LPARAM(lParam));
            
            double dx = xpos - ctx->lastMouseX;
            double dy = ypos - ctx->lastMouseY;
            
            // 锟斤拷锟絊hift锟斤拷状态
            bool shiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

            g_shiftPressed = shiftPressed;
            
            // ==================== 3D锟斤拷锟斤拷锟斤拷锟解处锟斤拷 ====================
            if (ctx->is3DView) {
                // 3D锟斤拷锟节ｏ拷锟揭硷拷锟斤拷转/Shift+锟揭硷拷锟津、伙拷锟斤拷锟叫硷拷锟斤拷锟斤拷平锟斤拷
                const bool primaryDragging = (ctx->isRightDragging || ctx->isDragging);
                if (ctx->isMiddleDragging || (primaryDragging && shiftPressed)) {
                    float scale = 0.0025f / std::max(0.2f, ctx->viewZoom);
                    ctx->viewPanX += static_cast<float>(dx) * scale;
                    ctx->viewPanY -= static_cast<float>(dy) * scale;
                    ctx->is3DPanning = true;
                    // While panning, disable arcball to avoid stale state.
                    ctx->arcballActive = false;
                } else if (primaryDragging) {
                    // Arcball orbit (accumulated matrix)
                    if (!ctx->viewRotMatInitialized) {
                        Mat4_Identity(ctx->viewRotMat);
                        ctx->viewRotMatInitialized = true;
                    }

                    if (!shiftPressed) {
                        if (!ctx->arcballActive) {
                            ctx->arcballActive = true;
                            Win32_ArcballVec(ctx->width, ctx->height, ctx->lastMouseX, ctx->lastMouseY, ctx->arcballLast);
                        }

                        float vCur[3];
                        Win32_ArcballVec(ctx->width, ctx->height, xpos, ypos, vCur);

                        // axis = last x cur
                        float axis[3] = {
                            ctx->arcballLast[1] * vCur[2] - ctx->arcballLast[2] * vCur[1],
                            ctx->arcballLast[2] * vCur[0] - ctx->arcballLast[0] * vCur[2],
                            ctx->arcballLast[0] * vCur[1] - ctx->arcballLast[1] * vCur[0]
                        };
                        const float axisLen = std::sqrt(axis[0]*axis[0] + axis[1]*axis[1] + axis[2]*axis[2]);
                        float dot = ctx->arcballLast[0] * vCur[0] + ctx->arcballLast[1] * vCur[1] + ctx->arcballLast[2] * vCur[2];
                        dot = std::max(-1.0f, std::min(1.0f, dot));

                        if (axisLen > 1e-6f) {
                            const float angleRad = std::acos(dot);
                            const float angleDeg = angleRad * 180.0f / 3.14159265358979323846f;

                            float r[16];
                            Mat4_RotationAxisAngle(r, axis[0], axis[1], axis[2], angleDeg);
                            float tmp[16];
                            Mat4_Mul(tmp, r, ctx->viewRotMat);
                            std::memcpy(ctx->viewRotMat, tmp, sizeof(float) * 16);
                            Mat4_Orthonormalize3x3(ctx->viewRotMat);
                        }

                        ctx->arcballLast[0] = vCur[0];
                        ctx->arcballLast[1] = vCur[1];
                        ctx->arcballLast[2] = vCur[2];
                        ctx->is3DRotating = true;
                    }
                }
                
                ctx->lastMouseX = xpos;
                ctx->lastMouseY = ypos;
                return 0;
            }

            // 未锟斤拷锟斤拷染锟斤拷锟侥达拷锟节匡拷锟斤拷锟斤拷3D锟斤拷锟斤拷锟斤拷图
            if (!ctx->boundRenderer && g_3dRotating) {
                ctx->viewRotY += static_cast<float>(dx) * 0.5f;
                ctx->viewRotX += static_cast<float>(dy) * 0.5f;
                if (ctx->viewRotX > 89.0f) ctx->viewRotX = 89.0f;
                if (ctx->viewRotX < -89.0f) ctx->viewRotX = -89.0f;
                g_3dLastMouseX = xpos;
                g_3dLastMouseY = ypos;
                ctx->lastMouseX = xpos;
                ctx->lastMouseY = ypos;
                return 0;
            }
            
            // ==================== APR/MPR锟斤拷锟节达拷锟斤拷 ====================
            Win32_UpdateMeasurementCursor(ctx, xpos, ypos);

            // RMB window/level adjustment (APR/MPR)
            if (ctx->isRightDragging && ctx->boundRenderer && ctx->isWindowing) {
                const double ddx = xpos - ctx->windowingStartX;
                const double ddy = ypos - ctx->windowingStartY;

                float ww = ctx->windowingStartWW + static_cast<float>(ddx) * 2.0f;
                float wl = ctx->windowingStartWL - static_cast<float>(ddy) * 2.0f;
                if (ww < 1.0f) ww = 1.0f;

                if (ctx->rendererType == 0 || ctx->rendererType == 2) {
                    auto aprCtx = static_cast<APRContext*>(ctx->boundRenderer);
                    aprCtx->windowWidthHU = ww;
                    aprCtx->windowLevelHU = wl;
                } else if (ctx->rendererType == 1) {
                    auto mprCtx = static_cast<MPRContext*>(ctx->boundRenderer);
                    mprCtx->windowWidthHU = ww;
                    mprCtx->windowLevelHU = wl;
                }

                // Keep WW/WL consistent across all views in the same session.
                Win32_SyncWindowLevelToSession(ctx, ww, wl);

                ctx->lastMouseX = xpos;
                ctx->lastMouseY = ypos;
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }

            if (g_aprCropBox.isDragging && ctx->cropBoxVisible && (ctx->rendererType == 0 || ctx->rendererType == 2)) {
                Win32_HandleCropBoxMouseMove(ctx, xpos, ypos);
                ctx->lastMouseX = xpos;
                ctx->lastMouseY = ypos;
                return 0;
            }

            // 锟斤拷锟斤拷锟阶?- 只锟节癸拷锟斤拷锟斤拷锟斤拷为0时锟狡讹拷锟斤拷位锟斤拷
            if (ctx->isDragging && ctx->boundRenderer && g_currentToolType == 0) {
                if (shiftPressed && g_currentToolType == 0) {
                    // Shift+锟斤拷锟斤拷锟斤拷锟阶拷锟斤拷锟紸PR锟斤拷使锟斤拷全锟斤拷同锟斤拷锟斤拷
                    if (ctx->rendererType == 0 || ctx->rendererType == 2) {  // APR
                        APRContext* aprCtx = static_cast<APRContext*>(ctx->boundRenderer);

                        // Rotate around the current slice local normal (in world space).
                        // This intentionally causes world Euler angles to couple when the
                        // volume is already rotated.
                        if (ctx->isShiftRotateZ) {
                            const double cx = (ctx->width > 0) ? (ctx->width * 0.5) : 0.0;
                            const double cy = (ctx->height > 0) ? (ctx->height * 0.5) : 0.0;
                            float angleNow = Win32_AngleDeg(xpos, ypos, cx, cy);
                            float deltaDeg = angleNow - ctx->shiftRotateLastAngleDeg;
                            if (deltaDeg > 180.0f) deltaDeg -= 360.0f;
                            if (deltaDeg < -180.0f) deltaDeg += 360.0f;
                            ctx->shiftRotateLastAngleDeg = angleNow;

                            // Direction fix:
                            // In our current slice mappings, axial and sagittal have a flipped vertical axis
                            // relative to the expected interaction, so their rotation sign must be inverted.
                            if (aprCtx->sliceDirection == 0 || aprCtx->sliceDirection == 2) {
                                deltaDeg = -deltaDeg;
                            }

                            // Compute the slice local normal in world space.
                            float axisLocal[3] = { 0.0f, 0.0f, 0.0f };
                            if (aprCtx->sliceDirection == 0) axisLocal[2] = 1.0f;      // Axial normal = +Z
                            else if (aprCtx->sliceDirection == 1) axisLocal[1] = 1.0f; // Coronal normal = +Y
                            else axisLocal[0] = 1.0f;                                  // Sagittal normal = +X

                            float axisWorld[3] = { 0.0f, 0.0f, 0.0f };
                            Mat4_MulVec3_3x3(aprCtx->rotMat, axisLocal, axisWorld);

                            // Apply incremental world rotation: rotMat = R(axisWorld, delta) * rotMat
                            float rMove[16];
                            Mat4_RotationAxisAngle(rMove, axisWorld[0], axisWorld[1], axisWorld[2], deltaDeg);
                            float tmpRot[16];
                            Mat4_Mul(tmpRot, rMove, aprCtx->rotMat);
                            std::memcpy(aprCtx->rotMat, tmpRot, sizeof(float) * 16);
                            Mat4_Orthonormalize3x3(aprCtx->rotMat);

                            // Rotate locator intersection in the slice plane around slice center so it
                            // visually rotates together with the image.
                            int volW = 0, volH = 0, volD = 0;
                            if (aprCtx->volume &&
                                Dicom_Volume_GetDimensions(aprCtx->volume, &volW, &volH, &volD) == NATIVE_OK) {
                                APR_RotateCrosshairInSlice(aprCtx, volW, volH, volD, deltaDeg);
                            }

                            // Sync to linked APRs in the same session (per-tab state, no globals).
                            GlobalAPRCenter* aprCenter = GetSessionAPRCenter(aprCtx->sessionId);
                            std::memcpy(aprCenter->rotMat, aprCtx->rotMat, sizeof(float) * 16);
                            for (APRHandle linkedAPR : aprCenter->linkedAPRs) {
                                if (linkedAPR != aprCtx) {
                                    auto linkedCtx = static_cast<APRContext*>(linkedAPR);
                                    std::memcpy(linkedCtx->rotMat, aprCtx->rotMat, sizeof(float) * 16);

                                    int lW = 0, lH = 0, lD = 0;
                                    if (linkedCtx->volume &&
                                        Dicom_Volume_GetDimensions(linkedCtx->volume, &lW, &lH, &lD) == NATIVE_OK) {
                                        APR_RotateCrosshairInSlice(linkedCtx, lW, lH, lD, deltaDeg);
                                    }
                                }
                            }
                        }
                    }
                    // 锟斤拷锟斤拷锟斤拷刷锟铰ｏ拷锟斤拷锟斤拷染锟竭筹拷统一锟斤拷锟斤拷
                    
                } else {
                    // 锟斤拷锟斤拷锟斤拷贫锟斤拷锟轿伙拷撸锟斤拷锟斤拷锟斤拷锟斤拷牡悖?- 锟轿匡拷GLFW实锟斤拷
                    if (ctx->rendererType == 1) {  // MPR
                        MPRContext* mprCtx = static_cast<MPRContext*>(ctx->boundRenderer);
                        
                        if (mprCtx->volume) {
                            int width, height, depth;
                            if (Dicom_Volume_GetDimensions(mprCtx->volume, &width, &height, &depth) == NATIVE_OK) {
                                // 确锟斤拷锟斤拷前锟斤拷片锟斤拷实锟绞尺寸（锟斤拷锟截ｏ拷
                                int sliceWidth = 0, sliceHeight = 0;
                                if (mprCtx->sliceDirection == 0) {  // Axial (XY)
                                    sliceWidth = width;
                                    sliceHeight = height;
                                } else if (mprCtx->sliceDirection == 1) {  // Coronal (XZ)
                                    sliceWidth = width;
                                    sliceHeight = depth;
                                } else if (mprCtx->sliceDirection == 2) {  // Sagittal (YZ)
                                    sliceWidth = height;
                                    sliceHeight = depth;
                                }
                                
                                // Map mouse -> image using the same mapping used for rendering (zoom anchored at crosshair).
                                const float winW = static_cast<float>(std::max(1, ctx->width));
                                const float winH = static_cast<float>(std::max(1, ctx->height));
                                const float ndcX = (static_cast<float>(xpos) / winW) * 2.0f - 1.0f;
                                const float ndcY = 1.0f - (static_cast<float>(ypos) / winH) * 2.0f;

                                float crossTexX = 0.5f, crossTexY = 0.5f;
                                if (mprCtx->sliceDirection == 0) {
                                    crossTexX = mprCtx->centerX / std::max(1.0f, (width - 1.0f));
                                    crossTexY = 1.0f - (mprCtx->centerY / std::max(1.0f, (height - 1.0f)));
                                } else if (mprCtx->sliceDirection == 1) {
                                    crossTexX = mprCtx->centerX / std::max(1.0f, (width - 1.0f));
                                    crossTexY = 1.0f - (mprCtx->centerZ / std::max(1.0f, (depth - 1.0f)));
                                } else {
                                    crossTexX = mprCtx->centerY / std::max(1.0f, (height - 1.0f));
                                    crossTexY = 1.0f - (mprCtx->centerZ / std::max(1.0f, (depth - 1.0f)));
                                }

                                const TexWindowMapping map = ComputeTexWindowMapping(sliceWidth, sliceHeight, ctx->width, ctx->height, mprCtx->zoomFactor, crossTexX, crossTexY);
                                if (!map.valid || ndcX < map.baseLeft || ndcX > map.baseRight || ndcY < map.baseBottom || ndcY > map.baseTop) {
                                    ctx->lastMouseX = xpos;
                                    ctx->lastMouseY = ypos;
                                    return 0;
                                }

                                const float relX = (ndcX - map.baseLeft) / std::max(1e-6f, (map.baseRight - map.baseLeft));
                                const float relY = (ndcY - map.baseBottom) / std::max(1e-6f, (map.baseTop - map.baseBottom));
                                const float texU = map.texLeft + relX * (map.texRight - map.texLeft);
                                const float texV = map.texBottom + relY * (map.texTop - map.texBottom);

                                const float imgX = texU * std::max(1.0f, (sliceWidth - 1.0f));
                                const float imgY = (1.0f - texV) * std::max(1.0f, (sliceHeight - 1.0f));

                                float newCenterX = mprCtx->centerX;
                                float newCenterY = mprCtx->centerY;
                                float newCenterZ = mprCtx->centerZ;

                                if (mprCtx->sliceDirection == 0) {
                                    newCenterX = imgX;
                                    newCenterY = imgY;
                                } else if (mprCtx->sliceDirection == 1) {
                                    newCenterX = imgX;
                                    newCenterZ = imgY;
                                } else {
                                    newCenterY = imgX;
                                    newCenterZ = imgY;
                                }
                                
                                // 锟竭斤拷锟斤拷
                                if (newCenterX < 0) newCenterX = 0;
                                if (newCenterX > width - 1) newCenterX = static_cast<float>(width - 1);
                                if (newCenterY < 0) newCenterY = 0;
                                if (newCenterY > height - 1) newCenterY = static_cast<float>(height - 1);
                                if (newCenterZ < 0) newCenterZ = 0;
                                if (newCenterZ > depth - 1) newCenterZ = static_cast<float>(depth - 1);
                                
                                // 锟斤拷锟斤拷 MPR_SetCenter 锟斤拷锟斤拷同锟斤拷锟斤拷锟斤拷锟斤拷锟叫达拷锟斤拷
                                MPR_SetCenter(mprCtx, newCenterX, newCenterY, newCenterZ);
                            }
                        }
                    } else if (ctx->rendererType == 0 || ctx->rendererType == 2) {  // APR
                        // APR锟斤拷锟斤拷贫锟斤拷锟轿伙拷锟?- 锟轿匡拷GLFW实锟斤拷
                        APRContext* aprCtx = static_cast<APRContext*>(ctx->boundRenderer);
                        
                        if (aprCtx->volume) {
                            int width, height, depth;
                            if (Dicom_Volume_GetDimensions(aprCtx->volume, &width, &height, &depth) == NATIVE_OK) {
                                // 确锟斤拷锟斤拷前锟斤拷片锟斤拷实锟绞尺寸（锟斤拷锟截ｏ拷
                                int sliceWidth = 0, sliceHeight = 0;
                                if (aprCtx->sliceDirection == 0) {  // Axial (XY)
                                    sliceWidth = width;
                                    sliceHeight = height;
                                } else if (aprCtx->sliceDirection == 1) {  // Coronal (XZ)
                                    sliceWidth = width;
                                    sliceHeight = depth;
                                } else if (aprCtx->sliceDirection == 2) {  // Sagittal (YZ)
                                    sliceWidth = height;
                                    sliceHeight = depth;
                                }

                                const float winW = static_cast<float>(std::max(1, ctx->width));
                                const float winH = static_cast<float>(std::max(1, ctx->height));
                                const float ndcX = (static_cast<float>(xpos) / winW) * 2.0f - 1.0f;
                                const float ndcY = 1.0f - (static_cast<float>(ypos) / winH) * 2.0f;

                                float crossTexX = 0.5f, crossTexY = 0.5f;
                                const bool hasRotation = !Mat4_IsIdentity(aprCtx->rotMat, 1e-4f);
                                if (!hasRotation) {
                                    crossTexX = aprCtx->crosshairU / std::max(1.0f, (sliceWidth - 1.0f));
                                    crossTexY = 1.0f - (aprCtx->crosshairV / std::max(1.0f, (sliceHeight - 1.0f)));
                                } else {
                                    // Match APR_Render's rotated crosshair projection:
                                    // virtual = C + R^T(real - C)
                                    const float rotationCenterX = width / 2.0f;
                                    const float rotationCenterY = height / 2.0f;
                                    const float rotationCenterZ = depth / 2.0f;

                                    float realX = 0.0f, realY = 0.0f, realZ = 0.0f;
                                    if (aprCtx->sliceDirection == 0) {  // Axial
                                        realX = aprCtx->crosshairU;
                                        realY = aprCtx->crosshairV;
                                        realZ = aprCtx->centerZ;
                                    } else if (aprCtx->sliceDirection == 1) {  // Coronal
                                        realX = aprCtx->crosshairU;
                                        realY = aprCtx->centerY;
                                        realZ = aprCtx->crosshairV;
                                    } else {  // Sagittal
                                        realX = aprCtx->centerX;
                                        realY = aprCtx->crosshairU;
                                        realZ = aprCtx->crosshairV;
                                    }

                                    const float* rm = aprCtx->rotMat;
                                    const float r00 = rm[0],  r01 = rm[4],  r02 = rm[8];
                                    const float r10 = rm[1],  r11 = rm[5],  r12 = rm[9];
                                    const float r20 = rm[2],  r21 = rm[6],  r22 = rm[10];

                                    const float dx = realX - rotationCenterX;
                                    const float dy = realY - rotationCenterY;
                                    const float dz = realZ - rotationCenterZ;
                                    const float virtualX = rotationCenterX + (r00 * dx + r10 * dy + r20 * dz);
                                    const float virtualY = rotationCenterY + (r01 * dx + r11 * dy + r21 * dz);
                                    const float virtualZ = rotationCenterZ + (r02 * dx + r12 * dy + r22 * dz);

                                    float uPix = 0.0f, vPix = 0.0f;
                                    if (aprCtx->sliceDirection == 0) {  // Axial: (X,Y)
                                        uPix = virtualX;
                                        vPix = virtualY;
                                    } else if (aprCtx->sliceDirection == 1) {  // Coronal: (X,Z)
                                        uPix = virtualX;
                                        vPix = virtualZ;
                                    } else {  // Sagittal: (Y,Z)
                                        uPix = virtualY;
                                        vPix = virtualZ;
                                    }

                                    uPix = std::max(0.0f, std::min(static_cast<float>(sliceWidth - 1), uPix));
                                    vPix = std::max(0.0f, std::min(static_cast<float>(sliceHeight - 1), vPix));
                                    crossTexX = uPix / std::max(1.0f, (sliceWidth - 1.0f));
                                    crossTexY = 1.0f - (vPix / std::max(1.0f, (sliceHeight - 1.0f)));
                                }

                                const TexWindowMapping map = ComputeTexWindowMapping(sliceWidth, sliceHeight, ctx->width, ctx->height, aprCtx->zoomFactor, crossTexX, crossTexY);
                                if (!map.valid || ndcX < map.baseLeft || ndcX > map.baseRight || ndcY < map.baseBottom || ndcY > map.baseTop) {
                                    ctx->lastMouseX = xpos;
                                    ctx->lastMouseY = ypos;
                                    return 0;
                                }

                                const float relX = (ndcX - map.baseLeft) / std::max(1e-6f, (map.baseRight - map.baseLeft));
                                const float relY = (ndcY - map.baseBottom) / std::max(1e-6f, (map.baseTop - map.baseBottom));
                                const float texU = map.texLeft + relX * (map.texRight - map.texLeft);
                                const float texV = map.texBottom + relY * (map.texTop - map.texBottom);

                                const float imgX = texU * std::max(1.0f, (sliceWidth - 1.0f));
                                const float imgY = (1.0f - texV) * std::max(1.0f, (sliceHeight - 1.0f));

                                if (imgX >= 0 && imgX < sliceWidth && imgY >= 0 && imgY < sliceHeight) {
                                    if (!hasRotation) {
                                        if (aprCtx->sliceDirection == 0) {
                                            APR_SetCenter(aprCtx, imgX, imgY, aprCtx->centerZ);
                                        } else if (aprCtx->sliceDirection == 1) {
                                            APR_SetCenter(aprCtx, imgX, aprCtx->centerY, imgY);
                                        } else {
                                            APR_SetCenter(aprCtx, aprCtx->centerX, imgX, imgY);
                                        }
                                    } else {
                                        // 鼠标位置 (imgX, imgY) 是虚拟坐标系中的点
                                        // 渲染时: real = C + R * (virtual - C)
                                        // 所以: virtual -> real 也是用 R
                                        
                                        const float rcX = width / 2.0f;
                                        const float rcY = height / 2.0f;
                                        const float rcZ = depth / 2.0f;
                                        
                                        const float* rm = aprCtx->rotMat;
                                        // R矩阵 (column-major): rm[col*4 + row]
                                        const float r00 = rm[0],  r01 = rm[4],  r02 = rm[8];
                                        const float r10 = rm[1],  r11 = rm[5],  r12 = rm[9];
                                        const float r20 = rm[2],  r21 = rm[6],  r22 = rm[10];
                                        
                                        // 当前center是体数据坐标，先变换到虚拟坐标
                                        // 逆变换: virtual = C + R^T * (real - C)
                                        const float drx = aprCtx->centerX - rcX;
                                        const float dry = aprCtx->centerY - rcY;
                                        const float drz = aprCtx->centerZ - rcZ;
                                        // R^T: 行列互换，即 (r00,r10,r20), (r01,r11,r21), (r02,r12,r22)
                                        const float virtualCX = rcX + (r00 * drx + r10 * dry + r20 * drz);
                                        const float virtualCY = rcY + (r01 * drx + r11 * dry + r21 * drz);
                                        const float virtualCZ = rcZ + (r02 * drx + r12 * dry + r22 * drz);
                                        
                                        // 构造新的虚拟坐标：鼠标位置替换对应分量，保持当前切面的垂直分量不变
                                        float newVX, newVY, newVZ;
                                        if (aprCtx->sliceDirection == 0) {
                                            // Axial: XY平面，Z不变
                                            newVX = imgX;
                                            newVY = imgY;
                                            newVZ = virtualCZ;
                                        } else if (aprCtx->sliceDirection == 1) {
                                            // Coronal: XZ平面，Y不变
                                            newVX = imgX;
                                            newVY = virtualCY;
                                            newVZ = imgY;
                                        } else {
                                            // Sagittal: YZ平面，X不变
                                            newVX = virtualCX;
                                            newVY = imgX;
                                            newVZ = imgY;
                                        }
                                        
                                        // 新的虚拟坐标变换回体数据坐标: real = C + R * (virtual - C)
                                        const float dvx = newVX - rcX;
                                        const float dvy = newVY - rcY;
                                        const float dvz = newVZ - rcZ;
                                        // R矩阵乘法
                                        const float newRealX = rcX + (r00 * dvx + r01 * dvy + r02 * dvz);
                                        const float newRealY = rcY + (r10 * dvx + r11 * dvy + r12 * dvz);
                                        const float newRealZ = rcZ + (r20 * dvx + r21 * dvy + r22 * dvz);
                                        
                                        APR_SetCenter(aprCtx, newRealX, newRealY, newRealZ);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            // 锟揭硷拷锟斤拷拽锟斤拷锟斤拷时锟斤拷锟斤拷APR/MPR锟斤拷锟揭硷拷锟斤拷锟斤拷
            // if (ctx->isRightDragging && ctx->boundRenderer) {
            //     // APR锟斤拷转锟饺癸拷锟斤拷锟斤拷时锟斤拷锟斤拷
            // }
            
            ctx->lastMouseX = xpos;
            ctx->lastMouseY = ypos;
        }
        return 0;
        
    case WM_MOUSEWHEEL:
        if (ctx) {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            float zoomDelta = delta > 0 ? 1.1f : 0.9f;
            bool shiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            
            // ==================== 3D锟斤拷锟斤拷锟斤拷锟解处锟斤拷 ====================
            if (ctx->is3DView) {
                // 3D: Wheel zoom (Shift = fine zoom)
                const float factor = shiftPressed ? (delta > 0 ? 1.02f : 0.98f) : zoomDelta;
                ctx->viewZoom *= factor;
                ctx->viewZoom = std::max(0.1f, std::min(10.0f, ctx->viewZoom));
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
            
            // ==================== APR/MPR锟斤拷锟节达拷锟斤拷 ====================
            if (ctx->boundRenderer) {
                if (shiftPressed) {
                    // Shift+Wheel zoom
                    if (ctx->rendererType == 0 || ctx->rendererType == 2) {  // APR
                        APRContext* aprCtx = static_cast<APRContext*>(ctx->boundRenderer);
                        aprCtx->zoomFactor *= zoomDelta;
                        aprCtx->zoomFactor = std::max(0.1f, std::min(10.0f, aprCtx->zoomFactor));
                    } else if (ctx->rendererType == 1) {  // MPR
                        MPRContext* mprCtx = static_cast<MPRContext*>(ctx->boundRenderer);
                        mprCtx->zoomFactor *= zoomDelta;
                        mprCtx->zoomFactor = std::max(0.1f, std::min(10.0f, mprCtx->zoomFactor));
                    }
                } else {
                    // Wheel = change slice (page)
                    const int step = (delta > 0) ? 1 : -1;
                    if (ctx->rendererType == 0 || ctx->rendererType == 2) {
                        APRContext* aprCtx = static_cast<APRContext*>(ctx->boundRenderer);
                        float cx = aprCtx->centerX;
                        float cy = aprCtx->centerY;
                        float cz = aprCtx->centerZ;
                        if (aprCtx->sliceDirection == 0) cz += (float)step;
                        else if (aprCtx->sliceDirection == 1) cy += (float)step;
                        else cx += (float)step;
                        APR_SetCenter(aprCtx, cx, cy, cz);
                    } else if (ctx->rendererType == 1) {
                        MPRContext* mprCtx = static_cast<MPRContext*>(ctx->boundRenderer);
                        float cx = mprCtx->centerX;
                        float cy = mprCtx->centerY;
                        float cz = mprCtx->centerZ;
                        if (mprCtx->sliceDirection == 0) cz += (float)step;
                        else if (mprCtx->sliceDirection == 1) cy += (float)step;
                        else cx += (float)step;
                        MPR_SetCenter(mprCtx, cx, cy, cz);
                    }
                }

                InvalidateRect(hwnd, nullptr, FALSE);
            }
            
            // 锟斤拷锟斤拷锟斤拷刷锟铰ｏ拷锟斤拷锟斤拷染锟竭筹拷统一锟斤拷锟斤拷
        }
        return 0;
        
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    // Some cases intentionally fall through (e.g. WM_PAINT when no GL context).
    // Always return a valid result to keep WndProc contract.
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

#if defined(_WIN32)

// 注锟斤拷Win32锟斤拷锟斤拷锟斤拷
static bool RegisterWin32WindowClass() {
    if (g_WindowClassRegistered) return true;
    
    WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = Win32WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = g_WindowClassName;
    
    if (!RegisterClassEx(&wc)) {
        SetLastError("Failed to register Win32 window class");
        return false;
    }
    
    g_WindowClassRegistered = true;
    return true;
}
#endif

#endif // _WIN32

// ==================== 锟斤拷锟斤拷锟斤拷 ====================
const char* Visualization_GetLastError() {
    return g_lastError.c_str();
}

// ==================== Tab Session Management API ====================

// Create or get a tab session context
NativeResult Session_Create(const char* sessionId) {
    if (!sessionId || sessionId[0] == '\0') {
        return NATIVE_E_INVALID_ARGUMENT;
    }
    TabSessionContext* ctx = GetTabSession(sessionId);
    return ctx ? NATIVE_OK : NATIVE_E_FAIL;
}

// Destroy a tab session and all its resources
NativeResult Session_Destroy(const char* sessionId) {
    if (!sessionId || sessionId[0] == '\0') {
        return NATIVE_E_INVALID_ARGUMENT;
    }
    
    std::string sessId(sessionId);
    TabSessionContext* tabCtx = FindTabSession(sessId);
    
    if (tabCtx) {
        // Clean up all APRs in this session
        for (APRHandle apr : tabCtx->linkedAPRs) {
            if (apr) {
                // Remove from global linkedAPRs if present
                auto& globalAPRs = g_globalAPRCenter.linkedAPRs;
                globalAPRs.erase(std::remove(globalAPRs.begin(), globalAPRs.end(), apr), globalAPRs.end());
                APR_Destroy(apr);
            }
        }
        tabCtx->linkedAPRs.clear();
        
        // Clean up cropped APR
        if (tabCtx->croppedAPR) {
            APR_Destroy(tabCtx->croppedAPR);
            tabCtx->croppedAPR = nullptr;
        }
        
        // Clean up from legacy storage
        auto cropIt = g_sessionCroppedAPRs.find(sessId);
        if (cropIt != g_sessionCroppedAPRs.end()) {
            g_sessionCroppedAPRs.erase(cropIt);
        }
        
        // Clean up windows
        for (WindowHandle win : tabCtx->windows) {
            if (win) {
                // Remove from global window list
                auto& allWins = g_AllWindows;
                allWins.erase(std::remove(allWins.begin(), allWins.end(), win), allWins.end());
                Window_Destroy(win);
            }
        }
        tabCtx->windows.clear();
        
        // Destroy the tab session
        DestroyTabSession(sessId);
        
        printf("[Session] Destroyed session: %s\n", sessionId);
    }
    
    return NATIVE_OK;
}

// Get the tab session's APR center (for linking multiple APRs in one tab)
NativeResult Session_GetAPRCenter(const char* sessionId, float* x, float* y, float* z) {
    if (!sessionId) return NATIVE_E_INVALID_ARGUMENT;
    
    TabSessionContext* tabCtx = FindTabSession(sessionId);
    if (tabCtx) {
        if (x) *x = tabCtx->aprCenter.x;
        if (y) *y = tabCtx->aprCenter.y;
        if (z) *z = tabCtx->aprCenter.z;
        return NATIVE_OK;
    }
    
    // Fallback to global
    if (x) *x = g_globalAPRCenter.x;
    if (y) *y = g_globalAPRCenter.y;
    if (z) *z = g_globalAPRCenter.z;
    return NATIVE_OK;
}

// Set the tab session's APR center
NativeResult Session_SetAPRCenter(const char* sessionId, float x, float y, float z) {
    if (!sessionId) return NATIVE_E_INVALID_ARGUMENT;
    
    TabSessionContext* tabCtx = GetTabSession(sessionId);
    if (tabCtx) {
        tabCtx->aprCenter.x = x;
        tabCtx->aprCenter.y = y;
        tabCtx->aprCenter.z = z;
    }
    
    // Also update global for backward compatibility
    g_globalAPRCenter.x = x;
    g_globalAPRCenter.y = y;
    g_globalAPRCenter.z = z;
    
    return NATIVE_OK;
}

// Get 3D view state for a session
NativeResult Session_Get3DState(const char* sessionId, float* rotX, float* rotY, float* zoom, float* panX, float* panY) {
    if (!sessionId) return NATIVE_E_INVALID_ARGUMENT;
    GetSession3DState(sessionId, rotX, rotY, zoom, panX, panY);
    return NATIVE_OK;
}

// Set 3D view state for a session
NativeResult Session_Set3DState(const char* sessionId, float rotX, float rotY, float zoom, float panX, float panY) {
    if (!sessionId) return NATIVE_E_INVALID_ARGUMENT;
    SetSession3DState(sessionId, rotX, rotY, zoom, panX, panY);
    return NATIVE_OK;
}

// Get 3D rotation matrix for a session
NativeResult Session_Get3DRotMat(const char* sessionId, float outMat[16]) {
    if (!sessionId || !outMat) return NATIVE_E_INVALID_ARGUMENT;
    
    float* mat = GetSessionRotMat(sessionId);
    std::memcpy(outMat, mat, sizeof(float) * 16);
    return NATIVE_OK;
}

// Set 3D rotation matrix for a session
NativeResult Session_Set3DRotMat(const char* sessionId, const float inMat[16]) {
    if (!sessionId || !inMat) return NATIVE_E_INVALID_ARGUMENT;
    
    TabSessionContext* tabCtx = FindTabSession(sessionId);
    if (tabCtx) {
        std::memcpy(tabCtx->rotMat, inMat, sizeof(float) * 16);
    }
    // Also update global
    std::memcpy(g_3dRotMat, inMat, sizeof(float) * 16);
    return NATIVE_OK;
}

// Reset 3D view for a session to default state
NativeResult Session_Reset3DView(const char* sessionId) {
    if (!sessionId) return NATIVE_E_INVALID_ARGUMENT;
    
    TabSessionContext* tabCtx = FindTabSession(sessionId);
    if (tabCtx) {
        tabCtx->rotX = 30.0f;
        tabCtx->rotY = 45.0f;
        tabCtx->zoom = 1.0f;
        tabCtx->panX = 0.0f;
        tabCtx->panY = 0.0f;
        Mat4_Identity(tabCtx->rotMat);
    }
    
    // Also reset global
    g_3dRotX = 30.0f;
    g_3dRotY = 45.0f;
    g_3dZoom = 1.0f;
    g_3dPanX = 0.0f;
    g_3dPanY = 0.0f;
    Mat4_Identity(g_3dRotMat);
    
    return NATIVE_OK;
}

// ==================== NanoVG 锟斤拷锟斤拷 ====================
extern "C" void Visualization_CleanupNanoVG() {
    if (g_nvgContext) {
        nvgDeleteGL3(g_nvgContext);
        g_nvgContext = nullptr;
    }
    g_nvgFontId = -1;
    g_nvgGlrc = nullptr;
}

// ==================== MPR ====================
MPRHandle MPR_Create() {
    auto ctx = new MPRContext();
    // 锟斤拷锟斤拷 OpenGL 锟斤拷锟斤拷
    glGenTextures(1, &ctx->textureID);
    return ctx;
}

void MPR_Destroy(MPRHandle handle) {
    if (handle) {
        auto ctx = static_cast<MPRContext*>(handle);
        if (ctx->textureID) {
            glDeleteTextures(1, &ctx->textureID);
        }
        delete ctx;
    }
}

NativeResult MPR_SetVolume(MPRHandle handle, VolumeHandle volume) {
    if (!handle) return NATIVE_E_INVALID_ARGUMENT;
    static_cast<MPRContext*>(handle)->volume = volume;
    return NATIVE_OK;
}

void MPR_SetSliceDirection(MPRHandle handle, MPRSliceDirection direction) {
    if (!handle) return;
    static_cast<MPRContext*>(handle)->sliceDirection = direction;
}

MPRSliceDirection MPR_GetSliceDirection(MPRHandle handle) {
    if (!handle) return MPR_CORONAL;
    return (MPRSliceDirection)static_cast<MPRContext*>(handle)->sliceDirection;
}

void MPR_SetCenter(MPRHandle handle, float x, float y, float z) {
    if (!handle) return;
    auto ctx = static_cast<MPRContext*>(handle);
    ctx->centerX = x; ctx->centerY = y; ctx->centerZ = z;
    
    // 锟斤拷锟斤拷全锟斤拷锟斤拷锟侥碉拷
    g_globalMPRCenter.x = x;
    g_globalMPRCenter.y = y;
    g_globalMPRCenter.z = z;
    g_globalMPRCenter.volume = ctx->volume;
    
    // 同锟斤拷锟斤拷锟斤拷锟斤拷锟接碉拷 MPR
    for (MPRHandle linkedMPR : g_globalMPRCenter.linkedMPRs) {
        if (linkedMPR != handle) {  // 锟斤拷锟截革拷锟斤拷锟斤拷锟皆硷拷
            auto linkedCtx = static_cast<MPRContext*>(linkedMPR);
            linkedCtx->centerX = x;
            linkedCtx->centerY = y;
            linkedCtx->centerZ = z;
        }
    }
}

void MPR_GetCenter(MPRHandle handle, float* x, float* y, float* z) {
    if (!handle) return;
    auto ctx = static_cast<MPRContext*>(handle);
    if (x) *x = ctx->centerX;
    if (y) *y = ctx->centerY;
    if (z) *z = ctx->centerZ;
}

void MPR_LinkCenter(MPRHandle* handles, int count) {
    if (!handles || count <= 0) return;
    
    // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟?
    g_globalMPRCenter.linkedMPRs.clear();
    
    // 锟斤拷锟斤拷锟斤拷锟斤拷 MPR 锟斤拷锟斤拷锟斤拷锟叫憋拷
    for (int i = 0; i < count; ++i) {
        if (handles[i]) {
            g_globalMPRCenter.linkedMPRs.push_back(handles[i]);
            
            // 锟斤拷锟斤拷堑锟揭伙拷锟斤拷锟绞癸拷锟斤拷锟斤拷锟斤拷锟斤拷牡锟斤拷锟轿拷锟斤拷锟斤拷牡锟?
            if (i == 0) {
                auto ctx = static_cast<MPRContext*>(handles[i]);
                g_globalMPRCenter.x = ctx->centerX;
                g_globalMPRCenter.y = ctx->centerY;
                g_globalMPRCenter.z = ctx->centerZ;
                g_globalMPRCenter.volume = ctx->volume;
            }
        }
    }
    
    // 同锟斤拷锟斤拷锟斤拷 MPR 锟斤拷锟斤拷锟侥碉拷
    for (MPRHandle mpr : g_globalMPRCenter.linkedMPRs) {
        auto ctx = static_cast<MPRContext*>(mpr);
        ctx->centerX = g_globalMPRCenter.x;
        ctx->centerY = g_globalMPRCenter.y;
        ctx->centerZ = g_globalMPRCenter.z;
    }
}

void* MPR_GetSlice(MPRHandle handle, int direction, int* width, int* height) {
    if (!handle) return nullptr;
    // TODO: 实锟斤拷实锟斤拷锟斤拷片锟斤拷取
    if (width) *width = 512;
    if (height) *height = 512;
    return nullptr;
}

void MPR_SetShowCrossHair(MPRHandle handle, bool show) {
    if (!handle) return;
    static_cast<MPRContext*>(handle)->showCrossHair = show;
}

void MPR_SetZoom(MPRHandle handle, float zoomFactor) {
    if (!handle) return;
    auto ctx = static_cast<MPRContext*>(handle);
    // 锟斤拷锟斤拷锟斤拷锟脚凤拷围 0.1 ~ 10.0
    if (zoomFactor < 0.1f) zoomFactor = 0.1f;
    if (zoomFactor > 10.0f) zoomFactor = 10.0f;
    ctx->zoomFactor = zoomFactor;
}

float MPR_GetZoom(MPRHandle handle) {
    if (!handle) return 1.0f;
    return static_cast<MPRContext*>(handle)->zoomFactor;
}

// 璁剧疆鍏宠仈鐨凷ession ID锛堢敤浜庝粠Session鑾峰彇mask鏁版嵁锛?
void MPR_SetSessionId(MPRHandle handle, const char* sessionId) {
    if (!handle) return;
    auto ctx = static_cast<MPRContext*>(handle);
    ctx->sessionId = sessionId ? sessionId : "";
}

void MPR_SetWindowLevel(MPRHandle handle, float windowWidth, float windowLevel) {
    if (!handle) return;
    auto ctx = static_cast<MPRContext*>(handle);
    if (windowWidth < 1.0f) windowWidth = 1.0f;
    ctx->windowWidthHU = windowWidth;
    ctx->windowLevelHU = windowLevel;
}

void MPR_GetWindowLevel(MPRHandle handle, float* windowWidth, float* windowLevel) {
    if (!handle) return;
    auto ctx = static_cast<MPRContext*>(handle);
    if (windowWidth) *windowWidth = ctx->windowWidthHU;
    if (windowLevel) *windowLevel = ctx->windowLevelHU;
}

// ==================== MPR Mask 锟斤拷锟接癸拷锟斤拷 ====================

NativeResult MPR_AddMask(MPRHandle handle, MaskManagerHandle manager, int maskIndex) {
    if (!handle || !manager) return NATIVE_E_INVALID_ARGUMENT;
    
    auto ctx = static_cast<MPRContext*>(handle);
    
    // 锟斤拷锟?mask 锟斤拷锟斤拷锟角凤拷锟斤拷效
    int maskCount = MaskManager_GetCount(manager);
    if (maskIndex < 0 || maskIndex >= maskCount) {
        return NATIVE_E_INVALID_ARGUMENT;
    }
    
    // 锟斤拷锟斤拷欠锟斤拷丫锟斤拷锟斤拷锟?
    for (const auto& overlay : ctx->maskOverlays) {
        if (overlay.manager == manager && overlay.maskIndex == maskIndex) {
            return NATIVE_OK;  // 锟窖达拷锟节ｏ拷锟斤拷锟截革拷锟斤拷锟斤拷
        }
    }
    
    // 锟斤拷锟斤拷锟铰碉拷 mask overlay
    MPRContext::MaskOverlay overlay;
    overlay.manager = manager;
    overlay.maskIndex = maskIndex;
    
    // 锟斤拷取 mask 锟斤拷锟斤拷色锟斤拷void 锟斤拷锟斤拷锟斤拷锟酵ｏ拷直锟接碉拷锟矫ｏ拷
    MaskManager_GetColor(manager, maskIndex, &overlay.r, &overlay.g, &overlay.b, &overlay.a);
    
    overlay.visible = MaskManager_GetVisible(manager, maskIndex);
    
    ctx->maskOverlays.push_back(overlay);
    return NATIVE_OK;
}

void MPR_RemoveMask(MPRHandle handle, int maskIndex) {
    if (!handle) return;
    
    auto ctx = static_cast<MPRContext*>(handle);
    
    // 锟狡筹拷指锟斤拷 maskIndex 锟斤拷锟斤拷锟斤拷 overlay
    ctx->maskOverlays.erase(
        std::remove_if(ctx->maskOverlays.begin(), ctx->maskOverlays.end(),
            [maskIndex](const MPRContext::MaskOverlay& overlay) {
                return overlay.maskIndex == maskIndex;
            }),
        ctx->maskOverlays.end()
    );
}

void MPR_ClearMasks(MPRHandle handle) {
    if (!handle) return;
    static_cast<MPRContext*>(handle)->maskOverlays.clear();
}

void MPR_SetMaskOpacity(MPRHandle handle, int maskIndex, float opacity) {
    if (!handle) return;
    
    auto ctx = static_cast<MPRContext*>(handle);
    
    // 锟斤拷锟斤拷透锟斤拷锟饺凤拷围 0.0 ~ 1.0
    if (opacity < 0.0f) opacity = 0.0f;
    if (opacity > 1.0f) opacity = 1.0f;
    
    // 锟斤拷锟斤拷锟斤拷锟斤拷匹锟斤拷锟?overlay
    for (auto& overlay : ctx->maskOverlays) {
        if (overlay.maskIndex == maskIndex) {
            overlay.a = opacity;
        }
    }
}

void MPR_SetMaskColor(MPRHandle handle, int maskIndex, float r, float g, float b, float a) {
    if (!handle) return;
    
    auto ctx = static_cast<MPRContext*>(handle);
    
    // 锟斤拷锟斤拷锟斤拷色锟斤拷围 0.0 ~ 1.0
    if (r < 0.0f) r = 0.0f;
    if (r > 1.0f) r = 1.0f;
    if (g < 0.0f) g = 0.0f;
    if (g > 1.0f) g = 1.0f;
    if (b < 0.0f) b = 0.0f;
    if (b > 1.0f) b = 1.0f;
    if (a < 0.0f) a = 0.0f;
    if (a > 1.0f) a = 1.0f;
    
    // 锟斤拷锟斤拷锟斤拷锟斤拷匹锟斤拷锟?overlay
    for (auto& overlay : ctx->maskOverlays) {
        if (overlay.maskIndex == maskIndex) {
            overlay.r = r;
            overlay.g = g;
            overlay.b = b;
            overlay.a = a;
        }
    }
}

void MPR_SetMaskVisible(MPRHandle handle, int maskIndex, bool visible) {
    if (!handle) return;
    
    auto ctx = static_cast<MPRContext*>(handle);
    
    // 锟斤拷锟斤拷锟斤拷锟斤拷匹锟斤拷锟?overlay
    for (auto& overlay : ctx->maskOverlays) {
        if (overlay.maskIndex == maskIndex) {
            overlay.visible = visible;
        }
    }

    // Trigger 3D mask iso-surface refresh when mask visibility changes.
    ctx->maskRevision++;
}

void MPR_SetShowAllMasks(MPRHandle handle, bool show) {
    if (!handle) return;
    static_cast<MPRContext*>(handle)->showAllMasks = show;
}

// Forward declaration for ParseHexColor
static void ParseHexColor(const char* hexColor, float& r, float& g, float& b);

static inline uint8_t ApplyWindowLevelToByte(int hu, float ww, float wl) {
    if (ww < 1.0f) ww = 1.0f;
    const float low = wl - ww * 0.5f;
    const float high = wl + ww * 0.5f;
    float t = (high > low) ? ((static_cast<float>(hu) - low) / (high - low)) : 0.0f;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return static_cast<uint8_t>(t * 255.0f + 0.5f);
}

NativeResult MPR_Render(MPRHandle handle) {
    if (!handle) return NATIVE_E_INVALID_ARGUMENT;
    auto ctx = static_cast<MPRContext*>(handle);
    if (!ctx->volume) return NATIVE_E_INVALID_ARGUMENT;

    float rescaleSlope = 1.0f;
    float rescaleIntercept = 0.0f;
    (void)Dicom_Volume_GetRescale(ctx->volume, &rescaleSlope, &rescaleIntercept);
    
    // 锟斤拷取 Volume 锟竭寸、锟斤拷锟捷和硷拷锟?
    int width, height, depth;
    float spacingX, spacingY, spacingZ;
    
    if (Dicom_Volume_GetDimensions(ctx->volume, &width, &height, &depth) != NATIVE_OK) {
        SetLastError("Failed to get volume dimensions");
        return NATIVE_E_INTERNAL_ERROR;
    }
    
    if (Dicom_Volume_GetSpacing(ctx->volume, &spacingX, &spacingY, &spacingZ) != NATIVE_OK) {
        SetLastError("Failed to get volume spacing");
        return NATIVE_E_INTERNAL_ERROR;
    }
    
    uint16_t* volumeData = static_cast<uint16_t*>(Dicom_Volume_GetData(ctx->volume));
    if (!volumeData) {
        SetLastError("Volume data is null");
        return NATIVE_E_INTERNAL_ERROR;
    }
    
    // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷呀锟斤拷锟斤拷约锟斤拷锟斤拷锟街撅拷锟?
    // static int debugCounter = 0;
    // if (debugCounter++ % 30 == 0) {
    //     printf("[MPR_Render] Direction=%d, Center=(%.1f,%.1f,%.1f), Volume=%dx%dx%d\n",
    //            ctx->sliceDirection, ctx->centerX, ctx->centerY, ctx->centerZ, width, height, depth);
    // }

    int sliceIndex, sliceWidth, sliceHeight;
    size_t sliceSize;
    std::vector<uint16_t> sliceData;
    const char* sliceName = "Unknown";
    
    // 锟斤拷锟斤拷锟斤拷片锟斤拷锟斤拷锟斤拷取锟斤拷同锟斤拷锟斤拷片
    switch (ctx->sliceDirection) {
        case 0: // Axial (XY plane, Z direction)
            sliceName = "Axial";
            sliceIndex = static_cast<int>(ctx->centerZ);
            if (sliceIndex < 0) sliceIndex = 0;
            if (sliceIndex >= depth) sliceIndex = depth - 1;
            
            sliceWidth = width;
            sliceHeight = height;
            sliceSize = static_cast<size_t>(sliceWidth) * sliceHeight;
            sliceData.resize(sliceSize);
            
            // 锟斤拷锟斤拷锟斤拷片锟斤拷锟教讹拷 Z锟斤拷锟斤拷取 XY 平锟斤拷
            // Volume 锟斤拷锟街ｏ拷[z][y][x]
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    size_t volumeIdx = static_cast<size_t>(sliceIndex) * width * height + y * width + x;
                    size_t sliceIdx = static_cast<size_t>(height - 1 - y) * width + x;  // 锟斤拷转Y
                    sliceData[sliceIdx] = volumeData[volumeIdx];
                }
            }
            break;
            
        case 1: // Coronal (XZ plane, Y direction)
            sliceName = "Coronal";
            sliceIndex = static_cast<int>(ctx->centerY);
            if (sliceIndex < 0) sliceIndex = 0;
            if (sliceIndex >= height) sliceIndex = height - 1;
            
            sliceWidth = width;
            sliceHeight = depth;
            sliceSize = static_cast<size_t>(sliceWidth) * sliceHeight;
            sliceData.resize(sliceSize);
            
            // 锟斤拷状锟芥：锟教讹拷 Y锟斤拷锟斤拷锟斤拷 X 锟斤拷 Z
            for (int z = 0; z < depth; ++z) {
                for (int x = 0; x < width; ++x) {
                    size_t volumeIdx = static_cast<size_t>(z) * width * height + sliceIndex * width + x;
                    size_t sliceIdx = static_cast<size_t>(depth - 1 - z) * width + x;  // 锟斤拷转Z
                    sliceData[sliceIdx] = volumeData[volumeIdx];
                }
            }
            break;
            
        case 2: // Sagittal (YZ plane, X direction)
            sliceName = "Sagittal";
            sliceIndex = static_cast<int>(ctx->centerX);
            if (sliceIndex < 0) sliceIndex = 0;
            if (sliceIndex >= width) sliceIndex = width - 1;
            
            sliceWidth = height;
            sliceHeight = depth;
            sliceSize = static_cast<size_t>(sliceWidth) * sliceHeight;
            sliceData.resize(sliceSize);
            
            // 矢状锟芥：锟教讹拷 X锟斤拷锟斤拷锟斤拷 Y 锟斤拷 Z
            for (int z = 0; z < depth; ++z) {
                for (int y = 0; y < height; ++y) {
                    size_t volumeIdx = static_cast<size_t>(z) * width * height + y * width + sliceIndex;
                    size_t sliceIdx = static_cast<size_t>(depth - 1 - z) * height + y;  // 锟斤拷转Z
                    sliceData[sliceIdx] = volumeData[volumeIdx];
                }
            }
            break;
            
        default:
            SetLastError("Invalid slice direction");
            return NATIVE_E_INVALID_ARGUMENT;
    }
    
    // 统锟斤拷锟斤拷片锟斤拷锟斤拷
    uint16_t minVal = 65535, maxVal = 0;
    uint64_t sum = 0;
    int nonZeroCount = 0;
    
    for (size_t i = 0; i < sliceSize; ++i) {
        uint16_t val = sliceData[i];
        if (val > 0) nonZeroCount++;
        if (val < minVal) minVal = val;
        if (val > maxVal) maxVal = val;
        sum += val;
    }
    
    float avgVal = nonZeroCount > 0 ? static_cast<float>(sum) / nonZeroCount : 0;
    
    // 锟斤拷锟斤拷锟斤拷锟斤拷锟狡拷锟较?
    // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷瞥锟斤拷锟斤拷锟斤拷锟剿拷锟斤拷锟?
    
    // Apply HU window/level to 0-255
    std::vector<uint8_t> displayData(sliceSize);
    for (size_t i = 0; i < sliceSize; ++i) {
        const float value = static_cast<float>(sliceData[i]) * rescaleSlope + rescaleIntercept;
        const int hu = static_cast<int>(value);
        displayData[i] = ApplyWindowLevelToByte(hu, ctx->windowWidthHU, ctx->windowLevelHU);
    }
    
    // 锟斤拷锟芥到锟斤拷锟斤拷锟斤拷
    ctx->displayBuffer = displayData;
    ctx->sliceWidth = sliceWidth;
    ctx->sliceHeight = sliceHeight;
    
    // 锟较达拷锟斤拷锟斤拷锟斤拷 OpenGL
    glBindTexture(GL_TEXTURE_2D, ctx->textureID);
    // 锟斤拷锟斤拷锟斤拷锟截讹拷锟斤拷为1锟斤拷锟斤拷锟斤拷锟斤拷炔锟斤拷锟?锟斤拷锟斤拷时锟侥达拷位锟斤拷锟斤拷
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, sliceWidth, sliceHeight, 0,
                 GL_LUMINANCE, GL_UNSIGNED_BYTE, displayData.data());
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // 锟街革拷默锟斤拷值
    
    // 锟斤拷锟姐定位锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷 [0, 1] 锟叫碉拷位锟斤拷
    float crossTexX = 0.5f, crossTexY = 0.5f;
    
    if (ctx->sliceDirection == 0) {
        // Axial (XY 平锟芥，Z 锟斤拷片)
        // X锟斤拷 锟斤拷 水平锟斤拷锟斤拷Y锟斤拷 锟斤拷 锟斤拷直锟斤拷锟斤拷
        crossTexX = ctx->centerX / (width - 1);           // X: [0, width-1] 锟斤拷 [0, 1]
        crossTexY = 1.0f - (ctx->centerY / (height - 1)); // Y: [0, height-1] 锟斤拷 [1, 0] (锟斤拷转)
    } else if (ctx->sliceDirection == 1) {
        // Coronal (XZ 平锟芥，Y 锟斤拷片)
        // X锟斤拷 锟斤拷 水平锟斤拷锟斤拷Z锟斤拷 锟斤拷 锟斤拷直锟斤拷锟斤拷
        crossTexX = ctx->centerX / (width - 1);           // X: [0, width-1] 锟斤拷 [0, 1]
        crossTexY = 1.0f - (ctx->centerZ / (depth - 1));  // Z: [0, depth-1] 锟斤拷 [1, 0] (锟斤拷转)
    } else if (ctx->sliceDirection == 2) {
        // Sagittal (YZ 平锟芥，X 锟斤拷片)
        // Y锟斤拷 锟斤拷 水平锟斤拷锟斤拷Z锟斤拷 锟斤拷 锟斤拷直锟斤拷锟斤拷
        crossTexX = ctx->centerY / (height - 1);          // Y: [0, height-1] 锟斤拷 [0, 1]
        crossTexY = 1.0f - (ctx->centerZ / (depth - 1));  // Z: [0, depth-1] 锟斤拷 [1, 0] (锟斤拷转)
    }
    
    // 锟斤拷染锟斤拷锟斤拷锟斤拷锟斤拷锟节ｏ拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟疥，锟斤拷锟斤拷围锟狡讹拷位锟斤拷锟斤拷锟脚ｏ拷
    RenderTextureToWindow(ctx->textureID, sliceWidth, sliceHeight, ctx->zoomFactor, crossTexX, crossTexY);
    
    // ==================== 锟斤拷染Mask锟斤拷锟接诧拷 ====================
    // 1. 锟斤拷染预锟斤拷mask锟斤拷锟斤拷时mask锟斤拷锟斤拷锟斤拷锟斤拷值锟斤拷锟斤拷预锟斤拷锟斤拷
    if (ctx->previewMask && ctx->previewMask->visible && !ctx->previewMask->data.empty()) {
        // 锟斤拷取锟斤拷前锟斤拷锟斤拷锟絤ask锟斤拷锟斤拷
        std::vector<uint8_t> maskSlice(sliceSize, 0);
        
        for (int i = 0; i < sliceHeight; ++i) {
            for (int j = 0; j < sliceWidth; ++j) {
                size_t volumeIdx = 0;
                
                // 锟斤拷锟斤拷锟斤拷片锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟?
                if (ctx->sliceDirection == 0) { // Axial
                    volumeIdx = static_cast<size_t>(sliceIndex) * width * height + (height - 1 - i) * width + j;
                } else if (ctx->sliceDirection == 1) { // Coronal
                    volumeIdx = static_cast<size_t>(depth - 1 - i) * width * height + sliceIndex * width + j;
                } else { // Sagittal
                    volumeIdx = static_cast<size_t>(depth - 1 - i) * width * height + j * width + sliceIndex;
                }
                
                if (volumeIdx < ctx->previewMask->data.size()) {
                    maskSlice[i * sliceWidth + j] = ctx->previewMask->data[volumeIdx];
                }
            }
        }
        
        // 锟斤拷锟斤拷锟斤拷色
        float r, g, b;
        ParseHexColor(ctx->previewMask->color.c_str(), r, g, b);
        
        // 锟斤拷锟斤拷RGBA锟斤拷锟斤拷锟斤拷锟斤拷透锟斤拷锟斤拷
        std::vector<uint8_t> rgba(sliceSize * 4);
        for (size_t i = 0; i < sliceSize; ++i) {
            if (maskSlice[i] > 0) {
                rgba[i * 4 + 0] = static_cast<uint8_t>(r * 255);
                rgba[i * 4 + 1] = static_cast<uint8_t>(g * 255);
                rgba[i * 4 + 2] = static_cast<uint8_t>(b * 255);
                rgba[i * 4 + 3] = 76;  // 30% alpha (0.3 * 255)
            } else {
                rgba[i * 4 + 3] = 0;  // 锟斤拷全透锟斤拷
            }
        }
        
        // 锟斤拷染锟斤拷透锟斤拷mask
        RenderMaskOverlay(rgba.data(), sliceWidth, sliceHeight, ctx->zoomFactor, crossTexX, crossTexY);
    }
    
    // 2. 娓叉煋permanent masks锛堜粠鏈湴ctx鍜宻ession ctx鍚堝苟锛?
    // 鑾峰彇瑕佹覆鏌撶殑masks鏉ユ簮
    const std::vector<MPRContext::MaskData>* masksToRender = &ctx->masks;
    MPRContext* sessionCtx = nullptr;
    
    // 濡傛灉鏈湴ctx鏈塻essionId锛屽皾璇曚粠session鑾峰彇masks
    if (!ctx->sessionId.empty()) {
        sessionCtx = GetMPRContextFromSession(ctx->sessionId.c_str());
        if (sessionCtx && !sessionCtx->masks.empty()) {
            masksToRender = &sessionCtx->masks;
        }
        // 鍚屾椂娓叉煋session鐨刾review mask
        if (sessionCtx && sessionCtx->previewMask && sessionCtx->previewMask->visible && !sessionCtx->previewMask->data.empty()) {
            std::vector<uint8_t> maskSlice(sliceSize, 0);
            for (int i = 0; i < sliceHeight; ++i) {
                for (int j = 0; j < sliceWidth; ++j) {
                    size_t volumeIdx = 0;
                    if (ctx->sliceDirection == 0) {
                        volumeIdx = static_cast<size_t>(sliceIndex) * width * height + (height - 1 - i) * width + j;
                    } else if (ctx->sliceDirection == 1) {
                        volumeIdx = static_cast<size_t>(depth - 1 - i) * width * height + sliceIndex * width + j;
                    } else {
                        volumeIdx = static_cast<size_t>(depth - 1 - i) * width * height + j * width + sliceIndex;
                    }
                    if (volumeIdx < sessionCtx->previewMask->data.size()) {
                        maskSlice[i * sliceWidth + j] = sessionCtx->previewMask->data[volumeIdx];
                    }
                }
            }
            float r, g, b;
            ParseHexColor(sessionCtx->previewMask->color.c_str(), r, g, b);
            std::vector<uint8_t> rgba(sliceSize * 4);
            for (size_t i = 0; i < sliceSize; ++i) {
                if (maskSlice[i] > 0) {
                    rgba[i * 4 + 0] = static_cast<uint8_t>(r * 255);
                    rgba[i * 4 + 1] = static_cast<uint8_t>(g * 255);
                    rgba[i * 4 + 2] = static_cast<uint8_t>(b * 255);
                    rgba[i * 4 + 3] = 76;
                } else {
                    rgba[i * 4 + 3] = 0;
                }
            }
            RenderMaskOverlay(rgba.data(), sliceWidth, sliceHeight, ctx->zoomFactor, crossTexX, crossTexY);
        }
    }
    
    for (const auto& mask : *masksToRender) {
        if (!mask.visible || mask.data.empty()) continue;
        
        // 锟斤拷取锟斤拷前锟斤拷锟斤拷锟絤ask锟斤拷锟斤拷
        std::vector<uint8_t> maskSlice(sliceSize, 0);
        
        for (int i = 0; i < sliceHeight; ++i) {
            for (int j = 0; j < sliceWidth; ++j) {
                size_t volumeIdx = 0;
                
                // 锟斤拷锟斤拷锟斤拷片锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟?
                if (ctx->sliceDirection == 0) { // Axial
                    volumeIdx = static_cast<size_t>(sliceIndex) * width * height + (height - 1 - i) * width + j;
                } else if (ctx->sliceDirection == 1) { // Coronal
                    volumeIdx = static_cast<size_t>(depth - 1 - i) * width * height + sliceIndex * width + j;
                } else { // Sagittal
                    volumeIdx = static_cast<size_t>(depth - 1 - i) * width * height + j * width + sliceIndex;
                }
                
                if (volumeIdx < mask.data.size()) {
                    maskSlice[i * sliceWidth + j] = mask.data[volumeIdx];
                }
            }
        }
        
        // 锟斤拷锟斤拷锟斤拷色
        float r, g, b;
        ParseHexColor(mask.color.c_str(), r, g, b);
        
        // 锟斤拷锟斤拷RGBA锟斤拷锟斤拷锟斤拷锟斤拷透锟斤拷锟斤拷
        std::vector<uint8_t> rgba(sliceSize * 4);
        for (size_t i = 0; i < sliceSize; ++i) {
            if (maskSlice[i] > 0) {
                rgba[i * 4 + 0] = static_cast<uint8_t>(r * 255);
                rgba[i * 4 + 1] = static_cast<uint8_t>(g * 255);
                rgba[i * 4 + 2] = static_cast<uint8_t>(b * 255);
                rgba[i * 4 + 3] = 128;  // 50% alpha (0.5 * 255)
            } else {
                rgba[i * 4 + 3] = 0;  // 锟斤拷全透锟斤拷
            }
        }
        
        // 锟斤拷染锟斤拷透锟斤拷mask
        RenderMaskOverlay(rgba.data(), sliceWidth, sliceHeight, ctx->zoomFactor, crossTexX, crossTexY);
    }
    
    // 锟斤拷染 Mask 锟斤拷锟接ｏ拷锟斤拷锟斤拷械幕锟斤拷锟?
    if (ctx->showAllMasks && !ctx->maskOverlays.empty()) {
        // 锟斤拷取锟斤拷锟斤拷锟斤拷维锟斤拷
        int width, height, depth;
        if (Dicom_Volume_GetDimensions(ctx->volume, &width, &height, &depth) != NATIVE_OK) {
            return NATIVE_E_INTERNAL_ERROR;
        }
        
        // 锟斤拷取锟斤拷前锟斤拷片锟斤拷锟斤拷
        int currentSliceIndex;
        if (ctx->sliceDirection == 0) {
            currentSliceIndex = (int)(ctx->centerZ + 0.5f);
        } else if (ctx->sliceDirection == 1) {
            currentSliceIndex = (int)(ctx->centerY + 0.5f);
        } else {
            currentSliceIndex = (int)(ctx->centerX + 0.5f);
        }
        
        // 锟斤拷锟斤拷锟斤拷锟斤拷要锟斤拷示锟斤拷mask
        for (auto& overlay : ctx->maskOverlays) {  // 注锟解：锟斤拷为锟斤拷const锟斤拷锟矫ｏ拷锟皆憋拷锟斤拷禄锟斤拷锟?
            if (!overlay.visible || !overlay.manager) continue;
            
            // 锟斤拷取mask维锟斤拷
            int maskWidth, maskHeight, maskDepth;
            MaskManager_GetDimensions(overlay.manager, overlay.maskIndex, 
                                     &maskWidth, &maskHeight, &maskDepth);
            
            // 锟斤拷锟絤ask维锟斤拷锟角凤拷锟斤拷锟斤拷锟斤拷锟斤拷匹锟斤拷
            if (maskWidth != width || maskHeight != height || maskDepth != depth) {
                continue;
            }
            
            // 锟斤拷锟斤拷欠锟斤拷锟揭拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟狡拷谋锟斤拷锟阶次硷拷锟截伙拷mask锟斤拷锟捷憋拷锟睫改ｏ拷
            bool needUpdate = (overlay.cachedSliceIndex != currentSliceIndex) || 
                            (overlay.cachedTextureID == 0) ||
                            g_maskStrokeNeedsUpdate;
            
            if (needUpdate) {
                // 锟斤拷取mask锟斤拷锟斤拷
                const uint8_t* maskData = MaskManager_GetData(overlay.manager, overlay.maskIndex);
                if (!maskData) continue;
                
                // 锟斤拷锟捷碉拷前锟斤拷片锟斤拷锟斤拷锟斤拷取2D mask锟斤拷片锟斤拷锟斤拷锟斤拷RGBA锟斤拷锟斤拷
                int texWidth, texHeight;
                std::vector<uint8_t> maskTexture;
                
                if (ctx->sliceDirection == 0) {  // Axial (XY平锟斤拷)
                    if (currentSliceIndex < 0 || currentSliceIndex >= depth) continue;
                    
                    texWidth = width;
                    texHeight = height;
                    maskTexture.resize(texWidth * texHeight * 4);  // RGBA
                    
                    for (int y = 0; y < height; y++) {
                        for (int x = 0; x < width; x++) {
                            int srcIndex = currentSliceIndex * width * height + y * width + x;
                            // 锟斤拷转Y锟斤拷锟斤拷匹锟斤拷OpenGL锟斤拷锟斤拷锟斤拷锟斤拷系锟斤拷texBottom锟斤拷锟铰ｏ拷texTop锟斤拷锟较ｏ拷
                            int dstIndex = ((height - 1 - y) * width + x) * 4;
                            
                            if (maskData[srcIndex] > 0) {
                                maskTexture[dstIndex + 0] = (uint8_t)(overlay.r * 255);
                                maskTexture[dstIndex + 1] = (uint8_t)(overlay.g * 255);
                                maskTexture[dstIndex + 2] = (uint8_t)(overlay.b * 255);
                                maskTexture[dstIndex + 3] = (uint8_t)(overlay.a * 255);
                            } else {
                                maskTexture[dstIndex + 3] = 0;
                            }
                        }
                    }
                }
                else if (ctx->sliceDirection == 1) {  // Coronal (XZ平锟斤拷)
                    if (currentSliceIndex < 0 || currentSliceIndex >= height) continue;
                    
                    texWidth = width;
                    texHeight = depth;
                    maskTexture.resize(texWidth * texHeight * 4);
                    
                    for (int z = 0; z < depth; z++) {
                        for (int x = 0; x < width; x++) {
                            int srcIndex = z * width * height + currentSliceIndex * width + x;
                            // 锟斤拷转Z锟斤拷匹锟斤拷MPR锟斤拷锟斤拷转Y锟斤拷锟斤拷匹锟斤拷OpenGL锟斤拷锟斤拷锟斤拷锟斤拷系
                            int dstIndex = (z * width + x) * 4;  // 注锟解：锟斤拷锟斤拷z锟窖撅拷锟角达拷锟较碉拷锟铰ｏ拷锟斤拷锟斤拷要锟劫凤拷转
                            
                            if (maskData[srcIndex] > 0) {
                                maskTexture[dstIndex + 0] = (uint8_t)(overlay.r * 255);
                                maskTexture[dstIndex + 1] = (uint8_t)(overlay.g * 255);
                                maskTexture[dstIndex + 2] = (uint8_t)(overlay.b * 255);
                                maskTexture[dstIndex + 3] = (uint8_t)(overlay.a * 255);
                            } else {
                                maskTexture[dstIndex + 3] = 0;
                            }
                        }
                    }
                }
                else {  // Sagittal (YZ平锟斤拷)
                    if (currentSliceIndex < 0 || currentSliceIndex >= width) continue;
                    
                    texWidth = height;
                    texHeight = depth;
                    maskTexture.resize(texWidth * texHeight * 4);
                    
                    for (int z = 0; z < depth; z++) {
                        for (int y = 0; y < height; y++) {
                            int srcIndex = z * width * height + y * width + currentSliceIndex;
                            // 锟斤拷转Z锟斤拷匹锟斤拷MPR锟斤拷Y锟结保锟街诧拷锟戒（锟窖撅拷锟角达拷锟较碉拷锟铰ｏ拷
                            int dstIndex = (z * height + y) * 4;
                            
                            if (maskData[srcIndex] > 0) {
                                maskTexture[dstIndex + 0] = (uint8_t)(overlay.r * 255);
                                maskTexture[dstIndex + 1] = (uint8_t)(overlay.g * 255);
                                maskTexture[dstIndex + 2] = (uint8_t)(overlay.b * 255);
                                maskTexture[dstIndex + 3] = (uint8_t)(overlay.a * 255);
                            } else {
                                maskTexture[dstIndex + 3] = 0;
                            }
                        }
                    }
                }
                
                // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟?
                if (overlay.cachedTextureID == 0) {
                    glGenTextures(1, &overlay.cachedTextureID);
                }
                
                glBindTexture(GL_TEXTURE_2D, overlay.cachedTextureID);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, 
                            GL_RGBA, GL_UNSIGNED_BYTE, maskTexture.data());
                glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
                
                // 锟斤拷锟铰伙拷锟斤拷锟斤拷息
                overlay.cachedSliceIndex = currentSliceIndex;
            }
            
            // 使锟斤拷锟斤拷MPR锟斤拷同锟侥变换锟斤拷染mask
            // 锟斤拷取锟斤拷锟节尺达拷
            GLint viewport[4];
            glGetIntegerv(GL_VIEWPORT, viewport);
            int winWidth = viewport[2];
            int winHeight = viewport[3];
            
            // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟捷猴拷龋锟斤拷锟組PR使锟斤拷锟斤拷同锟斤拷sliceWidth/sliceHeight锟斤拷
            float texAspect = static_cast<float>(sliceWidth) / sliceHeight;
            float winAspect = static_cast<float>(winWidth) / winHeight;
            
            // 锟斤拷锟姐保锟斤拷锟捷猴拷鹊锟斤拷锟绞撅拷锟斤拷锟斤拷锟組PR锟斤拷全一锟铰ｏ拷
            float baseLeft, baseRight, baseBottom, baseTop;
            if (texAspect > winAspect) {
                baseLeft = -1.0f;
                baseRight = 1.0f;
                float h = winAspect / texAspect;
                baseBottom = -h;
                baseTop = h;
            } else {
                baseBottom = -1.0f;
                baseTop = 1.0f;
                float w = texAspect / winAspect;
                baseLeft = -w;
                baseRight = w;
            }
            
            // 应锟斤拷锟斤拷锟脚ｏ拷围锟狡讹拷位锟竭ｏ拷
            float texLeft, texRight, texBottom, texTop;
            if (ctx->zoomFactor != 1.0f) {
                float displayTexWidth = 1.0f / ctx->zoomFactor;
                float displayTexHeight = 1.0f / ctx->zoomFactor;
                
                float leftRatio = crossTexX;
                float rightRatio = 1.0f - crossTexX;
                float bottomRatio = crossTexY;
                float topRatio = 1.0f - crossTexY;
                
                texLeft = crossTexX - displayTexWidth * leftRatio;
                texRight = crossTexX + displayTexWidth * rightRatio;
                texBottom = crossTexY - displayTexHeight * bottomRatio;
                texTop = crossTexY + displayTexHeight * topRatio;
                
                // 锟竭斤拷锟斤拷
                texLeft = std::max(0.0f, std::min(1.0f, texLeft));
                texRight = std::max(0.0f, std::min(1.0f, texRight));
                texBottom = std::max(0.0f, std::min(1.0f, texBottom));
                texTop = std::max(0.0f, std::min(1.0f, texTop));
            } else {
                texLeft = 0.0f;
                texRight = 1.0f;
                texBottom = 0.0f;
                texTop = 1.0f;
            }
            
            // 锟斤拷染mask锟斤拷锟斤拷锟斤拷使锟斤拷锟斤拷MPR锟斤拷同锟斤拷锟斤拷锟疥）
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, overlay.cachedTextureID);
            
            glBegin(GL_QUADS);
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            glTexCoord2f(texLeft, texTop);    glVertex2f(baseLeft, baseBottom);
            glTexCoord2f(texRight, texTop);   glVertex2f(baseRight, baseBottom);
            glTexCoord2f(texRight, texBottom); glVertex2f(baseRight, baseTop);
            glTexCoord2f(texLeft, texBottom);  glVertex2f(baseLeft, baseTop);
            glEnd();
            
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_BLEND);
        }
        
        // 锟斤拷锟矫革拷锟铰憋拷志
        g_maskStrokeNeedsUpdate = false;
    }
    
    // 锟斤拷锟斤拷锟斤拷锟斤拷硕锟轿伙拷撸锟斤拷业锟角帮拷锟斤拷锟斤拷嵌锟轿伙拷吖锟斤拷锟?tool 0)锟斤拷锟斤拷锟狡讹拷位锟斤拷
    if (ctx->showCrossHair && g_currentToolType == 0) {
        // 锟斤拷取锟斤拷前锟斤拷锟节尺寸（锟斤拷OpenGL锟接口伙拷取锟斤拷锟斤拷锟斤拷Win32锟斤拷GLFW锟斤拷
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        int winWidth = viewport[2];
        int winHeight = viewport[3];

        const TexWindowMapping map = ComputeTexWindowMapping(sliceWidth, sliceHeight, winWidth, winHeight, ctx->zoomFactor, crossTexX, crossTexY);
        if (map.valid) {
            const float dx = map.texRight - map.texLeft;
            const float dy = map.texTop - map.texBottom;

            float crossScreenX = 0.0f;
            float crossScreenY = 0.0f;
            if (std::fabs(dx) > 1e-6f && std::fabs(dy) > 1e-6f) {
                crossScreenX = map.baseLeft + ((crossTexX - map.texLeft) / dx) * (map.baseRight - map.baseLeft);
                crossScreenY = map.baseBottom + ((crossTexY - map.texBottom) / dy) * (map.baseTop - map.baseBottom);
            } else {
                crossScreenX = 0.0f;
                crossScreenY = 0.0f;
            }

            DrawCrossHair(crossScreenX, crossScreenY, ctx->sliceDirection, 0.0f);
        }
    }
    
    // ==================== 锟斤拷锟狡诧拷锟斤拷锟斤拷锟斤拷 (tool 1-6) ====================
    if (g_currentToolType >= 1 && g_currentToolType <= 6) {
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_LINE_SMOOTH);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

        GLint viewportMeasure[4];
        glGetIntegerv(GL_VIEWPORT, viewportMeasure);
        const int winWidthMeasure = viewportMeasure[2];
        const int winHeightMeasure = viewportMeasure[3];
        const TexWindowMapping measureMap = ComputeTexWindowMapping(sliceWidth, sliceHeight, winWidthMeasure, winHeightMeasure, ctx->zoomFactor, crossTexX, crossTexY);
    
        // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟狡碉拷锟斤拷锟斤拷锟竭ｏ拷锟斤拷锟斤拷为3D锟斤拷锟斤拷锟斤拷锟疥，转锟斤拷为NDC锟斤拷锟斤拷疲锟?
    // 锟斤拷锟节伙拷锟斤拷锟斤拷锟节伙拷锟侥诧拷锟斤拷锟斤拷牵锟绞癸拷玫锟角帮拷锟絚enterX/Y/Z锟斤拷
    auto DrawTool = [ctx, &measureMap](int toolType, const std::vector<MeasurementPoint>& points, bool isActive, float result) {
        if (points.empty()) return;

        std::vector<MeasurementPoint> adjustedPoints = points;
        if (isActive && g_shiftPressed && adjustedPoints.size() >= 2 && (toolType == 3 || toolType == 4)) {
            adjustedPoints[1] = ConstrainSquareInPlane(adjustedPoints[0], adjustedPoints[1], ctx->sliceDirection);
        }

        // 锟斤拷3D锟斤拷锟斤拷锟斤拷锟斤拷转锟斤拷为2D NDC锟斤拷锟斤拷
        std::vector<Point2D> ndcPoints;
        for (const auto& wp : adjustedPoints) {
            Point2D p = MPR_WorldToNDC(wp, ctx);
            if (measureMap.valid) {
                p = ImageNdcToScreenNdc(measureMap, p);
            }
            ndcPoints.push_back(p);
        }

        glColor3f(isActive ? 1.0f : 0.0f, 1.0f, isActive ? 0.0f : 0.0f);  // 锟斤拷锟斤拷=锟斤拷色锟斤拷锟斤拷锟?锟斤拷色
        glLineWidth(2.0f);

        switch (toolType) {
            case 1: // 直锟斤拷
                if (ndcPoints.size() >= 2) {
                    glBegin(GL_LINES);
                    glVertex2f(ndcPoints[0].x, ndcPoints[0].y);
                    glVertex2f(ndcPoints[1].x, ndcPoints[1].y);
                    glEnd();
                }
                break;

            case 2: // 锟角讹拷
                if (ndcPoints.size() >= 2) {
                    glBegin(GL_LINES);
                    glVertex2f(ndcPoints[0].x, ndcPoints[0].y);
                    glVertex2f(ndcPoints[1].x, ndcPoints[1].y);
                    if (ndcPoints.size() >= 3) {
                        glVertex2f(ndcPoints[1].x, ndcPoints[1].y);
                        glVertex2f(ndcPoints[2].x, ndcPoints[2].y);
                    }
                    glEnd();
                }
                break;

            case 3: // 锟斤拷锟斤拷
                if (ndcPoints.size() >= 2) {
                    float x1 = ndcPoints[0].x, y1 = ndcPoints[0].y;
                    float x2 = ndcPoints[1].x, y2 = ndcPoints[1].y;

                    glBegin(GL_LINE_LOOP);
                    glVertex2f(x1, y1);
                    glVertex2f(x2, y1);
                    glVertex2f(x2, y2);
                    glVertex2f(x1, y2);
                    glEnd();
                }
                break;

            case 4: // 圆锟轿ｏ拷锟斤拷圆锟斤拷锟斤拷泳锟斤拷危锟?
                if (ndcPoints.size() >= 2) {
                    float x1 = ndcPoints[0].x, y1 = ndcPoints[0].y;
                    float x2 = ndcPoints[1].x, y2 = ndcPoints[1].y;

                    float dx = x2 - x1, dy = y2 - y1;

                    // 锟斤拷锟斤拷锟斤拷锟侥和半径
                    float cx = (x1 + x2) / 2.0f;
                    float cy = (y1 + y2) / 2.0f;
                    float rx = fabsf(dx) / 2.0f;
                    float ry = fabsf(dy) / 2.0f;

                    // 锟斤拷锟斤拷锟斤拷圆锟斤拷锟斤拷泳锟斤拷危锟?
                    glBegin(GL_LINE_LOOP);
                    int segments = 64;
                    for (int i = 0; i < segments; i++) {
                        float angle = 2.0f * 3.14159f * i / segments;
                        glVertex2f(cx + rx * cosf(angle), cy + ry * sinf(angle));
                    }
                    glEnd();

                    // 锟斤拷锟斤拷圆锟斤拷
                    glPointSize(6.0f);
                    glBegin(GL_POINTS);
                    glVertex2f(cx, cy);
                    glEnd();
                }
                break;

            case 5: // Catmull-Rom 锟斤拷锟斤拷锟斤拷通锟斤拷锟斤拷锟叫匡拷锟狡碉拷锟狡斤拷锟斤拷锟斤拷撸锟?
                if (ndcPoints.size() >= 2) {
                    // Catmull-Rom 锟斤拷值锟斤拷锟斤拷
                    auto catmullRom = [](float t, float p0, float p1, float p2, float p3) {
                        float t2 = t * t;
                        float t3 = t2 * t;
                        return 0.5f * (
                            (2.0f * p1) +
                            (-p0 + p2) * t +
                            (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                            (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
                        );
                    };
                    
                    if (ndcPoints.size() == 2) {
                        // 只锟斤拷锟斤拷锟斤拷锟姐，直锟斤拷锟斤拷锟斤拷
                        glBegin(GL_LINES);
                        glVertex2f(ndcPoints[0].x, ndcPoints[0].y);
                        glVertex2f(ndcPoints[1].x, ndcPoints[1].y);
                        glEnd();
                    } else {
                        // 锟斤拷锟斤拷悖癸拷锟?Catmull-Rom 锟斤拷锟斤拷锟斤拷直锟斤拷锟斤拷 NDC 锟斤拷锟斤拷锟斤拷锟?
                        const int segmentsPerCurve = 20;
                        glBegin(GL_LINE_STRIP);
                        
                        for (size_t i = 0; i < ndcPoints.size() - 1; ++i) {
                            // 锟斤拷取4锟斤拷锟斤拷锟狡点（p0, p1, p2, p3锟斤拷
                            Point2D p0 = (i == 0) ? ndcPoints[0] : ndcPoints[i - 1];
                            Point2D p1 = ndcPoints[i];
                            Point2D p2 = ndcPoints[i + 1];
                            Point2D p3 = (i + 2 < ndcPoints.size()) ? ndcPoints[i + 2] : ndcPoints[i + 1];
                            
                            // 锟斤拷 p1 锟斤拷 p2 之锟斤拷锟街?
                            for (int j = 0; j <= segmentsPerCurve; ++j) {
                                float t = (float)j / (float)segmentsPerCurve;
                                float x = catmullRom(t, p0.x, p1.x, p2.x, p3.x);
                                float y = catmullRom(t, p0.y, p1.y, p2.y, p3.y);
                                glVertex2f(x, y);
                            }
                        }
                        
                        glEnd();
                    }
                }
                break;

            case 6: // 锟斤拷锟斤拷锟斤拷锟竭ｏ拷锟斤拷眨锟?
                if (ndcPoints.size() >= 2) {
                    // 只锟斤拷锟斤拷锟竭ｏ拷锟斤拷锟斤拷锟狡匡拷锟狡点（锟矫伙拷要锟斤拷
                    glBegin(GL_LINE_LOOP);
                    for (const auto& pt : ndcPoints) {
                        glVertex2f(pt.x, pt.y);
                    }
                    glEnd();
                }
                break;
        }

        // 锟斤拷锟狡匡拷锟狡点（锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟解）
        if (toolType != 6) {
            glColor3f(1.0f, 0.0f, 0.0f);  // 锟斤拷色
            glPointSize(8.0f);
            glBegin(GL_POINTS);
            for (const auto& pt : ndcPoints) {
                glVertex2f(pt.x, pt.y);
            }
            glEnd();
        }
    };
    
    // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷傻牟锟斤拷锟斤拷锟角ｏ拷使锟矫达拷锟斤拷时锟侥固讹拷centerX/Y/Z锟斤拷锟斤拷锟斤拷锟斤拷center锟戒化锟斤拷锟斤拷锟狡讹拷锟斤拷
    auto DrawToolFixed = [ctx, &measureMap](int measurementIndex, int toolType, const std::vector<MeasurementPoint>& points, const MeasurementLocation& loc, float result) {
        if (points.empty()) return;

        // 锟斤拷3D锟斤拷锟斤拷锟斤拷锟斤拷转锟斤拷为2D NDC锟斤拷锟疥（使锟矫达拷锟斤拷时锟侥固讹拷center锟斤拷
        std::vector<Point2D> ndcPoints;
        for (const auto& wp : points) {
            Point2D p = MPR_WorldToNDC_Fixed(wp, ctx, loc.centerX, loc.centerY, loc.centerZ);
            if (measureMap.valid) {
                p = ImageNdcToScreenNdc(measureMap, p);
            }
            ndcPoints.push_back(p);
        }

        glColor3f(0.0f, 1.0f, 0.0f);  // 锟斤拷傻牟锟斤拷锟斤拷锟斤拷=锟斤拷色
        glLineWidth(2.0f);

        switch (toolType) {
            case 1: // 直锟斤拷
                if (ndcPoints.size() >= 2) {
                    glBegin(GL_LINES);
                    glVertex2f(ndcPoints[0].x, ndcPoints[0].y);
                    glVertex2f(ndcPoints[1].x, ndcPoints[1].y);
                    glEnd();
                }
                break;

            case 2: // 锟角讹拷
                if (ndcPoints.size() >= 2) {
                    glBegin(GL_LINES);
                    glVertex2f(ndcPoints[0].x, ndcPoints[0].y);
                    glVertex2f(ndcPoints[1].x, ndcPoints[1].y);
                    if (ndcPoints.size() >= 3) {
                        glVertex2f(ndcPoints[1].x, ndcPoints[1].y);
                        glVertex2f(ndcPoints[2].x, ndcPoints[2].y);
                    }
                    glEnd();
                }
                break;

            case 3: // 锟斤拷锟斤拷
                if (ndcPoints.size() >= 2) {
                    float x1 = ndcPoints[0].x, y1 = ndcPoints[0].y;
                    float x2 = ndcPoints[1].x, y2 = ndcPoints[1].y;
                    glBegin(GL_LINE_LOOP);
                    glVertex2f(x1, y1);
                    glVertex2f(x2, y1);
                    glVertex2f(x2, y2);
                    glVertex2f(x1, y2);
                    glEnd();
                }
                break;

            case 4: // 鍦嗗舰锛堝疄闄呮槸妞渾锛岄€氳繃涓ょ偣瀹氫箟鍖呭洿鐩掞級
                if (ndcPoints.size() >= 2) {
                    float x1 = ndcPoints[0].x, y1 = ndcPoints[0].y;
                    float x2 = ndcPoints[1].x, y2 = ndcPoints[1].y;
                    float dx = x2 - x1;
                    float dy = y2 - y1;
                    // 璁＄畻涓績鍜屽崐寰?
                    float cx = (x1 + x2) / 2.0f;
                    float cy = (y1 + y2) / 2.0f;
                    float rx = fabsf(dx) / 2.0f;
                    float ry = fabsf(dy) / 2.0f;
                    // 缁樺埗妞渾锛堝鏋渞x==ry鍒欎负姝ｅ渾锛?
                    glBegin(GL_LINE_LOOP);
                    int segments = 64;
                    for (int i = 0; i < segments; i++) {
                        float angle = 2.0f * 3.14159f * i / segments;
                        glVertex2f(cx + rx * cosf(angle), cy + ry * sinf(angle));
                    }
                    glEnd();
                    // 缁樺埗鍦嗗績
                    glPointSize(6.0f);
                    glBegin(GL_POINTS);
                    glVertex2f(cx, cy);
                    glEnd();
                }
                break;

            case 5: // Catmull-Rom 锟斤拷锟斤拷
                if (ndcPoints.size() >= 2) {
                    auto catmullRom = [](float t, float p0, float p1, float p2, float p3) {
                        float t2 = t * t;
                        float t3 = t2 * t;
                        return 0.5f * (
                            (2.0f * p1) +
                            (-p0 + p2) * t +
                            (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                            (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
                        );
                    };
                    
                    if (ndcPoints.size() == 2) {
                        // 只锟斤拷锟斤拷锟斤拷锟姐，直锟斤拷锟斤拷锟斤拷
                        glBegin(GL_LINES);
                        glVertex2f(ndcPoints[0].x, ndcPoints[0].y);
                        glVertex2f(ndcPoints[1].x, ndcPoints[1].y);
                        glEnd();
                    } else {
                        // 锟斤拷锟斤拷悖癸拷锟?Catmull-Rom 锟斤拷锟斤拷
                        const int segmentsPerCurve = 20;
                        glBegin(GL_LINE_STRIP);
                        
                        for (size_t i = 0; i < ndcPoints.size() - 1; ++i) {
                            Point2D p0 = (i > 0) ? ndcPoints[i - 1] : ndcPoints[i];
                            Point2D p1 = ndcPoints[i];
                            Point2D p2 = ndcPoints[i + 1];
                            Point2D p3 = (i + 2 < ndcPoints.size()) ? ndcPoints[i + 2] : ndcPoints[i + 1];
                            
                            for (int t = 0; t <= segmentsPerCurve; ++t) {
                                float tNorm = t / (float)segmentsPerCurve;
                                float x = catmullRom(tNorm, p0.x, p1.x, p2.x, p3.x);
                                float y = catmullRom(tNorm, p0.y, p1.y, p2.y, p3.y);
                                glVertex2f(x, y);
                            }
                        }
                        
                        glEnd();
                    }
                    
                    // 锟斤拷锟狡匡拷锟狡碉拷
                    glPointSize(8.0f);
                    glBegin(GL_POINTS);
                    for (const auto& pt : ndcPoints) {
                        glVertex2f(pt.x, pt.y);
                    }
                    glEnd();
                }
                break;

            case 6: // 锟斤拷锟斤拷锟斤拷锟竭ｏ拷锟斤拷眨锟?
                if (ndcPoints.size() >= 2) {
                    // 只锟斤拷锟狡凤拷锟斤拷锟斤拷撸锟斤拷锟斤拷锟斤拷瓶锟斤拷频悖拷没锟揭拷锟?
                    glBegin(GL_LINE_LOOP);
                    for (const auto& pt : ndcPoints) {
                        glVertex2f(pt.x, pt.y);
                    }
                    glEnd();
                }
                break;
        }

        // Draw control points for completed measurements (except freehand)
        if (toolType != 6) {
            glColor3f(1.0f, 0.0f, 0.0f);
            glPointSize(8.0f);
            glBegin(GL_POINTS);
            for (const auto& pt : ndcPoints) {
                glVertex2f(pt.x, pt.y);
            }
            glEnd();

            if (measurementIndex == g_hoverMeasurementIndex && g_hoverPointIndex >= 0 && g_hoverPointIndex < (int)ndcPoints.size()) {
                const auto& hp = ndcPoints[(size_t)g_hoverPointIndex];
                glColor3f(1.0f, 1.0f, 0.0f);
                glPointSize(12.0f);
                glBegin(GL_POINTS);
                glVertex2f(hp.x, hp.y);
                glEnd();
            }
        }
    };
    
    // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷欠锟接︼拷锟斤拷诘锟角帮拷锟狡拷锟绞?
    auto ShouldDisplayMeasurement = [ctx](const MeasurementLocation& loc) -> bool {
        // MPR锟斤拷图锟斤拷锟斤拷示APR锟斤拷锟斤拷锟侥憋拷牵锟斤拷锟轿狝PR锟斤拷锟斤拷转锟斤拷锟斤拷锟斤拷系锟斤拷同锟斤拷
        if (loc.isAPR) {
            return false;
        }
        
        // 锟斤拷锟斤拷锟狡拷锟斤拷锟斤拷欠锟狡ワ拷锟?
        if (loc.sliceDirection != ctx->sliceDirection) {
            return false;
        }
        
        // 锟斤拷锟姐当前锟斤拷片锟斤拷锟斤拷
        int currentSliceIndex = 0;
        if (ctx->sliceDirection == 0) {  // Axial - Z锟斤拷锟斤拷
            currentSliceIndex = (int)(ctx->centerZ + 0.5f);
        } else if (ctx->sliceDirection == 1) {  // Coronal - Y锟斤拷锟斤拷
            currentSliceIndex = (int)(ctx->centerY + 0.5f);
        } else {  // Sagittal - X锟斤拷锟斤拷
            currentSliceIndex = (int)(ctx->centerX + 0.5f);
        }
        
        // 只锟斤拷锟斤拷同一锟斤拷锟斤拷锟绞?
        return loc.sliceIndex == currentSliceIndex;
    };
    
    // 锟斤拷锟斤拷锟斤拷锟斤拷傻牟锟斤拷锟酵硷拷危锟街伙拷锟斤拷锟斤拷锟斤拷诘锟角帮拷锟狡拷模锟?
    {
        std::lock_guard<std::mutex> lock(g_completedMeasurementsMutex);
        for (size_t mi = 0; mi < g_completedMeasurements.size(); ++mi) {
            const auto& measurement = g_completedMeasurements[mi];
            if (ShouldDisplayMeasurement(measurement.location)) {
                DrawToolFixed((int)mi, measurement.toolType, measurement.points, measurement.location, measurement.result);
            }
        }
    }
    
    // 锟斤拷锟狡碉拷前锟斤拷锟节伙拷锟狡碉拷图锟轿ｏ拷锟斤拷锟斤拷锟斤拷示锟斤拷锟斤拷为锟斤拷锟斤拷锟节碉拷前锟斤拷片锟斤拷
    if (!g_measurementPoints.empty()) {
        auto activePoints = g_measurementPoints;
        
        // 锟斤拷锟斤拷锟较讹拷锟斤拷锟竭ｏ拷锟斤拷锟接碉拷前锟斤拷锟轿伙拷锟斤拷锟轿拷诙锟斤拷锟斤拷锟?
        if (g_isDrawing && (g_currentToolType == 1 || g_currentToolType == 3 || g_currentToolType == 4)) {
            if (activePoints.size() == 1) {
                activePoints.push_back(g_currentMousePos);
            }
        }
        
        // 锟角度癸拷锟竭ｏ拷锟斤拷锟斤拷锟斤拷选锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷预锟斤拷
        if (g_currentToolType == 2) {
            if (activePoints.size() == 1) {
                activePoints.push_back(g_currentMousePos);
            } else if (activePoints.size() == 2) {
                activePoints.push_back(g_currentMousePos);
            }
        }

        // 锟斤拷锟节憋拷锟斤拷锟斤拷锟斤拷锟竭ｏ拷锟斤拷锟斤拷预锟斤拷锟斤拷锟斤拷前锟斤拷锟轿伙拷锟?
        if (g_currentToolType == 5 && activePoints.size() >= 1) {
            activePoints.push_back(g_currentMousePos);
        }
        
        DrawTool(g_currentToolType, activePoints, true, 0.0f);
    }

    // Draw measurement labels (NanoVG)
    // Ensure NanoVG/font are ready so HUD is reliable across tool switches.
    EnsureNanoVGReady();
    if (g_nvgContext) {
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        int winWidth = viewport[2];
        int winHeight = viewport[3];

        if (winWidth > 0 && winHeight > 0) {
            PrepareGLForNanoVGOverlay();
            nvgBeginFrame(g_nvgContext, winWidth, winHeight, 1.0f);

            // Slice header top-center (text requires font)
            if (g_nvgFontId >= 0) {
                int w = 0, h = 0, d = 0;
                Dicom_Volume_GetDimensions(ctx->volume, &w, &h, &d);
                int sliceIndex = 0;
                int sliceTotal = 0;
                const char* plane = "";
                if (ctx->sliceDirection == 0) { plane = "Axial"; sliceIndex = (int)(ctx->centerZ + 0.5f); sliceTotal = d; }
                else if (ctx->sliceDirection == 1) { plane = "Coronal"; sliceIndex = (int)(ctx->centerY + 0.5f); sliceTotal = h; }
                else { plane = "Sagittal"; sliceIndex = (int)(ctx->centerX + 0.5f); sliceTotal = w; }
                if (sliceTotal <= 0) sliceTotal = 1;
                if (sliceIndex < 0) sliceIndex = 0;
                if (sliceIndex > sliceTotal - 1) sliceIndex = sliceTotal - 1;
                char header[96];
                snprintf(header, sizeof(header), "%s Slice %d / %d", plane, sliceIndex, sliceTotal);
                DrawSliceHeaderNVG_InFrame(winWidth, winHeight, header);
            }

            DrawWindowLevelHudNVG_InFrame(winWidth, winHeight, ctx->windowWidthHU, ctx->windowLevelHU);
            DrawVerticalScaleBarNVG_InFrame(winWidth, winHeight, ctx->sliceDirection, ctx->zoomFactor, ctx->volume);

            // Measurement labels require a font; skip if font isn't available.
            if (g_nvgFontId >= 0) {
                nvgFontSize(g_nvgContext, 16.0f);
                nvgFontFace(g_nvgContext, "ui");
                nvgTextAlign(g_nvgContext, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

                auto NdcToScreen = [&](const Point2D& ndc) -> Point2D {
                    Point2D s;
                    s.x = (ndc.x + 1.0f) * 0.5f * (float)winWidth;
                    s.y = (1.0f - (ndc.y + 1.0f) * 0.5f) * (float)winHeight;
                    return s;
                };

                auto FormatLabel = [&](int toolType, float value, char* out, size_t outSize) {
                    switch (toolType) {
                        case 1:
                            snprintf(out, outSize, "%.2f mm", value);
                            break;
                        case 2:
                            snprintf(out, outSize, "%.1f\xC2\xB0", value);
                            break;
                        case 3:
                        case 4:
                            snprintf(out, outSize, "%.2f mm\xC2\xB2", value);
                            break;
                        case 5:
                        case 6:
                            snprintf(out, outSize, "%.2f mm", value);
                            break;
                        default:
                            snprintf(out, outSize, "%.2f", value);
                            break;
                    }
                };

                auto AnchorFromNdc = [&](int toolType, const std::vector<Point2D>& ndcPoints) -> Point2D {
                    if (ndcPoints.empty()) return {0, 0};
                    if (toolType == 2 && ndcPoints.size() >= 2) {
                        return ndcPoints[1];
                    }
                    if ((toolType == 1 || toolType == 3 || toolType == 4) && ndcPoints.size() >= 2) {
                        return {(ndcPoints[0].x + ndcPoints[1].x) * 0.5f, (ndcPoints[0].y + ndcPoints[1].y) * 0.5f};
                    }

                    // Curve: average points
                    float sx = 0.0f, sy = 0.0f;
                    for (const auto& p : ndcPoints) { sx += p.x; sy += p.y; }
                    float inv = 1.0f / (float)ndcPoints.size();
                    return {sx * inv, sy * inv};
                };

                auto DrawLabel = [&](const Point2D& ndcAnchor, const char* text, bool active) {
                    Point2D sp = NdcToScreen(ndcAnchor);
                    sp.x += 6.0f;
                    sp.y += 0.0f;

                    // shadow
                    nvgFillColor(g_nvgContext, nvgRGBA(0, 0, 0, 160));
                    nvgText(g_nvgContext, sp.x + 1.0f, sp.y + 1.0f, text, nullptr);

                    if (active) {
                        nvgFillColor(g_nvgContext, nvgRGBA(255, 255, 0, 220));
                    } else {
                        nvgFillColor(g_nvgContext, nvgRGBA(0, 255, 0, 220));
                    }
                    nvgText(g_nvgContext, sp.x, sp.y, text, nullptr);
                };

                // Completed measurements
                {
                    std::lock_guard<std::mutex> lock(g_completedMeasurementsMutex);
                    for (size_t mi = 0; mi < g_completedMeasurements.size(); ++mi) {
                        const auto& measurement = g_completedMeasurements[mi];
                        if (!ShouldDisplayMeasurement(measurement.location)) continue;

                        std::vector<Point2D> ndcPoints;
                        ndcPoints.reserve(measurement.points.size());
                        for (const auto& wp : measurement.points) {
                            ndcPoints.push_back(MPR_WorldToNDC_Fixed(wp, ctx, measurement.location.centerX, measurement.location.centerY, measurement.location.centerZ));
                        }

                        char buf[64] = {0};
                        FormatLabel(measurement.toolType, measurement.result, buf, sizeof(buf));
                        DrawLabel(AnchorFromNdc(measurement.toolType, ndcPoints), buf, false);
                    }
                }

                // Active measurement preview
                if (!g_measurementPoints.empty()) {
                    auto previewPoints = g_measurementPoints;
                    if (g_isDrawing && (g_currentToolType == 1 || g_currentToolType == 3 || g_currentToolType == 4)) {
                        if (previewPoints.size() == 1) previewPoints.push_back(g_currentMousePos);
                    }
                    if (g_currentToolType == 2) {
                        if (previewPoints.size() == 1) previewPoints.push_back(g_currentMousePos);
                        else if (previewPoints.size() == 2) previewPoints.push_back(g_currentMousePos);
                    }
                    if (g_currentToolType == 5 && previewPoints.size() >= 1) {
                        previewPoints.push_back(g_currentMousePos);
                    }

                    CompletedMeasurement tmp{};
                    tmp.toolType = g_currentToolType;
                    tmp.points = previewPoints;
                    tmp.location.sliceDirection = ctx->sliceDirection;
                    tmp.result = RecomputeCompletedMeasurementResult(tmp);

                    std::vector<Point2D> ndcPoints;
                    ndcPoints.reserve(tmp.points.size());
                    for (const auto& wp : tmp.points) {
                        ndcPoints.push_back(MPR_WorldToNDC(wp, ctx));
                    }

                    if (!ndcPoints.empty()) {
                        char buf[64] = {0};
                        FormatLabel(tmp.toolType, tmp.result, buf, sizeof(buf));
                        DrawLabel(AnchorFromNdc(tmp.toolType, ndcPoints), buf, true);
                    }
                }
            }

            nvgEndFrame(g_nvgContext);
        }
    }
    }  // 锟斤拷锟斤拷 if (g_currentToolType >= 1 && g_currentToolType <= 6)
    
    // ==================== 锟斤拷锟斤拷Mask锟洁辑锟斤拷锟斤拷预锟斤拷圈 (tool 7=MaskEdit) ====================
    if (g_currentToolType == 7 && (g_currentMaskTool == 1 || g_currentMaskTool == 2)) {
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_LINE_SMOOTH);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // 锟斤拷锟斤拷锟轿伙拷茫锟?D锟斤拷锟斤拷锟斤拷锟疥）转锟斤拷为NDC
        Point2D mouseNDC = MPR_WorldToNDC(g_currentMousePos, ctx);
        
        // 锟斤拷取锟斤拷锟节猴拷锟斤拷锟斤拷锟竭寸，锟斤拷锟斤拷锟斤拷确锟斤拷锟斤拷锟截憋拷锟斤拷
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        int winWidth = viewport[2];
        int winHeight = viewport[3];
        
        // 锟斤拷锟斤拷锟斤拷锟斤拷锟捷猴拷群锟斤拷锟绞撅拷锟斤拷锟?
        float texAspect = static_cast<float>(ctx->sliceWidth) / ctx->sliceHeight;
        float winAspect = static_cast<float>(winWidth) / winHeight;
        
        // 锟斤拷锟斤拷实锟斤拷锟斤拷示锟斤拷锟斤拷目锟斤拷弑鹊锟斤拷锟较碉拷锟?
        float scaleX = 1.0f, scaleY = 1.0f;
        if (texAspect > winAspect) {
            // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟叫黑憋拷
            scaleY = winAspect / texAspect;
        } else {
            // 锟斤拷锟斤拷锟斤拷锟竭ｏ拷锟斤拷锟斤拷锟叫黑憋拷
            scaleX = texAspect / winAspect;
        }
        
        // 锟斤拷锟斤拷刖讹拷锟絅DC锟秸硷拷锟酵队帮拷锟斤拷锟斤拷锟斤拷莺锟饺猴拷锟斤拷锟脚ｏ拷
        // 锟斤拷锟绞半径锟斤拷锟斤拷锟截碉拷位锟斤拷锟斤拷要转锟斤拷为NDC锟秸硷拷
        float radiusTexX = g_brushRadius / ctx->sliceWidth * 2.0f * scaleX / ctx->zoomFactor;
        float radiusTexY = g_brushRadius / ctx->sliceHeight * 2.0f * scaleY / ctx->zoomFactor;
        
        // 直锟斤拷使锟矫硷拷锟斤拷锟斤拷陌刖讹拷锟斤拷丫锟斤拷锟斤拷锟斤拷锟斤拷莺锟饺ｏ拷
        float radiusX = radiusTexX;
        float radiusY = radiusTexY;
        
        // 锟斤拷锟斤拷圆锟斤拷预锟斤拷锟斤拷使锟矫碉拷前锟斤拷锟斤拷锟斤拷色锟斤拷
        if (g_currentMaskTool == 1) {  // Brush
            glColor3f(0.0f, 1.0f, 0.0f);  // 锟斤拷锟斤拷 - 锟斤拷色
        } else {  // Eraser
            glColor3f(1.0f, 0.0f, 0.0f);  // 锟斤拷皮锟斤拷 - 锟斤拷色
        }
        glLineWidth(2.0f);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 64; i++) {  // 锟斤拷锟接分讹拷锟斤拷使圆锟斤拷锟解滑
            float angle = (float)i / 64.0f * 2.0f * 3.14159f;
            float x = mouseNDC.x + radiusX * cosf(angle);
            float y = mouseNDC.y + radiusY * sinf(angle);
            glVertex2f(x, y);
        }
        glEnd();
        
        // 锟斤拷锟斤拷锟斤拷锟斤拷十锟斤拷
        glLineWidth(1.0f);
        glBegin(GL_LINES);
        glVertex2f(mouseNDC.x - radiusX * 0.2f, mouseNDC.y);
        glVertex2f(mouseNDC.x + radiusX * 0.2f, mouseNDC.y);
        glVertex2f(mouseNDC.x, mouseNDC.y - radiusY * 0.2f);
        glVertex2f(mouseNDC.x, mouseNDC.y + radiusY * 0.2f);
        glEnd();
        
        // 锟斤拷锟狡笔伙拷预锟斤拷路锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷诨锟斤拷疲锟? 使锟斤拷NanoVG锟斤拷锟斤拷圆锟轿端碉拷平锟斤拷锟斤拷锟斤拷
        if (g_isDrawing && !g_maskStrokePath.empty()) {
            EnsureNanoVGReady();
            if (!g_nvgContext) {
                glDisable(GL_BLEND);
                glDisable(GL_LINE_SMOOTH);
                return NATIVE_OK;
            }

            // 锟斤拷取锟斤拷前锟接口达拷小锟斤拷NanoVG锟斤拷要锟斤拷
            GLint viewport[4];
            glGetIntegerv(GL_VIEWPORT, viewport);
            int winWidth = viewport[2];
            int winHeight = viewport[3];
            
            // 锟斤拷始NanoVG锟斤拷锟斤拷
            PrepareGLForNanoVGOverlay();
            nvgBeginFrame(g_nvgContext, winWidth, winHeight, 1.0f);
            
            // 锟斤拷取锟斤拷片锟竭达拷
            int sliceWidth, sliceHeight;
            if (ctx->sliceDirection == 0) {
                Dicom_Volume_GetDimensions(ctx->volume, &sliceWidth, &sliceHeight, nullptr);
            } else if (ctx->sliceDirection == 1) {
                int depth;
                Dicom_Volume_GetDimensions(ctx->volume, &sliceWidth, nullptr, &depth);
                sliceHeight = depth;
            } else {
                int height, depth;
                Dicom_Volume_GetDimensions(ctx->volume, nullptr, &height, &depth);
                sliceWidth = height;
                sliceHeight = depth;
            }
            
            // 转锟斤拷路锟斤拷锟姐（锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷幕锟斤拷锟斤拷锟斤拷锟斤拷
            // 锟斤拷锟斤拷锟斤拷要锟斤拷锟斤拷锟斤拷悖猴拷锟斤拷锟斤拷锟斤拷锟?锟斤拷 锟斤拷锟斤拷锟斤拷锟斤拷 锟斤拷 NDC 锟斤拷 锟斤拷幕锟斤拷锟斤拷
            std::vector<Point2D> screenPoints;
            
            // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷示锟斤拷锟斤拷
            float texAspect = (float)sliceWidth / sliceHeight;
            float winAspect = (float)winWidth / winHeight;
            
            float baseLeft, baseRight, baseBottom, baseTop;
            if (texAspect > winAspect) {
                baseLeft = -1.0f;
                baseRight = 1.0f;
                float h = winAspect / texAspect;
                baseBottom = -h;
                baseTop = h;
            } else {
                baseBottom = -1.0f;
                baseTop = 1.0f;
                float w = texAspect / winAspect;
                baseLeft = -w;
                baseRight = w;
            }
            
            // 锟斤拷锟斤拷锟斤拷锟疥范围锟斤拷锟斤拷锟斤拷锟斤拷锟脚ｏ拷
            float crossTexX = 0.5f, crossTexY = 0.5f;
            float texLeft, texRight, texBottom, texTop;
            if (ctx->zoomFactor != 1.0f) {
                float displayTexWidth = 1.0f / ctx->zoomFactor;
                float displayTexHeight = 1.0f / ctx->zoomFactor;
                texLeft = crossTexX - displayTexWidth * crossTexX;
                texRight = crossTexX + displayTexWidth * (1.0f - crossTexX);
                texBottom = crossTexY - displayTexHeight * crossTexY;
                texTop = crossTexY + displayTexHeight * (1.0f - crossTexY);
                texLeft = std::max(0.0f, std::min(1.0f, texLeft));
                texRight = std::max(0.0f, std::min(1.0f, texRight));
                texBottom = std::max(0.0f, std::min(1.0f, texBottom));
                texTop = std::max(0.0f, std::min(1.0f, texTop));
            } else {
                texLeft = 0.0f;
                texRight = 1.0f;
                texBottom = 0.0f;
                texTop = 1.0f;
            }
            
            for (const auto& pt : g_maskStrokePath) {
                // 锟斤拷锟斤拷锟斤拷锟斤拷 锟斤拷 锟斤拷锟斤拷锟斤拷锟斤拷 [0,1]
                float texU = pt.x / sliceWidth;
                float texV = 1.0f - (pt.y / sliceHeight); // 锟斤拷转Y
                
                // 锟斤拷锟斤拷锟斤拷锟斤拷锟节碉拷前锟斤拷示锟斤拷围锟节碉拷锟斤拷锟轿伙拷锟?
                float relX = (texU - texLeft) / (texRight - texLeft);
                float relY = (texV - texBottom) / (texTop - texBottom);
                
                // 锟斤拷锟轿伙拷锟?锟斤拷 NDC
                float ndcX = baseLeft + relX * (baseRight - baseLeft);
                float ndcY = baseBottom + relY * (baseTop - baseBottom);
                
                // NDC 锟斤拷 锟斤拷幕锟斤拷锟斤拷
                float screenX = (ndcX + 1.0f) * 0.5f * winWidth;
                float screenY = (1.0f - ndcY) * 0.5f * winHeight;
                screenPoints.push_back({screenX, screenY});
            }
            
            // 锟斤拷始锟斤拷锟斤拷路锟斤拷
            if (!screenPoints.empty()) {
                nvgBeginPath(g_nvgContext);
                nvgMoveTo(g_nvgContext, screenPoints[0].x, screenPoints[0].y);
                for (size_t i = 1; i < screenPoints.size(); i++) {
                    nvgLineTo(g_nvgContext, screenPoints[i].x, screenPoints[i].y);
                }
                
                // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷式锟斤拷圆锟轿端点、圆锟斤拷锟斤拷锟接ｏ拷
                float strokeWidth = g_brushRadius * 2.0f;  // 直锟斤拷
                nvgStrokeWidth(g_nvgContext, strokeWidth);
                nvgLineCap(g_nvgContext, NVG_ROUND);    // 圆锟轿端碉拷
                nvgLineJoin(g_nvgContext, NVG_ROUND);   // 圆锟斤拷锟斤拷锟斤拷
                
                // 锟斤拷锟斤拷锟斤拷色锟斤拷锟斤拷透锟斤拷锟斤拷
                if (g_currentMaskTool == 1) {  // Brush
                    nvgStrokeColor(g_nvgContext, nvgRGBA(0, 255, 0, 128));  // 锟斤拷透锟斤拷锟斤拷色
                } else {  // Eraser
                    nvgStrokeColor(g_nvgContext, nvgRGBA(255, 0, 0, 128));  // 锟斤拷透锟斤拷锟斤拷色
                }
                
                nvgStroke(g_nvgContext);
            }
            
            // 锟斤拷锟斤拷NanoVG锟斤拷锟斤拷
            nvgEndFrame(g_nvgContext);
            
            glDisable(GL_BLEND);
        }
        
        glDisable(GL_BLEND);
        glDisable(GL_LINE_SMOOTH);
    }

    // HUD overlay (slice header + WW/WL + scale bar) should be visible even when toolType==0.
    // Previously this lived inside the measurement-tool branch, so ROI 缂栬緫榛樿宸ュ叿(0)鐪嬩笉鍒版枃瀛椼€?
    if (!(g_currentToolType >= 1 && g_currentToolType <= 6)) {
        EnsureNanoVGReady();
        if (g_nvgContext) {
            GLint viewport[4];
            glGetIntegerv(GL_VIEWPORT, viewport);
            int winWidth = viewport[2];
            int winHeight = viewport[3];

            if (winWidth > 0 && winHeight > 0) {
                PrepareGLForNanoVGOverlay();
                nvgBeginFrame(g_nvgContext, winWidth, winHeight, 1.0f);

                // Slice header top-center (text requires font)
                if (g_nvgFontId >= 0) {
                    int w = 0, h = 0, d = 0;
                    Dicom_Volume_GetDimensions(ctx->volume, &w, &h, &d);
                    int sliceIndex = 0;
                    int sliceTotal = 0;
                    const char* plane = "";
                    if (ctx->sliceDirection == 0) { plane = "Axial"; sliceIndex = (int)(ctx->centerZ + 0.5f); sliceTotal = d; }
                    else if (ctx->sliceDirection == 1) { plane = "Coronal"; sliceIndex = (int)(ctx->centerY + 0.5f); sliceTotal = h; }
                    else { plane = "Sagittal"; sliceIndex = (int)(ctx->centerX + 0.5f); sliceTotal = w; }
                    if (sliceTotal <= 0) sliceTotal = 1;
                    if (sliceIndex < 0) sliceIndex = 0;
                    if (sliceIndex > sliceTotal - 1) sliceIndex = sliceTotal - 1;
                    char header[96];
                    snprintf(header, sizeof(header), "%s Slice %d / %d", plane, sliceIndex, sliceTotal);
                    DrawSliceHeaderNVG_InFrame(winWidth, winHeight, header);
                }

                DrawWindowLevelHudNVG_InFrame(winWidth, winHeight, ctx->windowWidthHU, ctx->windowLevelHU);
                DrawVerticalScaleBarNVG_InFrame(winWidth, winHeight, ctx->sliceDirection, ctx->zoomFactor, ctx->volume);

                nvgEndFrame(g_nvgContext);
            }
        }
    }
    
    return NATIVE_OK;
}

// ==================== APR ====================
APRHandle APR_Create() {
    auto ctx = new APRContext();
    glGenTextures(1, &ctx->textureID);
    Mat4_Identity(ctx->rotMat);
    return ctx;
}

// 锟斤拷锟斤拷锟捷结构锟斤拷锟解部锟斤拷锟藉，锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
// VolumeContext is now defined in ../Common/VolumeData.h

// ==================== 锟斤拷锟斤拷转锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷实锟斤拷 ====================
// MPR: 锟斤拷幕NDC锟斤拷锟斤拷 锟斤拷 3D锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
static MeasurementPoint MPR_NDCToWorld(float ndcX, float ndcY, MPRContext* ctx) {
    if (!ctx || !ctx->volume) return {0, 0, 0};
    
    auto vol = static_cast<VolumeContext*>(ctx->volume);
    MeasurementPoint world = {0, 0, 0};
    
    // NDC锟斤拷围 [-1, 1]锟斤拷锟斤拷要转锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
    // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
    float halfWidth = ctx->sliceWidth * 0.5f / ctx->zoomFactor;
    float halfHeight = ctx->sliceHeight * 0.5f / ctx->zoomFactor;
    
    switch (ctx->sliceDirection) {
        case 0: // Axial (XY平锟芥，Z锟教讹拷)
            world.x = ctx->centerX + ndcX * halfWidth * vol->spacingX;
            world.y = ctx->centerY + ndcY * halfHeight * vol->spacingY;
            world.z = ctx->centerZ;
            break;
        case 1: // Coronal (XZ平锟芥，Y锟教讹拷)
            world.x = ctx->centerX + ndcX * halfWidth * vol->spacingX;
            world.y = ctx->centerY;
            world.z = ctx->centerZ - ndcY * halfHeight * vol->spacingZ;
            break;
        case 2: // Sagittal (YZ平锟芥，X锟教讹拷)
            world.x = ctx->centerX;
            world.y = ctx->centerY + ndcX * halfWidth * vol->spacingY;
            world.z = ctx->centerZ - ndcY * halfHeight * vol->spacingZ;
            break;
    }
    
    return world;
}

static MeasurementPoint MPR_NDCToWorld_Fixed(float ndcX, float ndcY, MPRContext* ctx,
                                            float fixedCenterX, float fixedCenterY, float fixedCenterZ) {
    if (!ctx || !ctx->volume) return {0, 0, 0};

    auto vol = static_cast<VolumeContext*>(ctx->volume);
    MeasurementPoint world = {0, 0, 0};

    float halfWidth = ctx->sliceWidth * 0.5f / ctx->zoomFactor;
    float halfHeight = ctx->sliceHeight * 0.5f / ctx->zoomFactor;

    switch (ctx->sliceDirection) {
        case 0: // Axial (XY plane, Z fixed)
            world.x = fixedCenterX + ndcX * halfWidth * vol->spacingX;
            world.y = fixedCenterY + ndcY * halfHeight * vol->spacingY;
            world.z = fixedCenterZ;
            break;
        case 1: // Coronal (XZ plane, Y fixed)
            world.x = fixedCenterX + ndcX * halfWidth * vol->spacingX;
            world.y = fixedCenterY;
            world.z = fixedCenterZ - ndcY * halfHeight * vol->spacingZ;
            break;
        case 2: // Sagittal (YZ plane, X fixed)
            world.x = fixedCenterX;
            world.y = fixedCenterY + ndcX * halfWidth * vol->spacingY;
            world.z = fixedCenterZ - ndcY * halfHeight * vol->spacingZ;
            break;
    }

    return world;
}

// MPR: 3D锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷 锟斤拷 锟斤拷幕NDC锟斤拷锟疥（投影锟斤拷锟斤拷前锟斤拷片锟斤拷
static Point2D MPR_WorldToNDC(const MeasurementPoint& world, MPRContext* ctx) {
    if (!ctx || !ctx->volume) return {0, 0};
    
    auto vol = static_cast<VolumeContext*>(ctx->volume);
    float halfWidth = ctx->sliceWidth * 0.5f / ctx->zoomFactor;
    float halfHeight = ctx->sliceHeight * 0.5f / ctx->zoomFactor;
    
    Point2D ndc = {0, 0};
    
    switch (ctx->sliceDirection) {
        case 0: // Axial (XY平锟芥，Z锟斤拷锟斤拷片)
            ndc.x = (world.x - ctx->centerX) / (halfWidth * vol->spacingX);
            ndc.y = (world.y - ctx->centerY) / (halfHeight * vol->spacingY);
            break;
        case 1: // Coronal (XZ平锟芥，Y锟斤拷锟斤拷片)
            ndc.x = (world.x - ctx->centerX) / (halfWidth * vol->spacingX);
            ndc.y = -(world.z - ctx->centerZ) / (halfHeight * vol->spacingZ);
            break;
        case 2: // Sagittal (YZ平锟芥，X锟斤拷锟斤拷片)
            ndc.x = (world.y - ctx->centerY) / (halfWidth * vol->spacingY);
            ndc.y = -(world.z - ctx->centerZ) / (halfHeight * vol->spacingZ);
            break;
    }
    
    return ndc;
}

// 使锟矫固讹拷锟斤拷锟斤拷片锟斤拷锟斤拷锟斤拷锟斤拷转锟斤拷锟斤拷锟斤拷锟斤拷锟疥到NDC锟斤拷锟斤拷锟节伙拷锟斤拷锟斤拷锟斤拷傻牟锟斤拷锟斤拷锟角ｏ拷锟斤拷锟斤拷锟斤拷center锟戒化锟斤拷锟斤拷锟狡讹拷锟斤拷
static Point2D MPR_WorldToNDC_Fixed(const MeasurementPoint& world, MPRContext* ctx, 
                                     float fixedCenterX, float fixedCenterY, float fixedCenterZ) {
    if (!ctx || !ctx->volume) return {0, 0};
    
    auto vol = static_cast<VolumeContext*>(ctx->volume);
    float halfWidth = ctx->sliceWidth * 0.5f / ctx->zoomFactor;
    float halfHeight = ctx->sliceHeight * 0.5f / ctx->zoomFactor;
    
    Point2D ndc = {0, 0};
    
    switch (ctx->sliceDirection) {
        case 0: // Axial (XY平锟芥，Z锟斤拷锟斤拷片)
            ndc.x = (world.x - fixedCenterX) / (halfWidth * vol->spacingX);
            ndc.y = (world.y - fixedCenterY) / (halfHeight * vol->spacingY);
            break;
        case 1: // Coronal (XZ平锟芥，Y锟斤拷锟斤拷片)
            ndc.x = (world.x - fixedCenterX) / (halfWidth * vol->spacingX);
            ndc.y = -(world.z - fixedCenterZ) / (halfHeight * vol->spacingZ);
            break;
        case 2: // Sagittal (YZ平锟芥，X锟斤拷锟斤拷片)
            ndc.x = (world.y - fixedCenterY) / (halfWidth * vol->spacingY);
            ndc.y = -(world.z - fixedCenterZ) / (halfHeight * vol->spacingZ);
            break;
    }
    
    return ndc;
}

// APR: 锟斤拷幕NDC锟斤拷锟斤拷 + 锟斤拷转锟斤拷锟斤拷 锟斤拷 3D锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
static MeasurementPoint APR_NDCToWorld(float ndcX, float ndcY, APRContext* ctx) {
    if (!ctx || !ctx->volume) return {0, 0, 0};
    
    // APR使锟斤拷3D锟斤拷锟斤拷锟斤拷图锟斤拷锟斤拷要锟斤拷锟斤拷锟斤拷转
    // 锟斤拷锟斤拷蚧锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟节碉拷前锟斤拷锟斤拷锟斤拷
    auto vol = static_cast<VolumeContext*>(ctx->volume);
    MeasurementPoint world = {0, 0, 0};
    
    float halfWidth = ctx->sliceWidth * 0.5f / ctx->zoomFactor;
    float halfHeight = ctx->sliceHeight * 0.5f / ctx->zoomFactor;
    
    // 锟斤拷锟斤拷锟斤拷锟疥（未锟斤拷转锟斤拷
    switch (ctx->sliceDirection) {
        case 0: // Axial
            world.x = ctx->centerX + ndcX * halfWidth * vol->spacingX;
            world.y = ctx->centerY + ndcY * halfHeight * vol->spacingY;
            world.z = ctx->centerZ;
            break;
        case 1: // Coronal
            world.x = ctx->centerX + ndcX * halfWidth * vol->spacingX;
            world.y = ctx->centerY;
            world.z = ctx->centerZ - ndcY * halfHeight * vol->spacingZ;
            break;
        case 2: // Sagittal
            world.x = ctx->centerX;
            world.y = ctx->centerY + ndcX * halfWidth * vol->spacingY;
            world.z = ctx->centerZ - ndcY * halfHeight * vol->spacingZ;
            break;
    }
    
    // TODO: 锟斤拷锟斤拷锟揭э拷锟斤拷锟阶拷锟斤拷锟斤拷锟接︼拷锟斤拷锟阶拷锟斤拷锟?
    return world;
}

static MeasurementPoint APR_NDCToWorld_Fixed(float ndcX, float ndcY, APRContext* ctx,
                                            float fixedCenterX, float fixedCenterY, float fixedCenterZ) {
    if (!ctx || !ctx->volume) return {0, 0, 0};

    auto vol = static_cast<VolumeContext*>(ctx->volume);
    MeasurementPoint world = {0, 0, 0};

    float halfWidth = ctx->sliceWidth * 0.5f / ctx->zoomFactor;
    float halfHeight = ctx->sliceHeight * 0.5f / ctx->zoomFactor;

    switch (ctx->sliceDirection) {
        case 0: // Axial
            world.x = fixedCenterX + ndcX * halfWidth * vol->spacingX;
            world.y = fixedCenterY + ndcY * halfHeight * vol->spacingY;
            world.z = fixedCenterZ;
            break;
        case 1: // Coronal
            world.x = fixedCenterX + ndcX * halfWidth * vol->spacingX;
            world.y = fixedCenterY;
            world.z = fixedCenterZ - ndcY * halfHeight * vol->spacingZ;
            break;
        case 2: // Sagittal
            world.x = fixedCenterX;
            world.y = fixedCenterY + ndcX * halfWidth * vol->spacingY;
            world.z = fixedCenterZ - ndcY * halfHeight * vol->spacingZ;
            break;
    }

    // TODO: rotation-aware mapping for APR
    return world;
}

// APR: 3D锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷 锟斤拷 锟斤拷幕NDC锟斤拷锟疥（锟斤拷锟斤拷锟斤拷转锟斤拷
static Point2D APR_WorldToNDC(const MeasurementPoint& world, APRContext* ctx) {
    if (!ctx || !ctx->volume) return {0, 0};
    
    // TODO: 应锟斤拷锟斤拷锟斤拷转锟斤拷锟斤拷
    // 锟津化版本锟斤拷直锟斤拷投影
    auto vol = static_cast<VolumeContext*>(ctx->volume);
    float halfWidth = ctx->sliceWidth * 0.5f / ctx->zoomFactor;
    float halfHeight = ctx->sliceHeight * 0.5f / ctx->zoomFactor;
    
    Point2D ndc = {0, 0};
    
    switch (ctx->sliceDirection) {
        case 0: // Axial
            ndc.x = (world.x - ctx->centerX) / (halfWidth * vol->spacingX);
            ndc.y = (world.y - ctx->centerY) / (halfHeight * vol->spacingY);
            break;
        case 1: // Coronal
            ndc.x = (world.x - ctx->centerX) / (halfWidth * vol->spacingX);
            ndc.y = -(world.z - ctx->centerZ) / (halfHeight * vol->spacingZ);
            break;
        case 2: // Sagittal
            ndc.x = (world.y - ctx->centerY) / (halfWidth * vol->spacingY);
            ndc.y = -(world.z - ctx->centerZ) / (halfHeight * vol->spacingZ);
            break;
    }
    
    return ndc;
}

// 使锟矫固讹拷锟斤拷锟斤拷片锟斤拷锟斤拷锟斤拷锟斤拷转锟斤拷锟斤拷锟斤拷锟斤拷锟疥到NDC锟斤拷锟斤拷锟节伙拷锟斤拷锟斤拷锟斤拷傻牟锟斤拷锟斤拷锟角ｏ拷锟斤拷锟斤拷锟斤拷center锟戒化锟斤拷锟斤拷锟狡讹拷锟斤拷
static Point2D APR_WorldToNDC_Fixed(const MeasurementPoint& world, APRContext* ctx,
                                     float fixedCenterX, float fixedCenterY, float fixedCenterZ) {
    if (!ctx || !ctx->volume) return {0, 0};
    
    auto vol = static_cast<VolumeContext*>(ctx->volume);
    float halfWidth = ctx->sliceWidth * 0.5f / ctx->zoomFactor;
    float halfHeight = ctx->sliceHeight * 0.5f / ctx->zoomFactor;
    
    Point2D ndc = {0, 0};
    
    switch (ctx->sliceDirection) {
        case 0: // Axial
            ndc.x = (world.x - fixedCenterX) / (halfWidth * vol->spacingX);
            ndc.y = (world.y - fixedCenterY) / (halfHeight * vol->spacingY);
            break;
        case 1: // Coronal
            ndc.x = (world.x - fixedCenterX) / (halfWidth * vol->spacingX);
            ndc.y = -(world.z - fixedCenterZ) / (halfHeight * vol->spacingZ);
            break;
        case 2: // Sagittal
            ndc.x = (world.y - fixedCenterY) / (halfWidth * vol->spacingY);
            ndc.y = -(world.z - fixedCenterZ) / (halfHeight * vol->spacingZ);
            break;
    }
    
    return ndc;
}


APRHandle APR_CropVolume(APRHandle sourceHandle) {
    if (!sourceHandle) {
        printf("[Crop] Error: sourceHandle is null\n");
        return nullptr;
    }
    
    auto srcCtx = static_cast<APRContext*>(sourceHandle);
    if (!srcCtx->volume) {
        printf("[Crop] Error: volume is null\n");
        return nullptr;
    }
    
    // Get sessionId from source handle for per-session storage
    std::string cropSessionId = srcCtx->sessionId;
    
    // Try to get TabSessionContext for this session
    TabSessionContext* tabCtx = !cropSessionId.empty() ? FindTabSession(cropSessionId) : nullptr;
    
    // Destroy previous cropped APR for this session
    if (tabCtx && tabCtx->croppedAPR) {
        APR_Destroy(tabCtx->croppedAPR);
        tabCtx->croppedAPR = nullptr;
    }
    // Also clean up from legacy storage
    auto cropIt = g_sessionCroppedAPRs.find(cropSessionId);
    if (cropIt != g_sessionCroppedAPRs.end() && cropIt->second) {
        if (!tabCtx || tabCtx->croppedAPR != cropIt->second) {
            APR_Destroy(cropIt->second);
        }
        g_sessionCroppedAPRs.erase(cropIt);
    }
    if (g_lastCroppedAPR) {
        auto lastCtx = static_cast<APRContext*>(g_lastCroppedAPR);
        if (lastCtx->sessionId == cropSessionId) {
            g_lastCroppedAPR = nullptr;
        }
    }
    
    // 锟斤拷取锟斤拷锟斤拷锟捷尺达拷
    int width, height, depth;
    Dicom_Volume_GetDimensions(srcCtx->volume, &width, &height, &depth);
    
    // Get crop box - prefer TabSessionContext, fallback to global
    GlobalAPRCropBox* cropBoxPtr = tabCtx ? &tabCtx->cropBox : &g_aprCropBox;
    int xStart = static_cast<int>(std::round(cropBoxPtr->xStart));
    int xEnd = static_cast<int>(std::round(cropBoxPtr->xEnd));
    int yStart = static_cast<int>(std::round(cropBoxPtr->yStart));
    int yEnd = static_cast<int>(std::round(cropBoxPtr->yEnd));
    int zStart = static_cast<int>(std::round(cropBoxPtr->zStart));
    int zEnd = static_cast<int>(std::round(cropBoxPtr->zEnd));
    
    // 确锟斤拷锟斤拷围锟较凤拷
    if (xStart < 0) xStart = 0;
    if (xStart >= width) xStart = width - 1;
    if (xEnd > width) xEnd = width;
    if (xEnd <= xStart) xEnd = xStart + 1;
    
    if (yStart < 0) yStart = 0;
    if (yStart >= height) yStart = height - 1;
    if (yEnd > height) yEnd = height;
    if (yEnd <= yStart) yEnd = yStart + 1;
    
    if (zStart < 0) zStart = 0;
    if (zStart >= depth) zStart = depth - 1;
    if (zEnd > depth) zEnd = depth;
    if (zEnd <= zStart) zEnd = zStart + 1;
    
    // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟捷尺达拷
    int newWidth = xEnd - xStart;
    int newHeight = yEnd - yStart;
    int newDepth = zEnd - zStart;
    
    // 锟斤拷锟斤拷锟铰碉拷锟斤拷锟斤拷锟斤拷
    auto newVol = new VolumeContext();
    newVol->width = newWidth;
    newVol->height = newHeight;
    newVol->depth = newDepth;
    newVol->data.resize(newWidth * newHeight * newDepth);
    
    // 锟斤拷取spacing锟斤拷origin锟斤拷使锟斤拷DICOM API锟斤拷
    float spacing[3];
    Dicom_Volume_GetSpacing(srcCtx->volume, &spacing[0], &spacing[1], &spacing[2]);
    newVol->spacingX = spacing[0];
    newVol->spacingY = spacing[1];
    newVol->spacingZ = spacing[2];

    
    // Copy rescale parameters from source volume
    float rescaleSlope = 1.0f, rescaleIntercept = 0.0f;
    Dicom_Volume_GetRescale(srcCtx->volume, &rescaleSlope, &rescaleIntercept);
    newVol->rescaleSlope = rescaleSlope;
    newVol->rescaleIntercept = rescaleIntercept;
    newVol->originX = 0.0f;  // origin锟斤拷时锟斤拷为0
    newVol->originY = 0.0f;
    newVol->originZ = 0.0f;
    
    printf("[Crop] New volume will be: %d x %d x %d\n", newWidth, newHeight, newDepth);
    
    // 锟斤拷取源锟斤拷锟斤拷锟斤拷指锟斤拷
    uint16_t* srcData = static_cast<uint16_t*>(Dicom_Volume_GetData(srcCtx->volume));
    if (!srcData) {
        printf("[Crop] Error: cannot get source volume data\n");
        delete newVol;
        return nullptr;
    }
    
    // 锟斤拷锟狡诧拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷荩锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷转锟斤拷
    // Use the authoritative world rotation matrix.
    const float* rm = srcCtx->rotMat;
    
    // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
    float centerX = width * 0.5f;
    float centerY = height * 0.5f;
    float centerZ = depth * 0.5f;
    
    float dbgX = 0.0f, dbgY = 0.0f, dbgZ = 0.0f;
    Mat4_ExtractEulerZYXDeg(srcCtx->rotMat, &dbgX, &dbgY, &dbgZ);
    printf("[Crop] Copying data with rotation (%.1f, %.1f, %.1f)...\n", dbgX, dbgY, dbgZ);
    
    // 统锟斤拷锟斤拷锟斤拷
    int validSamples = 0;
    uint16_t minVal = 65535, maxVal = 0;
    
    // 锟斤拷锟斤拷锟斤拷锟斤拷
    for (int z = 0; z < newDepth; z++) {
        for (int y = 0; y < newHeight; y++) {
            for (int x = 0; x < newWidth; x++) {
                // 锟斤拷锟斤拷锟斤拷锟斤拷
                float vx = static_cast<float>(xStart + x);
                float vy = static_cast<float>(yStart + y);
                float vz = static_cast<float>(zStart + z);
                
                // 转锟斤拷锟斤拷锟斤拷锟斤拷原锟斤拷
                float dx = vx - centerX;
                float dy = vy - centerY;
                float dz = vz - centerZ;
                
                // Apply world rotation about volume center.
                float v[3] = { dx, dy, dz };
                float v2[3] = { 0.0f, 0.0f, 0.0f };
                Mat4_MulVec3_3x3(rm, v, v2);
                float rx = v2[0] + centerX;
                float ry = v2[1] + centerY;
                float rz = v2[2] + centerZ;
                
                // 锟斤拷锟斤拷
                int ix = static_cast<int>(std::round(rx));
                int iy = static_cast<int>(std::round(ry));
                int iz = static_cast<int>(std::round(rz));
                
                uint16_t value = 0;
                if (ix >= 0 && ix < width &&
                    iy >= 0 && iy < height &&
                    iz >= 0 && iz < depth) {
                    value = srcData[iz * width * height + iy * width + ix];
                    validSamples++;
                    if (value < minVal) minVal = value;
                    if (value > maxVal) maxVal = value;
                }
                
                newVol->data[z * newWidth * newHeight + y * newWidth + x] = value;
            }
        }
    }
    

    // 锟斤拷锟斤拷锟铰碉拷APR锟斤拷锟斤拷锟斤拷
    auto newCtx = new APRContext();
    newCtx->volume = nullptr;  // 锟斤拷使锟斤拷DICOM volume
    newCtx->croppedVolumeData = newVol;  // 使锟矫诧拷锟叫猴拷锟斤拷锟斤拷锟?
    newCtx->sliceDirection = srcCtx->sliceDirection;
    newCtx->centerX = static_cast<float>(newWidth) * 0.5f;
    newCtx->centerY = static_cast<float>(newHeight) * 0.5f;
    newCtx->centerZ = static_cast<float>(newDepth) * 0.5f;
    Mat4_Identity(newCtx->rotMat);
    newCtx->showCrossHair = true;
    newCtx->zoomFactor = 1.0f;
    
    // 锟斤拷锟斤拷OpenGL锟斤拷锟斤拷
    glGenTextures(1, &newCtx->textureID);
    
    // Save to session-specific storage
    newCtx->sessionId = cropSessionId;
    
    // Store in TabSessionContext (new architecture) - tabCtx was already obtained above
    if (tabCtx) {
        tabCtx->croppedAPR = newCtx;
    }
    
    // Also store in legacy storage for backward compatibility
    g_sessionCroppedAPRs[cropSessionId] = newCtx;
    g_lastCroppedAPR = newCtx;
    
    printf("[Crop] APR context created successfully!\n");
    
    return newCtx;
}

APRHandle APR_GetLastCroppedVolume() {
    return g_lastCroppedAPR;
}

void APR_GetCroppedVolumeDimensions(int* width, int* height, int* depth) {
    if (width) *width = 0;
    if (height) *height = 0;
    if (depth) *depth = 0;
    
    if (!g_lastCroppedAPR) return;
    
    auto ctx = static_cast<APRContext*>(g_lastCroppedAPR);
    if (ctx->croppedVolumeData) {
        if (width) *width = ctx->croppedVolumeData->width;
        if (height) *height = ctx->croppedVolumeData->height;
        if (depth) *depth = ctx->croppedVolumeData->depth;
    }
}

void APR_GetCroppedVolumeSpacing(float* spacingX, float* spacingY, float* spacingZ) {
    if (spacingX) *spacingX = 1.0f;
    if (spacingY) *spacingY = 1.0f;
    if (spacingZ) *spacingZ = 1.0f;
    
    if (!g_lastCroppedAPR) return;
    
    auto ctx = static_cast<APRContext*>(g_lastCroppedAPR);
    if (ctx->croppedVolumeData) {
        if (spacingX) *spacingX = ctx->croppedVolumeData->spacingX;
        if (spacingY) *spacingY = ctx->croppedVolumeData->spacingY;
        if (spacingZ) *spacingZ = ctx->croppedVolumeData->spacingZ;
    }
}

// Session-aware version: apply cropped volume for specific session
int APR_ApplyCroppedVolumeForSession(const char* sessionId) {
    if (!sessionId || sessionId[0] == '\0') {
        printf("[ApplyCroppedVolume] Error: sessionId is null or empty\n");
        return 0;
    }
    
    std::string sessId(sessionId);
    
    // First try to get from TabSessionContext (new architecture)
    TabSessionContext* tabCtx = FindTabSession(sessId);
    APRHandle croppedHandle = nullptr;
    
    if (tabCtx && tabCtx->croppedAPR) {
        croppedHandle = tabCtx->croppedAPR;
    } else {
        // Fallback to legacy storage
        auto it = g_sessionCroppedAPRs.find(sessId);
        if (it != g_sessionCroppedAPRs.end() && it->second) {
            croppedHandle = it->second;
        } else if (g_lastCroppedAPR) {
            auto lastCtx = static_cast<APRContext*>(g_lastCroppedAPR);
            if (lastCtx->sessionId == sessId) {
                croppedHandle = g_lastCroppedAPR;
            }
        }
    }
    
    if (!croppedHandle) {
        printf("[ApplyCroppedVolume] Error: No cropped volume for session %s\n", sessionId);
        return 0;
    }
    
    auto croppedCtx = static_cast<APRContext*>(croppedHandle);
    if (!croppedCtx->croppedVolumeData) {
        printf("[ApplyCroppedVolume] Error: Cropped volume data is null\n");
        return 0;
    }
    
    auto croppedVol = croppedCtx->croppedVolumeData;
    VolumeHandle newVolume = static_cast<VolumeHandle>(croppedVol);
    
    printf("[ApplyCroppedVolume] Applying cropped volume for session %s: %d x %d x %d\n",
           sessionId, croppedVol->width, croppedVol->height, croppedVol->depth);
    
    // Use TabSessionContext's linkedAPRs if available, otherwise fallback to global
    std::vector<APRContext*> sessionAPRs;
    if (tabCtx) {
        for (APRHandle apr : tabCtx->linkedAPRs) {
            auto ctx = static_cast<APRContext*>(apr);
            if (ctx) {
                sessionAPRs.push_back(ctx);
            }
        }
    } else {
        // Fallback: find APRs from global list
        for (APRHandle apr : g_globalAPRCenter.linkedAPRs) {
            auto ctx = static_cast<APRContext*>(apr);
            if (ctx && ctx->sessionId == sessId) {
                sessionAPRs.push_back(ctx);
            }
        }
    }
    
    // Replace volume for all session APRs
    for (auto ctx : sessionAPRs) {
        ctx->volume = newVolume;
        ctx->croppedVolumeData = nullptr;
        ctx->ownsVolumeData = false;
        
        ctx->centerX = croppedVol->width * 0.5f;
        ctx->centerY = croppedVol->height * 0.5f;
        ctx->centerZ = croppedVol->depth * 0.5f;
        
        Mat4_Identity(ctx->rotMat);
    }
    
    // Update TabSessionContext's aprCenter
    if (tabCtx) {
        tabCtx->aprCenter.x = croppedVol->width * 0.5f;
        tabCtx->aprCenter.y = croppedVol->height * 0.5f;
        tabCtx->aprCenter.z = croppedVol->depth * 0.5f;
        tabCtx->aprCenter.volume = newVolume;
        Mat4_Identity(tabCtx->aprCenter.rotMat);
        tabCtx->cropBox.enabled = false;
    }
    
    // Also update legacy global center for backward compatibility
    if (g_globalAPRCenter.volume) {
        g_globalAPRCenter.x = croppedVol->width * 0.5f;
        g_globalAPRCenter.y = croppedVol->height * 0.5f;
        g_globalAPRCenter.z = croppedVol->depth * 0.5f;
        g_globalAPRCenter.volume = newVolume;
        Mat4_Identity(g_globalAPRCenter.rotMat);
    }
    
    g_aprCropBox.enabled = false;
    
    // Clear ownership from cropped context
    croppedCtx->croppedVolumeData = nullptr;
    croppedCtx->ownsVolumeData = false;
    
    printf("[ApplyCroppedVolume] Volume replaced for %zu APRs in session %s\n", 
           sessionAPRs.size(), sessionId);
    
    return 1;
}

// Legacy version for backward compatibility
int APR_ApplyCroppedVolume() {
    // 鑾峰彇鏈€杩戜竴娆¤鍒囩殑缁撴灉
    if (!g_lastCroppedAPR) {
        printf("[ApplyCroppedVolume] Error: No cropped volume available\n");
        return 0;
    }
    
    auto croppedCtx = static_cast<APRContext*>(g_lastCroppedAPR);
    if (!croppedCtx->croppedVolumeData) {
        printf("[ApplyCroppedVolume] Error: Cropped volume data is null\n");
        return 0;
    }
    
    auto croppedVol = croppedCtx->croppedVolumeData;
    VolumeHandle newVolume = static_cast<VolumeHandle>(croppedVol);
    
    printf("[ApplyCroppedVolume] Applying cropped volume: %d x %d x %d\n",
           croppedVol->width, croppedVol->height, croppedVol->depth);
    
    // 鑾峰彇鎵€鏈夊叧鑱旂殑APR
    std::vector<APRContext*> linkedAPRs;
    if (!g_globalAPRCenter.linkedAPRs.empty()) {
        for (APRHandle apr : g_globalAPRCenter.linkedAPRs) {
            linkedAPRs.push_back(static_cast<APRContext*>(apr));
        }
    }
    
    // 鏇挎崲鎵€鏈堿PR鐨剉olume
    for (auto ctx : linkedAPRs) {
        ctx->volume = newVolume;
        ctx->croppedVolumeData = nullptr;
        ctx->ownsVolumeData = false;
        
        // 閲嶇疆涓績鐐瑰埌鏂皏olume涓績
        ctx->centerX = croppedVol->width * 0.5f;
        ctx->centerY = croppedVol->height * 0.5f;
        ctx->centerZ = croppedVol->depth * 0.5f;
        
        // Reset rotation
        Mat4_Identity(ctx->rotMat);
    }
    
    // 鏇存柊鍏ㄥ眬涓績鐐?
    g_globalAPRCenter.x = croppedVol->width * 0.5f;
    g_globalAPRCenter.y = croppedVol->height * 0.5f;
    g_globalAPRCenter.z = croppedVol->depth * 0.5f;
    g_globalAPRCenter.volume = newVolume;
    Mat4_Identity(g_globalAPRCenter.rotMat);
    
    // 绂佺敤瑁佸垏妗?
    g_aprCropBox.enabled = false;
    
    // 娓呴櫎cropped handle鐨勬墍鏈夋潈
    croppedCtx->croppedVolumeData = nullptr;
    croppedCtx->ownsVolumeData = false;
    
    printf("[ApplyCroppedVolume] Volume replaced! New size: %d x %d x %d\n",
           croppedVol->width, croppedVol->height, croppedVol->depth);
    printf("[ApplyCroppedVolume] Crop box disabled. Press B to re-enable.\n");
    
    return 1;
}

void APR_Destroy(APRHandle handle) {
    if (handle) {
        auto ctx = static_cast<APRContext*>(handle);
        if (ctx->textureID) {
            glDeleteTextures(1, &ctx->textureID);
        }
        // 只锟斤拷拥锟斤拷锟斤拷锟斤拷时锟斤拷删锟斤拷锟斤拷锟叫猴拷锟斤拷锟斤拷锟斤拷锟?
        if (ctx->croppedVolumeData && ctx->ownsVolumeData) {
            delete ctx->croppedVolumeData;
        }
        delete ctx;
        // 锟斤拷锟缴撅拷锟斤拷锟斤拷遣锟斤拷薪锟斤拷锟斤拷锟斤拷锟饺拷直锟斤拷锟?
        if (handle == g_lastCroppedAPR) {
            g_lastCroppedAPR = nullptr;
        }
    }
}

NativeResult APR_SetVolume(APRHandle handle, VolumeHandle volume) {
    if (!handle) return NATIVE_E_INVALID_ARGUMENT;
    static_cast<APRContext*>(handle)->volume = volume;
    return NATIVE_OK;
}

NativeResult APR_GetVolume(APRHandle handle, VolumeHandle* outVolume) {
    if (!handle || !outVolume) return NATIVE_E_INVALID_ARGUMENT;
    *outVolume = static_cast<APRContext*>(handle)->volume;
    return NATIVE_OK;
}

void APR_SetSliceDirection(APRHandle handle, int direction) {
    if (!handle) return;
    auto ctx = static_cast<APRContext*>(handle);
    // 锟斤拷锟狡凤拷锟斤拷围 0-2
    if (direction < 0) direction = 0;
    if (direction > 2) direction = 2;
    ctx->sliceDirection = direction;
}

int APR_GetSliceDirection(APRHandle handle) {
    if (!handle) return 1;  // 默锟斤拷 Coronal
    return static_cast<APRContext*>(handle)->sliceDirection;
}

void APR_SetCenter(APRHandle handle, float x, float y, float z) {
    if (!handle) return;
    auto ctx = static_cast<APRContext*>(handle);
    
    ctx->centerX = x; ctx->centerY = y; ctx->centerZ = z;
    
    // 锟斤拷位锟竭碉拷UV锟斤拷锟斤拷锟斤拷莸锟角帮拷锟斤拷锟斤拷锟斤拷茫锟斤拷锟斤拷平锟斤拷锟斤拷锟疥）
    // 默锟较讹拷位锟斤拷锟斤拷锟斤拷片锟斤拷锟斤拷位锟矫ｏ拷未锟斤拷转锟斤拷锟斤拷系锟斤拷
    if (ctx->sliceDirection == 0) {  // Axial: U=X, V=Y
        ctx->crosshairU = x;
        ctx->crosshairV = y;
    } else if (ctx->sliceDirection == 1) {  // Coronal: U=X, V=Z
        ctx->crosshairU = x;
        ctx->crosshairV = z;
    } else if (ctx->sliceDirection == 2) {  // Sagittal: U=Y, V=Z
        ctx->crosshairU = y;
        ctx->crosshairV = z;
    }
    
    // Session-scoped APR center replaces legacy global state
    GlobalAPRCenter* aprCenter = GetSessionAPRCenter(ctx->sessionId);
    aprCenter->x = x;
    aprCenter->y = y;
    aprCenter->z = z;
    aprCenter->volume = ctx->volume;
    std::memcpy(aprCenter->rotMat, ctx->rotMat, sizeof(float) * 16);
    
    // 同步同 tab/session 内关联的 APR
    for (APRHandle linkedAPR : aprCenter->linkedAPRs) {
        if (linkedAPR != handle) {
            auto linkedCtx = static_cast<APRContext*>(linkedAPR);

            linkedCtx->centerX = x;
            linkedCtx->centerY = y;
            linkedCtx->centerZ = z;
            
            if (linkedCtx->sliceDirection == 0) {  // Axial: U=X, V=Y
                linkedCtx->crosshairU = x;
                linkedCtx->crosshairV = y;
            } else if (linkedCtx->sliceDirection == 1) {  // Coronal: U=X, V=Z
                linkedCtx->crosshairU = x;
                linkedCtx->crosshairV = z;
            } else if (linkedCtx->sliceDirection == 2) {  // Sagittal: U=Y, V=Z
                linkedCtx->crosshairU = y;
                linkedCtx->crosshairV = z;
            }
        }
    }
}

void APR_GetCenter(APRHandle handle, float* x, float* y, float* z) {
    if (!handle) return;
    auto ctx = static_cast<APRContext*>(handle);
    if (x) *x = ctx->centerX;
    if (y) *y = ctx->centerY;
    if (z) *z = ctx->centerZ;
}

void APR_LinkCenter(APRHandle* handles, int count) {
    if (!handles || count <= 0) return;
    
    // Get sessionId from first handle to determine which session's APR center to use
    std::string sessionId;
    if (handles[0]) {
        auto firstCtx = static_cast<APRContext*>(handles[0]);
        sessionId = firstCtx->sessionId;
    }
    
    // Get session-specific APR center (or global fallback)
    GlobalAPRCenter* aprCenter = GetSessionAPRCenter(sessionId);
    TabSessionContext* tabCtx = !sessionId.empty() ? FindTabSession(sessionId) : nullptr;
    
    // Clear previous linked APRs
    aprCenter->linkedAPRs.clear();
    if (tabCtx) {
        tabCtx->linkedAPRs.clear();
    }
    
    // Add all APRs to the linked list
    for (int i = 0; i < count; ++i) {
        if (handles[i]) {
            aprCenter->linkedAPRs.push_back(handles[i]);
            if (tabCtx) {
                tabCtx->linkedAPRs.push_back(handles[i]);
            }
            
            // Use first handle's center and rotation as initial values
            if (i == 0) {
                auto ctx = static_cast<APRContext*>(handles[i]);
                aprCenter->x = ctx->centerX;
                aprCenter->y = ctx->centerY;
                aprCenter->z = ctx->centerZ;
                aprCenter->volume = ctx->volume;
                std::memcpy(aprCenter->rotMat, ctx->rotMat, sizeof(float) * 16);
            }
        }
    }
    
    // Sync all APRs to the shared center
    for (APRHandle apr : aprCenter->linkedAPRs) {
        auto ctx = static_cast<APRContext*>(apr);
        ctx->centerX = aprCenter->x;
        ctx->centerY = aprCenter->y;
        ctx->centerZ = aprCenter->z;
        std::memcpy(ctx->rotMat, aprCenter->rotMat, sizeof(float) * 16);
    }
}

void APR_SetRotation(APRHandle handle, float angleX, float angleY, float angleZ) {
    if (!handle) return;
    auto ctx = static_cast<APRContext*>(handle);

    // Authoritative rotation is a matrix: R = Rz * Ry * Rx
    float rx[16];
    float ry[16];
    float rz[16];
    Mat4_RotationAxisAngle(rx, 1.0f, 0.0f, 0.0f, angleX);
    Mat4_RotationAxisAngle(ry, 0.0f, 1.0f, 0.0f, angleY);
    Mat4_RotationAxisAngle(rz, 0.0f, 0.0f, 1.0f, angleZ);
    float tmp[16];
    Mat4_Mul(tmp, ry, rx);
    Mat4_Mul(ctx->rotMat, rz, tmp);
    Mat4_Orthonormalize3x3(ctx->rotMat);

    // Get session-specific APR center
    GlobalAPRCenter* aprCenter = GetSessionAPRCenter(ctx->sessionId);
    std::memcpy(aprCenter->rotMat, ctx->rotMat, sizeof(float) * 16);
    
    // Sync all linked APRs in this session
    for (APRHandle linkedAPR : aprCenter->linkedAPRs) {
        if (linkedAPR != handle) {
            auto linkedCtx = static_cast<APRContext*>(linkedAPR);
            std::memcpy(linkedCtx->rotMat, ctx->rotMat, sizeof(float) * 16);
        }
    }
}

void APR_GetRotation(APRHandle handle, float* angleX, float* angleY, float* angleZ) {
    if (!handle) return;
    auto ctx = static_cast<APRContext*>(handle);
    float x = 0.0f, y = 0.0f, z = 0.0f;
    Mat4_ExtractEulerZYXDeg(ctx->rotMat, &x, &y, &z);
    if (angleX) *angleX = x;
    if (angleY) *angleY = y;
    if (angleZ) *angleZ = z;
}

void* APR_GetSlice(APRHandle handle, int direction, int* width, int* height) {
    if (!handle) return nullptr;
    auto ctx = static_cast<APRContext*>(handle);
    
    // 根据方向返回对应的切片尺寸和displayBuffer
    if (!ctx->volume) return nullptr;
    
    int volWidth, volHeight, volDepth;
    if (Dicom_Volume_GetDimensions(ctx->volume, &volWidth, &volHeight, &volDepth) != NATIVE_OK) {
        return nullptr;
    }
    
    // 确定切片尺寸
    switch (direction) {
        case 0:  // Axial (XY平面)
            if (width) *width = volWidth;
            if (height) *height = volHeight;
            break;
        case 1:  // Coronal (XZ平面)
            if (width) *width = volWidth;
            if (height) *height = volDepth;
            break;
        case 2:  // Sagittal (YZ平面)
            if (width) *width = volHeight;
            if (height) *height = volDepth;
            break;
        default:
            if (width) *width = 0;
            if (height) *height = 0;
            return nullptr;
    }
    
    // 返回displayBuffer（APR_Render/APR_UpdateSlice会填充这个buffer）
    void* result = ctx->displayBuffer.empty() ? nullptr : ctx->displayBuffer.data();
    
    // Debug log
    FILE* logFile = nullptr;
    fopen_s(&logFile, "D:\\3d_render_debug.log", "a");
    if (logFile) {
        fprintf(logFile, "[APR_GetSlice] handle=%p dir=%d bufferSize=%zu result=%p\n", 
                handle, direction, ctx->displayBuffer.size(), result);
        fclose(logFile);
    }
    
    return result;
}

void APR_SetShowCrossHair(APRHandle handle, bool show) {
    if (!handle) return;
    static_cast<APRContext*>(handle)->showCrossHair = show;
}

bool APR_GetShowCrossHair(APRHandle handle) {
    if (!handle) return false;
    return static_cast<APRContext*>(handle)->showCrossHair;
}

// ============ 锟斤拷锟叫匡拷 API ============

void APR_SetCropBox(int volumeWidth, int volumeHeight, int volumeDepth) {
    // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟侥筹拷始锟斤拷锟斤拷锟叫匡拷占锟斤拷锟斤拷锟捷碉拷50%锟斤拷
    float centerX = volumeWidth / 2.0f;
    float centerY = volumeHeight / 2.0f;
    float centerZ = volumeDepth / 2.0f;
    
    float halfSizeX = volumeWidth * 0.25f;
    float halfSizeY = volumeHeight * 0.25f;
    float halfSizeZ = volumeDepth * 0.25f;
    
    g_aprCropBox.xStart = centerX - halfSizeX;
    g_aprCropBox.xEnd = centerX + halfSizeX;
    g_aprCropBox.yStart = centerY - halfSizeY;
    g_aprCropBox.yEnd = centerY + halfSizeY;
    g_aprCropBox.zStart = centerZ - halfSizeZ;
    g_aprCropBox.zEnd = centerZ + halfSizeZ;
}

void APR_GetCropBox(float* xStart, float* xEnd, float* yStart, float* yEnd, float* zStart, float* zEnd) {
    if (xStart) *xStart = g_aprCropBox.xStart;
    if (xEnd) *xEnd = g_aprCropBox.xEnd;
    if (yStart) *yStart = g_aprCropBox.yStart;
    if (yEnd) *yEnd = g_aprCropBox.yEnd;
    if (zStart) *zStart = g_aprCropBox.zStart;
    if (zEnd) *zEnd = g_aprCropBox.zEnd;
}

void APR_SetCropBoxRange(float xStart, float xEnd, float yStart, float yEnd, float zStart, float zEnd) {
    if (xStart <= xEnd) {
        g_aprCropBox.xStart = xStart;
        g_aprCropBox.xEnd = xEnd;
    } else {
        g_aprCropBox.xStart = xEnd;
        g_aprCropBox.xEnd = xStart;
    }

    if (yStart <= yEnd) {
        g_aprCropBox.yStart = yStart;
        g_aprCropBox.yEnd = yEnd;
    } else {
        g_aprCropBox.yStart = yEnd;
        g_aprCropBox.yEnd = yStart;
    }

    if (zStart <= zEnd) {
        g_aprCropBox.zStart = zStart;
        g_aprCropBox.zEnd = zEnd;
    } else {
        g_aprCropBox.zStart = zEnd;
        g_aprCropBox.zEnd = zStart;
    }
}

void APR_EnableCropBox(bool enable) {
    g_aprCropBox.enabled = enable;
}

bool APR_IsCropBoxEnabled() {
    return g_aprCropBox.enabled;
}

// ==================== Session-aware Crop Box APIs ====================

void APR_SetCropBoxRangeForSession(const char* sessionId, float xStart, float xEnd, float yStart, float yEnd, float zStart, float zEnd) {
    GlobalAPRCropBox* cropBox = sessionId ? GetSessionCropBox(sessionId) : &g_aprCropBox;
    
    if (xStart <= xEnd) {
        cropBox->xStart = xStart;
        cropBox->xEnd = xEnd;
    } else {
        cropBox->xStart = xEnd;
        cropBox->xEnd = xStart;
    }
    if (yStart <= yEnd) {
        cropBox->yStart = yStart;
        cropBox->yEnd = yEnd;
    } else {
        cropBox->yStart = yEnd;
        cropBox->yEnd = yStart;
    }
    if (zStart <= zEnd) {
        cropBox->zStart = zStart;
        cropBox->zEnd = zEnd;
    } else {
        cropBox->zStart = zEnd;
        cropBox->zEnd = zStart;
    }
}

void APR_GetCropBoxForSession(const char* sessionId, float* xStart, float* xEnd, float* yStart, float* yEnd, float* zStart, float* zEnd) {
    GlobalAPRCropBox* cropBox = sessionId ? GetSessionCropBox(sessionId) : &g_aprCropBox;
    if (xStart) *xStart = cropBox->xStart;
    if (xEnd) *xEnd = cropBox->xEnd;
    if (yStart) *yStart = cropBox->yStart;
    if (yEnd) *yEnd = cropBox->yEnd;
    if (zStart) *zStart = cropBox->zStart;
    if (zEnd) *zEnd = cropBox->zEnd;
}

void APR_EnableCropBoxForSession(const char* sessionId, bool enable) {
    GlobalAPRCropBox* cropBox = sessionId ? GetSessionCropBox(sessionId) : &g_aprCropBox;
    cropBox->enabled = enable;
}

bool APR_IsCropBoxEnabledForSession(const char* sessionId) {
    GlobalAPRCropBox* cropBox = sessionId ? GetSessionCropBox(sessionId) : &g_aprCropBox;
    return cropBox->enabled;
}

void APR_SetCropShapeForSession(const char* sessionId, int shape) {
    GlobalAPRCropBox* cropBox = sessionId ? GetSessionCropBox(sessionId) : &g_aprCropBox;
    if (shape >= 0 && shape <= 2) {
        cropBox->shape = static_cast<CropShapeType>(shape);
    }
}

int APR_GetCropShapeForSession(const char* sessionId) {
    GlobalAPRCropBox* cropBox = sessionId ? GetSessionCropBox(sessionId) : &g_aprCropBox;
    return static_cast<int>(cropBox->shape);
}

// ==================== End Session-aware Crop Box APIs ====================

void APR_SetCropShape(int shape) {
    if (shape >= 0 && shape <= 2) {
        g_aprCropBox.shape = static_cast<CropShapeType>(shape);
    }
}

int APR_GetCropShape() {
    return static_cast<int>(g_aprCropBox.shape);
}

void APR_SetCropCylinderDirection(int direction) {
    if (direction >= 0 && direction <= 2) {
        g_aprCropBox.cylinderDirection = static_cast<CropCylinderDirection>(direction);
    }
}

int APR_GetCropCylinderDirection() {
    return static_cast<int>(g_aprCropBox.cylinderDirection);
}

void APR_SetCropBoxSize(int sizeX, int sizeY, int sizeZ, int volumeWidth, int volumeHeight, int volumeDepth) {
    // 璁＄畻褰撳墠瑁佸垏妗嗙殑涓績鐐?
    float centerX = (g_aprCropBox.xStart + g_aprCropBox.xEnd) / 2.0f;
    float centerY = (g_aprCropBox.yStart + g_aprCropBox.yEnd) / 2.0f;
    float centerZ = (g_aprCropBox.zStart + g_aprCropBox.zEnd) / 2.0f;
    
    // 濡傛灉瑁佸垏妗嗘湭鍒濆鍖栵紝浣跨敤浣撴暟鎹腑蹇?
    if (!g_aprCropBox.enabled) {
        centerX = volumeWidth / 2.0f;
        centerY = volumeHeight / 2.0f;
        centerZ = volumeDepth / 2.0f;
    }
    
    // 璁＄畻鍗婂昂瀵?
    float halfX = sizeX / 2.0f;
    float halfY = sizeY / 2.0f;
    float halfZ = sizeZ / 2.0f;
    
    // 璁剧疆鏂扮殑瑁佸垏妗嗚寖鍥?
    g_aprCropBox.xStart = centerX - halfX;
    g_aprCropBox.xEnd = centerX + halfX;
    g_aprCropBox.yStart = centerY - halfY;
    g_aprCropBox.yEnd = centerY + halfY;
    g_aprCropBox.zStart = centerZ - halfZ;
    g_aprCropBox.zEnd = centerZ + halfZ;
    
    // 绾︽潫鍒颁綋鏁版嵁鑼冨洿鍐?
    if (g_aprCropBox.xStart < 0) g_aprCropBox.xStart = 0;
    if (g_aprCropBox.yStart < 0) g_aprCropBox.yStart = 0;
    if (g_aprCropBox.zStart < 0) g_aprCropBox.zStart = 0;
    if (g_aprCropBox.xEnd > volumeWidth - 1) g_aprCropBox.xEnd = (float)(volumeWidth - 1);
    if (g_aprCropBox.yEnd > volumeHeight - 1) g_aprCropBox.yEnd = (float)(volumeHeight - 1);
    if (g_aprCropBox.zEnd > volumeDepth - 1) g_aprCropBox.zEnd = (float)(volumeDepth - 1);
}

void APR_SetZoom(APRHandle handle, float zoomFactor) {
    if (!handle) return;
    auto ctx = static_cast<APRContext*>(handle);
    // 锟斤拷锟斤拷锟斤拷锟脚凤拷围 0.1 ~ 10.0
    if (zoomFactor < 0.1f) zoomFactor = 0.1f;
    if (zoomFactor > 10.0f) zoomFactor = 10.0f;
    ctx->zoomFactor = zoomFactor;
}

float APR_GetZoom(APRHandle handle) {
    if (!handle) return 1.0f;
    return static_cast<APRContext*>(handle)->zoomFactor;
}

void APR_SetWindowLevel(APRHandle handle, float windowWidth, float windowLevel) {
    if (!handle) return;
    auto ctx = static_cast<APRContext*>(handle);
    if (windowWidth < 1.0f) windowWidth = 1.0f;
    ctx->windowWidthHU = windowWidth;
    ctx->windowLevelHU = windowLevel;
}

void APR_GetWindowLevel(APRHandle handle, float* windowWidth, float* windowLevel) {
    if (!handle) return;
    auto ctx = static_cast<APRContext*>(handle);
    if (windowWidth) *windowWidth = ctx->windowWidthHU;
    if (windowLevel) *windowLevel = ctx->windowLevelHU;
}

void APR_SetProjectionMode(APRHandle handle, int mode, float thickness) {
    if (!handle) return;
    auto ctx = static_cast<APRContext*>(handle);
    // mode: 0=Normal, 1=MIP, 2=MinIP
    if (mode < 0 || mode > 2) mode = 0;
    ctx->projectionMode = mode;
    if (thickness < 1.0f) thickness = 1.0f;
    if (thickness > 200.0f) thickness = 200.0f;
    ctx->projectionThickness = thickness;
}

void APR_GetProjectionMode(APRHandle handle, int* mode, float* thickness) {
    if (!handle) return;
    auto ctx = static_cast<APRContext*>(handle);
    if (mode) *mode = ctx->projectionMode;
    if (thickness) *thickness = ctx->projectionThickness;
}

void APR_ResetRotation(APRHandle handle) {
    if (!handle) return;
    auto ctx = static_cast<APRContext*>(handle);
    Mat4_Identity(ctx->rotMat);

    std::memcpy(g_globalAPRCenter.rotMat, ctx->rotMat, sizeof(float) * 16);
    for (APRHandle linkedAPR : g_globalAPRCenter.linkedAPRs) {
        if (linkedAPR != handle) {
            auto linkedCtx = static_cast<APRContext*>(linkedAPR);
            std::memcpy(linkedCtx->rotMat, ctx->rotMat, sizeof(float) * 16);
        }
    }
}

void APR_SetSessionId(APRHandle handle, const char* sessionId) {
    if (!handle) return;
    auto ctx = static_cast<APRContext*>(handle);
    std::string oldSessionId = ctx->sessionId;
    ctx->sessionId = sessionId ? sessionId : "";
    
    // Register with TabSessionContext
    if (!ctx->sessionId.empty()) {
        TabSessionContext* tabCtx = GetTabSession(ctx->sessionId);
        if (tabCtx) {
            // Add to linked APRs if not already present
            auto& aprs = tabCtx->linkedAPRs;
            if (std::find(aprs.begin(), aprs.end(), handle) == aprs.end()) {
                aprs.push_back(handle);
            }
        }
    }
    
    // Remove from old session if different
    if (!oldSessionId.empty() && oldSessionId != ctx->sessionId) {
        TabSessionContext* oldTabCtx = FindTabSession(oldSessionId);
        if (oldTabCtx) {
            auto& aprs = oldTabCtx->linkedAPRs;
            aprs.erase(std::remove(aprs.begin(), aprs.end(), handle), aprs.end());
        }
    }
}

// APR_UpdateSlice: Update the slice data in displayBuffer without OpenGL rendering
// This is used by 3D orthogonal renderer to get slice textures
NativeResult APR_UpdateSlice(APRHandle handle) {
    // For now, just call APR_Render which also updates displayBuffer
    return APR_Render(handle);
}

NativeResult APR_Render(APRHandle handle) {
    if (!handle) return NATIVE_E_INVALID_ARGUMENT;
    auto ctx = static_cast<APRContext*>(handle);
    if (!ctx->volume) return NATIVE_E_INVALID_ARGUMENT;

    float rescaleSlope = 1.0f;
    float rescaleIntercept = 0.0f;
    (void)Dicom_Volume_GetRescale(ctx->volume, &rescaleSlope, &rescaleIntercept);
    
    // ===== 锟竭程帮拷全锟斤拷锟斤拷锟斤拷锟斤拷锟揭绑定碉拷 WindowContext =====
    WindowContext* windowCtx = nullptr;
    for (auto winHandle : g_AllWindows) {
        auto win = static_cast<WindowContext*>(winHandle);
        if (win && win->boundRenderer == handle) {
            windowCtx = win;
            break;
        }
    }
    
    // 锟斤拷锟斤拷业锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷模锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
    std::unique_ptr<std::lock_guard<std::mutex>> lock;
    if (windowCtx) {
        lock = std::make_unique<std::lock_guard<std::mutex>>(windowCtx->renderMutex);
    }
    
    // 锟斤拷取 Volume 锟竭寸、锟斤拷锟捷和硷拷锟?
    int width, height, depth;
    float spacingX, spacingY, spacingZ;
    
    if (Dicom_Volume_GetDimensions(ctx->volume, &width, &height, &depth) != NATIVE_OK) {
        SetLastError("Failed to get volume dimensions");
        return NATIVE_E_INTERNAL_ERROR;
    }
    
    if (Dicom_Volume_GetSpacing(ctx->volume, &spacingX, &spacingY, &spacingZ) != NATIVE_OK) {
        SetLastError("Failed to get volume spacing");
        return NATIVE_E_INTERNAL_ERROR;
    }
    
    uint16_t* volumeData = static_cast<uint16_t*>(Dicom_Volume_GetData(ctx->volume));
    if (!volumeData) {
        SetLastError("Volume data is null");
        return NATIVE_E_INTERNAL_ERROR;
    }
    
    // 锟斤拷转锟斤拷锟侥ｏ拷锟斤拷锟斤拷锟斤拷锟斤拷锟侥ｏ拷锟教讹拷锟斤拷锟戒）
    float rotationCenterX = width / 2.0f;
    float rotationCenterY = height / 2.0f;
    float rotationCenterZ = depth / 2.0f;
    
    // 锟斤拷片锟斤拷锟侥ｏ拷锟矫伙拷锟斤拷锟狡讹拷锟侥观诧拷悖?
    float sliceCenterX = ctx->centerX;
    float sliceCenterY = ctx->centerY;
    float sliceCenterZ = ctx->centerZ;
    
    // 锟斤拷锟斤拷锟斤拷片锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟竭达拷锟斤拷锟饺★拷呒锟?
    int outWidth = 0, outHeight = 0;
    float crossTexX = 0, crossTexY = 0;
    const char* directionName = "";
    
    // 锟斤拷锟捷凤拷锟斤拷确锟斤拷锟斤拷锟斤拷叽锟?
    switch (ctx->sliceDirection) {
        case 0:  // Axial (XY 平锟斤拷, Z 锟斤拷片)
            outWidth = width;
            outHeight = height;
            directionName = "Axial";
            break;
        case 1:  // Coronal (XZ 平锟斤拷, Y 锟斤拷片)
            outWidth = width;
            outHeight = depth;
            directionName = "Coronal";
            break;
        case 2:  // Sagittal (YZ 平锟斤拷, X 锟斤拷片)
            outWidth = height;
            outHeight = depth;
            directionName = "Sagittal";
            break;
        default:
            ctx->sliceDirection = 1;  // 默锟斤拷 Coronal
            outWidth = width;
            outHeight = depth;
            directionName = "Coronal";
            break;
    }
    
    size_t sliceSize = static_cast<size_t>(outWidth) * outHeight;
    std::vector<uint16_t> sliceData(sliceSize);

    
    
    // Rotation is matrix-based; identity means no rotation.
    bool hasRotation = !Mat4_IsIdentity(ctx->rotMat, 1e-4f);
    
    // MIP/MinIP mode settings
    const int projMode = ctx->projectionMode;  // 0=Normal, 1=MIP, 2=MinIP
    int halfThickness = static_cast<int>(ctx->projectionThickness / 2.0f);
    
    // 限制MIP厚度，避免性能问题（旋转+MIP时计算量 = width*height*thickness）
    if (halfThickness > 10) {
        printf("[PERF WARNING] MIP thickness %d is too large (samples=%d per pixel), clamping to 10. Expect slow rendering!\n", 
               halfThickness*2+1, halfThickness*2+1);
        halfThickness = 10;
    }
    
    // 性能统计
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Debug flag for rotation mode (only print once)
    static bool debugPrinted = false;
    
    if (!hasRotation) {
        // 鏃犳棆杞紝鐩存帴鎻愬彇鍒囩墖锛堟敮鎸丮IP/MinIP锛?
        switch (ctx->sliceDirection) {
            case 0: {  // Axial (XY 骞抽潰, Z 鍒囩墖)
                int sliceZ = static_cast<int>(sliceCenterZ);
                if (sliceZ < 0) sliceZ = 0;
                if (sliceZ >= depth) sliceZ = depth - 1;
                
                // MIP/MinIP: 鍦╖鏂瑰悜閲囨牱澶氫釜鍒囩墖
                int zStart = (projMode == 0) ? sliceZ : std::max(0, sliceZ - halfThickness);
                int zEnd = (projMode == 0) ? sliceZ : std::min(depth - 1, sliceZ + halfThickness);
                
                for (int y = 0; y < height; ++y) {
                    for (int x = 0; x < width; ++x) {
                        uint16_t value;
                        if (projMode == 0) {
                            // Normal: single slice
                            size_t volumeIdx = static_cast<size_t>(sliceZ) * width * height + y * width + x;
                            value = volumeData[volumeIdx];
                        } else if (projMode == 1) {
                            // MIP: maximum intensity
                            value = 0;
                            for (int z = zStart; z <= zEnd; ++z) {
                                size_t volumeIdx = static_cast<size_t>(z) * width * height + y * width + x;
                                value = std::max(value, volumeData[volumeIdx]);
                            }
                        } else {
                            // MinIP: minimum intensity
                            value = 65535;
                            for (int z = zStart; z <= zEnd; ++z) {
                                size_t volumeIdx = static_cast<size_t>(z) * width * height + y * width + x;
                                value = std::min(value, volumeData[volumeIdx]);
                            }
                        }
                        size_t sliceIdx = static_cast<size_t>(height - 1 - y) * width + x;  // 缈昏浆Y
                        sliceData[sliceIdx] = value;
                    }
                }
                crossTexX = ctx->crosshairU / (width - 1);
                crossTexY = 1.0f - (ctx->crosshairV / (height - 1));
                break;
            }
            
            case 1: {  // Coronal (XZ 骞抽潰, Y 鍒囩墖)
                int sliceY = static_cast<int>(sliceCenterY);
                if (sliceY < 0) sliceY = 0;
                if (sliceY >= height) sliceY = height - 1;
                
                // MIP/MinIP: 鍦╕鏂瑰悜閲囨牱澶氫釜鍒囩墖
                int yStart = (projMode == 0) ? sliceY : std::max(0, sliceY - halfThickness);
                int yEnd = (projMode == 0) ? sliceY : std::min(height - 1, sliceY + halfThickness);
                
                for (int z = 0; z < depth; ++z) {
                    for (int x = 0; x < width; ++x) {
                        uint16_t value;
                        if (projMode == 0) {
                            size_t volumeIdx = static_cast<size_t>(z) * width * height + sliceY * width + x;
                            value = volumeData[volumeIdx];
                        } else if (projMode == 1) {
                            value = 0;
                            for (int y = yStart; y <= yEnd; ++y) {
                                size_t volumeIdx = static_cast<size_t>(z) * width * height + y * width + x;
                                value = std::max(value, volumeData[volumeIdx]);
                            }
                        } else {
                            value = 65535;
                            for (int y = yStart; y <= yEnd; ++y) {
                                size_t volumeIdx = static_cast<size_t>(z) * width * height + y * width + x;
                                value = std::min(value, volumeData[volumeIdx]);
                            }
                        }
                        size_t sliceIdx = static_cast<size_t>(depth - 1 - z) * width + x;
                        sliceData[sliceIdx] = value;
                    }
                }
                crossTexX = ctx->crosshairU / (width - 1);
                crossTexY = 1.0f - (ctx->crosshairV / (depth - 1));
                break;
            }
            
            case 2: {  // Sagittal (YZ 骞抽潰, X 鍒囩墖)
                int sliceX = static_cast<int>(sliceCenterX);
                if (sliceX < 0) sliceX = 0;
                if (sliceX >= width) sliceX = width - 1;
                
                // MIP/MinIP: 鍦╔鏂瑰悜閲囨牱澶氫釜鍒囩墖
                int xStart = (projMode == 0) ? sliceX : std::max(0, sliceX - halfThickness);
                int xEnd = (projMode == 0) ? sliceX : std::min(width - 1, sliceX + halfThickness);
                
                for (int z = 0; z < depth; ++z) {
                    for (int y = 0; y < height; ++y) {
                        uint16_t value;
                        if (projMode == 0) {
                            size_t volumeIdx = static_cast<size_t>(z) * width * height + y * width + sliceX;
                            value = volumeData[volumeIdx];
                        } else if (projMode == 1) {
                            value = 0;
                            for (int x = xStart; x <= xEnd; ++x) {
                                size_t volumeIdx = static_cast<size_t>(z) * width * height + y * width + x;
                                value = std::max(value, volumeData[volumeIdx]);
                            }
                        } else {
                            value = 65535;
                            for (int x = xStart; x <= xEnd; ++x) {
                                size_t volumeIdx = static_cast<size_t>(z) * width * height + y * width + x;
                                value = std::min(value, volumeData[volumeIdx]);
                            }
                        }
                        size_t sliceIdx = static_cast<size_t>(depth - 1 - z) * height + y;
                        sliceData[sliceIdx] = value;
                    }
                }
                crossTexX = ctx->crosshairU / (height - 1);
                crossTexY = 1.0f - (ctx->crosshairV / (depth - 1));
                break;
            }
        }
    } else {
        // 锟斤拷锟斤拷转锟斤拷使锟斤拷统一锟斤拷 3D 锟斤拷转锟斤拷锟斤拷
        // 锟斤拷锟侥革拷锟筋：锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷通锟斤拷锟斤拷转锟斤拷锟斤拷映锟戒到实锟斤拷锟斤拷锟斤拷锟斤拷
        // - 锟斤拷锟斤拷锟斤拷锟斤拷 = 锟矫伙拷锟斤拷锟斤拷锟斤拷锟斤拷锟疥（centerX/Y/Z, crosshairU/V锟斤拷
        // - 实锟斤拷锟斤拷锟斤拷 = DICOM原始锟斤拷锟捷碉拷锟斤拷锟斤拷
        // - 映锟斤拷锟较碉拷锟绞碉拷锟?= 锟斤拷转锟斤拷锟斤拷 + R 锟斤拷 (锟斤拷锟斤拷 - 锟斤拷转锟斤拷锟斤拷)
        
        // 输出性能信息（仅一次）
        if (!debugPrinted) {
            const char* dirNames[] = {"Axial", "Coronal", "Sagittal"};
            const char* modeNames[] = {"Normal", "MIP", "MinIP"};
            long long totalSamples = (long long)outWidth * outHeight * (projMode == 0 ? 1 : halfThickness*2+1);
            printf("[PERF] Rotation+%s: %s view %dx%d, thickness=%d, total_samples=%lld\n", 
                   modeNames[projMode], dirNames[ctx->sliceDirection], 
                   outWidth, outHeight, halfThickness*2+1, totalSamples);
            debugPrinted = true;
        }
        
        // Use accumulated rotation matrix R (column-major): element(row, col) = m[col*4 + row]
        
        if (!debugPrinted) {
            const char* dirNames[] = {"Axial", "Coronal", "Sagittal"};
            const char* modeNames[] = {"Normal", "MIP", "MinIP"};
            printf("[PERF] Rotation rendering: %s view, %s mode, thickness=%d samples, resolution=%dx%d (total samples: %lld)\n", 
                   dirNames[ctx->sliceDirection], modeNames[projMode], halfThickness*2+1,
                   outWidth, outHeight, (long long)outWidth * outHeight * (projMode == 0 ? 1 : halfThickness*2+1));
            debugPrinted = true;
        }
        
        // Use accumulated rotation matrix R (column-major): element(row, col) = m[col*4 + row]
        const float* rm = ctx->rotMat;
        float r00 = rm[0];
        float r01 = rm[4];
        float r02 = rm[8];

        float r10 = rm[1];
        float r11 = rm[5];
        float r12 = rm[9];

        float r20 = rm[2];
        float r21 = rm[6];
        float r22 = rm[10];

        // 把切片中心从体数据坐标变换到虚拟坐标
        // virtual = C + R^T * (real - C)
        const float dx_center = sliceCenterX - rotationCenterX;
        const float dy_center = sliceCenterY - rotationCenterY;
        const float dz_center = sliceCenterZ - rotationCenterZ;
        
        const float virtualCenterX = rotationCenterX + (r00 * dx_center + r10 * dy_center + r20 * dz_center);
        const float virtualCenterY = rotationCenterY + (r01 * dx_center + r11 * dy_center + r21 * dz_center);
        const float virtualCenterZ = rotationCenterZ + (r02 * dx_center + r12 * dy_center + r22 * dz_center);
        
        // 计算窗宽窗位对应的像素值范围（用于提前退出优化）
        const float minHU = ctx->windowLevelHU - ctx->windowWidthHU / 2.0f;
        const float maxHU = ctx->windowLevelHU + ctx->windowWidthHU / 2.0f;
        const uint16_t minPixelValue = static_cast<uint16_t>(std::max(0.0f, (minHU - rescaleIntercept) / rescaleSlope));
        const uint16_t maxPixelValue = static_cast<uint16_t>(std::min(65535.0f, (maxHU - rescaleIntercept) / rescaleSlope));

        switch (ctx->sliceDirection) {
            case 0: {  // Axial (XY 平面, Z 切片)
                // 多线程并行渲染（使用std::thread池）
                const int numThreads = std::thread::hardware_concurrency();
                std::vector<std::thread> threads;
                threads.reserve(numThreads);
                int rowsPerThread = (outHeight + numThreads - 1) / numThreads;
                
                for (int t = 0; t < numThreads; ++t) {
                    int startY = t * rowsPerThread;
                    int endY = std::min(startY + rowsPerThread, outHeight);
                    if (startY >= outHeight) break;
                    
                    threads.emplace_back([=, &sliceData]() {
                        for (int outY = startY; outY < endY; ++outY) {
                            for (int outX = 0; outX < outWidth; ++outX) {
                                float virtualX = static_cast<float>(outX);
                                float virtualY = static_cast<float>(outY);
                                
                                uint16_t finalValue;
                                if (projMode == 0) {
                                    float virtualZ = virtualCenterZ;
                                    float dx = virtualX - rotationCenterX;
                                    float dy = virtualY - rotationCenterY;
                                    float dz = virtualZ - rotationCenterZ;
                                    float realX = rotationCenterX + (r00 * dx + r01 * dy + r02 * dz);
                                    float realY = rotationCenterY + (r10 * dx + r11 * dy + r12 * dz);
                                    float realZ = rotationCenterZ + (r20 * dx + r21 * dy + r22 * dz);
                                    finalValue = SampleVolume(volumeData, width, height, depth, realX, realY, realZ);
                                } else {
                                    finalValue = (projMode == 1) ? 0 : 65535;
                                    for (int k = -halfThickness; k <= halfThickness; ++k) {
                                        float virtualZ = virtualCenterZ + static_cast<float>(k);
                                        float dx = virtualX - rotationCenterX;
                                        float dy = virtualY - rotationCenterY;
                                        float dz = virtualZ - rotationCenterZ;
                                        float realX = rotationCenterX + (r00 * dx + r01 * dy + r02 * dz);
                                        float realY = rotationCenterY + (r10 * dx + r11 * dy + r12 * dz);
                                        float realZ = rotationCenterZ + (r20 * dx + r21 * dy + r22 * dz);
                                        uint16_t sample = SampleVolume(volumeData, width, height, depth, realX, realY, realZ);
                                        if (projMode == 1) { finalValue = std::max(finalValue, sample); }
                                        else { finalValue = std::min(finalValue, sample); }
                                    }
                                }
                                size_t outIdx = static_cast<size_t>(outHeight - 1 - outY) * outWidth + outX;
                                sliceData[outIdx] = finalValue;
                            }
                        }
                    });
                }
                
                for (auto& th : threads) {
                    if (th.joinable()) th.join();
                }
                
                // Locator crosshair position: use virtualCenter directly
                // Since we render the slice at virtualCenterZ, the crosshair should be at (virtualCenterX, virtualCenterY)
                {
                    const float crossPixelX = std::max(0.0f, std::min(static_cast<float>(outWidth - 1), virtualCenterX));
                    const float crossPixelY = std::max(0.0f, std::min(static_cast<float>(outHeight - 1), virtualCenterY));

                    crossTexX = crossPixelX / (outWidth - 1);
                    crossTexY = 1.0f - (crossPixelY / (outHeight - 1));
                }
                break;
            }

            case 1: {  // Coronal (XZ 平锟斤拷, Y 锟斤拷片)
                const int numThreads = std::thread::hardware_concurrency();
                std::vector<std::thread> threads;
                threads.reserve(numThreads);
                int rowsPerThread = (outHeight + numThreads - 1) / numThreads;
                
                for (int t = 0; t < numThreads; ++t) {
                    int startZ = t * rowsPerThread;
                    int endZ = std::min(startZ + rowsPerThread, outHeight);
                    if (startZ >= outHeight) break;
                    
                    threads.emplace_back([=, &sliceData]() {
                        for (int outZ = startZ; outZ < endZ; ++outZ) {
                            for (int outX = 0; outX < outWidth; ++outX) {
                                float virtualX = static_cast<float>(outX);
                                float virtualZ = static_cast<float>(outZ);
                                
                                uint16_t finalValue;
                                if (projMode == 0) {
                                    // Normal mode: single slice
                                    float virtualY = virtualCenterY;
                                    float dx = virtualX - rotationCenterX;
                                    float dy = virtualY - rotationCenterY;
                                    float dz = virtualZ - rotationCenterZ;
                                    float realX = rotationCenterX + (r00 * dx + r01 * dy + r02 * dz);
                                    float realY = rotationCenterY + (r10 * dx + r11 * dy + r12 * dz);
                                    float realZ = rotationCenterZ + (r20 * dx + r21 * dy + r22 * dz);
                                    finalValue = SampleVolume(volumeData, width, height, depth, realX, realY, realZ);
                                } else if (projMode == 1) {
                                    // MIP: 提前退出优化
                                    finalValue = 0;
                                    for (int k = -halfThickness; k <= halfThickness; ++k) {
                                        float virtualY = virtualCenterY + static_cast<float>(k);
                                        float dx = virtualX - rotationCenterX;
                                        float dy = virtualY - rotationCenterY;
                                        float dz = virtualZ - rotationCenterZ;
                                        float realX = rotationCenterX + (r00 * dx + r01 * dy + r02 * dz);
                                        float realY = rotationCenterY + (r10 * dx + r11 * dy + r12 * dz);
                                        float realZ = rotationCenterZ + (r20 * dx + r21 * dy + r22 * dz);
                                        uint16_t sample = SampleVolume(volumeData, width, height, depth, realX, realY, realZ);
                                        finalValue = std::max(finalValue, sample);
                                        if (finalValue >= maxPixelValue) break;
                                    }
                                } else {
                                    // MinIP: 提前退出优化
                                    finalValue = 65535;
                                    for (int k = -halfThickness; k <= halfThickness; ++k) {
                                        float virtualY = virtualCenterY + static_cast<float>(k);
                                        float dx = virtualX - rotationCenterX;
                                        float dy = virtualY - rotationCenterY;
                                        float dz = virtualZ - rotationCenterZ;
                                        float realX = rotationCenterX + (r00 * dx + r01 * dy + r02 * dz);
                                        float realY = rotationCenterY + (r10 * dx + r11 * dy + r12 * dz);
                                        float realZ = rotationCenterZ + (r20 * dx + r21 * dy + r22 * dz);
                                        uint16_t sample = SampleVolume(volumeData, width, height, depth, realX, realY, realZ);
                                        finalValue = std::min(finalValue, sample);
                                        if (finalValue <= minPixelValue) break;
                                    }
                                }

                                // 锟斤拷转Z锟斤拷锟斤拷MPR一锟斤拷
                                size_t outIdx = static_cast<size_t>(outHeight - 1 - outZ) * outWidth + outX;
                                sliceData[outIdx] = finalValue;
                            }
                        }
                    });
                }
                
                for (auto& th : threads) {
                    if (th.joinable()) th.join();
                }

                // Locator crosshair position: use virtualCenter directly
                // Since we render the slice at virtualCenterY, the crosshair should be at (virtualCenterX, virtualCenterZ)
                {
                    const float crossPixelX = std::max(0.0f, std::min(static_cast<float>(outWidth - 1), virtualCenterX));
                    const float crossPixelZ = std::max(0.0f, std::min(static_cast<float>(outHeight - 1), virtualCenterZ));

                    crossTexX = crossPixelX / (outWidth - 1);
                    crossTexY = 1.0f - (crossPixelZ / (outHeight - 1));
                }
                break;
            }

            case 2: {  // Sagittal (YZ 平锟斤拷, X 锟斤拷片)
                const int numThreads = std::thread::hardware_concurrency();
                std::vector<std::thread> threads;
                threads.reserve(numThreads);
                int rowsPerThread = (outHeight + numThreads - 1) / numThreads;
                
                for (int t = 0; t < numThreads; ++t) {
                    int startZ = t * rowsPerThread;
                    int endZ = std::min(startZ + rowsPerThread, outHeight);
                    if (startZ >= outHeight) break;
                    
                    threads.emplace_back([=, &sliceData]() {
                        for (int outZ = startZ; outZ < endZ; ++outZ) {
                            for (int outY = 0; outY < outWidth; ++outY) {
                                float virtualY = static_cast<float>(outY);
                                float virtualZ = static_cast<float>(outZ);
                                
                                uint16_t finalValue;
                                if (projMode == 0) {
                                    // Normal mode: single slice
                                    float virtualX = virtualCenterX;
                                    float dx = virtualX - rotationCenterX;
                                    float dy = virtualY - rotationCenterY;
                                    float dz = virtualZ - rotationCenterZ;
                                    float realX = rotationCenterX + (r00 * dx + r01 * dy + r02 * dz);
                                    float realY = rotationCenterY + (r10 * dx + r11 * dy + r12 * dz);
                                    float realZ = rotationCenterZ + (r20 * dx + r21 * dy + r22 * dz);
                                    finalValue = SampleVolume(volumeData, width, height, depth, realX, realY, realZ);
                                } else if (projMode == 1) {
                                    // MIP: 提前退出优化
                                    finalValue = 0;
                                    for (int k = -halfThickness; k <= halfThickness; ++k) {
                                        float virtualX = virtualCenterX + static_cast<float>(k);
                                        float dx = virtualX - rotationCenterX;
                                        float dy = virtualY - rotationCenterY;
                                        float dz = virtualZ - rotationCenterZ;
                                        float realX = rotationCenterX + (r00 * dx + r01 * dy + r02 * dz);
                                        float realY = rotationCenterY + (r10 * dx + r11 * dy + r12 * dz);
                                        float realZ = rotationCenterZ + (r20 * dx + r21 * dy + r22 * dz);
                                        uint16_t sample = SampleVolume(volumeData, width, height, depth, realX, realY, realZ);
                                        finalValue = std::max(finalValue, sample);
                                        if (finalValue >= maxPixelValue) break;
                                    }
                                } else {
                                    // MinIP: 提前退出优化
                                    finalValue = 65535;
                                    for (int k = -halfThickness; k <= halfThickness; ++k) {
                                        float virtualX = virtualCenterX + static_cast<float>(k);
                                        float dx = virtualX - rotationCenterX;
                                        float dy = virtualY - rotationCenterY;
                                        float dz = virtualZ - rotationCenterZ;
                                        float realX = rotationCenterX + (r00 * dx + r01 * dy + r02 * dz);
                                        float realY = rotationCenterY + (r10 * dx + r11 * dy + r12 * dz);
                                        float realZ = rotationCenterZ + (r20 * dx + r21 * dy + r22 * dz);
                                        uint16_t sample = SampleVolume(volumeData, width, height, depth, realX, realY, realZ);
                                        finalValue = std::min(finalValue, sample);
                                        if (finalValue == 0) break;
                                    }
                                }

                                // 锟斤拷转Z锟斤拷锟斤拷MPR一锟斤拷
                                size_t outIdx = static_cast<size_t>(outHeight - 1 - outZ) * outWidth + outY;
                                sliceData[outIdx] = finalValue;
                            }
                        }
                    });
                }
                
                // 锟饺达拷锟斤拷锟斤拷锟竭筹拷锟斤拷锟?
                for (auto& thread : threads) {
                    if (thread.joinable()) {
                        thread.join();
                    }
                }

                // Locator crosshair position: use virtualCenter directly
                // Since we render the slice at virtualCenterX, the crosshair should be at (virtualCenterY, virtualCenterZ)
                {
                    const float crossPixelY = std::max(0.0f, std::min(static_cast<float>(outWidth - 1), virtualCenterY));
                    const float crossPixelZ = std::max(0.0f, std::min(static_cast<float>(outHeight - 1), virtualCenterZ));

                    crossTexX = crossPixelY / (outWidth - 1);
                    crossTexY = 1.0f - (crossPixelZ / (outHeight - 1));
                }
                break;
            }
        }
    }
    

    // 统锟斤拷锟斤拷片锟斤拷锟斤拷
    uint16_t minVal = 65535, maxVal = 0;
    uint64_t sum = 0;
    int nonZeroCount = 0;
    
    for (size_t i = 0; i < sliceSize; ++i) {
        uint16_t val = sliceData[i];
        if (val > 0) nonZeroCount++;
        if (val < minVal) minVal = val;
        if (val > maxVal) maxVal = val;
        sum += val;
    }
    
    float avgVal = nonZeroCount > 0 ? static_cast<float>(sum) / nonZeroCount : 0;
    
    // 性能统计输出
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    if (hasRotation && projMode != 0 && duration > 500) {
        printf("[PERF WARNING] Slice rendering took %lld ms (rotation + %s, %dx%d x %d samples = %lld total)\n", 
               (long long)duration, projMode == 1 ? "MIP" : "MinIP",
               outWidth, outHeight, halfThickness*2+1, (long long)outWidth * outHeight * (halfThickness*2+1));
    }
    
    // Apply HU window/level to 0-255
    std::vector<uint8_t> displayData(sliceSize);
    for (size_t i = 0; i < sliceSize; ++i) {
        const float value = static_cast<float>(sliceData[i]) * rescaleSlope + rescaleIntercept;
        const int hu = static_cast<int>(value);
        displayData[i] = ApplyWindowLevelToByte(hu, ctx->windowWidthHU, ctx->windowLevelHU);
    }
    
    // 锟斤拷锟芥到锟斤拷锟斤拷锟斤拷
    ctx->displayBuffer = displayData;
    ctx->sliceWidth = outWidth;
    ctx->sliceHeight = outHeight;
    
    // 锟较达拷锟斤拷锟斤拷锟斤拷 OpenGL
    glBindTexture(GL_TEXTURE_2D, ctx->textureID);
    // 锟斤拷锟斤拷锟斤拷锟截讹拷锟斤拷为1锟斤拷锟斤拷锟斤拷锟斤拷炔锟斤拷锟?锟斤拷锟斤拷时锟侥达拷位锟斤拷锟斤拷
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, outWidth, outHeight, 0,
                 GL_LUMINANCE, GL_UNSIGNED_BYTE, displayData.data());
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // 锟街革拷默锟斤拷值
    
    // 锟斤拷染锟斤拷锟斤拷锟斤拷锟斤拷锟节ｏ拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷樱锟斤拷锟斤拷锟轿э拷贫锟轿伙拷锟斤拷锟斤拷牛锟?
    RenderTextureToWindow(ctx->textureID, outWidth, outHeight, ctx->zoomFactor, crossTexX, crossTexY);
    
    // 锟斤拷锟斤拷锟斤拷锟斤拷硕锟轿伙拷撸锟斤拷锟斤拷贫锟轿伙拷撸锟斤拷锟組PR一锟铰ｏ拷锟斤拷锟斤拷图锟斤拷锟节达拷锟斤拷锟叫碉拷实锟斤拷位锟矫ｏ拷
    if (ctx->showCrossHair) {
        // Use the exact mapping used by RenderTextureToWindow so crosshair stays aligned for zoom-in/out.
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        const int winWidth = viewport[2];
        const int winHeight = viewport[3];

        const TexWindowMapping map = ComputeTexWindowMapping(outWidth, outHeight, winWidth, winHeight, ctx->zoomFactor, crossTexX, crossTexY);
        if (!map.valid) {
            return NATIVE_OK;
        }

        const float denomX = std::max(1e-6f, (map.texRight - map.texLeft));
        const float denomY = std::max(1e-6f, (map.texTop - map.texBottom));
        const float nx = (crossTexX - map.texLeft) / denomX;
        const float ny = (crossTexY - map.texBottom) / denomY;

        const float crossScreenX = map.baseLeft + nx * (map.baseRight - map.baseLeft);
        const float crossScreenY = map.baseBottom + ny * (map.baseTop - map.baseBottom);
        
        // 只锟斤拷锟节讹拷位锟竭癸拷锟竭硷拷锟斤拷时锟脚伙拷锟狡讹拷位锟竭ｏ拷锟斤拷锟斤拷0锟斤拷
        if (g_currentToolType == 0) {
            // 锟斤拷锟狡讹拷位锟竭ｏ拷使锟斤拷锟斤拷锟接︼拷锟缴拷锟?
            // Only update the intersection point. Keep locator lines axis-aligned (not rotated).
            DrawCrossHair(crossScreenX, crossScreenY, ctx->sliceDirection, 0.0f);
        }
        
        // 锟斤拷锟狡边框（革拷锟斤拷锟斤拷片锟斤拷锟斤拷使锟矫诧拷同锟斤拷色锟斤拷
        // Axial=锟斤拷色(Z锟斤拷), Coronal=锟斤拷色(Y锟斤拷), Sagittal=锟斤拷色(X锟斤拷)
        glDisable(GL_TEXTURE_2D);
        glLineWidth(1.5f);
        glBegin(GL_LINE_LOOP);
        if (ctx->sliceDirection == 0) {
            glColor3f(0.0f, 0.0f, 1.0f);  // Axial - 锟斤拷色
        } else if (ctx->sliceDirection == 1) {
            glColor3f(0.0f, 1.0f, 0.0f);  // Coronal - 锟斤拷色
        } else {
            glColor3f(1.0f, 0.0f, 0.0f);  // Sagittal - 锟斤拷色
        }
        glVertex2f(-1.0f, -1.0f);
        glVertex2f( 1.0f, -1.0f);
        glVertex2f( 1.0f,  1.0f);
        glVertex2f(-1.0f,  1.0f);
        glEnd();
    }
    
    // 只锟斤拷锟节讹拷位锟竭癸拷锟竭硷拷锟斤拷时锟脚伙拷锟狡诧拷锟叫框（癸拷锟斤拷0锟斤拷
    if (g_currentToolType == 0 && g_aprCropBox.enabled) {
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        int winWidth = viewport[2];
        int winHeight = viewport[3];

        // Compute the same mapping used by RenderTextureToWindow (zoom anchored at crosshair).
        float crossTexX = 0.5f, crossTexY = 0.5f;
        if (ctx->sliceDirection == 0) {
            crossTexX = ctx->crosshairU / std::max(1, (outWidth - 1));
            crossTexY = 1.0f - (ctx->crosshairV / std::max(1, (outHeight - 1)));
        } else {
            // For Coronal/Sagittal, outWidth/outHeight already match the active slice axes.
            crossTexX = ctx->crosshairU / std::max(1, (outWidth - 1));
            crossTexY = 1.0f - (ctx->crosshairV / std::max(1, (outHeight - 1)));
        }

        const TexWindowMapping map = ComputeTexWindowMapping(outWidth, outHeight, winWidth, winHeight, ctx->zoomFactor, crossTexX, crossTexY);
        if (!map.valid) {
            // Fallback to old behavior would be misleading; just skip drawing.
            return NATIVE_OK;
        }

        float imgLeft = 0, imgRight = 0, imgTop = 0, imgBottom = 0;
        if (ctx->sliceDirection == 0) {
            imgLeft = g_aprCropBox.xStart;
            imgRight = g_aprCropBox.xEnd;
            imgTop = g_aprCropBox.yStart;
            imgBottom = g_aprCropBox.yEnd;
        } else if (ctx->sliceDirection == 1) {
            imgLeft = g_aprCropBox.xStart;
            imgRight = g_aprCropBox.xEnd;
            imgTop = g_aprCropBox.zStart;
            imgBottom = g_aprCropBox.zEnd;
        } else {
            imgLeft = g_aprCropBox.yStart;
            imgRight = g_aprCropBox.yEnd;
            imgTop = g_aprCropBox.zStart;
            imgBottom = g_aprCropBox.zEnd;
        }

        auto imgToNdc = [&](float ix, float iy, float& outX, float& outY) {
            float u = ix / std::max(1.0f, (outWidth - 1.0f));
            float v = 1.0f - (iy / std::max(1.0f, (outHeight - 1.0f)));
            u = std::max(0.0f, std::min(1.0f, u));
            v = std::max(0.0f, std::min(1.0f, v));
            float nx = (u - map.texLeft) / std::max(1e-6f, (map.texRight - map.texLeft));
            float ny = (v - map.texBottom) / std::max(1e-6f, (map.texTop - map.texBottom));
            outX = map.baseLeft + nx * (map.baseRight - map.baseLeft);
            outY = map.baseBottom + ny * (map.baseTop - map.baseBottom);
        };

        float glLeft = 0, glRight = 0, glTop = 0, glBottom = 0;
        imgToNdc(imgLeft, imgTop, glLeft, glTop);
        imgToNdc(imgRight, imgBottom, glRight, glBottom);
        
        // 根据裁切形状绘制不同的图形
        glColor3f(0.0f, 1.0f, 0.0f);  // 绿色
        glLineWidth(2.0f);
        
        float midX = (glLeft + glRight) / 2.0f;
        float midY = (glTop + glBottom) / 2.0f;
        float radiusX = (glRight - glLeft) / 2.0f;
        float radiusY = (glBottom - glTop) / 2.0f;  // Note: glTop > glBottom in NDC
        
        // 判断当前切片方向是否需要绘制椭圆
        bool drawEllipse = false;
        bool drawRect = true;
        
        if (g_aprCropBox.shape == CROP_SHAPE_SPHERE) {
            // 球体：所有视图都显示椭圆（实际是圆）
            drawEllipse = true;
            drawRect = false;
        } else if (g_aprCropBox.shape == CROP_SHAPE_CYLINDER) {
            // 圆柱体：根据方向和当前视图决定显示椭圆还是矩形
            int cylDir = static_cast<int>(g_aprCropBox.cylinderDirection);
            if (cylDir == 0) {  // 轴向圆柱 (沿Z轴)
                if (ctx->sliceDirection == 0) {  // Axial视图：显示圆
                    drawEllipse = true;
                    drawRect = false;
                }
                // Coronal和Sagittal显示矩形
            } else if (cylDir == 1) {  // 冠状圆柱 (沿Y轴)
                if (ctx->sliceDirection == 1) {  // Coronal视图：显示圆
                    drawEllipse = true;
                    drawRect = false;
                }
            } else if (cylDir == 2) {  // 矢状圆柱 (沿X轴)
                if (ctx->sliceDirection == 2) {  // Sagittal视图：显示圆
                    drawEllipse = true;
                    drawRect = false;
                }
            }
        }
        
        if (drawEllipse) {
            // 绘制椭圆/圆
            glBegin(GL_LINE_LOOP);
            const int segments = 64;
            for (int i = 0; i < segments; i++) {
                float angle = (float)i / segments * 2.0f * 3.14159265f;
                float x = midX + radiusX * cosf(angle);
                float y = midY + radiusY * sinf(angle);
                glVertex2f(x, y);
            }
            glEnd();
            
            // 绘制中心点
            glColor3f(1.0f, 0.0f, 0.0f);
            glPointSize(8.0f);
            glBegin(GL_POINTS);
            glVertex2f(midX, midY);
            glEnd();
            
            // 绘制4个边缘控制点（用于调整半径）
            glColor3f(0.0f, 1.0f, 1.0f);
            glBegin(GL_POINTS);
            glVertex2f(midX, glTop);      // 上
            glVertex2f(glRight, midY);    // 右
            glVertex2f(midX, glBottom);   // 下
            glVertex2f(glLeft, midY);     // 左
            glEnd();
        }
        
        if (drawRect) {
            // 绘制矩形（立方体或圆柱侧视图）
            glBegin(GL_LINE_LOOP);
            glVertex2f(glLeft, glTop);
            glVertex2f(glRight, glTop);
            glVertex2f(glRight, glBottom);
            glVertex2f(glLeft, glBottom);
            glEnd();
            
            // 绘制4个角点（红色小圆）
            glColor3f(1.0f, 0.0f, 0.0f);
            glPointSize(8.0f);
            glBegin(GL_POINTS);
            glVertex2f(glLeft, glTop);
            glVertex2f(glRight, glTop);
            glVertex2f(glLeft, glBottom);
            glVertex2f(glRight, glBottom);
            glEnd();
            
            // 绘制4个边缘中点（青色小圆）
            glColor3f(0.0f, 1.0f, 1.0f);
            glBegin(GL_POINTS);
            glVertex2f(midX, glTop);      // 上
            glVertex2f(glRight, midY);    // 右
            glVertex2f(midX, glBottom);   // 下
            glVertex2f(glLeft, midY);     // 左
            glEnd();
        }
        
        // 恢复颜色为白色
        glColor3f(1.0f, 1.0f, 1.0f);
    }
    
    // ==================== 锟斤拷锟斤拷APR锟斤拷锟斤拷锟斤拷锟斤拷 (tool 1-6) ====================
    if (g_currentToolType >= 1 && g_currentToolType <= 6) {
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_LINE_SMOOTH);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        
        // Compute mapping used by RenderTextureToWindow so overlays match image under zoom/letterbox.
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        const int winWidthMeasure = viewport[2];
        const int winHeightMeasure = viewport[3];
        const TexWindowMapping measureMap = ComputeTexWindowMapping(outWidth, outHeight, winWidthMeasure, winHeightMeasure, ctx->zoomFactor, crossTexX, crossTexY);

        // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷欠锟接︼拷锟斤拷诘锟角癆PR锟斤拷片锟斤拷示
        auto ShouldDisplayMeasurement = [ctx](const MeasurementLocation& loc) -> bool {
            // 锟斤拷APR锟斤拷遣锟斤拷锟绞撅拷锟斤拷锟轿狹PR锟斤拷APR锟斤拷锟斤拷系锟斤拷同锟斤拷
            if (!loc.isAPR) {
                return false;
            }
            
            // 锟斤拷锟斤拷锟狡拷锟斤拷锟斤拷欠锟狡ワ拷锟?
            if (loc.sliceDirection != ctx->sliceDirection) {
                return false;
            }
            
            // 锟斤拷锟斤拷锟阶拷嵌锟斤拷欠锟狡ワ拷洌拷锟斤拷锟叫★拷锟筋）
            const float angleTolerance = 1.0f;  // 1锟斤拷锟斤拷锟斤拷锟斤拷锟?
            float curX = 0.0f, curY = 0.0f, curZ = 0.0f;
            Mat4_ExtractEulerZYXDeg(ctx->rotMat, &curX, &curY, &curZ);
            if (fabsf(loc.rotX - curX) > angleTolerance ||
                fabsf(loc.rotY - curY) > angleTolerance ||
                fabsf(loc.rotZ - curZ) > angleTolerance) {
                return false;
            }
            
            // 锟斤拷锟姐当前锟斤拷片锟斤拷锟斤拷
            int currentSliceIndex = 0;
            if (ctx->sliceDirection == 0) {  // Axial - Z锟斤拷锟斤拷
                currentSliceIndex = (int)(ctx->centerZ + 0.5f);
            } else if (ctx->sliceDirection == 1) {  // Coronal - Y锟斤拷锟斤拷
                currentSliceIndex = (int)(ctx->centerY + 0.5f);
            } else {  // Sagittal - X锟斤拷锟斤拷
                currentSliceIndex = (int)(ctx->centerX + 0.5f);
            }
            
            // 只锟斤拷锟斤拷同一锟斤拷锟斤拷锟绞?
            return loc.sliceIndex == currentSliceIndex;
        };
        
        // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟狡碉拷锟斤拷锟斤拷锟竭ｏ拷锟斤拷锟斤拷为3D锟斤拷锟斤拷锟斤拷锟疥，转锟斤拷为NDC锟斤拷锟斤拷疲锟?
        auto DrawTool = [ctx, &measureMap](int toolType, const std::vector<MeasurementPoint>& points, bool isActive, float result) {
            if (points.empty()) return;

                std::vector<MeasurementPoint> adjustedPoints = points;
                if (isActive && g_shiftPressed && adjustedPoints.size() >= 2 && (toolType == 3 || toolType == 4)) {
                    adjustedPoints[1] = ConstrainSquareInPlane(adjustedPoints[0], adjustedPoints[1], ctx->sliceDirection);
                }

            // 锟斤拷3D锟斤拷锟斤拷锟斤拷锟斤拷转锟斤拷为2D NDC锟斤拷锟斤拷
            std::vector<Point2D> ndcPoints;
            for (const auto& wp : adjustedPoints) {
                Point2D imageNdc = APR_WorldToNDC(wp, ctx);
                ndcPoints.push_back(ImageNdcToScreenNdc(measureMap, imageNdc));
            }

            glColor3f(isActive ? 1.0f : 0.0f, 1.0f, isActive ? 0.0f : 0.0f);  // 锟斤拷锟斤拷=锟斤拷色锟斤拷锟斤拷锟?锟斤拷色
            glLineWidth(2.0f);

            switch (toolType) {
                case 1: // 直锟斤拷
                    if (ndcPoints.size() >= 2) {
                        glBegin(GL_LINES);
                        glVertex2f(ndcPoints[0].x, ndcPoints[0].y);
                        glVertex2f(ndcPoints[1].x, ndcPoints[1].y);
                        glEnd();
                    }
                    break;

                case 2: // 锟角讹拷
                    if (ndcPoints.size() >= 2) {
                        glBegin(GL_LINES);
                        glVertex2f(ndcPoints[0].x, ndcPoints[0].y);
                        glVertex2f(ndcPoints[1].x, ndcPoints[1].y);
                        if (ndcPoints.size() >= 3) {
                            glVertex2f(ndcPoints[1].x, ndcPoints[1].y);
                            glVertex2f(ndcPoints[2].x, ndcPoints[2].y);
                        }
                        glEnd();
                    }
                    break;

                case 3: // 锟斤拷锟斤拷
                    if (ndcPoints.size() >= 2) {
                        float x1 = ndcPoints[0].x, y1 = ndcPoints[0].y;
                        float x2 = ndcPoints[1].x, y2 = ndcPoints[1].y;

                        glBegin(GL_LINE_LOOP);
                        glVertex2f(x1, y1);
                        glVertex2f(x2, y1);
                        glVertex2f(x2, y2);
                        glVertex2f(x1, y2);
                        glEnd();
                    }
                    break;

                case 4: // 圆锟轿ｏ拷锟斤拷圆锟斤拷锟斤拷泳锟斤拷危锟?
                    if (ndcPoints.size() >= 2) {
                        float x1 = ndcPoints[0].x, y1 = ndcPoints[0].y;
                        float x2 = ndcPoints[1].x, y2 = ndcPoints[1].y;
                        float dx = x2 - x1, dy = y2 - y1;

                        float cx = (x1 + x2) / 2.0f;
                        float cy = (y1 + y2) / 2.0f;
                        float rx = fabsf(dx) / 2.0f;
                        float ry = fabsf(dy) / 2.0f;

                        glBegin(GL_LINE_LOOP);
                        int segments = 64;
                        for (int i = 0; i < segments; i++) {
                            float angle = 2.0f * 3.14159f * i / segments;
                            glVertex2f(cx + rx * cosf(angle), cy + ry * sinf(angle));
                        }
                        glEnd();

                        glPointSize(6.0f);
                        glBegin(GL_POINTS);
                        glVertex2f(cx, cy);
                        glEnd();
                    }
                    break;

                case 5: // Catmull-Rom 锟斤拷锟斤拷
                    if (ndcPoints.size() >= 2) {
                        auto catmullRom = [](float t, float p0, float p1, float p2, float p3) {
                            float t2 = t * t;
                            float t3 = t2 * t;
                            return 0.5f * (
                                (2.0f * p1) +
                                (-p0 + p2) * t +
                                (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                                (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
                                );
                        };

                        if (ndcPoints.size() == 2) {
                            // 只锟斤拷锟斤拷锟斤拷锟姐，直锟斤拷锟斤拷锟斤拷
                            glBegin(GL_LINES);
                            glVertex2f(ndcPoints[0].x, ndcPoints[0].y);
                            glVertex2f(ndcPoints[1].x, ndcPoints[1].y);
                            glEnd();
                        }
                        else {
                            // 锟斤拷锟斤拷悖癸拷锟?Catmull-Rom 锟斤拷锟斤拷
                            const int segmentsPerCurve = 20;
                            glBegin(GL_LINE_STRIP);

                            for (size_t i = 0; i < ndcPoints.size() - 1; ++i) {
                                Point2D p0 = (i == 0) ? ndcPoints[0] : ndcPoints[i - 1];
                                Point2D p1 = ndcPoints[i];
                                Point2D p2 = ndcPoints[i + 1];
                                Point2D p3 = (i + 2 < ndcPoints.size()) ? ndcPoints[i + 2] : ndcPoints[i + 1];

                                for (int j = 0; j <= segmentsPerCurve; ++j) {
                                    float t = (float)j / (float)segmentsPerCurve;
                                    float x = catmullRom(t, p0.x, p1.x, p2.x, p3.x);
                                    float y = catmullRom(t, p0.y, p1.y, p2.y, p3.y);
                                    glVertex2f(x, y);
                                }
                            }

                            glEnd();
                        }
                    }
                    break;

                case 6: // 锟斤拷锟斤拷锟斤拷锟竭ｏ拷锟斤拷眨锟?
                    if (ndcPoints.size() >= 2) {
                        glBegin(GL_LINE_LOOP);
                        for (const auto& pt : ndcPoints) {
                            glVertex2f(pt.x, pt.y);
                        }
                        glEnd();
                    }
                    break;
            }

            // 锟斤拷锟狡匡拷锟狡点（锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟解）
            if (toolType != 6) {
                glColor3f(1.0f, 0.0f, 0.0f);  // 锟斤拷色
                glPointSize(8.0f);
                glBegin(GL_POINTS);
                for (const auto& pt : ndcPoints) {
                    glVertex2f(pt.x, pt.y);
                }
                glEnd();
            }
        };
        
        // 锟斤拷锟斤拷锟斤拷锟斤拷傻牟锟斤拷锟斤拷锟绞癸拷霉潭锟斤拷锟斤拷牡悖拷锟斤拷锟斤拷锟狡拷贫锟绞憋拷锟斤拷锟斤拷锟斤拷锟狡讹拷锟斤拷
        auto DrawToolFixed = [ctx, &measureMap](int measurementIndex, int toolType, const std::vector<MeasurementPoint>& points, 
                        const MeasurementLocation& loc, float result) {
            if (points.empty()) return;

            // 使锟矫固讹拷锟斤拷锟侥点将3D锟斤拷锟斤拷锟斤拷锟斤拷转锟斤拷为2D NDC锟斤拷锟斤拷
            std::vector<Point2D> ndcPoints;
            for (const auto& wp : points) {
                Point2D imageNdc = APR_WorldToNDC_Fixed(wp, ctx, loc.centerX, loc.centerY, loc.centerZ);
                ndcPoints.push_back(ImageNdcToScreenNdc(measureMap, imageNdc));
            }

            glColor3f(0.0f, 1.0f, 0.0f);  // 锟斤拷傻牟锟斤拷锟?锟斤拷色
            glLineWidth(2.0f);

            switch (toolType) {
                case 1: // 直锟斤拷
                    if (ndcPoints.size() >= 2) {
                        glBegin(GL_LINES);
                        glVertex2f(ndcPoints[0].x, ndcPoints[0].y);
                        glVertex2f(ndcPoints[1].x, ndcPoints[1].y);
                        glEnd();
                    }
                    break;

                case 2: // 锟角讹拷
                    if (ndcPoints.size() >= 2) {
                        glBegin(GL_LINES);
                        glVertex2f(ndcPoints[0].x, ndcPoints[0].y);
                        glVertex2f(ndcPoints[1].x, ndcPoints[1].y);
                        if (ndcPoints.size() >= 3) {
                            glVertex2f(ndcPoints[1].x, ndcPoints[1].y);
                            glVertex2f(ndcPoints[2].x, ndcPoints[2].y);
                        }
                        glEnd();
                    }
                    break;

                case 3: // 锟斤拷锟斤拷
                    if (ndcPoints.size() >= 2) {
                        float x1 = ndcPoints[0].x, y1 = ndcPoints[0].y;
                        float x2 = ndcPoints[1].x, y2 = ndcPoints[1].y;
                        glBegin(GL_LINE_LOOP);
                        glVertex2f(x1, y1);
                        glVertex2f(x2, y1);
                        glVertex2f(x2, y2);
                        glVertex2f(x1, y2);
                        glEnd();
                    }
                    break;

                case 4: // 圆锟轿ｏ拷锟斤拷圆锟斤拷
                    if (ndcPoints.size() >= 2) {
                        float x1 = ndcPoints[0].x, y1 = ndcPoints[0].y;
                        float x2 = ndcPoints[1].x, y2 = ndcPoints[1].y;
                        float cx = (x1 + x2) / 2.0f;
                        float cy = (y1 + y2) / 2.0f;
                        float rx = fabsf(x2 - x1) / 2.0f;
                        float ry = fabsf(y2 - y1) / 2.0f;
                        glBegin(GL_LINE_LOOP);
                        int segments = 64;
                        for (int i = 0; i < segments; i++) {
                            float angle = 2.0f * 3.14159f * i / segments;
                            glVertex2f(cx + rx * cosf(angle), cy + ry * sinf(angle));
                        }
                        glEnd();
                        glPointSize(6.0f);
                        glBegin(GL_POINTS);
                        glVertex2f(cx, cy);
                        glEnd();
                    }
                    break;

                case 5: // Catmull-Rom锟斤拷锟斤拷
                    if (ndcPoints.size() >= 2) {
                        auto catmullRom = [](float t, float p0, float p1, float p2, float p3) {
                            float t2 = t * t;
                            float t3 = t2 * t;
                            return 0.5f * (
                                (2.0f * p1) +
                                (-p0 + p2) * t +
                                (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                                (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
                            );
                        };
                        
                        if (ndcPoints.size() == 2) {
                            // 只锟斤拷锟斤拷锟斤拷锟姐，直锟斤拷锟斤拷锟斤拷
                            glBegin(GL_LINES);
                            glVertex2f(ndcPoints[0].x, ndcPoints[0].y);
                            glVertex2f(ndcPoints[1].x, ndcPoints[1].y);
                            glEnd();
                        } else {
                            // 锟斤拷锟斤拷悖癸拷锟?Catmull-Rom 锟斤拷锟斤拷
                            const int segmentsPerCurve = 20;
                            glBegin(GL_LINE_STRIP);
                            
                            for (size_t i = 0; i < ndcPoints.size() - 1; ++i) {
                                Point2D p0 = (i == 0) ? ndcPoints[0] : ndcPoints[i - 1];
                                Point2D p1 = ndcPoints[i];
                                Point2D p2 = ndcPoints[i + 1];
                                Point2D p3 = (i + 2 < ndcPoints.size()) ? ndcPoints[i + 2] : ndcPoints[i + 1];
                                
                                for (int j = 0; j <= segmentsPerCurve; ++j) {
                                    float t = (float)j / (float)segmentsPerCurve;
                                    float x = catmullRom(t, p0.x, p1.x, p2.x, p3.x);
                                    float y = catmullRom(t, p0.y, p1.y, p2.y, p3.y);
                                    glVertex2f(x, y);
                                }
                            }
                            
                            glEnd();
                        }
                    }
                    break;

                case 6: // 锟斤拷锟斤拷锟斤拷锟竭ｏ拷锟斤拷眨锟?
                    if (ndcPoints.size() >= 2) {
                        glBegin(GL_LINE_LOOP);  // 使锟斤拷 LOOP 锟斤拷锟斤拷锟斤拷锟?
                        for (const auto& pt : ndcPoints) {
                            glVertex2f(pt.x, pt.y);
                        }
                        glEnd();
                    }
                    break;
            }

            // 锟斤拷锟狡匡拷锟狡碉拷
            if (toolType != 6) {
                glColor3f(1.0f, 0.0f, 0.0f);
                glPointSize(8.0f);
                glBegin(GL_POINTS);
                for (const auto& pt : ndcPoints) {
                    glVertex2f(pt.x, pt.y);
                }
                glEnd();

                if (measurementIndex == g_hoverMeasurementIndex && g_hoverPointIndex >= 0 && g_hoverPointIndex < (int)ndcPoints.size()) {
                    const auto& hp = ndcPoints[(size_t)g_hoverPointIndex];
                    glColor3f(1.0f, 1.0f, 0.0f);
                    glPointSize(12.0f);
                    glBegin(GL_POINTS);
                    glVertex2f(hp.x, hp.y);
                    glEnd();
                }
            }
        };
        
        // ==================== Mask overlays (from MPR session) ====================
        // For correctness, we currently render overlays only when there is no APR rotation.
        if (!hasRotation && !ctx->sessionId.empty()) {
            MPRContext* mprSession = GetMPRContextFromSession(ctx->sessionId.c_str());
            if (mprSession && mprSession->volume) {
                int maskSliceIndex = 0;
                if (ctx->sliceDirection == 0) {
                    maskSliceIndex = (int)(sliceCenterZ + 0.5f);
                    if (maskSliceIndex < 0) maskSliceIndex = 0;
                    if (maskSliceIndex >= depth) maskSliceIndex = depth - 1;
                } else if (ctx->sliceDirection == 1) {
                    maskSliceIndex = (int)(sliceCenterY + 0.5f);
                    if (maskSliceIndex < 0) maskSliceIndex = 0;
                    if (maskSliceIndex >= height) maskSliceIndex = height - 1;
                } else {
                    maskSliceIndex = (int)(sliceCenterX + 0.5f);
                    if (maskSliceIndex < 0) maskSliceIndex = 0;
                    if (maskSliceIndex >= width) maskSliceIndex = width - 1;
                }

                const int sliceW = outWidth;
                const int sliceH = outHeight;
                const size_t sliceSizeU = (size_t)sliceW * (size_t)sliceH;

                auto renderMaskVolume = [&](const std::vector<uint8_t>& maskData, const std::string& color, uint8_t alpha) {
                    if (maskData.empty()) return;

                    std::vector<uint8_t> maskSlice(sliceSizeU, 0);
                    for (int i = 0; i < sliceH; ++i) {
                        for (int j = 0; j < sliceW; ++j) {
                            size_t volumeIdx = 0;
                            if (ctx->sliceDirection == 0) {
                                // Axial: flip Y
                                volumeIdx = (size_t)maskSliceIndex * (size_t)width * (size_t)height + (size_t)(height - 1 - i) * (size_t)width + (size_t)j;
                            } else if (ctx->sliceDirection == 1) {
                                // Coronal: flip Z
                                volumeIdx = (size_t)(depth - 1 - i) * (size_t)width * (size_t)height + (size_t)maskSliceIndex * (size_t)width + (size_t)j;
                            } else {
                                // Sagittal: flip Z
                                volumeIdx = (size_t)(depth - 1 - i) * (size_t)width * (size_t)height + (size_t)j * (size_t)width + (size_t)maskSliceIndex;
                            }

                            if (volumeIdx < maskData.size()) {
                                maskSlice[(size_t)i * (size_t)sliceW + (size_t)j] = maskData[volumeIdx];
                            }
                        }
                    }

                    float r, g, b;
                    ParseHexColor(color.c_str(), r, g, b);

                    std::vector<uint8_t> rgba(sliceSizeU * 4);
                    for (size_t k = 0; k < sliceSizeU; ++k) {
                        if (maskSlice[k] > 0) {
                            rgba[k * 4 + 0] = (uint8_t)(r * 255);
                            rgba[k * 4 + 1] = (uint8_t)(g * 255);
                            rgba[k * 4 + 2] = (uint8_t)(b * 255);
                            rgba[k * 4 + 3] = alpha;
                        } else {
                            rgba[k * 4 + 3] = 0;
                        }
                    }

                    RenderMaskOverlay(rgba.data(), sliceW, sliceH, ctx->zoomFactor, crossTexX, crossTexY);
                };

                // Preview
                if (mprSession->previewMask && mprSession->previewMask->visible) {
                    renderMaskVolume(mprSession->previewMask->data, mprSession->previewMask->color, 76 /*30%*/);
                }

                // Permanent masks
                for (const auto& mask : mprSession->masks) {
                    if (!mask.visible) continue;
                    renderMaskVolume(mask.data, mask.color, 128 /*50%*/);
                }
            }
        }

        // 锟斤拷锟斤拷锟斤拷锟斤拷傻牟锟斤拷锟酵硷拷危锟街伙拷锟斤拷锟斤拷锟斤拷诘锟角癆PR锟斤拷片锟斤拷锟斤拷转锟角度的ｏ拷
        {
            std::lock_guard<std::mutex> lock(g_completedMeasurementsMutex);
            for (size_t mi = 0; mi < g_completedMeasurements.size(); ++mi) {
                const auto& measurement = g_completedMeasurements[mi];
                if (ShouldDisplayMeasurement(measurement.location)) {
                    DrawToolFixed((int)mi, measurement.toolType, measurement.points, measurement.location, measurement.result);
                }
            }
        }
        
        // 锟斤拷锟狡碉拷前锟斤拷锟节伙拷锟狡碉拷图锟轿ｏ拷锟斤拷锟斤拷锟斤拷示锟斤拷锟斤拷为锟斤拷锟斤拷锟节碉拷前锟斤拷片锟斤拷
        if (!g_measurementPoints.empty()) {
            auto activePoints = g_measurementPoints;
            
            // 锟斤拷锟斤拷锟较讹拷锟斤拷锟竭ｏ拷锟斤拷锟接碉拷前锟斤拷锟轿伙拷锟斤拷锟轿拷诙锟斤拷锟斤拷锟?
            if (g_isDrawing && (g_currentToolType == 1 || g_currentToolType == 3 || g_currentToolType == 4)) {
                if (activePoints.size() == 1) {
                    activePoints.push_back(g_currentMousePos);
                }
            }
            
            // 锟角度癸拷锟竭ｏ拷锟斤拷锟斤拷锟斤拷选锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷预锟斤拷
            if (g_currentToolType == 2) {
                if (activePoints.size() == 1) {
                    activePoints.push_back(g_currentMousePos);
                } else if (activePoints.size() == 2) {
                    activePoints.push_back(g_currentMousePos);
                }
            }

            // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟竭ｏ拷锟斤拷锟斤拷预锟斤拷锟斤拷锟斤拷前锟斤拷锟轿伙拷锟?
            if (g_currentToolType == 5 && activePoints.size() >= 1) {
                activePoints.push_back(g_currentMousePos);
            }
            
            DrawTool(g_currentToolType, activePoints, true, 0.0f);
        }

        // Draw measurement labels (NanoVG)
        // Ensure NanoVG/font are ready so HUD is reliable across tool switches.
        EnsureNanoVGReady();
        if (g_nvgContext) {
            GLint viewport[4];
            glGetIntegerv(GL_VIEWPORT, viewport);
            int winWidth = viewport[2];
            int winHeight = viewport[3];

            if (winWidth > 0 && winHeight > 0) {
                PrepareGLForNanoVGOverlay();
                nvgBeginFrame(g_nvgContext, winWidth, winHeight, 1.0f);

                // Slice header top-center (text requires font)
                if (g_nvgFontId >= 0) {
                    int w = 0, h = 0, d = 0;
                    Dicom_Volume_GetDimensions(ctx->volume, &w, &h, &d);
                    int sliceIndex = 0;
                    int sliceTotal = 0;
                    const char* plane = "";
                    if (ctx->sliceDirection == 0) { plane = "Axial"; sliceIndex = (int)(ctx->centerZ + 0.5f); sliceTotal = d; }
                    else if (ctx->sliceDirection == 1) { plane = "Coronal"; sliceIndex = (int)(ctx->centerY + 0.5f); sliceTotal = h; }
                    else { plane = "Sagittal"; sliceIndex = (int)(ctx->centerX + 0.5f); sliceTotal = w; }
                    if (sliceTotal <= 0) sliceTotal = 1;
                    if (sliceIndex < 0) sliceIndex = 0;
                    if (sliceIndex > sliceTotal - 1) sliceIndex = sliceTotal - 1;
                    char header[96];
                    snprintf(header, sizeof(header), "%s Slice %d / %d", plane, sliceIndex, sliceTotal);
                    DrawSliceHeaderNVG_InFrame(winWidth, winHeight, header);
                }

                DrawWindowLevelHudNVG_InFrame(winWidth, winHeight, ctx->windowWidthHU, ctx->windowLevelHU);
                DrawVerticalScaleBarNVG_InFrame(winWidth, winHeight, ctx->sliceDirection, ctx->zoomFactor, ctx->volume);

                // Measurement labels require a font; skip labels if font isn't available.
                if (g_nvgFontId >= 0) {
                    nvgFontSize(g_nvgContext, 16.0f);
                    nvgFontFace(g_nvgContext, "ui");
                    nvgTextAlign(g_nvgContext, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

                auto NdcToScreen = [&](const Point2D& ndc) -> Point2D {
                    Point2D s;
                    s.x = (ndc.x + 1.0f) * 0.5f * (float)winWidth;
                    s.y = (1.0f - (ndc.y + 1.0f) * 0.5f) * (float)winHeight;
                    return s;
                };

                auto FormatLabel = [&](int toolType, float value, char* out, size_t outSize) {
                    switch (toolType) {
                        case 1:
                            snprintf(out, outSize, "%.2f mm", value);
                            break;
                        case 2:
                            snprintf(out, outSize, "%.1f\xC2\xB0", value);
                            break;
                        case 3:
                        case 4:
                            snprintf(out, outSize, "%.2f mm\xC2\xB2", value);
                            break;
                        case 5:
                        case 6:
                            snprintf(out, outSize, "%.2f mm", value);
                            break;
                        default:
                            snprintf(out, outSize, "%.2f", value);
                            break;
                    }
                };

                auto AnchorFromNdc = [&](int toolType, const std::vector<Point2D>& ndcPoints) -> Point2D {
                    if (ndcPoints.empty()) return {0, 0};
                    if (toolType == 2 && ndcPoints.size() >= 2) {
                        return ndcPoints[1];
                    }
                    if ((toolType == 1 || toolType == 3 || toolType == 4) && ndcPoints.size() >= 2) {
                        return {(ndcPoints[0].x + ndcPoints[1].x) * 0.5f, (ndcPoints[0].y + ndcPoints[1].y) * 0.5f};
                    }

                    float sx = 0.0f, sy = 0.0f;
                    for (const auto& p : ndcPoints) {
                        sx += p.x;
                        sy += p.y;
                    }
                    float inv = 1.0f / (float)ndcPoints.size();
                    return {sx * inv, sy * inv};
                };

                auto DrawLabel = [&](const Point2D& ndcAnchor, const char* text, bool active) {
                    Point2D sp = NdcToScreen(ndcAnchor);
                    sp.x += 6.0f;
                    sp.y += 0.0f;

                    nvgFillColor(g_nvgContext, nvgRGBA(0, 0, 0, 160));
                    nvgText(g_nvgContext, sp.x + 1.0f, sp.y + 1.0f, text, nullptr);

                    if (active) {
                        nvgFillColor(g_nvgContext, nvgRGBA(255, 255, 0, 220));
                    } else {
                        nvgFillColor(g_nvgContext, nvgRGBA(0, 255, 0, 220));
                    }
                    nvgText(g_nvgContext, sp.x, sp.y, text, nullptr);
                };

                // Completed measurements
                {
                    std::lock_guard<std::mutex> lock(g_completedMeasurementsMutex);
                    for (size_t mi = 0; mi < g_completedMeasurements.size(); ++mi) {
                        const auto& measurement = g_completedMeasurements[mi];
                        if (!ShouldDisplayMeasurement(measurement.location)) continue;

                        std::vector<Point2D> ndcPoints;
                        ndcPoints.reserve(measurement.points.size());
                        for (const auto& wp : measurement.points) {
                            ndcPoints.push_back(APR_WorldToNDC_Fixed(wp, ctx, measurement.location.centerX, measurement.location.centerY, measurement.location.centerZ));
                        }

                        char buf[64] = {0};
                        FormatLabel(measurement.toolType, measurement.result, buf, sizeof(buf));
                        DrawLabel(AnchorFromNdc(measurement.toolType, ndcPoints), buf, false);
                    }
                }

                    // Active measurement preview
                    if (!g_measurementPoints.empty()) {
                    auto previewPoints = g_measurementPoints;
                    if (g_isDrawing && (g_currentToolType == 1 || g_currentToolType == 3 || g_currentToolType == 4)) {
                        if (previewPoints.size() == 1) previewPoints.push_back(g_currentMousePos);
                    }
                    if (g_currentToolType == 2) {
                        if (previewPoints.size() == 1) previewPoints.push_back(g_currentMousePos);
                        else if (previewPoints.size() == 2) previewPoints.push_back(g_currentMousePos);
                    }
                    if (g_currentToolType == 5 && previewPoints.size() >= 1) {
                        previewPoints.push_back(g_currentMousePos);
                    }

                    CompletedMeasurement tmp{};
                    tmp.toolType = g_currentToolType;
                    tmp.points = previewPoints;
                    tmp.location.sliceDirection = ctx->sliceDirection;
                    tmp.location.isAPR = true;
                    tmp.result = RecomputeCompletedMeasurementResult(tmp);

                    std::vector<Point2D> ndcPoints;
                    ndcPoints.reserve(tmp.points.size());
                    for (const auto& wp : tmp.points) {
                        ndcPoints.push_back(APR_WorldToNDC(wp, ctx));
                    }

                    if (!ndcPoints.empty()) {
                        char buf[64] = {0};
                        FormatLabel(tmp.toolType, tmp.result, buf, sizeof(buf));
                        DrawLabel(AnchorFromNdc(tmp.toolType, ndcPoints), buf, true);
                    }
                    }
                }

                nvgEndFrame(g_nvgContext);
            }
        }
    }
    
    // 锟斤拷锟斤拷锟斤拷锟斤拷锟较拷锟矫?0帧一锟轿ｏ拷
    static int frameCount = 0;
    // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷瞥锟斤拷锟斤拷锟斤拷锟剿拷锟斤拷锟?

    // HUD overlay (slice header + WW/WL + scale bar) should be visible even when toolType==0.
    if (!(g_currentToolType >= 1 && g_currentToolType <= 6)) {
        EnsureNanoVGReady();
        if (g_nvgContext) {
            GLint viewport[4];
            glGetIntegerv(GL_VIEWPORT, viewport);
            int winWidth = viewport[2];
            int winHeight = viewport[3];

            if (winWidth > 0 && winHeight > 0) {
                PrepareGLForNanoVGOverlay();
                nvgBeginFrame(g_nvgContext, winWidth, winHeight, 1.0f);

                // Slice header top-center
                {
                    int w = 0, h = 0, d = 0;
                    Dicom_Volume_GetDimensions(ctx->volume, &w, &h, &d);
                    int sliceIndex = 0;
                    int sliceTotal = 0;
                    const char* plane = "";
                    if (ctx->sliceDirection == 0) { plane = "Axial"; sliceIndex = (int)(ctx->centerZ + 0.5f); sliceTotal = d; }
                    else if (ctx->sliceDirection == 1) { plane = "Coronal"; sliceIndex = (int)(ctx->centerY + 0.5f); sliceTotal = h; }
                    else { plane = "Sagittal"; sliceIndex = (int)(ctx->centerX + 0.5f); sliceTotal = w; }
                    if (sliceTotal <= 0) sliceTotal = 1;
                    if (sliceIndex < 0) sliceIndex = 0;
                    if (sliceIndex > sliceTotal - 1) sliceIndex = sliceTotal - 1;
                    char header[96];
                    snprintf(header, sizeof(header), "%s Slice %d / %d", plane, sliceIndex, sliceTotal);
                    DrawSliceHeaderNVG_InFrame(winWidth, winHeight, header);
                }

                DrawWindowLevelHudNVG_InFrame(winWidth, winHeight, ctx->windowWidthHU, ctx->windowLevelHU);
                DrawVerticalScaleBarNVG_InFrame(winWidth, winHeight, ctx->sliceDirection, ctx->zoomFactor, ctx->volume);

                nvgEndFrame(g_nvgContext);
            }
        }
    }

    return NATIVE_OK;
}

// APR 3D锟斤拷锟斤拷锟斤拷图锟斤拷染锟斤拷锟斤拷示锟斤拷锟斤拷APR平锟斤拷锟洁互锟斤拷直锟斤拷锟斤拷
NativeResult APR_RenderOrthogonal3D(APRHandle axial, APRHandle coronal, APRHandle sagittal) {
    if (!axial || !coronal || !sagittal) {
        printf("[3D Render] Invalid handles: axial=%p, coronal=%p, sagittal=%p\n", axial, coronal, sagittal);
        return NATIVE_E_INVALID_ARGUMENT;
    }
    
    APRContext* ctxAxial = static_cast<APRContext*>(axial);
    APRContext* ctxCoronal = static_cast<APRContext*>(coronal);
    APRContext* ctxSagittal = static_cast<APRContext*>(sagittal);
    
    // 锟斤拷取锟斤拷前锟斤拷锟节尺寸（锟斤拷OpenGL锟接口伙拷取锟斤拷锟斤拷锟斤拷Win32锟斤拷GLFW锟斤拷
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    int winWidth = viewport[2];
    int winHeight = viewport[3];
    
    // 锟斤拷锟斤拷锟斤拷锟斤拷锟矫伙拷写锟斤拷锟斤拷锟斤拷锟斤拷诘锟角帮拷锟斤拷锟斤拷锟斤拷写锟斤拷锟斤拷锟斤拷锟斤拷锟?
    // 注锟解：锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟侥癸拷锟斤拷锟斤拷锟斤拷些锟斤拷锟斤拷锟斤拷锟皆憋拷锟斤拷锟斤拷锟斤拷锟斤拷使锟斤拷
    // 实锟绞碉拷锟斤拷锟斤拷锟斤拷锟斤拷锟缴革拷锟斤拷APR锟斤拷锟斤拷锟皆硷拷锟斤拷锟斤拷染锟斤拷锟斤拷锟斤拷
    // IMPORTANT:
    // The 3 APR windows (axial/coronal/sagittal) typically each have their own OpenGL context.
    // If WGL context sharing is enabled (wglShareLists), their ctx->textureID can be directly
    // used here. When sharing is NOT available (or broken), binding ctx->textureID here will
    // sample as an empty texture and the 3D window appears black.
    //
    // Strategy:
    // - Prefer the shared ctx->textureID when it is a valid texture object in the *current*
    //   context (glIsTexture == GL_TRUE).
    // - Otherwise, fall back to uploading from the APR CPU-side displayBuffer into textures
    //   owned by the current context.
    struct TriPlanarTextures {
        GLuint texAxial = 0;
        GLuint texCoronal = 0;
        GLuint texSagittal = 0;
        int wAx = 0, hAx = 0;
        int wCo = 0, hCo = 0;
        int wSa = 0, hSa = 0;
    };

    static std::unordered_map<void*, TriPlanarTextures> s_triTex;

#ifdef _WIN32
    void* ctxKey = reinterpret_cast<void*>(wglGetCurrentContext());
#else
    void* ctxKey = reinterpret_cast<void*>(glfwGetCurrentContext());
#endif
    if (!ctxKey) ctxKey = reinterpret_cast<void*>(1);

    TriPlanarTextures& tri = s_triTex[ctxKey];

    auto ensureTex = [](GLuint& texId) {
        if (!texId) glGenTextures(1, &texId);
    };
    ensureTex(tri.texAxial);
    ensureTex(tri.texCoronal);
    ensureTex(tri.texSagittal);

    auto hasSlice = [](const APRContext* c) {
        if (!c) return false;
        if (c->sliceWidth <= 0 || c->sliceHeight <= 0) return false;
        const size_t needed = static_cast<size_t>(c->sliceWidth) * static_cast<size_t>(c->sliceHeight);
        return c->displayBuffer.size() >= needed;
    };
    const bool hasAxCpu = hasSlice(ctxAxial);
    const bool hasCoCpu = hasSlice(ctxCoronal);
    const bool hasSaCpu = hasSlice(ctxSagittal);

    const bool hasAxShared = (ctxAxial && ctxAxial->textureID && glIsTexture(ctxAxial->textureID) == GL_TRUE);
    const bool hasCoShared = (ctxCoronal && ctxCoronal->textureID && glIsTexture(ctxCoronal->textureID) == GL_TRUE);
    const bool hasSaShared = (ctxSagittal && ctxSagittal->textureID && glIsTexture(ctxSagittal->textureID) == GL_TRUE);

    auto uploadGray = [](GLuint texId, int w, int h, const std::vector<uint8_t>& buf) {
        if (!texId || w <= 0 || h <= 0) return;
        const size_t needed = static_cast<size_t>(w) * static_cast<size_t>(h);
        if (buf.size() < needed) return;

        glBindTexture(GL_TEXTURE_2D, texId);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // Legacy LUMINANCE keeps fixed-pipeline sampling simple.
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w, h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buf.data());
    };

    if (!hasAxShared && hasAxCpu) {
        tri.wAx = ctxAxial->sliceWidth;
        tri.hAx = ctxAxial->sliceHeight;
        uploadGray(tri.texAxial, tri.wAx, tri.hAx, ctxAxial->displayBuffer);
    }
    if (!hasCoShared && hasCoCpu) {
        tri.wCo = ctxCoronal->sliceWidth;
        tri.hCo = ctxCoronal->sliceHeight;
        uploadGray(tri.texCoronal, tri.wCo, tri.hCo, ctxCoronal->displayBuffer);
    }
    if (!hasSaShared && hasSaCpu) {
        tri.wSa = ctxSagittal->sliceWidth;
        tri.hSa = ctxSagittal->sliceHeight;
        uploadGray(tri.texSagittal, tri.wSa, tri.hSa, ctxSagittal->displayBuffer);
    }
    
    // 锟斤拷锟斤拷锟接匡拷
    glViewport(0, 0, winWidth, winHeight);
    
    // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟缴拷锟?
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 锟斤拷锟斤拷锟斤拷炔锟斤拷院锟斤拷锟斤拷锟?
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    
    // 锟斤拷锟斤拷投影锟斤拷锟斤拷透锟斤拷投影锟斤拷
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = static_cast<float>(winWidth) / winHeight;
    glFrustum(-aspect * 0.1, aspect * 0.1, -0.1, 0.1, 0.2, 10.0);
    
    // 锟斤拷取锟斤拷锟斤拷锟捷的尺达拷
    int volW, volH, volD;
    if (Dicom_Volume_GetDimensions(ctxAxial->volume, &volW, &volH, &volD) != NATIVE_OK) {
        return NATIVE_E_INVALID_ARGUMENT;
    }
    
    // 锟斤拷取 APR 锟斤拷锟斤拷锟侥点（锟斤拷位锟竭斤拷锟斤拷位锟矫ｏ拷
    float centerX = ctxAxial->centerX;
    float centerY = ctxAxial->centerY;
    float centerZ = ctxAxial->centerZ;
    
    // 锟斤拷锟斤拷锟斤拷锟轿拷龋锟斤拷锟斤拷诠锟揭伙拷锟斤拷锟绞撅拷锟?
    int maxDim = volW;
    if (volH > maxDim) maxDim = volH;
    if (volD > maxDim) maxDim = volD;
    
    float normW = static_cast<float>(volW) / maxDim;
    float normH = static_cast<float>(volH) / maxDim;
    float normD = static_cast<float>(volD) / maxDim;
    
    // 锟斤拷一锟斤拷锟斤拷锟侥点到 [-0.5, 0.5] 锟斤拷围锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷为原锟姐）
    float normCenterX = (centerX / (volW - 1)) - 0.5f;
    float normCenterY = (centerY / (volH - 1)) - 0.5f;
    float normCenterZ = (centerZ / (volD - 1)) - 0.5f;
    
    // Get session-specific 3D state if available
    float panX = g_3dPanX, panY = g_3dPanY, zoom = g_3dZoom;
    float* rotMat = g_3dRotMat;
    
    if (!ctxAxial->sessionId.empty()) {
        TabSessionContext* tabCtx = FindTabSession(ctxAxial->sessionId);
        if (tabCtx) {
            panX = tabCtx->panX;
            panY = tabCtx->panY;
            zoom = tabCtx->zoom;
            rotMat = tabCtx->rotMat;
        }
    }
    
    // 锟斤拷锟斤拷模锟斤拷锟斤拷图锟斤拷锟斤拷
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(panX, panY, -3.0f / zoom);  // Pan + zoom
    
    // 应锟斤拷锟斤拷转锟斤拷使锟斤拷全锟街憋拷锟斤拷锟斤拷通锟斤拷锟斤拷锟斤拷壹锟斤拷锟斤拷拢锟?
    glMultMatrixf(rotMat);
    
    // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷平锟芥（锟皆讹拷位锟竭斤拷锟斤拷为锟斤拷锟侥ｏ拷
    glColor3f(1.0f, 1.0f, 1.0f);
    
    // 锟斤拷锟姐定位锟斤拷锟节癸拷一锟斤拷锟秸硷拷锟叫碉拷位锟斤拷
    float posX = normCenterX * normW;
    float posY = normCenterY * normH;
    float posZ = normCenterZ * normD;

    auto rot3 = [](const float m[16], float x, float y, float z, float& ox, float& oy, float& oz) {
        // Column-major 4x4; apply upper-left 3x3.
        ox = m[0] * x + m[4] * y + m[8]  * z;
        oy = m[1] * x + m[5] * y + m[9]  * z;
        oz = m[2] * x + m[6] * y + m[10] * z;
    };
    
    // 1. Axial plane (XY, Z=posZ)
    if (hasAxShared || hasAxCpu) {
        glBindTexture(GL_TEXTURE_2D, hasAxShared ? ctxAxial->textureID : tri.texAxial);
        float v0x, v0y, v0z;
        float v1x, v1y, v1z;
        float v2x, v2y, v2z;
        float v3x, v3y, v3z;
        rot3(ctxAxial->rotMat, -normW/2, -normH/2, posZ, v0x, v0y, v0z);
        rot3(ctxAxial->rotMat, -normW/2,  normH/2, posZ, v1x, v1y, v1z);
        rot3(ctxAxial->rotMat,  normW/2,  normH/2, posZ, v2x, v2y, v2z);
        rot3(ctxAxial->rotMat,  normW/2, -normH/2, posZ, v3x, v3y, v3z);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(v0x, v0y, v0z);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(v1x, v1y, v1z);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(v2x, v2y, v2z);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(v3x, v3y, v3z);
        glEnd();
    }
    
    // 2. Coronal plane (XZ, Y=posY)
    if (hasCoShared || hasCoCpu) {
        glBindTexture(GL_TEXTURE_2D, hasCoShared ? ctxCoronal->textureID : tri.texCoronal);
        float v0x, v0y, v0z;
        float v1x, v1y, v1z;
        float v2x, v2y, v2z;
        float v3x, v3y, v3z;
        // Use the shared volume rotation (same across linked APRs).
        rot3(ctxAxial->rotMat, -normW/2, posY, -normD/2, v0x, v0y, v0z);
        rot3(ctxAxial->rotMat,  normW/2, posY, -normD/2, v1x, v1y, v1z);
        rot3(ctxAxial->rotMat,  normW/2, posY,  normD/2, v2x, v2y, v2z);
        rot3(ctxAxial->rotMat, -normW/2, posY,  normD/2, v3x, v3y, v3z);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(v0x, v0y, v0z);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(v1x, v1y, v1z);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(v2x, v2y, v2z);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(v3x, v3y, v3z);
        glEnd();
    }
    
    // 3. Sagittal plane (YZ, X=posX)
    if (hasSaShared || hasSaCpu) {
        glBindTexture(GL_TEXTURE_2D, hasSaShared ? ctxSagittal->textureID : tri.texSagittal);
        float v0x, v0y, v0z;
        float v1x, v1y, v1z;
        float v2x, v2y, v2z;
        float v3x, v3y, v3z;
        rot3(ctxAxial->rotMat, posX, -normH/2, -normD/2, v0x, v0y, v0z);
        rot3(ctxAxial->rotMat, posX,  normH/2, -normD/2, v1x, v1y, v1z);
        rot3(ctxAxial->rotMat, posX,  normH/2,  normD/2, v2x, v2y, v2z);
        rot3(ctxAxial->rotMat, posX, -normH/2,  normD/2, v3x, v3y, v3z);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(v0x, v0y, v0z);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(v1x, v1y, v1z);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(v2x, v2y, v2z);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(v3x, v3y, v3z);
        glEnd();
    }
    
    // 绘制定位线：使用旋转后的crosshair位置
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);  // 定位线不要深度测试
    glLineWidth(3.0f);

    // 获取旋转后的crosshair位置（体素坐标）
    float crosshairX = centerX, crosshairY = centerY, crosshairZ = centerZ;
    
    // 从axial APR获取旋转后的X,Y坐标  
    if (ctxAxial) {
        crosshairX = ctxAxial->crosshairU;
        crosshairY = ctxAxial->crosshairV;
    }
    
    // 从coronal APR获取旋转后的Z坐标
    if (ctxCoronal) {
        crosshairZ = ctxCoronal->crosshairV;  // coronal的V对应Z坐标
    }

    // 转换到归一化3D坐标
    const float crossX = (crosshairX / (volW - 1) - 0.5f) * normW;
    const float crossY = (crosshairY / (volH - 1) - 0.5f) * normH;  
    const float crossZ = (crosshairZ / (volD - 1) - 0.5f) * normD;

    glBegin(GL_LINES);
    // X轴（红色）- 穿过旋转后的交点位置，延伸到边界
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(-normW, crossY, crossZ);
    glVertex3f( normW, crossY, crossZ);
    // Y轴（绿色）
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(crossX, -normH, crossZ);
    glVertex3f(crossX,  normH, crossZ);
    // Z轴（蓝色）
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(crossX, crossY, -normD);
    glVertex3f(crossX, crossY,  normD);
    glEnd();
    
    // 缁樺埗瑁佸垏妗嗭紙濡傛灉鍚敤锛?
    if (g_aprCropBox.enabled) {
        // 灏嗚鍒囨鍧愭爣浠庝綋绱犵┖闂磋浆鎹㈠埌褰掍竴鍖栫┖闂?
        float cropX0 = ((g_aprCropBox.xStart / (volW - 1)) - 0.5f) * normW;
        float cropX1 = ((g_aprCropBox.xEnd / (volW - 1)) - 0.5f) * normW;
        float cropY0 = ((g_aprCropBox.yStart / (volH - 1)) - 0.5f) * normH;
        float cropY1 = ((g_aprCropBox.yEnd / (volH - 1)) - 0.5f) * normH;
        float cropZ0 = ((g_aprCropBox.zStart / (volD - 1)) - 0.5f) * normD;
        float cropZ1 = ((g_aprCropBox.zEnd / (volD - 1)) - 0.5f) * normD;
        
        // 设置裁切框线条样式（黄色虚线）
        glLineWidth(2.0f);
        glEnable(GL_LINE_STIPPLE);
        glLineStipple(1, 0x00FF);  // 虚线
        glColor4f(1.0f, 1.0f, 0.0f, 0.8f);  // 黄色
        
        const float midX = (cropX0 + cropX1) / 2.0f;
        const float midY = (cropY0 + cropY1) / 2.0f;
        const float midZ = (cropZ0 + cropZ1) / 2.0f;
        const float radX = (cropX1 - cropX0) / 2.0f;
        const float radY = (cropY1 - cropY0) / 2.0f;
        const float radZ = (cropZ1 - cropZ0) / 2.0f;
        
        if (g_aprCropBox.shape == CROP_SHAPE_SPHERE) {
            // 球体：绘制3个正交的圆
            const int segments = 32;
            const float pi2 = 2.0f * 3.14159265f;
            const float radius = std::min({radX, radY, radZ});
            
            // XY平面圆
            glBegin(GL_LINE_LOOP);
            for (int i = 0; i < segments; i++) {
                float a = (float)i / segments * pi2;
                glVertex3f(midX + radius * cosf(a), midY + radius * sinf(a), midZ);
            }
            glEnd();
            
            // XZ平面圆
            glBegin(GL_LINE_LOOP);
            for (int i = 0; i < segments; i++) {
                float a = (float)i / segments * pi2;
                glVertex3f(midX + radius * cosf(a), midY, midZ + radius * sinf(a));
            }
            glEnd();
            
            // YZ平面圆
            glBegin(GL_LINE_LOOP);
            for (int i = 0; i < segments; i++) {
                float a = (float)i / segments * pi2;
                glVertex3f(midX, midY + radius * cosf(a), midZ + radius * sinf(a));
            }
            glEnd();
        } else if (g_aprCropBox.shape == CROP_SHAPE_CYLINDER) {
            // 圆柱体
            const int segments = 32;
            const float pi2 = 2.0f * 3.14159265f;
            const int cylDir = static_cast<int>(g_aprCropBox.cylinderDirection);
            
            if (cylDir == 0) {
                // 轴向圆柱（沿Z轴）
                const float radius = std::min(radX, radY);
                // 底圆
                glBegin(GL_LINE_LOOP);
                for (int i = 0; i < segments; i++) {
                    float a = (float)i / segments * pi2;
                    glVertex3f(midX + radius * cosf(a), midY + radius * sinf(a), cropZ0);
                }
                glEnd();
                // 顶圆
                glBegin(GL_LINE_LOOP);
                for (int i = 0; i < segments; i++) {
                    float a = (float)i / segments * pi2;
                    glVertex3f(midX + radius * cosf(a), midY + radius * sinf(a), cropZ1);
                }
                glEnd();
                // 垂直线
                glBegin(GL_LINES);
                for (int i = 0; i < 4; i++) {
                    float a = (float)i / 4 * pi2;
                    float x = midX + radius * cosf(a);
                    float y = midY + radius * sinf(a);
                    glVertex3f(x, y, cropZ0);
                    glVertex3f(x, y, cropZ1);
                }
                glEnd();
            } else if (cylDir == 1) {
                // 冠状圆柱（沿Y轴）
                const float radius = std::min(radX, radZ);
                // 前圆
                glBegin(GL_LINE_LOOP);
                for (int i = 0; i < segments; i++) {
                    float a = (float)i / segments * pi2;
                    glVertex3f(midX + radius * cosf(a), cropY0, midZ + radius * sinf(a));
                }
                glEnd();
                // 后圆
                glBegin(GL_LINE_LOOP);
                for (int i = 0; i < segments; i++) {
                    float a = (float)i / segments * pi2;
                    glVertex3f(midX + radius * cosf(a), cropY1, midZ + radius * sinf(a));
                }
                glEnd();
                // 垂直线
                glBegin(GL_LINES);
                for (int i = 0; i < 4; i++) {
                    float a = (float)i / 4 * pi2;
                    float x = midX + radius * cosf(a);
                    float z = midZ + radius * sinf(a);
                    glVertex3f(x, cropY0, z);
                    glVertex3f(x, cropY1, z);
                }
                glEnd();
            } else {
                // 矢状圆柱（沿X轴）
                const float radius = std::min(radY, radZ);
                // 左圆
                glBegin(GL_LINE_LOOP);
                for (int i = 0; i < segments; i++) {
                    float a = (float)i / segments * pi2;
                    glVertex3f(cropX0, midY + radius * cosf(a), midZ + radius * sinf(a));
                }
                glEnd();
                // 右圆
                glBegin(GL_LINE_LOOP);
                for (int i = 0; i < segments; i++) {
                    float a = (float)i / segments * pi2;
                    glVertex3f(cropX1, midY + radius * cosf(a), midZ + radius * sinf(a));
                }
                glEnd();
                // 垂直线
                glBegin(GL_LINES);
                for (int i = 0; i < 4; i++) {
                    float a = (float)i / 4 * pi2;
                    float y = midY + radius * cosf(a);
                    float z = midZ + radius * sinf(a);
                    glVertex3f(cropX0, y, z);
                    glVertex3f(cropX1, y, z);
                }
                glEnd();
            }
        } else {
            // 立方体（默认）- 绘制裁切框的12条边
            glBegin(GL_LINES);
            // 底面 (Z = cropZ0)
            glVertex3f(cropX0, cropY0, cropZ0);
            glVertex3f(cropX1, cropY0, cropZ0);
            
            glVertex3f(cropX1, cropY0, cropZ0);
            glVertex3f(cropX1, cropY1, cropZ0);
            
            glVertex3f(cropX1, cropY1, cropZ0);
            glVertex3f(cropX0, cropY1, cropZ0);
            
            glVertex3f(cropX0, cropY1, cropZ0);
            glVertex3f(cropX0, cropY0, cropZ0);
            
            // 顶面 (Z = cropZ1)
            glVertex3f(cropX0, cropY0, cropZ1);
            glVertex3f(cropX1, cropY0, cropZ1);
            
            glVertex3f(cropX1, cropY0, cropZ1);
            glVertex3f(cropX1, cropY1, cropZ1);
            
            glVertex3f(cropX1, cropY1, cropZ1);
            glVertex3f(cropX0, cropY1, cropZ1);
            
            glVertex3f(cropX0, cropY1, cropZ1);
            glVertex3f(cropX0, cropY0, cropZ1);
            
            // 四条垂直边
            glVertex3f(cropX0, cropY0, cropZ0);
            glVertex3f(cropX0, cropY0, cropZ1);
            
            glVertex3f(cropX1, cropY0, cropZ0);
            glVertex3f(cropX1, cropY0, cropZ1);
            
            glVertex3f(cropX1, cropY1, cropZ0);
            glVertex3f(cropX1, cropY1, cropZ1);
            
            glVertex3f(cropX0, cropY1, cropZ0);
            glVertex3f(cropX0, cropY1, cropZ1);
            glEnd();
        }
        
        glDisable(GL_LINE_STIPPLE);
    }

    // Debug overlay: show 3D navigation & texture availability.
    // This helps diagnose "black 3D window" issues without attaching a debugger.
    {
        char dbg[512];
        snprintf(
            dbg,
            sizeof(dbg),
            "Orth3D g_3dZoom=%.4f pan=(%.3f,%.3f) shared(A/C/S)=%d/%d/%d cpu(A/C/S)=%d/%d/%d tex(A/C/S)=%u/%u/%u",
            g_3dZoom,
            g_3dPanX,
            g_3dPanY,
            (int)hasAxShared,
            (int)hasCoShared,
            (int)hasSaShared,
            (int)hasAxCpu,
            (int)hasCoCpu,
            (int)hasSaCpu,
            ctxAxial ? ctxAxial->textureID : 0u,
            ctxCoronal ? ctxCoronal->textureID : 0u,
            ctxSagittal ? ctxSagittal->textureID : 0u
        );
        DrawNanoVGDebugTextTopLeft(dbg);
    }
    
    // 绘制比例尺（3D正交视图）
    FILE* logFile = nullptr;
    fopen_s(&logFile, "D:\\3d_render_debug.log", "a");
    if (logFile) {
        fprintf(logFile, "[APR_RenderOrthogonal3D] About to draw scale bar: ctxAxial=%p volume=%p winSize=%dx%d zoom=%.3f\n",
                ctxAxial, ctxAxial ? ctxAxial->volume : nullptr, winWidth, winHeight, zoom);
        fclose(logFile);
    }
    
    if (ctxAxial && ctxAxial->volume && winWidth > 0 && winHeight > 0) {
        fopen_s(&logFile, "D:\\3d_render_debug.log", "a");
        if (logFile) {
            fprintf(logFile, "[APR_RenderOrthogonal3D] Calling DrawVerticalScaleBarNVG_InFrame...\n");
            fclose(logFile);
        }
        DrawVerticalScaleBarNVG_InFrame(winWidth, winHeight, 0, zoom, ctxAxial->volume);
        fopen_s(&logFile, "D:\\3d_render_debug.log", "a");
        if (logFile) {
            fprintf(logFile, "[APR_RenderOrthogonal3D] Scale bar drawn successfully\n");
            fclose(logFile);
        }
    } else {
        fopen_s(&logFile, "D:\\3d_render_debug.log", "a");
        if (logFile) {
            fprintf(logFile, "[APR_RenderOrthogonal3D] Scale bar NOT drawn - conditions failed\n");
            fclose(logFile);
        }
    }
    
    return NATIVE_OK;
    
}
NativeResult Window_Set3DRendererKind(WindowHandle handle, int kind) {
    if (!handle) return NATIVE_E_INVALID_ARGUMENT;
    auto ctx = static_cast<WindowContext*>(handle);
    if (kind < 1 || kind > 3) return NATIVE_E_INVALID_ARGUMENT;
    ctx->threeDRendererKind = kind;
    ctx->threeDRendererKindExplicit = true;
    return NATIVE_OK;
}


void APR_GetCrosshairPosition(APRHandle handle, float* u, float* v) {
    if (!handle || !u || !v) return;
    APRContext* ctx = static_cast<APRContext*>(handle);
    *u = ctx->crosshairU;
    *v = ctx->crosshairV;
}

void APR_GetRotationMatrix(APRHandle handle, float* outMatrix16) {
    if (!handle || !outMatrix16) return;
    APRContext* ctx = static_cast<APRContext*>(handle);
    memcpy(outMatrix16, ctx->rotMat, sizeof(float) * 16);
}

void APR_SetOrthogonal3DMode(APRHandle handle, bool enable) {
    if (!handle) return;
    APRContext* ctx = static_cast<APRContext*>(handle);
    ctx->orthogonal3DMode = enable;
}

// ==================== Volume3D ====================
Volume3DHandle Volume3D_Create() {
    return new Volume3DContext();
}

void Volume3D_Destroy(Volume3DHandle handle) {
    delete static_cast<Volume3DContext*>(handle);
}

NativeResult Volume3D_AddVolume(Volume3DHandle handle, VolumeHandle volume) {
    if (!handle) return NATIVE_E_INVALID_ARGUMENT;
    static_cast<Volume3DContext*>(handle)->volumes.push_back(volume);
    return NATIVE_OK;
}

NativeResult Volume3D_RemoveVolume(Volume3DHandle handle, int index) {
    if (!handle) return NATIVE_E_INVALID_ARGUMENT;
    auto ctx = static_cast<Volume3DContext*>(handle);
    if (index < 0 || index >= (int)ctx->volumes.size()) return NATIVE_E_INVALID_ARGUMENT;
    ctx->volumes.erase(ctx->volumes.begin() + index);
    return NATIVE_OK;
}

int Volume3D_GetVolumeCount(Volume3DHandle handle) {
    if (!handle) return 0;
    return (int)static_cast<Volume3DContext*>(handle)->volumes.size();
}

NativeResult Volume3D_SetTransferFunction(Volume3DHandle handle, int volumeIndex, TransferFunctionHandle tf) {
    if (!handle) return NATIVE_E_INVALID_ARGUMENT;
    auto ctx = static_cast<Volume3DContext*>(handle);
    if (volumeIndex < 0 || volumeIndex >= (int)ctx->volumes.size()) return NATIVE_E_INVALID_ARGUMENT;
    ctx->transferFunction = tf;
    return NATIVE_OK;
}

void Volume3D_SetLightParameters(Volume3DHandle handle, float ambient, float diffuse, float specular) {
    if (!handle) return;
    auto ctx = static_cast<Volume3DContext*>(handle);
    ctx->ambient = ambient; ctx->diffuse = diffuse; ctx->specular = specular;
}

void Volume3D_GetLightParameters(Volume3DHandle handle, float* ambient, float* diffuse, float* specular) {
    if (!handle) return;
    auto ctx = static_cast<Volume3DContext*>(handle);
    if (ambient) *ambient = ctx->ambient;
    if (diffuse) *diffuse = ctx->diffuse;
    if (specular) *specular = ctx->specular;
}

NativeResult Volume3D_Render(Volume3DHandle handle) {
    if (!handle) return NATIVE_E_INVALID_ARGUMENT;
    // TODO: 实锟斤拷锟斤拷染锟竭硷拷
    return NATIVE_OK;
}

// ==================== TransferFunction ====================
TransferFunctionHandle TransferFunction_Create() {
    return new TransferFunctionContext();
}

void TransferFunction_Destroy(TransferFunctionHandle handle) {
    delete static_cast<TransferFunctionContext*>(handle);
}

NativeResult TransferFunction_AddControlPoint(TransferFunctionHandle handle, float value, float r, float g, float b, float a) {
    if (!handle) return NATIVE_E_INVALID_ARGUMENT;
    auto ctx = static_cast<TransferFunctionContext*>(handle);
    ctx->points.push_back({value, r, g, b, a});
    return NATIVE_OK;
}

NativeResult TransferFunction_RemoveControlPoint(TransferFunctionHandle handle, int index) {
    if (!handle) return NATIVE_E_INVALID_ARGUMENT;
    auto ctx = static_cast<TransferFunctionContext*>(handle);
    if (index < 0 || index >= (int)ctx->points.size()) return NATIVE_E_INVALID_ARGUMENT;
    ctx->points.erase(ctx->points.begin() + index);
    return NATIVE_OK;
}

void TransferFunction_Clear(TransferFunctionHandle handle) {
    if (!handle) return;
    static_cast<TransferFunctionContext*>(handle)->points.clear();
}

int TransferFunction_GetControlPointCount(TransferFunctionHandle handle) {
    if (!handle) return 0;
    return (int)static_cast<TransferFunctionContext*>(handle)->points.size();
}

// ==================== 锟斤拷锟斤拷锟铰硷拷锟截碉拷 ====================
#if !defined(_WIN32)
static void OnFramebufferResize(GLFWwindow* window, int width, int height) {
    // 锟截硷拷锟斤拷锟斤拷锟叫伙拷锟斤拷锟斤拷前锟斤拷锟节碉拷 OpenGL 锟斤拷锟斤拷锟斤拷
    glfwMakeContextCurrent(window);
    glViewport(0, 0, width, height);
    
    // 锟斤拷锟斤拷 WindowContext 锟竭达拷
    WindowContext* ctx = static_cast<WindowContext*>(glfwGetWindowUserPointer(window));
    if (ctx) {
        ctx->width = width;
        ctx->height = height;
    }
}

static void OnMouseButton(GLFWwindow* window, int button, int action, int mods) {
    WindowContext* ctx = static_cast<WindowContext*>(glfwGetWindowUserPointer(window));
    if (!ctx) return;
    
    // 锟斤拷锟斤拷Shift锟斤拷状态
    g_shiftPressed = (mods & GLFW_MOD_SHIFT) != 0;
    
    // MPR/APR 锟斤拷锟节ｏ拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟竭碉拷锟?(只锟斤拷tool 1-6时锟斤拷锟斤拷)
    if ((ctx->rendererType == 1 || ctx->rendererType == 2) && ctx->boundRenderer && ctx->activeTool && g_currentToolType >= 1 && g_currentToolType <= 6) {
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        
        // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷转锟斤拷为OpenGL锟斤拷准锟斤拷锟斤拷锟斤拷 (-1 锟斤拷 1)
        float ndcX = (float)(mouseX / ctx->width) * 2.0f - 1.0f;
        float ndcY = 1.0f - (float)(mouseY / ctx->height) * 2.0f;  // Y锟结翻转
        
        // 转锟斤拷为3D锟斤拷锟斤拷锟斤拷锟斤拷锟疥（锟斤拷锟斤拷锟斤拷染锟斤拷锟斤拷锟酵ｏ拷
        MeasurementPoint worldPos;
        if (ctx->rendererType == 1) {
            auto mprCtx = static_cast<MPRContext*>(ctx->boundRenderer);
            worldPos = MPR_NDCToWorld(ndcX, ndcY, mprCtx);
        } else {
            auto aprCtx = static_cast<APRContext*>(ctx->boundRenderer);
            worldPos = APR_NDCToWorld(ndcX, ndcY, aprCtx);
        }
        
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                double currentTime = glfwGetTime();
                bool isDoubleClick = (currentTime - g_lastClickTime) < 0.3;  // 300ms锟斤拷为双锟斤拷
                g_lastClickTime = currentTime;
                
                printf("[%s Mouse] Left %s at (%.1f, %.1f) -> World(%.2f, %.2f, %.2f)\n", 
                    ctx->rendererType == 1 ? "MPR" : "APR",
                    isDoubleClick ? "Double-Click" : "Press", mouseX, mouseY, worldPos.x, worldPos.y, worldPos.z);
                
                if (ctx->activeTool && ctx->toolManager) {
                    // 锟斤拷录锟斤拷前锟斤拷锟斤拷锟斤拷堑锟轿伙拷锟斤拷锟较拷锟斤拷诳锟绞硷拷锟斤拷锟绞憋拷锟?
                    if (g_measurementPoints.empty()) {
                        g_currentMeasurementLocation = MeasurementLocation();
                        
                        if (ctx->rendererType == 1) {  // MPR
                            auto mprCtx = static_cast<MPRContext*>(ctx->boundRenderer);
                            g_currentMeasurementLocation.sliceDirection = mprCtx->sliceDirection;
                            g_currentMeasurementLocation.isAPR = false;
                            
                            // 锟斤拷录锟斤拷锟斤拷时锟斤拷锟斤拷图锟斤拷锟斤拷锟斤拷锟疥（锟斤拷锟节伙拷锟狡ｏ拷
                            g_currentMeasurementLocation.centerX = mprCtx->centerX;
                            g_currentMeasurementLocation.centerY = mprCtx->centerY;
                            g_currentMeasurementLocation.centerZ = mprCtx->centerZ;
                            
                            // 锟斤拷录锟斤拷前锟斤拷片锟斤拷锟斤拷
                            if (mprCtx->sliceDirection == 0) {  // Axial
                                g_currentMeasurementLocation.sliceIndex = (int)(mprCtx->centerZ + 0.5f);
                            } else if (mprCtx->sliceDirection == 1) {  // Coronal
                                g_currentMeasurementLocation.sliceIndex = (int)(mprCtx->centerY + 0.5f);
                            } else {  // Sagittal
                                g_currentMeasurementLocation.sliceIndex = (int)(mprCtx->centerX + 0.5f);
                            }
                        } else {  // APR
                            auto aprCtx = static_cast<APRContext*>(ctx->boundRenderer);
                            g_currentMeasurementLocation.sliceDirection = aprCtx->sliceDirection;
                            g_currentMeasurementLocation.isAPR = true;
                            Mat4_ExtractEulerZYXDeg(aprCtx->rotMat,
                                &g_currentMeasurementLocation.rotX,
                                &g_currentMeasurementLocation.rotY,
                                &g_currentMeasurementLocation.rotZ);
                            
                            // 锟斤拷录锟斤拷锟斤拷时锟斤拷锟斤拷图锟斤拷锟斤拷锟斤拷锟疥（锟斤拷锟节伙拷锟狡ｏ拷
                            g_currentMeasurementLocation.centerX = aprCtx->centerX;
                            g_currentMeasurementLocation.centerY = aprCtx->centerY;
                            g_currentMeasurementLocation.centerZ = aprCtx->centerZ;
                            
                            // 锟斤拷录锟斤拷前锟斤拷片锟斤拷锟斤拷
                            if (aprCtx->sliceDirection == 0) {
                                g_currentMeasurementLocation.sliceIndex = (int)(aprCtx->centerZ + 0.5f);
                            } else if (aprCtx->sliceDirection == 1) {
                                g_currentMeasurementLocation.sliceIndex = (int)(aprCtx->centerY + 0.5f);
                            } else {
                                g_currentMeasurementLocation.sliceIndex = (int)(aprCtx->centerX + 0.5f);
                            }
                        }
                        
                        const char* dirNames[] = {"Axial", "Coronal", "Sagittal"};
                        printf("  -> Measurement location: %s view, slice %d\n", 
                               dirNames[g_currentMeasurementLocation.sliceDirection],
                               g_currentMeasurementLocation.sliceIndex);
                    }
                    
                    switch (g_currentToolType) {
                        case 1: // 直锟斤拷 - 锟斤拷锟铰匡拷始锟较讹拷
                        case 3: // 锟斤拷锟斤拷 - 锟斤拷锟铰匡拷始锟较讹拷  
                        case 4: // 圆锟斤拷 - 锟斤拷锟铰匡拷始锟较讹拷
                            g_isDrawing = true;
                            g_measurementPoints.clear();
                            g_measurementPoints.push_back(worldPos);
                            g_currentMousePos = worldPos;
                            printf("  -> Started dragging\n");
                            break;
                            
                        case 2: // 锟角讹拷 - 锟斤拷要锟斤拷锟斤拷锟斤拷
                            g_measurementPoints.push_back(worldPos);
                            printf("  -> Angle point %zu/3\n", g_measurementPoints.size());
                            if (g_measurementPoints.size() == 3) {
                                // 锟斤拷锟斤拷嵌炔锟斤拷锟斤拷妫拷锟轿伙拷锟斤拷锟较拷锟?
                                float angle = CalculateAngle(g_measurementPoints[0], g_measurementPoints[1], g_measurementPoints[2]);
                                printf("  -> Angle = %.2f degrees\n", angle);
                                CompletedMeasurement measurement;
                                measurement.sessionId = GetSessionIdFromWindowContext(ctx);
                                measurement.id = g_nextMeasurementId++;
                                measurement.toolType = 2;
                                measurement.points = g_measurementPoints;
                                measurement.result = angle;
                                measurement.location = g_currentMeasurementLocation;
                                {
                                    std::lock_guard<std::mutex> lock(g_completedMeasurementsMutex);
                                    g_completedMeasurements.push_back(measurement);
                                }
                                g_measurementPoints.clear();  // 锟斤拷锟阶硷拷锟斤拷锟揭伙拷锟斤拷嵌锟?
                            }
                            break;
                            
                        case 5: // Catmull-Rom锟斤拷锟斤拷 - 锟斤拷悖拷锟斤拷锟斤拷锟?
                            if (isDoubleClick && g_measurementPoints.size() >= 2) {
                                printf("  -> Spline completed with %zu points\n", g_measurementPoints.size());
                                CompletedMeasurement measurement;
                                measurement.sessionId = GetSessionIdFromWindowContext(ctx);
                                measurement.id = g_nextMeasurementId++;
                                measurement.toolType = 5;
                                measurement.points = g_measurementPoints;
                                measurement.result = CalculatePolylineLength(measurement.points);
                                measurement.location = g_currentMeasurementLocation;
                                {
                                    std::lock_guard<std::mutex> lock(g_completedMeasurementsMutex);
                                    g_completedMeasurements.push_back(measurement);
                                }
                                g_measurementPoints.clear();
                            } else {
                                g_measurementPoints.push_back(worldPos);
                                printf("  -> Spline point %zu added\n", g_measurementPoints.size());
                            }
                            break;
                            
                        case 6: // 锟斤拷锟斤拷锟斤拷锟斤拷 - 锟斤拷锟铰匡拷始锟斤拷录路锟斤拷
                            g_isDrawing = true;
                            g_measurementPoints.clear();
                            g_measurementPoints.push_back(worldPos);
                            printf("  -> Started freehand drawing\n");
                            break;
                    }
                    
                    Tool_AddPoint(ctx->activeTool, ndcX, ndcY);
                }
            }
            else if (action == GLFW_RELEASE) {
                printf("[%s Mouse] Left Release\n", ctx->rendererType == 1 ? "MPR" : "APR");
                
                if (g_isDrawing) {
                    // 转锟斤拷锟酵凤拷位锟斤拷为3D锟斤拷锟斤拷
                    MeasurementPoint releasePos;
                    if (ctx->rendererType == 1) {
                        auto mprCtx = static_cast<MPRContext*>(ctx->boundRenderer);
                        releasePos = MPR_NDCToWorld(ndcX, ndcY, mprCtx);
                    } else {
                        auto aprCtx = static_cast<APRContext*>(ctx->boundRenderer);
                        releasePos = APR_NDCToWorld(ndcX, ndcY, aprCtx);
                    }
                    
                    switch (g_currentToolType) {
                        case 1: // 直锟斤拷 - 锟斤拷锟斤拷锟斤拷锟?
                            if (g_measurementPoints.size() >= 1) {
                                g_measurementPoints.push_back(releasePos);
                                float distance = CalculateDistance(g_measurementPoints[0], g_measurementPoints[1]);
                                printf("  -> Line distance = %.2f mm\n", distance);
                                CompletedMeasurement measurement;
                                measurement.sessionId = GetSessionIdFromWindowContext(ctx);
                                measurement.id = g_nextMeasurementId++;
                                measurement.toolType = 1;
                                measurement.points = g_measurementPoints;
                                measurement.result = distance;
                                measurement.location = g_currentMeasurementLocation;
                                {
                                    std::lock_guard<std::mutex> lock(g_completedMeasurementsMutex);
                                    g_completedMeasurements.push_back(measurement);
                                }
                                g_measurementPoints.clear();
                            }
                            break;
                            
                        case 3: // 锟斤拷锟斤拷 - 锟斤拷锟斤拷锟斤拷锟?
                            if (g_measurementPoints.size() >= 1) {
                                g_measurementPoints.push_back(releasePos);
                                float area = CalculateRectangleAreaInPlane(g_measurementPoints[0], g_measurementPoints[1], g_currentMeasurementLocation.sliceDirection);
                                printf("  -> Rectangle area = %.2f mm?\n", area);
                                CompletedMeasurement measurement;
                                measurement.sessionId = GetSessionIdFromWindowContext(ctx);
                                measurement.id = g_nextMeasurementId++;
                                measurement.toolType = 3;
                                measurement.points = g_measurementPoints;
                                measurement.result = area;
                                measurement.location = g_currentMeasurementLocation;
                                {
                                    std::lock_guard<std::mutex> lock(g_completedMeasurementsMutex);
                                    g_completedMeasurements.push_back(measurement);
                                }
                                g_measurementPoints.clear();
                            }
                            break;
                            
                        case 4: // 鍦嗗舰 - 鎷栨嫿瀹屾垚
                            if (g_measurementPoints.size() >= 1) {
                                MeasurementPoint startPos = g_measurementPoints[0];
                                MeasurementPoint endPos = releasePos;

                                // Shift绾︽潫锛氬己鍒舵鍦?
                                if (g_shiftPressed) {
                                    float dx = endPos.x - startPos.x;
                                    float dy = endPos.y - startPos.y;
                                    float dz = endPos.z - startPos.z;
                                    int sliceDir = g_currentMeasurementLocation.sliceDirection;
                                    if (sliceDir == 0) {
                                        float size = fmaxf(fabsf(dx), fabsf(dy));
                                        endPos.x = startPos.x + (dx >= 0 ? size : -size);
                                        endPos.y = startPos.y + (dy >= 0 ? size : -size);
                                    } else if (sliceDir == 1) {
                                        float size = fmaxf(fabsf(dx), fabsf(dz));
                                        endPos.x = startPos.x + (dx >= 0 ? size : -size);
                                        endPos.z = startPos.z + (dz >= 0 ? size : -size);
                                    } else {
                                        float size = fmaxf(fabsf(dy), fabsf(dz));
                                        endPos.y = startPos.y + (dy >= 0 ? size : -size);
                                        endPos.z = startPos.z + (dz >= 0 ? size : -size);
                                    }
                                }

                                g_measurementPoints.push_back(endPos);
                                float area = CalculateEllipseAreaInPlane(g_measurementPoints[0], g_measurementPoints[1], g_currentMeasurementLocation.sliceDirection);
                                printf("  -> Ellipse area = %.2f mm^2\n", area);
                                CompletedMeasurement measurement;
                                measurement.sessionId = GetSessionIdFromWindowContext(ctx);
                                measurement.id = g_nextMeasurementId++;
                                measurement.toolType = 4;
                                measurement.points = g_measurementPoints;
                                measurement.result = area;
                                measurement.location = g_currentMeasurementLocation;
                                {
                                    std::lock_guard<std::mutex> lock(g_completedMeasurementsMutex);
                                    g_completedMeasurements.push_back(measurement);
                                }
                                g_measurementPoints.clear();
                            }
                            break;
                            
                        case 6: // 锟斤拷锟斤拷锟斤拷锟斤拷 - 锟斤拷刹锟斤拷蘸锟?
                            printf("  -> Freehand completed with %zu points\n", g_measurementPoints.size());
                            if (g_measurementPoints.size() >= 3) {
                                CompletedMeasurement measurement;
                                measurement.sessionId = GetSessionIdFromWindowContext(ctx);
                                measurement.id = g_nextMeasurementId++;
                                measurement.toolType = 6;
                                measurement.points = g_measurementPoints;
                                measurement.result = CalculatePolylineLength(measurement.points);
                                measurement.location = g_currentMeasurementLocation;
                                {
                                    std::lock_guard<std::mutex> lock(g_completedMeasurementsMutex);
                                    g_completedMeasurements.push_back(measurement);
                                }
                            }
                            g_measurementPoints.clear();
                            break;
                    }
                    g_isDrawing = false;
                }
            }
        }
        else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
            printf("[MPR Mouse] Right Click - Clear current drawing\n");
            g_measurementPoints.clear();
            g_isDrawing = false;
        }
        
        return; // MPR锟斤拷锟斤拷锟斤拷锟竭达拷锟斤拷锟斤拷希锟斤拷锟斤拷锟街达拷锟斤拷锟斤拷锟侥诧拷锟叫匡拷锟竭硷拷
    }
    
    // MPR 锟斤拷锟节ｏ拷锟斤拷锟斤拷 Mask 锟洁辑锟斤拷锟竭碉拷锟?(tool 7=MaskEdit, 锟斤拷MPR支锟斤拷)
    if (ctx->rendererType == 1 && ctx->boundRenderer &&
        g_currentToolType == 7) {
        
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                // 锟斤拷始锟斤拷锟狡笔伙拷
                g_isDrawing = true;
                g_maskStrokePath.clear();  // 锟斤拷锟街帮拷锟铰凤拷锟?
                
                auto mprCtx = static_cast<MPRContext*>(ctx->boundRenderer);
                const char* maskToolNames[] = {"", "Brush", "Eraser", "RectROI", "CircleROI", "PolygonROI", "FloodFill", "ConnectedComp"};

                const char* maskName = nullptr;
                if (!GetCurrentEditableMaskData(mprCtx, &maskName)) {
                    g_isDrawing = false;
                    printf("[MPR Mask] No editable mask selected (sessionId=%s)\n", mprCtx->sessionId.c_str());
                    return;
                }
                
                printf("[MPR Mask] Started %s stroke (mask %d '%s', radius=%.1f, slice=%d)\n", 
                       maskToolNames[g_currentMaskTool],
                       (g_currentMaskManager ? g_currentMaskIndex : g_currentSessionMaskId),
                       maskName ? maskName : "Unknown", g_brushRadius,
                       mprCtx->sliceDirection == 0 ? (int)mprCtx->centerZ : 
                       (mprCtx->sliceDirection == 1 ? (int)mprCtx->centerY : (int)mprCtx->centerX));
                
                // 锟斤拷臧达拷锟绞憋拷锟斤拷锟斤拷锟斤拷锟绞硷拷悖拷锟斤拷锟斤拷锟狡讹拷锟铰硷拷锟斤拷锟斤拷锟斤拷
                // 锟斤拷锟斤拷锟斤拷锟斤拷转锟斤拷锟竭硷拷统一锟斤拷OnCursorPos锟斤拷
            }
            else if (action == GLFW_RELEASE) {
                // 锟斤拷锟教э拷锟绞癸拷锟斤拷锟绞?D锟斤拷锟斤拷锟斤拷锟斤拷锟劫伙拷锟狡ｏ拷然锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷mask
                if (g_isDrawing && !g_maskStrokePath.empty()) {
                    auto mprCtx = static_cast<MPRContext*>(ctx->boundRenderer);
                    
                    // 锟斤拷取锟斤拷锟斤拷锟斤拷维锟斤拷
                    int width, height, depth;
                    Dicom_Volume_GetDimensions(mprCtx->volume, &width, &height, &depth);
                    
                    // 锟斤拷取锟斤拷锟截硷拷锟?
                    float spacingX, spacingY, spacingZ;
                    Dicom_Volume_GetSpacing(mprCtx->volume, &spacingX, &spacingY, &spacingZ);
                    
                    // 确锟斤拷2D锟斤拷片锟侥尺寸（锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
                    int sliceWidth, sliceHeight;
                    if (mprCtx->sliceDirection == 0) {  // Axial (XY)
                        sliceWidth = width;
                        sliceHeight = height;
                    } else if (mprCtx->sliceDirection == 1) {  // Coronal (XZ)
                        sliceWidth = width;
                        sliceHeight = depth;
                    } else {  // Sagittal (YZ)
                        sliceWidth = height;
                        sliceHeight = depth;
                    }
                    
                    // 锟斤拷锟斤拷锟斤拷时2D锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷片一锟斤拷锟斤拷
                    std::vector<uint8_t> tempSlice(sliceWidth * sliceHeight, 0);
                    
                    bool isErase = (g_currentMaskTool == 2);  // 2=Eraser
                    
                    // 锟斤拷锟斤拷1: 锟斤拷锟斤拷时2D锟斤拷锟斤拷锟斤拷锟较伙拷锟斤拷圆锟轿笔伙拷
                    auto drawCircleOnTempSlice = [&](float centerU, float centerV) {
                        // centerU, centerV 锟窖撅拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
                        int cx = (int)(centerU + 0.5f);
                        int cy = (int)(centerV + 0.5f);
                        int radius = (int)(g_brushRadius + 0.5f); // 锟斤拷锟绞半径锟斤拷锟斤拷锟截碉拷位锟斤拷
                        
                        int minU = std::max(0, cx - radius);
                        int maxU = std::min(sliceWidth - 1, cx + radius);
                        int minV = std::max(0, cy - radius);
                        int maxV = std::min(sliceHeight - 1, cy + radius);
                        
                        float radiusSq = g_brushRadius * g_brushRadius;
                        
                        for (int v = minV; v <= maxV; v++) {
                            for (int u = minU; u <= maxU; u++) {
                                float du = u - centerU;
                                float dv = v - centerV;
                                float distSq = du * du + dv * dv;
                                
                                if (distSq <= radiusSq) {
                                    tempSlice[v * sliceWidth + u] = 1;  // 锟斤拷锟轿拷锟揭拷薷锟?
                                }
                            }
                        }
                    };
                    
                    // 锟斤拷路锟斤拷锟斤拷每锟斤拷锟斤拷之锟斤拷锟街碉拷锟斤拷锟?
                    if (g_maskStrokePath.size() == 1) {
                        drawCircleOnTempSlice(g_maskStrokePath[0].x, g_maskStrokePath[0].y);
                    } else {
                        for (size_t i = 1; i < g_maskStrokePath.size(); i++) {
                            Point2D& p1 = g_maskStrokePath[i - 1];
                            Point2D& p2 = g_maskStrokePath[i];
                            
                            float dx = p2.x - p1.x;
                            float dy = p2.y - p1.y;
                            float dist = sqrtf(dx * dx + dy * dy);
                            
                            int steps = (int)(dist / 0.5f);  // 每0.5锟斤拷锟截诧拷值
                            if (steps < 1) steps = 1;
                            
                            for (int j = 0; j <= steps; j++) {
                                float t = (float)j / steps;
                                float x = p1.x + dx * t;
                                float y = p1.y + dy * t;
                                drawCircleOnTempSlice(x, y);
                            }
                        }
                    }
                    
                    // 统锟斤拷锟斤拷时锟斤拷锟斤拷锟斤拷锟叫憋拷锟轿?锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
                    int markedPixels = 0;
                    for (int i = 0; i < sliceWidth * sliceHeight; i++) {
                        if (tempSlice[i] == 1) markedPixels++;
                    }
                    
                    // 锟斤拷锟斤拷2: 锟斤拷锟斤拷锟斤拷时锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷3D mask
                    const char* maskName = nullptr;
                    uint8_t* maskData = GetCurrentEditableMaskData(mprCtx, &maskName);
                    
                    if (maskData) {
                        // 锟斤拷前锟斤拷片锟斤拷锟斤拷
                        int sliceIndex;
                        if (mprCtx->sliceDirection == 0) {  // Axial
                            sliceIndex = (int)(mprCtx->centerZ + 0.5f);
                        } else if (mprCtx->sliceDirection == 1) {  // Coronal
                            sliceIndex = (int)(mprCtx->centerY + 0.5f);
                        } else {  // Sagittal
                            sliceIndex = (int)(mprCtx->centerX + 0.5f);
                        }
                        
                        
                        // 锟斤拷锟斤拷锟斤拷片锟斤拷锟津，斤拷2D锟斤拷锟斤拷映锟戒到3D mask
                        int modifiedVoxels = 0;
                        for (int v = 0; v < sliceHeight; v++) {
                            for (int u = 0; u < sliceWidth; u++) {
                                if (tempSlice[v * sliceWidth + u] == 1) {
                                    int x, y, z;
                                    
                                    if (mprCtx->sliceDirection == 0) {  // Axial (XY平锟芥，Z锟教讹拷)
                                        x = u;
                                        y = v;
                                        z = sliceIndex;
                                    } else if (mprCtx->sliceDirection == 1) {  // Coronal (XZ平锟芥，Y锟教讹拷)
                                        x = u;
                                        y = sliceIndex;
                                        z = v;
                                    } else {  // Sagittal (YZ平锟芥，X锟教讹拷)
                                        x = sliceIndex;
                                        y = u;
                                        z = v;
                                    }
                                    
                                    // 锟竭斤拷锟斤拷
                                    if (x >= 0 && x < width && y >= 0 && y < height && z >= 0 && z < depth) {
                                        int index3D = z * width * height + y * width + x;
                                        if (isErase) {
                                            maskData[index3D] = 0;
                                        } else {
                                            maskData[index3D] = 255;
                                        }
                                        modifiedVoxels++;
                                    }
                                }
                            }
                        }
                        printf("[DEBUG] Modified %d voxels in 3D mask\n", modifiedVoxels);
                    } else {
                        printf("[ERROR] maskData is NULL!\n");
                    }
                    
                    // 锟斤拷锟斤拷锟揭拷锟斤拷锟絤ask锟斤拷锟斤拷锟斤拷锟斤拷
                    g_maskStrokeNeedsUpdate = true;
                    
                    const char* maskToolNames[] = {"", "Brush", "Eraser", "RectROI", "CircleROI", "PolygonROI", "FloodFill", "ConnectedComp"};
                    printf("[MPR Mask] Finished %s stroke (%zu points, temp buffer method)\n", 
                           maskToolNames[g_currentMaskTool], g_maskStrokePath.size());
                }
                
                // 停止锟斤拷锟斤拷
                g_isDrawing = false;
                g_maskStrokePath.clear();
            }
        }
        else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
            // 锟揭硷拷取锟斤拷
            g_isDrawing = false;
            g_maskStrokePath.clear();
            printf("[Mask] Right click - cancelled stroke\n");
        }
        
        return; // Mask锟斤拷锟竭达拷锟斤拷锟斤拷锟?
    }
    
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            // 锟斤拷锟斤拷欠锟斤拷锟斤拷瞬锟斤拷锌锟侥匡拷锟狡碉拷
            bool clickedCropBox = false;
            
            if (g_aprCropBox.enabled && (ctx->rendererType == 0 || ctx->rendererType == 2) && ctx->boundRenderer) {
                auto aprCtx = static_cast<APRContext*>(ctx->boundRenderer);
                if (aprCtx->volume) {
                    // 锟斤拷取锟斤拷锟斤拷锟捷尺达拷
                    int width, height, depth;
                    if (Dicom_Volume_GetDimensions(aprCtx->volume, &width, &height, &depth) != NATIVE_OK) return;
                    
                    // 确锟斤拷锟斤拷前锟斤拷片锟斤拷实锟绞尺寸（锟斤拷锟截ｏ拷
                    int sliceWidth = 0, sliceHeight = 0;
                    if (aprCtx->sliceDirection == 0) {
                        sliceWidth = width; sliceHeight = height;
                    } else if (aprCtx->sliceDirection == 1) {
                        sliceWidth = width; sliceHeight = depth;
                    } else {
                        sliceWidth = height; sliceHeight = depth;
                    }
                    
                    // 锟斤拷锟斤拷图锟斤拷锟节达拷锟斤拷锟叫碉拷锟斤拷示锟斤拷锟斤拷
                    int winWidth, winHeight;
                    glfwGetFramebufferSize(window, &winWidth, &winHeight);
                    double mouseX, mouseY;
                    glfwGetCursorPos(window, &mouseX, &mouseY);

                    const float ndcX = (static_cast<float>(mouseX) / std::max(1.0f, static_cast<float>(winWidth))) * 2.0f - 1.0f;
                    const float ndcY = 1.0f - (static_cast<float>(mouseY) / std::max(1.0f, static_cast<float>(winHeight))) * 2.0f;

                    const float crossTexX = aprCtx->crosshairU / std::max(1.0f, (sliceWidth - 1.0f));
                    const float crossTexY = 1.0f - (aprCtx->crosshairV / std::max(1.0f, (sliceHeight - 1.0f)));
                    const TexWindowMapping map = ComputeTexWindowMapping(sliceWidth, sliceHeight, winWidth, winHeight, aprCtx->zoomFactor, crossTexX, crossTexY);
                    if (!map.valid || ndcX < map.baseLeft || ndcX > map.baseRight || ndcY < map.baseBottom || ndcY > map.baseTop) {
                        // Clicked in letterbox/empty area.
                        return;
                    }

                    const float relX = (ndcX - map.baseLeft) / std::max(1e-6f, (map.baseRight - map.baseLeft));
                    const float relY = (ndcY - map.baseBottom) / std::max(1e-6f, (map.baseTop - map.baseBottom));
                    const float texU = map.texLeft + relX * (map.texRight - map.texLeft);
                    const float texV = map.texBottom + relY * (map.texTop - map.texBottom);
                    const float imgX = texU * std::max(1.0f, (sliceWidth - 1.0f));
                    const float imgY = (1.0f - texV) * std::max(1.0f, (sliceHeight - 1.0f));
                    
                    // 锟斤拷锟斤拷锟斤拷图锟斤拷锟津，伙拷取锟斤拷锟叫匡拷锟节碉拷前平锟斤拷谋呓纾拷锟斤拷锟斤拷锟斤拷辏?
                    float boxLeft = 0, boxRight = 0, boxTop = 0, boxBottom = 0;
                    if (aprCtx->sliceDirection == 0) {  // Axial
                        boxLeft = g_aprCropBox.xStart;
                        boxRight = g_aprCropBox.xEnd;
                        boxTop = g_aprCropBox.yStart;
                        boxBottom = g_aprCropBox.yEnd;
                    } else if (aprCtx->sliceDirection == 1) {  // Coronal
                        boxLeft = g_aprCropBox.xStart;
                        boxRight = g_aprCropBox.xEnd;
                        boxTop = g_aprCropBox.zStart;
                        boxBottom = g_aprCropBox.zEnd;
                    } else {  // Sagittal
                        boxLeft = g_aprCropBox.yStart;
                        boxRight = g_aprCropBox.yEnd;
                        boxTop = g_aprCropBox.zStart;
                        boxBottom = g_aprCropBox.zEnd;
                    }
                    
                    // Threshold in image pixels equivalent to ~10 screen pixels.
                    const float ndcPerPxX = 2.0f / std::max(1.0f, static_cast<float>(winWidth));
                    const float ndcPerPxY = 2.0f / std::max(1.0f, static_cast<float>(winHeight));
                    const float duPerNdc = (map.texRight - map.texLeft) / std::max(1e-6f, (map.baseRight - map.baseLeft));
                    const float dvPerNdc = (map.texTop - map.texBottom) / std::max(1e-6f, (map.baseTop - map.baseBottom));
                    const float thresholdX = 10.0f * ndcPerPxX * duPerNdc * std::max(1.0f, (sliceWidth - 1.0f));
                    const float thresholdY = 10.0f * ndcPerPxY * dvPerNdc * std::max(1.0f, (sliceHeight - 1.0f));
                    
                    // 锟斤拷锟斤拷母锟斤拷堑锟?
                    if (std::abs(imgX - boxLeft) < thresholdX && std::abs(imgY - boxTop) < thresholdY) {
                        g_aprCropBox.dragCorner = 0;  // 锟斤拷锟较斤拷
                        g_aprCropBox.dragDirection = aprCtx->sliceDirection;
                        clickedCropBox = true;
                    } else if (std::abs(imgX - boxRight) < thresholdX && std::abs(imgY - boxTop) < thresholdY) {
                        g_aprCropBox.dragCorner = 1;  // 锟斤拷锟较斤拷
                        g_aprCropBox.dragDirection = aprCtx->sliceDirection;
                        clickedCropBox = true;
                    } else if (std::abs(imgX - boxLeft) < thresholdX && std::abs(imgY - boxBottom) < thresholdY) {
                        g_aprCropBox.dragCorner = 2;  // 锟斤拷锟铰斤拷
                        g_aprCropBox.dragDirection = aprCtx->sliceDirection;
                        clickedCropBox = true;
                    } else if (std::abs(imgX - boxRight) < thresholdX && std::abs(imgY - boxBottom) < thresholdY) {
                        g_aprCropBox.dragCorner = 3;  // 锟斤拷锟铰斤拷
                        g_aprCropBox.dragDirection = aprCtx->sliceDirection;
                        clickedCropBox = true;
                    }
                    // 锟斤拷锟斤拷锟斤拷锟斤拷叩锟斤拷械锟?
                    else {
                        float midX = (boxLeft + boxRight) / 2.0f;
                        float midY = (boxTop + boxBottom) / 2.0f;

                        if (std::abs(static_cast<float>(imgX) - midX) < thresholdX && std::abs(static_cast<float>(imgY) - boxTop) < thresholdY) {
                            g_aprCropBox.dragEdge = 0;  // 锟较憋拷
                            g_aprCropBox.dragDirection = aprCtx->sliceDirection;
                            clickedCropBox = true;
                        } else if (std::abs(static_cast<float>(imgX) - boxRight) < thresholdX && std::abs(static_cast<float>(imgY) - midY) < thresholdY) {
                            g_aprCropBox.dragEdge = 1;  // 锟揭憋拷
                            g_aprCropBox.dragDirection = aprCtx->sliceDirection;
                            clickedCropBox = true;
                        } else if (std::abs(imgX - midX) < thresholdX && std::abs(imgY - boxBottom) < thresholdY) {
                            g_aprCropBox.dragEdge = 2;  // 锟铰憋拷
                            g_aprCropBox.dragDirection = aprCtx->sliceDirection;
                            clickedCropBox = true;
                        } else if (std::abs(imgX - boxLeft) < thresholdX && std::abs(imgY - midY) < thresholdY) {
                            g_aprCropBox.dragEdge = 3;  // 锟斤拷锟?
                            g_aprCropBox.dragDirection = aprCtx->sliceDirection;
                            clickedCropBox = true;
                        }
                        // 锟斤拷锟斤拷欠锟斤拷诳锟斤拷锟?+ Shift锟斤拷锟斤拷锟狡讹拷锟斤拷锟斤拷锟斤拷
                        else if ((mods & GLFW_MOD_SHIFT) && 
                                imgX >= boxLeft && imgX <= boxRight &&
                                imgY >= boxTop && imgY <= boxBottom) {
                            g_aprCropBox.isDraggingBox = true;
                            g_aprCropBox.dragDirection = aprCtx->sliceDirection;
                            g_aprCropBox.dragStartMouseX = mouseX;
                            g_aprCropBox.dragStartMouseY = mouseY;
                            // 锟斤拷锟芥当前锟斤拷锟叫匡拷锟斤拷锟斤拷斜呓锟?
                            g_aprCropBox.dragStartXStart = g_aprCropBox.xStart;
                            g_aprCropBox.dragStartXEnd = g_aprCropBox.xEnd;
                            g_aprCropBox.dragStartYStart = g_aprCropBox.yStart;
                            g_aprCropBox.dragStartYEnd = g_aprCropBox.yEnd;
                            g_aprCropBox.dragStartZStart = g_aprCropBox.zStart;
                            g_aprCropBox.dragStartZEnd = g_aprCropBox.zEnd;
                            clickedCropBox = true;
                        }
                    }
                }
            }
            
            if (clickedCropBox) {
                g_aprCropBox.isDragging = true;
            } else {
                ctx->isDragging = true;
            }
            glfwGetCursorPos(window, &ctx->lastMouseX, &ctx->lastMouseY);
        } else if (action == GLFW_RELEASE) {
            ctx->isDragging = false;
            g_aprCropBox.isDragging = false;
            g_aprCropBox.isDraggingBox = false;
            g_aprCropBox.dragCorner = -1;
            g_aprCropBox.dragEdge = -1;
        }
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            // 锟斤拷锟斤拷欠锟斤拷锟?3D 锟斤拷锟斤拷锟斤拷图锟斤拷锟节ｏ拷没锟叫帮拷锟斤拷染锟斤拷锟斤拷
            if (!ctx->boundRenderer) {
                g_3dRotating = true;
                glfwGetCursorPos(window, &g_3dLastMouseX, &g_3dLastMouseY);
            } else {
                ctx->isRightDragging = true;
                glfwGetCursorPos(window, &ctx->lastMouseX, &ctx->lastMouseY);
            }
        } else if (action == GLFW_RELEASE) {
            g_3dRotating = false;
            ctx->isRightDragging = false;
        }
    }
}

static void OnCursorPos(GLFWwindow* window, double xpos, double ypos) {
    WindowContext* ctx = static_cast<WindowContext*>(glfwGetWindowUserPointer(window));
    if (!ctx) return;
    
    // MPR/APR锟斤拷锟斤拷锟斤拷锟竭ｏ拷锟斤拷锟铰碉拷前锟斤拷锟轿伙拷锟斤拷锟斤拷锟绞凳痹わ拷锟?(只锟斤拷tool 1-6时锟斤拷锟斤拷)
    if ((ctx->rendererType == 1 || ctx->rendererType == 2) && ctx->boundRenderer && ctx->activeTool && g_currentToolType >= 1 && g_currentToolType <= 6) {
        const float winW = std::max(1.0f, static_cast<float>(ctx->width));
        const float winH = std::max(1.0f, static_cast<float>(ctx->height));
        const float ndcX = (static_cast<float>(xpos) / winW) * 2.0f - 1.0f;
        const float ndcY = 1.0f - (static_cast<float>(ypos) / winH) * 2.0f;

        // Convert screen NDC -> image/view NDC (the coordinate system expected by *_NDCToWorld).
        if (ctx->rendererType == 1) {
            auto mprCtx = static_cast<MPRContext*>(ctx->boundRenderer);
            if (mprCtx->volume) {
                int width = 0, height = 0, depth = 0;
                if (Dicom_Volume_GetDimensions(mprCtx->volume, &width, &height, &depth) == NATIVE_OK) {
                    int sliceWidth = 0, sliceHeight = 0;
                    if (mprCtx->sliceDirection == 0) { sliceWidth = width; sliceHeight = height; }
                    else if (mprCtx->sliceDirection == 1) { sliceWidth = width; sliceHeight = depth; }
                    else { sliceWidth = height; sliceHeight = depth; }

                    float crossTexX = 0.5f, crossTexY = 0.5f;
                    if (mprCtx->sliceDirection == 0) {
                        crossTexX = mprCtx->centerX / std::max(1.0f, (width - 1.0f));
                        crossTexY = 1.0f - (mprCtx->centerY / std::max(1.0f, (height - 1.0f)));
                    } else if (mprCtx->sliceDirection == 1) {
                        crossTexX = mprCtx->centerX / std::max(1.0f, (width - 1.0f));
                        crossTexY = 1.0f - (mprCtx->centerZ / std::max(1.0f, (depth - 1.0f)));
                    } else {
                        crossTexX = mprCtx->centerY / std::max(1.0f, (height - 1.0f));
                        crossTexY = 1.0f - (mprCtx->centerZ / std::max(1.0f, (depth - 1.0f)));
                    }

                    const TexWindowMapping map = ComputeTexWindowMapping(sliceWidth, sliceHeight, ctx->width, ctx->height, mprCtx->zoomFactor, crossTexX, crossTexY);
                    if (map.valid && ndcX >= map.baseLeft && ndcX <= map.baseRight && ndcY >= map.baseBottom && ndcY <= map.baseTop) {
                        const float relX = (ndcX - map.baseLeft) / std::max(1e-6f, (map.baseRight - map.baseLeft));
                        const float relY = (ndcY - map.baseBottom) / std::max(1e-6f, (map.baseTop - map.baseBottom));
                        const float imageNdcX = relX * 2.0f - 1.0f;
                        const float imageNdcY = relY * 2.0f - 1.0f;
                        g_currentMousePos = MPR_NDCToWorld(imageNdcX, imageNdcY, mprCtx);
                    }
                }
            }
        } else {
            auto aprCtx = static_cast<APRContext*>(ctx->boundRenderer);
            if (aprCtx->volume) {
                int width = 0, height = 0, depth = 0;
                if (Dicom_Volume_GetDimensions(aprCtx->volume, &width, &height, &depth) == NATIVE_OK) {
                    int sliceWidth = 0, sliceHeight = 0;
                    if (aprCtx->sliceDirection == 0) { sliceWidth = width; sliceHeight = height; }
                    else if (aprCtx->sliceDirection == 1) { sliceWidth = width; sliceHeight = depth; }
                    else { sliceWidth = height; sliceHeight = depth; }

                    const float crossTexX = aprCtx->crosshairU / std::max(1.0f, (sliceWidth - 1.0f));
                    const float crossTexY = 1.0f - (aprCtx->crosshairV / std::max(1.0f, (sliceHeight - 1.0f)));
                    const TexWindowMapping map = ComputeTexWindowMapping(sliceWidth, sliceHeight, ctx->width, ctx->height, aprCtx->zoomFactor, crossTexX, crossTexY);
                    if (map.valid && ndcX >= map.baseLeft && ndcX <= map.baseRight && ndcY >= map.baseBottom && ndcY <= map.baseTop) {
                        const float relX = (ndcX - map.baseLeft) / std::max(1e-6f, (map.baseRight - map.baseLeft));
                        const float relY = (ndcY - map.baseBottom) / std::max(1e-6f, (map.baseTop - map.baseBottom));
                        const float imageNdcX = relX * 2.0f - 1.0f;
                        const float imageNdcY = relY * 2.0f - 1.0f;
                        g_currentMousePos = APR_NDCToWorld(imageNdcX, imageNdcY, aprCtx);
                    }
                }
            }
        }
        
        // 锟斤拷锟斤拷锟斤拷锟竭癸拷锟竭ｏ拷锟斤拷录锟斤拷锟铰凤拷锟?
        if (g_isDrawing && g_currentToolType == 6) {
            // 锟斤拷锟斤拷锟教拷芗锟斤拷锟街伙拷锟斤拷贫锟斤拷愎伙拷锟斤拷锟斤拷锟斤拷锟斤拷樱锟绞癸拷锟?D锟斤拷锟诫）
            if (g_measurementPoints.empty() || 
                CalculateDistance(g_measurementPoints.back(), g_currentMousePos) > 1.0f) {  // 1mm锟斤拷值
                g_measurementPoints.push_back(g_currentMousePos);
            }
        }
    }
    
    // MPR Mask锟洁辑锟斤拷锟竭ｏ拷锟斤拷锟铰碉拷前锟斤拷锟轿伙拷貌锟斤拷锟斤拷拥锟斤拷驶锟铰凤拷锟?(tool 7=MaskEdit, 锟斤拷MPR支锟斤拷)
    if (ctx->rendererType == 1 && ctx->boundRenderer && 
        g_currentToolType == 7 && 
        (g_currentMaskManager != nullptr || (!g_currentSessionMaskEditSessionId.empty() && g_currentSessionMaskId >= 0))) {
        
        auto mprCtx = static_cast<MPRContext*>(ctx->boundRenderer);

        // No editable mask selected for this session/window
        if (!GetCurrentEditableMaskData(mprCtx, nullptr)) {
            return;
        }
        
        // 锟斤拷锟饺革拷锟斤拷 g_currentMousePos 锟斤拷锟斤拷锟斤拷示锟斤拷锟斤拷圆圈
        float ndcX = (float)(xpos / ctx->width) * 2.0f - 1.0f;
        float ndcY = 1.0f - (float)(ypos / ctx->height) * 2.0f;
        
        // 锟斤拷取锟斤拷锟斤拷锟斤拷维锟斤拷
        int width, height, depth;
        Dicom_Volume_GetDimensions(mprCtx->volume, &width, &height, &depth);
        
        // 确锟斤拷锟斤拷前锟斤拷片锟侥尺达拷
        int sliceWidth, sliceHeight;
        if (mprCtx->sliceDirection == 0) {  // Axial (XY)
            sliceWidth = width;
            sliceHeight = height;
        } else if (mprCtx->sliceDirection == 1) {  // Coronal (XZ)
            sliceWidth = width;
            sliceHeight = depth;
        } else {  // Sagittal (YZ)
            sliceWidth = height;
            sliceHeight = depth;
        }

        // Update g_currentMousePos using image/view NDC (not raw screen NDC).
        {
            float crossTexX = 0.5f, crossTexY = 0.5f;
            if (mprCtx->sliceDirection == 0) {
                crossTexX = mprCtx->centerX / std::max(1.0f, (width - 1.0f));
                crossTexY = 1.0f - (mprCtx->centerY / std::max(1.0f, (height - 1.0f)));
            } else if (mprCtx->sliceDirection == 1) {
                crossTexX = mprCtx->centerX / std::max(1.0f, (width - 1.0f));
                crossTexY = 1.0f - (mprCtx->centerZ / std::max(1.0f, (depth - 1.0f)));
            } else {
                crossTexX = mprCtx->centerY / std::max(1.0f, (height - 1.0f));
                crossTexY = 1.0f - (mprCtx->centerZ / std::max(1.0f, (depth - 1.0f)));
            }

            const TexWindowMapping map = ComputeTexWindowMapping(sliceWidth, sliceHeight, ctx->width, ctx->height, mprCtx->zoomFactor, crossTexX, crossTexY);
            if (map.valid && ndcX >= map.baseLeft && ndcX <= map.baseRight && ndcY >= map.baseBottom && ndcY <= map.baseTop) {
                const float relX = (ndcX - map.baseLeft) / std::max(1e-6f, (map.baseRight - map.baseLeft));
                const float relY = (ndcY - map.baseBottom) / std::max(1e-6f, (map.baseTop - map.baseBottom));
                const float imageNdcX = relX * 2.0f - 1.0f;
                const float imageNdcY = relY * 2.0f - 1.0f;
                g_currentMousePos = MPR_NDCToWorld(imageNdcX, imageNdcY, mprCtx);
            }
        }
        
        // 锟斤拷锟斤拷锟斤拷诨锟斤拷疲锟街憋拷咏锟斤拷锟斤拷锟斤拷锟斤拷锟阶拷锟轿拷锟狡拷锟斤拷锟斤拷锟斤拷锟?
        if (g_isDrawing && (g_currentMaskTool == 1 || g_currentMaskTool == 2)) {
            // Map screen -> slice image pixels using the same mapping as RenderTextureToWindow.
            const float sNdcX = (float)(xpos / ctx->width) * 2.0f - 1.0f;
            const float sNdcY = 1.0f - (float)(ypos / ctx->height) * 2.0f;

            float crossTexX = 0.5f, crossTexY = 0.5f;
            if (mprCtx->sliceDirection == 0) {
                crossTexX = mprCtx->centerX / std::max(1.0f, (width - 1.0f));
                crossTexY = 1.0f - (mprCtx->centerY / std::max(1.0f, (height - 1.0f)));
            } else if (mprCtx->sliceDirection == 1) {
                crossTexX = mprCtx->centerX / std::max(1.0f, (width - 1.0f));
                crossTexY = 1.0f - (mprCtx->centerZ / std::max(1.0f, (depth - 1.0f)));
            } else {
                crossTexX = mprCtx->centerY / std::max(1.0f, (height - 1.0f));
                crossTexY = 1.0f - (mprCtx->centerZ / std::max(1.0f, (depth - 1.0f)));
            }

            GLint viewport[4];
            glGetIntegerv(GL_VIEWPORT, viewport);
            const int winWidth = viewport[2];
            const int winHeight = viewport[3];

            const TexWindowMapping map = ComputeTexWindowMapping(sliceWidth, sliceHeight, winWidth, winHeight, mprCtx->zoomFactor, crossTexX, crossTexY);
            if (!map.valid || sNdcX < map.baseLeft || sNdcX > map.baseRight || sNdcY < map.baseBottom || sNdcY > map.baseTop) {
                return;
            }

            const float relX = (sNdcX - map.baseLeft) / std::max(1e-6f, (map.baseRight - map.baseLeft));
            const float relY = (sNdcY - map.baseBottom) / std::max(1e-6f, (map.baseTop - map.baseBottom));
            const float texU = map.texLeft + relX * (map.texRight - map.texLeft);
            const float texV = map.texBottom + relY * (map.texTop - map.texBottom);

            const float u = texU * std::max(1.0f, (sliceWidth - 1.0f));
            const float v = (1.0f - texV) * std::max(1.0f, (sliceHeight - 1.0f));
            
            // 锟秸硷拷路锟斤拷锟姐（锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
            if (g_maskStrokePath.empty()) {
                g_maskStrokePath.push_back({u, v});
                printf("[PATH] Added first point\n");
            } else {
                Point2D& last = g_maskStrokePath.back();
                float dx = u - last.x;
                float dy = v - last.y;
                float dist = sqrtf(dx*dx + dy*dy);
                if (dist > 0.5f) {  // 0.5锟斤拷锟截硷拷锟?
                    g_maskStrokePath.push_back({u, v});
                    printf("[PATH] Added point #%zu (dist=%.2f)\n", g_maskStrokePath.size(), dist);
                }
            }
            
            // 强锟狡达拷锟斤拷锟截伙拷锟斤拷锟斤拷示实时预锟斤拷
            glfwPostEmptyEvent();
        }
    }
    
    // 锟斤拷锟斤拷 3D 锟斤拷锟斤拷锟斤拷图锟斤拷锟斤拷转锟斤拷锟揭硷拷锟斤拷拽锟斤拷
    if (g_3dRotating && !ctx->boundRenderer) {
        double dx = xpos - g_3dLastMouseX;
        double dy = ypos - g_3dLastMouseY;
        
        // Use session-specific 3D state if available
        std::string sessionId;
        if (ctx->aprAxial) {
            APRContext* aprCtx = static_cast<APRContext*>(ctx->aprAxial);
            sessionId = aprCtx->sessionId;
        }
        
        // Update both session and global state
        if (!sessionId.empty()) {
            TabSessionContext* tabCtx = FindTabSession(sessionId);
            if (tabCtx) {
                tabCtx->rotY += static_cast<float>(dx) * 0.5f;
                tabCtx->rotX += static_cast<float>(dy) * 0.5f;
                if (tabCtx->rotX > 89.0f) tabCtx->rotX = 89.0f;
                if (tabCtx->rotX < -89.0f) tabCtx->rotX = -89.0f;
            }
        }
        
        // 锟斤拷锟斤拷锟斤拷转锟角讹拷
        g_3dRotY += static_cast<float>(dx) * 0.5f;
        g_3dRotX += static_cast<float>(dy) * 0.5f;
        
        // 锟斤拷锟斤拷 X 锟斤拷转锟角度憋拷锟解翻转
        if (g_3dRotX > 89.0f) g_3dRotX = 89.0f;
        if (g_3dRotX < -89.0f) g_3dRotX = -89.0f;
        
        g_3dLastMouseX = xpos;
        g_3dLastMouseY = ypos;
        return;
    }
    
    // 锟斤拷通 2D 锟斤拷锟斤拷锟竭硷拷
    if (!ctx->boundRenderer) return;
    
    double dx = xpos - ctx->lastMouseX;
    double dy = ypos - ctx->lastMouseY;
    
    // 锟斤拷锟斤拷锟阶э拷锟斤拷贫锟斤拷锟轿伙拷撸锟酵拷锟斤拷锟斤拷位锟矫硷拷锟斤拷锟铰碉拷锟斤拷锟侥点）
    if (ctx->isDragging && ctx->rendererType == 1) {  // MPR
        MPRContext* mprCtx = static_cast<MPRContext*>(ctx->boundRenderer);
        
        // 锟斤拷取 Volume 维锟斤拷
        if (!mprCtx->volume) return;
        int width, height, depth;
        if (Dicom_Volume_GetDimensions(mprCtx->volume, &width, &height, &depth) != NATIVE_OK) return;
        
        // 确锟斤拷锟斤拷前锟斤拷片锟斤拷实锟绞尺寸（锟斤拷锟截ｏ拷
        int sliceWidth = 0, sliceHeight = 0;
        if (mprCtx->sliceDirection == 0) {  // Axial (XY)
            sliceWidth = width;
            sliceHeight = height;
        } else if (mprCtx->sliceDirection == 1) {  // Coronal (XZ)
            sliceWidth = width;
            sliceHeight = depth;
        } else if (mprCtx->sliceDirection == 2) {  // Sagittal (YZ)
            sliceWidth = height;
            sliceHeight = depth;
        }
        
        const float winW = std::max(1.0f, static_cast<float>(ctx->width));
        const float winH = std::max(1.0f, static_cast<float>(ctx->height));
        const float ndcX = (static_cast<float>(xpos) / winW) * 2.0f - 1.0f;
        const float ndcY = 1.0f - (static_cast<float>(ypos) / winH) * 2.0f;

        float crossTexX = 0.5f, crossTexY = 0.5f;
        if (mprCtx->sliceDirection == 0) {
            crossTexX = mprCtx->centerX / std::max(1.0f, (width - 1.0f));
            crossTexY = 1.0f - (mprCtx->centerY / std::max(1.0f, (height - 1.0f)));
        } else if (mprCtx->sliceDirection == 1) {
            crossTexX = mprCtx->centerX / std::max(1.0f, (width - 1.0f));
            crossTexY = 1.0f - (mprCtx->centerZ / std::max(1.0f, (depth - 1.0f)));
        } else {
            crossTexX = mprCtx->centerY / std::max(1.0f, (height - 1.0f));
            crossTexY = 1.0f - (mprCtx->centerZ / std::max(1.0f, (depth - 1.0f)));
        }

        const TexWindowMapping map = ComputeTexWindowMapping(sliceWidth, sliceHeight, ctx->width, ctx->height, mprCtx->zoomFactor, crossTexX, crossTexY);
        if (!map.valid || ndcX < map.baseLeft || ndcX > map.baseRight || ndcY < map.baseBottom || ndcY > map.baseTop) {
            ctx->lastMouseX = xpos;
            ctx->lastMouseY = ypos;
            return;
        }

        const float relX = (ndcX - map.baseLeft) / std::max(1e-6f, (map.baseRight - map.baseLeft));
        const float relY = (ndcY - map.baseBottom) / std::max(1e-6f, (map.baseTop - map.baseBottom));
        const float texU = map.texLeft + relX * (map.texRight - map.texLeft);
        const float texV = map.texBottom + relY * (map.texTop - map.texBottom);
        const float imgX = texU * std::max(1.0f, (sliceWidth - 1.0f));
        const float imgY = (1.0f - texV) * std::max(1.0f, (sliceHeight - 1.0f));

        float newCenterX = mprCtx->centerX;
        float newCenterY = mprCtx->centerY;
        float newCenterZ = mprCtx->centerZ;

        if (mprCtx->sliceDirection == 0) {
            newCenterX = imgX;
            newCenterY = imgY;
        } else if (mprCtx->sliceDirection == 1) {
            newCenterX = imgX;
            newCenterZ = imgY;
        } else {
            newCenterY = imgX;
            newCenterZ = imgY;
        }
        
        // 锟竭斤拷锟斤拷
        if (newCenterX < 0) newCenterX = 0;
        if (newCenterX > width - 1) newCenterX = static_cast<float>(width - 1);
        if (newCenterY < 0) newCenterY = 0;
        if (newCenterY > height - 1) newCenterY = static_cast<float>(height - 1);
        if (newCenterZ < 0) newCenterZ = 0;
        if (newCenterZ > depth - 1) newCenterZ = static_cast<float>(depth - 1);
        
        // 锟斤拷锟斤拷 MPR_SetCenter 锟斤拷锟斤拷同锟斤拷锟斤拷锟斤拷
        MPR_SetCenter(mprCtx, newCenterX, newCenterY, newCenterZ);
    }
    
    // 锟斤拷锟斤拷锟斤拷锟叫匡拷锟较讹拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟疥）
    if (g_aprCropBox.isDragging && (ctx->rendererType == 0 || ctx->rendererType == 2)) {  // APR
        auto aprCtx = static_cast<APRContext*>(ctx->boundRenderer);
        if (!aprCtx->volume) return;
        
        // 锟斤拷取锟斤拷锟斤拷锟捷尺达拷
        int width, height, depth;
        if (Dicom_Volume_GetDimensions(aprCtx->volume, &width, &height, &depth) != NATIVE_OK) return;
        
        // 确锟斤拷锟斤拷前锟斤拷片锟斤拷实锟绞尺达拷
        int sliceWidth = 0, sliceHeight = 0;
        if (g_aprCropBox.dragDirection == 0) {
            sliceWidth = width; sliceHeight = height;
        } else if (g_aprCropBox.dragDirection == 1) {
            sliceWidth = width; sliceHeight = depth;
        } else {
            sliceWidth = height; sliceHeight = depth;
        }
        
        // 锟斤拷锟斤拷图锟斤拷锟斤拷示锟斤拷锟斤拷
        int winWidth, winHeight;
        glfwGetFramebufferSize(window, &winWidth, &winHeight);

        const float ndcX = (static_cast<float>(xpos) / std::max(1.0f, static_cast<float>(winWidth))) * 2.0f - 1.0f;
        const float ndcY = 1.0f - (static_cast<float>(ypos) / std::max(1.0f, static_cast<float>(winHeight))) * 2.0f;

        const float crossTexX = aprCtx->crosshairU / std::max(1.0f, (sliceWidth - 1.0f));
        const float crossTexY = 1.0f - (aprCtx->crosshairV / std::max(1.0f, (sliceHeight - 1.0f)));
        const TexWindowMapping map = ComputeTexWindowMapping(sliceWidth, sliceHeight, winWidth, winHeight, aprCtx->zoomFactor, crossTexX, crossTexY);
        if (!map.valid || ndcX < map.baseLeft || ndcX > map.baseRight || ndcY < map.baseBottom || ndcY > map.baseTop) {
            ctx->lastMouseX = xpos;
            ctx->lastMouseY = ypos;
            return;
        }

        const float relX = (ndcX - map.baseLeft) / std::max(1e-6f, (map.baseRight - map.baseLeft));
        const float relY = (ndcY - map.baseBottom) / std::max(1e-6f, (map.baseTop - map.baseBottom));
        const float texU = map.texLeft + relX * (map.texRight - map.texLeft);
        const float texV = map.texBottom + relY * (map.texTop - map.texBottom);
        const float imgX = texU * std::max(1.0f, (sliceWidth - 1.0f));
        const float imgY = (1.0f - texV) * std::max(1.0f, (sliceHeight - 1.0f));
        
        // 锟斤拷锟斤拷锟较讹拷锟侥匡拷锟狡碉拷锟斤拷虏锟斤拷锌锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟较低筹拷校锟?
        if (g_aprCropBox.isDraggingBox) {
            // 锟狡讹拷锟斤拷锟斤拷锟斤拷 - 锟斤拷锟斤拷锟斤拷锟斤拷贫锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
            double deltaMouseX = xpos - g_aprCropBox.dragStartMouseX;
            double deltaMouseY = ypos - g_aprCropBox.dragStartMouseY;
            const float ndcPerPxX = 2.0f / std::max(1.0f, static_cast<float>(winWidth));
            const float ndcPerPxY = 2.0f / std::max(1.0f, static_cast<float>(winHeight));
            const float duPerNdc = (map.texRight - map.texLeft) / std::max(1e-6f, (map.baseRight - map.baseLeft));
            const float dvPerNdc = (map.texTop - map.texBottom) / std::max(1e-6f, (map.baseTop - map.baseBottom));
            const float dImgXPerPx = ndcPerPxX * duPerNdc * std::max(1.0f, (sliceWidth - 1.0f));
            const float dImgYPerPx = ndcPerPxY * dvPerNdc * std::max(1.0f, (sliceHeight - 1.0f));
            const float deltaX = static_cast<float>(deltaMouseX) * dImgXPerPx;
            const float deltaY = static_cast<float>(deltaMouseY) * dImgYPerPx;
            
            // 锟斤拷锟斤拷锟斤拷图锟斤拷锟斤拷锟斤拷露锟接︼拷锟斤拷锟斤拷锟斤拷锟斤拷锟?
            if (g_aprCropBox.dragDirection == 0) {  // Axial (XY)
                g_aprCropBox.xStart = g_aprCropBox.dragStartXStart + deltaX;
                g_aprCropBox.xEnd = g_aprCropBox.dragStartXEnd + deltaX;
                g_aprCropBox.yStart = g_aprCropBox.dragStartYStart + deltaY;
                g_aprCropBox.yEnd = g_aprCropBox.dragStartYEnd + deltaY;
            } else if (g_aprCropBox.dragDirection == 1) {  // Coronal (XZ)
                g_aprCropBox.xStart = g_aprCropBox.dragStartXStart + deltaX;
                g_aprCropBox.xEnd = g_aprCropBox.dragStartXEnd + deltaX;
                g_aprCropBox.zStart = g_aprCropBox.dragStartZStart + deltaY;
                g_aprCropBox.zEnd = g_aprCropBox.dragStartZEnd + deltaY;
            } else {  // Sagittal (YZ)
                g_aprCropBox.yStart = g_aprCropBox.dragStartYStart + deltaX;
                g_aprCropBox.yEnd = g_aprCropBox.dragStartYEnd + deltaX;
                g_aprCropBox.zStart = g_aprCropBox.dragStartZStart + deltaY;
                g_aprCropBox.zEnd = g_aprCropBox.dragStartZEnd + deltaY;
            }
        } else if (g_aprCropBox.dragCorner >= 0) {
            // 锟较讹拷锟角碉拷
            if (g_aprCropBox.dragDirection == 0) {  // Axial
                if (g_aprCropBox.dragCorner == 0) { g_aprCropBox.xStart = imgX; g_aprCropBox.yStart = imgY; }
                else if (g_aprCropBox.dragCorner == 1) { g_aprCropBox.xEnd = imgX; g_aprCropBox.yStart = imgY; }
                else if (g_aprCropBox.dragCorner == 2) { g_aprCropBox.xStart = imgX; g_aprCropBox.yEnd = imgY; }
                else if (g_aprCropBox.dragCorner == 3) { g_aprCropBox.xEnd = imgX; g_aprCropBox.yEnd = imgY; }
            } else if (g_aprCropBox.dragDirection == 1) {  // Coronal
                if (g_aprCropBox.dragCorner == 0) { g_aprCropBox.xStart = imgX; g_aprCropBox.zStart = imgY; }
                else if (g_aprCropBox.dragCorner == 1) { g_aprCropBox.xEnd = imgX; g_aprCropBox.zStart = imgY; }
                else if (g_aprCropBox.dragCorner == 2) { g_aprCropBox.xStart = imgX; g_aprCropBox.zEnd = imgY; }
                else if (g_aprCropBox.dragCorner == 3) { g_aprCropBox.xEnd = imgX; g_aprCropBox.zEnd = imgY; }
            } else {  // Sagittal
                if (g_aprCropBox.dragCorner == 0) { g_aprCropBox.yStart = imgX; g_aprCropBox.zStart = imgY; }
                else if (g_aprCropBox.dragCorner == 1) { g_aprCropBox.yEnd = imgX; g_aprCropBox.zStart = imgY; }
                else if (g_aprCropBox.dragCorner == 2) { g_aprCropBox.yStart = imgX; g_aprCropBox.zEnd = imgY; }
                else if (g_aprCropBox.dragCorner == 3) { g_aprCropBox.yEnd = imgX; g_aprCropBox.zEnd = imgY; }
            }
        } else if (g_aprCropBox.dragEdge >= 0) {
            // 锟较讹拷锟斤拷缘
            if (g_aprCropBox.dragDirection == 0) {  // Axial
                if (g_aprCropBox.dragEdge == 0) g_aprCropBox.yStart = imgY;
                else if (g_aprCropBox.dragEdge == 1) g_aprCropBox.xEnd = imgX;
                else if (g_aprCropBox.dragEdge == 2) g_aprCropBox.yEnd = imgY;
                else if (g_aprCropBox.dragEdge == 3) g_aprCropBox.xStart = imgX;
            } else if (g_aprCropBox.dragDirection == 1) {  // Coronal
                if (g_aprCropBox.dragEdge == 0) g_aprCropBox.zStart = imgY;
                else if (g_aprCropBox.dragEdge == 1) g_aprCropBox.xEnd = imgX;
                else if (g_aprCropBox.dragEdge == 2) g_aprCropBox.zEnd = imgY;
                else if (g_aprCropBox.dragEdge == 3) g_aprCropBox.xStart = imgX;
            } else {  // Sagittal
                if (g_aprCropBox.dragEdge == 0) g_aprCropBox.zStart = imgY;
                else if (g_aprCropBox.dragEdge == 1) g_aprCropBox.yEnd = imgX;
                else if (g_aprCropBox.dragEdge == 2) g_aprCropBox.zEnd = imgY;
                else if (g_aprCropBox.dragEdge == 3) g_aprCropBox.yStart = imgX;
            }
        }
        
        // 确锟斤拷 start < end
        if (g_aprCropBox.xStart > g_aprCropBox.xEnd) std::swap(g_aprCropBox.xStart, g_aprCropBox.xEnd);
        if (g_aprCropBox.yStart > g_aprCropBox.yEnd) std::swap(g_aprCropBox.yStart, g_aprCropBox.yEnd);
        if (g_aprCropBox.zStart > g_aprCropBox.zEnd) std::swap(g_aprCropBox.zStart, g_aprCropBox.zEnd);
        
        // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟捷凤拷围锟斤拷
        if (g_aprCropBox.xStart < 0) g_aprCropBox.xStart = 0;
        if (g_aprCropBox.xEnd >= width) g_aprCropBox.xEnd = static_cast<float>(width - 1);
        if (g_aprCropBox.yStart < 0) g_aprCropBox.yStart = 0;
        if (g_aprCropBox.yEnd >= height) g_aprCropBox.yEnd = static_cast<float>(height - 1);
        if (g_aprCropBox.zStart < 0) g_aprCropBox.zStart = 0;
        if (g_aprCropBox.zEnd >= depth) g_aprCropBox.zEnd = static_cast<float>(depth - 1);
        
        ctx->lastMouseX = xpos;
        ctx->lastMouseY = ypos;
        return;
    }
    
    // 锟斤拷锟斤拷锟阶э拷锟紸PR 锟狡讹拷锟斤拷位锟斤拷
    if (ctx->isDragging && ctx->rendererType == 2) {  // APR
        APRContext* aprCtx = static_cast<APRContext*>(ctx->boundRenderer);
        
        // 锟斤拷取 Volume 维锟斤拷
        if (!aprCtx->volume) return;
        int width, height, depth;
        if (Dicom_Volume_GetDimensions(aprCtx->volume, &width, &height, &depth) != NATIVE_OK) return;
        
        // 确锟斤拷锟斤拷前锟斤拷片锟斤拷实锟绞尺寸（锟斤拷锟截ｏ拷
        int sliceWidth = 0, sliceHeight = 0;
        if (aprCtx->sliceDirection == 0) {  // Axial (XY)
            sliceWidth = width;
            sliceHeight = height;
        } else if (aprCtx->sliceDirection == 1) {  // Coronal (XZ)
            sliceWidth = width;
            sliceHeight = depth;
        } else if (aprCtx->sliceDirection == 2) {  // Sagittal (YZ)
            sliceWidth = height;
            sliceHeight = depth;
        }
        
        const float winW = std::max(1.0f, static_cast<float>(ctx->width));
        const float winH = std::max(1.0f, static_cast<float>(ctx->height));
        const float ndcX = (static_cast<float>(xpos) / winW) * 2.0f - 1.0f;
        const float ndcY = 1.0f - (static_cast<float>(ypos) / winH) * 2.0f;

        const float crossTexX = aprCtx->crosshairU / std::max(1.0f, (sliceWidth - 1.0f));
        const float crossTexY = 1.0f - (aprCtx->crosshairV / std::max(1.0f, (sliceHeight - 1.0f)));
        const TexWindowMapping map = ComputeTexWindowMapping(sliceWidth, sliceHeight, ctx->width, ctx->height, aprCtx->zoomFactor, crossTexX, crossTexY);
        if (!map.valid || ndcX < map.baseLeft || ndcX > map.baseRight || ndcY < map.baseBottom || ndcY > map.baseTop) {
            ctx->lastMouseX = xpos;
            ctx->lastMouseY = ypos;
            return;
        }

        const float relX = (ndcX - map.baseLeft) / std::max(1e-6f, (map.baseRight - map.baseLeft));
        const float relY = (ndcY - map.baseBottom) / std::max(1e-6f, (map.baseTop - map.baseBottom));
        const float texU = map.texLeft + relX * (map.texRight - map.texLeft);
        const float texV = map.texBottom + relY * (map.texTop - map.texBottom);
        const float imgX = texU * std::max(1.0f, (sliceWidth - 1.0f));
        const float imgY = (1.0f - texV) * std::max(1.0f, (sliceHeight - 1.0f));
        
        // 锟斤拷转锟斤拷锟斤拷
        float rotationCenterX = width / 2.0f;
        float rotationCenterY = height / 2.0f;
        float rotationCenterZ = depth / 2.0f;
        
        // APR直锟接诧拷锟斤拷锟斤拷锟斤拷锟斤拷锟疥，锟斤拷锟斤拷锟角凤拷锟斤拷转
        // 锟斤拷锟斤拷锟斤拷锟斤拷系统锟叫ｏ拷图锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷 = 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
        if (aprCtx->sliceDirection == 0) {  // Axial (XY平锟斤拷)
            // 锟斤拷锟斤拷X = imgX, 锟斤拷锟斤拷Y = imgY, 锟斤拷锟斤拷Z = centerZ
            aprCtx->crosshairU = imgX;
            aprCtx->crosshairV = imgY;
            APR_SetCenter(aprCtx, imgX, imgY, aprCtx->centerZ);
        } else if (aprCtx->sliceDirection == 1) {  // Coronal (XZ平锟斤拷)
            // 锟斤拷锟斤拷X = imgX, 锟斤拷锟斤拷Y = centerY, 锟斤拷锟斤拷Z = imgY
            aprCtx->crosshairU = imgX;
            aprCtx->crosshairV = imgY;
            APR_SetCenter(aprCtx, imgX, aprCtx->centerY, imgY);
        } else if (aprCtx->sliceDirection == 2) {  // Sagittal (YZ平锟斤拷)
            // 锟斤拷锟斤拷X = centerX, 锟斤拷锟斤拷Y = imgX, 锟斤拷锟斤拷Z = imgY
            aprCtx->crosshairU = imgX;
            aprCtx->crosshairV = imgY;
            APR_SetCenter(aprCtx, aprCtx->centerX, imgX, imgY);
        }
    }
    
    // 锟揭硷拷锟斤拷拽锟斤拷锟斤拷锟斤拷锟斤拷位锟斤拷锟斤拷锟斤拷TODO: 锟斤拷锟斤拷实锟街ｏ拷
    if (ctx->isRightDragging && ctx->rendererType == 1) {
        // TODO: 实锟街达拷锟斤拷锟斤拷位锟斤拷锟斤拷
    }
    
    ctx->lastMouseX = xpos;
    ctx->lastMouseY = ypos;
}

static void OnKey(GLFWwindow* window, int key, int scancode, int action, int mods) {
    WindowContext* ctx = static_cast<WindowContext*>(glfwGetWindowUserPointer(window));
    if (!ctx) return;
    
    // 只锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟铰硷拷
    if (action != GLFW_PRESS) return;
    
    // 锟斤拷锟街硷拷0-6锟斤拷锟叫伙拷锟斤拷锟斤拷锟斤拷锟竭ｏ拷锟斤拷锟斤拷要boundRenderer锟斤拷
    if (key >= GLFW_KEY_0 && key <= GLFW_KEY_6) {
        int toolType = key - GLFW_KEY_0;  // 0-6
        
        // 锟斤拷0锟斤拷锟解处锟斤拷锟斤拷锟节讹拷位锟竭猴拷锟较次诧拷锟斤拷锟斤拷锟斤拷之锟斤拷锟叫伙拷
        if (toolType == 0) {
            if (g_currentToolType == 0) {
                // 锟斤拷前锟角讹拷位锟竭ｏ拷锟叫伙拷锟斤拷锟较次的诧拷锟斤拷锟斤拷锟斤拷
                g_currentToolType = g_lastMeasurementTool;
                printf("[Tool] Switched from CrossHair to Measurement Tool %d\n", g_currentToolType);
            } else {
                // 锟斤拷前锟角诧拷锟斤拷锟斤拷锟竭ｏ拷锟斤拷锟芥并锟叫伙拷锟斤拷锟斤拷位锟斤拷
                g_lastMeasurementTool = g_currentToolType;
                g_currentToolType = 0;
                printf("[Tool] Switched from Tool %d to CrossHair\n", g_lastMeasurementTool);
            }
        } else {
            // 锟斤拷1-6锟斤拷直锟斤拷锟叫伙拷锟斤拷锟斤拷应锟斤拷锟斤拷锟斤拷锟斤拷
            g_lastMeasurementTool = toolType;
            g_currentToolType = toolType;
            const char* toolNames[] = {"CrossHair", "Line", "Angle", "Rectangle", "Circle", "Spline", "Freehand"};
            printf("[Tool] Switched to %s Tool\n", toolNames[toolType]);
        }
        
        // 锟斤拷盏锟角帮拷锟斤拷锟?
        g_measurementPoints.clear();
        g_isDrawing = false;
        
        return;
    }
    
    // 锟斤拷锟街硷拷7锟斤拷锟叫伙拷锟斤拷Mask锟洁辑模式锟斤拷锟斤拷锟斤拷要boundRenderer锟斤拷
    if (key == GLFW_KEY_7) {
        g_currentToolType = 7;
        const char* maskToolNames[] = {"", "Brush", "Eraser", "RectROI", "CircleROI", "PolygonROI", "FloodFill", "ConnectedComp"};
        printf("[Tool] Switched to Mask Edit Mode - %s (press B/E/R/C/P/F/N to switch sub-tool)\n", maskToolNames[g_currentMaskTool]);
        
        // 锟斤拷盏锟角帮拷锟斤拷锟阶刺?
        g_measurementPoints.clear();
        g_isDrawing = false;
        
        return;
    }
    
    // Mask锟接癸拷锟斤拷锟叫伙拷锟斤拷只锟斤拷Mask模式锟斤拷锟斤拷效锟斤拷
    if (g_currentToolType == 7) {
        const char* maskToolNames[] = {"", "Brush", "Eraser", "RectROI", "CircleROI", "PolygonROI", "FloodFill", "ConnectedComp"};
        
        if (key == GLFW_KEY_B) {  // B = Brush
            g_currentMaskTool = 1;
            printf("[Mask] Switched to %s (radius=%.1f)\n", maskToolNames[1], g_brushRadius);
            return;
        }
        else if (key == GLFW_KEY_E) {  // E = Eraser
            g_currentMaskTool = 2;
            printf("[Mask] Switched to %s (radius=%.1f)\n", maskToolNames[2], g_brushRadius);
            return;
        }
        else if (key == GLFW_KEY_R) {  // R = Rectangle ROI (未实锟斤拷)
            g_currentMaskTool = 3;
            printf("[Mask] Switched to %s (not implemented yet)\n", maskToolNames[3]);
            return;
        }
        else if (key == GLFW_KEY_C && !(mods & GLFW_MOD_CONTROL)) {  // C = Circle ROI (未实锟斤拷)
            g_currentMaskTool = 4;
            printf("[Mask] Switched to %s (not implemented yet)\n", maskToolNames[4]);
            return;
        }
        else if (key == GLFW_KEY_P) {  // P = Polygon ROI (未实锟斤拷)
            g_currentMaskTool = 5;
            printf("[Mask] Switched to %s (not implemented yet)\n", maskToolNames[5]);
            return;
        }
        else if (key == GLFW_KEY_F) {  // F = Flood Fill (未实锟斤拷)
            g_currentMaskTool = 6;
            printf("[Mask] Switched to %s (not implemented yet)\n", maskToolNames[6]);
            return;
        }
        else if (key == GLFW_KEY_N) {  // N = Connected Component (未实锟斤拷)
            g_currentMaskTool = 7;
            printf("[Mask] Switched to %s (not implemented yet)\n", maskToolNames[7]);
            return;
        }
    }
    
    // [ ] 锟斤拷锟斤拷锟斤拷锟节伙拷锟绞达拷小锟斤拷锟斤拷锟斤拷要boundRenderer锟斤拷
    if (key == GLFW_KEY_LEFT_BRACKET || key == GLFW_KEY_RIGHT_BRACKET) {
        float delta = (mods & GLFW_MOD_SHIFT) ? 0.5f : 2.0f;  // Shift微锟斤拷锟斤拷锟斤拷锟斤拷值锟?
        if (key == GLFW_KEY_LEFT_BRACKET) {
            g_brushRadius = std::max(0.5f, g_brushRadius - delta);
        } else {
            g_brushRadius = std::min(50.0f, g_brushRadius + delta);
        }
        printf("[Brush] Radius adjusted to %.1f\n", g_brushRadius);
        return;
    }
    
    // C锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟缴的诧拷锟斤拷锟斤拷锟斤拷锟斤拷要boundRenderer锟斤拷
    if (key == GLFW_KEY_C && (mods & GLFW_MOD_CONTROL)) {
        {
            std::lock_guard<std::mutex> lock(g_completedMeasurementsMutex);
            g_completedMeasurements.clear();
        }
        g_measurementPoints.clear();
        g_isDrawing = false;
        printf("[Tool] Cleared all measurements\n");
        return;
    }
    
    // 锟斤拷锟铰癸拷锟斤拷锟斤拷要boundRenderer
    if (!ctx->boundRenderer) return;
    
    // V锟斤拷锟斤拷执锟叫诧拷锟叫ｏ拷Volume Crop锟斤拷
    if (key == GLFW_KEY_V && ctx->rendererType == 2) {  // APR
        if (g_aprCropBox.enabled) {
            auto aprCtx = static_cast<APRContext*>(ctx->boundRenderer);
            if (aprCtx->volume || aprCtx->croppedVolumeData) {
                printf("\n========== CROPPING VOLUME ==========\n");
                APRHandle croppedHandle = APR_CropVolume(aprCtx);
                if (croppedHandle) {
                    auto croppedCtx = static_cast<APRContext*>(croppedHandle);
                    if (croppedCtx->croppedVolumeData) {
                        auto croppedVol = croppedCtx->croppedVolumeData;
                        printf("SUCCESS! Cropped volume size: %d x %d x %d\n", 
                               croppedVol->width, croppedVol->height, croppedVol->depth);
                        
                        // 锟矫诧拷锟叫猴拷锟斤拷锟斤拷荽锟斤拷锟斤拷碌锟紻ICOM Volume锟斤拷锟芥换锟斤拷锟斤拷APR锟斤拷锟斤拷锟斤拷
                        printf("Replacing all APR volumes with cropped data...\n");
                        
                        // 锟斤拷锟斤拷锟铰碉拷VolumeHandle锟斤拷直锟斤拷使锟斤拷VolumeContext锟斤拷
                        VolumeHandle newVolume = static_cast<VolumeHandle>(croppedVol);
                        
                        // 锟斤拷取锟斤拷锟斤拷锟斤拷锟接碉拷APR
                        std::vector<APRContext*> linkedAPRs;
                        if (!g_globalAPRCenter.linkedAPRs.empty()) {
                            for (APRHandle apr : g_globalAPRCenter.linkedAPRs) {
                                linkedAPRs.push_back(static_cast<APRContext*>(apr));
                            }
                        } else {
                            linkedAPRs.push_back(aprCtx);
                        }
                        
                        // 锟芥换锟斤拷锟斤拷锟斤拷锟斤拷APR锟斤拷volume
                        for (auto ctx : linkedAPRs) {
                            ctx->volume = newVolume;
                            ctx->croppedVolumeData = nullptr;  // 锟斤拷詹锟斤拷锟斤拷锟斤拷荼锟斤拷
                            ctx->ownsVolumeData = false;
                            
                            // 锟斤拷锟斤拷锟斤拷锟侥点到锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
                            ctx->centerX = croppedVol->width * 0.5f;
                            ctx->centerY = croppedVol->height * 0.5f;
                            ctx->centerZ = croppedVol->depth * 0.5f;

                            // Reset rotation
                            Mat4_Identity(ctx->rotMat);
                        }
                        
                        // 锟斤拷锟斤拷全锟斤拷锟斤拷锟侥碉拷
                        g_globalAPRCenter.x = croppedVol->width * 0.5f;
                        g_globalAPRCenter.y = croppedVol->height * 0.5f;
                        g_globalAPRCenter.z = croppedVol->depth * 0.5f;
                        g_globalAPRCenter.volume = newVolume;
                        Mat4_Identity(g_globalAPRCenter.rotMat);
                        
                        // 锟斤拷锟矫诧拷锟叫匡拷
                        g_aprCropBox.enabled = false;
                        
                        // 锟斤拷锟絚roppedHandle锟斤拷拥锟斤拷锟斤拷锟捷ｏ拷锟斤拷为锟窖撅拷锟斤拷Volume锟接管ｏ拷
                        croppedCtx->croppedVolumeData = nullptr;
                        croppedCtx->ownsVolumeData = false;
                        
                        printf("Volume replaced! New size: %d x %d x %d\n",
                               croppedVol->width, croppedVol->height, croppedVol->depth);
                        printf("Crop box disabled. Press B to re-enable.\n");
                        printf("====================================\n\n");
                    } else {
                        printf("ERROR: Cropped volume data is null!\n");
                        printf("====================================\n\n");
                    }
                } else {
                    printf("FAILED to crop volume!\n");
                    printf("====================================\n\n");
                }
            } else {
                printf("ERROR: No volume data to crop!\n");
            }
        }
        return;  // V锟斤拷锟斤拷锟斤拷锟斤拷锟?
    }
    
    // Note: legacy WASDQE APR rotation controls removed (rotation is handled via API / Shift+drag).
    
    // Z/X锟斤拷锟斤拷APR锟斤拷锟脚ｏ拷锟斤拷APR锟斤拷图锟斤拷
    if (ctx->rendererType == 2 && ctx->boundRenderer) {  // APR
        auto aprCtx = static_cast<APRContext*>(ctx->boundRenderer);
        bool zoomChanged = false;
        float newZoom = aprCtx->zoomFactor;
        
        if (key == GLFW_KEY_Z) {  // 锟脚达拷
            newZoom *= 1.1f;
            if (newZoom > 10.0f) newZoom = 10.0f;
            zoomChanged = true;
        } else if (key == GLFW_KEY_X) {  // 锟斤拷小
            newZoom /= 1.1f;
            if (newZoom < 0.1f) newZoom = 0.1f;
            zoomChanged = true;
        }
        
        if (zoomChanged) {
            // 同锟斤拷锟斤拷锟斤拷锟斤拷锟接碉拷APR锟斤拷锟斤拷锟斤拷
            for (APRHandle linkedAPR : g_globalAPRCenter.linkedAPRs) {
                auto linkedCtx = static_cast<APRContext*>(linkedAPR);
                linkedCtx->zoomFactor = newZoom;
            }
            printf("[APR] Zoom: %.2fx\n", newZoom);
            return;
        }
    }
    
    // R锟斤拷锟斤拷APR锟斤拷锟斤拷锟斤拷转锟斤拷锟斤拷锟侥ｏ拷锟斤拷APR锟斤拷图锟斤拷
    if (key == GLFW_KEY_R && ctx->rendererType == 2 && ctx->boundRenderer) {  // APR
        auto aprCtx = static_cast<APRContext*>(ctx->boundRenderer);
        
        // 锟斤拷取锟斤拷锟斤拷锟捷尺达拷
        int width, height, depth;
        if (aprCtx->volume && Dicom_Volume_GetDimensions(aprCtx->volume, &width, &height, &depth) == NATIVE_OK) {
            float cx = width / 2.0f;
            float cy = height / 2.0f;
            float cz = depth / 2.0f;
            
            // 锟斤拷锟斤拷全锟斤拷锟斤拷锟侥猴拷锟斤拷转
            g_globalAPRCenter.x = cx;
            g_globalAPRCenter.y = cy;
            g_globalAPRCenter.z = cz;
            Mat4_Identity(g_globalAPRCenter.rotMat);
            
            // 同锟斤拷锟斤拷锟斤拷锟斤拷锟接碉拷APR
            for (APRHandle linkedAPR : g_globalAPRCenter.linkedAPRs) {
                auto linkedCtx = static_cast<APRContext*>(linkedAPR);
                linkedCtx->centerX = cx;
                linkedCtx->centerY = cy;
                linkedCtx->centerZ = cz;
                Mat4_Identity(linkedCtx->rotMat);
            }
            
            printf("[APR] Reset: Center=(%.1f, %.1f, %.1f), Rotation=(0锟斤拷, 0锟斤拷, 0锟斤拷)\n", cx, cy, cz);
            return;
        }
    }
    
    // B锟斤拷锟斤拷锟叫伙拷APR锟斤拷锟叫框（斤拷APR锟斤拷图锟斤拷
    if (key == GLFW_KEY_B && ctx->rendererType == 2) {  // APR
        bool enabled = APR_IsCropBoxEnabled();
        APR_EnableCropBox(!enabled);
        printf("[APR] Crop box %s\n", !enabled ? "enabled" : "disabled");
        return;
    }
}

static void OnScroll(GLFWwindow* window, double xoffset, double yoffset) {
    WindowContext* ctx = static_cast<WindowContext*>(glfwGetWindowUserPointer(window));
    if (!ctx) return;
    
    // 3D 锟斤拷锟斤拷锟斤拷图锟斤拷锟节ｏ拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
    if (!ctx->boundRenderer) {
        // Update session-specific zoom if available
        std::string sessionId;
        if (ctx->aprAxial) {
            APRContext* aprCtx = static_cast<APRContext*>(ctx->aprAxial);
            sessionId = aprCtx->sessionId;
        }
        
        if (!sessionId.empty()) {
            TabSessionContext* tabCtx = FindTabSession(sessionId);
            if (tabCtx) {
                tabCtx->zoom += static_cast<float>(yoffset) * 0.1f;
                if (tabCtx->zoom < 0.1f) tabCtx->zoom = 0.1f;
                if (tabCtx->zoom > 10.0f) tabCtx->zoom = 10.0f;
            }
        }
        
        g_3dZoom += static_cast<float>(yoffset) * 0.1f;
        // 锟斤拷锟斤拷锟斤拷锟脚凤拷围
        if (g_3dZoom < 0.1f) g_3dZoom = 0.1f;
        if (g_3dZoom > 10.0f) g_3dZoom = 10.0f;
        return;
    }
    
    // 锟斤拷锟街ｏ拷锟节碉拷前锟斤拷锟节凤拷锟斤拷锟斤拷锟斤拷谢锟斤拷锟狡?
    if (ctx->rendererType == 1) {  // MPR
        MPRContext* mprCtx = static_cast<MPRContext*>(ctx->boundRenderer);
        
        if (!mprCtx->volume) return;
        int width, height, depth;
        if (Dicom_Volume_GetDimensions(mprCtx->volume, &width, &height, &depth) != NATIVE_OK) return;
        
        float step = static_cast<float>(yoffset);
        float newCenterX = mprCtx->centerX;
        float newCenterY = mprCtx->centerY;
        float newCenterZ = mprCtx->centerZ;
        
        if (mprCtx->sliceDirection == 0) {  // Axial 锟斤拷 锟叫伙拷 Z 锟斤拷锟斤拷
            newCenterZ += step;
            if (newCenterZ < 0) newCenterZ = 0;
            if (newCenterZ > depth - 1) newCenterZ = static_cast<float>(depth - 1);
        } else if (mprCtx->sliceDirection == 1) {  // Coronal 锟斤拷 锟叫伙拷 Y 锟斤拷锟斤拷
            newCenterY += step;
            if (newCenterY < 0) newCenterY = 0;
            if (newCenterY > height - 1) newCenterY = static_cast<float>(height - 1);
        } else if (mprCtx->sliceDirection == 2) {  // Sagittal 锟斤拷 锟叫伙拷 X 锟斤拷锟斤拷
            newCenterX += step;
            if (newCenterX < 0) newCenterX = 0;
            if (newCenterX > width - 1) newCenterX = static_cast<float>(width - 1);
        }
        
        // 锟斤拷锟斤拷 MPR_SetCenter 锟斤拷锟斤拷同锟斤拷锟斤拷锟斤拷
        MPR_SetCenter(mprCtx, newCenterX, newCenterY, newCenterZ);
    }
    
    // 锟斤拷锟街ｏ拷APR 锟斤拷锟斤拷锟斤拷片锟斤拷锟斤拷锟叫伙拷锟斤拷应锟斤拷锟斤拷
    if (ctx->rendererType == 2) {  // APR
        APRContext* aprCtx = static_cast<APRContext*>(ctx->boundRenderer);
        
        if (!aprCtx->volume) return;
        int width, height, depth;
        if (Dicom_Volume_GetDimensions(aprCtx->volume, &width, &height, &depth) != NATIVE_OK) return;
        
        float step = static_cast<float>(yoffset);
        float newCenterX = aprCtx->centerX;
        float newCenterY = aprCtx->centerY;
        float newCenterZ = aprCtx->centerZ;
        
        // 锟斤拷锟斤拷锟斤拷片锟斤拷锟斤拷锟叫伙拷锟斤拷应锟斤拷锟斤拷
        if (aprCtx->sliceDirection == 0) {  // Axial 锟斤拷 锟叫伙拷 Z 锟斤拷锟斤拷
            newCenterZ += step;
            if (newCenterZ < 0) newCenterZ = 0;
            if (newCenterZ > depth - 1) newCenterZ = static_cast<float>(depth - 1);
        } else if (aprCtx->sliceDirection == 1) {  // Coronal 锟斤拷 锟叫伙拷 Y 锟斤拷锟斤拷
            newCenterY += step;
            if (newCenterY < 0) newCenterY = 0;
            if (newCenterY > height - 1) newCenterY = static_cast<float>(height - 1);
        } else if (aprCtx->sliceDirection == 2) {  // Sagittal 锟斤拷 锟叫伙拷 X 锟斤拷锟斤拷
            newCenterX += step;
            if (newCenterX < 0) newCenterX = 0;
            if (newCenterX > width - 1) newCenterX = static_cast<float>(width - 1);
        }
        
        // 锟斤拷锟斤拷 APR_SetCenter 锟斤拷锟斤拷同锟斤拷锟斤拷锟斤拷
        APR_SetCenter(aprCtx, newCenterX, newCenterY, newCenterZ);
    }
}

#endif // !defined(_WIN32)

// ==================== Window ====================
WindowHandle Window_Create(int width, int height, const char* title) {
    printf("[Window_Create] START - width=%d, height=%d, title=%s\n", width, height, title ? title : "NULL");
    
#ifdef _WIN32
    printf("[Window_Create] Using Win32+WGL implementation\n");
    
    // Win32 + WGL实锟斤拷
    if (!RegisterWin32WindowClass()) {
        char err[256];
        sprintf_s(err, sizeof(err), "Failed to register window class, error: %lu", GetLastError());
        SetLastError(err);
        return nullptr;
    }
    
    // 锟斤拷锟斤拷Win32锟斤拷锟节ｏ拷锟斤拷锟截ｏ拷
    HWND hwnd = CreateWindowEx(
        0,
        g_WindowClassName,
        L"Viewer",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        nullptr, nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );
    
    if (!hwnd) {
        char err[256];
        sprintf_s(err, sizeof(err), "Failed to create Win32 window, error: %lu", GetLastError());
        SetLastError(err);
        return nullptr;
    }
    
    // 锟斤拷锟截达拷锟节ｏ拷锟饺达拷嵌锟诫）
    ShowWindow(hwnd, SW_HIDE);
    printf("[Window_Create] Win32 window created, HWND=%p\n", hwnd);
    
    // 锟斤拷取锟借备锟斤拷锟斤拷锟斤拷
    HDC hdc = GetDC(hwnd);
    printf("[Window_Create] Got device context, HDC=%p\n", hdc);
    
    // 锟斤拷锟斤拷锟斤拷锟截革拷式
    PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR) };
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;
    
    int pixelFormat = ChoosePixelFormat(hdc, &pfd);
    if (!pixelFormat) {
        SetLastError("Failed to choose pixel format");
        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);
        return nullptr;
    }
    
    if (!SetPixelFormat(hdc, pixelFormat, &pfd)) {
        SetLastError("Failed to set pixel format");
        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);
        return nullptr;
    }
    
    printf("[Window_Create] Pixel format set\n");
    
    // 锟斤拷锟斤拷OpenGL锟斤拷锟斤拷锟斤拷
    HGLRC hglrc = wglCreateContext(hdc);
    if (!hglrc) {
        SetLastError("Failed to create OpenGL context");
        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);
        return nullptr;
    }
    
    printf("[Window_Create] OpenGL context created, HGLRC=%p\n", hglrc);
    
    // 锟斤拷锟斤拷泄锟斤拷锟斤拷锟斤拷锟斤拷模锟斤拷锟斤拷锟斤拷锟皆?
    if (g_SharedGLContext) {
        printf("[Window_Create] Sharing with existing context %p\n", g_SharedGLContext);
        if (!wglShareLists(g_SharedGLContext, hglrc)) {
            SetLastError("Failed to share OpenGL contexts");
        }
    } else {
        printf("[Window_Create] First context, setting as shared context\n");
        g_SharedGLContext = hglrc;  // 锟斤拷一锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟节癸拷锟斤拷
    }
    
    // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
    wglMakeCurrent(hdc, hglrc);
    printf("[Window_Create] Context made current\n");
    
    // 锟斤拷始锟斤拷GLEW
    static bool glewInitialized = false;
    if (!glewInitialized) {
        printf("[Window_Create] Initializing GLEW...\n");
        if (glewInit() != GLEW_OK) {
            SetLastError("Failed to initialize GLEW");
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(hglrc);
            ReleaseDC(hwnd, hdc);
            DestroyWindow(hwnd);
            return nullptr;
        }
        glewInitialized = true;
        printf("[Window_Create] GLEW initialized\n");
    }
    
    // Initialize NanoVG (retry-safe). Some GL setups can't create stencil-based NanoVG.
    if (!g_nvgContext) {
        printf("[Window_Create] Creating NanoVG context...\n");
    }
    EnsureNanoVGReady();
    if (!g_nvgContext) {
        printf("[Window_Create] WARNING: Failed to create NanoVG context\n");
        SetLastError("Failed to create NanoVG context");
    } else if (g_nvgFontId < 0) {
        printf("[Window_Create] WARNING: Failed to load NanoVG font (ui)\n");
    }
    
    // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
    auto ctx = new WindowContext();
    ctx->hwnd = hwnd;
    ctx->hdc = hdc;
    ctx->hglrc = hglrc;
    ctx->debugTitle = title ? title : "";
    ctx->width = width;
    ctx->height = height;
    
    // 锟斤拷锟矫达拷锟斤拷锟矫伙拷锟斤拷锟斤拷
    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ctx));
    
    // 锟斤拷锟矫筹拷始锟接匡拷
    glViewport(0, 0, width, height);
    
    // 锟斤拷锟接碉拷全锟街达拷锟斤拷锟叫憋拷
    g_AllWindows.push_back(ctx);
    
    printf("[Window_Create] SUCCESS - HWND=%p, WindowContext=%p, total windows=%zu\n", 
           hwnd, ctx, g_AllWindows.size());
    return ctx;
    
#else
    printf("[Window_Create] Using GLFW implementation (non-Windows)\n");
    // 锟斤拷Windows平台锟斤拷锟斤拷使锟斤拷GLFW
    if (!glfwInit()) {
        SetLastError("Failed to initialize GLFW");
        return nullptr;
    }
    
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    
    GLFWwindow* window = glfwCreateWindow(width, height, title ? title : "Viewer", nullptr, g_sharedContextWindow);
    if (!window) {
        SetLastError("Failed to create GLFW window");
        glfwTerminate();
        return nullptr;
    }
    
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    
    if (!g_sharedContextWindow) {
        g_sharedContextWindow = window;
    }
    
    glfwMakeContextCurrent(window);
    
    if (glewInit() != GLEW_OK) {
        SetLastError("Failed to initialize GLEW");
        glfwDestroyWindow(window);
        glfwTerminate();
        return nullptr;
    }
    
    if (!g_nvgContext) {
        g_nvgContext = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
        if (!g_nvgContext) {
            SetLastError("Failed to create NanoVG context");
        }
    }

    if (g_nvgContext && g_nvgFontId < 0) {
        g_nvgFontId = nvgCreateFont(g_nvgContext, "ui", "C:\\Windows\\Fonts\\arial.ttf");
        if (g_nvgFontId < 0) {
            g_nvgFontId = nvgCreateFont(g_nvgContext, "ui", "C:\\Windows\\Fonts\\segoeui.ttf");
        }
    }
    
    auto ctx = new WindowContext();
    ctx->window = window;
    ctx->width = width;
    ctx->height = height;
    
    glfwSetWindowUserPointer(window, ctx);
    glfwSetFramebufferSizeCallback(window, OnFramebufferResize);
    glfwSetMouseButtonCallback(window, OnMouseButton);
    glfwSetCursorPosCallback(window, OnCursorPos);
    glfwSetScrollCallback(window, OnScroll);
    glfwSetKeyCallback(window, OnKey);
    
    glViewport(0, 0, width, height);
    
    return ctx;
#endif
}

void Window_Destroy(WindowHandle handle) {
    std::cout << "[Window_Destroy] START - handle=" << handle << std::endl;
    if (!handle) return;
    auto ctx = static_cast<WindowContext*>(handle);

    if (ctx->raycast3D) {
        Raycast3D_Destroy(static_cast<Raycast3DContext*>(ctx->raycast3D));
        ctx->raycast3D = nullptr;
    }
    
#ifdef _WIN32
    std::cout << "[Window_Destroy] Win32 path - HWND=" << ctx->hwnd << std::endl;
    if (ctx->hglrc) {
        wglMakeCurrent(nullptr, nullptr);
        if (ctx->hglrc == g_SharedGLContext) {
            g_SharedGLContext = nullptr;
        }
        wglDeleteContext(ctx->hglrc);
    }
    if (ctx->hdc && ctx->hwnd) {
        ReleaseDC(ctx->hwnd, ctx->hdc);
    }
    if (ctx->hwnd) {
        DestroyWindow(ctx->hwnd);
    }
#else
    if (ctx->window) {
        if (ctx->window == g_sharedContextWindow) {
            g_sharedContextWindow = nullptr;
        }
        glfwDestroyWindow(ctx->window);
        glfwTerminate();
    }
#endif
    
    // 锟斤拷全锟街达拷锟斤拷锟叫憋拷锟斤拷锟狡筹拷
    g_AllWindows.erase(std::remove(g_AllWindows.begin(), g_AllWindows.end(), handle), g_AllWindows.end());
    
    delete ctx;
    std::cout << "[Window_Destroy] SUCCESS" << std::endl;
}

// 锟斤拷锟斤拷锟斤拷锟叫达拷锟节诧拷锟斤拷锟斤拷锟斤拷息锟斤拷锟斤拷锟斤拷Tab锟叫伙拷时使锟矫ｏ拷
VIZ_API void Window_HideAllWindows() {

    Window_StopRenderLoop();

    
    for (size_t i = 0; i < g_AllWindows.size(); i++) {
        auto handle = g_AllWindows[i];
        auto ctx = static_cast<WindowContext*>(handle);
        if (!ctx) continue;

        ctx->isHidden = true;

        
#ifdef _WIN32
        if (ctx->hwnd) {

            MSG msg;
            while (PeekMessage(&msg, ctx->hwnd, WM_PAINT, WM_PAINT, PM_REMOVE)) {
                // 直锟接讹拷锟斤拷 WM_PAINT 锟斤拷息锟斤拷锟斤拷锟街凤拷
                printf("[Window_HideAllWindows]   Discarded WM_PAINT message\n");
            }
            
            // ===== 锟截硷拷锟睫革拷3: 锟酵凤拷锟轿何活动锟斤拷 OpenGL 锟斤拷锟斤拷锟斤拷 =====
            if (ctx->hdc && ctx->hglrc) {
                if (wglGetCurrentContext() == ctx->hglrc) {
                    wglMakeCurrent(nullptr, nullptr);
                }
            }      
            ShowWindow(ctx->hwnd, SW_HIDE);
            EnableWindow(ctx->hwnd, FALSE);

        }
#endif
    }
}

// 锟斤拷示锟斤拷锟叫达拷锟节诧拷锟斤拷锟斤拷锟斤拷息锟斤拷锟斤拷锟斤拷锟叫伙拷锟斤拷Viewer Tab时使锟矫ｏ拷
VIZ_API void Window_ShowAllWindows() {
    
    for (auto handle : g_AllWindows) {
        auto ctx = static_cast<WindowContext*>(handle);
        if (!ctx) continue;
        
        // 锟斤拷锟斤拷锟斤拷乇锟街撅拷锟斤拷锟斤拷锟斤拷锟饺撅拷叱谭锟斤拷蚀锟斤拷冢锟?
        ctx->isHidden = false;
        
#ifdef _WIN32
        if (ctx->hwnd) {
            // 锟斤拷锟矫达拷锟斤拷锟斤拷息锟斤拷锟斤拷
            EnableWindow(ctx->hwnd, TRUE);
            // 锟斤拷示锟斤拷锟斤拷
            ShowWindow(ctx->hwnd, SW_SHOWNA);
        }
#endif
    }
    
    // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷染锟竭程ｏ拷锟斤拷锟斤拷帧锟绞硷拷锟斤拷CPU锟斤拷锟斤拷锟斤拷锟截憋拷锟斤拷锟斤拷转时锟斤拷
    Window_StartRenderLoop(30);  // 锟斤拷60fps锟斤拷锟酵碉拷30fps
    
    
}

// 锟酵凤拷锟斤拷锟斤拷3D锟斤拷锟斤拷
VIZ_API void Window_DestroyAll3DWindows() {
    
    // 锟斤拷停止锟斤拷染锟竭筹拷
    Window_StopRenderLoop();
    
    // 锟秸硷拷锟斤拷锟斤拷3D锟斤拷锟斤拷
    std::vector<WindowHandle> windows3D;
    for (auto handle : g_AllWindows) {
        auto ctx = static_cast<WindowContext*>(handle);
        if (ctx && ctx->is3DView) {
            windows3D.push_back(handle);
        }
    }
    
    // 锟斤拷锟斤拷锟斤拷锟斤拷3D锟斤拷锟斤拷
    for (auto handle : windows3D) {
        Window_Destroy(handle);
    }

}

// 锟酵凤拷锟斤拷锟叫达拷锟节猴拷锟斤拷源
VIZ_API void Window_DestroyAllWindows() {
    
    // 锟斤拷停止锟斤拷染锟竭程ｏ拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷俚拇锟斤拷锟?
    Window_StopRenderLoop();
    
    // 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟节碉拷锟斤拷时锟睫革拷锟斤拷锟斤拷
    std::vector<WindowHandle> allWindows = g_AllWindows;
    
    // 锟斤拷锟斤拷锟斤拷锟叫达拷锟斤拷
    for (auto handle : allWindows) {
        Window_Destroy(handle);
    }
    
    // 锟酵放诧拷锟叫猴拷锟紸PR
    if (g_lastCroppedAPR) {
        APR_Destroy(g_lastCroppedAPR);
        g_lastCroppedAPR = nullptr;
    }
    
    // 锟斤拷锟饺拷植锟斤拷锟斤拷锟斤拷锟?
    g_measurementPoints.clear();
    {
        std::lock_guard<std::mutex> lock(g_completedMeasurementsMutex);
        g_completedMeasurements.clear();
    }
    g_currentToolType = 0;
    g_isDrawing = false;
    
    printf("[Window_DestroyAllWindows] COMPLETE - remaining windows: %zu\n", g_AllWindows.size());
}

NativeResult Window_BindRenderer(WindowHandle handle, void* rendererHandle, int rendererType) {
    std::cout << "[Window_BindRenderer] handle=" << handle << ", rendererHandle=" << rendererHandle << ", type=" << rendererType << std::endl;
    if (!handle) return NATIVE_E_INVALID_ARGUMENT;
    auto ctx = static_cast<WindowContext*>(handle);
    ctx->boundRenderer = rendererHandle;
    ctx->rendererType = rendererType;
    return NATIVE_OK;
}

static bool ContainsCaseInsensitive(const std::string& haystack, const char* needle) {
    if (!needle || !needle[0]) return false;
    std::string h;
    h.reserve(haystack.size());
    for (char c : haystack) h.push_back((char)tolower((unsigned char)c));

    std::string n;
    for (const char* p = needle; *p; ++p) n.push_back((char)tolower((unsigned char)*p));
    return h.find(n) != std::string::npos;
}

static void AutoInfer3DRendererKindIfNeeded(WindowContext* ctx) {
    if (!ctx) return;
    if (ctx->threeDRendererKindExplicit) return;
    if (!ctx->is3DView) return;

    if (!ctx->orthogonal3DView) {
        ctx->threeDRendererKind = 3; // reconstruction raycast
        return;
    }

    // Orthogonal mode: distinguish ImageBrowser vs ROI by window title.
    if (ContainsCaseInsensitive(ctx->debugTitle, "roi")) {
        ctx->threeDRendererKind = 2;
        return;
    }

    // Heuristic: allow future dedicated reconstruction orthogonal windows if needed.
    if (ContainsCaseInsensitive(ctx->debugTitle, "recon") ||
        ContainsCaseInsensitive(ctx->debugTitle, "reconstruct") ||
        ContainsCaseInsensitive(ctx->debugTitle, "raycast")) {
        ctx->threeDRendererKind = 3;
        return;
    }

    ctx->threeDRendererKind = 1;
}

NativeResult Window_Set3DViewAPRs(WindowHandle handle, void* aprAxial, void* aprCoronal, void* aprSagittal) {
    printf("[Window_Set3DViewAPRs] handle=%p, aprAxial=%p, aprCoronal=%p, aprSagittal=%p\n", 
           handle, aprAxial, aprCoronal, aprSagittal);
    if (!handle) return NATIVE_E_INVALID_ARGUMENT;
    auto ctx = static_cast<WindowContext*>(handle);
    ctx->is3DView = true;
    ctx->orthogonal3DView = true;
    ctx->aprAxial = aprAxial;
    ctx->aprCoronal = aprCoronal;
    ctx->aprSagittal = aprSagittal;

    AutoInfer3DRendererKindIfNeeded(ctx);

    // Init per-window navigation defaults
    ctx->viewRotX = 30.0f;
    ctx->viewRotY = 45.0f;
    ctx->viewZoom = 1.0f;
    ctx->viewPanX = 0.0f;
    ctx->viewPanY = 0.0f;
    {
        float rx[16];
        float ry[16];
        Mat4_RotationAxisAngle(rx, 1.0f, 0.0f, 0.0f, ctx->viewRotX);
        Mat4_RotationAxisAngle(ry, 0.0f, 1.0f, 0.0f, ctx->viewRotY);
        Mat4_Mul(ctx->viewRotMat, rx, ry);  // match old glRotatef order: Rx then Ry
        ctx->viewRotMatInitialized = true;
    }
    ctx->isMiddleDragging = false;
    ctx->is3DRotating = false;
    ctx->is3DPanning = false;
    return NATIVE_OK;
}

void Window_ResetView(WindowHandle handle) {
    if (!handle) return;
    auto ctx = static_cast<WindowContext*>(handle);
    if (!ctx) return;

    ctx->isDragging = false;
    ctx->isRightDragging = false;
    ctx->isWindowing = false;
    ctx->isShiftRotateZ = false;
    ctx->arcballActive = false;
    ctx->isMiddleDragging = false;
    ctx->is3DRotating = false;
    ctx->is3DPanning = false;

    // 3D window navigation defaults
    if (ctx->is3DView) {
        ctx->viewRotX = 30.0f;
        ctx->viewRotY = 45.0f;
        ctx->viewZoom = 1.0f;
        ctx->viewPanX = 0.0f;
        ctx->viewPanY = 0.0f;
        {
            float rx[16];
            float ry[16];
            Mat4_RotationAxisAngle(rx, 1.0f, 0.0f, 0.0f, ctx->viewRotX);
            Mat4_RotationAxisAngle(ry, 0.0f, 1.0f, 0.0f, ctx->viewRotY);
            Mat4_Mul(ctx->viewRotMat, rx, ry);
            ctx->viewRotMatInitialized = true;
        }
#ifdef _WIN32
        if (ctx->hwnd) InvalidateRect(ctx->hwnd, nullptr, FALSE);
#endif
        return;
    }

    // APR/MPR defaults: zoom=1, WL=(4096,2048), center=volume mid, APR rotation reset
    if (ctx->boundRenderer) {
        if (ctx->rendererType == 0 || ctx->rendererType == 2) {
            auto aprCtx = static_cast<APRContext*>(ctx->boundRenderer);
            if (aprCtx) {
                aprCtx->zoomFactor = 1.0f;
                aprCtx->windowWidthHU = 4096.0f;
                aprCtx->windowLevelHU = 2048.0f;
                APR_ResetRotation(aprCtx);

                if (aprCtx->volume) {
                    int w = 0, h = 0, d = 0;
                    if (Dicom_Volume_GetDimensions(aprCtx->volume, &w, &h, &d) == NATIVE_OK) {
                        float cx = (w > 1) ? (0.5f * (w - 1)) : 0.0f;
                        float cy = (h > 1) ? (0.5f * (h - 1)) : 0.0f;
                        float cz = (d > 1) ? (0.5f * (d - 1)) : 0.0f;
                        APR_SetCenter(aprCtx, cx, cy, cz);
                    }
                }
            }
        } else if (ctx->rendererType == 1) {
            auto mprCtx = static_cast<MPRContext*>(ctx->boundRenderer);
            if (mprCtx) {
                mprCtx->zoomFactor = 1.0f;
                mprCtx->windowWidthHU = 4096.0f;
                mprCtx->windowLevelHU = 2048.0f;

                if (mprCtx->volume) {
                    int w = 0, h = 0, d = 0;
                    if (Dicom_Volume_GetDimensions(mprCtx->volume, &w, &h, &d) == NATIVE_OK) {
                        float cx = (w > 1) ? (0.5f * (w - 1)) : 0.0f;
                        float cy = (h > 1) ? (0.5f * (h - 1)) : 0.0f;
                        float cz = (d > 1) ? (0.5f * (d - 1)) : 0.0f;
                        MPR_SetCenter(mprCtx, cx, cy, cz);
                    }
                }
            }
        }
    }

#ifdef _WIN32
    if (ctx->hwnd) InvalidateRect(ctx->hwnd, nullptr, FALSE);
#endif
}

NativeResult Window_Set3DViewOrthogonalMode(WindowHandle handle, bool enableOrthogonal) {
    if (!handle) return NATIVE_E_INVALID_ARGUMENT;
    auto ctx = static_cast<WindowContext*>(handle);
    if (!ctx) return NATIVE_E_INVALID_ARGUMENT;
    ctx->orthogonal3DView = enableOrthogonal;
    AutoInfer3DRendererKindIfNeeded(ctx);
    Window_Invalidate(handle);
    return NATIVE_OK;
}

NativeResult Window_Set3DViewVramOptimized(WindowHandle handle, bool enableOptimized) {
    if (!handle) return NATIVE_E_INVALID_ARGUMENT;
    auto ctx = static_cast<WindowContext*>(handle);
    if (!ctx) return NATIVE_E_INVALID_ARGUMENT;
    ctx->vramOptimized3D = enableOptimized;
    // Force a repaint; Raycast3D_EnsureVolumeTexture will reupload if needed.
    Window_Invalidate(handle);
    return NATIVE_OK;
}

NativeResult Window_Set3DViewMaskIsoSurfaceEnabled(WindowHandle handle, bool enable) {
    if (!handle) return NATIVE_E_INVALID_ARGUMENT;
    auto ctx = static_cast<WindowContext*>(handle);
    if (!ctx) return NATIVE_E_INVALID_ARGUMENT;
    ctx->maskIsoSurface3D = enable;
    Window_Invalidate(handle);
    return NATIVE_OK;
}

NativeResult Window_Set3DViewTransferFunctionPoints(WindowHandle handle, const float* pointsPacked, int pointCount) {
    if (!handle) return NATIVE_E_INVALID_ARGUMENT;
    auto ctx = static_cast<WindowContext*>(handle);
    if (!ctx) return NATIVE_E_INVALID_ARGUMENT;

    if (!pointsPacked || pointCount <= 0) {
        ctx->transferLutRGBA3D.clear();
        ctx->transferLutCustom3D = false;
        ctx->transferLutDirty3D = true;
        Window_Invalidate(handle);
        return NATIVE_OK;
    }

    std::vector<unsigned char> lut;
    BuildTransferLutFromPoints(pointsPacked, pointCount, lut);
    ctx->transferLutRGBA3D = std::move(lut);
    ctx->transferLutCustom3D = true;
    ctx->transferLutDirty3D = true;
    Window_Invalidate(handle);
    return NATIVE_OK;
}

NativeResult Window_Set3DViewLightParameters(WindowHandle handle, float ambient, float diffuse, float specular) {
    if (!handle) return NATIVE_E_INVALID_ARGUMENT;
    auto ctx = static_cast<WindowContext*>(handle);
    if (!ctx) return NATIVE_E_INVALID_ARGUMENT;

    ctx->lightAmbient3D = Clamp01f(ambient);
    ctx->lightDiffuse3D = Clamp01f(diffuse);
    ctx->lightSpecular3D = Clamp01f(specular);
    Window_Invalidate(handle);
    return NATIVE_OK;
}

NativeResult Window_Get3DViewLightParameters(WindowHandle handle, float* ambient, float* diffuse, float* specular) {
    if (!handle) return NATIVE_E_INVALID_ARGUMENT;
    auto ctx = static_cast<WindowContext*>(handle);
    if (!ctx) return NATIVE_E_INVALID_ARGUMENT;
    if (!ambient || !diffuse || !specular) return NATIVE_E_INVALID_ARGUMENT;

    *ambient = ctx->lightAmbient3D;
    *diffuse = ctx->lightDiffuse3D;
    *specular = ctx->lightSpecular3D;
    return NATIVE_OK;
}

NativeResult Window_SetToolManager(WindowHandle handle, ToolManagerHandle toolManager) {
    if (!handle) return NATIVE_E_INVALID_ARGUMENT;
    auto ctx = static_cast<WindowContext*>(handle);
    ctx->toolManager = toolManager;
    return NATIVE_OK;
}

NativeResult Window_SetActiveTool(WindowHandle handle, ToolHandle tool) {
    if (!handle) return NATIVE_E_INVALID_ARGUMENT;
    auto ctx = static_cast<WindowContext*>(handle);
    ctx->activeTool = tool;
    return NATIVE_OK;
}

void Window_SetToolType(WindowHandle handle, int toolType) {
    
    g_currentToolType = toolType;
    g_measurementPoints.clear();
    g_isDrawing = false;
    
    if (!handle) {
        return;
    }
    
    auto ctx = static_cast<WindowContext*>(handle);
    
    // Update session-specific tool state
    std::string sessionId = GetSessionIdFromWindowContext(ctx);
    if (!sessionId.empty()) {
        SetSessionToolType(sessionId, toolType);
        SetSessionIsDrawing(sessionId, false);
    }
    
    // If switching to measurement tools (1-6), ensure ToolManager exists for this window
    if (toolType >= 1 && toolType <= 6) {
        const bool isMPR = ctx->rendererType == 1;
        const bool isAPR = ctx->rendererType == 0 || ctx->rendererType == 2;
        
        if (ctx->boundRenderer && (isMPR || isAPR)) {
            // Create ToolManager if it doesn't exist
            if (!ctx->toolManager) {
                printf("[Window_SetToolType] Creating ToolManager for window\n");
                ctx->toolManager = ToolManager_Create();
                
                // Create all measurement tools (tools are auto-registered with manager)
                ctx->tools[1] = Tool_CreateLine(ctx->toolManager);
                ctx->tools[2] = Tool_CreateAngle(ctx->toolManager);
                ctx->tools[3] = Tool_CreateRectangle(ctx->toolManager);
                ctx->tools[4] = Tool_CreateCircle(ctx->toolManager);
                ctx->tools[5] = Tool_CreateBezier(ctx->toolManager);
                ctx->tools[6] = Tool_CreateFreehand(ctx->toolManager);
                
                printf("[Window_SetToolType] Created 6 measurement tools\n");
            }
            
            // Activate the selected tool
            if (ctx->tools[toolType]) {
                ToolManager_SetActiveTool(ctx->toolManager, ctx->tools[toolType]);
                ctx->activeTool = ctx->tools[toolType];
                printf("[Window_SetToolType] Activated measurement tool type %d, activeTool=%p\n", 
                       toolType, ctx->activeTool);
            } else {
                printf("[Window_SetToolType] WARNING: Tool type %d not created\n", toolType);
            }
        } else {
            printf("[Window_SetToolType] Skipping tool creation: boundRenderer=%p, isMPR=%d, isAPR=%d\n",
                   ctx->boundRenderer, isMPR, isAPR);
        }
    } else {
        // Switching away from measurement tools - deactivate
        if (ctx->toolManager) {
            ToolManager_SetActiveTool(ctx->toolManager, nullptr);
            ctx->activeTool = nullptr;
            printf("[Window_SetToolType] Deactivated measurement tools\n");
        }
    }
}

void Window_Refresh(WindowHandle handle) {
    if (!handle) return;
    auto ctx = static_cast<WindowContext*>(handle);
#ifdef _WIN32
    if (ctx->hdc) {
        SwapBuffers(ctx->hdc);
    }
#else
    std::cout << "[Window_Refresh] GLFW path" << std::endl;
    if (ctx->window) {
        glfwMakeContextCurrent(ctx->window);
        glfwSwapBuffers(ctx->window);
        glfwPollEvents();
    }
#endif
}

void Window_MakeCurrent(WindowHandle handle) {
    if (!handle) return;
    auto ctx = static_cast<WindowContext*>(handle);
#ifdef _WIN32
    if (ctx->hdc && ctx->hglrc) {
        wglMakeCurrent(ctx->hdc, ctx->hglrc);

        // Legacy 3D render paths (e.g. APR_RenderOrthogonal3D) still read global g_3d*.
        // Keep globals in sync with per-window state for the *current* 3D window.
        if (ctx->is3DView) {
            g_3dRotX = ctx->viewRotX;
            g_3dRotY = ctx->viewRotY;
            g_3dZoom = ctx->viewZoom;
            g_3dPanX = ctx->viewPanX;
            g_3dPanY = ctx->viewPanY;

            if (ctx->viewRotMatInitialized) {
                std::memcpy(g_3dRotMat, ctx->viewRotMat, sizeof(float) * 16);
            } else {
                Mat4_Identity(g_3dRotMat);
            }
        }
    }
#else
    std::cout << "[Window_MakeCurrent] GLFW path" << std::endl;
    if (ctx->window) {
        glfwMakeContextCurrent(ctx->window);
    }
#endif
}

static void CopyCStringSafe(char* outBuf, int outBufSize, const char* src) {
    if (!outBuf || outBufSize <= 0) return;
    if (!src) src = "";
#ifdef _WIN32
    strncpy_s(outBuf, static_cast<size_t>(outBufSize), src, _TRUNCATE);
#else
    std::snprintf(outBuf, static_cast<size_t>(outBufSize), "%s", src);
#endif
    outBuf[outBufSize - 1] = '\0';
}

VIZ_API NativeResult Window_GetGLInfo(
    WindowHandle handle,
    char* outVendor,
    int vendorSize,
    char* outRenderer,
    int rendererSize,
    char* outVersion,
    int versionSize
) {
    if (!handle) {
        SetLastError("Invalid window handle");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    auto ctx = static_cast<WindowContext*>(handle);
    std::lock_guard<std::mutex> lock(ctx->renderMutex);

    Window_MakeCurrent(handle);

    const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));

    if (!vendor && !renderer && !version) {
        SetLastError("glGetString returned null (OpenGL context not current?)");
        return NATIVE_E_NOT_INITIALIZED;
    }

    CopyCStringSafe(outVendor, vendorSize, vendor);
    CopyCStringSafe(outRenderer, rendererSize, renderer);
    CopyCStringSafe(outVersion, versionSize, version);
    return NATIVE_OK;
}

void* Window_GetNativeHandle(WindowHandle handle) {
    if (!handle) return nullptr;
    auto ctx = static_cast<WindowContext*>(handle);
#ifdef _WIN32
    return ctx->hwnd;
#else
    return ctx->window;
#endif
}

bool Window_PollEvents(WindowHandle handle) {
    if (!handle) return false;
    auto ctx = static_cast<WindowContext*>(handle);
    
#ifdef _WIN32
    // Win32锟斤拷息循锟斤拷 - 锟斤拷锟斤拷锟斤拷锟斤拷锟竭程碉拷锟斤拷息锟斤拷NULL锟斤拷示锟斤拷锟叫达拷锟节ｏ拷
    // printf("[Window_PollEvents] Calling PeekMessage...\n");  // 太频锟斤拷锟斤拷注锟酵碉拷
    MSG msg;
    int msgCount = 0;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        msgCount++;
        if (msg.message == WM_QUIT) {
            return false;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    // if (msgCount > 0) printf("[Window_PollEvents] Processed %d messages\n", msgCount);
    return true;
#else
    std::cout << "[Window_PollEvents] GLFW path" << std::endl;
    if (!ctx->window) return false;
    glfwPollEvents();
    return !glfwWindowShouldClose(ctx->window);
#endif
}

// 刷锟铰达拷锟节层级锟斤拷锟斤拷Electron锟斤拷锟斤拷resize锟斤拷锟斤拷茫锟?
NativeResult Window_RefreshZOrder(WindowHandle handle) {
    if (!handle) {
        SetLastError("Invalid window handle");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    
    auto ctx = static_cast<WindowContext*>(handle);
    
#ifdef _WIN32
    HWND hwnd = ctx->hwnd;
    
    if (!hwnd || !IsWindow(hwnd)) {
        SetLastError("Invalid window handle");
        return NATIVE_E_INTERNAL_ERROR;
    }

    const bool hidden = ctx->isHidden;

    // 锟斤拷锟斤拷锟铰硷拷锟角讹拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟接︼拷锟轿伙拷煤痛锟叫?
    if (ctx->parentHwnd && IsWindow(ctx->parentHwnd)) {
        int width = ctx->embeddedRect.right - ctx->embeddedRect.left;
        int height = ctx->embeddedRect.bottom - ctx->embeddedRect.top;
        ctx->width = width;
        ctx->height = height;
        // Embedded into Electron: never fight for foreground/z-order.
        // Changing Z-order (HWND_TOP / TOPMOST toggling) during resize/maximize causes visible flicker.
        UINT childFlags = SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOZORDER;
        childFlags |= hidden ? SWP_HIDEWINDOW : SWP_SHOWWINDOW;

        if (!SetWindowPos(hwnd, nullptr, ctx->embeddedRect.left, ctx->embeddedRect.top,
                          width, height, childFlags)) {
            SetLastError("SetWindowPos failed when refreshing z-order");
            return NATIVE_E_INTERNAL_ERROR;
        }
    } else {
        // 锟斤拷锟斤拷锟斤拷锟节憋拷锟斤拷原锟斤拷锟竭硷拷
        UINT topmostFlags = SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE;
        if (hidden) {
            topmostFlags |= SWP_HIDEWINDOW;
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, topmostFlags);
            SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, topmostFlags);
        } else {
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                         topmostFlags | SWP_SHOWWINDOW);
            SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                         topmostFlags | SWP_SHOWWINDOW);
        }
    }

    if (hidden) {
        ShowWindow(hwnd, SW_HIDE);
    } else {
        // Show without activation.
        ShowWindow(hwnd, SW_SHOWNA);
        // 使锟斤拷锟届步 InvalidateRect 锟斤拷锟斤拷同锟斤拷 RedrawWindow锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
        InvalidateRect(hwnd, nullptr, FALSE);
    }

    return NATIVE_OK;
#else
    SetLastError("Window_RefreshZOrder only supported on Windows");
    return NATIVE_E_NOT_INITIALIZED;
#endif
}

NativeResult Window_SetParentWindow(WindowHandle handle, void* parentHwnd, int x, int y, int width, int height) {
    std::cout << "[Window_SetParentWindow] START - handle=" << handle << ", parent=" << parentHwnd << std::endl;
    if (!handle || !parentHwnd) {
        SetLastError("Invalid window handle or parent handle");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    
    auto ctx = static_cast<WindowContext*>(handle);
    
#ifdef _WIN32
    std::cout << "[Window_SetParentWindow] Win32 path" << std::endl;
    HWND hwnd = ctx->hwnd;
    HWND parentWnd = static_cast<HWND>(parentHwnd);
    
    if (!hwnd || !IsWindow(hwnd)) {
        SetLastError("Invalid window handle");
        return NATIVE_E_INTERNAL_ERROR;
    }
    
    if (!IsWindow(parentWnd)) {
        SetLastError("Invalid parent window handle");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    
    // 淇敼绐楀彛鏍峰紡涓哄瓙绐楀彛
    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    style &= ~(WS_POPUP | WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_THICKFRAME);
    // WS_CLIPSIBLINGS: 闃叉琚叾浠栧厔寮熺獥鍙ｈ鐩栨椂闂儊
    // WS_CLIPCHILDREN: 濡傛灉姝ょ獥鍙ｆ湁瀛愮獥鍙ｏ紝闃叉缁樺埗鍒板瓙绐楀彛鍖哄煙
    style |= WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    SetWindowLongPtr(hwnd, GWL_STYLE, style);
    
    // 锟睫革拷锟斤拷展锟斤拷式
    LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    exStyle &= ~(WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);
    exStyle |= WS_EX_NOPARENTNOTIFY;
    // Don't activate/focus the embedded HWND.
    exStyle |= WS_EX_NOACTIVATE;
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);
    
    // 锟斤拷锟矫革拷锟斤拷锟斤拷
    if (!SetParent(hwnd, parentWnd)) {
        SetLastError("SetParent failed");
        return NATIVE_E_INTERNAL_ERROR;
    }

    // 锟斤拷录锟斤拷锟斤拷锟节猴拷嵌锟斤拷锟斤拷锟津，猴拷锟斤拷刷锟斤拷/锟斤拷锟斤拷锟斤拷使锟斤拷
    ctx->parentHwnd = parentWnd;
    // Once embedded, the host controls window lifetime.
    ctx->allowClose = false;
    ctx->embeddedRect.left = x;
    ctx->embeddedRect.top = y;
    ctx->embeddedRect.right = x + width;
    ctx->embeddedRect.bottom = y + height;
    ctx->width = width;
    ctx->height = height;

    // 锟斤拷锟斤拷位锟矫和达拷小锟斤拷嵌锟诫到Electron锟酵伙拷锟斤拷锟叫ｏ拷锟斤拷锟斤拷要/锟斤拷应锟斤拷谋锟絑-order锟斤拷
    UINT embedFlags = SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOZORDER;
    embedFlags |= ctx->isHidden ? SWP_HIDEWINDOW : SWP_SHOWWINDOW;
    if (!SetWindowPos(hwnd, nullptr, x, y, width, height, embedFlags)) {
        SetLastError("SetWindowPos failed");
        return NATIVE_E_INTERNAL_ERROR;
    }

    // 确锟斤拷锟斤拷锟皆斤拷锟斤拷锟斤拷锟诫（锟斤拷锟斤拷状态锟铰憋拷锟街斤拷锟矫ｏ拷
    EnableWindow(hwnd, ctx->isHidden ? FALSE : TRUE);

    if (ctx->isHidden) {
        ShowWindow(hwnd, SW_HIDE);
    }

    // No aggressive z-order fix here; it causes maximize/resize flicker.

    printf("[Window_SetParentWindow] SUCCESS - window embedded at (%d,%d) %dx%d\n", x, y, width, height);
    
    return NATIVE_OK;
#else
    SetLastError("Window_SetParentWindow only supported on Windows");
    return NATIVE_E_NOT_INITIALIZED;
#endif
}

NativeResult Window_Resize(WindowHandle handle, int x, int y, int width, int height) {
    if (!handle) {
        SetLastError("Invalid window handle");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    
    auto ctx = static_cast<WindowContext*>(handle);
    
#ifdef _WIN32
    HWND hwnd = ctx->hwnd;
    
    if (!hwnd || !IsWindow(hwnd)) {
        SetLastError("Invalid window handle");
        return NATIVE_E_INTERNAL_ERROR;
    }
    
    // 锟斤拷录嵌锟斤拷锟斤拷锟津，癸拷锟斤拷锟斤拷刷锟斤拷使锟斤拷
    ctx->embeddedRect.left = x;
    ctx->embeddedRect.top = y;
    ctx->embeddedRect.right = x + width;
    ctx->embeddedRect.bottom = y + height;
    ctx->width = width;
    ctx->height = height;

    // Embedded resize: never change Z-order.
    UINT flags = SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOZORDER;
    flags |= ctx->isHidden ? SWP_HIDEWINDOW : SWP_SHOWWINDOW;
    if (!SetWindowPos(hwnd, nullptr, x, y, width, height, flags)) {
        SetLastError("SetWindowPos failed");
        return NATIVE_E_INTERNAL_ERROR;
    }

    if (!ctx->isHidden) {
        // 锟斤拷锟斤拷锟截伙拷锟斤拷锟斤拷应锟铰尺达拷
        Window_Invalidate(handle);
    } else {
        ShowWindow(hwnd, SW_HIDE);
    }
    return NATIVE_OK;
#else
    SetLastError("Window_Resize only supported on Windows");
    return NATIVE_E_NOT_INITIALIZED;
#endif
}

// 锟铰硷拷循锟斤拷锟竭程ｏ拷Win32锟芥本锟斤拷锟斤拷要锟斤拷锟斤拷为使锟斤拷PeekMessage锟斤拷锟斤拷锟斤拷
#ifndef _WIN32
static std::thread g_eventLoopThread;
static bool g_eventLoopRunning = false;
static std::mutex g_eventLoopMutex;

static void EventLoopThreadFunc() {
    while (g_eventLoopRunning) {
        // 锟节讹拷锟斤拷锟竭筹拷锟叫达拷锟斤拷GLFW锟铰硷拷锟斤拷锟斤拷锟斤拷Windows平台锟斤拷
        glfwPollEvents();
        
        // 锟斤拷要占锟斤拷太锟斤拷CPU锟斤拷锟斤拷微锟斤拷锟斤拷
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
#endif

NativeResult Window_StartEventLoop() {
#ifdef _WIN32
    // Windows使锟斤拷Win32锟斤拷息循锟斤拷锟斤拷锟斤拷锟斤拷要锟竭筹拷
    return NATIVE_OK;
#else
    std::lock_guard<std::mutex> lock(g_eventLoopMutex);
    
    if (g_eventLoopRunning) {
        return NATIVE_OK; // 锟窖撅拷锟斤拷锟斤拷锟斤拷
    }
    
    g_eventLoopRunning = true;
    g_eventLoopThread = std::thread(EventLoopThreadFunc);
    
    return NATIVE_OK;
#endif
}

void Window_StopEventLoop() {
#ifdef _WIN32
    // Windows锟斤拷锟斤拷要停止
    return;
#else
    std::lock_guard<std::mutex> lock(g_eventLoopMutex);
    
    if (!g_eventLoopRunning) {
        return;
    }
    
    g_eventLoopRunning = false;
    
    if (g_eventLoopThread.joinable()) {
        g_eventLoopThread.join();
    }
#endif
}

// ==================== Window Update API ====================

void Window_Invalidate(WindowHandle handle) {
    if (!handle) return;
    auto ctx = static_cast<WindowContext*>(handle);
    
    // 锟斤拷锟斤拷锟斤拷诒锟斤拷锟斤拷兀锟斤拷锟斤拷锟斤拷锟斤拷锟饺撅拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟?
    if (ctx->isHidden) {
        return;
    }

#ifdef _WIN32
    if (ctx->hwnd && IsWindow(ctx->hwnd)) {
        // 使锟斤拷原锟接憋拷志锟斤拷锟斤拷锟截革拷锟斤拷InvalidateRect锟斤拷锟矫ｏ拷锟斤拷止锟斤拷息锟斤拷锟叫堆伙拷锟斤拷
        bool expected = false;
        if (ctx->needsRedraw.compare_exchange_strong(expected, true)) {
            // 只锟叫碉拷needsRedraw锟斤拷false锟斤拷为true时锟脚碉拷锟斤拷InvalidateRect
            InvalidateRect(ctx->hwnd, nullptr, FALSE);
        }
        // 锟斤拷锟斤拷丫锟斤拷锟絧ending锟斤拷锟截伙拷锟斤拷锟斤拷锟斤拷锟斤拷锟剿次碉拷锟斤拷
    }

#else
    // 锟斤拷Windows平台锟斤拷GLFW锟竭硷拷
    if (ctx->window) {
        glfwMakeContextCurrent(ctx->window);
        if (ctx->boundRenderer) {
            if (ctx->rendererType == 0 || ctx->rendererType == 2) {
                APR_Render(static_cast<APRHandle>(ctx->boundRenderer));
            }
            else if (ctx->rendererType == 1) {
                MPR_Render(static_cast<MPRHandle>(ctx->boundRenderer));
            }
            glfwSwapBuffers(ctx->window);
        }
    }
#endif
}

void Window_InvalidateAll() {
    static int frameCount = 0;
    frameCount++;
    
    // 每60帧锟斤拷1锟诫）锟斤拷印一锟轿碉拷锟斤拷锟斤拷息
    if (frameCount % 60 == 0) {
        for (size_t i = 0; i < g_AllWindows.size(); i++) {
            auto ctx = static_cast<WindowContext*>(g_AllWindows[i]);
        }
    }
    
    // 锟斤拷锟叫碉拷锟矫ｏ拷InvalidateRect只锟角凤拷锟斤拷锟斤拷息锟斤拷锟角筹拷锟届，锟斤拷锟斤拷要锟斤拷锟竭程ｏ拷
    for (auto handle : g_AllWindows) {
        Window_Invalidate(handle);
    }
}

// ==================== 锟教讹拷帧锟斤拷锟斤拷染锟竭筹拷 ====================
static std::thread g_RenderThread;
static std::atomic<bool> g_RenderThreadRunning(false);
static int g_TargetFPS = 30;  // 默锟斤拷30fps锟斤拷锟斤拷锟斤拷CPU锟斤拷锟斤拷

static void RenderThreadFunc() {
    int frameTimeMs = 1000 / g_TargetFPS;
    int consecutiveSlowFrames = 0;  // 锟斤拷锟斤拷锟斤拷帧锟斤拷锟斤拷
    
    while (g_RenderThreadRunning) {
        auto startTime = std::chrono::steady_clock::now();
        
        // 锟斤拷锟斤拷锟斤拷锟叫达拷锟斤拷锟截伙拷
        Window_InvalidateAll();
        
        // 锟斤拷锟斤拷实锟斤拷锟斤拷染锟斤拷时
        auto endTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        
        // 锟斤拷锟斤拷应帧锟绞ｏ拷锟斤拷锟斤拷锟斤拷锟?帧锟斤拷时锟斤拷锟斤拷锟斤拷目锟斤拷帧锟斤拷
        if (elapsed > frameTimeMs) {
            consecutiveSlowFrames++;
            if (consecutiveSlowFrames >= 3 && g_TargetFPS > 15) {
                g_TargetFPS = std::max(15, g_TargetFPS - 5);  // 锟斤拷锟?5fps
                frameTimeMs = 1000 / g_TargetFPS;
                consecutiveSlowFrames = 0;
            }
        } else {
            consecutiveSlowFrames = 0;
        }
        
        // 锟斤拷锟竭碉拷锟斤拷一帧
        int sleepTime = frameTimeMs - static_cast<int>(elapsed);
        if (sleepTime > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
        }
    }
}

NativeResult Window_StartRenderLoop(int targetFPS) {
    if (g_RenderThreadRunning) {
        SetLastError("Render loop already running");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    
    if (targetFPS <= 0 || targetFPS > 240) {
        SetLastError("Invalid target FPS (must be 1-240)");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    
    g_TargetFPS = targetFPS;
    g_RenderThreadRunning = true;
    g_RenderThread = std::thread(RenderThreadFunc);
    return NATIVE_OK;
}

void Window_StopRenderLoop() {
    if (g_RenderThreadRunning) {
        g_RenderThreadRunning = false;
        if (g_RenderThread.joinable()) {
            g_RenderThread.join();
        }
    }
}

// ==================== Offscreen Rendering ====================
WindowHandle OffscreenContext_Create(int width, int height) {
    
#ifdef _WIN32
    if (width <= 0 || height <= 0) {
        SetLastError("Invalid offscreen size");
        return nullptr;
    }

    if (!RegisterWin32WindowClass()) {
        char err[256];
        sprintf_s(err, sizeof(err), "Failed to register window class, error: %lu", GetLastError());
        SetLastError(err);
        return nullptr;
    }

    HWND hwnd = CreateWindowEx(
        0,
        g_WindowClassName,
        L"Offscreen",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        nullptr, nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );
    if (!hwnd) {
        char err[256];
        sprintf_s(err, sizeof(err), "Failed to create Win32 offscreen window, error: %lu", GetLastError());
        SetLastError(err);
        return nullptr;
    }
    ShowWindow(hwnd, SW_HIDE);

    HDC hdc = GetDC(hwnd);
    if (!hdc) {
        SetLastError("Failed to get DC for offscreen window");
        DestroyWindow(hwnd);
        return nullptr;
    }

    PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR) };
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pixelFormat = ChoosePixelFormat(hdc, &pfd);
    if (!pixelFormat) {
        SetLastError("Failed to choose pixel format for offscreen window");
        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);
        return nullptr;
    }
    if (!SetPixelFormat(hdc, pixelFormat, &pfd)) {
        SetLastError("Failed to set pixel format for offscreen window");
        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);
        return nullptr;
    }

    HGLRC hglrc = wglCreateContext(hdc);
    if (!hglrc) {
        SetLastError("Failed to create OpenGL context for offscreen window");
        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);
        return nullptr;
    }

    if (g_SharedGLContext) {
        if (!wglShareLists(g_SharedGLContext, hglrc)) {
            SetLastError("Failed to share OpenGL contexts for offscreen window");
        }
    } else {
        g_SharedGLContext = hglrc;
    }

    if (!wglMakeCurrent(hdc, hglrc)) {
        SetLastError("wglMakeCurrent failed for offscreen window");
        wglDeleteContext(hglrc);
        ReleaseDC(hwnd, hdc);
        DestroyWindow(hwnd);
        return nullptr;
    }

    static bool glewInitialized = false;
    if (!glewInitialized) {
        if (glewInit() != GLEW_OK) {
            SetLastError("Failed to initialize GLEW (offscreen)");
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(hglrc);
            ReleaseDC(hwnd, hdc);
            DestroyWindow(hwnd);
            return nullptr;
        }
        glewInitialized = true;
    }

    glViewport(0, 0, width, height);

    auto ctx = new OffscreenContext();
    ctx->hwnd = hwnd;
    ctx->hdc = hdc;
    ctx->hglrc = hglrc;
    ctx->width = width;
    ctx->height = height;
    return ctx;
#else
    // 锟斤拷Windows平台锟斤拷使锟斤拷GLFW锟斤拷锟斤拷锟斤拷锟斤拷
    if (!glfwInit()) {
        SetLastError("Failed to initialize GLFW");
        return nullptr;
    }
    
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(width, height, "Offscreen", nullptr, nullptr);
    if (!window) {
        SetLastError("Failed to create offscreen context");
        glfwTerminate();
        return nullptr;
    }
    
    glfwMakeContextCurrent(window);
    
    if (glewInit() != GLEW_OK) {
        SetLastError("Failed to initialize GLEW");
        glfwDestroyWindow(window);
        glfwTerminate();
        return nullptr;
    }
    
    auto ctx = new OffscreenContext();
    ctx->window = window;
    ctx->width = width;
    ctx->height = height;
    
    return ctx;
#endif
}

void OffscreenContext_Destroy(WindowHandle handle) {
#ifdef _WIN32
    if (!handle) return;
    auto ctx = static_cast<OffscreenContext*>(handle);
    if (ctx->hglrc) {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(ctx->hglrc);
        ctx->hglrc = nullptr;
    }
    if (ctx->hwnd && ctx->hdc) {
        ReleaseDC(ctx->hwnd, ctx->hdc);
        ctx->hdc = nullptr;
    }
    if (ctx->hwnd) {
        DestroyWindow(ctx->hwnd);
        ctx->hwnd = nullptr;
    }
    delete ctx;
#else
    // 锟斤拷Windows平台锟斤拷锟斤拷锟斤拷GLFW锟斤拷锟斤拷
    if (!handle) return;
    auto ctx = static_cast<OffscreenContext*>(handle);
    if (ctx->window) {
        glfwDestroyWindow(ctx->window);
    }
    delete ctx;
#endif
}

FrameBuffer* OffscreenContext_RenderToBuffer(WindowHandle handle, void* rendererHandle, int rendererType, int targetWidth, int targetHeight) {
#ifdef _WIN32
    if (!handle || !rendererHandle || targetWidth <= 0 || targetHeight <= 0) return nullptr;
    auto ctx = static_cast<OffscreenContext*>(handle);
    if (!ctx->hdc || !ctx->hglrc) {
        SetLastError("Invalid offscreen context (Win32)");
        return nullptr;
    }

    if (!wglMakeCurrent(ctx->hdc, ctx->hglrc)) {
        SetLastError("wglMakeCurrent failed (offscreen render)");
        return nullptr;
    }

    glViewport(0, 0, targetWidth, targetHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    NativeResult result = NATIVE_OK;
    if (rendererType == 0) {
        APRHandle apr = static_cast<APRHandle>(rendererHandle);
        // Ensure CPU-side buffer is ready in case GL texture sharing isn't available.
        APR_UpdateSlice(apr);
        result = APR_Render(apr);
        if (result != NATIVE_OK) {
            SetLastError("APR_Render failed");
            return nullptr;
        }
    } else if (rendererType == 1) {
        MPRHandle mpr = static_cast<MPRHandle>(rendererHandle);
        result = MPR_Render(mpr);
        if (result != NATIVE_OK) {
            SetLastError("MPR_Render failed");
            return nullptr;
        }
    } else if (rendererType == 2) {
        // Orthogonal 3D tri-planar render: rendererHandle points to 3 APRHandle values.
        APRHandle* tri = static_cast<APRHandle*>(rendererHandle);
        if (!tri[0] || !tri[1] || !tri[2]) {
            SetLastError("Invalid tri-planar APR handles for 3D render");
            return nullptr;
        }
        // Make sure CPU buffers exist for upload fallback.
        APR_UpdateSlice(tri[0]);
        APR_UpdateSlice(tri[1]);
        APR_UpdateSlice(tri[2]);

        result = APR_RenderOrthogonal3D(tri[0], tri[1], tri[2]);
        if (result != NATIVE_OK) {
            SetLastError("APR_RenderOrthogonal3D failed");
            return nullptr;
        }
    } else {
        SetLastError("Unsupported renderer type");
        return nullptr;
    }

    glFinish();

    auto buffer = new FrameBuffer();
    buffer->width = targetWidth;
    buffer->height = targetHeight;
    buffer->pixelCount = buffer->width * buffer->height;
    buffer->pixels = new unsigned char[buffer->pixelCount * 4];

    glReadPixels(0, 0, targetWidth, targetHeight, GL_RGBA, GL_UNSIGNED_BYTE, buffer->pixels);

    unsigned char* flipped = new unsigned char[buffer->pixelCount * 4];
    for (int y = 0; y < targetHeight; ++y) {
        memcpy(
            flipped + y * targetWidth * 4,
            buffer->pixels + (targetHeight - 1 - y) * targetWidth * 4,
            targetWidth * 4
        );
    }
    delete[] buffer->pixels;
    buffer->pixels = flipped;

    return buffer;
#else
    if (!handle || !rendererHandle || targetWidth <= 0 || targetHeight <= 0) return nullptr;
    auto ctx = static_cast<OffscreenContext*>(handle);
    
    // 锟叫伙拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
    glfwMakeContextCurrent(ctx->window);
    
    // 锟斤拷锟斤拷锟接匡拷为目锟斤拷叽锟?
    glViewport(0, 0, targetWidth, targetHeight);
    
    // 锟斤拷毡锟斤拷锟?
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 锟斤拷锟斤拷锟斤拷染锟斤拷锟斤拷锟酵碉拷锟斤拷锟斤拷应锟斤拷锟斤拷染锟斤拷锟斤拷
    NativeResult result = NATIVE_OK;
    
    if (rendererType == 0) {  // APR
        APRHandle apr = static_cast<APRHandle>(rendererHandle);
        result = APR_Render(apr);
        
        if (result != NATIVE_OK) {
            SetLastError("APR_Render failed");
            return nullptr;
        }
        
        // 锟斤拷帧锟斤拷锟斤拷锟饺★拷锟斤拷锟斤拷锟斤拷锟?
        auto buffer = new FrameBuffer();
        buffer->width = targetWidth;
        buffer->height = targetHeight;
        buffer->pixelCount = buffer->width * buffer->height;
        buffer->pixels = new unsigned char[buffer->pixelCount * 4]; // RGBA
        
        // 锟斤拷取帧锟斤拷锟斤拷锟斤拷锟斤拷
        glReadPixels(0, 0, targetWidth, targetHeight, GL_RGBA, GL_UNSIGNED_BYTE, buffer->pixels);
        
        // OpenGL 锟斤拷锟斤拷锟斤拷系锟角底诧拷锟斤拷始锟侥ｏ拷锟斤拷要锟斤拷转 Y 锟斤拷
        unsigned char* flipped = new unsigned char[buffer->pixelCount * 4];
        for (int y = 0; y < targetHeight; ++y) {
            memcpy(
                flipped + y * targetWidth * 4,
                buffer->pixels + (targetHeight - 1 - y) * targetWidth * 4,
                targetWidth * 4
            );
        }
        delete[] buffer->pixels;
        buffer->pixels = flipped;
        
        return buffer;
        
    } else if (rendererType == 1) {  // MPR
        MPRHandle mpr = static_cast<MPRHandle>(rendererHandle);
        result = MPR_Render(mpr);
        
        if (result != NATIVE_OK) {
            SetLastError("MPR_Render failed");
            return nullptr;
        }
        
        // 锟斤拷帧锟斤拷锟斤拷锟饺★拷锟斤拷锟斤拷锟斤拷锟?
        auto buffer = new FrameBuffer();
        buffer->width = targetWidth;
        buffer->height = targetHeight;
        buffer->pixelCount = buffer->width * buffer->height;
        buffer->pixels = new unsigned char[buffer->pixelCount * 4]; // RGBA
        
        // 锟斤拷取帧锟斤拷锟斤拷锟斤拷锟斤拷
        glReadPixels(0, 0, targetWidth, targetHeight, GL_RGBA, GL_UNSIGNED_BYTE, buffer->pixels);
        
        // OpenGL 锟斤拷锟斤拷锟斤拷系锟角底诧拷锟斤拷始锟侥ｏ拷锟斤拷要锟斤拷转 Y 锟斤拷
        unsigned char* flipped = new unsigned char[buffer->pixelCount * 4];
        for (int y = 0; y < targetHeight; ++y) {
            memcpy(
                flipped + y * targetWidth * 4,
                buffer->pixels + (targetHeight - 1 - y) * targetWidth * 4,
                targetWidth * 4
            );
        }
        delete[] buffer->pixels;
        buffer->pixels = flipped;
        
        return buffer;
        
    } else {
        SetLastError("Unsupported renderer type");
        return nullptr;
    }
#endif
}

void FrameBuffer_Destroy(FrameBuffer* buffer) {
    if (!buffer) return;
    delete[] buffer->pixels;
    delete buffer;
}

// ==================== Completed Measurements API ====================

VIZ_API int Measurement_GetCompletedCount() {
    std::lock_guard<std::mutex> lock(g_completedMeasurementsMutex);
    return (int)g_completedMeasurements.size();
}

VIZ_API int Measurement_GetCompletedList(CompletedMeasurementInfo* outItems, int maxItems) {
    if (!outItems || maxItems <= 0) return 0;

    std::lock_guard<std::mutex> lock(g_completedMeasurementsMutex);
    int total = (int)g_completedMeasurements.size();
    int count = (total < maxItems) ? total : maxItems;

    for (int i = 0; i < count; ++i) {
        const auto& m = g_completedMeasurements[(size_t)i];
        CompletedMeasurementInfo info{};
        info.id = m.id;
        info.toolType = m.toolType;
        info.result = m.result;
        info.isAPR = m.location.isAPR;
        info.sliceDirection = m.location.sliceDirection;
        info.sliceIndex = m.location.sliceIndex;
        info.centerX = m.location.centerX;
        info.centerY = m.location.centerY;
        info.centerZ = m.location.centerZ;
        info.rotX = m.location.rotX;
        info.rotY = m.location.rotY;
        info.rotZ = m.location.rotZ;
        SafeCopySessionId(info.sessionId, m.sessionId);
        outItems[i] = info;
    }

    return count;
}

static bool FindCompletedMeasurementByIdLocked(int measurementId, CompletedMeasurement& outMeasurement) {
    for (const auto& m : g_completedMeasurements) {
        if (m.id == measurementId) {
            outMeasurement = m;
            return true;
        }
    }
    return false;
}

VIZ_API bool Measurement_Delete(int measurementId) {
    std::lock_guard<std::mutex> lock(g_completedMeasurementsMutex);
    auto it = std::find_if(g_completedMeasurements.begin(), g_completedMeasurements.end(),
        [&](const CompletedMeasurement& m) { return m.id == measurementId; });
    if (it == g_completedMeasurements.end()) return false;

    const int removedIndex = (int)std::distance(g_completedMeasurements.begin(), it);
    g_completedMeasurements.erase(it);

    // Keep hover/drag indices consistent
    if (g_hoverMeasurementIndex == removedIndex) g_hoverMeasurementIndex = -1;
    else if (g_hoverMeasurementIndex > removedIndex) g_hoverMeasurementIndex--;

    if (g_dragMeasurementIndex == removedIndex) {
        g_dragMeasurementIndex = -1;
        g_dragPointIndex = -1;
        g_isDraggingPoint = false;
    } else if (g_dragMeasurementIndex > removedIndex) {
        g_dragMeasurementIndex--;
    }

    return true;
}

// ==================== Session-aware Measurement APIs ====================

VIZ_API int Measurement_GetCompletedCountForSession(const char* sessionId) {
    if (!sessionId) return Measurement_GetCompletedCount();
    TabSessionContext* ctx = FindTabSession(sessionId);
    if (!ctx) return Measurement_GetCompletedCount();
    
    std::lock_guard<std::mutex> lock(ctx->measurementsMutex);
    return (int)ctx->completedMeasurements.size();
}

VIZ_API int Measurement_GetCompletedListForSession(const char* sessionId, CompletedMeasurementInfo* outItems, int maxItems) {
    if (!outItems || maxItems <= 0) return 0;
    
    if (!sessionId) return Measurement_GetCompletedList(outItems, maxItems);
    TabSessionContext* ctx = FindTabSession(sessionId);
    if (!ctx) return Measurement_GetCompletedList(outItems, maxItems);
    
    std::lock_guard<std::mutex> lock(ctx->measurementsMutex);
    int total = (int)ctx->completedMeasurements.size();
    int count = (total < maxItems) ? total : maxItems;
    
    for (int i = 0; i < count; ++i) {
        const auto& m = ctx->completedMeasurements[(size_t)i];
        CompletedMeasurementInfo info{};
        info.id = m.id;
        info.toolType = m.toolType;
        info.result = m.result;
        info.isAPR = m.location.isAPR;
        info.sliceDirection = m.location.sliceDirection;
        info.sliceIndex = m.location.sliceIndex;
        info.centerX = m.location.centerX;
        info.centerY = m.location.centerY;
        info.centerZ = m.location.centerZ;
        info.rotX = m.location.rotX;
        info.rotY = m.location.rotY;
        info.rotZ = m.location.rotZ;
        SafeCopySessionId(info.sessionId, m.sessionId);
        outItems[i] = info;
    }
    
    return count;
}

VIZ_API bool Measurement_DeleteForSession(const char* sessionId, int measurementId) {
    if (!sessionId) return Measurement_Delete(measurementId);
    TabSessionContext* ctx = FindTabSession(sessionId);
    if (!ctx) return Measurement_Delete(measurementId);
    
    std::lock_guard<std::mutex> lock(ctx->measurementsMutex);
    auto it = std::find_if(ctx->completedMeasurements.begin(), ctx->completedMeasurements.end(),
        [&](const CompletedMeasurement& m) { return m.id == measurementId; });
    if (it == ctx->completedMeasurements.end()) return false;
    
    const int removedIndex = (int)std::distance(ctx->completedMeasurements.begin(), it);
    ctx->completedMeasurements.erase(it);
    
    // Keep hover/drag indices consistent
    if (ctx->hoverMeasurementIndex == removedIndex) ctx->hoverMeasurementIndex = -1;
    else if (ctx->hoverMeasurementIndex > removedIndex) ctx->hoverMeasurementIndex--;
    
    if (ctx->dragMeasurementIndex == removedIndex) {
        ctx->dragMeasurementIndex = -1;
        ctx->dragPointIndex = -1;
        ctx->isDraggingPoint = false;
    } else if (ctx->dragMeasurementIndex > removedIndex) {
        ctx->dragMeasurementIndex--;
    }
    
    return true;
}

VIZ_API void Measurement_ClearAllForSession(const char* sessionId) {
    if (!sessionId) {
        std::lock_guard<std::mutex> lock(g_completedMeasurementsMutex);
        g_completedMeasurements.clear();
        g_hoverMeasurementIndex = -1;
        g_dragMeasurementIndex = -1;
        g_dragPointIndex = -1;
        g_isDraggingPoint = false;
        return;
    }
    TabSessionContext* ctx = FindTabSession(sessionId);
    if (!ctx) return;
    
    std::lock_guard<std::mutex> lock(ctx->measurementsMutex);
    ctx->completedMeasurements.clear();
    ctx->hoverMeasurementIndex = -1;
    ctx->dragMeasurementIndex = -1;
    ctx->dragPointIndex = -1;
    ctx->isDraggingPoint = false;
}

// ==================== End Session-aware Measurement APIs ====================

VIZ_API NativeResult Measurement_GetRegionHistogram(
    const char* sessionId,
    int measurementId,
    int outBins[256],
    int* outMinValue,
    int* outMaxValue) {

    if (!sessionId || !outBins || !outMinValue || !outMaxValue) return NATIVE_E_INVALID_ARGUMENT;
    for (int i = 0; i < 256; ++i) outBins[i] = 0;

    CompletedMeasurement m;
    {
        std::lock_guard<std::mutex> lock(g_completedMeasurementsMutex);
        if (!FindCompletedMeasurementByIdLocked(measurementId, m)) return NATIVE_E_INVALID_ARGUMENT;
    }

    if (m.sessionId != sessionId) return NATIVE_E_INVALID_ARGUMENT;
    if (!(m.toolType == 3 || m.toolType == 4)) return NATIVE_E_INVALID_ARGUMENT;
    if (m.points.size() < 2) return NATIVE_E_INVALID_ARGUMENT;

    MPRContext* mprCtx = GetMPRContextFromSession(sessionId);
    if (!mprCtx || !mprCtx->volume) return NATIVE_E_INVALID_ARGUMENT;

    int width = 0, height = 0, depth = 0;
    if (Dicom_Volume_GetDimensions(mprCtx->volume, &width, &height, &depth) != NATIVE_OK) {
        return NATIVE_E_INTERNAL_ERROR;
    }

    uint16_t* volumeData = static_cast<uint16_t*>(Dicom_Volume_GetData(mprCtx->volume));
    if (!volumeData) return NATIVE_E_INTERNAL_ERROR;

    const int dir = m.location.sliceDirection;

    // ROI geometry in voxel coordinates
    const auto& p0 = m.points[0];
    const auto& p1 = m.points[1];

    auto clampi = [](int v, int lo, int hi) { return (v < lo) ? lo : (v > hi) ? hi : v; };

    // First pass: collect min/max HU and count
    int minHU = 0;
    int maxHU = 0;
    bool hasAny = false;

    auto visitVoxel = [&](int xi, int yi, int zi) {
        if (xi < 0 || yi < 0 || zi < 0 || xi >= width || yi >= height || zi >= depth) return;
        uint16_t v = SampleVolume(volumeData, width, height, depth, (float)xi, (float)yi, (float)zi);
        int hu = (int)v - 1024;
        if (!hasAny) {
            minHU = maxHU = hu;
            hasAny = true;
        } else {
            if (hu < minHU) minHU = hu;
            if (hu > maxHU) maxHU = hu;
        }
    };

    if (m.toolType == 3) {
        // Rectangle: p0 and p1 are opposite corners.
        if (dir == 0) {
            int z = clampi((int)std::round(m.location.sliceIndex), 0, depth - 1);
            int x0 = clampi((int)std::floor(std::min(p0.x, p1.x)), 0, width - 1);
            int x1 = clampi((int)std::ceil(std::max(p0.x, p1.x)), 0, width - 1);
            int y0 = clampi((int)std::floor(std::min(p0.y, p1.y)), 0, height - 1);
            int y1 = clampi((int)std::ceil(std::max(p0.y, p1.y)), 0, height - 1);
            for (int y = y0; y <= y1; ++y) for (int x = x0; x <= x1; ++x) visitVoxel(x, y, z);
        } else if (dir == 1) {
            int y = clampi((int)std::round(m.location.sliceIndex), 0, height - 1);
            int x0 = clampi((int)std::floor(std::min(p0.x, p1.x)), 0, width - 1);
            int x1 = clampi((int)std::ceil(std::max(p0.x, p1.x)), 0, width - 1);
            int z0 = clampi((int)std::floor(std::min(p0.z, p1.z)), 0, depth - 1);
            int z1 = clampi((int)std::ceil(std::max(p0.z, p1.z)), 0, depth - 1);
            for (int z = z0; z <= z1; ++z) for (int x = x0; x <= x1; ++x) visitVoxel(x, y, z);
        } else {
            int x = clampi((int)std::round(m.location.sliceIndex), 0, width - 1);
            int y0 = clampi((int)std::floor(std::min(p0.y, p1.y)), 0, height - 1);
            int y1 = clampi((int)std::ceil(std::max(p0.y, p1.y)), 0, height - 1);
            int z0 = clampi((int)std::floor(std::min(p0.z, p1.z)), 0, depth - 1);
            int z1 = clampi((int)std::ceil(std::max(p0.z, p1.z)), 0, depth - 1);
            for (int z = z0; z <= z1; ++z) for (int y = y0; y <= y1; ++y) visitVoxel(x, y, z);
        }
    } else if (m.toolType == 4) {
        // Circle: p0 center, p1 edge.
        if (dir == 0) {
            int z = clampi((int)std::round(m.location.sliceIndex), 0, depth - 1);
            float cx = p0.x;
            float cy = p0.y;
            float r = std::sqrt((p1.x - cx) * (p1.x - cx) + (p1.y - cy) * (p1.y - cy));
            int x0 = clampi((int)std::floor(cx - r), 0, width - 1);
            int x1 = clampi((int)std::ceil(cx + r), 0, width - 1);
            int y0 = clampi((int)std::floor(cy - r), 0, height - 1);
            int y1 = clampi((int)std::ceil(cy + r), 0, height - 1);
            float r2 = r * r;
            for (int y = y0; y <= y1; ++y) {
                for (int x = x0; x <= x1; ++x) {
                    float dx = (float)x - cx;
                    float dy = (float)y - cy;
                    if (dx * dx + dy * dy <= r2) visitVoxel(x, y, z);
                }
            }
        } else if (dir == 1) {
            int y = clampi((int)std::round(m.location.sliceIndex), 0, height - 1);
            float cx = p0.x;
            float cz = p0.z;
            float r = std::sqrt((p1.x - cx) * (p1.x - cx) + (p1.z - cz) * (p1.z - cz));
            int x0 = clampi((int)std::floor(cx - r), 0, width - 1);
            int x1 = clampi((int)std::ceil(cx + r), 0, width - 1);
            int z0 = clampi((int)std::floor(cz - r), 0, depth - 1);
            int z1 = clampi((int)std::ceil(cz + r), 0, depth - 1);
            float r2 = r * r;
            for (int z = z0; z <= z1; ++z) {
                for (int x = x0; x <= x1; ++x) {
                    float dx = (float)x - cx;
                    float dz = (float)z - cz;
                    if (dx * dx + dz * dz <= r2) visitVoxel(x, y, z);
                }
            }
        } else {
            int x = clampi((int)std::round(m.location.sliceIndex), 0, width - 1);
            float cy = p0.y;
            float cz = p0.z;
            float r = std::sqrt((p1.y - cy) * (p1.y - cy) + (p1.z - cz) * (p1.z - cz));
            int y0 = clampi((int)std::floor(cy - r), 0, height - 1);
            int y1 = clampi((int)std::ceil(cy + r), 0, height - 1);
            int z0 = clampi((int)std::floor(cz - r), 0, depth - 1);
            int z1 = clampi((int)std::ceil(cz + r), 0, depth - 1);
            float r2 = r * r;
            for (int z = z0; z <= z1; ++z) {
                for (int y2 = y0; y2 <= y1; ++y2) {
                    float dy = (float)y2 - cy;
                    float dz = (float)z - cz;
                    if (dy * dy + dz * dz <= r2) visitVoxel(x, y2, z);
                }
            }
        }
    }

    if (!hasAny) {
        *outMinValue = 0;
        *outMaxValue = 0;
        return NATIVE_E_INTERNAL_ERROR;
    }

    *outMinValue = minHU;
    *outMaxValue = maxHU;

    const int range = (maxHU - minHU);
    if (range <= 0) {
        outBins[128] = 1;
        return NATIVE_OK;
    }

    auto addBin = [&](int xi, int yi, int zi) {
        if (xi < 0 || yi < 0 || zi < 0 || xi >= width || yi >= height || zi >= depth) return;
        uint16_t v = SampleVolume(volumeData, width, height, depth, (float)xi, (float)yi, (float)zi);
        int hu = (int)v - 1024;
        int bin = (int)((double)(hu - minHU) * 255.0 / (double)range);
        if (bin < 0) bin = 0;
        if (bin > 255) bin = 255;
        outBins[bin]++;
    };

    // Second pass: fill bins
    if (m.toolType == 3) {
        if (dir == 0) {
            int z = clampi((int)std::round(m.location.sliceIndex), 0, depth - 1);
            int x0 = clampi((int)std::floor(std::min(p0.x, p1.x)), 0, width - 1);
            int x1 = clampi((int)std::ceil(std::max(p0.x, p1.x)), 0, width - 1);
            int y0 = clampi((int)std::floor(std::min(p0.y, p1.y)), 0, height - 1);
            int y1 = clampi((int)std::ceil(std::max(p0.y, p1.y)), 0, height - 1);
            for (int y = y0; y <= y1; ++y) for (int x2 = x0; x2 <= x1; ++x2) addBin(x2, y, z);
        } else if (dir == 1) {
            int y = clampi((int)std::round(m.location.sliceIndex), 0, height - 1);
            int x0 = clampi((int)std::floor(std::min(p0.x, p1.x)), 0, width - 1);
            int x1 = clampi((int)std::ceil(std::max(p0.x, p1.x)), 0, width - 1);
            int z0 = clampi((int)std::floor(std::min(p0.z, p1.z)), 0, depth - 1);
            int z1 = clampi((int)std::ceil(std::max(p0.z, p1.z)), 0, depth - 1);
            for (int z = z0; z <= z1; ++z) for (int x2 = x0; x2 <= x1; ++x2) addBin(x2, y, z);
        } else {
            int x = clampi((int)std::round(m.location.sliceIndex), 0, width - 1);
            int y0 = clampi((int)std::floor(std::min(p0.y, p1.y)), 0, height - 1);
            int y1 = clampi((int)std::ceil(std::max(p0.y, p1.y)), 0, height - 1);
            int z0 = clampi((int)std::floor(std::min(p0.z, p1.z)), 0, depth - 1);
            int z1 = clampi((int)std::ceil(std::max(p0.z, p1.z)), 0, depth - 1);
            for (int z = z0; z <= z1; ++z) for (int y2 = y0; y2 <= y1; ++y2) addBin(x, y2, z);
        }
    } else {
        if (dir == 0) {
            int z = clampi((int)std::round(m.location.sliceIndex), 0, depth - 1);
            float cx = p0.x;
            float cy = p0.y;
            float r = std::sqrt((p1.x - cx) * (p1.x - cx) + (p1.y - cy) * (p1.y - cy));
            int x0 = clampi((int)std::floor(cx - r), 0, width - 1);
            int x1 = clampi((int)std::ceil(cx + r), 0, width - 1);
            int y0 = clampi((int)std::floor(cy - r), 0, height - 1);
            int y1 = clampi((int)std::ceil(cy + r), 0, height - 1);
            float r2 = r * r;
            for (int y = y0; y <= y1; ++y) {
                for (int x2 = x0; x2 <= x1; ++x2) {
                    float dx = (float)x2 - cx;
                    float dy = (float)y - cy;
                    if (dx * dx + dy * dy <= r2) addBin(x2, y, z);
                }
            }
        } else if (dir == 1) {
            int y = clampi((int)std::round(m.location.sliceIndex), 0, height - 1);
            float cx = p0.x;
            float cz = p0.z;
            float r = std::sqrt((p1.x - cx) * (p1.x - cx) + (p1.z - cz) * (p1.z - cz));
            int x0 = clampi((int)std::floor(cx - r), 0, width - 1);
            int x1 = clampi((int)std::ceil(cx + r), 0, width - 1);
            int z0 = clampi((int)std::floor(cz - r), 0, depth - 1);
            int z1 = clampi((int)std::ceil(cz + r), 0, depth - 1);
            float r2 = r * r;
            for (int z = z0; z <= z1; ++z) {
                for (int x2 = x0; x2 <= x1; ++x2) {
                    float dx = (float)x2 - cx;
                    float dz = (float)z - cz;
                    if (dx * dx + dz * dz <= r2) addBin(x2, y, z);
                }
            }
        } else {
            int x = clampi((int)std::round(m.location.sliceIndex), 0, width - 1);
            float cy = p0.y;
            float cz = p0.z;
            float r = std::sqrt((p1.y - cy) * (p1.y - cy) + (p1.z - cz) * (p1.z - cz));
            int y0 = clampi((int)std::floor(cy - r), 0, height - 1);
            int y1 = clampi((int)std::ceil(cy + r), 0, height - 1);
            int z0 = clampi((int)std::floor(cz - r), 0, depth - 1);
            int z1 = clampi((int)std::ceil(cz + r), 0, depth - 1);
            float r2 = r * r;
            for (int z = z0; z <= z1; ++z) {
                for (int y2 = y0; y2 <= y1; ++y2) {
                    float dy = (float)y2 - cy;
                    float dz = (float)z - cz;
                    if (dy * dy + dz * dz <= r2) addBin(x, y2, z);
                }
            }
        }
    }

    return NATIVE_OK;
}

VIZ_API int Measurement_GetProfileData(const char* sessionId, int measurementId, double* outAxis, double* outValues, int maxPoints) {
    if (!sessionId || !outAxis || !outValues || maxPoints <= 0) return 0;

    CompletedMeasurement m;
    {
        std::lock_guard<std::mutex> lock(g_completedMeasurementsMutex);
        if (!FindCompletedMeasurementByIdLocked(measurementId, m)) return 0;
    }

    if (!(m.toolType == 1 || m.toolType == 5 || m.toolType == 6)) return 0;
    if (m.points.size() < 2) return 0;

    MPRContext* mprCtx = GetMPRContextFromSession(sessionId);
    if (!mprCtx || !mprCtx->volume) return 0;

    int width = 0, height = 0, depth = 0;
    if (Dicom_Volume_GetDimensions(mprCtx->volume, &width, &height, &depth) != NATIVE_OK) return 0;

    uint16_t* volumeData = static_cast<uint16_t*>(Dicom_Volume_GetData(mprCtx->volume));
    if (!volumeData) return 0;

    float spacingX = 1.0f, spacingY = 1.0f, spacingZ = 1.0f;
    Dicom_Volume_GetSpacing(mprCtx->volume, &spacingX, &spacingY, &spacingZ);

    std::vector<double> segLenMm;
    segLenMm.reserve(m.points.size() - 1);
    double totalLenMm = 0.0;
    for (size_t i = 1; i < m.points.size(); ++i) {
        const auto& a = m.points[i - 1];
        const auto& b = m.points[i];
        double dx = (double)(b.x - a.x) * spacingX;
        double dy = (double)(b.y - a.y) * spacingY;
        double dz = (double)(b.z - a.z) * spacingZ;
        double len = sqrt(dx * dx + dy * dy + dz * dz);
        segLenMm.push_back(len);
        totalLenMm += len;
    }
    if (totalLenMm <= 1e-6) return 0;

    int n = maxPoints;
    if (n > 1024) n = 1024;
    if (n < 2) n = 2;

    for (int i = 0; i < n; ++i) {
        double t = (double)i / (double)(n - 1);
        double targetDist = t * totalLenMm;

        size_t seg = 0;
        double acc = 0.0;
        while (seg < segLenMm.size()) {
            double next = acc + segLenMm[seg];
            if (targetDist <= next || seg == segLenMm.size() - 1) break;
            acc = next;
            ++seg;
        }

        double segLen = segLenMm[seg];
        double localT = (segLen > 1e-9) ? (targetDist - acc) / segLen : 0.0;
        if (localT < 0.0) localT = 0.0;
        if (localT > 1.0) localT = 1.0;

        const auto& p0 = m.points[seg];
        const auto& p1 = m.points[seg + 1];

        float x = (float)((1.0 - localT) * p0.x + localT * p1.x);
        float y = (float)((1.0 - localT) * p0.y + localT * p1.y);
        float z = (float)((1.0 - localT) * p0.z + localT * p1.z);

        uint16_t v = SampleVolume(volumeData, width, height, depth, x, y, z);
        int hu = (int)v - 1024;

        outAxis[i] = targetDist;
        outValues[i] = (double)hu;
    }

    return n;
}

// ==================== Measurement Tools API ====================
#include "MeasurementTools.h"
#include <map>

// 全锟街癸拷锟竭存储锟斤拷使锟斤拷map锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟节ｏ拷
struct ToolManagerData {
    std::unique_ptr<ToolManager> manager;
    std::map<ToolHandle, std::shared_ptr<MeasurementTool>> tools;
};

// ToolManager 锟斤拷锟斤拷
VIZ_API ToolManagerHandle ToolManager_Create() {
    try {
        auto* data = new ToolManagerData();
        data->manager = std::make_unique<ToolManager>();
        return static_cast<ToolManagerHandle>(data);
    }
    catch (...) {
        return nullptr;
    }
}

VIZ_API void ToolManager_Destroy(ToolManagerHandle manager) {
    if (!manager) return;
    try {
        auto* data = static_cast<ToolManagerData*>(manager);
        delete data;
    }
    catch (...) {}
}

// 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟竭诧拷锟芥储
static ToolHandle CreateAndStoreTool(ToolManagerHandle manager, std::shared_ptr<MeasurementTool> tool) {
    if (!manager || !tool) return nullptr;
    try {
        auto* data = static_cast<ToolManagerData*>(manager);
        ToolHandle handle = static_cast<ToolHandle>(tool.get());
        data->tools[handle] = tool;
        return handle;
    }
    catch (...) {
        return nullptr;
    }
}

// 锟斤拷锟竭达拷锟斤拷锟斤拷锟斤拷
VIZ_API ToolHandle Tool_CreateLine(ToolManagerHandle manager) {
    auto tool = std::make_shared<LineTool>();
    return CreateAndStoreTool(manager, tool);
}

VIZ_API ToolHandle Tool_CreateAngle(ToolManagerHandle manager) {
    auto tool = std::make_shared<AngleTool>();
    return CreateAndStoreTool(manager, tool);
}

VIZ_API ToolHandle Tool_CreateRectangle(ToolManagerHandle manager) {
    auto tool = std::make_shared<RectangleTool>();
    return CreateAndStoreTool(manager, tool);
}

VIZ_API ToolHandle Tool_CreateCircle(ToolManagerHandle manager) {
    auto tool = std::make_shared<CircleTool>();
    return CreateAndStoreTool(manager, tool);
}

VIZ_API ToolHandle Tool_CreateBezier(ToolManagerHandle manager) {
    auto tool = std::make_shared<BezierTool>();
    return CreateAndStoreTool(manager, tool);
}

VIZ_API ToolHandle Tool_CreateFreehand(ToolManagerHandle manager) {
    auto tool = std::make_shared<FreehandTool>();
    return CreateAndStoreTool(manager, tool);
}

// 锟斤拷锟竭诧拷锟斤拷锟斤拷锟斤拷
VIZ_API void Tool_AddPoint(ToolHandle tool, float x, float y) {
    if (!tool) return;
    try {
        auto* measurementTool = static_cast<MeasurementTool*>(tool);
        Point2D screenPos(x, y);
        Point3D worldPos(x, y, 0);  // 锟津化ｏ拷2D锟斤拷锟竭诧拷锟斤拷要锟斤拷锟斤拷锟斤拷3D锟斤拷锟斤拷
        // 使锟斤拷handleMouseEvent模锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷拥锟?
        measurementTool->handleMouseEvent(MOUSE_DOWN, MOUSE_LEFT, screenPos, worldPos);
        measurementTool->handleMouseEvent(MOUSE_UP, MOUSE_LEFT, screenPos, worldPos);
    }
    catch (...) {}
}

VIZ_API void Tool_Finish(ToolHandle tool) {
    if (!tool) return;
    try {
        auto* measurementTool = static_cast<MeasurementTool*>(tool);
        // 锟斤拷锟斤拷锟斤拷锟斤拷使锟矫匡拷锟斤拷锟斤拷锟捷诧拷锟斤拷锟斤拷
        measurementTool->calculateResult(nullptr, 0, 0, 0, 1.0f, 1.0f, 1.0f);
    }
    catch (...) {}
}

VIZ_API float Tool_GetMeasurement(ToolHandle tool) {
    if (!tool) return 0.0f;
    try {
        auto* measurementTool = static_cast<MeasurementTool*>(tool);
        auto result = measurementTool->getResult();
        
        if (!result) return 0.0f;
        
        switch (result->getType()) {
        case ToolResult::RESULT_DISTANCE: {
            auto* distResult = dynamic_cast<DistanceResult*>(result.get());
            return distResult ? distResult->distance : 0.0f;
        }
        case ToolResult::RESULT_ANGLE: {
            auto* angleResult = dynamic_cast<AngleResult*>(result.get());
            return angleResult ? angleResult->angleDegrees : 0.0f;
        }
        case ToolResult::RESULT_STATISTICS: {
            auto* statsResult = dynamic_cast<AreaStatisticsResult*>(result.get());
            return statsResult ? statsResult->area : 0.0f;
        }
        default:
            return 0.0f;
        }
    }
    catch (...) {
        return 0.0f;
    }
}

VIZ_API const char* Tool_GetName(ToolHandle tool) {
    if (!tool) return "Unknown";
    try {
        auto* measurementTool = static_cast<MeasurementTool*>(tool);
        ToolType type = measurementTool->getType();
        
        switch (type) {
        case TOOL_LINE: return "Line Tool";
        case TOOL_ANGLE: return "Angle Tool";
        case TOOL_RECTANGLE: return "Rectangle Tool";
        case TOOL_CIRCLE: return "Circle Tool";
        case TOOL_BEZIER: return "Bezier Tool";
        case TOOL_CLOSED_BEZIER: return "Closed Bezier Tool";
        case TOOL_FREEHAND: return "Freehand Tool";
        case TOOL_WINDOW_LEVEL: return "Window Level Tool";
        case TOOL_SLICE_SWITCH: return "Slice Switch Tool";
        default: return "Unknown Tool";
        }
    }
    catch (...) {
        return "Error";
    }
}

VIZ_API int Tool_GetPointCount(ToolHandle tool) {
    if (!tool) return 0;
    try {
        auto* measurementTool = static_cast<MeasurementTool*>(tool);
        return static_cast<int>(measurementTool->getPoints().size());
    }
    catch (...) {
        return 0;
    }
}

VIZ_API void Tool_GetPoint(ToolHandle tool, int index, float* x, float* y) {
    if (!tool || !x || !y) return;
    try {
        auto* measurementTool = static_cast<MeasurementTool*>(tool);
        const auto& points = measurementTool->getPoints();
        if (index >= 0 && index < static_cast<int>(points.size())) {
            *x = points[index].x;
            *y = points[index].y;
        }
    }
    catch (...) {}
}

// ToolManager 锟斤拷锟斤拷锟斤拷锟斤拷
VIZ_API void ToolManager_SetActiveTool(ToolManagerHandle manager, ToolHandle tool) {
    if (!manager || !tool) return;
    try {
        auto* data = static_cast<ToolManagerData*>(manager);
        auto* measurementTool = static_cast<MeasurementTool*>(tool);
        data->manager->selectTool(measurementTool->getType());
    }
    catch (...) {}
}

VIZ_API int ToolManager_GetToolCount(ToolManagerHandle manager) {
    if (!manager) return 0;
    try {
        auto* data = static_cast<ToolManagerData*>(manager);
        return static_cast<int>(data->tools.size());
    }
    catch (...) {
        return 0;
    }
}

VIZ_API ToolHandle ToolManager_GetTool(ToolManagerHandle manager, int index) {
    if (!manager) return nullptr;
    try {
        auto* data = static_cast<ToolManagerData*>(manager);
        if (index >= 0 && index < static_cast<int>(data->tools.size())) {
            auto it = data->tools.begin();
            std::advance(it, index);
            return it->first;
        }
        return nullptr;
    }
    catch (...) {
        return nullptr;
    }
}

VIZ_API void ToolManager_DeleteTool(ToolManagerHandle manager, ToolHandle tool) {
    if (!manager || !tool) return;
    try {
        auto* data = static_cast<ToolManagerData*>(manager);
        data->tools.erase(tool);
    }
    catch (...) {}
}

VIZ_API void ToolManager_Clear(ToolManagerHandle manager) {
    if (!manager) return;
    try {
        auto* data = static_cast<ToolManagerData*>(manager);
        data->tools.clear();
        data->manager->clearAll();
    }
    catch (...) {}
}

// ==================== Mask锟洁辑锟斤拷锟斤拷API实锟斤拷 ====================

VIZ_API void Mask_SetCurrentMask(void* maskManager, int maskIndex) {
    g_currentMaskManager = maskManager;
    g_currentMaskIndex = maskIndex;
    g_currentSessionMaskEditSessionId.clear();
    g_currentSessionMaskId = -1;
    printf("[Mask] Set current mask: manager=%p, index=%d\n", maskManager, maskIndex);
}

VIZ_API NativeResult MPR_SelectMaskForEditing(const char* sessionId, int maskId) {
    if (!sessionId) {
        SetLastError("Invalid sessionId");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    MPRContext* ctx = GetMPRContextFromSession(sessionId);
    if (!ctx) {
        SetLastError("Invalid session or no MPR context");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    auto it = std::find_if(ctx->masks.begin(), ctx->masks.end(),
        [maskId](const MPRContext::MaskData& m) { return m.id == maskId; });
    if (it == ctx->masks.end()) {
        SetLastError("Mask not found");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    g_currentMaskManager = nullptr;
    g_currentMaskIndex = -1;
    g_currentSessionMaskEditSessionId = sessionId;
    g_currentSessionMaskId = maskId;

    printf("[Mask] Selected session mask for editing: session=%s, maskId=%d, name=%s\n",
           sessionId, maskId, it->name.c_str());
    return NATIVE_OK;
}

VIZ_API void Mask_SetTool(int maskTool) {
    if (maskTool < 1) maskTool = 1;
    if (maskTool > 7) maskTool = 7;
    g_currentMaskTool = maskTool;
    const char* maskToolNames[] = {"", "Brush", "Eraser", "RectROI", "CircleROI", "PolygonROI", "FloodFill", "ConnectedComp"};
    printf("[Mask] Tool set to %d (%s)\n", g_currentMaskTool, maskToolNames[g_currentMaskTool]);
}

VIZ_API int Mask_GetTool() {
    return g_currentMaskTool;
}

VIZ_API void Mask_SetBrushRadius(float radius) {
    g_brushRadius = std::max(0.5f, std::min(50.0f, radius));
    printf("[Mask] Brush radius set to %.1fmm\n", g_brushRadius);
}

VIZ_API float Mask_GetBrushRadius() {
    return g_brushRadius;
}

// ==================== Session-aware Mask Editing APIs ====================

VIZ_API void Mask_SetToolForSession(const char* sessionId, int tool) {
    if (!sessionId) {
        g_currentMaskTool = tool;
        return;
    }
    SetSessionMaskTool(sessionId, tool);
}

VIZ_API int Mask_GetToolForSession(const char* sessionId) {
    if (!sessionId) return g_currentMaskTool;
    return GetSessionMaskTool(sessionId);
}

VIZ_API void Mask_SetBrushRadiusForSession(const char* sessionId, float radius) {
    float clampedRadius = std::max(0.5f, std::min(50.0f, radius));
    if (!sessionId) {
        g_brushRadius = clampedRadius;
        return;
    }
    SetSessionBrushRadius(sessionId, clampedRadius);
}

VIZ_API float Mask_GetBrushRadiusForSession(const char* sessionId) {
    if (!sessionId) return g_brushRadius;
    return GetSessionBrushRadius(sessionId);
}

VIZ_API void Mask_SetCurrentIndexForSession(const char* sessionId, int maskIndex) {
    if (!sessionId) {
        g_currentMaskIndex = maskIndex;
        return;
    }
    SetSessionMaskIndex(sessionId, maskIndex);
}

VIZ_API int Mask_GetCurrentIndexForSession(const char* sessionId) {
    if (!sessionId) return g_currentMaskIndex;
    return GetSessionMaskIndex(sessionId);
}

// ==================== End Session-aware Mask Editing APIs ====================

// ==================== MPR Mask Overlay API ====================

VIZ_API void MPR_AddMaskOverlay(MPRHandle handle, void* maskManager, int maskIndex,
                                float r, float g, float b, float a) {
    if (!handle) return;
    auto ctx = static_cast<MPRContext*>(handle);
    
    MPRContext::MaskOverlay overlay;
    overlay.manager = maskManager;
    overlay.maskIndex = maskIndex;
    overlay.r = r;
    overlay.g = g;
    overlay.b = b;
    overlay.a = a;
    overlay.visible = true;
    
    ctx->maskOverlays.push_back(overlay);
    printf("[MPR] Added mask overlay: index=%d, color=(%.2f,%.2f,%.2f,%.2f)\n", 
           maskIndex, r, g, b, a);
}

VIZ_API void MPR_RemoveMaskOverlay(MPRHandle handle, int overlayIndex) {
    if (!handle) return;
    auto ctx = static_cast<MPRContext*>(handle);
    
    if (overlayIndex >= 0 && overlayIndex < (int)ctx->maskOverlays.size()) {
        ctx->maskOverlays.erase(ctx->maskOverlays.begin() + overlayIndex);
        printf("[MPR] Removed mask overlay at index %d\n", overlayIndex);
    }
}

VIZ_API void MPR_SetMaskOverlayColor(MPRHandle handle, int overlayIndex,
                                     float r, float g, float b, float a) {
    if (!handle) return;
    auto ctx = static_cast<MPRContext*>(handle);
    
    if (overlayIndex >= 0 && overlayIndex < (int)ctx->maskOverlays.size()) {
        auto& overlay = ctx->maskOverlays[overlayIndex];
        overlay.r = r;
        overlay.g = g;
        overlay.b = b;
        overlay.a = a;
        printf("[MPR] Set overlay %d color to (%.2f,%.2f,%.2f,%.2f)\n", 
               overlayIndex, r, g, b, a);
    }
}

VIZ_API void MPR_SetMaskOverlayVisible(MPRHandle handle, int overlayIndex, bool visible) {
    if (!handle) return;
    auto ctx = static_cast<MPRContext*>(handle);
    
    if (overlayIndex >= 0 && overlayIndex < (int)ctx->maskOverlays.size()) {
        ctx->maskOverlays[overlayIndex].visible = visible;
        printf("[MPR] Set overlay %d visible=%d\n", overlayIndex, visible);
    }
}

VIZ_API void MPR_ClearMaskOverlays(MPRHandle handle) {
    if (!handle) return;
    auto ctx = static_cast<MPRContext*>(handle);
    
    ctx->maskOverlays.clear();
    printf("[MPR] Cleared all mask overlays\n");
}

VIZ_API int MPR_GetMaskOverlayCount(MPRHandle handle) {
    if (!handle) return 0;
    auto ctx = static_cast<MPRContext*>(handle);
    return (int)ctx->maskOverlays.size();
}

// ==================== Mask锟洁辑锟酵癸拷锟斤拷 API 实锟斤拷 ====================

// 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷Base64锟斤拷锟斤拷
static std::string Base64Encode(const uint8_t* data, size_t len) {
    static const char* base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    
    std::string ret;
    int i = 0;
    uint8_t char_array_3[3];
    uint8_t char_array_4[4];
    
    while (len--) {
        char_array_3[i++] = *(data++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            
            for (i = 0; i < 4; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }
    
    if (i) {
        for (int j = i; j < 3; j++)
            char_array_3[j] = '\0';
        
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        
        for (int j = 0; j < i + 1; j++)
            ret += base64_chars[char_array_4[j]];
        
        while (i++ < 3)
            ret += '=';
    }
    
    return ret;
}

// 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷Base64锟斤拷锟斤拷
static std::vector<uint8_t> Base64Decode(const std::string& encoded_string) {
    static const std::string base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    
    int in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    uint8_t char_array_4[4], char_array_3[3];
    std::vector<uint8_t> ret;
    
    while (in_len-- && (encoded_string[in_] != '=') && 
           (isalnum(encoded_string[in_]) || (encoded_string[in_] == '+') || (encoded_string[in_] == '/'))) {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);
            
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            
            for (i = 0; i < 3; i++)
                ret.push_back(char_array_3[i]);
            i = 0;
        }
    }
    
    if (i) {
        for (j = 0; j < i; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]);
        
        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        
        for (j = 0; j < i - 1; j++)
            ret.push_back(char_array_3[j]);
    }
    
    return ret;
}

// 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷十锟斤拷锟斤拷锟斤拷锟斤拷色
static void ParseHexColor(const char* hexColor, float& r, float& g, float& b) {
    if (!hexColor || hexColor[0] != '#') {
        r = g = b = 1.0f;
        return;
    }
    
    unsigned int rgb = 0;
    sscanf_s(hexColor + 1, "%x", &rgb);
    r = ((rgb >> 16) & 0xFF) / 255.0f;
    g = ((rgb >> 8) & 0xFF) / 255.0f;
    b = (rgb & 0xFF) / 255.0f;
}

// 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷取Session锟斤拷应锟斤拷MPRContext
static MPRContext* GetMPRContextFromSession(const char* sessionId) {
    if (!sessionId) return nullptr;
    
    std::lock_guard<std::mutex> lock(g_SessionMutex);
    auto it = g_Sessions.find(sessionId);
    if (it == g_Sessions.end() || !it->second.mprHandle) {
        return nullptr;
    }
    
    return static_cast<MPRContext*>(it->second.mprHandle);
}

// 注锟斤拷Session锟斤拷Volume锟斤拷锟斤拷锟斤拷APR锟斤拷图锟斤拷
VIZ_API NativeResult MPR_RegisterSessionVolume(const char* sessionId, VolumeHandle volume) {
    if (!sessionId || !volume) {
        SetLastError("Invalid session ID or volume handle");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    
    // 锟斤拷锟斤拷 MPRContext
    MPRContext* ctx = new MPRContext();
    ctx->volume = volume;
    ctx->histogramCalculated = false;
    ctx->sessionId = sessionId;
    
    // 锟斤拷锟芥到全锟斤拷Session映锟斤拷
    std::lock_guard<std::mutex> lock(g_SessionMutex);
    SessionContext& session = g_Sessions[sessionId];
    session.sessionId = sessionId;
    session.mprHandle = ctx;
    session.volumeHandle = volume;
    
    printf("[MPR_RegisterSessionVolume] Registered session: %s, volume: %p\n", sessionId, volume);
    return NATIVE_OK;
}

// 1. 锟斤拷取锟斤拷锟斤拷锟斤拷直锟斤拷图
VIZ_API NativeResult MPR_GetVolumeHistogram(
    const char* sessionId,
    int* outData,
    int* outMinValue,
    int* outMaxValue
) {
    MPRContext* ctx = GetMPRContextFromSession(sessionId);
    if (!ctx || !ctx->volume) {
        SetLastError("Invalid session or no volume loaded");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    
    // 锟斤拷锟斤拷丫锟斤拷锟斤拷锟斤拷锟斤拷直锟接凤拷锟截伙拷锟斤拷慕锟斤拷
    if (ctx->histogramCalculated) {
        if (outData) memcpy(outData, ctx->histogram, sizeof(ctx->histogram));
        if (outMinValue) *outMinValue = ctx->histogramMinValue;
        if (outMaxValue) *outMaxValue = ctx->histogramMaxValue;
        return NATIVE_OK;
    }
    
    // 锟斤拷取锟斤拷锟斤拷锟斤拷
    int width, height, depth;
    if (Dicom_Volume_GetDimensions(ctx->volume, &width, &height, &depth) != NATIVE_OK) {
        SetLastError("Failed to get volume dimensions");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    
    uint16_t* volumeData = static_cast<uint16_t*>(Dicom_Volume_GetData(ctx->volume));
    if (!volumeData) {
        SetLastError("Failed to get volume data");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    
    // 锟斤拷锟斤拷直锟斤拷图锟斤拷256锟斤拷bin锟斤拷映锟戒到HU锟斤拷围锟斤拷
    memset(ctx->histogram, 0, sizeof(ctx->histogram));
    
    int minHU = 32767, maxHU = -32768;
    size_t totalVoxels = static_cast<size_t>(width) * height * depth;
    
    // 锟斤拷一锟轿憋拷锟斤拷锟斤拷锟揭碉拷min/max
    for (size_t i = 0; i < totalVoxels; i++) {
        int hu = static_cast<int>(volumeData[i]) - 1024;  // 转锟斤拷为HU值
        if (hu < minHU) minHU = hu;
        if (hu > maxHU) maxHU = hu;
    }
    
    ctx->histogramMinValue = minHU;
    ctx->histogramMaxValue = maxHU;
    
    // 锟节讹拷锟轿憋拷锟斤拷锟斤拷统锟斤拷直锟斤拷图
    float binWidth = (maxHU - minHU) / 256.0f;
    if (binWidth < 1.0f) binWidth = 1.0f;
    
    for (size_t i = 0; i < totalVoxels; i++) {
        int hu = static_cast<int>(volumeData[i]) - 1024;
        int bin = static_cast<int>((hu - minHU) / binWidth);
        if (bin < 0) bin = 0;
        if (bin > 255) bin = 255;
        ctx->histogram[bin]++;
    }
    
    ctx->histogramCalculated = true;
    
    // 锟斤拷锟斤拷锟斤拷
    if (outData) memcpy(outData, ctx->histogram, sizeof(ctx->histogram));
    if (outMinValue) *outMinValue = ctx->histogramMinValue;
    if (outMaxValue) *outMaxValue = ctx->histogramMaxValue;
    
    printf("[MPR] Histogram calculated: min=%d, max=%d\n", minHU, maxHU);
    return NATIVE_OK;
}

// 1.1 鑾峰彇鎸囧畾 mask 鐨勭粺璁′俊鎭紙HU min/max/hist/mean/std/volume锛?
VIZ_API NativeResult MPR_GetMaskStatistics(
    const char* sessionId,
    int maskId,
    int* outHistogram,
    int* outMinValue,
    int* outMaxValue,
    double* outMean,
    double* outStdDev,
    unsigned long long* outCount,
    double* outVolumeMm3
) {
    MPRContext* ctx = GetMPRContextFromSession(sessionId);
    if (!ctx || !ctx->volume) {
        SetLastError("Invalid session or no volume loaded");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    // Find mask by id
    MPRContext::MaskData* mask = nullptr;
    for (auto& m : ctx->masks) {
        if (m.id == maskId) {
            mask = &m;
            break;
        }
    }
    if (!mask) {
        SetLastError("Mask not found");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    int width = 0, height = 0, depth = 0;
    if (Dicom_Volume_GetDimensions(ctx->volume, &width, &height, &depth) != NATIVE_OK) {
        SetLastError("Failed to get volume dimensions");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    uint16_t* volumeData = static_cast<uint16_t*>(Dicom_Volume_GetData(ctx->volume));
    if (!volumeData) {
        SetLastError("Failed to get volume data");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    const size_t totalVoxels = static_cast<size_t>(width) * height * depth;
    if (mask->data.size() != totalVoxels) {
        SetLastError("Mask size mismatch with volume");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    // First pass: min/max + sum/sumsq + count
    unsigned long long count = 0;
    int minHU = 0;
    int maxHU = 0;
    double sum = 0.0;
    double sumsq = 0.0;
    bool hasAny = false;

    for (size_t i = 0; i < totalVoxels; ++i) {
        if (mask->data[i] == 0) continue;
        const int hu = static_cast<int>(volumeData[i]) - 1024;
        if (!hasAny) {
            minHU = maxHU = hu;
            hasAny = true;
        } else {
            if (hu < minHU) minHU = hu;
            if (hu > maxHU) maxHU = hu;
        }
        sum += static_cast<double>(hu);
        sumsq += static_cast<double>(hu) * static_cast<double>(hu);
        ++count;
    }

    if (outCount) *outCount = count;

    // Empty mask: return zeros
    if (!hasAny || count == 0) {
        if (outMinValue) *outMinValue = 0;
        if (outMaxValue) *outMaxValue = 0;
        if (outMean) *outMean = 0.0;
        if (outStdDev) *outStdDev = 0.0;
        if (outHistogram) memset(outHistogram, 0, sizeof(int) * 256);
        if (outVolumeMm3) *outVolumeMm3 = 0.0;
        return NATIVE_OK;
    }

    const double mean = sum / static_cast<double>(count);
    double var = (sumsq / static_cast<double>(count)) - (mean * mean);
    if (var < 0.0) var = 0.0;
    const double stddev = sqrt(var);

    if (outMinValue) *outMinValue = minHU;
    if (outMaxValue) *outMaxValue = maxHU;
    if (outMean) *outMean = mean;
    if (outStdDev) *outStdDev = stddev;

    // Volume (mm^3)
    if (outVolumeMm3) {
        float sx = 1.0f, sy = 1.0f, sz = 1.0f;
        (void)Dicom_Volume_GetSpacing(ctx->volume, &sx, &sy, &sz);
        *outVolumeMm3 = static_cast<double>(count) * static_cast<double>(sx) * static_cast<double>(sy) * static_cast<double>(sz);
    }

    // Histogram (256 bins mapped to [minHU, maxHU])
    if (outHistogram) {
        memset(outHistogram, 0, sizeof(int) * 256);
        float binWidth = (maxHU - minHU) / 256.0f;
        if (binWidth < 1.0f) binWidth = 1.0f;

        for (size_t i = 0; i < totalVoxels; ++i) {
            if (mask->data[i] == 0) continue;
            const int hu = static_cast<int>(volumeData[i]) - 1024;
            int bin = static_cast<int>((hu - minHU) / binWidth);
            if (bin < 0) bin = 0;
            if (bin > 255) bin = 255;
            outHistogram[bin]++;
        }
    }

    return NATIVE_OK;
}

// 2. 锟斤拷锟斤拷预锟斤拷mask
VIZ_API NativeResult MPR_UpdatePreviewMask(
    const char* sessionId,
    float minThreshold,
    float maxThreshold,
    const char* hexColor
) {
    MPRContext* ctx = GetMPRContextFromSession(sessionId);
    if (!ctx || !ctx->volume) {
        SetLastError("Invalid session or no volume loaded");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    
    // 锟斤拷取锟斤拷锟斤拷锟捷尺达拷
    int width, height, depth;
    if (Dicom_Volume_GetDimensions(ctx->volume, &width, &height, &depth) != NATIVE_OK) {
        SetLastError("Failed to get volume dimensions");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    
    uint16_t* volumeData = static_cast<uint16_t*>(Dicom_Volume_GetData(ctx->volume));
    if (!volumeData) {
        SetLastError("Failed to get volume data");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    
    // 锟斤拷锟斤拷锟斤拷锟斤拷锟皆わ拷锟絤ask
    if (!ctx->previewMask) {
        ctx->previewMask = new MPRContext::MaskData();
    }
    
    ctx->previewMask->minThreshold = minThreshold;
    ctx->previewMask->maxThreshold = maxThreshold;
    ctx->previewMask->color = hexColor ? hexColor : "#ffffff";
    ctx->previewMask->visible = true;
    
    // 锟斤拷锟斤拷mask锟斤拷锟斤拷
    size_t totalVoxels = static_cast<size_t>(width) * height * depth;
    ctx->previewMask->data.resize(totalVoxels);
    
    // 锟斤拷锟斤拷锟斤拷值锟斤拷锟斤拷mask
    for (size_t i = 0; i < totalVoxels; i++) {
        int hu = static_cast<int>(volumeData[i]) - 1024;
        ctx->previewMask->data[i] = (hu >= minThreshold && hu <= maxThreshold) ? 255 : 0;
    }
    
    printf("[MPR] Preview mask updated: threshold=[%.1f, %.1f], color=%s\n",
           minThreshold, maxThreshold, hexColor ? hexColor : "null");

    ctx->maskRevision++;
    
    return NATIVE_OK;
}

// 3. 锟斤拷锟皆わ拷锟絤ask
VIZ_API NativeResult MPR_ClearPreviewMask(const char* sessionId) {
    MPRContext* ctx = GetMPRContextFromSession(sessionId);
    if (!ctx) {
        SetLastError("Invalid session");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    
    if (ctx->previewMask) {
        delete ctx->previewMask;
        ctx->previewMask = nullptr;
        printf("[MPR] Preview mask cleared\n");
        ctx->maskRevision++;
    }
    
    return NATIVE_OK;
}

// 4. 锟斤拷锟斤拷permanent mask
VIZ_API NativeResult MPR_CreateMaskFromThreshold(
    const char* sessionId,
    float minThreshold,
    float maxThreshold,
    const char* hexColor,
    const char* maskName,
    int* outMaskId
) {
    MPRContext* ctx = GetMPRContextFromSession(sessionId);
    if (!ctx || !ctx->volume) {
        SetLastError("Invalid session or no volume loaded");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    
    // 锟斤拷取锟斤拷锟斤拷锟捷尺达拷
    int width, height, depth;
    if (Dicom_Volume_GetDimensions(ctx->volume, &width, &height, &depth) != NATIVE_OK) {
        SetLastError("Failed to get volume dimensions");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    
    uint16_t* volumeData = static_cast<uint16_t*>(Dicom_Volume_GetData(ctx->volume));
    if (!volumeData) {
        SetLastError("Failed to get volume data");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    
    // 锟斤拷锟斤拷锟斤拷mask
    MPRContext::MaskData mask;
    mask.id = ctx->masks.empty() ? 0 : (ctx->masks.back().id + 1);
    mask.name = maskName ? maskName : "Mask";
    mask.color = hexColor ? hexColor : "#ffffff";
    mask.visible = true;
    mask.minThreshold = minThreshold;
    mask.maxThreshold = maxThreshold;
    
    // 锟斤拷锟戒并锟斤拷锟斤拷mask锟斤拷锟斤拷
    size_t totalVoxels = static_cast<size_t>(width) * height * depth;
    mask.data.resize(totalVoxels);
    
    for (size_t i = 0; i < totalVoxels; i++) {
        int hu = static_cast<int>(volumeData[i]) - 1024;
        mask.data[i] = (hu >= minThreshold && hu <= maxThreshold) ? 255 : 0;
    }
    
    // 锟斤拷锟接碉拷锟叫憋拷
    ctx->masks.push_back(mask);

    ctx->maskRevision++;
    
    if (outMaskId) {
        *outMaskId = mask.id;
    }
    
    printf("[MPR] Mask created: id=%d, name=%s, threshold=[%.1f, %.1f]\n",
           mask.id, mask.name.c_str(), minThreshold, maxThreshold);
    
    return NATIVE_OK;
}

VIZ_API NativeResult MPR_CreateEmptyMask(
    const char* sessionId,
    const char* hexColor,
    const char* maskName,
    int* outMaskId
) {
    MPRContext* ctx = GetMPRContextFromSession(sessionId);
    if (!ctx || !ctx->volume) {
        SetLastError("Invalid session or no volume loaded");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    int width = 0, height = 0, depth = 0;
    if (Dicom_Volume_GetDimensions(ctx->volume, &width, &height, &depth) != NATIVE_OK ||
        width <= 0 || height <= 0 || depth <= 0) {
        SetLastError("Failed to get volume dimensions");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    MPRContext::MaskData mask;
    mask.id = ctx->masks.empty() ? 0 : (ctx->masks.back().id + 1);
    mask.name = maskName ? maskName : "Mask";
    mask.color = hexColor ? hexColor : "#ffffff";
    mask.visible = true;
    mask.minThreshold = -1000.0f;
    mask.maxThreshold = 3000.0f;

    const size_t totalVoxels = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(depth);
    mask.data.assign(totalVoxels, 0);

    ctx->masks.push_back(mask);
    ctx->maskRevision++;
    if (outMaskId) {
        *outMaskId = mask.id;
    }

    printf("[MPR] Empty mask created: id=%d, name=%s\n", mask.id, mask.name.c_str());
    return NATIVE_OK;
}

// 5. 删锟斤拷mask
VIZ_API NativeResult MPR_DeleteMask(const char* sessionId, int maskId) {
    MPRContext* ctx = GetMPRContextFromSession(sessionId);
    if (!ctx) {
        SetLastError("Invalid session");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    
    // 锟斤拷锟揭诧拷删锟斤拷mask
    auto it = std::find_if(ctx->masks.begin(), ctx->masks.end(),
                          [maskId](const MPRContext::MaskData& m) { return m.id == maskId; });
    
    if (it == ctx->masks.end()) {
        SetLastError("Mask not found");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    
    ctx->masks.erase(it);
    ctx->maskRevision++;
    printf("[MPR] Mask deleted: id=%d\n", maskId);
    
    return NATIVE_OK;
}

// ==================== Bone Metrics (Mask-based) ====================

VIZ_API NativeResult MPR_GetVolumeSpacing(
    const char* sessionId,
    float* outSpacingX,
    float* outSpacingY,
    float* outSpacingZ
) {
    if (!sessionId || !outSpacingX || !outSpacingY || !outSpacingZ) {
        SetLastError("Invalid arguments");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    MPRContext* ctx = GetMPRContextFromSession(sessionId);
    if (!ctx || !ctx->volume) {
        SetLastError("Invalid session or no volume loaded");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    float sx = 1.0f, sy = 1.0f, sz = 1.0f;
    if (Dicom_Volume_GetSpacing(ctx->volume, &sx, &sy, &sz) != NATIVE_OK) {
        SetLastError("Failed to get volume spacing");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    *outSpacingX = sx;
    *outSpacingY = sy;
    *outSpacingZ = sz;
    return NATIVE_OK;
}

namespace {

constexpr double kPi = 3.14159265358979323846;

struct Vec3d {
    double x = 0;
    double y = 0;
    double z = 0;
};

static inline uint8_t MaskAt(const uint8_t* mask, int w, int h, int d, int x, int y, int z) {
    if (x < 0 || y < 0 || z < 0 || x >= w || y >= h || z >= d) return 0;
    const size_t idx = static_cast<size_t>(z) * static_cast<size_t>(w) * static_cast<size_t>(h)
        + static_cast<size_t>(y) * static_cast<size_t>(w)
        + static_cast<size_t>(x);
    return mask[idx];
}

static bool FindMaskById(const MPRContext* ctx, int maskId, const MPRContext::MaskData** outMask) {
    if (!ctx || !outMask) return false;
    for (const auto& m : ctx->masks) {
        if (m.id == maskId) {
            *outMask = &m;
            return true;
        }
    }
    return false;
}

struct StlTri {
    float nx, ny, nz;
    float ax, ay, az;
    float bx, by, bz;
    float cx, cy, cz;
};

static inline void StlWriteU32(std::ofstream& os, uint32_t v) {
    os.write(reinterpret_cast<const char*>(&v), sizeof(v));
}

static inline void StlWriteU16(std::ofstream& os, uint16_t v) {
    os.write(reinterpret_cast<const char*>(&v), sizeof(v));
}

static inline void StlWriteF32(std::ofstream& os, float v) {
    os.write(reinterpret_cast<const char*>(&v), sizeof(v));
}

static inline void StlWriteTri(std::ofstream& os, const StlTri& t) {
    StlWriteF32(os, t.nx); StlWriteF32(os, t.ny); StlWriteF32(os, t.nz);
    StlWriteF32(os, t.ax); StlWriteF32(os, t.ay); StlWriteF32(os, t.az);
    StlWriteF32(os, t.bx); StlWriteF32(os, t.by); StlWriteF32(os, t.bz);
    StlWriteF32(os, t.cx); StlWriteF32(os, t.cy); StlWriteF32(os, t.cz);
    StlWriteU16(os, 0);
}

// Greedy voxel surface meshing on a downsampled occupancy grid.
// This targets binary masks and dramatically reduces triangle count vs naive per-voxel faces.
static bool ExportBinaryMaskToBinaryStl(
    const uint8_t* mask,
    int w,
    int h,
    int d,
    float sx,
    float sy,
    float sz,
    const char* filepath,
    int step
) {
    if (!mask || !filepath) return false;
    if (w <= 0 || h <= 0 || d <= 0) return false;
    step = std::max(1, std::min(step, 16));

    const int rw = (w + step - 1) / step;
    const int rh = (h + step - 1) / step;
    const int rd = (d + step - 1) / step;
    const size_t rCount = static_cast<size_t>(rw) * static_cast<size_t>(rh) * static_cast<size_t>(rd);
    if (rCount == 0) return false;

    std::vector<uint8_t> occ(rCount, 0);
    // Downsample by OR: any voxel in a block marks the reduced cell occupied.
    for (int z = 0; z < d; ++z) {
        const int rz = z / step;
        const size_t zBase = static_cast<size_t>(z) * static_cast<size_t>(w) * static_cast<size_t>(h);
        const size_t rzBase = static_cast<size_t>(rz) * static_cast<size_t>(rw) * static_cast<size_t>(rh);
        for (int y = 0; y < h; ++y) {
            const int ry = y / step;
            const size_t yBase = zBase + static_cast<size_t>(y) * static_cast<size_t>(w);
            const size_t ryBase = rzBase + static_cast<size_t>(ry) * static_cast<size_t>(rw);
            for (int x = 0; x < w; ++x) {
                const uint8_t v = mask[yBase + static_cast<size_t>(x)];
                if (!v) continue;
                const int rx = x / step;
                occ[ryBase + static_cast<size_t>(rx)] = 1;
            }
        }
    }

    auto occAt = [&](int x, int y, int z) -> uint8_t {
        if (x < 0 || y < 0 || z < 0 || x >= rw || y >= rh || z >= rd) return 0;
        const size_t idx = static_cast<size_t>(z) * static_cast<size_t>(rw) * static_cast<size_t>(rh)
            + static_cast<size_t>(y) * static_cast<size_t>(rw)
            + static_cast<size_t>(x);
        return occ[idx];
    };

    auto emitQuad = [&](
        int x, int y, int z,
        int dux, int duy, int duz,
        int dvx, int dvy, int dvz,
        int nx, int ny, int nz,
        bool write,
        uint32_t& triCount,
        std::ofstream* os
    ) {
        // Convert reduced-grid coordinates -> voxel coordinates -> mm.
        const float scaleX = sx * static_cast<float>(step);
        const float scaleY = sy * static_cast<float>(step);
        const float scaleZ = sz * static_cast<float>(step);

        auto toWorld = [&](int gx, int gy, int gz, float& ox, float& oy, float& oz) {
            ox = static_cast<float>(gx) * scaleX;
            oy = static_cast<float>(gy) * scaleY;
            oz = static_cast<float>(gz) * scaleZ;
        };

        float ax, ay, az;
        float bx, by, bz;
        float cx, cy, cz;
        float dx, dy, dz;
        toWorld(x, y, z, ax, ay, az);
        toWorld(x + dux, y + duy, z + duz, bx, by, bz);
        toWorld(x + dvx, y + dvy, z + dvz, dx, dy, dz);
        toWorld(x + dux + dvx, y + duy + dvy, z + duz + dvz, cx, cy, cz);

        // Two triangles per quad. Winding depends on normal direction.
        const float fnx = static_cast<float>(nx);
        const float fny = static_cast<float>(ny);
        const float fnz = static_cast<float>(nz);

        if (!write) {
            triCount += 2;
            return;
        }

        if (!os) return;
        if (nx + ny + nz > 0) {
            // (a, b, c) and (a, c, d)
            StlTri t1{ fnx, fny, fnz, ax, ay, az, bx, by, bz, cx, cy, cz };
            StlTri t2{ fnx, fny, fnz, ax, ay, az, cx, cy, cz, dx, dy, dz };
            StlWriteTri(*os, t1);
            StlWriteTri(*os, t2);
        } else {
            // flip winding
            StlTri t1{ fnx, fny, fnz, ax, ay, az, cx, cy, cz, bx, by, bz };
            StlTri t2{ fnx, fny, fnz, ax, ay, az, dx, dy, dz, cx, cy, cz };
            StlWriteTri(*os, t1);
            StlWriteTri(*os, t2);
        }
    };

    auto greedyPass = [&](bool write, uint32_t& triCount, std::ofstream* os) {
        const int dims[3] = { rw, rh, rd };
        std::vector<int8_t> mask2d;

        for (int axis = 0; axis < 3; ++axis) {
            const int u = (axis + 1) % 3;
            const int v = (axis + 2) % 3;
            const int du[3] = { 0,0,0 };
            const int dv[3] = { 0,0,0 };

            const int uDim = dims[u];
            const int vDim = dims[v];
            mask2d.assign(static_cast<size_t>(uDim) * static_cast<size_t>(vDim), 0);

            int x[3] = { 0,0,0 };
            int q[3] = { 0,0,0 };
            q[axis] = 1;

            for (x[axis] = -1; x[axis] < dims[axis]; ++x[axis]) {
                // Build 2D face mask for this slice.
                size_t n = 0;
                for (x[v] = 0; x[v] < dims[v]; ++x[v]) {
                    for (x[u] = 0; x[u] < dims[u]; ++x[u]) {
                        const uint8_t a = (x[axis] >= 0) ? occAt(x[0], x[1], x[2]) : 0;
                        const uint8_t b = (x[axis] < dims[axis] - 1) ? occAt(x[0] + q[0], x[1] + q[1], x[2] + q[2]) : 0;
                        if (a == b) {
                            mask2d[n++] = 0;
                        } else {
                            mask2d[n++] = a ? 1 : -1; // 1: face points +axis, -1: face points -axis
                        }
                    }
                }

                // Greedy merge rectangles.
                n = 0;
                for (int j = 0; j < vDim; ++j) {
                    for (int i = 0; i < uDim; ) {
                        const int8_t c = mask2d[n];
                        if (c == 0) {
                            ++i;
                            ++n;
                            continue;
                        }

                        // Compute width.
                        int wRun = 1;
                        while (i + wRun < uDim && mask2d[n + static_cast<size_t>(wRun)] == c) {
                            ++wRun;
                        }

                        // Compute height.
                        int hRun = 1;
                        bool done = false;
                        while (j + hRun < vDim && !done) {
                            for (int k = 0; k < wRun; ++k) {
                                if (mask2d[n + static_cast<size_t>(k) + static_cast<size_t>(hRun) * static_cast<size_t>(uDim)] != c) {
                                    done = true;
                                    break;
                                }
                            }
                            if (!done) ++hRun;
                        }

                        // Emit quad.
                        int duVec[3] = { 0,0,0 };
                        int dvVec[3] = { 0,0,0 };
                        duVec[u] = wRun;
                        dvVec[v] = hRun;

                        int base[3] = { 0,0,0 };
                        base[0] = x[0];
                        base[1] = x[1];
                        base[2] = x[2];
                        base[u] = i;
                        base[v] = j;

                        // For faces on boundary between x[axis] and x[axis]+1, the plane is at x[axis]+1.
                        // For c==1 (a filled), face normal points +axis, plane at x[axis]+1.
                        // For c==-1 (b filled), face normal points -axis, plane at x[axis]+1.
                        base[axis] = x[axis] + 1;

                        int nrm[3] = { 0,0,0 };
                        nrm[axis] = (c > 0) ? 1 : -1;

                        emitQuad(
                            base[0], base[1], base[2],
                            duVec[0], duVec[1], duVec[2],
                            dvVec[0], dvVec[1], dvVec[2],
                            nrm[0], nrm[1], nrm[2],
                            write,
                            triCount,
                            os
                        );

                        // Clear mask area.
                        for (int yy = 0; yy < hRun; ++yy) {
                            for (int xx = 0; xx < wRun; ++xx) {
                                mask2d[n + static_cast<size_t>(xx) + static_cast<size_t>(yy) * static_cast<size_t>(uDim)] = 0;
                            }
                        }

                        i += wRun;
                        n += static_cast<size_t>(wRun);
                    }
                }
            }
        }
    };

    uint32_t triCount = 0;
    greedyPass(false, triCount, nullptr);

    std::ofstream os(filepath, std::ios::binary);
    if (!os.is_open()) return false;

    // 80-byte header
    char header[80];
    std::memset(header, 0, sizeof(header));
    const char* title = "HIScan Mask STL";
    strncpy_s(header, sizeof(header), title, _TRUNCATE);
    os.write(header, sizeof(header));
    StlWriteU32(os, triCount);

    uint32_t triCountWritten = 0;
    greedyPass(true, triCountWritten, &os);

    // triCountWritten is a triangle count (we incremented by 2 per quad).
    // If something diverged, file is still valid but count header may be off.
    // We keep it simple and trust deterministic generation.

    return true;
}
static void ComputeSurfaceAreaAndCentroidAndBbox(
    const uint8_t* mask,
    int w,
    int h,
    int d,
    float sx,
    float sy,
    float sz,
    int& outVoxelCount,
    double& outAreaMm2,
    Vec3d& outCentroidVoxel,
    int& minX,
    int& minY,
    int& minZ,
    int& maxX,
    int& maxY,
    int& maxZ
) {
    outVoxelCount = 0;
    outAreaMm2 = 0.0;
    outCentroidVoxel = {};
    minX = w; minY = h; minZ = d;
    maxX = -1; maxY = -1; maxZ = -1;

    const double faceAreaX = static_cast<double>(sy) * static_cast<double>(sz);
    const double faceAreaY = static_cast<double>(sx) * static_cast<double>(sz);
    const double faceAreaZ = static_cast<double>(sx) * static_cast<double>(sy);

    const int threadCount = std::max(1u, std::thread::hardware_concurrency());
    const int actualThreads = std::max(1, std::min(threadCount, d));

    struct Acc {
        uint64_t voxelCount = 0;
        double areaMm2 = 0.0;
        long double sumX = 0;
        long double sumY = 0;
        long double sumZ = 0;
        int minX = 0;
        int minY = 0;
        int minZ = 0;
        int maxX = -1;
        int maxY = -1;
        int maxZ = -1;
        bool hasAny = false;
    };

    std::vector<Acc> accs(static_cast<size_t>(actualThreads));
    std::vector<std::thread> threads;
    threads.reserve(static_cast<size_t>(actualThreads));

    auto worker = [&](int zStart, int zEnd, Acc& acc) {
        acc.minX = w; acc.minY = h; acc.minZ = d;
        acc.maxX = -1; acc.maxY = -1; acc.maxZ = -1;
        for (int z = zStart; z < zEnd; ++z) {
            const size_t zBase = static_cast<size_t>(z) * static_cast<size_t>(w) * static_cast<size_t>(h);
            for (int y = 0; y < h; ++y) {
                const size_t yBase = zBase + static_cast<size_t>(y) * static_cast<size_t>(w);
                for (int x = 0; x < w; ++x) {
                    const size_t idx = yBase + static_cast<size_t>(x);
                    const uint8_t v = mask[idx];
                    if (!v) continue;

                    acc.hasAny = true;
                    acc.voxelCount++;
                    acc.sumX += static_cast<long double>(x);
                    acc.sumY += static_cast<long double>(y);
                    acc.sumZ += static_cast<long double>(z);
                    if (x < acc.minX) acc.minX = x;
                    if (y < acc.minY) acc.minY = y;
                    if (z < acc.minZ) acc.minZ = z;
                    if (x > acc.maxX) acc.maxX = x;
                    if (y > acc.maxY) acc.maxY = y;
                    if (z > acc.maxZ) acc.maxZ = z;

                    if (!MaskAt(mask, w, h, d, x - 1, y, z)) acc.areaMm2 += faceAreaX;
                    if (!MaskAt(mask, w, h, d, x + 1, y, z)) acc.areaMm2 += faceAreaX;
                    if (!MaskAt(mask, w, h, d, x, y - 1, z)) acc.areaMm2 += faceAreaY;
                    if (!MaskAt(mask, w, h, d, x, y + 1, z)) acc.areaMm2 += faceAreaY;
                    if (!MaskAt(mask, w, h, d, x, y, z - 1)) acc.areaMm2 += faceAreaZ;
                    if (!MaskAt(mask, w, h, d, x, y, z + 1)) acc.areaMm2 += faceAreaZ;
                }
            }
        }
    };

    for (int t = 0; t < actualThreads; ++t) {
        const int zStart = (t * d) / actualThreads;
        const int zEnd = ((t + 1) * d) / actualThreads;
        threads.emplace_back(worker, zStart, zEnd, std::ref(accs[static_cast<size_t>(t)]));
    }
    for (auto& th : threads) th.join();

    long double sumX = 0, sumY = 0, sumZ = 0;
    uint64_t voxelCount = 0;
    double areaMm2 = 0.0;
    bool hasAny = false;
    for (const auto& a : accs) {
        voxelCount += a.voxelCount;
        areaMm2 += a.areaMm2;
        sumX += a.sumX;
        sumY += a.sumY;
        sumZ += a.sumZ;
        if (a.hasAny) {
            hasAny = true;
            if (a.minX < minX) minX = a.minX;
            if (a.minY < minY) minY = a.minY;
            if (a.minZ < minZ) minZ = a.minZ;
            if (a.maxX > maxX) maxX = a.maxX;
            if (a.maxY > maxY) maxY = a.maxY;
            if (a.maxZ > maxZ) maxZ = a.maxZ;
        }
    }

    outVoxelCount = static_cast<int>(voxelCount);
    outAreaMm2 = areaMm2;
    if (!hasAny || voxelCount == 0) {
        outCentroidVoxel = { 0, 0, 0 };
        minX = minY = minZ = 0;
        maxX = maxY = maxZ = -1;
        return;
    }

    outCentroidVoxel.x = static_cast<double>(sumX / voxelCount);
    outCentroidVoxel.y = static_cast<double>(sumY / voxelCount);
    outCentroidVoxel.z = static_cast<double>(sumZ / voxelCount);
}

// Export permanent mask as STL mesh with adjustable precision.
extern "C" VIZ_API NativeResult MPR_ExportMaskToSTL(
    const char* sessionId,
    int maskId,
    const char* filepath,
    int step
) {
    if (!sessionId || !filepath) {
        SetLastError("Invalid arguments");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    MPRContext* ctx = GetMPRContextFromSession(sessionId);
    if (!ctx || !ctx->volume) {
        SetLastError("Invalid session or no volume loaded");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    const MPRContext::MaskData* maskObj = nullptr;
    if (!FindMaskById(ctx, maskId, &maskObj) || !maskObj) {
        SetLastError("Mask not found");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    int w = 0, h = 0, d = 0;
    if (Dicom_Volume_GetDimensions(ctx->volume, &w, &h, &d) != NATIVE_OK || w <= 0 || h <= 0 || d <= 0) {
        SetLastError("Failed to get volume dimensions");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    if (maskObj->data.empty()) {
        SetLastError("Mask is empty");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    float sx = 1.0f, sy = 1.0f, sz = 1.0f;
    // If spacing is unavailable, fallback to 1.
    (void)Dicom_Volume_GetSpacing(ctx->volume, &sx, &sy, &sz);

    const bool ok = ExportBinaryMaskToBinaryStl(
        maskObj->data.data(),
        w, h, d,
        sx, sy, sz,
        filepath,
        step
    );

    if (!ok) {
        SetLastError("Export STL failed");
        return NATIVE_E_INTERNAL_ERROR;
    }

    return NATIVE_OK;
}
static void DilateOneVoxel6N(const uint8_t* src, uint8_t* dst, int w, int h, int d) {
    const size_t total = static_cast<size_t>(w) * static_cast<size_t>(h) * static_cast<size_t>(d);
    std::memset(dst, 0, total);

    const int threadCount = std::max(1u, std::thread::hardware_concurrency());
    const int actualThreads = std::max(1, std::min(threadCount, d));
    std::vector<std::thread> threads;
    threads.reserve(static_cast<size_t>(actualThreads));

    auto worker = [&](int zStart, int zEnd) {
        for (int z = zStart; z < zEnd; ++z) {
            const size_t zBase = static_cast<size_t>(z) * static_cast<size_t>(w) * static_cast<size_t>(h);
            for (int y = 0; y < h; ++y) {
                const size_t yBase = zBase + static_cast<size_t>(y) * static_cast<size_t>(w);
                for (int x = 0; x < w; ++x) {
                    const size_t idx = yBase + static_cast<size_t>(x);
                    if (!src[idx]) continue;
                    dst[idx] = 255;
                    if (x > 0) dst[idx - 1] = 255;
                    if (x + 1 < w) dst[idx + 1] = 255;
                    if (y > 0) dst[idx - static_cast<size_t>(w)] = 255;
                    if (y + 1 < h) dst[idx + static_cast<size_t>(w)] = 255;
                    if (z > 0) dst[idx - static_cast<size_t>(w) * static_cast<size_t>(h)] = 255;
                    if (z + 1 < d) dst[idx + static_cast<size_t>(w) * static_cast<size_t>(h)] = 255;
                }
            }
        }
    };

    for (int t = 0; t < actualThreads; ++t) {
        const int zStart = (t * d) / actualThreads;
        const int zEnd = ((t + 1) * d) / actualThreads;
        threads.emplace_back(worker, zStart, zEnd);
    }
    for (auto& th : threads) th.join();
}

static inline uint8_t RoiAt(const uint8_t* roi, int w, int h, int d, int x, int y, int z) {
    if (!roi) return 255;
    return MaskAt(roi, w, h, d, x, y, z);
}

static void ComputeSurfaceAreaAndCentroidAndBboxRoi(
    const uint8_t* boneMask,
    const uint8_t* roiMask, // nullable; >0 = inside ROI
    int w,
    int h,
    int d,
    float sx,
    float sy,
    float sz,
    int& outBoneVoxelCount,
    double& outBoneAreaMm2,
    Vec3d& outBoneCentroidVoxel,
    int& minX,
    int& minY,
    int& minZ,
    int& maxX,
    int& maxY,
    int& maxZ,
    int& outRoiVoxelCount
) {
    outBoneVoxelCount = 0;
    outBoneAreaMm2 = 0.0;
    outBoneCentroidVoxel = {};
    minX = w; minY = h; minZ = d;
    maxX = -1; maxY = -1; maxZ = -1;
    outRoiVoxelCount = 0;

    if (!boneMask) {
        minX = minY = minZ = 0;
        maxX = maxY = maxZ = -1;
        return;
    }

    const double faceAreaX = static_cast<double>(sy) * static_cast<double>(sz);
    const double faceAreaY = static_cast<double>(sx) * static_cast<double>(sz);
    const double faceAreaZ = static_cast<double>(sx) * static_cast<double>(sy);

    long double sumX = 0.0;
    long double sumY = 0.0;
    long double sumZ = 0.0;

    for (int z = 0; z < d; ++z) {
        const size_t zBase = static_cast<size_t>(z) * static_cast<size_t>(w) * static_cast<size_t>(h);
        for (int y = 0; y < h; ++y) {
            const size_t yBase = zBase + static_cast<size_t>(y) * static_cast<size_t>(w);
            for (int x = 0; x < w; ++x) {
                const size_t idx = yBase + static_cast<size_t>(x);
                if (roiMask) {
                    if (!roiMask[idx]) continue;
                    ++outRoiVoxelCount;
                }

                if (!boneMask[idx]) continue;

                ++outBoneVoxelCount;
                sumX += static_cast<long double>(x);
                sumY += static_cast<long double>(y);
                sumZ += static_cast<long double>(z);
                if (x < minX) minX = x;
                if (y < minY) minY = y;
                if (z < minZ) minZ = z;
                if (x > maxX) maxX = x;
                if (y > maxY) maxY = y;
                if (z > maxZ) maxZ = z;

                // Exposed faces: neighbor outside volume OR outside ROI OR not bone
                if (!RoiAt(roiMask, w, h, d, x - 1, y, z) || !MaskAt(boneMask, w, h, d, x - 1, y, z)) outBoneAreaMm2 += faceAreaX;
                if (!RoiAt(roiMask, w, h, d, x + 1, y, z) || !MaskAt(boneMask, w, h, d, x + 1, y, z)) outBoneAreaMm2 += faceAreaX;
                if (!RoiAt(roiMask, w, h, d, x, y - 1, z) || !MaskAt(boneMask, w, h, d, x, y - 1, z)) outBoneAreaMm2 += faceAreaY;
                if (!RoiAt(roiMask, w, h, d, x, y + 1, z) || !MaskAt(boneMask, w, h, d, x, y + 1, z)) outBoneAreaMm2 += faceAreaY;
                if (!RoiAt(roiMask, w, h, d, x, y, z - 1) || !MaskAt(boneMask, w, h, d, x, y, z - 1)) outBoneAreaMm2 += faceAreaZ;
                if (!RoiAt(roiMask, w, h, d, x, y, z + 1) || !MaskAt(boneMask, w, h, d, x, y, z + 1)) outBoneAreaMm2 += faceAreaZ;
            }
        }
    }

    if (!roiMask) {
        outRoiVoxelCount = w * h * d;
    }

    if (outBoneVoxelCount <= 0) {
        outBoneCentroidVoxel = { 0, 0, 0 };
        minX = minY = minZ = 0;
        maxX = maxY = maxZ = -1;
        return;
    }

    outBoneCentroidVoxel.x = static_cast<double>(sumX / outBoneVoxelCount);
    outBoneCentroidVoxel.y = static_cast<double>(sumY / outBoneVoxelCount);
    outBoneCentroidVoxel.z = static_cast<double>(sumZ / outBoneVoxelCount);
}

static void DilateOneVoxel6N_Roi(const uint8_t* src, uint8_t* dst, const uint8_t* roi, int w, int h, int d) {
    const size_t total = static_cast<size_t>(w) * static_cast<size_t>(h) * static_cast<size_t>(d);
    std::memset(dst, 0, total);

    for (int z = 0; z < d; ++z) {
        const size_t zBase = static_cast<size_t>(z) * static_cast<size_t>(w) * static_cast<size_t>(h);
        for (int y = 0; y < h; ++y) {
            const size_t yBase = zBase + static_cast<size_t>(y) * static_cast<size_t>(w);
            for (int x = 0; x < w; ++x) {
                const size_t idx = yBase + static_cast<size_t>(x);
                if (roi && !roi[idx]) continue;

                if (!src[idx]) {
                    // check 6 neighbors (only if neighbor is inside ROI)
                    bool any = false;
                    if (x > 0 && (!roi || roi[idx - 1]) && src[idx - 1]) any = true;
                    if (!any && x + 1 < w && (!roi || roi[idx + 1]) && src[idx + 1]) any = true;
                    if (!any && y > 0 && (!roi || roi[idx - static_cast<size_t>(w)]) && src[idx - static_cast<size_t>(w)]) any = true;
                    if (!any && y + 1 < h && (!roi || roi[idx + static_cast<size_t>(w)]) && src[idx + static_cast<size_t>(w)]) any = true;
                    if (!any && z > 0 && (!roi || roi[idx - static_cast<size_t>(w) * static_cast<size_t>(h)]) && src[idx - static_cast<size_t>(w) * static_cast<size_t>(h)]) any = true;
                    if (!any && z + 1 < d && (!roi || roi[idx + static_cast<size_t>(w) * static_cast<size_t>(h)]) && src[idx + static_cast<size_t>(w) * static_cast<size_t>(h)]) any = true;
                    if (!any) continue;
                }

                dst[idx] = 255;
            }
        }
    }
}

static void ErodeOneVoxel6N_Roi(const uint8_t* src, uint8_t* dst, const uint8_t* roi, int w, int h, int d) {
    const size_t total = static_cast<size_t>(w) * static_cast<size_t>(h) * static_cast<size_t>(d);
    std::memset(dst, 0, total);

    for (int z = 0; z < d; ++z) {
        const size_t zBase = static_cast<size_t>(z) * static_cast<size_t>(w) * static_cast<size_t>(h);
        for (int y = 0; y < h; ++y) {
            const size_t yBase = zBase + static_cast<size_t>(y) * static_cast<size_t>(w);
            for (int x = 0; x < w; ++x) {
                const size_t idx = yBase + static_cast<size_t>(x);
                if (roi && !roi[idx]) continue;
                if (!src[idx]) continue;

                // if any neighbor is outside ROI/volume OR not bone -> erode
                if (!RoiAt(roi, w, h, d, x - 1, y, z) || !MaskAt(src, w, h, d, x - 1, y, z)) continue;
                if (!RoiAt(roi, w, h, d, x + 1, y, z) || !MaskAt(src, w, h, d, x + 1, y, z)) continue;
                if (!RoiAt(roi, w, h, d, x, y - 1, z) || !MaskAt(src, w, h, d, x, y - 1, z)) continue;
                if (!RoiAt(roi, w, h, d, x, y + 1, z) || !MaskAt(src, w, h, d, x, y + 1, z)) continue;
                if (!RoiAt(roi, w, h, d, x, y, z - 1) || !MaskAt(src, w, h, d, x, y, z - 1)) continue;
                if (!RoiAt(roi, w, h, d, x, y, z + 1) || !MaskAt(src, w, h, d, x, y, z + 1)) continue;

                dst[idx] = 255;
            }
        }
    }
}

static Vec3d RandomDirectionFromIndex(uint32_t seed) {
    std::mt19937 gen(seed);
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    const double u1 = dist(gen);
    const double u2 = dist(gen);
    const double u3 = dist(gen);

    const double a = std::sqrt(1.0 - u1) * std::sin(2.0 * kPi * u2);
    const double b = std::sqrt(1.0 - u1) * std::cos(2.0 * kPi * u2);
    const double c = std::sqrt(u1) * std::sin(2.0 * kPi * u3);
    const double d = std::sqrt(u1) * std::cos(2.0 * kPi * u3);

    Vec3d v;
    v.x = (2 * b * d - 2 * a * c);
    v.y = (2 * a * b + 2 * c * d);
    v.z = (1 - 2 * b * b - 2 * c * c);
    const double n = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (n <= 1e-12) return { 0, 0, 1 };
    v.x /= n;
    v.y /= n;
    v.z /= n;
    return v;
}

static bool RayBoxIntersect(const Vec3d& origin, const Vec3d& dir, const Vec3d& minC, const Vec3d& maxC, double& tEnter, double& tExit) {
    const double eps = 1e-12;
    double tmin = -std::numeric_limits<double>::infinity();
    double tmax = std::numeric_limits<double>::infinity();
    const double o[3] = { origin.x, origin.y, origin.z };
    const double d[3] = { dir.x, dir.y, dir.z };
    const double mn[3] = { minC.x, minC.y, minC.z };
    const double mx[3] = { maxC.x, maxC.y, maxC.z };
    for (int axis = 0; axis < 3; ++axis) {
        if (std::fabs(d[axis]) < eps) {
            if (o[axis] < mn[axis] || o[axis] > mx[axis]) return false;
            continue;
        }
        double t0 = (mn[axis] - o[axis]) / d[axis];
        double t1 = (mx[axis] - o[axis]) / d[axis];
        if (t0 > t1) std::swap(t0, t1);
        if (t0 > tmin) tmin = t0;
        if (t1 < tmax) tmax = t1;
        if (tmax < tmin) return false;
    }
    tEnter = tmin;
    tExit = tmax;
    return tmax > tmin;
}

static inline bool IsBonePhysical(const uint8_t* mask, int w, int h, int d, float sx, float sy, float sz, double xMm, double yMm, double zMm) {
    if (xMm < 0.0 || yMm < 0.0 || zMm < 0.0) return false;
    const int xi = static_cast<int>(std::floor(xMm / sx + 0.5));
    const int yi = static_cast<int>(std::floor(yMm / sy + 0.5));
    const int zi = static_cast<int>(std::floor(zMm / sz + 0.5));
    if (xi < 0 || yi < 0 || zi < 0 || xi >= w || yi >= h || zi >= d) return false;
    const size_t idx = static_cast<size_t>(zi) * static_cast<size_t>(w) * static_cast<size_t>(h)
        + static_cast<size_t>(yi) * static_cast<size_t>(w)
        + static_cast<size_t>(xi);
    return mask[idx] != 0;
}

static int CountIntersections(
    const uint8_t* mask,
    int w,
    int h,
    int d,
    float sx,
    float sy,
    float sz,
    const Vec3d& entry,
    const Vec3d& dir,
    int steps,
    double stepMm
) {
    if (steps <= 0) return 1;
    int count = 0;
    bool prev = IsBonePhysical(mask, w, h, d, sx, sy, sz, entry.x, entry.y, entry.z);
    for (int i = 1; i <= steps; ++i) {
        const double dist = stepMm * static_cast<double>(i);
        const double x = entry.x + dir.x * dist;
        const double y = entry.y + dir.y * dist;
        const double z = entry.z + dir.z * dist;
        const bool cur = IsBonePhysical(mask, w, h, d, sx, sy, sz, x, y, z);
        if (cur != prev) {
            ++count;
            prev = cur;
        }
    }
    return count > 0 ? count : 1;
}

static void ComputeAnisotropyMIL(
    const uint8_t* mask,
    int w,
    int h,
    int d,
    float sx,
    float sy,
    float sz,
    const Vec3d& centerVoxel,
    const Vec3d& boxMinVoxel,
    const Vec3d& boxMaxVoxel,
    double& outDa,
    double& outE1,
    double& outE2,
    double& outE3
) {
    outDa = 0.0;
    outE1 = outE2 = outE3 = 0.0;

    const int directions = 2000;
    const int threadCount = std::max(1u, std::thread::hardware_concurrency());
    const int actualThreads = std::max(1, std::min(threadCount, directions));

    const Vec3d centerMm{ centerVoxel.x * sx, centerVoxel.y * sy, centerVoxel.z * sz };
    const Vec3d minCorner{ boxMinVoxel.x * sx, boxMinVoxel.y * sy, boxMinVoxel.z * sz };
    const Vec3d maxCorner{ boxMaxVoxel.x * sx, boxMaxVoxel.y * sy, boxMaxVoxel.z * sz };

    const double stepMm = 0.5 * std::min({ (double)sx, (double)sy, (double)sz });
    if (stepMm <= 1e-9) return;

    std::vector<std::vector<Vec3d>> locals(static_cast<size_t>(actualThreads));
    std::vector<std::thread> threads;
    threads.reserve(static_cast<size_t>(actualThreads));

    auto worker = [&](int start, int end, std::vector<Vec3d>& outPts) {
        outPts.reserve(static_cast<size_t>(end - start));
        for (int i = start; i < end; ++i) {
            const Vec3d dir = RandomDirectionFromIndex(static_cast<uint32_t>(i * 2654435761U + 1U));
            double tEnter = 0.0, tExit = 0.0;
            if (!RayBoxIntersect(centerMm, dir, minCorner, maxCorner, tEnter, tExit)) continue;
            const double lineLen = tExit - tEnter;
            if (lineLen <= 1e-9) continue;
            const Vec3d entry{ centerMm.x + dir.x * tEnter, centerMm.y + dir.y * tEnter, centerMm.z + dir.z * tEnter };
            int steps = static_cast<int>(lineLen / stepMm);
            if (steps < 1) steps = 1;
            const int intersects = CountIntersections(mask, w, h, d, sx, sy, sz, entry, dir, steps, stepMm);
            if (intersects <= 0) continue;
            const double mil = lineLen / static_cast<double>(intersects);
            if (mil <= 1e-9) continue;
            outPts.push_back(Vec3d{ dir.x * mil, dir.y * mil, dir.z * mil });
        }
    };

    for (int t = 0; t < actualThreads; ++t) {
        const int start = (t * directions) / actualThreads;
        const int end = ((t + 1) * directions) / actualThreads;
        threads.emplace_back(worker, start, end, std::ref(locals[static_cast<size_t>(t)]));
    }
    for (auto& th : threads) th.join();

    std::vector<Vec3d> points;
    size_t total = 0;
    for (const auto& v : locals) total += v.size();
    points.reserve(total);
    for (auto& v : locals) points.insert(points.end(), v.begin(), v.end());
    if (points.size() < 3) return;

    long double mx = 0, my = 0, mz = 0;
    for (const auto& p : points) {
        mx += p.x;
        my += p.y;
        mz += p.z;
    }
    mx /= (long double)points.size();
    my /= (long double)points.size();
    mz /= (long double)points.size();

    long double c00 = 0, c01 = 0, c02 = 0, c11 = 0, c12 = 0, c22 = 0;
    for (const auto& p : points) {
        const long double dx = p.x - mx;
        const long double dy = p.y - my;
        const long double dz = p.z - mz;
        c00 += dx * dx;
        c01 += dx * dy;
        c02 += dx * dz;
        c11 += dy * dy;
        c12 += dy * dz;
        c22 += dz * dz;
    }
    const long double invN = 1.0L / (long double)points.size();
    c00 *= invN;
    c01 *= invN;
    c02 *= invN;
    c11 *= invN;
    c12 *= invN;
    c22 *= invN;

    auto jacobi = [&](long double a00, long double a01, long double a02, long double a11, long double a12, long double a22, long double& e0, long double& e1, long double& e2) {
        long double A[3][3] = {
            {a00, a01, a02},
            {a01, a11, a12},
            {a02, a12, a22},
        };
        for (int it = 0; it < 30; ++it) {
            int p = 0, q = 1;
            long double maxOff = std::fabsl(A[0][1]);
            if (std::fabsl(A[0][2]) > maxOff) { maxOff = std::fabsl(A[0][2]); p = 0; q = 2; }
            if (std::fabsl(A[1][2]) > maxOff) { maxOff = std::fabsl(A[1][2]); p = 1; q = 2; }
            if (maxOff < 1e-18L) break;

            const long double app = A[p][p];
            const long double aqq = A[q][q];
            const long double apq = A[p][q];
            const long double phi = 0.5L * std::atan2(2.0L * apq, (aqq - app));
            const long double c = std::cos(phi);
            const long double s = std::sin(phi);

            for (int k = 0; k < 3; ++k) {
                const long double aik = A[p][k];
                const long double aqk = A[q][k];
                A[p][k] = c * aik - s * aqk;
                A[q][k] = s * aik + c * aqk;
            }
            for (int k = 0; k < 3; ++k) {
                const long double akp = A[k][p];
                const long double akq = A[k][q];
                A[k][p] = c * akp - s * akq;
                A[k][q] = s * akp + c * akq;
            }
        }
        e0 = A[0][0];
        e1 = A[1][1];
        e2 = A[2][2];
    };

    long double e0 = 0, e1 = 0, e2 = 0;
    jacobi(c00, c01, c02, c11, c12, c22, e0, e1, e2);

    long double ev[3] = { e0, e1, e2 };
    std::sort(ev, ev + 3);
    const long double eps = 1e-18L;
    const long double l1 = std::max(ev[0], eps);
    const long double l2 = std::max(ev[1], eps);
    const long double l3 = std::max(ev[2], eps);

    const long double r1 = std::sqrt(l1);
    const long double r2 = std::sqrt(l2);
    const long double r3 = std::sqrt(l3);

    outE1 = (double)r1;
    outE2 = (double)r2;
    outE3 = (double)r3;
    outDa = (r1 > 0) ? (double)(r3 / r1) : 0.0;
}

} // namespace

VIZ_API NativeResult MPR_CalculateBoneMetrics(
    const char* sessionId,
    int maskId,
    BoneMetrics* outMetrics
) {
    // Backward-compatible wrapper (legacy behavior: no ROI mask)
    return MPR_CalculateBoneMetricsEx(sessionId, maskId, 0, outMetrics);
}

VIZ_API NativeResult MPR_CalculateBoneMetricsEx(
    const char* sessionId,
    int maskId,
    int roiMaskId,
    BoneMetrics* outMetrics
) {
    if (!sessionId || !outMetrics) {
        SetLastError("Invalid arguments");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    MPRContext* ctx = GetMPRContextFromSession(sessionId);
    if (!ctx || !ctx->volume) {
        SetLastError("Invalid session or no volume loaded");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    int width = 0, height = 0, depth = 0;
    if (Dicom_Volume_GetDimensions(ctx->volume, &width, &height, &depth) != NATIVE_OK || width <= 0 || height <= 0 || depth <= 0) {
        SetLastError("Failed to get volume dimensions");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    float sx = 1.0f, sy = 1.0f, sz = 1.0f;
    if (Dicom_Volume_GetSpacing(ctx->volume, &sx, &sy, &sz) != NATIVE_OK) {
        SetLastError("Failed to get volume spacing");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    const MPRContext::MaskData* maskPtr = nullptr;
    if (!FindMaskById(ctx, maskId, &maskPtr) || !maskPtr) {
        SetLastError("Mask not found");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    if (maskPtr->data.empty()) {
        SetLastError("Mask data is empty");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    const uint8_t* roiMask = nullptr;
    if (roiMaskId > 0) {
        const MPRContext::MaskData* roiPtr = nullptr;
        if (!FindMaskById(ctx, roiMaskId, &roiPtr) || !roiPtr || roiPtr->data.empty()) {
            SetLastError("ROI mask not found or empty");
            return NATIVE_E_INVALID_ARGUMENT;
        }
        roiMask = roiPtr->data.data();
    }

    const uint8_t* mask = maskPtr->data.data();
    BoneMetrics m{};
    m.maskId = maskId;
    m.roiMaskId = (roiMaskId > 0) ? roiMaskId : 0;

    Vec3d centroidVoxel;
    int minX, minY, minZ, maxX, maxY, maxZ;
    double areaMm2 = 0.0;
    int voxelCount = 0;
    int roiVoxelCount = 0;
    ComputeSurfaceAreaAndCentroidAndBboxRoi(mask, roiMask, width, height, depth, sx, sy, sz, voxelCount, areaMm2, centroidVoxel, minX, minY, minZ, maxX, maxY, maxZ, roiVoxelCount);

    m.voxelCount = voxelCount;
    m.roiVoxelCount = roiVoxelCount;
    m.surfaceAreaMm2 = areaMm2;
    m.surfaceAreaCm2 = areaMm2 / 100.0;
    m.bboxMinX = minX;
    m.bboxMinY = minY;
    m.bboxMinZ = minZ;
    m.bboxMaxX = maxX;
    m.bboxMaxY = maxY;
    m.bboxMaxZ = maxZ;

    m.centroidXmm = centroidVoxel.x * sx;
    m.centroidYmm = centroidVoxel.y * sy;
    m.centroidZmm = centroidVoxel.z * sz;

    const double voxelVolMm3 = static_cast<double>(sx) * static_cast<double>(sy) * static_cast<double>(sz);
    m.volumeMm3 = static_cast<double>(voxelCount) * voxelVolMm3;
    m.volumeCm3 = m.volumeMm3 / 1000.0;
    m.bs_bv_1_per_mm = (m.volumeMm3 > 0.0) ? (m.surfaceAreaMm2 / m.volumeMm3) : 0.0;

    if (maxX >= minX && maxY >= minY && maxZ >= minZ) {
        const double dx = (double)(maxX - minX + 1) * sx;
        const double dy = (double)(maxY - minY + 1) * sy;
        const double dz = (double)(maxZ - minZ + 1) * sz;
        m.tvBoxMm3 = dx * dy * dz;
    } else {
        m.tvBoxMm3 = 0.0;
    }
    m.bv_tv = (m.tvBoxMm3 > 0.0) ? (m.volumeMm3 / m.tvBoxMm3) : 0.0;

    m.tvRoiMm3 = static_cast<double>(roiVoxelCount) * voxelVolMm3;
    m.mvRoiMm3 = std::max(0.0, m.tvRoiMm3 - m.volumeMm3);
    m.bv_tv_roi = (m.tvRoiMm3 > 0.0) ? (m.volumeMm3 / m.tvRoiMm3) : 0.0;

    // Plate-model approximations (prefer ROI BV/TV if available)
    const double bv_tv_used = (m.roiMaskId > 0) ? m.bv_tv_roi : m.bv_tv;
    m.tbThMm = (m.surfaceAreaMm2 > 0.0) ? (2.0 * (m.volumeMm3 / m.surfaceAreaMm2)) : 0.0;
    m.tbSpMm = 0.0;
    m.tbNm_1_per_mm = 0.0;
    if (m.tbThMm > 0.0 && bv_tv_used > 0.0) {
        m.tbNm_1_per_mm = bv_tv_used / m.tbThMm;
        if (m.tbNm_1_per_mm > 0.0) {
            m.tbSpMm = (1.0 / m.tbNm_1_per_mm) - m.tbThMm;
            if (m.tbSpMm < 0.0) m.tbSpMm = 0.0;
        }
    }

    // SMI via symmetric offset (dilate+erode), derivative of surface area w.r.t. distance
    m.smi = 0.0;
    if (m.volumeMm3 > 0.0 && m.surfaceAreaMm2 > 0.0) {
        std::vector<uint8_t> dilated(maskPtr->data.size());
        std::vector<uint8_t> eroded(maskPtr->data.size());
        DilateOneVoxel6N_Roi(mask, dilated.data(), roiMask, width, height, depth);
        ErodeOneVoxel6N_Roi(mask, eroded.data(), roiMask, width, height, depth);

        Vec3d dummyCenter;
        int tminX, tminY, tminZ, tmaxX, tmaxY, tmaxZ;
        double areaDilMm2 = 0.0;
        int boneCountDil = 0;
        int roiCountDil = 0;
        ComputeSurfaceAreaAndCentroidAndBboxRoi(dilated.data(), roiMask, width, height, depth, sx, sy, sz, boneCountDil, areaDilMm2, dummyCenter, tminX, tminY, tminZ, tmaxX, tmaxY, tmaxZ, roiCountDil);

        double areaEroMm2 = 0.0;
        int boneCountEro = 0;
        int roiCountEro = 0;
        ComputeSurfaceAreaAndCentroidAndBboxRoi(eroded.data(), roiMask, width, height, depth, sx, sy, sz, boneCountEro, areaEroMm2, dummyCenter, tminX, tminY, tminZ, tmaxX, tmaxY, tmaxZ, roiCountEro);

        const double pixelXY = std::max((double)sx, (double)sy);
        const double pixelZ = (double)sz;
        const double delta = 0.5 * std::sqrt(pixelXY * pixelXY * 2.0 + pixelZ * pixelZ);
        if (delta > 1e-9) {
            const double derivative_surface = (areaDilMm2 - areaEroMm2) / (2.0 * delta);
            const double smiVal = 6.0 * (m.volumeMm3 / (m.surfaceAreaMm2 * m.surfaceAreaMm2)) * derivative_surface;
            m.smi = std::max(-3.0, std::min(smiVal, 3.0));
        }
    }

    // DA via MIL (using bbox to reduce ray length)
    if (maxX >= minX && maxY >= minY && maxZ >= minZ) {
        const Vec3d boxMinV{ (double)minX, (double)minY, (double)minZ };
        const Vec3d boxMaxV{ (double)maxX, (double)maxY, (double)maxZ };
        ComputeAnisotropyMIL(mask, width, height, depth, sx, sy, sz, centroidVoxel, boxMinV, boxMaxV, m.da, m.daEigen1, m.daEigen2, m.daEigen3);
    } else {
        m.da = 0.0;
        m.daEigen1 = m.daEigen2 = m.daEigen3 = 0.0;
    }

    *outMetrics = m;
    return NATIVE_OK;
}

// 6. 锟斤拷锟斤拷masks锟斤拷锟侥硷拷
VIZ_API NativeResult MPR_SaveMasks(
    const char* sessionId,
    const char* folderPath,
    const char* maskName,
    char* outFilePath,
    int outFilePathSize
) {
    MPRContext* ctx = GetMPRContextFromSession(sessionId);
    if (!ctx) {
        SetLastError("Invalid session");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    
    if (ctx->masks.empty()) {
        SetLastError("No masks to save");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    
    if (!folderPath || !maskName || !outFilePath) {
        SetLastError("Invalid parameters");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    
    // 锟斤拷锟斤拷masks锟侥硷拷锟斤拷
    std::string masksFolder = std::string(folderPath) + "\\masks";
    CreateDirectoryA(masksFolder.c_str(), NULL);
    
    // 锟斤拷锟斤拷锟侥硷拷路锟斤拷
    std::string filePath = masksFolder + "\\" + maskName + ".json";
    
    // 锟斤拷取锟斤拷锟斤拷锟捷尺达拷
    int width, height, depth;
    if (Dicom_Volume_GetDimensions(ctx->volume, &width, &height, &depth) != NATIVE_OK) {
        SetLastError("Failed to get volume dimensions");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    
    // 锟斤拷锟斤拷JSON
    std::ostringstream json;
    json << "{\n";
    json << "  \"version\": \"1.0\",\n";
    json << "  \"volumeSize\": {\n";
    json << "    \"width\": " << width << ",\n";
    json << "    \"height\": " << height << ",\n";
    json << "    \"depth\": " << depth << "\n";
    json << "  },\n";
    json << "  \"masks\": [\n";
    
    for (size_t i = 0; i < ctx->masks.size(); i++) {
        const auto& mask = ctx->masks[i];
        
        // Base64锟斤拷锟斤拷mask锟斤拷锟斤拷
        std::string encodedData = Base64Encode(mask.data.data(), mask.data.size());
        
        json << "    {\n";
        json << "      \"id\": " << mask.id << ",\n";
        json << "      \"name\": \"" << mask.name << "\",\n";
        json << "      \"color\": \"" << mask.color << "\",\n";
        json << "      \"visible\": " << (mask.visible ? "true" : "false") << ",\n";
        json << "      \"minThreshold\": " << mask.minThreshold << ",\n";
        json << "      \"maxThreshold\": " << mask.maxThreshold << ",\n";
        json << "      \"data\": \"" << encodedData << "\"\n";
        json << "    }";
        
        if (i < ctx->masks.size() - 1) {
            json << ",";
        }
        json << "\n";
    }
    
    json << "  ]\n";
    json << "}\n";
    
    // 写锟斤拷锟侥硷拷
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        SetLastError("Failed to create file");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    
    file << json.str();
    file.close();
    
    // 锟斤拷锟斤拷锟侥硷拷路锟斤拷
    strncpy_s(outFilePath, outFilePathSize, filePath.c_str(), _TRUNCATE);
    
    printf("[MPR] Masks saved: count=%zu, file=%s\n", ctx->masks.size(), filePath.c_str());
    
    return NATIVE_OK;
}

// 7. 锟斤拷锟斤拷masks锟斤拷锟侥硷拷
VIZ_API NativeResult MPR_LoadMasks(
    const char* sessionId,
    const char* folderPath,
    int* outMaskCount,
    MaskInfo** outMaskInfos
) {
    MPRContext* ctx = GetMPRContextFromSession(sessionId);
    if (!ctx) {
        SetLastError("Invalid session");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    // Replace current masks with the loaded snapshot (per-frame workflow expects load to overwrite).
    ctx->masks.clear();
    if (ctx->previewMask) {
        delete ctx->previewMask;
        ctx->previewMask = nullptr;
    }
    
    // 锟斤拷Windows锟侥硷拷选锟斤拷曰锟斤拷锟?
    OPENFILENAMEA ofn;
    char szFile[260] = {0};
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Mask Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    
    // 锟斤拷锟矫筹拷始目录
    std::string masksFolder = std::string(folderPath) + "\\masks";
    ofn.lpstrInitialDir = masksFolder.c_str();
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    
    if (GetOpenFileNameA(&ofn) == FALSE) {
        return NATIVE_USER_CANCELLED;  // 锟矫伙拷取锟斤拷
    }
    
    // 锟斤拷取JSON锟侥硷拷
    std::ifstream file(szFile, std::ios::binary);
    if (!file.is_open()) {
        SetLastError("Failed to open file");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    
    std::string jsonContent((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
    file.close();
    
    // 锟津单碉拷JSON锟斤拷锟斤拷锟斤拷只锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷要锟斤拷masks锟斤拷锟介）
    // 锟斤拷锟斤拷 "masks": [
    size_t masksStart = jsonContent.find("\"masks\"");
    if (masksStart == std::string::npos) {
        SetLastError("Invalid JSON format: no masks array");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    
    // 统锟斤拷mask锟斤拷锟斤拷锟斤拷锟斤拷{ 锟侥革拷锟斤拷锟斤拷
    int maskCount = 0;
    size_t pos = masksStart;
    while ((pos = jsonContent.find("{", pos + 1)) != std::string::npos) {
        // 锟斤拷锟斤拷欠锟斤拷锟絤asks锟斤拷锟斤拷锟斤拷
        size_t arrayEnd = jsonContent.find("]", masksStart);
        if (pos > arrayEnd) break;
        
        // 锟斤拷锟斤拷锟斤拷{前锟斤拷锟角凤拷锟斤拷"id":
        size_t idPos = jsonContent.find("\"id\"", pos - 20);
        if (idPos != std::string::npos && idPos < pos) {
            maskCount++;
        }
    }
    
    if (maskCount == 0) {
        SetLastError("No masks found in file");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    
    // 锟斤拷取锟斤拷锟斤拷锟捷尺达拷
    int width, height, depth;
    if (Dicom_Volume_GetDimensions(ctx->volume, &width, &height, &depth) != NATIVE_OK) {
        SetLastError("Failed to get volume dimensions");
        return NATIVE_E_INVALID_ARGUMENT;
    }
    
    size_t totalVoxels = static_cast<size_t>(width) * height * depth;
    
    // 锟斤拷锟斤拷MaskInfo锟斤拷锟斤拷
    MaskInfo* maskInfos = new MaskInfo[maskCount];
    int currentMask = 0;
    
    // 锟斤拷锟斤拷每锟斤拷mask
    pos = masksStart;
    while (currentMask < maskCount) {
        // 锟斤拷锟斤拷锟斤拷一锟斤拷mask锟斤拷锟斤拷
        pos = jsonContent.find("\"id\"", pos + 1);
        if (pos == std::string::npos) break;
        
        // 锟斤拷锟斤拷id
        size_t colonPos = jsonContent.find(":", pos);
        size_t commaPos = jsonContent.find(",", colonPos);
        int id = std::stoi(jsonContent.substr(colonPos + 1, commaPos - colonPos - 1));
        
        // 锟斤拷锟斤拷name
        pos = jsonContent.find("\"name\"", pos);
        size_t nameStart = jsonContent.find("\"", pos + 7) + 1;
        size_t nameEnd = jsonContent.find("\"", nameStart);
        std::string name = jsonContent.substr(nameStart, nameEnd - nameStart);
        
        // 锟斤拷锟斤拷color
        pos = jsonContent.find("\"color\"", pos);
        size_t colorStart = jsonContent.find("\"", pos + 8) + 1;
        size_t colorEnd = jsonContent.find("\"", colorStart);
        std::string color = jsonContent.substr(colorStart, colorEnd - colorStart);
        
        // 锟斤拷锟斤拷visible
        pos = jsonContent.find("\"visible\"", pos);
        colonPos = jsonContent.find(":", pos);
        bool visible = jsonContent.substr(colonPos + 1, 4) == "true";
        
        // 锟斤拷锟斤拷minThreshold
        pos = jsonContent.find("\"minThreshold\"", pos);
        colonPos = jsonContent.find(":", pos);
        commaPos = jsonContent.find(",", colonPos);
        float minThreshold = std::stof(jsonContent.substr(colonPos + 1, commaPos - colonPos - 1));
        
        // 锟斤拷锟斤拷maxThreshold
        pos = jsonContent.find("\"maxThreshold\"", pos);
        colonPos = jsonContent.find(":", pos);
        commaPos = jsonContent.find(",", colonPos);
        float maxThreshold = std::stof(jsonContent.substr(colonPos + 1, commaPos - colonPos - 1));
        
        // 锟斤拷锟斤拷data (Base64)
        pos = jsonContent.find("\"data\"", pos);
        size_t dataStart = jsonContent.find("\"", pos + 7) + 1;
        size_t dataEnd = jsonContent.find("\"", dataStart);
        std::string encodedData = jsonContent.substr(dataStart, dataEnd - dataStart);
        
        // Base64锟斤拷锟斤拷
        std::vector<uint8_t> maskData = Base64Decode(encodedData);
        
        // 锟斤拷锟斤拷mask
        MPRContext::MaskData mask;
        mask.id = id;
        mask.name = name;
        mask.color = color;
        mask.visible = visible;
        mask.minThreshold = minThreshold;
        mask.maxThreshold = maxThreshold;
        mask.data = maskData;
        
        // 锟斤拷锟接碉拷ctx
        ctx->masks.push_back(mask);
        
        // 锟斤拷锟組askInfo
        maskInfos[currentMask].maskId = id;
        strncpy_s(maskInfos[currentMask].name, sizeof(maskInfos[currentMask].name), name.c_str(), _TRUNCATE);
        strncpy_s(maskInfos[currentMask].color, sizeof(maskInfos[currentMask].color), color.c_str(), _TRUNCATE);
        maskInfos[currentMask].visible = visible;
        maskInfos[currentMask].minThreshold = minThreshold;
        maskInfos[currentMask].maxThreshold = maxThreshold;
        
        currentMask++;
    }
    
    *outMaskCount = maskCount;
    *outMaskInfos = maskInfos;

    ctx->maskRevision++;
    
    printf("[MPR] Masks loaded: count=%d, file=%s\n", maskCount, szFile);
    
    return NATIVE_OK;
}

// ============================================================
// Morphology Operations (褰㈡€佸鎿嶄綔)
// ============================================================

// kernelSize: kernel side length (odd), e.g. 3/5/7/9
// 2D morphology: processes each slice independently
VIZ_API NativeResult MPR_MaskMorphology2D(
    const char* sessionId,
    int maskId,
    MorphologyOperation operation,
    int kernelSize
) {
    MPRContext* ctx = GetMPRContextFromSession(sessionId);
    if (!ctx || !ctx->volume) {
        SetLastError("Invalid session or no volume loaded");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    // Find mask by id
    MPRContext::MaskData* mask = nullptr;
    for (auto& m : ctx->masks) {
        if (m.id == maskId) {
            mask = &m;
            break;
        }
    }
    if (!mask) {
        SetLastError("Mask not found");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    int width = 0, height = 0, depth = 0;
    if (Dicom_Volume_GetDimensions(ctx->volume, &width, &height, &depth) != NATIVE_OK) {
        SetLastError("Failed to get volume dimensions");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    if (kernelSize <= 1) {
        return NATIVE_OK;
    }
    if ((kernelSize % 2) == 0) {
        SetLastError("kernelSize must be odd (e.g. 3/5/7/9)");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    const int radius = kernelSize / 2;

    const size_t sliceSize = static_cast<size_t>(width) * height;

    auto erode2D = [&](const uint8_t* src, uint8_t* dst) {
        std::memset(dst, 0, sliceSize);
        for (int y = radius; y < height - radius; ++y) {
            for (int x = radius; x < width - radius; ++x) {
                const size_t idx = static_cast<size_t>(y) * width + x;
                if (src[idx] == 0) continue;
                bool allSet = true;
                for (int dy = -radius; dy <= radius && allSet; ++dy) {
                    for (int dx = -radius; dx <= radius && allSet; ++dx) {
                        const size_t nidx = static_cast<size_t>(y + dy) * width + (x + dx);
                        if (src[nidx] == 0) allSet = false;
                    }
                }
                if (allSet) dst[idx] = 255;
            }
        }
    };

    auto dilate2D = [&](const uint8_t* src, uint8_t* dst) {
        std::memset(dst, 0, sliceSize);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const size_t idx = static_cast<size_t>(y) * width + x;
                if (src[idx] == 0) continue;
                const int y0 = std::max(0, y - radius);
                const int y1 = std::min(height - 1, y + radius);
                const int x0 = std::max(0, x - radius);
                const int x1 = std::min(width - 1, x + radius);
                for (int yy = y0; yy <= y1; ++yy) {
                    const size_t base = static_cast<size_t>(yy) * width;
                    for (int xx = x0; xx <= x1; ++xx) {
                        dst[base + static_cast<size_t>(xx)] = 255;
                    }
                }
            }
        }
    };

    // Process each slice (one operation per call)
    for (int z = 0; z < depth; ++z) {
        uint8_t* sliceData = mask->data.data() + static_cast<size_t>(z) * sliceSize;
        std::vector<uint8_t> src(sliceSize);
        std::vector<uint8_t> dst(sliceSize);
        std::memcpy(src.data(), sliceData, sliceSize);

        if (operation == MORPH_ERODE) {
            erode2D(src.data(), dst.data());
        } else if (operation == MORPH_DILATE) {
            dilate2D(src.data(), dst.data());
        } else if (operation == MORPH_OPEN) {
            erode2D(src.data(), dst.data());
            src.swap(dst);
            dilate2D(src.data(), dst.data());
        } else if (operation == MORPH_CLOSE) {
            dilate2D(src.data(), dst.data());
            src.swap(dst);
            erode2D(src.data(), dst.data());
        }

        std::memcpy(sliceData, dst.data(), sliceSize);
    }

    ctx->maskRevision++;
    printf("[MPR] Morphology2D: sessionId=%s, maskId=%d, op=%d, kernel=%d\n", 
           sessionId, maskId, static_cast<int>(operation), kernelSize);
    
    return NATIVE_OK;
}

// 3D morphology: radius-based cubic kernel (kernelSize 3/5/7/9)
VIZ_API NativeResult MPR_MaskMorphology3D(
    const char* sessionId,
    int maskId,
    MorphologyOperation operation,
    int kernelSize
) {
    MPRContext* ctx = GetMPRContextFromSession(sessionId);
    if (!ctx || !ctx->volume) {
        SetLastError("Invalid session or no volume loaded");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    // Find mask by id
    MPRContext::MaskData* mask = nullptr;
    for (auto& m : ctx->masks) {
        if (m.id == maskId) {
            mask = &m;
            break;
        }
    }
    if (!mask) {
        SetLastError("Mask not found");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    int width = 0, height = 0, depth = 0;
    if (Dicom_Volume_GetDimensions(ctx->volume, &width, &height, &depth) != NATIVE_OK) {
        SetLastError("Failed to get volume dimensions");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    if (kernelSize <= 1) {
        return NATIVE_OK;
    }
    if ((kernelSize % 2) == 0) {
        SetLastError("kernelSize must be odd (e.g. 3/5/7/9)");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    const int radius = kernelSize / 2;

    const size_t totalVoxels = static_cast<size_t>(width) * height * depth;

    // --- Algorithm A: separable 1D sliding-window max/min for box kernels ---
    // Boundary policy: virtual padding with 0 (matches previous behavior: erosion clears edges).
    const int k = 2 * radius + 1;

    struct MorphScratch {
        std::vector<uint8_t> padded;
        std::vector<uint8_t> line;
        std::vector<uint8_t> outLine;
        std::deque<int> dq;
    };

    auto slidingMaxPadded = [&](const uint8_t* in, int len, uint8_t* out, MorphScratch& s) {
        const int paddedLen = len + 2 * radius;
        s.padded.assign(static_cast<size_t>(paddedLen), 0);
        std::memcpy(s.padded.data() + radius, in, static_cast<size_t>(len));
        s.dq.clear();

        for (int i = 0; i < paddedLen; ++i) {
            const uint8_t v = s.padded[static_cast<size_t>(i)];
            while (!s.dq.empty() && s.padded[static_cast<size_t>(s.dq.back())] <= v) s.dq.pop_back();
            s.dq.push_back(i);

            const int winStart = i - k + 1;
            if (!s.dq.empty() && s.dq.front() < winStart) s.dq.pop_front();

            if (i >= k - 1) {
                const int outIdx = i - (k - 1);
                out[outIdx] = s.padded[static_cast<size_t>(s.dq.front())];
            }
        }
    };

    auto slidingMinPadded = [&](const uint8_t* in, int len, uint8_t* out, MorphScratch& s) {
        const int paddedLen = len + 2 * radius;
        s.padded.assign(static_cast<size_t>(paddedLen), 0);
        std::memcpy(s.padded.data() + radius, in, static_cast<size_t>(len));
        s.dq.clear();

        for (int i = 0; i < paddedLen; ++i) {
            const uint8_t v = s.padded[static_cast<size_t>(i)];
            while (!s.dq.empty() && s.padded[static_cast<size_t>(s.dq.back())] >= v) s.dq.pop_back();
            s.dq.push_back(i);

            const int winStart = i - k + 1;
            if (!s.dq.empty() && s.dq.front() < winStart) s.dq.pop_front();

            if (i >= k - 1) {
                const int outIdx = i - (k - 1);
                out[outIdx] = s.padded[static_cast<size_t>(s.dq.front())];
            }
        }
    };

    auto applyBox3D = [&](bool isDilate) {
        std::vector<uint8_t> tmp1(totalVoxels);
        std::vector<uint8_t> tmp2(totalVoxels);

        const auto apply1D = [&](const uint8_t* in, int len, uint8_t* out, MorphScratch& s) {
            if (isDilate) slidingMaxPadded(in, len, out, s);
            else slidingMinPadded(in, len, out, s);
        };

        const int threadCount = std::max(1u, std::thread::hardware_concurrency());

        // Pass X (contiguous lines): src -> tmp1
        {
            const int totalLines = depth * height;
            const int actualThreads = std::min(threadCount, std::max(1, totalLines));
            std::vector<std::thread> threads;
            threads.reserve(static_cast<size_t>(actualThreads));

            auto worker = [&](int lineStart, int lineEnd) {
                MorphScratch s;
                for (int line = lineStart; line < lineEnd; ++line) {
                    const int z = line / height;
                    const int y = line % height;
                    const size_t base = static_cast<size_t>(z) * width * height + static_cast<size_t>(y) * width;
                    apply1D(mask->data.data() + base, width, tmp1.data() + base, s);
                }
            };

            const int chunk = (totalLines + actualThreads - 1) / actualThreads;
            for (int t = 0; t < actualThreads; ++t) {
                const int start = t * chunk;
                const int end = std::min(totalLines, start + chunk);
                if (start >= end) break;
                threads.emplace_back(worker, start, end);
            }
            for (auto& th : threads) th.join();
        }

        // Pass Y (strided lines): tmp1 -> tmp2
        {
            const int totalLines = depth * width;
            const int actualThreads = std::min(threadCount, std::max(1, totalLines));
            std::vector<std::thread> threads;
            threads.reserve(static_cast<size_t>(actualThreads));

            auto worker = [&](int lineStart, int lineEnd) {
                MorphScratch s;
                s.line.resize(static_cast<size_t>(height));
                s.outLine.resize(static_cast<size_t>(height));

                for (int line = lineStart; line < lineEnd; ++line) {
                    const int z = line / width;
                    const int x = line % width;
                    const size_t zBase = static_cast<size_t>(z) * width * height;

                    for (int y = 0; y < height; ++y) {
                        s.line[static_cast<size_t>(y)] = tmp1[zBase + static_cast<size_t>(y) * width + static_cast<size_t>(x)];
                    }

                    apply1D(s.line.data(), height, s.outLine.data(), s);

                    for (int y = 0; y < height; ++y) {
                        tmp2[zBase + static_cast<size_t>(y) * width + static_cast<size_t>(x)] = s.outLine[static_cast<size_t>(y)];
                    }
                }
            };

            const int chunk = (totalLines + actualThreads - 1) / actualThreads;
            for (int t = 0; t < actualThreads; ++t) {
                const int start = t * chunk;
                const int end = std::min(totalLines, start + chunk);
                if (start >= end) break;
                threads.emplace_back(worker, start, end);
            }
            for (auto& th : threads) th.join();
        }

        // Pass Z (strided lines): tmp2 -> mask->data
        {
            const int totalLines = height * width;
            const int actualThreads = std::min(threadCount, std::max(1, totalLines));
            std::vector<std::thread> threads;
            threads.reserve(static_cast<size_t>(actualThreads));

            auto worker = [&](int lineStart, int lineEnd) {
                MorphScratch s;
                s.line.resize(static_cast<size_t>(depth));
                s.outLine.resize(static_cast<size_t>(depth));

                const size_t sliceStride = static_cast<size_t>(width) * height;
                for (int line = lineStart; line < lineEnd; ++line) {
                    const int y = line / width;
                    const int x = line % width;

                    for (int z = 0; z < depth; ++z) {
                        const size_t idx = static_cast<size_t>(z) * sliceStride + static_cast<size_t>(y) * width + static_cast<size_t>(x);
                        s.line[static_cast<size_t>(z)] = tmp2[idx];
                    }

                    apply1D(s.line.data(), depth, s.outLine.data(), s);

                    for (int z = 0; z < depth; ++z) {
                        const size_t idx = static_cast<size_t>(z) * sliceStride + static_cast<size_t>(y) * width + static_cast<size_t>(x);
                        mask->data[idx] = s.outLine[static_cast<size_t>(z)];
                    }
                }
            };

            const int chunk = (totalLines + actualThreads - 1) / actualThreads;
            for (int t = 0; t < actualThreads; ++t) {
                const int start = t * chunk;
                const int end = std::min(totalLines, start + chunk);
                if (start >= end) break;
                threads.emplace_back(worker, start, end);
            }
            for (auto& th : threads) th.join();
        }
    };

    // Execute one operation per call (iterations are handled by the caller)
    if (operation == MORPH_DILATE) {
        applyBox3D(true);
    } else if (operation == MORPH_ERODE) {
        applyBox3D(false);
    } else if (operation == MORPH_OPEN) {
        applyBox3D(false);
        applyBox3D(true);
    } else if (operation == MORPH_CLOSE) {
        applyBox3D(true);
        applyBox3D(false);
    }

    ctx->maskRevision++;
    printf("[MPR] Morphology3D: sessionId=%s, maskId=%d, op=%d, kernel=%d\n", 
           sessionId, maskId, static_cast<int>(operation), kernelSize);
    
    return NATIVE_OK;
}

// ============================================================
// Boolean Operations (甯冨皵杩愮畻)
// ============================================================

VIZ_API NativeResult MPR_MaskBoolean(
    const char* sessionId,
    int maskIdA,
    int maskIdB,
    BooleanOperation operation,
    int* outNewMaskId,
    const char* newMaskName,
    const char* newMaskColor
) {
    MPRContext* ctx = GetMPRContextFromSession(sessionId);
    if (!ctx || !ctx->volume) {
        SetLastError("Invalid session or no volume loaded");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    // Find both masks
    MPRContext::MaskData* maskA = nullptr;
    MPRContext::MaskData* maskB = nullptr;
    for (auto& m : ctx->masks) {
        if (m.id == maskIdA) maskA = &m;
        if (m.id == maskIdB) maskB = &m;
    }
    if (!maskA || !maskB) {
        SetLastError("One or both masks not found");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    if (maskA->data.size() != maskB->data.size()) {
        SetLastError("Mask sizes do not match");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    // Generate new mask ID
    int newId = 1;
    for (const auto& m : ctx->masks) {
        if (m.id >= newId) newId = m.id + 1;
    }

    // Create result mask
    MPRContext::MaskData newMask;
    newMask.id = newId;
    newMask.name = newMaskName ? newMaskName : "Boolean Result";
    newMask.color = newMaskColor ? newMaskColor : "#FFFF00";
    newMask.visible = true;
    newMask.minThreshold = 0.0f;
    newMask.maxThreshold = 0.0f;
    newMask.data.resize(maskA->data.size());

    // Perform boolean operation
    const size_t size = maskA->data.size();
    switch (operation) {
        case BOOL_UNION:
            for (size_t i = 0; i < size; ++i) {
                newMask.data[i] = (maskA->data[i] || maskB->data[i]) ? 255 : 0;
            }
            break;
        case BOOL_INTERSECT:
            for (size_t i = 0; i < size; ++i) {
                newMask.data[i] = (maskA->data[i] && maskB->data[i]) ? 255 : 0;
            }
            break;
        case BOOL_SUBTRACT:
            for (size_t i = 0; i < size; ++i) {
                newMask.data[i] = (maskA->data[i] && !maskB->data[i]) ? 255 : 0;
            }
            break;
    }

    ctx->masks.push_back(newMask);
    ctx->maskRevision++;

    if (outNewMaskId) *outNewMaskId = newId;

    printf("[MPR] Boolean: sessionId=%s, A=%d, B=%d, op=%d, result=%d\n", 
           sessionId, maskIdA, maskIdB, static_cast<int>(operation), newId);
    
    return NATIVE_OK;
}

VIZ_API NativeResult MPR_MaskInverse(
    const char* sessionId,
    int maskId
) {
    MPRContext* ctx = GetMPRContextFromSession(sessionId);
    if (!ctx || !ctx->volume) {
        SetLastError("Invalid session or no volume loaded");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    // Find mask by id
    MPRContext::MaskData* mask = nullptr;
    for (auto& m : ctx->masks) {
        if (m.id == maskId) {
            mask = &m;
            break;
        }
    }
    if (!mask) {
        SetLastError("Mask not found");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    // Invert the mask
    for (auto& v : mask->data) {
        v = (v == 0) ? 255 : 0;
    }

    ctx->maskRevision++;
    printf("[MPR] Inverse: sessionId=%s, maskId=%d\n", sessionId, maskId);
    
    return NATIVE_OK;
}
VIZ_API NativeResult MPR_GetMaskData(
    const char* sessionId,
    int maskId,
    unsigned char* buffer,
    size_t bufferSize
) {
    MPRContext* ctx = GetMPRContextFromSession(sessionId);
    if (!ctx) return NATIVE_E_INVALID_ARGUMENT;

    MPRContext::MaskData* mask = nullptr;
    for (auto& m : ctx->masks) {
        if (m.id == maskId) {
            mask = &m;
            break;
        }
    }
    if (!mask) return NATIVE_E_INVALID_ARGUMENT;

    if (mask->data.size() != bufferSize) {
        SetLastError("Buffer size mismatch");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    memcpy(buffer, mask->data.data(), bufferSize);
    return NATIVE_OK;
}

VIZ_API NativeResult MPR_UpdateMaskData(
    const char* sessionId,
    int maskId,
    const unsigned char* buffer,
    size_t bufferSize
) {
    MPRContext* ctx = GetMPRContextFromSession(sessionId);
    if (!ctx) return NATIVE_E_INVALID_ARGUMENT;

    MPRContext::MaskData* mask = nullptr;
    for (auto& m : ctx->masks) {
        if (m.id == maskId) {
            mask = &m;
            break;
        }
    }
    if (!mask) return NATIVE_E_INVALID_ARGUMENT;

    if (mask->data.size() != bufferSize) {
        SetLastError("Buffer size mismatch");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    memcpy(mask->data.data(), buffer, bufferSize);
    ctx->maskRevision++;
    return NATIVE_OK;
}

// ==================== Window Screenshot by HWND Implementation ====================

#ifdef _WIN32
// Windows and GDI+ headers must be included in correct order
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

// Helper: Get encoder CLSID for image format
static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0;
    UINT size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;

    Gdiplus::ImageCodecInfo* pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == nullptr) return -1;

    Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);
    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;
        }
    }
    free(pImageCodecInfo);
    return -1;
}

VIZ_API void* Window_CaptureByHWND(void* hwndPtr, int* outWidth, int* outHeight) {
    if (!hwndPtr || !outWidth || !outHeight) {
        SetLastError("Invalid arguments");
        return nullptr;
    }

    HWND hwnd = static_cast<HWND>(hwndPtr);
    
    // Get window client area size
    RECT rect;
    if (!GetClientRect(hwnd, &rect)) {
        SetLastError("Failed to get window rect");
        return nullptr;
    }

    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    
    if (width <= 0 || height <= 0) {
        SetLastError("Invalid window size");
        return nullptr;
    }

    // Create compatible DCs
    HDC hdcWindow = GetDC(hwnd);
    if (!hdcWindow) {
        SetLastError("Failed to get window DC");
        return nullptr;
    }

    HDC hdcMemory = CreateCompatibleDC(hdcWindow);
    if (!hdcMemory) {
        ReleaseDC(hwnd, hdcWindow);
        SetLastError("Failed to create compatible DC");
        return nullptr;
    }

    // Create bitmap
    HBITMAP hbmScreen = CreateCompatibleBitmap(hdcWindow, width, height);
    if (!hbmScreen) {
        DeleteDC(hdcMemory);
        ReleaseDC(hwnd, hdcWindow);
        SetLastError("Failed to create compatible bitmap");
        return nullptr;
    }

    SelectObject(hdcMemory, hbmScreen);

    // Capture window content
    // PrintWindow works even if window is obscured
    if (!PrintWindow(hwnd, hdcMemory, PW_CLIENTONLY)) {
        // Fallback to BitBlt if PrintWindow fails
        if (!BitBlt(hdcMemory, 0, 0, width, height, hdcWindow, 0, 0, SRCCOPY)) {
            DeleteObject(hbmScreen);
            DeleteDC(hdcMemory);
            ReleaseDC(hwnd, hdcWindow);
            SetLastError("Failed to capture window content");
            return nullptr;
        }
    }

    // Get bitmap data
    BITMAPINFOHEADER bi = {};
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height;  // Top-down DIB
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;

    size_t dataSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
    void* pixelData = malloc(dataSize);
    
    if (!pixelData) {
        DeleteObject(hbmScreen);
        DeleteDC(hdcMemory);
        ReleaseDC(hwnd, hdcWindow);
        SetLastError("Failed to allocate pixel buffer");
        return nullptr;
    }

    auto ReadBitmapToBuffer = [&]() -> bool {
        return GetDIBits(hdcMemory, hbmScreen, 0, height, pixelData,
                         (BITMAPINFO*)&bi, DIB_RGB_COLORS) != 0;
    };

    if (!ReadBitmapToBuffer()) {
        free(pixelData);
        DeleteObject(hbmScreen);
        DeleteDC(hdcMemory);
        ReleaseDC(hwnd, hdcWindow);
        SetLastError("Failed to get bitmap bits");
        return nullptr;
    }

    // Heuristic: detect black frames (common for GPU/OpenGL windows with PrintWindow).
    // If mostly black, fallback to copying from the desktop (visible content).
    auto LooksMostlyBlack = [&]() -> bool {
        const unsigned char* p = static_cast<const unsigned char*>(pixelData);
        if (!p) return true;
        const int samplesX = 32;
        const int samplesY = 32;
        int nonBlack = 0;
        for (int sy = 0; sy < samplesY; sy++) {
            int y = (height - 1) * sy / (samplesY - 1);
            for (int sx = 0; sx < samplesX; sx++) {
                int x = (width - 1) * sx / (samplesX - 1);
                size_t idx = (static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)) * 4;
                unsigned char b = p[idx + 0];
                unsigned char g = p[idx + 1];
                unsigned char r = p[idx + 2];
                if (r > 4 || g > 4 || b > 4) {
                    nonBlack++;
                    if (nonBlack > 8) return false;
                }
            }
        }
        return true;
    };

    if (LooksMostlyBlack()) {
        // Capture from the desktop using the client rect's screen coordinates.
        POINT pt = { 0, 0 };
        if (ClientToScreen(hwnd, &pt)) {
            HDC hdcScreen = GetDC(nullptr);
            if (hdcScreen) {
                // CAPTUREBLT helps with layered windows; requires windows.h.
                const DWORD rop = SRCCOPY | CAPTUREBLT;
                if (BitBlt(hdcMemory, 0, 0, width, height, hdcScreen, pt.x, pt.y, rop)) {
                    // Re-read bitmap bits.
                    ReadBitmapToBuffer();
                }
                ReleaseDC(nullptr, hdcScreen);
            }
        }
    }

    // Cleanup (GDI objects)
    DeleteObject(hbmScreen);
    DeleteDC(hdcMemory);
    ReleaseDC(hwnd, hdcWindow);

    // Convert BGRA to RGBA
    unsigned char* pixels = static_cast<unsigned char*>(pixelData);
    for (int i = 0; i < width * height; i++) {
        unsigned char temp = pixels[i * 4 + 0];  // B
        pixels[i * 4 + 0] = pixels[i * 4 + 2];   // R
        pixels[i * 4 + 2] = temp;                // B
        // Many capture paths leave alpha as 0; force opaque so PNGs display correctly.
        pixels[i * 4 + 3] = 255;
    }

    *outWidth = width;
    *outHeight = height;
    return pixelData;
}

VIZ_API NativeResult Window_SaveScreenshotByHWND(void* hwndPtr, const char* filepath) {
    if (!hwndPtr || !filepath) {
        SetLastError("Invalid arguments");
        return NATIVE_E_INVALID_ARGUMENT;
    }

    int width, height;
    void* pixelData = Window_CaptureByHWND(hwndPtr, &width, &height);
    
    if (!pixelData) {
        return NATIVE_E_FAIL;
    }

    // Initialize GDI+
    static bool gdiplusInitialized = false;
    static ULONG_PTR gdiplusToken = 0;
    
    if (!gdiplusInitialized) {
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
        gdiplusInitialized = true;
    }

    // Create GDI+ bitmap from pixel data
    Gdiplus::Bitmap bitmap(width, height, width * 4, 
                          PixelFormat32bppRGB, 
                          static_cast<BYTE*>(pixelData));

    // Get PNG encoder
    CLSID pngClsid;
    if (GetEncoderClsid(L"image/png", &pngClsid) == -1) {
        free(pixelData);
        SetLastError("PNG encoder not found");
        return NATIVE_E_FAIL;
    }

    // Convert filepath to wide string
    int len = MultiByteToWideChar(CP_UTF8, 0, filepath, -1, nullptr, 0);
    std::vector<wchar_t> wpath(len);
    MultiByteToWideChar(CP_UTF8, 0, filepath, -1, wpath.data(), len);

    // Save to file
    Gdiplus::Status status = bitmap.Save(wpath.data(), &pngClsid, nullptr);
    
    free(pixelData);

    if (status != Gdiplus::Ok) {
        SetLastError("Failed to save PNG file");
        return NATIVE_E_FAIL;
    }

    return NATIVE_OK;
}

#else
// Non-Windows stub implementation
VIZ_API void* Window_CaptureByHWND(void* hwndPtr, int* outWidth, int* outHeight) {
    SetLastError("Window capture not supported on this platform");
    return nullptr;
}

VIZ_API NativeResult Window_SaveScreenshotByHWND(void* hwndPtr, const char* filepath) {
    SetLastError("Window capture not supported on this platform");
    return NATIVE_E_NOT_SUPPORTED;
}
#endif
