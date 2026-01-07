// SeGeneralModuleCtrlDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "afxdialogex.h"
#include "GeneralSwapData.h"
#include "SeGeneralModuleCtrlDlg.h"
#include "SeGeneralModuleDlg.h"
#include "SeGeneralModule.h"
#include "SeMPRView.h"
#include "Se3DView.h"
#include "SeNewMaskDlg.h"
#include "SeNewVolumeObjectDlg.h"
#include "SeMorphologyDlg.h"
#include "SeBooleanDlg.h"
#include "SeROIEditDlg.h"
#include "Se3DPropertityDlg.h"
#include "SeROIData.h"
#include "SeTransFuncDlg.h"
#include "SeFreeCutDlg.h"
#include "SeTranslateDlg.h"
#include "SeResultDlg.h"
#include "SeLightDlg.h"
#include "SeMovieDlg.h"
#include <random>


// 添加MessageBoxTimeout支持
extern "C"
{
	int WINAPI MessageBoxTimeoutA(IN HWND hWnd, IN LPCSTR lpText, IN LPCSTR lpCaption, IN UINT uType, IN WORD wLanguageId, IN DWORD dwMilliseconds);
	int WINAPI MessageBoxTimeoutW(IN HWND hWnd, IN LPCWSTR lpText, IN LPCWSTR lpCaption, IN UINT uType, IN WORD wLanguageId, IN DWORD dwMilliseconds);
};
#ifdef UNICODE
#define MessageBoxTimeout MessageBoxTimeoutW
#else
#define MessageBoxTimeout MessageBoxTimeoutA
#endif

IMPLEMENT_DYNAMIC(SeGeneralModuleCtrlDlg, CSeDialogBase)

SeGeneralModuleCtrlDlg::SeGeneralModuleCtrlDlg(CWnd* pParent /*=NULL*/)

	: CSeDialogBase(SeGeneralModuleCtrlDlg::IDD, pParent)
{
	m_bBorder = FALSE;
	m_bSetValue = FALSE;
	m_bFloodFill = FALSE;
	m_bRayCasting = TRUE;
	m_nColorIndex = 0;
	m_ColorLst[0] = RGB(0, 255, 0);
	m_ColorLst[1] = RGB(255, 0, 0);
	m_ColorLst[2] = RGB(0, 0, 255);
	m_ColorLst[3] = RGB(255, 255, 0);
	m_ColorLst[4] = RGB(255, 0, 255);
	m_ColorLst[5] = RGB(0, 255, 255);
	m_ColorLst[6] = RGB(128, 0, 255);
	m_ColorLst[7] = RGB(255, 128, 0);

	m_pMorPhologyDlg = NULL;
	m_pROIEditDlg = NULL;
	m_pBooleanDlg = NULL;
	m_p3DProrertityDlg = NULL;
	m_pTransFuncDlg = NULL;
	m_pFreeCutDlg = NULL;
	m_pResultDlg = NULL;
	m_pLightDlg = NULL;
	m_pMovieDlg = NULL;
	m_nPlaneNumNow = -1;

	m_fFreeCutRange[0] = 0.0f;
	m_fFreeCutRange[1] = 0.0f;
	m_fFreeCutRange[2] = 0.0f;
	m_fFreeCutRange[3] = 0.0f;
	m_fFreeCutRange[4] = 0.0f;
	m_fFreeCutRange[5] = 0.0f;

	m_bMaskTool = FALSE;
	m_nMipThickness = 1;
	m_nMinIpThickness = 1;

}

SeGeneralModuleCtrlDlg::~SeGeneralModuleCtrlDlg()
{
	Safe_Delete(m_pMorPhologyDlg);
	Safe_Delete(m_pBooleanDlg);
	Safe_Delete(m_pROIEditDlg);
	Safe_Delete(m_p3DProrertityDlg);
	Safe_Delete(m_pTransFuncDlg);
	Safe_Delete(m_pFreeCutDlg);
	Safe_Delete(m_pResultDlg);
	Safe_Delete(m_pLightDlg);
	Safe_Delete(m_pMovieDlg);
}

void SeGeneralModuleCtrlDlg::DoDataExchange(CDataExchange* pDX)
{
	CSeDialogBase::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_GROUP1, m_Group);
	DDX_Control(pDX, IDC_LIST_3D_PLANE, m_lst3DPlane);
	DDX_Control(pDX, IDC_LIST_2D_PLANE, m_lst2DPlane);
	DDX_Control(pDX, IDC_2D_PLANE, m_Group2DPlane);
	DDX_Control(pDX, IDC_3D_PLANE, m_Group3DPlane);
	DDX_Control(pDX, IDC_3D_PLANE2, m_Group3DPlane2);
	DDX_Control(pDX, IDC_TEMPLATE, m_templateCtl);
	DDX_Control(pDX, IDC_SLIDER_STEP_SCALE, m_sliderStepScale);
	DDX_Control(pDX, IDC_SLIDER_OFFSET_SCALE, m_sliderOffsetScale);
	DDX_Control(pDX, IDC_LIST_3D_PLANE2, m_lst3DPlane2);
}


BEGIN_MESSAGE_MAP(SeGeneralModuleCtrlDlg, CSeDialogBase)
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_TIMER()
	ON_WM_CTLCOLOR()
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_3D_CONTROL, &SeGeneralModuleCtrlDlg::OnBnClicked3dControl)
	ON_BN_CLICKED(IDC_BUTTON_2D_ADD, &SeGeneralModuleCtrlDlg::OnBnClickedButton2dAdd)
	ON_BN_CLICKED(IDC_BUTTON_2D_REMOVE, &SeGeneralModuleCtrlDlg::OnBnClickedButton2dRemove)
	ON_BN_CLICKED(IDC_BUTTON_2D_COLOR, &SeGeneralModuleCtrlDlg::OnBnClickedButton2dColor)
	ON_BN_CLICKED(IDC_BUTTON_3D_ADD, &SeGeneralModuleCtrlDlg::OnBnClickedButton3dAdd)
	ON_BN_CLICKED(IDC_BUTTON_3D_REMOVE, &SeGeneralModuleCtrlDlg::OnBnClickedButton3dRemove)
	ON_BN_CLICKED(IDC_BUTTON_3D_COLOR, &SeGeneralModuleCtrlDlg::OnBnClickedButton3dColor)
	ON_BN_CLICKED(IDC_MORPHOLOGY, &SeGeneralModuleCtrlDlg::OnBnClickedMorphology)
	ON_BN_CLICKED(IDC_ROI_EDIT, &SeGeneralModuleCtrlDlg::OnBnClickedRoiEdit)
	ON_BN_CLICKED(IDC_BOOLEAN, &SeGeneralModuleCtrlDlg::OnBnClickedBoolean)
	ON_BN_CLICKED(IDC_PRINTSCREEN_3D, &SeGeneralModuleCtrlDlg::OnBnClickedPrintscreen3d)
	ON_BN_CLICKED(IDC_RENDER_MODE, &SeGeneralModuleCtrlDlg::OnBnClickedRenderMode)
	ON_BN_CLICKED(IDC_TRANS_FUNC, &SeGeneralModuleCtrlDlg::OnBnClickedTransFunc)
	ON_BN_CLICKED(IDC_BUTTON_2D_INFO, &SeGeneralModuleCtrlDlg::OnBnClickedButton2dInfo)
	ON_BN_CLICKED(IDC_FREE_CUT, &SeGeneralModuleCtrlDlg::OnBnClickedFreeCut)
	ON_BN_CLICKED(IDC_EXPORT_FROM_MASK, &SeGeneralModuleCtrlDlg::OnBnClickedExportFromMask)
	ON_BN_CLICKED(IDC_BUTTON_3D_TRANSLATE, &SeGeneralModuleCtrlDlg::OnBnClickedButton3dTranslate)

	// 自定消息
	ON_MESSAGE(WM_MASK, &SeGeneralModuleCtrlDlg::OnAddMask) 
	ON_MESSAGE(WM_MASK_ITEM, &SeGeneralModuleCtrlDlg::OnAddMaskItem)
	ON_MESSAGE(WM_MASK_ITEM_FALSE, &SeGeneralModuleCtrlDlg::OnAddMaskItemFalse)
	ON_MESSAGE(WM_MORPHYOLOGY_OPERATION, &SeGeneralModuleCtrlDlg::OnMorphologyOperation)
	ON_MESSAGE(WM_BOOLEAN_OPERATION, &SeGeneralModuleCtrlDlg::OnBooleanOperation)
	ON_MESSAGE(WM_CHANGE_PLANE_NUMBER, &SeGeneralModuleCtrlDlg::OnChangeROIEditPlane)
	ON_MESSAGE(WM_ROI_SHAPE_CHANGED, &SeGeneralModuleCtrlDlg::OnChangeROIShape)
	ON_MESSAGE(WM_MID_LAYER, &SeGeneralModuleCtrlDlg::OnMidLayer)
	ON_MESSAGE(WM_DELETE_ROI, &SeGeneralModuleCtrlDlg::OnDeleteROI)
	ON_MESSAGE(WM_EXECUTE_ROI, &SeGeneralModuleCtrlDlg::OnExecuteROI)
	ON_MESSAGE(WM_SET_MOUSE_TOOL, &SeGeneralModuleCtrlDlg::OnSetMouseTool)
	ON_MESSAGE(WM_SET_3D_PERFORMANCE, &SeGeneralModuleCtrlDlg::OnSet3DState)
	ON_MESSAGE(WM_TRANSFUNC, &SeGeneralModuleCtrlDlg::OnSetTransFunc)
	ON_MESSAGE(WM_FREECUT, &SeGeneralModuleCtrlDlg::OnAdjustFreeCut)
	ON_MESSAGE(WM_ClOSE_FCWND, &SeGeneralModuleCtrlDlg::OnCloseFreeCutWindow)
	ON_MESSAGE(WM_TRANSLATE, &SeGeneralModuleCtrlDlg::OnChangeTranslate)
	ON_MESSAGE(WM_CLOSE_ALL_WINDOW, &SeGeneralModuleCtrlDlg::OnCloseAllWindow)
	ON_MESSAGE(WM_SET_3D_LIGHT, &SeGeneralModuleCtrlDlg::OnSetLightState)
	ON_MESSAGE(WM_ROTATE_ACTION, &SeGeneralModuleCtrlDlg::OnRotateAction)
	ON_MESSAGE(WM_SCALE_ACTION, &SeGeneralModuleCtrlDlg::OnScaleAction)
	ON_MESSAGE(WM_CUT_ACTION, &SeGeneralModuleCtrlDlg::OnCutAction)
	

	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST_2D_PLANE, &SeGeneralModuleCtrlDlg::OnNMCustomdrawList2dPlane)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST_3D_PLANE, &SeGeneralModuleCtrlDlg::OnNMCustomdrawList3dPlane)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST_3D_PLANE2, &SeGeneralModuleCtrlDlg::OnNMCustomdrawList3dPlane2)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_2D_PLANE, &SeGeneralModuleCtrlDlg::OnNMDblclkList2dPlane)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_3D_PLANE, &SeGeneralModuleCtrlDlg::OnNMDblclkList3dPlane)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_3D_PLANE2, &SeGeneralModuleCtrlDlg::OnNMDblclkList3dPlane2)
	ON_NOTIFY(NM_CLICK, IDC_LIST_2D_PLANE, &SeGeneralModuleCtrlDlg::OnNMClickList2dPlane)
	ON_NOTIFY(NM_CLICK, IDC_LIST_3D_PLANE, &SeGeneralModuleCtrlDlg::OnNMClickList3dPlane)
	ON_NOTIFY(NM_CLICK, IDC_LIST_3D_PLANE2, &SeGeneralModuleCtrlDlg::OnNMClickList3dPlane2)
	
	ON_BN_CLICKED(IDC_RANDOM_PICK, &SeGeneralModuleCtrlDlg::OnBnClickedRandomPick)
	ON_BN_CLICKED(IDC_3D_OUTPUT, &SeGeneralModuleCtrlDlg::OnBnClicked3dOutput)
	ON_CBN_SELCHANGE(IDC_TEMPLATE, &SeGeneralModuleCtrlDlg::OnCbnSelchangeTemplate)
	ON_BN_CLICKED(IDC_TOOL_LINE, &SeGeneralModuleCtrlDlg::OnBnClickedToolLine)
	ON_BN_CLICKED(IDC_TOOL_ANGLE, &SeGeneralModuleCtrlDlg::OnBnClickedToolAngle)
	ON_BN_CLICKED(IDC_TOOL_SHAPE, &SeGeneralModuleCtrlDlg::OnBnClickedToolShape)
	ON_BN_CLICKED(IDC_TOOL_VALUE, &SeGeneralModuleCtrlDlg::OnBnClickedToolValue)
	ON_BN_CLICKED(IDC_TOOL_AREA, &SeGeneralModuleCtrlDlg::OnBnClickedToolArea) 
	ON_BN_CLICKED(IDC_TOOL_SELECT, &SeGeneralModuleCtrlDlg::OnBnClickedToolSelect)
	ON_BN_CLICKED(IDC_EXPORT_WITH_MASK, &SeGeneralModuleCtrlDlg::OnBnClickedExportWithMask)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER_STEP_SCALE, &SeGeneralModuleCtrlDlg::OnNMCustomdrawSliderStepScale)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER_OFFSET_SCALE, &SeGeneralModuleCtrlDlg::OnNMCustomdrawSliderOffsetScale)
	ON_BN_CLICKED(IDC_3D_LIGHT, &SeGeneralModuleCtrlDlg::OnBnClicked3dLight)
	ON_BN_CLICKED(IDC_EXPORT_SHARP, &SeGeneralModuleCtrlDlg::OnBnClickedExportSharp)
	ON_BN_CLICKED(IDC_BUTTON_2D_SAVE, &SeGeneralModuleCtrlDlg::OnBnClickedButton2dSave)
	ON_BN_CLICKED(IDC_BUTTON_2D_LOAD, &SeGeneralModuleCtrlDlg::OnBnClickedButton2dLoad)
	ON_BN_CLICKED(IDC_3D_LINE, &SeGeneralModuleCtrlDlg::OnBnClicked3dLine)
	ON_BN_CLICKED(IDC_TOOL_SCALE, &SeGeneralModuleCtrlDlg::OnBnClickedToolScale)
	ON_BN_CLICKED(IDC_TOOL_RESET, &SeGeneralModuleCtrlDlg::OnBnClickedToolReset)
	ON_BN_CLICKED(IDC_MOVIE_RECORD, &SeGeneralModuleCtrlDlg::OnBnClickedMovieRecord)
	ON_BN_CLICKED(IDC_BUTTON_SAVE_MASK, &SeGeneralModuleCtrlDlg::OnBnClickedButtonSaveMask)
	ON_BN_CLICKED(IDC_BUTTON_LOAD_MASK, &SeGeneralModuleCtrlDlg::OnBnClickedButtonLoadMask)
	ON_BN_CLICKED(IDC_BUTTON_3D_ADD2, &SeGeneralModuleCtrlDlg::OnBnClickedButton3dAdd2)
	ON_BN_CLICKED(IDC_BUTTON_3D_REMOVE2, &SeGeneralModuleCtrlDlg::OnBnClickedButton3dRemove2)
	ON_BN_CLICKED(IDC_BUTTON_3D_COLOR2, &SeGeneralModuleCtrlDlg::OnBnClickedButton3dColor2)
	ON_BN_CLICKED(IDC_BUTTON_3D_TRANSLATE2, &SeGeneralModuleCtrlDlg::OnBnClickedButton3dTranslate2)
	ON_BN_CLICKED(IDC_TOOL_ELLIPSE, &SeGeneralModuleCtrlDlg::OnBnClickedToolEllipse)
