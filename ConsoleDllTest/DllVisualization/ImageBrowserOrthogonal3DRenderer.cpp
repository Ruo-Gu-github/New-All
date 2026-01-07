#include "pch.h"

#include "ImageBrowserOrthogonal3DRenderer.h"
#include "VisualizationApi.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <unordered_map>

#include <GL/glew.h>

namespace {
struct TriPlanarTextures {
    GLuint texAxial = 0;
    GLuint texCoronal = 0;
    GLuint texSagittal = 0;
};

void EnsureTex(GLuint& texId) {
    if (!texId) glGenTextures(1, &texId);
}

void UploadGray8(GLuint texId, int w, int h, const void* data) {
    if (!texId || w <= 0 || h <= 0 || !data) return;
    glBindTexture(GL_TEXTURE_2D, texId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w, h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
}

void Mat4_IdentityLocal(float out[16]) {
    static const float I[16] = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };
    std::memcpy(out, I, sizeof(float) * 16);
}

void Mat4_FromEulerZYXDegLocal(float out[16], float xDeg, float yDeg, float zDeg) {
    const float rx = xDeg * 3.14159265358979323846f / 180.0f;
    const float ry = yDeg * 3.14159265358979323846f / 180.0f;
    const float rz = zDeg * 3.14159265358979323846f / 180.0f;

    const float cx = std::cos(rx), sx = std::sin(rx);
    const float cy = std::cos(ry), sy = std::sin(ry);
    const float cz = std::cos(rz), sz = std::sin(rz);

    out[0] = cz * cy;
    out[1] = sz * cy;
    out[2] = -sy;
    out[3] = 0.0f;

    out[4] = cz * sy * sx - sz * cx;
    out[5] = sz * sy * sx + cz * cx;
    out[6] = cy * sx;
    out[7] = 0.0f;

    out[8] = cz * sy * cx + sz * sx;
    out[9] = sz * sy * cx - cz * sx;
    out[10] = cy * cx;
    out[11] = 0.0f;

    out[12] = 0.0f;
    out[13] = 0.0f;
    out[14] = 0.0f;
    out[15] = 1.0f;
}

void Rot3Local(const float m[16], float x, float y, float z, float& ox, float& oy, float& oz) {
    ox = m[0] * x + m[4] * y + m[8] * z;
    oy = m[1] * x + m[5] * y + m[9] * z;
    oz = m[2] * x + m[6] * y + m[10] * z;
}

void GetViewRotMat(const float* inMat, bool inited, float outMat[16]) {
    if (inited && inMat) {
        std::memcpy(outMat, inMat, sizeof(float) * 16);
    } else {
        Mat4_IdentityLocal(outMat);
    }
}

void GetVolumeDimsFromSlices(int axW, int axH, int coW, int coH, int saW, int saH, int& outW, int& outH, int& outD) {
    outW = std::max(1, axW);
    outH = std::max(1, axH);
    outD = std::max(1, std::max(coH, saH));
}

float NiceNumber(float value) {
    if (value <= 0.0f || !std::isfinite(value)) return 1.0f;
    const float exp10 = std::pow(10.0f, std::floor(std::log10(value)));
    const float f = value / exp10;
    float nice;
    if (f < 1.5f) nice = 1.0f;
    else if (f < 3.5f) nice = 2.0f;
    else if (f < 7.5f) nice = 5.0f;
    else nice = 10.0f;
    return nice * exp10;
}

} // namespace

