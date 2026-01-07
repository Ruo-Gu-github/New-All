#pragma once

#pragma warning(disable : 4244)
#pragma warning(disable : 4786)
#pragma warning(disable : 4018)

#include "../SeVisualLib/SeLibStruct.h"
#include "../SeVisualLib/SeVisualAPR.h"
#include "../SeVisualLib/SeVisualMPR.h"
//#include "../SeVisualLib/SeVisualCPR.h"
#include "../SeVisualLib/SeVisualRotate.h"
#include "../SeVisualLib/SeVisualAPR_with_CUDA.h"
//#include "../SeVisualLib/SeVisualV3D.h"
//#include "../SeVisualLib/SeVisualMPRView.h"
//#include "../SeVisualLib/SeVisualAPRView.h"
#include "../SeVisualLib/SeVisualLibDoc.h"
//#include "../SeVisualLib/LibSwapData.h"
//#include "../SeVisualLib/SeVisualMouseTool.h"
//#include "../SeVisualLib/SeVisualRotateCubeView.h"


#ifndef _SKIPSELF_

#ifdef _DEBUG
#pragma comment(lib,"SeVisualLibD.Lib")
#pragma message("Automatically linking with SeVisualLibD.dll")
#else
#pragma comment(lib,"SeVisualLib.Lib")
#pragma message("Automatically linking with SeVisualLib.dll")
#endif // _DEBUG

#endif	//_SKIPSELF_