END_MESSAGE_MAP()
// SeGeneralModuleCtrlDlg 消息处理程序


BOOL SeGeneralModuleCtrlDlg::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CRect rect;
	GetClientRect(&rect);
	CZKMemDC memDC(pDC);
	memDC.FillSolidRect(rect,RGB(106,112,128));

	return TRUE;
//	return CSeDialogBase::OnEraseBkgnd(pDC);
}

void SeGeneralModuleCtrlDlg::OnSize(UINT nType, int cx, int cy)
{
	CSeDialogBase::OnSize(nType, cx, cy);

	CRect	rect;
	CRect	rtHide(0,0,0,0);
	GetClientRect(&rect);
	CWnd* pWnd = GetDlgItem(IDC_2D_PLANE);
	if(pWnd)
	{
		m_Group.MoveWindow(CRect(500,1000,900,1400));	
		m_Group3DPlane.MoveWindow(CRect(1750, 1000, 2100, 1400));
		m_Group2DPlane.MoveWindow(CRect(2150, 1000, 2500, 1500));
		m_Group3DPlane2.MoveWindow(CRect(1350, 1000, 1700, 1400));

		GetDlgItem(IDC_RENDER_MODE)->MoveWindow(rtHide);
		GetDlgItem(IDC_MORPHOLOGY)->MoveWindow(rtHide);
		GetDlgItem(IDC_BOOLEAN)->MoveWindow(rtHide);
		GetDlgItem(IDC_ROI_EDIT)->MoveWindow(rtHide);
		GetDlgItem(IDC_PRINTSCREEN_3D)->MoveWindow(rtHide);
		GetDlgItem(IDC_3D_CONTROL)->MoveWindow(rtHide);
		GetDlgItem(IDC_TRANS_FUNC)->MoveWindow(rtHide);
		GetDlgItem(IDC_FREE_CUT)->MoveWindow(rtHide);
		GetDlgItem(IDC_EXPORT_FROM_MASK)->MoveWindow(rtHide);
		GetDlgItem(IDC_RANDOM_PICK)->MoveWindow(rtHide);
		GetDlgItem(IDC_3D_OUTPUT)->MoveWindow(rtHide);
		GetDlgItem(IDC_TEMPLATE)->MoveWindow(rtHide);
		GetDlgItem(IDC_TOOL_LINE)->MoveWindow(rtHide);
		GetDlgItem(IDC_TOOL_ANGLE)->MoveWindow(rtHide);
		GetDlgItem(IDC_TOOL_SHAPE)->MoveWindow(rtHide);
		GetDlgItem(IDC_TOOL_VALUE)->MoveWindow(rtHide);
		GetDlgItem(IDC_TOOL_AREA)->MoveWindow(rtHide);
		GetDlgItem(IDC_TOOL_SELECT)->MoveWindow(rtHide);
		GetDlgItem(IDC_EXPORT_WITH_MASK)->MoveWindow(rtHide);
		GetDlgItem(IDC_SLIDER_STEP_SCALE)->MoveWindow(rtHide);
		GetDlgItem(IDC_SLIDER_OFFSET_SCALE)->MoveWindow(rtHide);
		GetDlgItem(IDC_STATIC_MIP_LABEL)->MoveWindow(rtHide);
		GetDlgItem(IDC_STATIC_MINIP_LABEL)->MoveWindow(rtHide);
		GetDlgItem(IDC_3D_LIGHT)->MoveWindow(rtHide);
		GetDlgItem(IDC_EXPORT_SHARP)->MoveWindow(rtHide);
		GetDlgItem(IDC_3D_LINE)->MoveWindow(rtHide);
		GetDlgItem(IDC_TOOL_SCALE)->MoveWindow(rtHide);
		GetDlgItem(IDC_TOOL_RESET)->MoveWindow(rtHide);
		GetDlgItem(IDC_MOVIE_RECORD)->MoveWindow(rtHide);
		GetDlgItem(IDC_TOOL_ELLIPSE)->MoveWindow(rtHide);
		CRect rtButton(rect);
		rtButton.top = rect.top;
		rtButton.bottom = rtButton.top + 50;
		rtButton.DeflateRect(2,2,2,2);

		if (m_bRayCasting && !m_bMaskTool)
		{
			((CButton*)GetDlgItem(IDC_RENDER_MODE))->SetWindowText(_T("渲染方式:\r\n光线投射"));
			GetDlgItem(IDC_RENDER_MODE)->MoveWindow(rtButton);
			rtButton.OffsetRect(0,rtButton.Height() + 4);
			((CButton*)GetDlgItem(IDC_TOOL_SELECT))->SetWindowText(_T("选择工具:\r\nMask模式"));
			GetDlgItem(IDC_TOOL_SELECT)->MoveWindow(rtButton);
			rtButton.OffsetRect(0,rtButton.Height() + 4);
			GetDlgItem(IDC_3D_CONTROL)->MoveWindow(rtButton);
			rtButton.OffsetRect(0,rtButton.Height() + 4);
			CRect rtGroup(rtButton);
			rtGroup.bottom = rtButton.top + rtButton.Height() * 6 - 5;
			m_Group2DPlane.MoveWindow(&rtGroup);
			rtButton.OffsetRect(0,rtButton.Height() + 4);
			rtButton.OffsetRect(0,rtButton.Height() + 4);
			rtButton.OffsetRect(0,rtButton.Height() + 4);
			rtButton.OffsetRect(0,rtButton.Height() + 4);
			rtButton.OffsetRect(0,rtButton.Height() + 4);
			rtGroup = rtButton;
			rtGroup.bottom = rtButton.top + rtButton.Height() * 5 - 5;
			m_Group3DPlane.MoveWindow(&rtGroup);
			rtButton.OffsetRect(0,rtButton.Height() + 4);
			rtButton.OffsetRect(0,rtButton.Height() + 4);
			rtButton.OffsetRect(0,rtButton.Height() + 4);
			rtButton.OffsetRect(0,rtButton.Height() + 4);
			rtButton.OffsetRect(0,rtButton.Height() / 2 + 4);
			GetDlgItem(IDC_MORPHOLOGY)->MoveWindow(rtButton);
			rtButton.OffsetRect(0,rtButton.Height() + 4);
			GetDlgItem(IDC_BOOLEAN)->MoveWindow(rtButton);
			rtButton.OffsetRect(0,rtButton.Height() + 4);
			GetDlgItem(IDC_ROI_EDIT)->MoveWindow(rtButton);
			rtButton.OffsetRect(0,rtButton.Height() + 4);
			GetDlgItem(IDC_PRINTSCREEN_3D)->MoveWindow(rtButton);
			rtButton.OffsetRect(0,rtButton.Height() + 4);
			GetDlgItem(IDC_EXPORT_FROM_MASK)->MoveWindow(rtButton);
			rtButton.OffsetRect(0,rtButton.Height() + 4);
			GetDlgItem(IDC_EXPORT_WITH_MASK)->MoveWindow(rtButton);
			rtButton.OffsetRect(0,rtButton.Height() + 4);
// 			GetDlgItem(IDC_RANDOM_PICK)->MoveWindow(rtButton);
// 			rtButton.OffsetRect(0,rtButton.Height() + 4);
			GetDlgItem(IDC_EXPORT_SHARP)->MoveWindow(rtButton);
			rtButton.OffsetRect(0,rtButton.Height() + 4);
			//GetDlgItem(IDC_TEMPLATE)->MoveWindow(rtButton);
			//rtButton.OffsetRect(0, 40 + 4);
		}
		else if (!m_bRayCasting)
		{
			((CButton*)GetDlgItem(IDC_RENDER_MODE))->SetWindowText(_T("渲染方式:\r\n体绘制"));
			GetDlgItem(IDC_RENDER_MODE)->MoveWindow(rtButton);
			rtButton.OffsetRect(0,rtButton.Height() + 4);
			CRect rtGroup(rtButton);
			rtGroup.bottom = rtButton.top + rtButton.Height() * 5 - 5;
			m_Group3DPlane2.MoveWindow(&rtGroup);
			rtButton.OffsetRect(0,rtButton.Height() + 4);
			rtButton.OffsetRect(0,rtButton.Height() + 4);
			rtButton.OffsetRect(0,rtButton.Height() + 4);
			rtButton.OffsetRect(0,rtButton.Height() + 4);
			rtButton.OffsetRect(0,rtButton.Height() / 2 + 4);
			GetDlgItem(IDC_TRANS_FUNC)->MoveWindow(rtButton);
			rtButton.OffsetRect(0,rtButton.Height() + 4);
			GetDlgItem(IDC_3D_CONTROL)->MoveWindow(rtButton);
			rtButton.OffsetRect(0,rtButton.Height() + 4);
			GetDlgItem(IDC_3D_LIGHT)->MoveWindow(rtButton);
			rtButton.OffsetRect(0,rtButton.Height() + 4);
			GetDlgItem(IDC_PRINTSCREEN_3D)->MoveWindow(rtButton);
			rtButton.OffsetRect(0,rtButton.Height() + 4);
			GetDlgItem(IDC_FREE_CUT)->MoveWindow(rtButton);
			rtButton.OffsetRect(0,rtButton.Height() + 4);
			GetDlgItem(IDC_3D_OUTPUT)->MoveWindow(rtButton);
			rtButton.OffsetRect(0,rtButton.Height() + 4);
			GetDlgItem(IDC_MOVIE_RECORD)->MoveWindow(rtButton);
			rtButton.OffsetRect(0,rtButton.Height() + 4);
			//CRect rtGroup(rtButton);
			//rtGroup.bottom = rtButton.top + rtButton.Height() * 10 + 5;
// 			GetDlgItem(IDC_SLIDER_STEP_SCALE)->MoveWindow(rtButton);
// 			rtButton.OffsetRect(0,rtButton.Height() + 4);
// 			GetDlgItem(IDC_SLIDER_OFFSET_SCALE)->MoveWindow(rtButton);
// 			rtButton.OffsetRect(0,rtButton.Height() + 4);
		}
		else
		{
			((CButton*)GetDlgItem(IDC_TOOL_SELECT))->SetWindowText(_T("选择工具:\r\n标注模式"));
			GetDlgItem(IDC_TOOL_SELECT)->MoveWindow(rtButton);
			rtButton.OffsetRect(0, rtButton.Height() + 4);

			CRect rtTool = rtButton;
			const int kButtonWidth = 40;
			const int kButtonHeight = 40;
			const int kButtonGap = 8;
			rtTool.right = rtTool.left + kButtonWidth;
			rtTool.bottom = rtTool.top + kButtonHeight;

			GetDlgItem(IDC_TOOL_LINE)->MoveWindow(rtTool);
			rtTool.OffsetRect(kButtonWidth + kButtonGap, 0);
			GetDlgItem(IDC_TOOL_ANGLE)->MoveWindow(rtTool);
			rtTool.OffsetRect(kButtonWidth + kButtonGap, 0);
			GetDlgItem(IDC_TOOL_SHAPE)->MoveWindow(rtTool);
			rtTool.OffsetRect(kButtonWidth + kButtonGap, 0);
			GetDlgItem(IDC_TOOL_VALUE)->MoveWindow(rtTool);
			rtTool.OffsetRect(kButtonWidth + kButtonGap, 0);
			GetDlgItem(IDC_TOOL_AREA)->MoveWindow(rtTool);

			rtTool.OffsetRect(-(kButtonWidth + kButtonGap) * 4, kButtonHeight + kButtonGap);
			GetDlgItem(IDC_TOOL_ELLIPSE)->MoveWindow(rtTool);
			rtTool.OffsetRect(kButtonWidth + kButtonGap, 0);
			GetDlgItem(IDC_TOOL_SCALE)->MoveWindow(rtTool);
			rtTool.OffsetRect(kButtonWidth + kButtonGap, 0);
			GetDlgItem(IDC_TOOL_RESET)->MoveWindow(rtTool);

			const int sliderGap = 8;
			const int labelHeight = 18;
			const int sliderHeight = 24;
			const int sliderWidth = (kButtonWidth + kButtonGap) * 5 - kButtonGap;
			const int labelToSliderGap = 4;
			CRect mipLabelRect(rtButton.left,
				rtTool.bottom + sliderGap,
				rtButton.left + sliderWidth,
				rtTool.bottom + sliderGap + labelHeight);
			CRect mipSliderRect(mipLabelRect.left,
				mipLabelRect.bottom + labelToSliderGap,
				mipLabelRect.left + sliderWidth,
				mipLabelRect.bottom + labelToSliderGap + sliderHeight);

			CRect minipLabelRect(mipLabelRect.left,
				mipSliderRect.bottom + sliderGap,
				mipLabelRect.left + sliderWidth,
				mipSliderRect.bottom + sliderGap + labelHeight);
			CRect minipSliderRect(minipLabelRect.left,
				minipLabelRect.bottom + labelToSliderGap,
				minipLabelRect.left + sliderWidth,
				minipLabelRect.bottom + labelToSliderGap + sliderHeight);

			if (CWnd* pLabelMip = GetDlgItem(IDC_STATIC_MIP_LABEL))
			{
				pLabelMip->MoveWindow(mipLabelRect);
				pLabelMip->ShowWindow(SW_SHOW);
			}
			if (CWnd* pSliderMip = GetDlgItem(IDC_SLIDER_STEP_SCALE))
			{
				pSliderMip->MoveWindow(mipSliderRect);
				pSliderMip->ShowWindow(SW_SHOW);
			}

			if (CWnd* pLabelMinip = GetDlgItem(IDC_STATIC_MINIP_LABEL))
			{
				pLabelMinip->MoveWindow(minipLabelRect);
				pLabelMinip->ShowWindow(SW_SHOW);
			}
			if (CWnd* pSliderMinip = GetDlgItem(IDC_SLIDER_OFFSET_SCALE))
			{
				pSliderMinip->MoveWindow(minipSliderRect);
				pSliderMinip->ShowWindow(SW_SHOW);
			}
		}
	}
}


HBRUSH SeGeneralModuleCtrlDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CSeDialogBase::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  在此更改 DC 的任何特性
	switch(nCtlColor)
	{
	case CTLCOLOR_BTN:
		break;
	case CTLCOLOR_DLG:
		break;
	case CTLCOLOR_STATIC:
		pDC->SetBkMode(TRANSPARENT);
		pDC->SetBkColor(afxGlobalData.clrWindow);   
		pDC->SetTextColor(RGB(255,255,255));
		pDC->SelectObject(&afxGlobalData.fontRegular);//设置字体颜色
		return CreateSolidBrush(afxGlobalData.clrBarDkShadow);
	}

 	return hbr;
};


void SeGeneralModuleCtrlDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	bool bNeedRefresh = false;
	if (pScrollBar != NULL)
	{
		switch (pScrollBar->GetDlgCtrlID())
		{
		case IDC_SLIDER_STEP_SCALE:
			{
				int value = m_sliderStepScale.GetPos();
				value = value < 1 ? 1 : value;
				if (m_sliderStepScale.GetPos() != value)
				{
					m_sliderStepScale.SetPos(value);
				}
				if (m_nMipThickness != value)
				{
					m_nMipThickness = value;
					SeMPRView::SetMipThickness(value);
					UpdateProjectionSliderText();
					bNeedRefresh = true;
				}
			}
			break;
		case IDC_SLIDER_OFFSET_SCALE:
			{
				int value = m_sliderOffsetScale.GetPos();
				value = value < 1 ? 1 : value;
				if (m_sliderOffsetScale.GetPos() != value)
				{
					m_sliderOffsetScale.SetPos(value);
				}
				if (m_nMinIpThickness != value)
				{
					m_nMinIpThickness = value;
					SeMPRView::SetMinIpThickness(value);
					UpdateProjectionSliderText();
					bNeedRefresh = true;
				}
			}
			break;
		default:
			break;
		}
	}

	if (bNeedRefresh)
	{
		SeMPRView::RefreshAllViews();
	}

	CSeDialogBase::OnHScroll(nSBCode, nPos, pScrollBar);
}


BOOL SeGeneralModuleCtrlDlg::OnInitDialog()
{
	CSeDialogBase::OnInitDialog();

	// TODO:  在此添加额外的初始化


	DWORD dwStyle = m_lst2DPlane.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;// 选中某行使整行高亮（只适用与 report 风格的 listctrl）
	dwStyle |= LVS_EX_CHECKBOXES;//item 前生成 checkbox 控件
	m_lst2DPlane.SetExtendedStyle(dwStyle); // 设置扩展风格

	m_lst2DPlane.InsertColumn(0, "Color", LVCFMT_LEFT, 60);// 插入列
	m_lst2DPlane.InsertColumn(1, "Visible", LVCFMT_LEFT, 60);
	m_lst2DPlane.InsertColumn(2, "Min", LVCFMT_LEFT, 60);
	m_lst2DPlane.InsertColumn(3, "Max", LVCFMT_LEFT, 60);


	dwStyle = m_lst3DPlane.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;// 选中某行使整行高亮（只适用与 report 风格的 listctrl）
	dwStyle |= LVS_EX_CHECKBOXES;//item 前生成 checkbox 控件
	m_lst3DPlane.SetExtendedStyle(dwStyle); // 设置扩展风格

	m_lst3DPlane.InsertColumn(0, "Color", LVCFMT_LEFT, 60);// 插入列
	m_lst3DPlane.InsertColumn(1, "Visible", LVCFMT_LEFT, 60);

	dwStyle = m_lst3DPlane2.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;// 选中某行使整行高亮（只适用与 report 风格的 listctrl）
	dwStyle |= LVS_EX_CHECKBOXES;//item 前生成 checkbox 控件
	m_lst3DPlane2.SetExtendedStyle(dwStyle); // 设置扩展风格

	m_lst3DPlane2.InsertColumn(0, "Color", LVCFMT_LEFT, 60);// 插入列
	m_lst3DPlane2.InsertColumn(1, "Visible", LVCFMT_LEFT, 60);

	// 创建 形态学变换 操作窗口
	Safe_Delete(m_pMorPhologyDlg);
	m_pMorPhologyDlg = new CSeMorphologyDlg(this);
	m_pMorPhologyDlg->Create(IDD_DIALOG_MORPHOLOGY_OPERATION, this);
	m_pMorPhologyDlg->ShowWindow(SW_HIDE);

	// 创建 Boolean 操作窗口
	Safe_Delete(m_pBooleanDlg);
	m_pBooleanDlg = new CSeBooleanDlg(this);
	m_pBooleanDlg->Create(IDD_DIALOG_BOOLEAN_OPERATION, this);
	m_pBooleanDlg->ShowWindow(SW_HIDE);

	// 创建 ROI 编辑 操作窗口
	Safe_Delete(m_pROIEditDlg);
	m_pROIEditDlg = new CSeROIEditDlg(this);
	m_pROIEditDlg->Create(IDD_DIALOG_ROI_EDIT_OPERATION, this);
	m_pROIEditDlg->ShowWindow(SW_HIDE);
	SeMPRView::SetRoiShape(m_pROIEditDlg->GetShape());


	// 创建 3D工具 操作窗口
	Safe_Delete(m_p3DProrertityDlg);
	m_p3DProrertityDlg = new CSe3DPropertityDlg(this);
	m_p3DProrertityDlg->Create(IDD_DIALOG_VOLUME_PROPERTIES, this);
	m_p3DProrertityDlg->ShowWindow(SW_HIDE);

	// 创建 形态学变换 操作窗口
	Safe_Delete(m_pFreeCutDlg);
	m_pFreeCutDlg = new CSeFreeCutDlg(this);
	m_pFreeCutDlg->Create(IDD_DIALOG_FREE_CUT, this);
	m_pFreeCutDlg->ShowWindow(SW_HIDE);

	// 创建 光照 操作窗口
	Safe_Delete(m_pLightDlg);
	m_pLightDlg = new CSeLightDlg(this);
	m_pLightDlg->Create(IDD_DIALOG_LIGHT, this);
	m_pLightDlg->ShowWindow(SW_HIDE);

	// 创建 测量 操作窗口
	Safe_Delete(m_pResultDlg);
	m_pResultDlg = new CSeResultDlg(this);
	m_pResultDlg->Create(IDD_DIALOG_MEASURE, this);
	m_pResultDlg->ShowWindow(SW_HIDE);

	// 创建 录屏 操作窗口
	Safe_Delete(m_pMovieDlg);
	m_pMovieDlg = new CSeMovieDlg(this);
	m_pMovieDlg->Create(IDD_DIALOG_MOVIE, this);
	m_pMovieDlg->ShowWindow(SW_HIDE);


	m_templateCtl.AddString("默认(2048,4096)");
	m_templateCtl.AddString("骨窗()");
	m_templateCtl.AddString("肺窗()");
	m_templateCtl.AddString("软组织窗()");

	if (m_pResultDlg != NULL)
	{
		SeMPRView::m_wndResult = dynamic_cast<CWnd*>(m_pResultDlg);
	}

	// initialize projection sliders
	m_sliderStepScale.SetRange(1, 100);
	m_sliderStepScale.SetRangeMin(1, FALSE);
	m_sliderStepScale.SetTicFreq(1);
	m_sliderOffsetScale.SetRange(1, 100);
	m_sliderOffsetScale.SetRangeMin(1, FALSE);
	m_sliderOffsetScale.SetTicFreq(1);
	m_sliderStepScale.SetPos(m_nMipThickness);
	m_sliderOffsetScale.SetPos(m_nMinIpThickness);
	UpdateProjectionSliderText();
	UpdateData(TRUE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void SeGeneralModuleCtrlDlg::UpdateProjectionSliderText()
{
	CString text;
	text.Format(_T("MIP 厚度: %d"), m_nMipThickness);
	SetDlgItemText(IDC_STATIC_MIP_LABEL, text);
	text.Format(_T("MinIP 厚度: %d"), m_nMinIpThickness);
	SetDlgItemText(IDC_STATIC_MINIP_LABEL, text);
}

void SeGeneralModuleCtrlDlg::OnBnClickedChoiceHandle()
{
	// TODO: 在此添加控件通知处理程序代码
}

void SeGeneralModuleCtrlDlg::OnBnClickedButton2dAdd()
{
	// TODO: 在此添加控件通知处理程序代码
	if(g_pGeneralModule->m_OriDcmArray.GetDcmArray().size() == 0)
	{
		AfxMessageBox("请先读入数据！");
		return;
	}

	CSeNewMaskDlg dlg(
		&theGeneralSwapData.m_Histogram[0],
		theGeneralSwapData.m_lMaxNumber, 
		theGeneralSwapData.m_nMaxValue, 
		theGeneralSwapData.m_nMinValue,
		this);
	theGeneralSwapData.m_pXOYView->SetMouseTool(MT_MPR);
	theGeneralSwapData.m_pXOZView->SetMouseTool(MT_MPR);
	theGeneralSwapData.m_pYOZView->SetMouseTool(MT_MPR);
	theGeneralSwapData.m_pXOYView->m_bNewMask = TRUE;
	theGeneralSwapData.m_pYOZView->m_bNewMask = TRUE;
	theGeneralSwapData.m_pXOZView->m_bNewMask = TRUE;
	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
	if (dlg.DoModal() == IDOK)
	{
		int nMin = dlg.GetMin();
		int nMax = dlg.GetMax();
		int nRow = m_lst2DPlane.InsertItem(m_lst2DPlane.GetItemCount(), "    ");// 插入行
		m_lst2DPlane.SetItemText(nRow, 1, "true");// 设置数据
		CString str;
		str.Format("%d", nMin);
		m_lst2DPlane.SetItemText(nRow, 2, str);// 设置数据
		str.Format("%d", nMax);
		m_lst2DPlane.SetItemText(nRow, 3, str);// 设置数据
		m_lst2DPlane.SetCheck(nRow, TRUE);
		m_lst2DPlane.SetItemState(nRow, LVNI_FOCUSED | LVIS_SELECTED, LVNI_FOCUSED | LVIS_SELECTED);
		MaskInfo mkinfo(nMin, nMax, m_ColorLst[m_nColorIndex], 64, TRUE);
		SeMPRView::ConvertDcms2VolTex(mkinfo);
		m_nColorIndex = (m_nColorIndex + 1) % 8;
	}
	theGeneralSwapData.m_pXOYView->m_bNewMask = FALSE;
	theGeneralSwapData.m_pYOZView->m_bNewMask = FALSE;
	theGeneralSwapData.m_pXOZView->m_bNewMask = FALSE;
	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
}


void SeGeneralModuleCtrlDlg::OnBnClickedButton2dRemove()
{
	// TODO: 在此添加控件通知处理程序代码
	POSITION pos = m_lst2DPlane.GetFirstSelectedItemPosition();
	if (pos == NULL)
		MessageBoxTimeout(NULL, "            未选择有效行！        ", "提示", MB_ICONINFORMATION, 0, 300);
	else
	{
		m_lst2DPlane.DeleteItem((int)pos - 1);
		//SeMPRView::m_vecMaskInfo.erase(SeMPRView::m_vecMaskInfo.begin() + (int)pos - 1);
		Safe_Delete(SeMPRView::m_vecROIData[(int)pos - 1]);
		SeMPRView::m_vecROIData.erase(SeMPRView::m_vecROIData.begin() + (int)pos - 1);
		theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
		theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
		theGeneralSwapData.m_pYOZView->Invalidate(FALSE);
	}
}

void SeGeneralModuleCtrlDlg::OnBnClickedButton2dColor()
{
	// TODO: 在此添加控件通知处理程序代码
	POSITION pos = m_lst2DPlane.GetFirstSelectedItemPosition();
	if (pos == NULL)
		MessageBoxTimeout(NULL, "            未选择有效行！        ", "提示", MB_ICONINFORMATION, 0, 300);
	else
	{
		COLORREF color = RGB(255, 0, 0);      // 颜色对话框的初始颜色为红色  
		CColorDialog colorDlg(color);         // 构造颜色对话框，传入初始颜色值   
		if (IDOK == colorDlg.DoModal()) 
		{
			color = colorDlg.GetColor();
			SeMPRView::m_vecROIData[(int)pos - 1]->ChangeColor(color);
			m_lst2DPlane.Invalidate(FALSE);
			theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
			theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
			theGeneralSwapData.m_pYOZView->Invalidate(FALSE);
		}
	}
}

void SeGeneralModuleCtrlDlg::OnBnClickedButton2dInfo()
{
	// TODO: 在此添加控件通知处理程序代码
	POSITION pos = m_lst2DPlane.GetFirstSelectedItemPosition();
	if (pos == NULL)
	{
		MessageBoxTimeout(NULL, "            未选择有效行！        ", "提示", MB_ICONINFORMATION, 0, 300);
		return;
	}
	//theGeneralSwapData.m_pXOYView->GetInfo((int)pos - 1);
	SeMPRView::GetBasicInfo((int)pos - 1);
	//SeMPRView::m_vecROIData[(int)pos - 1].GetInfo();

}


void SeGeneralModuleCtrlDlg::OnBnClickedButton3dAdd()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_lst2DPlane.GetItemCount() == 0)
	{
		AfxMessageBox("请先生成2D Mask");
		return;
	}
	CSeNewVolumeObjectDlg dlg;
	dlg.m_pParent = this;
	//dlg.m_lstVolumeObject = m_lst2DPlane;
	dlg.m_pLstSource = &m_lst2DPlane;
	vector <COLORREF> vecColorList;
	for (int i=0; i<m_lst2DPlane.GetItemCount(); i++)
	{
		vecColorList.push_back(SeMPRView::m_vecROIData[i]->GetColor());
	}
	dlg.m_colorList = vecColorList;
	if (dlg.DoModal() == IDOK)
	{
		int nPos = dlg.m_nSelect;
		int nCount = m_lst3DPlane.GetItemCount();
		int nRow = m_lst3DPlane.InsertItem(nCount, m_lst2DPlane.GetItemText(nPos, 0));// 插入行
		m_lst3DPlane.SetItemText(nRow, 1, m_lst2DPlane.GetItemText(nPos, 1));// 设置数据
		m_lst3DPlane.SetCheck(nRow, TRUE);
		m_lst3DPlane.SetItemState(nRow, LVNI_FOCUSED | LVIS_SELECTED, LVNI_FOCUSED | LVIS_SELECTED);
		theGeneralSwapData.m_p3DView->AddVolTex(
			SeMPRView::m_vecROIData[nPos]->GetWidth(), 
			SeMPRView::m_vecROIData[nPos]->GetHeight(), 
			SeMPRView::m_vecROIData[nPos]->GetLength(), 
			SeMPRView::m_vecROIData[nPos]->GetData(),
			SeMPRView::m_vecROIData[nPos]->GetColor()
			);
	}
}