NativeResult ImageBrowserOrthogonal3DRenderer::Render(
    APRHandle axial,
    APRHandle coronal,
    APRHandle sagittal,
    int winWidth,
    int winHeight,
    const float viewRotMat[16],
    bool viewRotMatInitialized,
    float viewZoom,
    float viewPanX,
    float viewPanY,
    bool cropBoxVisible
) {
    if (!axial || !coronal || !sagittal) {
        return NATIVE_E_INVALID_ARGUMENT;
    }
    if (winWidth <= 0 || winHeight <= 0) {
        
        return NATIVE_OK;
    }

    // Update display buffers
    APR_UpdateSlice(axial);
    APR_UpdateSlice(coronal);
    APR_UpdateSlice(sagittal);

    int axW = 0, axH = 0;
    int coW = 0, coH = 0;
    int saW = 0, saH = 0;

    const int axDir = APR_GetSliceDirection(axial);
    const int coDir = APR_GetSliceDirection(coronal);
    const int saDir = APR_GetSliceDirection(sagittal);

    void* axSlice = APR_GetSlice(axial, axDir, &axW, &axH);
    void* coSlice = APR_GetSlice(coronal, coDir, &coW, &coH);
    void* saSlice = APR_GetSlice(sagittal, saDir, &saW, &saH);
    
    int volW = 1, volH = 1, volD = 1;
    GetVolumeDimsFromSlices(axW, axH, coW, coH, saW, saH, volW, volH, volD);
    float axCenterX = 0.0f, axCenterY = 0.0f, axCenterZ = 0.0f;
    float coCenterX = 0.0f, coCenterY = 0.0f, coCenterZ = 0.0f;
    float saCenterX = 0.0f, saCenterY = 0.0f, saCenterZ = 0.0f;
    APR_GetCenter(axial, &axCenterX, &axCenterY, &axCenterZ);
    APR_GetCenter(coronal, &coCenterX, &coCenterY, &coCenterZ);
    APR_GetCenter(sagittal, &saCenterX, &saCenterY, &saCenterZ);

    // All APR views in the same tab/session are linked via APR_SetCenter/APR_LinkCenter.
    // Use a single authoritative volume-space point to place the 3D orthogonal axes.
    const float centerX = axCenterX;
    const float centerY = axCenterY;
    const float centerZ = axCenterZ;

    float spacingX = 1.0f, spacingY = 1.0f, spacingZ = 1.0f;
    APR_GetCroppedVolumeSpacing(&spacingX, &spacingY, &spacingZ);

    float rotVol[16];
    {
        float rx = 0.0f, ry = 0.0f, rz = 0.0f;
        APR_GetRotation(axial, &rx, &ry, &rz);
        Mat4_FromEulerZYXDegLocal(rotVol, rx, ry, rz);
    }

    float camRot[16];
    GetViewRotMat(viewRotMat, viewRotMatInitialized, camRot);

    static std::unordered_map<void*, TriPlanarTextures> s_triTex;
#ifdef _WIN32
    void* ctxKey = reinterpret_cast<void*>(wglGetCurrentContext());
#else
    void* ctxKey = reinterpret_cast<void*>(glfwGetCurrentContext());
#endif
    if (!ctxKey) ctxKey = reinterpret_cast<void*>(1);

    TriPlanarTextures& tri = s_triTex[ctxKey];
    EnsureTex(tri.texAxial);
    EnsureTex(tri.texCoronal);
    EnsureTex(tri.texSagittal);

    if (axSlice && axW > 0 && axH > 0) {
        UploadGray8(tri.texAxial, axW, axH, axSlice);
    }
    if (coSlice && coW > 0 && coH > 0) {
        UploadGray8(tri.texCoronal, coW, coH, coSlice);
    }
    if (saSlice && saW > 0 && saH > 0) {
        UploadGray8(tri.texSagittal, saW, saH, saSlice);
    }

    // Some upstream paths leave depth writes or scissor enabled/disabled.
    // If GL_DEPTH_WRITEMASK is false, glClear(GL_DEPTH_BUFFER_BIT) will NOT clear the depth buffer,
    // which makes depth-tested slices disappear while depth-disabled lines still show.
    const GLboolean scissorWasEnabled = glIsEnabled(GL_SCISSOR_TEST);
    GLint scissorBox[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_SCISSOR_BOX, scissorBox);
    glDisable(GL_SCISSOR_TEST);
    glDepthMask(GL_TRUE);
    glClearDepth(1.0);

    glViewport(0, 0, winWidth, winHeight);
    // 黑色背景
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto restoreScissor = [&]() {
        if (scissorWasEnabled) {
            glEnable(GL_SCISSOR_TEST);
        } else {
            glDisable(GL_SCISSOR_TEST);
        }
        glScissor(scissorBox[0], scissorBox[1], scissorBox[2], scissorBox[3]);
    };

    // ========== 3D正交渲染 ==========
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    // 3D正交投影
    float aspect = (float)winWidth / (float)winHeight;
    float zoom = viewZoom > 0.1f ? viewZoom : 1.0f;
    float orthoSize = 0.8f / zoom;
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-aspect * orthoSize, aspect * orthoSize, -orthoSize, orthoSize, -10.0, 10.0);
    
    // ModelView: 应用相机旋转
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(viewPanX * 0.001f, viewPanY * 0.001f, 0.0f);  // 平移
    glMultMatrixf(camRot);  // 相机旋转
    
    // 计算归一化坐标
    const int maxDim = std::max(volW, std::max(volH, volD));
    const float normW = (float)volW / (float)maxDim;
    const float normH = (float)volH / (float)maxDim;
    const float normD = (float)volD / (float)maxDim;
    
    const float denomW = (float)std::max(1, volW - 1);
    const float denomH = (float)std::max(1, volH - 1);
    const float denomD = (float)std::max(1, volD - 1);

    // 与 stage 渲染一致：把体数据点(centerX/Y/Z)通过 APR 的旋转矩阵转换到世界坐标点。
    float aprRotMat2[16] = { 1,0,0,0,  0,1,0,0,  0,0,1,0,  0,0,0,1 };
    APR_GetRotationMatrix(axial, aprRotMat2);
    auto rot3b = [](const float m[16], float x, float y, float z, float& ox, float& oy, float& oz) {
        ox = m[0] * x + m[4] * y + m[8] * z;
        oy = m[1] * x + m[5] * y + m[9] * z;
        oz = m[2] * x + m[6] * y + m[10] * z;
    };
    const float pivotX2 = (float)volW * 0.5f;
    const float pivotY2 = (float)volH * 0.5f;
    const float pivotZ2 = (float)volD * 0.5f;
    const float dx2 = centerX - pivotX2;
    const float dy2 = centerY - pivotY2;
    const float dz2 = centerZ - pivotZ2;
    float rdx2 = 0.0f, rdy2 = 0.0f, rdz2 = 0.0f;
    rot3b(aprRotMat2, dx2, dy2, dz2, rdx2, rdy2, rdz2);
    const float worldCenterX2 = pivotX2 + rdx2;
    const float worldCenterY2 = pivotY2 + rdy2;
    const float worldCenterZ2 = pivotZ2 + rdz2;

    const float posX = ((worldCenterX2 / denomW) - 0.5f) * normW;
    const float posY = ((worldCenterY2 / denomH) - 0.5f) * normH;
    const float posZ = ((worldCenterZ2 / denomD) - 0.5f) * normD;
    
    const float hw = normW * 0.5f;
    const float hh = normH * 0.5f;
    const float hd = normD * 0.5f;
    
    int slicesDrawn = 0;
    
    // 启用纹理
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    
    // Axial切片 (XY平面, z=posZ)
    if (tri.texAxial && axSlice) {
        glBindTexture(GL_TEXTURE_2D, tri.texAxial);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-hw, -hh, posZ);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-hw,  hh, posZ);
        glTexCoord2f(1.0f, 0.0f); glVertex3f( hw,  hh, posZ);
        glTexCoord2f(1.0f, 1.0f); glVertex3f( hw, -hh, posZ);
        glEnd();
        slicesDrawn++;
    }
    
    // Coronal切片 (XZ平面, y=posY)
    if (tri.texCoronal && coSlice) {
        glBindTexture(GL_TEXTURE_2D, tri.texCoronal);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-hw, posY, -hd);
        glTexCoord2f(1.0f, 1.0f); glVertex3f( hw, posY, -hd);
        glTexCoord2f(1.0f, 0.0f); glVertex3f( hw, posY,  hd);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-hw, posY,  hd);
        glEnd();
        slicesDrawn++;
    }
    
    // Sagittal切片 (YZ平面, x=posX)
    if (tri.texSagittal && saSlice) {
        glBindTexture(GL_TEXTURE_2D, tri.texSagittal);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(posX, -hh, -hd);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(posX,  hh, -hd);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(posX,  hh,  hd);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(posX, -hh,  hd);
        glEnd();
        slicesDrawn++;
    }
    
    // 定位线：使用旋转后的crosshair位置
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glLineWidth(2.0f);

    // 获取旋转后的crosshair位置（体素坐标）
    float crosshairX = centerX, crosshairY = centerY, crosshairZ = centerZ;
    
    // 从axial APR获取旋转后的X,Y坐标
    if (axial) {
        APR_GetCrosshairPosition(axial, &crosshairX, &crosshairY);
    }
    
    // 从coronal APR获取旋转后的Z坐标
    if (coronal) {
        float tempU, tempV;
        APR_GetCrosshairPosition(coronal, &tempU, &tempV);
        crosshairZ = tempV;  // coronal的V对应Z坐标
    }

    // 转换到归一化3D坐标
    const float crossX = ((crosshairX / denomW) - 0.5f) * normW;
    const float crossY = ((crosshairY / denomH) - 0.5f) * normH;  
    const float crossZ = ((crosshairZ / denomD) - 0.5f) * normD;

    glBegin(GL_LINES);
    // X轴 红
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(-hw, crossY, crossZ); glVertex3f(hw, crossY, crossZ);
    // Y轴 绿
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(crossX, -hh, crossZ); glVertex3f(crossX, hh, crossZ);
    // Z轴 蓝
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(crossX, crossY, -hd); glVertex3f(crossX, crossY, hd);
    glEnd();

    // 裁切框：不要深度测试
    if (cropBoxVisible && APR_IsCropBoxEnabled()) {
        float x0 = 0, x1 = 0, y0 = 0, y1 = 0, z0 = 0, z1 = 0;
        APR_GetCropBox(&x0, &x1, &y0, &y1, &z0, &z1);

        const float cropX0 = ((x0 / denomW) - 0.5f) * normW;
        const float cropX1 = ((x1 / denomW) - 0.5f) * normW;
        const float cropY0 = ((y0 / denomH) - 0.5f) * normH;
        const float cropY1 = ((y1 / denomH) - 0.5f) * normH;
        const float cropZ0 = ((z0 / denomD) - 0.5f) * normD;
        const float cropZ1 = ((z1 / denomD) - 0.5f) * normD;

        glLineWidth(2.0f);
        glEnable(GL_LINE_STIPPLE);
        glLineStipple(1, 0x00FF);
        glColor4f(1.0f, 1.0f, 0.0f, 0.85f);

        const int shape = APR_GetCropShape();
        const int cylDir = APR_GetCropCylinderDirection();
        const float midX = (cropX0 + cropX1) * 0.5f;
        const float midY = (cropY0 + cropY1) * 0.5f;
        const float midZ = (cropZ0 + cropZ1) * 0.5f;
        const float radX = (cropX1 - cropX0) * 0.5f;
        const float radY = (cropY1 - cropY0) * 0.5f;
        const float radZ = (cropZ1 - cropZ0) * 0.5f;

        if (shape == 1) {
            const float radius = std::min({ radX, radY, radZ });
            DrawCropBoxSphere(midX, midY, midZ, radius, radius, radius);
        } else if (shape == 2) {
            DrawCropBoxCylinder(midX, midY, midZ, radX, radY, radZ, cylDir);
        } else {
            DrawCropBoxCube(cropX0, cropX1, cropY0, cropY1, cropZ0, cropZ1);
        }
        glDisable(GL_LINE_STIPPLE);
    }
    
    

    // 3D 视图比例尺（屏幕空间 overlay）
    // 优先使用 NanoVG 版本（字体更清晰），获取不到 volume 时再回退到简易 OpenGL 线段版本。
    {
        VolumeHandle volume = nullptr;
        if (APR_GetVolume(axial, &volume) == NATIVE_OK && volume) {
            // 3D视图使用正交投影，orthoSize = 0.8f / zoom
            // 正交投影的垂直范围是 [-orthoSize, orthoSize]
            // 体积的最大维度归一化到1.0，所以需要计算实际物理尺寸
            const int maxDim = std::max(volW, std::max(volH, volD));
            const float zoom = viewZoom > 0.1f ? viewZoom : 1.0f;
            const float orthoSize = 0.8f / zoom;
            
            // 使用最小spacing来计算比例尺（与水平比例尺函数一致）
            float minSpacing = spacingX;
            if (spacingY < minSpacing) minSpacing = spacingY;
            if (spacingZ < minSpacing) minSpacing = spacingZ;
            
            // 计算屏幕像素对应的物理尺寸：
            // - 正交投影范围是 [-orthoSize, orthoSize]，总跨度 2*orthoSize
            // - 归一化体积范围是 [-1, 1]，总跨度 2，对应物理尺寸 maxDim * minSpacing
            // - 屏幕高度 winHeight 显示正交投影的全部跨度 2*orthoSize
            // - 正交投影跨度 2*orthoSize 对应归一化跨度 2*orthoSize
            // - 归一化跨度 2*orthoSize 对应物理尺寸 (2*orthoSize/2) * (maxDim * minSpacing)
            // - 即：winHeight像素 = orthoSize * maxDim * minSpacing 毫米
            // - 因此：mmPerPixel = (orthoSize * maxDim * minSpacing) / winHeight
            const float mmPerPixel = (orthoSize * (float)maxDim * minSpacing) / (float)winHeight;
            const float effectiveZoom = 1.0f / mmPerPixel;
            
            // 使用水平比例尺（地图样式）
            DrawHorizontalScaleBarNVG(winWidth, winHeight, effectiveZoom, volume);
        } else {
            DrawScaleBar(winWidth, winHeight, viewZoom, spacingX, spacingY, spacingZ, volW, volH, volD);
        }
    }
    
    // 绘制方向指示立方体（左下角）
    DrawOrientationCubeNVG(winWidth, winHeight, camRot);
    
    restoreScissor();
    return NATIVE_OK;
}

