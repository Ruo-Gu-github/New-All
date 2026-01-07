// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件

#pragma once

#define  _USE_MATH_DEFINES

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

//////////////////////////////////////////////////////////////////////////////////////////////////
#include <afxcontrolbars.h>

#include <intsafe.h>

#undef	AFX_EXT_CLASS
#undef	AFX_EXT_DATA
#undef	AFX_EXT_API
#undef	SEV_EXT_CLASS
#define  AFX_EXT_CLASS	AFX_CLASS_IMPORT
#define  AFX_EXT_DATA	AFX_DATA_IMPORT
#define  AFX_EXT_API	AFX_API_IMPORT

#define SEV_EXT_CLASS AFX_CLASS_IMPORT


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

#include <queue>
using namespace std;

#define THREAD_NUMBER 4

#define PI	3.1415926

#define MT_ROI      MT_Ext+58
#define MT_REGROI	MT_Ext+59
#define MT_MPR		MT_Ext+60
#define MT_APR		MT_Ext+61
#define MT_RECTROI	MT_Ext+62
#define MT_FLOODFILL  MT_Ext+63
#define MT_MEASURE_LINE MT_Ext+64
#define MT_MEASURE_ANGLE MT_Ext+65
#define MT_MEASURE_SHAPE MT_Ext+66
#define MT_MEASURE_CT MT_Ext+67
#define MT_MEASURE_AREA MT_Ext+68
#define MT_MEASURE_SELECT MT_Ext+69
#define MT_MEASURE_ELLIPSE MT_Ext+70


// 自定义消息
#define WM_MASK						WM_USER + 111
#define WM_MASK_ITEM				WM_USER + 112 
#define WM_MASK_ITEM_FALSE			WM_USER + 113 
#define WM_MORPHYOLOGY_OPERATION	WM_USER + 114
#define WM_BOOLEAN_OPERATION		WM_USER + 115
#define WM_CHANGE_PLANE_NUMBER		WM_USER + 116
#define WM_DELETE_ROI				WM_USER + 117
#define WM_MID_LAYER				WM_USER + 118
#define WM_EXECUTE_ROI				WM_USER + 119
#define WM_SET_MOUSE_TOOL    		WM_USER + 120
#define WM_SET_3D_PERFORMANCE       WM_USER + 121
#define WM_3D_STATE                 WM_USER + 122
#define WM_TRANSFUNC                WM_USER + 123
#define WM_FREECUT                  WM_USER + 124
#define WM_ClOSE_FCWND              WM_USER + 125
#define WM_TRANSLATE                WM_USER + 126
#define WM_CLOSE_ALL_WINDOW         WM_USER + 127
#define WM_UPDATE_RESULT            WM_USER + 128
#define WM_ADD_RESULT				WM_USER + 129
#define WM_REFRESH_RESULT			WM_USER + 130
#define WM_ROI_SHAPE_CHANGED        WM_USER + 131
#define WM_SET_3D_LIGHT		        WM_USER + 132
#define WM_ROTATE_ACTION		    WM_USER + 133
#define WM_SCALE_ACTION		        WM_USER + 134
#define WM_CUT_ACTION		        WM_USER + 135

enum MOUSESTATE {CUR_IN = 0, CUR_ON, CUR_OUT};   
//enum SHOWSTATE {SP_ALL = 0, SP_FRONT, SP_TOP, SP_LEFT, SP_3D};
enum ROI_OPERATION {ROI_ADD = 0, ROI_REMOVE, ROI_INTERSET};
enum ROI_SHAPE {ROI_SHAPE_ANY = 0, ROI_SHAPE_RECT, ROI_SHAPE_ROUND};
enum BOOLEAN_OPERATION {BOOL_UNION = 0, BOOL_XOR, BOOL_INTEREST};
enum MORPHOLOGY_OPERATION {MORPHOLOGY_CORROSION = 0, MORPGOLOGY_DELITE, MORPHOLOGY_CLOSE, MORPHOLOGY_OPEN, MORPHOLOGY_FLOODFILL, MORPHOLOGY_INVERSE, MORPHOLOGY_QUIT_FLOODFILL, MORPHOLOGY_EXECUTE_FLOODFILL, MEASURE_SELECT, MEASURE_LINE, MEASURE_ANGLE, MEASURE_SHAPE, MEASURE_CT, MEASURE_AREA};
enum SHOW_STATE {SHOW_ALL=0, SHOW_TOP, SHOW_LEFT, SHOW_FRONT, SHOW_3D};
enum SPEED_TYPE {MOVE_SPEED = 0, ROTATE_SPEED, SCALE_SPEED, QUALITY};

struct POSITION_3D
{
	float fXpos;
	float fYpos;
	float fZpos;
	POSITION_3D(float x, float y, float z)
	{
		fXpos = x;
		fYpos = y;
		fZpos = z;
	}
	POSITION_3D()
	{
		fXpos = 0.0f;
		fYpos = 0.0f;
		fZpos = 0.0f;
	}
};

struct VOLUME_INFO
{
	int nMoveSpeed;
	int nRotateSpeed;
	int nScaleSpeed;
	int nQuality;
	double dViewAngle;

	// POSITION_3D ViewPosition;
	POSITION_3D RotateCenterPosition;

	COLORREF    color;
};

struct LIGHT_INFO
{
		POSITION_3D lightPosition;
		COLORREF	LightColor;
		COLORREF    MaterialColor;
		float       emission;
		float       diffuse;
		float       reflect;
		float       specular;
		float       shadowD;
		bool        light;
		bool        shadow;	
};

struct ACTION_INFO
{
	CString type;
	int xpos;
	int ypos;
	int zpos;
	float fxpos;
	float fypos;
	float fzpos;
	double scale;
	ACTION_INFO()
	{
		type = "";
		xpos = 0;
		ypos = 0;
		zpos = 0;
		fxpos = 0.5;
		fypos = 0.5;
		fzpos = 0.5;
		scale = 1.0;
	}
};