void SeGeneralModuleCtrlDlg::OnBnClickedButton3dRemove()
{
	// TODO: 在此添加控件通知处理程序代码
	POSITION pos = m_lst3DPlane.GetFirstSelectedItemPosition();
	if (pos == NULL)
		MessageBoxTimeout(NULL, "            未选择有效行！        ", "提示", MB_ICONINFORMATION, 0, 300);
	else
	{

		m_lst3DPlane.DeleteItem((int)pos - 1);
		theGeneralSwapData.m_p3DView->RemoveVolTex((int)pos - 1);
	}
}


void SeGeneralModuleCtrlDlg::OnBnClickedButton3dColor()
{
	// TODO: 在此添加控件通知处理程序代码
	POSITION pos = m_lst3DPlane.GetFirstSelectedItemPosition();
	if (pos == NULL)
		MessageBoxTimeout(NULL, "            未选择有效行！        ", "提示", MB_ICONINFORMATION, 0, 300);
	else
	{
		COLORREF color = RGB(255, 0, 0);      // 颜色对话框的初始颜色为红色  
		CColorDialog colorDlg(color); // 构造颜色对话框，传入初始颜色值 
		if (IDOK == colorDlg.DoModal()) 
		{
			color = colorDlg.GetColor();
			theGeneralSwapData.m_p3DView->ChangeColor((int)pos - 1, color);
			m_lst3DPlane.Invalidate(FALSE);
			theGeneralSwapData.m_p3DView->Invalidate(FALSE);
		}
	}

}

LRESULT SeGeneralModuleCtrlDlg::OnAddMask(WPARAM wParam, LPARAM lParam)
{
	int nMin = (int)wParam;
	int nMax = (int)lParam;
	theGeneralSwapData.m_pXOYView->SetPngInfo(nMin, nMax, m_ColorLst[m_nColorIndex], 64);
	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->SetPngInfo(nMin, nMax, m_ColorLst[m_nColorIndex], 64);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->SetPngInfo(nMin, nMax, m_ColorLst[m_nColorIndex], 64);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);
	return 0;
}

LRESULT SeGeneralModuleCtrlDlg::OnAddMaskItem(WPARAM wParam, LPARAM lParam)
{
	int nMin = (int)wParam;
	int nMax = (int)lParam;
	int nRow = m_lst2DPlane.InsertItem(m_lst2DPlane.GetItemCount(), "    ");// 插入行
	m_lst2DPlane.SetItemText(nRow, 1, "true");// 设置数据
	CString str;
	str.Format("%d", nMin);
	m_lst2DPlane.SetItemText(nRow, 2, str);// 设置数据
	str.Format("%d", nMax);
	m_lst2DPlane.SetItemText(nRow, 3, str);// 设置数据
	m_lst2DPlane.SetCheck(nRow, TRUE);

	theGeneralSwapData.m_pXOYView->m_bNewMask = FALSE;
	theGeneralSwapData.m_pYOZView->m_bNewMask = FALSE;
	theGeneralSwapData.m_pXOZView->m_bNewMask = FALSE;
	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);

	m_nColorIndex = (m_nColorIndex + 1) % 8;
	return 0;
}

LRESULT SeGeneralModuleCtrlDlg::OnAddMaskItemFalse(WPARAM wParam, LPARAM lParam)
{
	theGeneralSwapData.m_pXOYView->m_bNewMask = FALSE;
	theGeneralSwapData.m_pYOZView->m_bNewMask = FALSE;
	theGeneralSwapData.m_pXOZView->m_bNewMask = FALSE;
	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
	return 0;
}




void SeGeneralModuleCtrlDlg::OnBnClickedMorphology()
{
	// TODO: 在此添加控件通知处理程序代码
	if(g_pGeneralModule->m_OriDcmArray.GetDcmArray().size() == 0)
	{
		AfxMessageBox("请先读入数据！");
		return;
	}

	if (m_pMorPhologyDlg->IsWindowVisible())
	{
		m_pMorPhologyDlg->ShowWindow(SW_HIDE);
	}
	else
	{
		theGeneralSwapData.m_pXOYView->SetMPRTool();
		theGeneralSwapData.m_pXOZView->SetMPRTool();
		theGeneralSwapData.m_pYOZView->SetMPRTool();

		m_p3DProrertityDlg->ShowWindow(SW_HIDE);
		m_pROIEditDlg->ShowWindow(SW_HIDE);
		m_pBooleanDlg->ShowWindow(SW_HIDE);

		m_pMorPhologyDlg->ShowWindow(SW_SHOW);
	}
	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);
}


void SeGeneralModuleCtrlDlg::OnBnClickedRoiEdit()
{
	// TODO: 在此添加控件通知处理程序代码
	if(g_pGeneralModule->m_OriDcmArray.GetDcmArray().size() == 0)
	{
		AfxMessageBox("请先读入数据！");
		return;
	}

	if (m_pROIEditDlg->IsWindowVisible())
	{
		m_pROIEditDlg->ShowWindow(SW_HIDE);
		theGeneralSwapData.m_pXOYView->SetMPRTool();
		theGeneralSwapData.m_pXOZView->SetMPRTool();
		theGeneralSwapData.m_pYOZView->SetMPRTool();
	}
	else
	{
		m_p3DProrertityDlg->ShowWindow(SW_HIDE);
		m_pMorPhologyDlg->ShowWindow(SW_HIDE);
		m_pBooleanDlg->ShowWindow(SW_HIDE);

		m_pROIEditDlg->ShowWindow(SW_SHOW);
		SendMessage(WM_CHANGE_PLANE_NUMBER, 0, 0);
	}
	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);
}


void SeGeneralModuleCtrlDlg::OnBnClickedBoolean()
{
	// TODO: 在此添加控件通知处理程序代码
	if(g_pGeneralModule->m_OriDcmArray.GetDcmArray().size() == 0)
	{
		AfxMessageBox("请先读入数据！");
		return;
	}

	if (m_pBooleanDlg->IsWindowVisible())
	{
		m_pBooleanDlg->ShowWindow(SW_HIDE);
	}
	else
	{
		theGeneralSwapData.m_pXOYView->SetMPRTool();
		theGeneralSwapData.m_pXOZView->SetMPRTool();
		theGeneralSwapData.m_pYOZView->SetMPRTool();

		m_p3DProrertityDlg->ShowWindow(SW_HIDE);
		m_pMorPhologyDlg->ShowWindow(SW_HIDE);
		m_pROIEditDlg->ShowWindow(SW_HIDE);

		m_pBooleanDlg->m_colorList.clear();
		for (int i=0; i<SeMPRView::m_vecROIData.size(); i++)
		{
			m_pBooleanDlg->m_colorList.push_back(SeMPRView::m_vecROIData[i]->GetColor());
		}
		m_pBooleanDlg->UpdateCBData();
		m_pBooleanDlg->ShowWindow(SW_SHOW);
	}
	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);

}

void SeGeneralModuleCtrlDlg::OnBnClicked3dControl()
{
	// TODO: 在此添加控件通知处理程序代码
	if(g_pGeneralModule->m_OriDcmArray.GetDcmArray().size() == 0)
	{
		AfxMessageBox("请先读入数据！");
		return;
	}

	if (m_p3DProrertityDlg->IsWindowVisible())
		m_p3DProrertityDlg->ShowWindow(SW_HIDE);
	else
	{
		theGeneralSwapData.m_pXOYView->SetMPRTool();
		theGeneralSwapData.m_pXOZView->SetMPRTool();
		theGeneralSwapData.m_pYOZView->SetMPRTool();

		m_pMorPhologyDlg->ShowWindow(SW_HIDE);
		m_pROIEditDlg->ShowWindow(SW_HIDE);
		m_pBooleanDlg->ShowWindow(SW_HIDE);

		m_p3DProrertityDlg->ShowWindow(SW_SHOW);
	}
	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);
}


LRESULT SeGeneralModuleCtrlDlg::OnMorphologyOperation(WPARAM wParam, LPARAM lParam)
{
	POSITION pos = m_lst2DPlane.GetFirstSelectedItemPosition();
	if (pos == NULL)
	{
		MessageBoxTimeout(NULL, "            未选择有效行！        ", "提示", MB_ICONINFORMATION, 0, 300);
		return 0;
	}

	switch (wParam)
	{
	case MORPHOLOGY_CORROSION:
		{
			SeMPRView::m_vecROIData[(int)pos - 1]->Corrosion(lParam, 1);
			break;
		}
	case MORPGOLOGY_DELITE:
		{
			SeMPRView::m_vecROIData[(int)pos - 1]->Dilate(lParam, 1);
			break;
		}
	case MORPHOLOGY_CLOSE:
		{
			SeMPRView::m_vecROIData[(int)pos - 1]->Close(lParam, 1);
			break;
		}
	case MORPHOLOGY_OPEN:
		{
			SeMPRView::m_vecROIData[(int)pos - 1]->Open(lParam, 1);
			break;
		}
	case MORPHOLOGY_FLOODFILL:
		{
			this->EnableWindow(FALSE);
			theGeneralSwapData.m_pXOYView->SetMouseTool(MT_FLOODFILL);
			theGeneralSwapData.m_pXOZView->SetMouseTool(MT_FLOODFILL);
			theGeneralSwapData.m_pYOZView->SetMouseTool(MT_FLOODFILL);
			for (int i=0; i<SeMPRView::m_vecROIData.size(); i++)
			{
				if (i != (int)pos -1)
				{
					SeMPRView::m_vecROIData[i]->SetVisible(FALSE);
					m_lst2DPlane.SetItemText(i, 1, "false");
					m_lst2DPlane.SetCheck(i, FALSE);
				}
				else
				{
					SeMPRView::m_vecROIData[i]->SetVisible(TRUE);
					m_lst2DPlane.SetItemText(i, 1, "true");
					m_lst2DPlane.SetCheck(i, TRUE);
				}
			}
			CSeROIData* data = new CSeROIData(SeMPRView::m_vecROIData[(int)pos - 1], m_ColorLst[m_nColorIndex], 64, TRUE);
			SeMPRView::m_vecROIData.push_back(data);
			m_nColorIndex = (m_nColorIndex + 1) % 8;
			break;
		}
	case MORPHOLOGY_INVERSE:
		{
			SeMPRView::m_vecROIData[(int)pos - 1]->Inverse();
			break;
		}
	case MORPHOLOGY_QUIT_FLOODFILL:
		{
			this->EnableWindow(TRUE);
			theGeneralSwapData.m_pXOYView->SetMouseTool(MT_MPR);
			theGeneralSwapData.m_pXOZView->SetMouseTool(MT_MPR);
			theGeneralSwapData.m_pYOZView->SetMouseTool(MT_MPR);
			SeMPRView::m_vecROIData.pop_back();
			break;
		}
	case MORPHOLOGY_EXECUTE_FLOODFILL:
		{
			this->EnableWindow(TRUE);

			int nRow = m_lst2DPlane.InsertItem(m_lst2DPlane.GetItemCount(), "    ");// 插入行
			m_lst2DPlane.SetItemText(nRow, 1, "true");// 设置数据
			m_lst2DPlane.SetItemText(nRow, 2, "--");// 设置数据
			m_lst2DPlane.SetItemText(nRow, 3, "--");// 设置数据
			m_lst2DPlane.SetCheck(nRow, TRUE);

			theGeneralSwapData.m_pXOYView->SetMouseTool(MT_MPR);
			theGeneralSwapData.m_pXOZView->SetMouseTool(MT_MPR);
			theGeneralSwapData.m_pYOZView->SetMouseTool(MT_MPR);
			Invalidate(FALSE);
			break;
		}
	default:
		break;
	}
	SeMPRView::m_bShowMask = TRUE;

	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);
	return 0;
}

