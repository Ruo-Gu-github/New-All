// ============================================================================
// 3D 正交渲染调试代码模板
// ============================================================================
// 
// 用途：当 3D 视图渲染出现问题时，可以使用这段代码进行分阶段调试
// 
// 使用方法：
// 1. 将此代码替换到 ImageBrowserOrthogonal3DRenderer::Render() 中
// 2. 创建文件 D:\3d_stage.txt，内容为阶段数字（0-7）
// 3. 查看日志 D:\3d_render_debug.log
// 
// 阶段说明：
//   stage 0 = 仅屏幕空间2D贴图（验证纹理数据是否正确）
//   stage 1 = NDC正交投影(-1..1) + MV=I，只画axial在z=0
//   stage 2 = 仍用NDC投影，但使用posX/posY/posZ
//   stage 3 = 使用3D的glOrtho(aspect*orthoSize)，MV=I
//   stage 4 = 在stage3基础上加pan平移
//   stage 5 = 在stage4基础上加camRot旋转
//   stage 6 = 启用深度测试，绘制3张切片（最终效果）
//   stage 7 = 诊断模式（半透明+无深度），用于确认三张切片都存在
// ============================================================================

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

// ============================================================================
// 辅助函数
// ============================================================================

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
    static const float I[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
    std::memcpy(out, I, sizeof(float) * 16);
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

bool TryReadIntFromFile(const char* path, int& outValue) {
    outValue = 0;
    FILE* f = nullptr;
    fopen_s(&f, path, "r");
    if (!f) return false;
    int v = 0;
    const int ok = fscanf_s(f, "%d", &v);
    fclose(f);
    if (ok != 1) return false;
    outValue = v;
    return true;
}

bool TryReadDebugStage(int& outStage) {
    outStage = -1;

#ifdef _WIN32
    // 优先从DLL目录读取 3d_stage.txt
    HMODULE mod = nullptr;
    if (GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(&TryReadDebugStage),
            &mod)) {
        char dllPath[MAX_PATH] = {};
        const DWORD len = GetModuleFileNameA(mod, dllPath, MAX_PATH);
        if (len > 0 && len < MAX_PATH) {
            for (int i = (int)len - 1; i >= 0; --i) {
                if (dllPath[i] == '\\' || dllPath[i] == '/') {
                    dllPath[i + 1] = '\0';
                    break;
                }
            }
            char stagePath[MAX_PATH] = {};
            strcpy_s(stagePath, dllPath);
            strcat_s(stagePath, "3d_stage.txt");

            if (TryReadIntFromFile(stagePath, outStage)) {
                FILE* lf = nullptr;
                fopen_s(&lf, "D:\\3d_render_debug.log", "a");
                if (lf) {
                    fprintf(lf, "[3D][STAGE] stage loaded from %s => %d\n", stagePath, outStage);
                    fclose(lf);
                }
                return true;
            }
        }
    }
#endif

    // 回退：从D:\3d_stage.txt读取
    if (TryReadIntFromFile("D:\\3d_stage.txt", outStage)) {
        FILE* lf = nullptr;
        fopen_s(&lf, "D:\\3d_render_debug.log", "a");
        if (lf) {
            fprintf(lf, "[3D][STAGE] stage loaded from D:\\3d_stage.txt => %d\n", outStage);
            fclose(lf);
        }
        return true;
    }
    return false;
}

void LogMat4(FILE* logFile, const char* name, const float m[16]) {
    if (!logFile || !name || !m) return;
    fprintf(logFile, "%s:\n", name);
    fprintf(logFile, "  [%7.3f %7.3f %7.3f %7.3f]\n", m[0], m[4], m[8], m[12]);
    fprintf(logFile, "  [%7.3f %7.3f %7.3f %7.3f]\n", m[1], m[5], m[9], m[13]);
    fprintf(logFile, "  [%7.3f %7.3f %7.3f %7.3f]\n", m[2], m[6], m[10], m[14]);
    fprintf(logFile, "  [%7.3f %7.3f %7.3f %7.3f]\n", m[3], m[7], m[11], m[15]);
}

