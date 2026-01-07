#include "pch.h"

#include "RoiOrthogonal3DRenderer.h"
#include "VisualizationApi.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include <GL/glew.h>

namespace {
struct TriPlanarTextures {
    GLuint texAxial = 0;
    GLuint texCoronal = 0;
    GLuint texSagittal = 0;
    int wAx = 0, hAx = 0;
    int wCo = 0, hCo = 0;
    int wSa = 0, hSa = 0;
};

static void EnsureTex(GLuint& texId) {
    if (!texId) glGenTextures(1, &texId);
}

static void UploadGray8(GLuint texId, int w, int h, const void* data) {
    if (!texId || w <= 0 || h <= 0 || !data) return;
    glBindTexture(GL_TEXTURE_2D, texId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w, h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
}

static void Mat4_IdentityLocal(float out[16]) {
    static const float I[16] = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };
    std::memcpy(out, I, sizeof(float) * 16);
}

static void Mat4_FromEulerZYXDeg(float out[16], float xDeg, float yDeg, float zDeg) {
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

static void Rot3(const float m[16], float x, float y, float z, float& ox, float& oy, float& oz) {
    ox = m[0] * x + m[4] * y + m[8] * z;
    oy = m[1] * x + m[5] * y + m[9] * z;
    oz = m[2] * x + m[6] * y + m[10] * z;
}

static void GetViewRotMat(const float* inMat, bool inited, float outMat[16]) {
    if (inited && inMat) {
        std::memcpy(outMat, inMat, sizeof(float) * 16);
    } else {
        Mat4_IdentityLocal(outMat);
    }
}

static void GetVolumeDimsFromSlices(int axW, int axH, int coW, int coH, int saW, int saH, int& outW, int& outH, int& outD) {
    outW = std::max(1, axW);
    outH = std::max(1, axH);
    outD = std::max(1, std::max(coH, saH));
}
} // namespace

NativeResult RoiOrthogonal3DRenderer::Render(
    APRHandle axial,
    APRHandle coronal,
    APRHandle sagittal,
    int winWidth,
    int winHeight,
    const float viewRotMat[16],
    bool viewRotMatInitialized,
    float viewZoom,
    float viewPanX,
    float viewPanY
) {
    printf("[RoiOrthogonal3DRenderer::Render] CALLED: axial=%p coronal=%p sagittal=%p win=%dx%d\n", axial, coronal, sagittal, winWidth, winHeight);
    fflush(stdout);
    if (!axial || !coronal || !sagittal) return NATIVE_E_INVALID_ARGUMENT;
    if (winWidth <= 0 || winHeight <= 0) return NATIVE_OK;

    // ROI 3D is intentionally independent from crop box state.
    {
        const bool prevEnabled = APR_IsCropBoxEnabled();
        APR_EnableCropBox(false);
        APR_Render(axial);
        APR_Render(coronal);
        APR_Render(sagittal);
        APR_EnableCropBox(prevEnabled);
    }

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

    float centerX = 0.0f, centerY = 0.0f, centerZ = 0.0f;
    APR_GetCenter(axial, &centerX, &centerY, &centerZ);

    float rotVol[16];
    {
        float rx = 0.0f, ry = 0.0f, rz = 0.0f;
        APR_GetRotation(axial, &rx, &ry, &rz);
        Mat4_FromEulerZYXDeg(rotVol, rx, ry, rz);
    }

    float camRot[16];
    GetViewRotMat(viewRotMat, viewRotMatInitialized, camRot);

    // Upload slices into textures owned by the *current* GL context.
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

    // ROI renderer always uploads CPU slices and never tries to sample shared textures.
    if (axSlice && axW > 0 && axH > 0) UploadGray8(tri.texAxial, axW, axH, axSlice);
    if (coSlice && coW > 0 && coH > 0) UploadGray8(tri.texCoronal, coW, coH, coSlice);
    if (saSlice && saW > 0 && saH > 0) UploadGray8(tri.texSagittal, saW, saH, saSlice);

    glViewport(0, 0, winWidth, winHeight);
    // CYAN background to identify RoiOrthogonal3DRenderer path
    glClearColor(0.0f, 0.9f, 0.9f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    printf("[RoiOrthogonal3DRenderer] Cleared with CYAN background\n");
    fflush(stdout);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = (winHeight > 0) ? (float)winWidth / (float)winHeight : 1.0f;
    glFrustum(-aspect * 0.1f, aspect * 0.1f, -0.1f, 0.1f, 0.2f, 10.0f);

    const int maxDim = std::max(volW, std::max(volH, volD));
    const float normW = (float)volW / (float)maxDim;
    const float normH = (float)volH / (float)maxDim;
    const float normD = (float)volD / (float)maxDim;

    const float denomW = (float)std::max(1, volW - 1);
    const float denomH = (float)std::max(1, volH - 1);
    const float denomD = (float)std::max(1, volD - 1);

    const float normCenterX = (centerX / denomW) - 0.5f;
    const float normCenterY = (centerY / denomH) - 0.5f;
    const float normCenterZ = (centerZ / denomD) - 0.5f;

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    const float zoom = (viewZoom <= 0.0001f) ? 0.0001f : viewZoom;
    glTranslatef(viewPanX, viewPanY, -3.0f / zoom);
    glMultMatrixf(camRot);

    const float posX = normCenterX * normW;
    const float posY = normCenterY * normH;
    const float posZ = normCenterZ * normD;

    // ROI renderer draws planes with a subtle outline emphasis.
    auto drawQuad = [](GLuint tex, float v0x,float v0y,float v0z,float v1x,float v1y,float v1z,float v2x,float v2y,float v2z,float v3x,float v3y,float v3z) {
        glBindTexture(GL_TEXTURE_2D, tex);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(v0x, v0y, v0z);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(v1x, v1y, v1z);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(v2x, v2y, v2z);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(v3x, v3y, v3z);
        glEnd();

        glDisable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(1.0f, 1.0f, 1.0f, 0.25f);
        glLineWidth(2.0f);
        glBegin(GL_LINE_LOOP);
        glVertex3f(v0x, v0y, v0z);
        glVertex3f(v1x, v1y, v1z);
        glVertex3f(v2x, v2y, v2z);
        glVertex3f(v3x, v3y, v3z);
        glEnd();
        glDisable(GL_BLEND);
        glColor4f(1,1,1,1);
        glEnable(GL_TEXTURE_2D);
    };

    if (tri.texAxial && axSlice) {
        float v0x, v0y, v0z, v1x, v1y, v1z, v2x, v2y, v2z, v3x, v3y, v3z;
        Rot3(rotVol, -normW / 2, -normH / 2, posZ, v0x, v0y, v0z);
        Rot3(rotVol, -normW / 2,  normH / 2, posZ, v1x, v1y, v1z);
        Rot3(rotVol,  normW / 2,  normH / 2, posZ, v2x, v2y, v2z);
        Rot3(rotVol,  normW / 2, -normH / 2, posZ, v3x, v3y, v3z);
        drawQuad(tri.texAxial, v0x,v0y,v0z, v1x,v1y,v1z, v2x,v2y,v2z, v3x,v3y,v3z);
    }

    if (tri.texCoronal && coSlice) {
        float v0x, v0y, v0z, v1x, v1y, v1z, v2x, v2y, v2z, v3x, v3y, v3z;
        Rot3(rotVol, -normW / 2, posY, -normD / 2, v0x, v0y, v0z);
        Rot3(rotVol,  normW / 2, posY, -normD / 2, v1x, v1y, v1z);
        Rot3(rotVol,  normW / 2, posY,  normD / 2, v2x, v2y, v2z);
        Rot3(rotVol, -normW / 2, posY,  normD / 2, v3x, v3y, v3z);
        drawQuad(tri.texCoronal, v0x,v0y,v0z, v1x,v1y,v1z, v2x,v2y,v2z, v3x,v3y,v3z);
    }

    if (tri.texSagittal && saSlice) {
        float v0x, v0y, v0z, v1x, v1y, v1z, v2x, v2y, v2z, v3x, v3y, v3z;
        Rot3(rotVol, posX, -normH / 2, -normD / 2, v0x, v0y, v0z);
        Rot3(rotVol, posX,  normH / 2, -normD / 2, v1x, v1y, v1z);
        Rot3(rotVol, posX,  normH / 2,  normD / 2, v2x, v2y, v2z);
        Rot3(rotVol, posX, -normH / 2,  normD / 2, v3x, v3y, v3z);
        drawQuad(tri.texSagittal, v0x,v0y,v0z, v1x,v1y,v1z, v2x,v2y,v2z, v3x,v3y,v3z);
    }

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    return NATIVE_OK;
}