LRESULT SeGeneralModuleCtrlDlg::OnBooleanOperation(WPARAM wParam, LPARAM lParam)
{
	int nMaskA = ((int*)wParam)[0];
	int nOperation = ((int*)wParam)[1];
	int nMaskB = ((int*)wParam)[2];
	CSeROIData* data = new CSeROIData(SeMPRView::m_vecROIData[nMaskA], SeMPRView::m_vecROIData[nMaskB], (BOOLEAN_OPERATION)nOperation, m_ColorLst[m_nColorIndex], 64, TRUE);
	SeMPRView::m_vecROIData.push_back(data);

	int nRow = m_lst2DPlane.InsertItem(m_lst2DPlane.GetItemCount(), "    ");// 插入行
	m_lst2DPlane.SetItemText(nRow, 1, "true");// 设置数据
	m_lst2DPlane.SetItemText(nRow, 2, "--");// 设置数据
	m_lst2DPlane.SetItemText(nRow, 3, "--");// 设置数据
	m_lst2DPlane.SetCheck(nRow, TRUE);

	m_nColorIndex = (m_nColorIndex + 1) % 8;

	SeMPRView::m_bShowMask = TRUE;

	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);

	return 0;
}

LRESULT SeGeneralModuleCtrlDlg::OnChangeROIEditPlane(WPARAM wParam, LPARAM lParam)
{
	// 重置鼠标工具状态
	theGeneralSwapData.m_pXOYView->SetMouseTool(MT_Select);
	theGeneralSwapData.m_pYOZView->SetMouseTool(MT_Select);
	theGeneralSwapData.m_pXOZView->SetMouseTool(MT_Select);

	if (m_nPlaneNumNow != m_pROIEditDlg->GetPlaneNumNow())
	{
		if (m_nPlaneNumNow == 1)
			theGeneralSwapData.m_pXOYView->DeleteROI();
		else if (m_nPlaneNumNow == 2)
			theGeneralSwapData.m_pXOZView->DeleteROI();
		else if (m_nPlaneNumNow == 3)
			theGeneralSwapData.m_pYOZView->DeleteROI();

		SeMPRView::m_bShowMask = TRUE;

		theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
		theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
		theGeneralSwapData.m_pYOZView->Invalidate(FALSE);

		m_nPlaneNumNow = m_pROIEditDlg->GetPlaneNumNow();
	}

	if (m_nPlaneNumNow == 1)
		theGeneralSwapData.m_pXOYView->SetMouseTool(MT_ROI);
	else if (m_nPlaneNumNow == 2)
		theGeneralSwapData.m_pXOZView->SetMouseTool(MT_ROI);
	else if (m_nPlaneNumNow == 3)
		theGeneralSwapData.m_pYOZView->SetMouseTool(MT_ROI);
	return 0;
}


LRESULT SeGeneralModuleCtrlDlg::OnChangeROIShape(WPARAM wParam, LPARAM lParam)
{
	SeMPRView::SetRoiShape(static_cast<int>(wParam));
	return 0;
}


LRESULT SeGeneralModuleCtrlDlg::OnDeleteROI(WPARAM wPAram, LPARAM lParam)
{
	theGeneralSwapData.m_pXOYView->DeleteROI();
	theGeneralSwapData.m_pXOZView->DeleteROI();
	theGeneralSwapData.m_pYOZView->DeleteROI();
	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);
	return 0;
}

LRESULT SeGeneralModuleCtrlDlg::OnMidLayer(WPARAM wParam, LPARAM lParam)
{
	int nPlaneNum = m_pROIEditDlg->GetPlaneNumNow();
	if (nPlaneNum == 1)
		theGeneralSwapData.m_pXOYView->FillMidLayer();
	else if (nPlaneNum == 2)
		theGeneralSwapData.m_pXOZView->FillMidLayer();
	else if (nPlaneNum == 3)
		theGeneralSwapData.m_pYOZView->FillMidLayer();
	return 0;
}

LRESULT SeGeneralModuleCtrlDlg::OnExecuteROI(WPARAM wParam, LPARAM lParam)
{
	POSITION pos = m_lst2DPlane.GetFirstSelectedItemPosition();
	if (pos == NULL)
	{
		MessageBoxTimeout(NULL, "            未选择有效行！        ", "提示", MB_ICONINFORMATION, 0, 300);
		return 0;
	}
	
	int nPlaneNum = m_pROIEditDlg->GetPlaneNumNow();


	if (nPlaneNum == 1)
	{
		theGeneralSwapData.m_pXOYView->ROI(int(pos)-1, (ROI_OPERATION)m_pROIEditDlg->GetOperation());
		theGeneralSwapData.m_pXOYView->DeleteROI();	
	}
	else if (nPlaneNum == 2)
	{
		theGeneralSwapData.m_pXOZView->ROI(int(pos)-1, (ROI_OPERATION)m_pROIEditDlg->GetOperation());
		theGeneralSwapData.m_pXOZView->DeleteROI();	
	} 
	else if (nPlaneNum == 3)
	{
		theGeneralSwapData.m_pYOZView->ROI(int(pos)-1, (ROI_OPERATION)m_pROIEditDlg->GetOperation());
		theGeneralSwapData.m_pYOZView->DeleteROI();	
	}

	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);

	return 0;
}




void SeGeneralModuleCtrlDlg::OnNMCustomdrawList2dPlane(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );

	*pResult = 0;
	// If this is the beginning of the control's paint cycle, request
	// notifications for each item.

	if ( CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	else if ( CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		// This is the pre-paint stage for an item.  We need to make another
		// request to be notified during the post-paint stage.

		*pResult = CDRF_NOTIFYPOSTPAINT;
	}
	else if ( CDDS_ITEMPOSTPAINT == pLVCD->nmcd.dwDrawStage )
	{
		// If this item is selected, re-draw the icon in its normal
		// color (not blended with the highlight color).
		LVITEM rItem;
		int    nItem = static_cast<int>( pLVCD->nmcd.dwItemSpec );
		ZeroMemory ( &rItem, sizeof(LVITEM) );
		rItem.mask  = LVIF_IMAGE | LVIF_STATE;
		rItem.iItem = nItem;
		rItem.stateMask = LVIS_SELECTED;
		m_lst2DPlane.GetItem ( &rItem );

		CDC*  pDC = CDC::FromHandle ( pLVCD->nmcd.hdc );
		CRect rcIcon;
		// Get the rect that holds the item's icon.
		m_lst2DPlane.GetSubItemRect(nItem, 0, LVIR_LABEL, rcIcon);
		rcIcon.right = rcIcon.left + rcIcon.Height();
		rcIcon.DeflateRect(2,2,2,2);
		if (SeMPRView::m_vecROIData.size() <= nItem)
			return;
		COLORREF crBkgnd = SeMPRView::m_vecROIData[nItem]->GetColor();
		// Draw the icon.
		pDC->FillSolidRect(rcIcon, crBkgnd);

		*pResult = CDRF_SKIPDEFAULT;

	} 
}


void SeGeneralModuleCtrlDlg::OnNMCustomdrawList3dPlane(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );

	*pResult = 0;
	// If this is the beginning of the control's paint cycle, request
	// notifications for each item.

	if ( CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	else if ( CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		// This is the pre-paint stage for an item.  We need to make another
		// request to be notified during the post-paint stage.

		*pResult = CDRF_NOTIFYPOSTPAINT;
	}
	else if ( CDDS_ITEMPOSTPAINT == pLVCD->nmcd.dwDrawStage )
	{
		// If this item is selected, re-draw the icon in its normal
		// color (not blended with the highlight color).
		LVITEM rItem;
		int    nItem = static_cast<int>( pLVCD->nmcd.dwItemSpec );
		ZeroMemory ( &rItem, sizeof(LVITEM) );
		rItem.mask  = LVIF_IMAGE | LVIF_STATE;
		rItem.iItem = nItem;
		rItem.stateMask = LVIS_SELECTED;
		m_lst3DPlane.GetItem ( &rItem );

		CDC*  pDC = CDC::FromHandle ( pLVCD->nmcd.hdc );
		CRect rcIcon;
		// Get the rect that holds the item's icon.
		m_lst3DPlane.GetSubItemRect(nItem, 0, LVIR_LABEL, rcIcon);
		rcIcon.right = rcIcon.left + rcIcon.Height();
		rcIcon.DeflateRect(2,2,2,2);
		COLORREF crBkgnd = theGeneralSwapData.m_p3DView->GetColor(nItem);
		// Draw the icon.
		pDC->FillSolidRect(rcIcon, crBkgnd);

		*pResult = CDRF_SKIPDEFAULT;

	}
}

void SeGeneralModuleCtrlDlg::OnNMCustomdrawList3dPlane2(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );

	*pResult = 0;
	// If this is the beginning of the control's paint cycle, request
	// notifications for each item.

	if ( CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	else if ( CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		// This is the pre-paint stage for an item.  We need to make another
		// request to be notified during the post-paint stage.

		*pResult = CDRF_NOTIFYPOSTPAINT;
	}
	else if ( CDDS_ITEMPOSTPAINT == pLVCD->nmcd.dwDrawStage )
	{
		// If this item is selected, re-draw the icon in its normal
		// color (not blended with the highlight color).
		LVITEM rItem;
		int    nItem = static_cast<int>( pLVCD->nmcd.dwItemSpec );
		ZeroMemory ( &rItem, sizeof(LVITEM) );
		rItem.mask  = LVIF_IMAGE | LVIF_STATE;
		rItem.iItem = nItem;
		rItem.stateMask = LVIS_SELECTED;
		m_lst3DPlane2.GetItem ( &rItem );

		CDC*  pDC = CDC::FromHandle ( pLVCD->nmcd.hdc );
		CRect rcIcon;
		// Get the rect that holds the item's icon.
		m_lst3DPlane2.GetSubItemRect(nItem, 0, LVIR_LABEL, rcIcon);
		rcIcon.right = rcIcon.left + rcIcon.Height();
		rcIcon.DeflateRect(2,2,2,2);
		COLORREF crBkgnd = theGeneralSwapData.m_p3DView->GetColor(nItem, FALSE);
		// Draw the icon.
		pDC->FillSolidRect(rcIcon, crBkgnd);

		*pResult = CDRF_SKIPDEFAULT;

	}
}

void SeGeneralModuleCtrlDlg::OnNMClickList2dPlane(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	DWORD dwPos = GetMessagePos();
	CPoint point(LOWORD(dwPos), HIWORD(dwPos) ); 
	m_lst2DPlane.ScreenToClient(&point);
	LVHITTESTINFO lvinfo; 
	lvinfo.pt = point; 
	lvinfo.flags = LVHT_ABOVE;  
	UINT nFlag;
	int nItem = m_lst2DPlane.HitTest(point, &nFlag);
	if (nFlag == LVHT_ONITEMSTATEICON)
	{
		BOOL bCheckState = m_lst2DPlane.GetCheck(nItem);

		//注意，bCheckState为TRUE，checkbox从勾选状态变为未勾选状态
		if (bCheckState)
		{
			m_lst2DPlane.SetItemText(nItem, 1, "false");
			SeMPRView::m_vecROIData[nItem]->SetVisible(FALSE);
		}
		else
		{
			m_lst2DPlane.SetItemText(nItem, 1, "true");
			SeMPRView::m_vecROIData[nItem]->SetVisible(TRUE);
		}
	}
	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);

	*pResult = 0;
}

void SeGeneralModuleCtrlDlg::OnNMClickList3dPlane(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	DWORD dwPos = GetMessagePos();
	CPoint point(LOWORD(dwPos), HIWORD(dwPos) ); 
	m_lst3DPlane.ScreenToClient(&point);
	LVHITTESTINFO lvinfo; 
	lvinfo.pt = point; 
	lvinfo.flags = LVHT_ABOVE;  
	UINT nFlag;
	int nItem = m_lst3DPlane.HitTest(point, &nFlag);
	if (nFlag == LVHT_ONITEMSTATEICON)
	{
		BOOL bCheckState = m_lst3DPlane.GetCheck(nItem);

		//注意，bCheckState为TRUE，checkbox从勾选状态变为未勾选状态
		if (bCheckState)
		{
			m_lst3DPlane.SetItemText(nItem, 1, "false");
			theGeneralSwapData.m_p3DView->ChangeVisible(nItem, FALSE);
		}
		else
		{
			m_lst3DPlane.SetItemText(nItem, 1, "true");
			theGeneralSwapData.m_p3DView->ChangeVisible(nItem, TRUE);
		}
		theGeneralSwapData.m_p3DView->Invalidate(FALSE);
	}
	*pResult = 0;
}

void SeGeneralModuleCtrlDlg::OnNMClickList3dPlane2(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	DWORD dwPos = GetMessagePos();
	CPoint point(LOWORD(dwPos), HIWORD(dwPos) ); 
	m_lst3DPlane2.ScreenToClient(&point);
	LVHITTESTINFO lvinfo; 
	lvinfo.pt = point; 
	lvinfo.flags = LVHT_ABOVE;  
	UINT nFlag;
	int nItem = m_lst3DPlane2.HitTest(point, &nFlag);
	if (nFlag == LVHT_ONITEMSTATEICON)
	{
		BOOL bCheckState = m_lst3DPlane2.GetCheck(nItem);

		//注意，bCheckState为TRUE，checkbox从勾选状态变为未勾选状态
		if (bCheckState)
		{
			m_lst3DPlane.SetItemText(nItem, 1, "false");
			theGeneralSwapData.m_p3DView->ChangeVisible(nItem, FALSE, FALSE);
		}
		else
		{
			m_lst3DPlane.SetItemText(nItem, 1, "true");
			theGeneralSwapData.m_p3DView->ChangeVisible(nItem, TRUE, FALSE);
		}
		theGeneralSwapData.m_p3DView->Invalidate(FALSE);
	}
	*pResult = 0;
}

void SeGeneralModuleCtrlDlg::OnNMDblclkList2dPlane(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	DWORD dwPos = GetMessagePos();
	CPoint point(LOWORD(dwPos), HIWORD(dwPos) ); 
	m_lst2DPlane.ScreenToClient(&point);
	LVHITTESTINFO lvinfo; 
	lvinfo.pt = point; 
	lvinfo.flags = LVHT_ABOVE;  
	UINT nFlag;
	int nItem = m_lst2DPlane.HitTest(point, &nFlag);
	if (nFlag == LVHT_ONITEMSTATEICON)
	{
		BOOL bCheckState = m_lst2DPlane.GetCheck(nItem);

		//注意，bCheckState为TRUE，checkbox从勾选状态变为未勾选状态
		if (bCheckState)
		{
			m_lst2DPlane.SetItemText(nItem, 1, "false");
			SeMPRView::m_vecROIData[nItem]->SetVisible(FALSE);
		}
		else
		{
			m_lst2DPlane.SetItemText(nItem, 1, "true");
			SeMPRView::m_vecROIData[nItem]->SetVisible(TRUE);
		}
	}
	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);

	*pResult = 0;
}