void LogGLStateAndMatrices(const char* tag) {
    FILE* logFile = nullptr;
    fopen_s(&logFile, "D:\\3d_render_debug.log", "a");
    if (!logFile) return;

    fprintf(logFile, "[3D][STAGE] %s\n", tag ? tag : "(null)");

    GLboolean depthTest, blend, texture2D, cullFace, lighting, depthWrite;
    glGetBooleanv(GL_DEPTH_TEST, &depthTest);
    glGetBooleanv(GL_BLEND, &blend);
    glGetBooleanv(GL_TEXTURE_2D, &texture2D);
    glGetBooleanv(GL_CULL_FACE, &cullFace);
    glGetBooleanv(GL_LIGHTING, &lighting);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWrite);

    fprintf(logFile, "  GL_DEPTH_TEST=%d GL_BLEND=%d GL_TEXTURE_2D=%d GL_CULL_FACE=%d GL_LIGHTING=%d GL_DEPTH_WRITEMASK=%d\n",
            depthTest, blend, texture2D, cullFace, lighting, depthWrite);

    float proj[16], mv[16];
    glGetFloatv(GL_PROJECTION_MATRIX, proj);
    glGetFloatv(GL_MODELVIEW_MATRIX, mv);
    LogMat4(logFile, "  PROJECTION", proj);
    LogMat4(logFile, "  MODELVIEW", mv);

    fclose(logFile);
}

