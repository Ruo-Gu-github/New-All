#pragma once

#include "../Common/NativeInterfaces.h"

// 裁切形状类型
enum class CropShapeType3D {
    Box = 0,      // 立方体
    Sphere = 1,   // 球体
    Cylinder = 2  // 圆柱体
};

// 圆柱方向
enum class CylinderDirection3D {
    Axial = 0,    // 轴向（沿Z轴）
    Coronal = 1,  // 冠状（沿Y轴）
    Sagittal = 2  // 矢状（沿X轴）
};

/**
 * ImageBrowser专用正交三平面3D渲染器
 * 
 * 功能：
 * 1. 正交切片渲染（Axial/Coronal/Sagittal三个平面）
 * 2. 裁切框显示（立方体/球体/圆柱体三种形状）
 * 3. 鼠标交互（左键平移、右键旋转、滚轮缩放）
 * 4. 比例尺显示
 */
class ImageBrowserOrthogonal3DRenderer {
public:
    /**
     * 渲染3D场景
     */
    static NativeResult Render(
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
    );

private:
    // 绘制立方体裁切框
    static void DrawCropBoxCube(
        float x0, float x1, float y0, float y1, float z0, float z1
    );
    
    // 绘制球体裁切框
    static void DrawCropBoxSphere(
        float midX, float midY, float midZ,
        float radX, float radY, float radZ,
        int segments = 48
    );
    
    // 绘制圆柱体裁切框
    static void DrawCropBoxCylinder(
        float midX, float midY, float midZ,
        float radX, float radY, float radZ,
        int cylDir,
        int segments = 48
    );
    
    // 绘制比例尺（屏幕空间）
    static void DrawScaleBar(
        int winWidth, int winHeight,
        float zoom,
        float spacingX, float spacingY, float spacingZ,
        int volW, int volH, int volD
    );
    
    // 绘制比例尺标签（mm数值）
    static void DrawScaleLabel(float mm, float centerX, float baseY);
    
    // 辅助函数
    static void Mat4_Identity(float out[16]);
    static void Mat4_FromEulerZYXDeg(float out[16], float xDeg, float yDeg, float zDeg);
    static void Rot3(const float m[16], float x, float y, float z, float& ox, float& oy, float& oz);
};