void SeGeneralModuleCtrlDlg::OnNMDblclkList3dPlane(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	DWORD dwPos = GetMessagePos();
	CPoint point(LOWORD(dwPos), HIWORD(dwPos) ); 
	m_lst3DPlane.ScreenToClient(&point);
	LVHITTESTINFO lvinfo; 
	lvinfo.pt = point; 
	lvinfo.flags = LVHT_ABOVE;  
	UINT nFlag;
	int nItem = m_lst3DPlane.HitTest(point, &nFlag);
	if (nFlag == LVHT_ONITEMSTATEICON)
	{
		BOOL bCheckState = m_lst3DPlane.GetCheck(nItem);

		//注意，bCheckState为TRUE，checkbox从勾选状态变为未勾选状态
		if (bCheckState)
		{
			m_lst3DPlane.SetItemText(nItem, 1, "false");
			theGeneralSwapData.m_p3DView->ChangeVisible(nItem, FALSE);
			//SeMPRView::m_vecROIData[nItem]->SetVisible(FALSE);
		}
		else
		{
			m_lst3DPlane.SetItemText(nItem, 1, "true");
			theGeneralSwapData.m_p3DView->ChangeVisible(nItem, TRUE);
			//SeMPRView::m_vecROIData[nItem]->SetVisible(TRUE);
		}
	}
	*pResult = 0;
}

void SeGeneralModuleCtrlDlg::OnNMDblclkList3dPlane2(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	DWORD dwPos = GetMessagePos();
	CPoint point(LOWORD(dwPos), HIWORD(dwPos) ); 
	m_lst3DPlane2.ScreenToClient(&point);
	LVHITTESTINFO lvinfo; 
	lvinfo.pt = point; 
	lvinfo.flags = LVHT_ABOVE;  
	UINT nFlag;
	int nItem = m_lst3DPlane2.HitTest(point, &nFlag);
	if (nFlag == LVHT_ONITEMSTATEICON)
	{
		BOOL bCheckState = m_lst3DPlane2.GetCheck(nItem);

		//注意，bCheckState为TRUE，checkbox从勾选状态变为未勾选状态
		if (bCheckState)
		{
			m_lst3DPlane2.SetItemText(nItem, 1, "false");
			theGeneralSwapData.m_p3DView->ChangeVisible(nItem, FALSE, FALSE);
			//SeMPRView::m_vecROIData[nItem]->SetVisible(FALSE);
		}
		else
		{
			m_lst3DPlane2.SetItemText(nItem, 1, "true");
			theGeneralSwapData.m_p3DView->ChangeVisible(nItem, TRUE, FALSE);
			//SeMPRView::m_vecROIData[nItem]->SetVisible(TRUE);
		}
	}
	*pResult = 0;
}

LRESULT SeGeneralModuleCtrlDlg::OnSetMouseTool(WPARAM wParam, LPARAM lParam)
{
	theGeneralSwapData.m_pXOYView->SetMouseTool(wParam);
	theGeneralSwapData.m_pXOZView->SetMouseTool(wParam);
	theGeneralSwapData.m_pYOZView->SetMouseTool(wParam);

	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);
	return 0;
}

LRESULT SeGeneralModuleCtrlDlg::OnSet3DState(WPARAM wParam, LPARAM lParam)
{
	VOLUME_INFO* pInfo = (VOLUME_INFO*)wParam;
// 	theGeneralSwapData.m_p3DView->SetViewPos(
// 		pInfo->ViewPosition.fXpos,
// 		pInfo->ViewPosition.fYpos,
// 		pInfo->ViewPosition.fZpos
// 		);
// 	theGeneralSwapData.m_p3DView->SetLightPos(
// 		pInfo->lightPosition.fXpos,
// 		pInfo->lightPosition.fYpos,
// 		pInfo->lightPosition.fZpos
// 		);
	theGeneralSwapData.m_p3DView->SetSpeed(
		pInfo->nMoveSpeed, MOVE_SPEED);

	theGeneralSwapData.m_p3DView->SetSpeed(
		pInfo->nRotateSpeed, ROTATE_SPEED);

	theGeneralSwapData.m_p3DView->SetSpeed(
		pInfo->nScaleSpeed, SCALE_SPEED);

	theGeneralSwapData.m_p3DView->SetSpeed(
		pInfo->nQuality, QUALITY);

	theGeneralSwapData.m_p3DView->SetBkGndColor(
		pInfo->color);

// 	theGeneralSwapData.m_p3DView->SetLightColor(
// 		pInfo->LightColor);
// 	theGeneralSwapData.m_p3DView->SetMaterialColor(
// 		pInfo->MaterialColor);
// 	theGeneralSwapData.m_p3DView->SetLightMat(
// 		pInfo->emission, pInfo->diffuse, pInfo->reflect, pInfo->specular);
// 	theGeneralSwapData.m_p3DView->NeedLight(
// 		pInfo->light);
// 	theGeneralSwapData.m_p3DView->NeedShadow(
// 		pInfo->shadow);
	return 0;
}

LRESULT SeGeneralModuleCtrlDlg::OnSetLightState(WPARAM wParam, LPARAM lParam)
{
	LIGHT_INFO* pInfo = (LIGHT_INFO*)wParam;
	theGeneralSwapData.m_p3DView->SetLightPos(
		pInfo->lightPosition.fXpos,
		pInfo->lightPosition.fYpos,
		pInfo->lightPosition.fZpos
		);
	theGeneralSwapData.m_p3DView->SetLightColor(
		pInfo->LightColor);
	theGeneralSwapData.m_p3DView->SetMaterialColor(
		pInfo->MaterialColor);
	theGeneralSwapData.m_p3DView->SetLightMat(
		pInfo->emission, pInfo->diffuse, pInfo->reflect, pInfo->specular);
	theGeneralSwapData.m_p3DView->NeedLight(
		pInfo->light);
	theGeneralSwapData.m_p3DView->NeedShadow(
		pInfo->shadow);
	theGeneralSwapData.m_p3DView->SetShadow(
		pInfo->shadowD);
	return 0;
}

void SeGeneralModuleCtrlDlg::Reset()
{
	m_lst2DPlane.DeleteAllItems();
	m_lst3DPlane.DeleteAllItems();
	m_lst3DPlane2.DeleteAllItems();
	if (!m_bRayCasting && theGeneralSwapData.m_p3DView != NULL)
	{
		//m_pTransFuncDlg->ShowWindow(SW_HIDE);
		Safe_Delete(m_pTransFuncDlg);
		theGeneralSwapData.m_p3DView->ReleaseVolumeData();
		theGeneralSwapData.m_p3DView->SetRenderMode(TRUE);
		theGeneralSwapData.m_p3DView->ChangeShowState(SHOW_ALL);
		m_bRayCasting = !m_bRayCasting;
	}

	m_nColorIndex = 0;
}



void SeGeneralModuleCtrlDlg::OnBnClickedPrintscreen3d()
{
	// TODO: 在此添加控件通知处理程序代
	theGeneralSwapData.m_p3DView->GetPrintScreen(theGeneralSwapData.m_csFolderPath);
}



LRESULT SeGeneralModuleCtrlDlg::OnSetTransFunc(WPARAM wParam, LPARAM lParam)
{
	vector <TransFuncColorCtlPoint>* pVecColor = (vector<TransFuncColorCtlPoint>*) wParam; 
	vector <TransFuncLightnessCtlPoint>* pVecLightness = (vector <TransFuncLightnessCtlPoint>*) lParam;

	POSITION pos = m_lst3DPlane2.GetFirstSelectedItemPosition();
	

	vector <float> vecStrengthPos; 
	vector <float> vecStrengthValue;
	vector <float> vecColorPos; 
	vector<COLORREF> vecColorValue;
	int nMinValue = theGeneralSwapData.m_nMinValue; 
	int nMaxValue = theGeneralSwapData.m_nMaxValue;
	for (int i=0; i<pVecLightness->size(); i++)
	{
		vecStrengthPos.push_back((*pVecLightness)[i].ptX);
		vecStrengthValue.push_back((*pVecLightness)[i].ptY);
	}
	for (int i=0; i<pVecColor->size(); i++)
	{
		vecColorPos.push_back((*pVecColor)[i].pos);
		vecColorValue.push_back((*pVecColor)[i].color);
	}
	// 230.0 is the transfunc upPlane window height. don't figure it out how to set this value.
	theGeneralSwapData.m_p3DView->m_pGLDataMgr->AdjustTransFunc(vecStrengthPos, vecStrengthValue, vecColorPos, vecColorValue, nMinValue, nMaxValue, 230.0, pos == NULL? -1 : (int)pos - 1);
	return 0;
}


void SeGeneralModuleCtrlDlg::OnBnClickedRenderMode()
{
	if(g_pGeneralModule->m_OriDcmArray.GetDcmArray().size() == 0)
	{
		AfxMessageBox("请先读入数据！");
		return;
	}
	if (m_bRayCasting)
	{
		//m_pTransFuncDlg->ShowWindow(SW_SHOW);
		m_pTransFuncDlg = new CSeTransFuncDlg(
			&theGeneralSwapData.m_Histogram[0],
			theGeneralSwapData.m_lMaxNumber, 
			theGeneralSwapData.m_nMaxValue, 
			theGeneralSwapData.m_nMinValue,
			this);
		m_pTransFuncDlg->Create(IDD_DIALOG_TRANSFUNC, this);
		m_pTransFuncDlg->ShowWindow(SW_HIDE);
		SendMessage(WM_CLOSE_ALL_WINDOW);
		theGeneralSwapData.m_p3DView->InitVolumeData(&(g_pGeneralModule->m_OriDcmArray));
		theGeneralSwapData.m_p3DView->SetRenderMode(FALSE);
		theGeneralSwapData.m_p3DView->ChangeShowState(SHOW_3D);
	}
	else
	{
		m_pTransFuncDlg->ShowWindow(SW_HIDE);
		Safe_Delete(m_pTransFuncDlg);
		SendMessage(WM_CLOSE_ALL_WINDOW);
		m_lst3DPlane2.DeleteAllItems();
		theGeneralSwapData.m_p3DView->ReleaseVolumeData();
		theGeneralSwapData.m_p3DView->SetRenderMode(TRUE);
		theGeneralSwapData.m_p3DView->ChangeShowState(SHOW_ALL);
	}
	m_bRayCasting = !m_bRayCasting;
	SendMessage(WM_SIZE);
// 	// TODO: 在此添加控件通知处理程序代码
// 	// TODO: 在此添加控件通知处理程序代码
// 	theGeneralSwapData.m_p3DView->ChangeShowState(SHOW_3D);
// 	CSeTransFuncDlg dlg(
// 		&theGeneralSwapData.m_Histogram[0],
// 		theGeneralSwapData.m_lMaxNumber, 
// 		theGeneralSwapData.m_nMaxValue, 
// 		theGeneralSwapData.m_nMinValue,
// 		this);
// 	//	CSeTransFuncDlg dlg;
// 	theGeneralSwapData.m_p3DView->SetRenderMode(FALSE);
// 	theGeneralSwapData.m_p3DView->InitVolumeData(&(g_pGeneralModule->m_OriDcmArray));
// 	dlg.DoModal();
// 	theGeneralSwapData.m_p3DView->ReleaseVolumeData();
// 	theGeneralSwapData.m_p3DView->SetRenderMode(TRUE);
// 	theGeneralSwapData.m_p3DView->ChangeShowState(SHOW_ALL);
}


void SeGeneralModuleCtrlDlg::OnBnClickedTransFunc()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_pTransFuncDlg == NULL)
		return;
	if (m_pTransFuncDlg->IsWindowVisible())
	{
		m_pTransFuncDlg->ShowWindow(SW_HIDE);
	}
	else
	{
		m_pTransFuncDlg->ShowWindow(SW_SHOW);
	}
}


void SeGeneralModuleCtrlDlg::OnBnClickedFreeCut()
{
// 	// TODO: 在此添加控件通知处理程序代码
// 	if (m_pFreeCutDlg == NULL)
// 		return;
// 	if (m_pFreeCutDlg->IsWindowVisible())
// 	{
// 		m_pFreeCutDlg->ShowWindow(SW_HIDE);
// 		theGeneralSwapData.m_p3DView->ShowBorder(FALSE);
// 	}
// 	else
// 	{
// 		m_pFreeCutDlg->ShowWindow(SW_SHOW);
// 		theGeneralSwapData.m_p3DView->ShowBorder(TRUE);
// 	}
	// need processes this message in FreeCutDlg,
	// so transform the real operation to a other message.
	
	
	
	OnCloseFreeCutWindow(0, 0);
}

LRESULT SeGeneralModuleCtrlDlg::OnAdjustFreeCut(WPARAM wParam, LPARAM lParam)
{
	int* pRangers = (int*)wParam;
	float fRangers[6];
	fRangers[0] = ((float)pRangers[0] - 5000.0f)/10000.0f;
	fRangers[1] = ((float)pRangers[1] - 5000.0f)/10000.0f;
	fRangers[2] = ((float)pRangers[2] - 5000.0f)/10000.0f;
	fRangers[3] = ((float)pRangers[3] - 5000.0f)/10000.0f;
	fRangers[4] = ((float)pRangers[4] - 5000.0f)/10000.0f;
	fRangers[5] = ((float)pRangers[5] - 5000.0f)/10000.0f;
	theGeneralSwapData.m_p3DView->m_pGLDataMgr->AdjustFreeCutVAO(
		fRangers[0],
		fRangers[1],
		fRangers[2],
		fRangers[3],
		fRangers[4],
		fRangers[5]);

	m_fFreeCutRange[0] = fRangers[0];
	m_fFreeCutRange[1] = fRangers[1];
	m_fFreeCutRange[2] = fRangers[2];
	m_fFreeCutRange[3] = fRangers[3];
	m_fFreeCutRange[4] = fRangers[4];
	m_fFreeCutRange[5] = fRangers[5];

	return 0;
}

