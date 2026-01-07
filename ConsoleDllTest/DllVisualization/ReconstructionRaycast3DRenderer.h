// Minimal declaration for volume raycast renderer
#pragma once

#include "../Common/NativeInterfaces.h"

struct WindowContext;

class ReconstructionRaycast3DRenderer {
public:
	// Stub renderer entry point; real implementation can be added later.
	static NativeResult Render(WindowContext* ctx);
};
