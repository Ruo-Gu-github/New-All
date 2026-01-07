#pragma once

#pragma warning(disable : 4244)
#pragma warning(disable : 4786)
#pragma warning(disable : 4018)

#include "stdafx.h"
#include "OpenGLMain.h"
#include "OpenGLDataMgr.h"
#include "OpenGLCamera.h"
#include "OpenGLShader.h"


#ifndef _SKIPSELF_

#ifdef _DEBUG
#pragma comment(lib,"Se3DModuleD.Lib")
#pragma message("Automatically linking with Se3DModuleD.dll")
#else
#pragma comment(lib,"Se3DModule.Lib")
#pragma message("Automatically linking with Se3DModule.dll")
#endif // _DEBUG

#endif	//_SKIPSELF_