LRESULT SeGeneralModuleCtrlDlg::OnCloseFreeCutWindow(WPARAM wParam, LPARAM lParam)
{
	if (m_pFreeCutDlg == NULL)
		return 0;
	if (m_pFreeCutDlg->IsWindowVisible())
	{
		m_pFreeCutDlg->ShowWindow(SW_HIDE);
		theGeneralSwapData.m_p3DView->ShowBorder(FALSE);
	}
	else
	{
		m_pFreeCutDlg->ShowWindow(SW_SHOW);
		theGeneralSwapData.m_p3DView->ShowBorder(TRUE);
	}
	return 0;
}


void SeGeneralModuleCtrlDlg::OnBnClickedExportFromMask()
{
	// TODO: 在此添加控件通知处理程序代码
	POSITION pos = m_lst2DPlane.GetFirstSelectedItemPosition();
	if (pos == NULL)
	{
		MessageBoxTimeout(NULL, "            未选择有效行！        ", "提示", MB_ICONINFORMATION, 0, 300);
		return;
	}
	BYTE* pByte = SeMPRView::m_vecROIData[(int)pos - 1]->GetData();
	theGeneralSwapData.m_pXOYView->ExportImages(pByte);
}


void SeGeneralModuleCtrlDlg::OnBnClickedButton3dTranslate()
{
	POSITION pos = m_lst3DPlane.GetFirstSelectedItemPosition();
	if (pos == NULL)
		MessageBoxTimeout(NULL, "            未选择有效行！        ", "提示", MB_ICONINFORMATION, 0, 300);
	else
	{
		CSeTranslateDlg dlg(this, (int)pos - 1, theGeneralSwapData.m_p3DView->GetTranslate((int)pos - 1));
		dlg.DoModal();
	}
}

LRESULT SeGeneralModuleCtrlDlg::OnChangeTranslate( WPARAM wParam, LPARAM lParam )
{
	int nTranslate = static_cast<int>(wParam);
	int nRow = static_cast<int>(lParam);
	theGeneralSwapData.m_p3DView->SetTranslate(nRow, nTranslate, m_bRayCasting);
	return 0;
}


LRESULT SeGeneralModuleCtrlDlg::OnCloseAllWindow(WPARAM wParam, LPARAM lParam)
{
	if(m_pTransFuncDlg != NULL) m_pTransFuncDlg->ShowWindow(SW_HIDE);
	if (m_pFreeCutDlg != NULL) m_pFreeCutDlg->ShowWindow(SW_HIDE);
	m_pFreeCutDlg->ShowWindow(SW_HIDE);
	m_pMorPhologyDlg->ShowWindow(SW_HIDE);
	m_pResultDlg->ShowWindow(SW_HIDE);
	m_pROIEditDlg->ShowWindow(SW_HIDE);
	m_pBooleanDlg->ShowWindow(SW_HIDE);
	m_pResultDlg->ShowWindow(SW_HIDE);
	theGeneralSwapData.m_p3DView->ShowBorder(FALSE);

	this->EnableWindow(TRUE);
	theGeneralSwapData.m_pXOYView->SetMouseTool(MT_MPR);
	theGeneralSwapData.m_pXOZView->SetMouseTool(MT_MPR);
	theGeneralSwapData.m_pYOZView->SetMouseTool(MT_MPR);

	return 0;
}


LRESULT SeGeneralModuleCtrlDlg::OnRotateAction(WPARAM wParam, LPARAM lParam)
{
	int* pos = (int*)wParam;
	int during = (int)lParam;
	theGeneralSwapData.m_p3DView->PushRotateCommand(pos[0], pos[3], pos[1], pos[4], pos[2], pos[5], during);
	return 0;
}

LRESULT SeGeneralModuleCtrlDlg::OnScaleAction(WPARAM wParam, LPARAM lParam)
{
	double* pos = (double*)wParam;
	int during = (int)lParam;
	theGeneralSwapData.m_p3DView->PushScaleCommand(pos[0], pos[1], during);
	return 0;
}

LRESULT SeGeneralModuleCtrlDlg::OnCutAction(WPARAM wParam, LPARAM lParam)
{
	int* pos = (int*)wParam;
	float fRangers[6];
	fRangers[0] = ((float)pos[0] - 5000.0f)/10000.0f;
	fRangers[1] = ((float)pos[1] - 5000.0f)/10000.0f;
	fRangers[2] = ((float)pos[2] - 5000.0f)/10000.0f;
	fRangers[3] = ((float)pos[3] - 5000.0f)/10000.0f;
	fRangers[4] = ((float)pos[4] - 5000.0f)/10000.0f;
	fRangers[5] = ((float)pos[5] - 5000.0f)/10000.0f;
	int during = (int)lParam;
	theGeneralSwapData.m_p3DView->PushCutCommand(fRangers[0], fRangers[3], fRangers[1], fRangers[4], fRangers[2], fRangers[5], during);
	return 0;
}

void SeGeneralModuleCtrlDlg::OnBnClickedRandomPick()
{
	// TODO: 在此添加控件通知处理程序代码
	int x = SeMPRView::m_nXPos;
	int y = SeMPRView::m_nYPos;
	int z = SeMPRView:: m_nZPos;

	const int width = theGeneralSwapData.m_nWidth;
	const int height = theGeneralSwapData.m_nWidth;
	const int depth = theGeneralSwapData.m_nDepth;

	default_random_engine e_w; //或者直接在这里改变种子 e(10) 
	e_w.seed(z - 2); //设置新的种子
	normal_distribution<double> u_w(x, width/6); //随机数分布对象 

	default_random_engine e_h; //或者直接在这里改变种子 e(10) 
	e_h.seed(z + 2); //设置新的种子
	normal_distribution<double> u_h(y, height/6); //随机数分布对象 

	default_random_engine e_d; //或者直接在这里改变种子 e(10) 
	e_d.seed(x + 5); //设置新的种子
	normal_distribution<double> u_d(z, depth/6); //随机数分布对象 

	for (int i=0; i<100; i++)
	{
		const int random_x = u_w(e_w);
		const int random_y = u_h(e_h);
		const int random_z = u_d(e_d);

		if (random_x > width || random_x < 0 || random_y > height || random_y < 0 || random_z > depth || random_z < 0)
		{
			i--;
			continue;
		}

		theGeneralSwapData.m_pXOYView->UpdateImage(random_z);
		theGeneralSwapData.m_pYOZView->UpdateImage(random_y);
		theGeneralSwapData.m_pXOZView->UpdateImage(random_x);

		theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
		theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
		theGeneralSwapData.m_pYOZView->Invalidate(FALSE);

		theGeneralSwapData.m_pXOYView->UpdateWindow();
		theGeneralSwapData.m_pXOZView->UpdateWindow();
		theGeneralSwapData.m_pYOZView->UpdateWindow();	
		Sleep(200);
	}
}


void SeGeneralModuleCtrlDlg::OnBnClicked3dOutput()
{
// 	// TODO: 在此添加控件通知处理程序代码
	if (m_fFreeCutRange[0] == 0.0f &&
		m_fFreeCutRange[1] == 0.0f &&
		m_fFreeCutRange[2] == 0.0f &&
		m_fFreeCutRange[3] == 0.0f &&
		m_fFreeCutRange[4] == 0.0f &&
		m_fFreeCutRange[5] == 0.0f
		) {
			MessageBoxTimeout(NULL, "          FreeCut 范围不存在       ", "提示", MB_ICONINFORMATION, 0, 300);
			return;
	}


	glm::mat4 matrix_total = glm::mat4(1.0f);


	glm::vec4 objMove = theGeneralSwapData.m_p3DView->m_pGLCamera->GetMove();
	glm::mat4 objModel;
	objModel = glm::mat4(1.0f);//m_pGLMatrix->GetMatrix();
	objModel = glm::translate(objModel, glm::vec3(objMove.x - 0.5f, objMove.y - 0.5f, objMove.z - 0.5f));

	glm::mat4 objRotate = theGeneralSwapData.m_p3DView->m_pGLCamera->GetMatrix();
	
	
	glm::vec4 borMove = theGeneralSwapData.m_p3DView->m_pGLCamera->GetFreeCutMove();	
	glm::mat4 borModel;
	borModel = glm::mat4(1.0f);//m_pGLMatrix->GetMatrix();
	borModel = glm::translate(borModel, glm::vec3(borMove.x - 0.5f, borMove.y - 0.5f, borMove.z - 0.5f));
	
	glm::mat4 borRotate = theGeneralSwapData.m_p3DView->m_pGLCamera->GetFreeCutMatrix();

// 	matrix_total *= objRotate;
// 	matrix_total *= objModel;
// 	matrix_total *= glm::inverse(borModel);
// 	matrix_total *= glm::inverse(borRotate);

 	matrix_total *= glm::inverse(borModel) * glm::inverse(borRotate) * objRotate * objModel;
	//matrix_total *= objModel * objRotate * glm::inverse(borRotate) * glm::inverse(borModel);
// 
// 	glm::inverse(borModel) *= objModel;


	//theGeneralSwapData.m_pXOYView->ExportFreeCutImages(m_fFreeCutRange[0],
		//m_fFreeCutRange[1], m_fFreeCutRange[2], m_fFreeCutRange[3], m_fFreeCutRange[4], m_fFreeCutRange[5], borModel, borRotate, objRotate, objModel);

	theGeneralSwapData.m_pXOYView->MultiThreadFreecut(m_fFreeCutRange[0],
		m_fFreeCutRange[1], m_fFreeCutRange[2], m_fFreeCutRange[3], m_fFreeCutRange[4], m_fFreeCutRange[5], borModel, borRotate, objRotate, objModel);

	// theGeneralSwapData.m_p3DView->m_pGLShader->RefleshShader();
}


void SeGeneralModuleCtrlDlg::OnCbnSelchangeTemplate()
{
	// TODO: 在此添加控件通知处理程序代码
	int nIndex = m_templateCtl.GetCurSel();
	int nWinCenter, nWinWidth;
	switch(nIndex) {
	case 0:
		nWinCenter = 2048;
		nWinWidth = 4096;
		break;
	case 1:
		nWinCenter = 2000;
		nWinWidth = 3000;
		break;
	case 2:
		nWinCenter = 1000;
		nWinWidth = 2000;
		break;
	case 3:
		nWinCenter = 2000;
		nWinWidth = 2000;
		break;
	default:
		break;
	}
	theGeneralSwapData.m_pXOYView->SetWinLevel(nWinCenter, nWinWidth);
	theGeneralSwapData.m_pXOZView->SetWinLevel(nWinCenter, nWinWidth);
	theGeneralSwapData.m_pYOZView->SetWinLevel(nWinCenter, nWinWidth);
}


void SeGeneralModuleCtrlDlg::OnBnClickedToolLine()
{
	theGeneralSwapData.m_pXOYView->SetMouseTool(MT_MEASURE_LINE);
	theGeneralSwapData.m_pXOZView->SetMouseTool(MT_MEASURE_LINE);
	theGeneralSwapData.m_pYOZView->SetMouseTool(MT_MEASURE_LINE);

	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);
	m_pResultDlg->ShowWindow(SW_SHOW);
}


void SeGeneralModuleCtrlDlg::OnBnClickedToolAngle()
{
	theGeneralSwapData.m_pXOYView->SetMouseTool(MT_MEASURE_ANGLE);
	theGeneralSwapData.m_pXOZView->SetMouseTool(MT_MEASURE_ANGLE);
	theGeneralSwapData.m_pYOZView->SetMouseTool(MT_MEASURE_ANGLE);

	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);
	m_pResultDlg->ShowWindow(SW_SHOW);
}


void SeGeneralModuleCtrlDlg::OnBnClickedToolShape()
{
	theGeneralSwapData.m_pXOYView->SetMouseTool(MT_MEASURE_SHAPE);
	theGeneralSwapData.m_pXOZView->SetMouseTool(MT_MEASURE_SHAPE);
	theGeneralSwapData.m_pYOZView->SetMouseTool(MT_MEASURE_SHAPE);

	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);
	m_pResultDlg->ShowWindow(SW_SHOW);
	
}


void SeGeneralModuleCtrlDlg::OnBnClickedToolValue()
{
	theGeneralSwapData.m_pXOYView->SetMouseTool(MT_MEASURE_CT);
	theGeneralSwapData.m_pXOZView->SetMouseTool(MT_MEASURE_CT);
	theGeneralSwapData.m_pYOZView->SetMouseTool(MT_MEASURE_CT);

	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);
	m_pResultDlg->ShowWindow(SW_SHOW);
}


void SeGeneralModuleCtrlDlg::OnBnClickedToolArea()
{
	theGeneralSwapData.m_pXOYView->SetMouseTool(MT_MEASURE_AREA);
	theGeneralSwapData.m_pXOZView->SetMouseTool(MT_MEASURE_AREA);
	theGeneralSwapData.m_pYOZView->SetMouseTool(MT_MEASURE_AREA);

	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);
	m_pResultDlg->ShowWindow(SW_SHOW);
}


void SeGeneralModuleCtrlDlg::OnBnClickedToolEllipse()
{
	theGeneralSwapData.m_pXOYView->SetMouseTool(MT_MEASURE_ELLIPSE);
	theGeneralSwapData.m_pXOZView->SetMouseTool(MT_MEASURE_ELLIPSE);
	theGeneralSwapData.m_pYOZView->SetMouseTool(MT_MEASURE_ELLIPSE);

	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);
	if (m_pResultDlg != NULL)
	{
		m_pResultDlg->ShowWindow(SW_SHOW);
	}
}