void DrawTexturedQuad_ScreenSpace(GLuint tex, int winWidth, int winHeight, float cx, float cy, float w, float h) {
    if (!tex || winWidth <= 0 || winHeight <= 0) return;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, (double)winWidth, 0.0, (double)winHeight, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    const float x0 = cx - w * 0.5f;
    const float x1 = cx + w * 0.5f;
    const float y0 = cy - h * 0.5f;
    const float y1 = cy + h * 0.5f;

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(x0, y0, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(x0, y1, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(x1, y1, 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(x1, y0, 0.0f);
    glEnd();
}

} // namespace

// ============================================================================
// 主渲染函数（调试版本）
// ============================================================================

NativeResult ImageBrowserOrthogonal3DRenderer::Render_DEBUG(
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
    // ========== 1. 写入日志 ==========
    FILE* logFile = nullptr;
    fopen_s(&logFile, "D:\\3d_render_debug.log", "a");
    if (logFile) {
        fprintf(logFile, "\n========== [3D Render Start] ==========\n");
        fprintf(logFile, "[3D] axial=%p coronal=%p sagittal=%p win=%dx%d\n", axial, coronal, sagittal, winWidth, winHeight);
        fprintf(logFile, "[3D] viewRotMatInit=%d zoom=%.3f pan=(%.3f,%.3f)\n",
                viewRotMatInitialized, viewZoom, viewPanX, viewPanY);
        fclose(logFile);
    }

    // ========== 2. 参数验证 ==========
    if (!axial || !coronal || !sagittal) return NATIVE_E_INVALID_ARGUMENT;
    if (winWidth <= 0 || winHeight <= 0) return NATIVE_OK;

    // ========== 3. 更新切片数据 ==========
    APR_UpdateSlice(axial);
    APR_UpdateSlice(coronal);
    APR_UpdateSlice(sagittal);

    // ========== 4. 获取切片数据 ==========
    int axW = 0, axH = 0, coW = 0, coH = 0, saW = 0, saH = 0;
    void* axSlice = APR_GetSlice(axial, APR_GetSliceDirection(axial), &axW, &axH);
    void* coSlice = APR_GetSlice(coronal, APR_GetSliceDirection(coronal), &coW, &coH);
    void* saSlice = APR_GetSlice(sagittal, APR_GetSliceDirection(sagittal), &saW, &saH);

    fopen_s(&logFile, "D:\\3d_render_debug.log", "a");
    if (logFile) {
        fprintf(logFile, "[3D] axSlice=%p (%dx%d)\n", axSlice, axW, axH);
        fprintf(logFile, "[3D] coSlice=%p (%dx%d)\n", coSlice, coW, coH);
        fprintf(logFile, "[3D] saSlice=%p (%dx%d)\n", saSlice, saW, saH);
        fclose(logFile);
    }

    // ========== 5. 计算体积尺寸 ==========
    int volW = 1, volH = 1, volD = 1;
    GetVolumeDimsFromSlices(axW, axH, coW, coH, saW, saH, volW, volH, volD);

    // ========== 6. 获取中心点和相机矩阵 ==========
    float centerX = 0, centerY = 0, centerZ = 0;
    APR_GetCenter(axial, &centerX, &centerY, &centerZ);

    float camRot[16];
    GetViewRotMat(viewRotMat, viewRotMatInitialized, camRot);

    // ========== 7. 上传纹理 ==========
    static std::unordered_map<void*, TriPlanarTextures> s_triTex;
    void* ctxKey = reinterpret_cast<void*>(wglGetCurrentContext());
    if (!ctxKey) ctxKey = reinterpret_cast<void*>(1);

    TriPlanarTextures& tri = s_triTex[ctxKey];
    EnsureTex(tri.texAxial);
    EnsureTex(tri.texCoronal);
    EnsureTex(tri.texSagittal);

    if (axSlice && axW > 0 && axH > 0) UploadGray8(tri.texAxial, axW, axH, axSlice);
    if (coSlice && coW > 0 && coH > 0) UploadGray8(tri.texCoronal, coW, coH, coSlice);
    if (saSlice && saW > 0 && saH > 0) UploadGray8(tri.texSagittal, saW, saH, saSlice);

    // ========== 8. OpenGL状态准备（关键！） ==========
    // 保存scissor状态
    const GLboolean scissorWasEnabled = glIsEnabled(GL_SCISSOR_TEST);
    GLint scissorBox[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_SCISSOR_BOX, scissorBox);
    glDisable(GL_SCISSOR_TEST);

    // 确保深度写入开启（否则glClear不会清除深度缓冲）
    glDepthMask(GL_TRUE);
    glClearDepth(1.0);

    // 设置视口并清除
    glViewport(0, 0, winWidth, winHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto restoreScissor = [&]() {
        if (scissorWasEnabled) glEnable(GL_SCISSOR_TEST);
        else glDisable(GL_SCISSOR_TEST);
        glScissor(scissorBox[0], scissorBox[1], scissorBox[2], scissorBox[3]);
    };

    // ========== 9. 读取调试阶段 ==========
    int debugStage = -1;
    const bool hasDebugStage = TryReadDebugStage(debugStage);

    if (!hasDebugStage) {
        // 没有调试文件，使用默认stage 6（完整渲染）
        debugStage = 6;
    }

    fopen_s(&logFile, "D:\\3d_render_debug.log", "a");
    if (logFile) {
        fprintf(logFile, "[3D][STAGE] Using stage=%d\n", debugStage);
        fclose(logFile);
    }

    // ========== 10. 计算归一化坐标 ==========
    const int maxDim = std::max(volW, std::max(volH, volD));
    const float normW = (float)volW / (float)maxDim;
    const float normH = (float)volH / (float)maxDim;
    const float normD = (float)volD / (float)maxDim;

    const float denomW = (float)std::max(1, volW - 1);
    const float denomH = (float)std::max(1, volH - 1);
    const float denomD = (float)std::max(1, volD - 1);

    // ========== 11. 计算virtualCenter（关键！） ==========
    float aprRotMat[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
    APR_GetRotationMatrix(axial, aprRotMat);

    // R^T 的元素（列优先存储，取转置）
    const float r00 = aprRotMat[0], r01 = aprRotMat[4], r02 = aprRotMat[8];
    const float r10 = aprRotMat[1], r11 = aprRotMat[5], r12 = aprRotMat[9];
    const float r20 = aprRotMat[2], r21 = aprRotMat[6], r22 = aprRotMat[10];

    const float pivotX = (float)volW * 0.5f;
    const float pivotY = (float)volH * 0.5f;
    const float pivotZ = (float)volD * 0.5f;

    const float dx = centerX - pivotX;
    const float dy = centerY - pivotY;
    const float dz = centerZ - pivotZ;

    // virtual = Pivot + R^T * (real - Pivot)
    const float virtualCenterX = pivotX + (r00 * dx + r10 * dy + r20 * dz);
    const float virtualCenterY = pivotY + (r01 * dx + r11 * dy + r21 * dz);
    const float virtualCenterZ = pivotZ + (r02 * dx + r12 * dy + r22 * dz);

    const float posX = ((virtualCenterX / denomW) - 0.5f) * normW;
    const float posY = ((virtualCenterY / denomH) - 0.5f) * normH;
    const float posZ = ((virtualCenterZ / denomD) - 0.5f) * normD;

    const float hw = normW * 0.5f;
    const float hh = normH * 0.5f;
    const float hd = normD * 0.5f;

    fopen_s(&logFile, "D:\\3d_render_debug.log", "a");
    if (logFile) {
        fprintf(logFile, "[3D] center=(%.2f,%.2f,%.2f) virtual=(%.2f,%.2f,%.2f)\n",
                centerX, centerY, centerZ, virtualCenterX, virtualCenterY, virtualCenterZ);
        fprintf(logFile, "[3D] pos=(%.4f,%.4f,%.4f) hw=%.4f hh=%.4f hd=%.4f\n",
                posX, posY, posZ, hw, hh, hd);
        fclose(logFile);
    }

    // ========== 12. Stage 0: 屏幕空间2D ==========
    if (debugStage <= 0) {
        LogGLStateAndMatrices("stage0: before draw");
        const float size = (float)(std::min)(winWidth, winHeight) * 0.8f;
        DrawTexturedQuad_ScreenSpace(tri.texAxial, winWidth, winHeight, winWidth * 0.5f, winHeight * 0.5f, size, size);
        LogGLStateAndMatrices("stage0: after draw");
        restoreScissor();
        return NATIVE_OK;
    }

    // ========== 13. Stage 1+: 设置矩阵 ==========
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    const float aspect = (float)winWidth / (float)winHeight;
    const float zoom = viewZoom > 0.1f ? viewZoom : 1.0f;
    const float orthoSize = 0.8f / zoom;

    // Projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (debugStage == 1 || debugStage == 2) {
        glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);  // NDC
    } else {
        glOrtho(-aspect * orthoSize, aspect * orthoSize, -orthoSize, orthoSize, -10.0, 10.0);
    }

    // ModelView
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    if (debugStage >= 4) {
        glTranslatef(viewPanX * 0.001f, viewPanY * 0.001f, 0.0f);
    }
    if (debugStage >= 5) {
        glMultMatrixf(camRot);
    }

    LogGLStateAndMatrices("stage1+: after set matrices");

    // ========== 14. 深度测试设置 ==========
    const bool debugTransparentNoDepth = (debugStage >= 7);
    if (debugStage >= 6) {
        if (debugTransparentNoDepth) {
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        } else {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LEQUAL);
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
        }
    }

    const float zAx = (debugStage >= 2) ? posZ : 0.0f;
    const float a = debugTransparentNoDepth ? 0.70f : 1.0f;

    int slicesDrawn = 0;

    // ========== 15. 绘制切片 ==========
    // Axial
    if (tri.texAxial && axSlice) {
        glBindTexture(GL_TEXTURE_2D, tri.texAxial);
        if (debugTransparentNoDepth) glColor4f(1.0f, 0.85f, 0.85f, a);
        else glColor4f(1.0f, 1.0f, 1.0f, a);

        glBegin(GL_QUADS);
        glTexCoord2f(0, 1); glVertex3f(-hw, -hh, zAx);
        glTexCoord2f(0, 0); glVertex3f(-hw,  hh, zAx);
        glTexCoord2f(1, 0); glVertex3f( hw,  hh, zAx);
        glTexCoord2f(1, 1); glVertex3f( hw, -hh, zAx);
        glEnd();
        slicesDrawn++;
    }

    // Coronal & Sagittal (stage 6+)
    if (debugStage >= 6) {
        if (tri.texCoronal && coSlice) {
            glBindTexture(GL_TEXTURE_2D, tri.texCoronal);
            if (debugTransparentNoDepth) glColor4f(0.85f, 1.0f, 0.85f, a);
            else glColor4f(1.0f, 1.0f, 1.0f, a);

            glBegin(GL_QUADS);
            glTexCoord2f(0, 1); glVertex3f(-hw, posY, -hd);
            glTexCoord2f(1, 1); glVertex3f( hw, posY, -hd);
            glTexCoord2f(1, 0); glVertex3f( hw, posY,  hd);
            glTexCoord2f(0, 0); glVertex3f(-hw, posY,  hd);
            glEnd();
            slicesDrawn++;
        }

        if (tri.texSagittal && saSlice) {
            glBindTexture(GL_TEXTURE_2D, tri.texSagittal);
            if (debugTransparentNoDepth) glColor4f(0.85f, 0.85f, 1.0f, a);
            else glColor4f(1.0f, 1.0f, 1.0f, a);

            glBegin(GL_QUADS);
            glTexCoord2f(0, 1); glVertex3f(posX, -hh, -hd);
            glTexCoord2f(1, 1); glVertex3f(posX,  hh, -hd);
            glTexCoord2f(1, 0); glVertex3f(posX,  hh,  hd);
            glTexCoord2f(0, 0); glVertex3f(posX, -hh,  hd);
            glEnd();
            slicesDrawn++;
        }
    }

    // ========== 16. 绘制定位线 ==========
    if (debugStage >= 6) {
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
        glLineWidth(2.0f);

        glBegin(GL_LINES);
        // X轴（红色）
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex3f(-hw, posY, posZ);
        glVertex3f(hw, posY, posZ);
        // Y轴（绿色）
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex3f(posX, -hh, posZ);
        glVertex3f(posX, hh, posZ);
        // Z轴（蓝色）
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex3f(posX, posY, -hd);
        glVertex3f(posX, posY, hd);
        glEnd();
    }

    fopen_s(&logFile, "D:\\3d_render_debug.log", "a");
    if (logFile) {
        fprintf(logFile, "[3D][STAGE] stage=%d slicesDrawn=%d\n", debugStage, slicesDrawn);
        fclose(logFile);
    }

    LogGLStateAndMatrices("stage1+: after draw");
    restoreScissor();
    return NATIVE_OK;
}

// ============================================================================
// 测试检查清单
// ============================================================================
// 
// [ ] 1. 创建 D:\3d_stage.txt，内容为 0
//        预期：应该看到屏幕中央的2D切片图像
//        如果全黑：纹理数据有问题
// 
// [ ] 2. 修改为 1
//        预期：切片应该在NDC空间正中央
//        如果偏了：投影矩阵有问题
// 
// [ ] 3. 修改为 2
//        预期：切片位置应该根据posZ偏移
//        如果没变化：posZ计算有问题
// 
// [ ] 4. 修改为 3
//        预期：切片大小应该根据zoom变化
//        如果太大/太小：orthoSize计算有问题
// 
// [ ] 5. 修改为 4
//        预期：拖动应该能平移视图
//        如果不动：pan没有传进来
// 
// [ ] 6. 修改为 5
//        预期：旋转应该生效
//        如果不旋转：camRot没有正确传入
// 
// [ ] 7. 修改为 6
//        预期：三张切片都可见，有深度遮挡
//        如果缺少切片：检查纹理上传日志
// 
// [ ] 8. 修改为 7
//        预期：三张半透明切片，不同颜色
//        如果颜色不对：alpha混合设置问题
// 
// [ ] 9. 删除 D:\3d_stage.txt
//        预期：正常渲染（等同于stage 6）
//
// ============================================================================