void ImageBrowserOrthogonal3DRenderer::DrawCropBoxCube(float x0, float x1, float y0, float y1, float z0, float z1) {
    glBegin(GL_LINES);
    glVertex3f(x0, y0, z0); glVertex3f(x1, y0, z0);
    glVertex3f(x1, y0, z0); glVertex3f(x1, y1, z0);
    glVertex3f(x1, y1, z0); glVertex3f(x0, y1, z0);
    glVertex3f(x0, y1, z0); glVertex3f(x0, y0, z0);
    glVertex3f(x0, y0, z1); glVertex3f(x1, y0, z1);
    glVertex3f(x1, y0, z1); glVertex3f(x1, y1, z1);
    glVertex3f(x1, y1, z1); glVertex3f(x0, y1, z1);
    glVertex3f(x0, y1, z1); glVertex3f(x0, y0, z1);
    glVertex3f(x0, y0, z0); glVertex3f(x0, y0, z1);
    glVertex3f(x1, y0, z0); glVertex3f(x1, y0, z1);
    glVertex3f(x1, y1, z0); glVertex3f(x1, y1, z1);
    glVertex3f(x0, y1, z0); glVertex3f(x0, y1, z1);
    glEnd();
}

void ImageBrowserOrthogonal3DRenderer::DrawCropBoxSphere(float midX, float midY, float midZ, float radX, float radY, float radZ, int segments) {
    const float radius = std::min({ radX, radY, radZ });
    const float pi2 = 2.0f * 3.14159265f;

    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; i++) {
        float a = (float)i / segments * pi2;
        glVertex3f(midX + radius * cosf(a), midY + radius * sinf(a), midZ);
    }
    glEnd();

    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; i++) {
        float a = (float)i / segments * pi2;
        glVertex3f(midX + radius * cosf(a), midY, midZ + radius * sinf(a));
    }
    glEnd();

    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; i++) {
        float a = (float)i / segments * pi2;
        glVertex3f(midX, midY + radius * cosf(a), midZ + radius * sinf(a));
    }
    glEnd();
}