void SeGeneralModuleCtrlDlg::OnBnClickedToolSelect()
{
	SendMessage(WM_CLOSE_ALL_WINDOW);
	if(!m_bMaskTool) {
		SeMPRView::m_bShowMask = FALSE;
		m_bMaskTool = TRUE;
		m_pResultDlg->ShowWindow(SW_SHOW);
		CRect rectRst;
		m_pResultDlg->GetWindowRect(&rectRst);
		CRect rect;
		theGeneralSwapData.m_p3DView->GetWindowRect(&rect);
		rect.right = rect.left + rectRst.Width();
		rect.bottom = rect.top + rectRst.Height();
		m_pResultDlg->MoveWindow(&rect);
		theGeneralSwapData.m_pXOYView->SetMouseTool(MT_MEASURE_SELECT);
		theGeneralSwapData.m_pXOZView->SetMouseTool(MT_MEASURE_SELECT);
		theGeneralSwapData.m_pYOZView->SetMouseTool(MT_MEASURE_SELECT);
	}
	else {
		SeMPRView::m_bShowMask = TRUE;
		theGeneralSwapData.m_pXOYView->SendMessage(WM_COMMAND, IDM_IMAGEVIEWER_ZOOM_100);
		theGeneralSwapData.m_pXOZView->SendMessage(WM_COMMAND, IDM_IMAGEVIEWER_ZOOM_100);
		theGeneralSwapData.m_pYOZView->SendMessage(WM_COMMAND, IDM_IMAGEVIEWER_ZOOM_100);
		theGeneralSwapData.m_pXOYView->SetMouseTool(MT_MPR);
		theGeneralSwapData.m_pXOZView->SetMouseTool(MT_MPR);
		theGeneralSwapData.m_pYOZView->SetMouseTool(MT_MPR);
		m_pResultDlg->ShowWindow(SW_HIDE);
		m_bMaskTool = FALSE;
	}

	SendMessage(WM_SIZE);
	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);
}


void SeGeneralModuleCtrlDlg::OnBnClickedExportWithMask()
{
	// TODO: 在此添加控件通知处理程序代码
	theGeneralSwapData.m_pXOYView->ExportImagesWithMask();
	//theGeneralSwapData.m_pXOYView->ExportCurrentImageWithMask();
	//theGeneralSwapData.m_pXOZView->ExportCurrentImageWithMask();
	//theGeneralSwapData.m_pYOZView->ExportCurrentImageWithMask();
}




void SeGeneralModuleCtrlDlg::OnNMCustomdrawSliderStepScale(NMHDR *pNMHDR, LRESULT *pResult)
{
	UNREFERENCED_PARAMETER(pNMHDR);
	*pResult = CDRF_DODEFAULT;
}


void SeGeneralModuleCtrlDlg::OnNMCustomdrawSliderOffsetScale(NMHDR *pNMHDR, LRESULT *pResult)
{
	UNREFERENCED_PARAMETER(pNMHDR);
	*pResult = CDRF_DODEFAULT;
}




void SeGeneralModuleCtrlDlg::OnBnClicked3dLight()
{
	// TODO: 在此添加控件通知处理程序代码
	if(g_pGeneralModule->m_OriDcmArray.GetDcmArray().size() == 0)
	{
		AfxMessageBox("请先读入数据！");
		return;
	}

	if (m_pLightDlg->IsWindowVisible())
		m_pLightDlg->ShowWindow(SW_HIDE);
	else
	{
		theGeneralSwapData.m_pXOYView->SetMPRTool();
		theGeneralSwapData.m_pXOZView->SetMPRTool();
		theGeneralSwapData.m_pYOZView->SetMPRTool();

		m_pMorPhologyDlg->ShowWindow(SW_HIDE);
		m_pROIEditDlg->ShowWindow(SW_HIDE);
		m_pBooleanDlg->ShowWindow(SW_HIDE);

		m_pLightDlg->ShowWindow(SW_SHOW);
	}
	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);
}


void SeGeneralModuleCtrlDlg::OnBnClickedExportSharp()
{
	// TODO: 在此添加控件通知处理程序代码
	theGeneralSwapData.m_pXOYView->ExportSharpImages();
}


void SeGeneralModuleCtrlDlg::OnBnClickedButton2dSave()
{
	// TODO: 在此添加控件通知处理程序代码
	POSITION pos = m_lst2DPlane.GetFirstSelectedItemPosition();
	if (pos == NULL)
		MessageBoxTimeout(NULL, "            未选择有效行！        ", "提示", MB_ICONINFORMATION, 0, 300);
	else
	{

		//SeMPRView::m_vecMaskInfo.erase(SeMPRView::m_vecMaskInfo.begin() + (int)pos - 1);	
		BYTE* pByte = SeMPRView::m_vecROIData[(int)pos - 1]->GetData();
		theGeneralSwapData.m_pXOYView->ExportMask(pByte);
	}
}


void SeGeneralModuleCtrlDlg::OnBnClickedButton2dLoad()
{
	// TODO: 在此添加控件通知处理程序代码
	if(g_pGeneralModule->m_OriDcmArray.GetDcmArray().size() == 0)
	{
		AfxMessageBox("请先读入数据！");
		return;
	}
	CFileDialog filedlg(true);
	if (IDCANCEL == filedlg.DoModal())
		return;
	CString strFilePath = filedlg.GetPathName();
	CString strFileName = filedlg.GetFileName();

	int nRow = m_lst2DPlane.InsertItem(m_lst2DPlane.GetItemCount(), "    ");// 插入行
	m_lst2DPlane.SetItemText(nRow, 1, "true");// 设置数据
	m_lst2DPlane.SetItemText(nRow, 2, "-");// 设置数据
	m_lst2DPlane.SetItemText(nRow, 3, "-");// 设置数据
	m_lst2DPlane.SetCheck(nRow, TRUE);
	m_lst2DPlane.SetItemState(nRow, LVNI_FOCUSED | LVIS_SELECTED, LVNI_FOCUSED | LVIS_SELECTED);
	MaskInfo mkinfo(0, 0, m_ColorLst[m_nColorIndex], 64, TRUE);
	theGeneralSwapData.m_pXOYView->LoadMask(strFilePath, mkinfo);
	m_nColorIndex = (m_nColorIndex + 1) % 8;

	theGeneralSwapData.m_pXOYView->m_bNewMask = FALSE;
	theGeneralSwapData.m_pYOZView->m_bNewMask = FALSE;
	theGeneralSwapData.m_pXOZView->m_bNewMask = FALSE;
	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
}


void SeGeneralModuleCtrlDlg::OnBnClicked3dLine()
{
	// TODO: 在此添加控件通知处理程序代码
}


void SeGeneralModuleCtrlDlg::OnBnClickedToolScale()
{
	// TODO: 在此添加控件通知处理程序代码
	theGeneralSwapData.m_pXOYView->SetMouseTool(MT_Zoom);
	theGeneralSwapData.m_pXOZView->SetMouseTool(MT_Zoom);
	theGeneralSwapData.m_pYOZView->SetMouseTool(MT_Zoom);

	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);
	m_pResultDlg->ShowWindow(SW_SHOW);
}


void SeGeneralModuleCtrlDlg::OnBnClickedToolReset()
{
	// TODO: 在此添加控件通知处理程序代码
	theGeneralSwapData.m_pXOYView->SendMessage(WM_COMMAND, IDM_IMAGEVIEWER_ZOOM_100);
	theGeneralSwapData.m_pXOZView->SendMessage(WM_COMMAND, IDM_IMAGEVIEWER_ZOOM_100);
	theGeneralSwapData.m_pYOZView->SendMessage(WM_COMMAND, IDM_IMAGEVIEWER_ZOOM_100);
	theGeneralSwapData.m_pXOYView->SetMouseTool(MT_MEASURE_SELECT);
	theGeneralSwapData.m_pXOZView->SetMouseTool(MT_MEASURE_SELECT);
	theGeneralSwapData.m_pYOZView->SetMouseTool(MT_MEASURE_SELECT);

	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);
	m_pResultDlg->ShowWindow(SW_SHOW);
}


void SeGeneralModuleCtrlDlg::OnBnClickedMovieRecord()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_pMovieDlg == NULL)
		return;
	if (m_pMovieDlg->IsWindowVisible())
	{
		m_pMovieDlg->ShowWindow(SW_HIDE);
	}
	else
	{
		m_pMovieDlg->ShowWindow(SW_SHOW);
	}
}





void SeGeneralModuleCtrlDlg::OnBnClickedButtonSaveMask()
{
	// TODO: 在此添加控件通知处理程序代码
	POSITION pos = m_lst2DPlane.GetFirstSelectedItemPosition();
	if (pos == NULL)
		MessageBoxTimeout(NULL, "            未选择有效行！        ", "提示", MB_ICONINFORMATION, 0, 300);
	else
	{

		//SeMPRView::m_vecMaskInfo.erase(SeMPRView::m_vecMaskInfo.begin() + (int)pos - 1);	
		BYTE* pByte = SeMPRView::m_vecROIData[(int)pos - 1]->GetData();
		theGeneralSwapData.m_pXOYView->ExportMask(pByte);
	}
}


void SeGeneralModuleCtrlDlg::OnBnClickedButtonLoadMask()
{
	// TODO: 在此添加控件通知处理程序代码
	if(g_pGeneralModule->m_OriDcmArray.GetDcmArray().size() == 0)
	{
		AfxMessageBox("请先读入数据！");
		return;
	}
	CFileDialog filedlg(true);
	if (IDCANCEL == filedlg.DoModal())
		return;
	CString strFilePath = filedlg.GetPathName();
	CString strFileName = filedlg.GetFileName();

	int nRow = m_lst2DPlane.InsertItem(m_lst2DPlane.GetItemCount(), "    ");// 插入行
	m_lst2DPlane.SetItemText(nRow, 1, "true");// 设置数据
	m_lst2DPlane.SetItemText(nRow, 2, "-");// 设置数据
	m_lst2DPlane.SetItemText(nRow, 3, "-");// 设置数据
	m_lst2DPlane.SetCheck(nRow, TRUE);
	m_lst2DPlane.SetItemState(nRow, LVNI_FOCUSED | LVIS_SELECTED, LVNI_FOCUSED | LVIS_SELECTED);
	MaskInfo mkinfo(0, 0, m_ColorLst[m_nColorIndex], 64, TRUE);
	theGeneralSwapData.m_pXOYView->LoadMask(strFilePath, mkinfo);
	m_nColorIndex = (m_nColorIndex + 1) % 8;

	theGeneralSwapData.m_pXOYView->m_bNewMask = FALSE;
	theGeneralSwapData.m_pYOZView->m_bNewMask = FALSE;
	theGeneralSwapData.m_pXOZView->m_bNewMask = FALSE;
	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
}


void SeGeneralModuleCtrlDlg::OnBnClickedButton3dAdd2()
{
	//TODO: 在此添加控件通知处理程序代码
	//CStringArray csaFiles;
	//GetFilesName(csaFiles);
	BOOL success = theGeneralSwapData.m_p3DView->AddVolTex(m_ColorLst[m_nColorIndex]);
	if (!success) return;
	m_nColorIndex = (m_nColorIndex + 1) % 8;
	int nCount = m_lst3DPlane2.GetItemCount();
	int nRow = m_lst3DPlane2.InsertItem(nCount, "     ");// 插入行
	m_lst3DPlane2.SetItemText(nRow, 1, "true");// 设置数据
	m_lst3DPlane2.SetCheck(nRow, TRUE);
	m_lst3DPlane2.SetItemState(nRow, LVNI_FOCUSED | LVIS_SELECTED, LVNI_FOCUSED | LVIS_SELECTED);
	
	m_lst3DPlane2.Invalidate(FALSE);
}


void SeGeneralModuleCtrlDlg::OnBnClickedButton3dRemove2()
{
	// TODO: 在此添加控件通知处理程序代码
	POSITION pos = m_lst3DPlane2.GetFirstSelectedItemPosition();
	if (pos == NULL)
		MessageBoxTimeout(NULL, "            未选择有效行！        ", "提示", MB_ICONINFORMATION, 0, 300);
	else
	{

		m_lst3DPlane2.DeleteItem((int)pos - 1);
		theGeneralSwapData.m_p3DView->RemoveVolTex((int)pos - 1, FALSE);
	}
}


void SeGeneralModuleCtrlDlg::OnBnClickedButton3dColor2()
{
	// TODO: 在此添加控件通知处理程序代码
	POSITION pos = m_lst3DPlane2.GetFirstSelectedItemPosition();
	if (pos == NULL)
		MessageBoxTimeout(NULL, "            未选择有效行！        ", "提示", MB_ICONINFORMATION, 0, 300);
	else
	{
		COLORREF color = RGB(255, 0, 0);      // 颜色对话框的初始颜色为红色  
		CColorDialog colorDlg(color); // 构造颜色对话框，传入初始颜色值 
		if (IDOK == colorDlg.DoModal()) 
		{
			color = colorDlg.GetColor();
			theGeneralSwapData.m_p3DView->ChangeColor((int)pos - 1, color, FALSE);
			m_lst3DPlane2.Invalidate(FALSE);
			theGeneralSwapData.m_p3DView->Invalidate(FALSE);
		}
	}
}


void SeGeneralModuleCtrlDlg::OnBnClickedButton3dTranslate2()
{
	// TODO: 在此添加控件通知处理程序代码
	POSITION pos = m_lst3DPlane2.GetFirstSelectedItemPosition();
	if (pos == NULL)
		MessageBoxTimeout(NULL, "            未选择有效行！        ", "提示", MB_ICONINFORMATION, 0, 300);
	else
	{
		CSeTranslateDlg dlg(this, (int)pos - 1, theGeneralSwapData.m_p3DView->GetTranslate((int)pos - 1, FALSE));
		dlg.DoModal();
	}
}

BOOL SeGeneralModuleCtrlDlg::GetFilesName(CStringArray& csaFiles)
{
	CFileDialog			filedlg(TRUE, "dcm", "*.*", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_ALLOWMULTISELECT , 
		"DICOM Files (*.dcm)|*.dcm|Bitmap (*.bmp)|*.bmp|All Files(*.*)|*.*|");
	// 默认的filedialog文件名储存空间为512B，空间不足，扩充为1024*1024B
	filedlg.m_ofn.nMaxFile = 1024*1024;
	filedlg.m_ofn.lpstrFile = new char[1024*1024+1];
	memset(filedlg.m_ofn.lpstrFile, 0, 1024*1024+1);
	if(filedlg.DoModal() == IDCANCEL)
	{
		delete []filedlg.m_ofn.lpstrFile ;
		return FALSE;
	}

	POSITION	pos = filedlg.GetStartPosition();
	while (pos != NULL)
	{
		CString	cs = filedlg.GetNextPathName(pos);
		csaFiles.Add(cs);
	}
	delete []filedlg.m_ofn.lpstrFile;

	return TRUE;
}

