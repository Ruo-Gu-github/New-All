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

////////////////////////////////////////////////////////////////////////////////////////
#include <afxcontrolbars.h>


#define CUDA_AVAILABLE

#include "resource.h"

#undef	AFX_EXT_CLASS
#undef	AFX_EXT_DATA
#undef	AFX_EXT_API
#undef	SEV_EXT_CLASS
#define  AFX_EXT_CLASS	AFX_CLASS_IMPORT
#define  AFX_EXT_DATA	AFX_DATA_IMPORT
#define  AFX_EXT_API	AFX_API_IMPORT

#define SEV_EXT_CLASS AFX_CLASS_IMPORT


#include <queue>
using namespace std;

#include <GL\glew.h>

#include <gdiplus.h>
using namespace Gdiplus;
#pragma comment(lib,"GdiPlus")

#include "../ImageViewer/ImageViewer.h"
#include "../ImageViewer/VisualInterface.h"
#include "../SeShare/SeShare.h"
#include "../Se3DModule/Se3DModule.h"

#include "../SeVisualLib/SeVisual.h"
#include "../SeProcessPro/ProcessInterface.h"
#pragma comment(lib, "Winmm.lib")

////////////////////////////////////////////////////////////////////
enum BONEPROCESSSTATE {SP_SELECTZ = 0,SP_ROTATEDATA, SP_CALCU, SP_BINARY};
enum MOUSESTATE {CUR_IN = 0, CUR_ON, CUR_OUT}; 
enum MOTIFY_OPERATION {MOTIFY_SLICE = 0, MOTIFY_ROI};

#define	 MT_BONEZROI	MT_Ext+41
#define	 MT_STDDATA		MT_Ext+42
#define	 MT_APRTOOL		MT_Ext+43
#define  MT_APRRECT		MT_Ext+44

// 自订消息
#define WM_EXPORT_EXCEL				WM_USER + 130
#define WM_MASK						WM_USER + 131
#define WM_PRINT_3D                 WM_USER + 132
#define WM_BONE_FUNCLIST			WM_USER + 133
#define WM_BONE_FUNCRESET			WM_USER + 134
#define WM_BONE_SHOWTWOPARTS		WM_USER + 135
#define WM_BONE_FUNCFINISHED		WM_USER + 136
#define WM_BONE_CALCHANGE			WM_USER + 137
#define WM_BONE_FUNCSHOWMASKSET		WM_USER + 138

#define THREAD_NUMBER 4