void ImageBrowserOrthogonal3DRenderer::DrawCropBoxCylinder(float midX, float midY, float midZ, float radX, float radY, float radZ, int cylDir, int segments) {
    const float pi2 = 2.0f * 3.14159265f;
    if (cylDir == 0) {
        const float r = std::min(radX, radY);
        const float z0 = midZ - radZ;
        const float z1 = midZ + radZ;
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < segments; i++) {
            float a = (float)i / segments * pi2;
            glVertex3f(midX + r * cosf(a), midY + r * sinf(a), z0);
        }
        glEnd();
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < segments; i++) {
            float a = (float)i / segments * pi2;
            glVertex3f(midX + r * cosf(a), midY + r * sinf(a), z1);
        }
        glEnd();
        glBegin(GL_LINES);
        for (int i = 0; i < 4; i++) {
            float a = (float)i / 4 * pi2;
            float x = midX + r * cosf(a);
            float y = midY + r * sinf(a);
            glVertex3f(x, y, z0);
            glVertex3f(x, y, z1);
        }
        glEnd();
    } else if (cylDir == 1) {
        const float r = std::min(radX, radZ);
        const float y0 = midY - radY;
        const float y1 = midY + radY;
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < segments; i++) {
            float a = (float)i / segments * pi2;
            glVertex3f(midX + r * cosf(a), y0, midZ + r * sinf(a));
        }
        glEnd();
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < segments; i++) {
            float a = (float)i / segments * pi2;
            glVertex3f(midX + r * cosf(a), y1, midZ + r * sinf(a));
        }
        glEnd();
        glBegin(GL_LINES);
        for (int i = 0; i < 4; i++) {
            float a = (float)i / 4 * pi2;
            float x = midX + r * cosf(a);
            float z = midZ + r * sinf(a);
            glVertex3f(x, y0, z);
            glVertex3f(x, y1, z);
        }
        glEnd();
    } else {
        const float r = std::min(radY, radZ);
        const float x0 = midX - radX;
        const float x1 = midX + radX;
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < segments; i++) {
            float a = (float)i / segments * pi2;
            glVertex3f(x0, midY + r * cosf(a), midZ + r * sinf(a));
        }
        glEnd();
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < segments; i++) {
            float a = (float)i / segments * pi2;
            glVertex3f(x1, midY + r * cosf(a), midZ + r * sinf(a));
        }
        glEnd();
        glBegin(GL_LINES);
        for (int i = 0; i < 4; i++) {
            float a = (float)i / 4 * pi2;
            float y = midY + r * cosf(a);
            float z = midZ + r * sinf(a);
            glVertex3f(x0, y, z);
            glVertex3f(x1, y, z);
        }
        glEnd();
    }
}

