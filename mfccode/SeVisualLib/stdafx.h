// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // 从 Windows 头中排除极少使用的资料
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // 某些 CString 构造函数将是显式的

#include <afxwin.h>         // MFC 核心组件和标准组件
#include <afxext.h>         // MFC 扩展

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>         // MFC OLE 类
#include <afxodlgs.h>       // MFC OLE 对话框类
#include <afxdisp.h>        // MFC 自动化类
#endif // _AFX_NO_OLE_SUPPORT

#ifndef _AFX_NO_DB_SUPPORT
#include <afxdb.h>                      // MFC ODBC 数据库类
#endif // _AFX_NO_DB_SUPPORT

#ifndef _AFX_NO_DAO_SUPPORT
#include <afxdao.h>                     // MFC DAO 数据库类
#endif // _AFX_NO_DAO_SUPPORT

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC 对 Internet Explorer 4 公共控件的支持
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>                     // MFC 对 Windows 公共控件的支持
#endif // _AFX_NO_AFXCMN_SUPPORT


#include <afxcontrolbars.h>


#define  CUDA_AVAILABLE


//////////////////////////////////////////////////////////////////////////
//
//引入GDI+库
//
//////////////////////////////////////////////////////////////////////////
#ifndef		ULONG_PTR
#	define	ULONG_PTR DWORD
#endif

#ifndef		PPVOID
#	define	PPVOID	  void**
#endif

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform2.hpp>
#include <glm/gtc/type_ptr.hpp>
// #include <GL\gl.h>
// #include <GL\glut.h>
// #include <GL\glaux.h>

#include <gdiplus.h>
using namespace Gdiplus;
#pragma comment(lib,"GdiPlus")
#pragma comment(lib, "Winmm.lib")


#define GLEW_STATIC
#include <GL/glew.h>
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "glew32s.lib")


#include <vector>
#include <algorithm>
#include <map>
#include <fstream>
#include <complex>
#include <windows.h> 
using namespace std;


#define SafeDeleteVec(X)	{					\
	if(X != NULL)	\
{				\
	delete []X;	\
	X = NULL;	\
}				\
}	

#define DIM(X)	(sizeof(X)/sizeof(X[0]))

#ifndef PI
#define PI 3.14159265358979323846
#endif

#undef	AFX_EXT_CLASS
#undef	AFX_EXT_DATA
#undef	AFX_EXT_API
#define  AFX_EXT_CLASS	AFX_CLASS_IMPORT
#define  AFX_EXT_DATA	AFX_DATA_IMPORT
#define  AFX_EXT_API	AFX_API_IMPORT


#define SEV_EXT_CLASS AFX_CLASS_EXPORT

#include "../ImageViewer/ImageViewer.h"
#include "../ImageViewer/VisualInterface.h"
#include "../SeShare/SeShare.h"


#include "SeLibStruct.h"
#define POW(x,y) pow(double(x), double(y))
#define LOG10(x) log10(double(x))
#define SQRT(x)  sqrt(double(x))

#define SafeDelete(X)		{					\
	if(X != NULL)	\
{				\
	delete X;	\
	X = NULL;	\
}				\
}		

#define MAX_CHAR 128

#include <string.h>

#ifdef CUDA_AVAILABLE
#include "cuda_runtime.h"
#include "helper_cuda.h"
#include "device_launch_parameters.h"

#pragma comment(lib, "cudart_static.lib")
#endif // _DEBUG

