// Minimal declaration for ROI orthogonal 3D renderer
#pragma once

#include "../Common/NativeInterfaces.h"

class RoiOrthogonal3DRenderer {
public:
	// Render tri-planar ROI view using three APR handles and camera state.
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
		float viewPanY);
};