void ImageBrowserOrthogonal3DRenderer::DrawScaleBar(int winWidth, int winHeight, float zoom, float spacingX, float spacingY, float spacingZ, int volW, int volH, int volD) {
    if (winWidth <= 0 || winHeight <= 0) return;

    const float minSpacing = std::max(0.0001f, std::min({ spacingX, spacingY, spacingZ }));
    const float maxDim = (float)std::max(volW, std::max(volH, volD));

    const float mmPerWorld = minSpacing * maxDim;
    const float pxPerWorld = (zoom <= 0.0f) ? 0.0f : (zoom * (float)winHeight) / 3.0f;
    if (pxPerWorld <= 0.0f) return;

    const float mmPerPx = mmPerWorld / pxPerWorld;
    const float targetPx = 100.0f;
    float rawMm = mmPerPx * targetPx;
    float niceMm = NiceNumber(rawMm);
    float barPx = niceMm / mmPerPx;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, winWidth, 0, winHeight, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(1.0f);

    // 右下角位置
    const float margin = 20.0f;
    const float x1 = (float)winWidth - margin;
    const float x0 = x1 - barPx;
    const float y0 = margin;
    const float tick = 5.0f;

    glBegin(GL_LINES);
    // 左端刻度
    glVertex2f(x0, y0); glVertex2f(x0, y0 + tick);
    // 横线
    glVertex2f(x0, y0); glVertex2f(x1, y0);
    // 右端刻度
    glVertex2f(x1, y0); glVertex2f(x1, y0 + tick);
    glEnd();

    // 绘制标签文字 (使用简单的点阵方式绘制数字)
    // 由于OpenGL固定管线没有内置文字渲染，我们用简单的线段画数字
    DrawScaleLabel(niceMm, (x0 + x1) * 0.5f, y0 + tick + 3.0f);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// 简单的7段数码管风格数字绘制
namespace {
void DrawDigit(char c, float x, float y, float w, float h) {
    // 7段: top, top-left, top-right, middle, bottom-left, bottom-right, bottom
    // 每个数字定义哪些段亮
    static const bool segs[11][7] = {
        {1,1,1,0,1,1,1}, // 0
        {0,0,1,0,0,1,0}, // 1
        {1,0,1,1,1,0,1}, // 2
        {1,0,1,1,0,1,1}, // 3
        {0,1,1,1,0,1,0}, // 4
        {1,1,0,1,0,1,1}, // 5
        {1,1,0,1,1,1,1}, // 6
        {1,0,1,0,0,1,0}, // 7
        {1,1,1,1,1,1,1}, // 8
        {1,1,1,1,0,1,1}, // 9
        {0,0,0,0,0,0,0}, // 空格或其他
    };
    
    int idx = 10;
    if (c >= '0' && c <= '9') idx = c - '0';
    
    const float hw = w * 0.5f;
    const float hh = h * 0.5f;
    
    glBegin(GL_LINES);
    // top
    if (segs[idx][0]) { glVertex2f(x, y + h); glVertex2f(x + w, y + h); }
    // top-left
    if (segs[idx][1]) { glVertex2f(x, y + hh); glVertex2f(x, y + h); }
    // top-right
    if (segs[idx][2]) { glVertex2f(x + w, y + hh); glVertex2f(x + w, y + h); }
    // middle
    if (segs[idx][3]) { glVertex2f(x, y + hh); glVertex2f(x + w, y + hh); }
    // bottom-left
    if (segs[idx][4]) { glVertex2f(x, y); glVertex2f(x, y + hh); }
    // bottom-right
    if (segs[idx][5]) { glVertex2f(x + w, y); glVertex2f(x + w, y + hh); }
    // bottom
    if (segs[idx][6]) { glVertex2f(x, y); glVertex2f(x + w, y); }
    glEnd();
}

void DrawChar(char c, float x, float y, float w, float h) {
    if (c == 'm') {
        glBegin(GL_LINES);
        glVertex2f(x, y); glVertex2f(x, y + h);
        glVertex2f(x, y + h); glVertex2f(x + w * 0.5f, y + h * 0.6f);
        glVertex2f(x + w * 0.5f, y + h * 0.6f); glVertex2f(x + w, y + h);
        glVertex2f(x + w, y + h); glVertex2f(x + w, y);
        glEnd();
    } else if (c == '.') {
        glBegin(GL_POINTS);
        glVertex2f(x + w * 0.5f, y);
        glEnd();
    } else if (c >= '0' && c <= '9') {
        DrawDigit(c, x, y, w, h);
    }
}
} // namespace

void ImageBrowserOrthogonal3DRenderer::DrawScaleLabel(float mm, float centerX, float baseY) {
    char buf[32];
    if (mm >= 1.0f) {
        snprintf(buf, sizeof(buf), "%.0fmm", mm);
    } else {
        snprintf(buf, sizeof(buf), "%.1fmm", mm);
    }
    
    const float charW = 6.0f;
    const float charH = 9.0f;
    const float spacing = 1.0f;
    
    int len = (int)strlen(buf);
    float totalW = len * (charW + spacing) - spacing;
    float startX = centerX - totalW * 0.5f;
    
    for (int i = 0; i < len; i++) {
        DrawChar(buf[i], startX + i * (charW + spacing), baseY, charW, charH);
    }
}

void ImageBrowserOrthogonal3DRenderer::Mat4_Identity(float out[16]) {
    Mat4_IdentityLocal(out);
}

void ImageBrowserOrthogonal3DRenderer::Mat4_FromEulerZYXDeg(float out[16], float xDeg, float yDeg, float zDeg) {
    Mat4_FromEulerZYXDegLocal(out, xDeg, yDeg, zDeg);
}

void ImageBrowserOrthogonal3DRenderer::Rot3(const float m[16], float x, float y, float z, float& ox, float& oy, float& oz) {
    Rot3Local(m, x, y, z, ox, oy, oz);
}
