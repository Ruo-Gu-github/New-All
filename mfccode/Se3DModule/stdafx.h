// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件

#pragma once

#ifndef _SECURE_ATL
#define _SECURE_ATL 1
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // 从 Windows 头中排除极少使用的资料
#endif

#pragma warning(disable: 4005)
#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // 某些 CString 构造函数将是显式的

// 关闭 MFC 对某些常见但经常可放心忽略的警告消息的隐藏
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC 核心组件和标准组件
#include <afxext.h>         // MFC 扩展

#include <afxdisp.h>        // MFC 自动化类

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC 对 Internet Explorer 4 公共控件的支持
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>                     // MFC 对 Windows 公共控件的支持
#endif // _AFX_NO_AFXCMN_SUPPORT


// need this for using SeShareD.dll
#include <afxcontrolbars.h>     // 功能区和控件条的 MFC 支持

#include <Gdiplus.h>
using namespace Gdiplus;
#pragma comment(lib,"Gdiplus")

#pragma comment(lib, "Winmm.lib")

// need use MBCS(多字节字符集) for using ImageViewerD.dll
#include "../ImageViewer/ImageViewer.h"
#include "../SeShare/SeShare.h"
 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform2.hpp>
#include <glm/gtc/type_ptr.hpp>

enum ResetDirection{XDIRECTION, YDIRECTION, ZDIRECTION};
enum Speed{MOVE, ROTATE, SCALE};


// need ignore specific library libcmt.lib;libcpmt.lib
#define GLEW_STATIC
#include <GL/glew.h>
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "glew32s.lib")

#pragma comment(lib, "freetyped.lib")
#include <ft2build.h>
#include FT_FREETYPE_H

#include <map>
using namespace std;

#include <soil/SOIL.h>
// image load library
#pragma comment(lib, "SOIL.lib")
