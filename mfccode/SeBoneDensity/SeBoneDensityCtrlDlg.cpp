// SeBoneDensityCtrlDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "SeBoneDensity.h"
#include "SeBoneDensityDlg.h"
#include "SeBoneDensityCtrlDlg.h"
#include "BoneDensitySwapData.h"
#include "afxdialogex.h"
#include "SeAPRView.h"
#include "SeROIView.h"
#include "SeBinaryView.h"
#include "SeNewMaskDlg.h"
#include "SeBoneSegProcedureDlg.h"
#include "SeBoneParamSelectionDlg.h"
#include "SeBoneParamCaculate.h"
#include "Easy3DViewThread.h"
#include "SeLogger_VS2010.h"          // VS2010兼容版本
#include "SePDFReportGenerator.h"     // libharu PDF生成器（支持中文）


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

// SeBoneDensityCtrlDlg 对话框

IMPLEMENT_DYNAMIC(SeBoneDensityCtrlDlg, CSeDialogBase)

SeBoneDensityCtrlDlg::SeBoneDensityCtrlDlg(CWnd* pParent /*=NULL*/)
	: CSeDialogBase(SeBoneDensityCtrlDlg::IDD, pParent)
	, m_bSquare(TRUE)
{
	m_pThread1 = NULL;
	m_pThread2 = NULL;
	m_bGetEage = FALSE;
	m_bCaculateDirect = FALSE;
	m_bBianry = FALSE;
	m_bSetValue = FALSE;
	m_bSegFuncsUsed = FALSE;
	m_pResult = new SeBoneParamCaculate();

	// 初始化日志系统 - 输出到exe所在目录
	TCHAR exePath[MAX_PATH];
	GetModuleFileName(NULL, exePath, MAX_PATH);
	CString strExePath(exePath);
	int nPos = strExePath.ReverseFind('\\');
	CString logPath = strExePath.Left(nPos) + "\\BoneDensity.log";
	SeLogger::GetInstance().Initialize(logPath);
	LOG_INFO("骨密度分析模块启动");
}

SeBoneDensityCtrlDlg::~SeBoneDensityCtrlDlg()
{
	m_pThread1 = NULL;
	m_pThread2 = NULL;
	Safe_Delete(m_pResult);
	
	// 关闭日志系统
	LOG_INFO("骨密度分析模块关闭");
	SeLogger::GetInstance().Close();
}

void SeBoneDensityCtrlDlg::DoDataExchange(CDataExchange* pDX)
{
	CSeDialogBase::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_BONE_DELETEOUTSIDE, m_nDeleteOutside);
	DDX_Control(pDX, IDC_GROUP_SLICE, m_GroupSlice);
	DDX_Radio(pDX, IDC_BONE_ROUND, m_bSquare);
}


BEGIN_MESSAGE_MAP(SeBoneDensityCtrlDlg, CSeDialogBase)
	ON_WM_ERASEBKGND()
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_TIMER()
	ON_COMMAND(IDOK, &SeBoneDensityCtrlDlg::OnIdok)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_BONE_CHIPTOOL, &SeBoneDensityCtrlDlg::OnBnClickedBoneChiptool)
	ON_BN_CLICKED(IDC_BONE_CREATESERIES, &SeBoneDensityCtrlDlg::OnBnClickedBoneCreateseries)
	ON_BN_CLICKED(IDC_BONE_GETEDGE, &SeBoneDensityCtrlDlg::OnBnClickedBoneGetedge)
	ON_BN_CLICKED(IDC_BONE_DELETEOUTSIDE, &SeBoneDensityCtrlDlg::OnBnClickedBoneDeleteSet)
	ON_BN_CLICKED(IDC_BONE_DELETEINSIDE, &SeBoneDensityCtrlDlg::OnBnClickedBoneDeleteSet)
	ON_BN_CLICKED(IDC_BONE_STARTDIVISION, &SeBoneDensityCtrlDlg::OnBnClickedBoneStartdivision)
	ON_BN_CLICKED(IDC_BONE_CONFIRMREGION, &SeBoneDensityCtrlDlg::OnBnClickedBoneConfirmregion)
	ON_BN_CLICKED(IDC_BONE_PARTBINARY, &SeBoneDensityCtrlDlg::OnBnClickedBonePartbinary)
	ON_BN_CLICKED(IDC_BONE_PARAMETERCALCU, &SeBoneDensityCtrlDlg::OnBnClickedBoneParametercalcu)
	ON_BN_CLICKED(IDC_BONE_DENSITY, &SeBoneDensityCtrlDlg::OnBnClickedBoneDensity)
	ON_BN_CLICKED(IDC_BONE_LOADMODEL, &SeBoneDensityCtrlDlg::OnBnClickedBoneLoadmodel)
	ON_BN_CLICKED(IDC_BONE_DENSITYCALCU, &SeBoneDensityCtrlDlg::OnBnClickedBoneDensitycalcu)
	ON_BN_CLICKED(IDC_BONE_LASTSTEP, &SeBoneDensityCtrlDlg::OnBnClickedBoneLaststep)
	ON_BN_CLICKED(IDC_BONE_PRINTREPORT, &SeBoneDensityCtrlDlg::OnBnClickedBonePrintreport)
    ON_BN_CLICKED(IDC_BONE_DRAWEDGE, &SeBoneDensityCtrlDlg::OnBnClickedBoneDrawedge)
    ON_BN_CLICKED(IDC_BONE_SELECTION, &SeBoneDensityCtrlDlg::OnBnClickedBoneParamSelect)
	ON_BN_CLICKED(IDC_BONE_CLEAREDGE, &SeBoneDensityCtrlDlg::OnBnClickedBoneClearedge)
	ON_BN_CLICKED(IDC_BONE_RECTCONFIRM, &SeBoneDensityCtrlDlg::OnBnClickedBoneRectconfirm)
	ON_BN_CLICKED(IDC_BONE_CLOSE, &SeBoneDensityCtrlDlg::OnBnClickedBoneClose)
	ON_BN_CLICKED(IDC_EXPORT_EXCEL, &SeBoneDensityCtrlDlg::OnBnClickedExportExcel)
	ON_BN_CLICKED(IDC_BONE_CALCULATE, &SeBoneDensityCtrlDlg::OnBnClickedBoneCalculate)
	ON_BN_CLICKED(IDC_BONE_EXPORT_FILE, &SeBoneDensityCtrlDlg::OnBnClickedBoneExportFile)
	ON_BN_CLICKED(IDC_BONE_SHOW_3D, &SeBoneDensityCtrlDlg::OnBnClickedBoneShow3d)
	ON_BN_CLICKED(IDC_BONE_PRINT_3D, &SeBoneDensityCtrlDlg::OnBnClickedBonePrint3d)

	ON_EN_CHANGE(IDC_EDIT_XS, &SeBoneDensityCtrlDlg::OnEnChangeEditXs)
	ON_EN_CHANGE(IDC_EDIT_XE, &SeBoneDensityCtrlDlg::OnEnChangeEditXe)
	ON_EN_CHANGE(IDC_EDIT_YS, &SeBoneDensityCtrlDlg::OnEnChangeEditYs)
	ON_EN_CHANGE(IDC_EDIT_YE, &SeBoneDensityCtrlDlg::OnEnChangeEditYe)
	ON_EN_CHANGE(IDC_EDIT_ZS, &SeBoneDensityCtrlDlg::OnEnChangeEditZs)
	ON_EN_CHANGE(IDC_EDIT_ZE, &SeBoneDensityCtrlDlg::OnEnChangeEditZe)
	ON_EN_KILLFOCUS(IDC_EDIT_XS, &SeBoneDensityCtrlDlg::OnEnKillfocusXs)
	ON_EN_KILLFOCUS(IDC_EDIT_XE, &SeBoneDensityCtrlDlg::OnEnKillfocusXe)
	ON_EN_KILLFOCUS(IDC_EDIT_YS, &SeBoneDensityCtrlDlg::OnEnKillfocusYs)
	ON_EN_KILLFOCUS(IDC_EDIT_YE, &SeBoneDensityCtrlDlg::OnEnKillfocusYe)
	ON_EN_KILLFOCUS(IDC_EDIT_ZS, &SeBoneDensityCtrlDlg::OnEnKillfocusZs)
	ON_EN_KILLFOCUS(IDC_EDIT_ZE, &SeBoneDensityCtrlDlg::OnEnKillfocusZe)
	ON_EN_SETFOCUS(IDC_EDIT_XS, &SeBoneDensityCtrlDlg::OnEnSetfocusXs)
	ON_EN_SETFOCUS(IDC_EDIT_XE, &SeBoneDensityCtrlDlg::OnEnSetfocusXe)
	ON_EN_SETFOCUS(IDC_EDIT_YS, &SeBoneDensityCtrlDlg::OnEnSetfocusYs)
	ON_EN_SETFOCUS(IDC_EDIT_YE, &SeBoneDensityCtrlDlg::OnEnSetfocusYe)
	ON_EN_SETFOCUS(IDC_EDIT_ZS, &SeBoneDensityCtrlDlg::OnEnSetfocusZs)
	ON_EN_SETFOCUS(IDC_EDIT_ZE, &SeBoneDensityCtrlDlg::OnEnSetfocusZe)

	//分割皮质骨
	ON_BN_CLICKED(IDC_BONE_THRE, &SeBoneDensityCtrlDlg::OnBnClickedBoneThre)
	ON_BN_CLICKED(IDC_BONE_SEGFUNC, &SeBoneDensityCtrlDlg::OnBnClickedBoneSegFunc)
	ON_BN_CLICKED(IDC_BONE_CONFIRMREGION2, &SeBoneDensityCtrlDlg::OnBnClickedBoneConfirmregion2)

	// 自订消息
	ON_MESSAGE(WM_MASK, &SeBoneDensityCtrlDlg::OnAddMask) 
	ON_MESSAGE(WM_BONE_FUNCLIST, &SeBoneDensityCtrlDlg::OnBoneSegmentation)
	ON_MESSAGE(WM_BONE_FUNCRESET, &SeBoneDensityCtrlDlg::OnBoneSegReset)
	ON_MESSAGE(WM_BONE_SHOWTWOPARTS, &SeBoneDensityCtrlDlg::OnBoneSegShowALL)
	ON_MESSAGE(WM_BONE_FUNCFINISHED, &SeBoneDensityCtrlDlg::OnBoneSegFinished)
	ON_MESSAGE(WM_BONE_FUNCSHOWMASKSET, &SeBoneDensityCtrlDlg::OnBoneSetShowMaskInfer)
	
	ON_BN_CLICKED(IDC_BONE_EXPORT_FILE_EX, &SeBoneDensityCtrlDlg::OnBnClickedBoneExportFileEx)
	ON_BN_CLICKED(IDC_EXPORT_PDF, &SeBoneDensityCtrlDlg::OnBnClickedExportPdf)
END_MESSAGE_MAP()


// SeBoneDensityCtrlDlg 消息处理程序


BOOL SeBoneDensityCtrlDlg::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CRect		rtClient;
	GetClientRect(&rtClient);
	CRect rtShow(rtClient);
	CZKMemDC		MemDC(pDC, rtShow,RGB(94,94,94)/*afxGlobalData.clrWindow*/);
	return TRUE;
}


int SeBoneDensityCtrlDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
// 	if (CSeDialogBase::OnCreate(lpCreateStruct) == -1)
// 		return -1;
// 	theBoneDensitySwapData.m_pCtrlDlg = this;
// 	m_brush = CreateSolidBrush(afxGlobalData.clrBarDkShadow);
 	return 0;
}

HBRUSH SeBoneDensityCtrlDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
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
		return m_brush;
		//return CreateSolidBrush(afxGlobalData.clrBarDkShadow);
	}
	// TODO:  如果默认的不是所需画笔，则返回另一个画笔
	return hbr;
}

void SeBoneDensityCtrlDlg::OnBnClickedBoneChiptool()
{
	if(g_pBoneDensityModule->m_OriDcmArray.GetDcmArray().size() == 0)
	{
		AfxMessageBox("请先读入数据！");
		return;
	}
	theBoneDensitySwapData.m_nStep++;
	theBoneDensitySwapData.m_pXOYView->SetAPRRectTool();
	theBoneDensitySwapData.m_pYOZView->SetAPRRectTool();
	theBoneDensitySwapData.m_pXOZView->SetAPRRectTool();
	theBoneDensitySwapData.m_pXOYView->Invalidate(FALSE);
	theBoneDensitySwapData.m_pXOYView->UpdateWindow();
	theBoneDensitySwapData.m_pXOZView->Invalidate(FALSE);
	theBoneDensitySwapData.m_pXOZView->UpdateWindow();
	theBoneDensitySwapData.m_pYOZView->Invalidate(FALSE);
	theBoneDensitySwapData.m_pYOZView->UpdateWindow();
	SendMessage(WM_SIZE);
	//g_pBoneDensityModule->m_pBoneDensityDlg->SendMessage(WM_SIZE);
	
	// 记录旋转矩阵信息 - 从APR视图获取
	MprRotateInfo rotateInfo = theBoneDensitySwapData.m_pXOYView->GetRotateInfor();
	CString rotateMsg;
	rotateMsg.Format(_T("[旋转矩阵] 裁剪范围: 0-%d, X轴:(%.2f,%.2f,%.2f), Y轴:(%.2f,%.2f,%.2f), Z轴:(%.2f,%.2f,%.2f)"), 
		theBoneDensitySwapData.m_nRotateDcmSideLength,
		rotateInfo.dWorldX_x, rotateInfo.dWorldX_y, rotateInfo.dWorldX_z,
		rotateInfo.dWorldY_x, rotateInfo.dWorldY_y, rotateInfo.dWorldY_z,
		rotateInfo.dWorldZ_x, rotateInfo.dWorldZ_y, rotateInfo.dWorldZ_z);
	LOG_INFO(rotateMsg);
	
	CString str;
	str.Format("裁剪范围：0 - %d", theBoneDensitySwapData.m_nRotateDcmSideLength);
	GetDlgItem(IDC_STATIC_RANGE)->SetWindowText(str); 
	SetTimer(1, 100, NULL);
	SetTimer(2, 500, NULL);
}


void SeBoneDensityCtrlDlg::OnBnClickedBoneCreateseries()
{
// 	// TODO: 在此添加控件通知处理程序代码
	theBoneDensitySwapData.m_nStep++;
	// 椭圆切割暂未开放

	int na = theBoneDensitySwapData.m_nScreenWidth;
	int nb = theBoneDensitySwapData.m_nScreenHeight;
	int nc = theBoneDensitySwapData.m_nRotateDcmSideLength;


	int nZStart = int(SeAPRView::m_nZStart * nc/ nb);
	int nZEnd   = int(SeAPRView::m_nZEnd * nc/ nb);
	int nYStart = int(SeAPRView::m_nYStart * nc/ nb);
	int nYEnd   = int(SeAPRView::m_nYEnd * nc/ nb);
	int nXStart = int((SeAPRView::m_nXStart - (na - nb)/2) * nc/ nb);
	int nXEnd   = int((SeAPRView::m_nXEnd - (na - nb)/2) * nc/ nb);

	nZStart = nZStart > 0 ? nZStart : 0;
	nZEnd   = nZEnd   < nc ? nZEnd : nc;
	nYStart = nYStart > 0 ? nYStart : 0;
	nYEnd   = nYEnd   < nc ? nYEnd : nc;
	nXStart = nXStart > 0 ? nXStart : 0;
	nXEnd   = nXEnd   < nc ? nXEnd : nc;
	UpdateData(TRUE);
	if (m_bSquare)
	{
		g_pBoneDensityModule->CutImage(
			nXStart, 
			nXEnd, 
			nYStart, 
			nYEnd, 
			nZStart, 
			nZEnd, 
			SQUARE);

	}
	else
	{
		g_pBoneDensityModule->CutImage(
			nXStart, 
			nXEnd, 
			nYStart, 
			nYEnd, 
			nZStart, 
			nZEnd, 
			ROUND);

	}


	theBoneDensitySwapData.m_pROIView->SetDcmArray(&(g_pBoneDensityModule->m_SliceArray));
	theBoneDensitySwapData.m_pROIView->SetShowNewMask(FALSE);
	SendMessage(WM_SIZE);
	g_pBoneDensityModule->GetUI()->SendMessage(WM_SIZE);
	CString msg;
	msg.Format("[圈定区域] 圈定范围 X:%d-%d, Y:%d-%d, Z:%d-%d", nXStart, nXEnd, nYStart, nYEnd, nZStart, nZEnd);
	LOG_INFO(msg);

// 	//theBoneDensitySwapData.m_pParentDlg->GetSeries();
// 	theBoneDensitySwapData.m_pParentDlg->GetROIArray();
// 	theBoneDensitySwapData.m_pBoneDensityView->SetProcessArray(&theBoneDensitySwapData.m_pParentDlg->m_RotateArray);
// 	theBoneDensitySwapData.m_nStep=SP_CALCU;

}


void SeBoneDensityCtrlDlg::OnBnClickedBoneGetedge()
{
	// TODO: 在此添加控件通知处理程序代码
	theBoneDensitySwapData.m_pROIView->FillMidLayer();
	theBoneDensitySwapData.m_pROIView->Invalidate(FALSE);
	m_bGetEage = TRUE;
	SendMessage(WM_SIZE);
}


void SeBoneDensityCtrlDlg::OnBnClickedBoneDeleteSet()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	switch(m_nDeleteOutside)
	{
	case 0:
		theBoneDensitySwapData.m_pROIView->SetClipOutside(TRUE);

		break;
	case 1:
		theBoneDensitySwapData.m_pROIView->SetClipOutside(FALSE);
		break;
	}	
}


void SeBoneDensityCtrlDlg::OnSize(UINT nType, int cx, int cy)
{
	CSeDialogBase::OnSize(nType, cx, cy);
	CWnd* pWnd = GetDlgItem(IDC_BONE_CHIPTOOL);

	CRect rect;
	GetClientRect(&rect);

	CRect rtHide(0,0,0,0);
	
	CRect rtShow(&rect);
	rtShow.bottom = rtShow.top + 40;
	rtShow.DeflateRect(4,2,4,2);


	CRect rtLastStep(&rect);
	rtLastStep.top = rtLastStep.bottom - 40;
	rtLastStep.DeflateRect(2,2,2,2);

	// 确认按钮已经初始化完成
	if (pWnd)
	{
		// CGroupControl 类的区域不可以设为0，否则不能计算 group 内的按钮，将 CGroupControl 的 Rect 放到一个没有其他控件的地方
	    // 假如其他按键位置出错，应该是 CGroupControl 的 Rect 出错。
		m_GroupSlice.MoveWindow(CRect(500, 500, 800, 800));	
		//m_GroupSave.MoveWindow(CRect(800, 500, 1100, 800));
		GetDlgItem(IDC_BONE_CHIPTOOL		)->MoveWindow(rtHide);
		GetDlgItem(IDC_BONE_CREATESERIES	)->MoveWindow(rtHide);
		GetDlgItem(IDC_BONE_GETEDGE			)->MoveWindow(rtHide);
		GetDlgItem(IDC_BONE_STARTDIVISION	)->MoveWindow(rtHide);
		GetDlgItem(IDC_BONE_CONFIRMREGION	)->MoveWindow(rtHide);
		GetDlgItem(IDC_BONE_PARTBINARY		)->MoveWindow(rtHide);
		GetDlgItem(IDC_EXPORT_EXCEL			)->MoveWindow(rtHide);
		GetDlgItem(IDC_EXPORT_PDF 			)->MoveWindow(rtHide);
		GetDlgItem(IDC_BONE_DRAWEDGE		)->MoveWindow(rtHide);
		GetDlgItem(IDC_BONE_PARAMETERCALCU	)->MoveWindow(rtHide);
		GetDlgItem(IDC_BONE_CLEAREDGE		)->MoveWindow(rtHide);
		GetDlgItem(IDC_BONE_LASTSTEP		)->MoveWindow(rtHide);
		GetDlgItem(IDC_BONE_CALCULATE       )->MoveWindow(rtHide);
		GetDlgItem(IDC_BONE_DELETEOUTSIDE   )->MoveWindow(rtHide);
		GetDlgItem(IDC_BONE_DELETEINSIDE    )->MoveWindow(rtHide);
		GetDlgItem(IDC_BONE_EXPORT_FILE     )->MoveWindow(rtHide);
		GetDlgItem(IDC_BONE_SHOW_3D         )->MoveWindow(rtHide);
		GetDlgItem(IDC_BONE_PRINT_3D        )->MoveWindow(rtHide);
		GetDlgItem(IDC_BONE_ROUND           )->MoveWindow(rtHide);
		GetDlgItem(IDC_BONE_SQUARE          )->MoveWindow(rtHide);

		//皮质骨松质骨
		GetDlgItem(IDC_BONE_THRE			)->MoveWindow(rtHide);
		GetDlgItem(IDC_BONE_SEGFUNC			)->MoveWindow(rtHide);
		GetDlgItem(IDC_BONE_CONFIRMREGION2	)->MoveWindow(rtHide);

		

		switch(theBoneDensitySwapData.m_nStep)
		{
		case SP_SELECTZ:
			{
 				CRect rtBtn(rtShow);
				CRect rtRadio(&rtBtn);
				CRect rtBtn2(&rtBtn);
				rtRadio.right = rtRadio.left + rtRadio.Width()/2;
				GetDlgItem(IDC_BONE_SQUARE)->MoveWindow(rtRadio);
				rtRadio.OffsetRect(rtRadio.Width(), 0);
				GetDlgItem(IDC_BONE_ROUND)->MoveWindow(rtRadio);
				rtBtn.OffsetRect(0, 40);
 				GetDlgItem(IDC_BONE_CHIPTOOL)->MoveWindow(rtBtn);
 				rtBtn.OffsetRect(0, 40);
 				GetDlgItem(IDC_BONE_PARAMETERCALCU)->MoveWindow(rtBtn);
			}
			break;
		case SP_ROTATEDATA:
			{
				CRect rtBtn(rtShow);
				GetDlgItem(IDC_BONE_CREATESERIES)->MoveWindow(rtBtn);
 				rtBtn.OffsetRect(0, 40);
				rtBtn.bottom = rtBtn.top + rtBtn.Height()*6.5;
				m_GroupSlice.MoveWindow(rtBtn);
				GetDlgItem(IDC_BONE_LASTSTEP)->MoveWindow(rtLastStep);
			}
			break;		
		case SP_CALCU:
			{
				CRect rtBtn(&rtShow);
				CRect rtRadio(&rtBtn);
				CRect rtBtn2(&rtBtn);
				rtRadio.right = rtRadio.left + rtRadio.Width()/2;
				GetDlgItem(IDC_BONE_DELETEOUTSIDE)->MoveWindow(rtRadio);
				rtRadio.OffsetRect(rtRadio.Width(), 0);
				GetDlgItem(IDC_BONE_DELETEINSIDE)->MoveWindow(rtRadio);
				rtBtn.OffsetRect(0, 40);
				GetDlgItem(IDC_BONE_DRAWEDGE)->MoveWindow(rtBtn);
				rtBtn.OffsetRect(0, 40);
				GetDlgItem(IDC_BONE_CLEAREDGE)->MoveWindow(rtBtn);
				rtBtn.OffsetRect(0, 40);
				GetDlgItem(IDC_BONE_GETEDGE)->MoveWindow(rtBtn);
				rtBtn.OffsetRect(0, 40);
				if (m_bGetEage)
				{
					GetDlgItem(IDC_BONE_CONFIRMREGION)->MoveWindow(rtBtn);
					rtBtn.OffsetRect(0, 40);
				}

				//rtBtn2.OffsetRect(0, 240);
				//GetDlgItem(IDC_BONE_THRE)->MoveWindow(rtBtn2);
				//rtBtn2.OffsetRect(0, 40);
				//GetDlgItem(IDC_BONE_SEGFUNC)->MoveWindow(rtBtn2);
				//rtBtn2.OffsetRect(0, 40);
				//GetDlgItem(IDC_BONE_CONFIRMREGION2)->MoveWindow(rtBtn2);


// 				rtBtn2.OffsetRect(0, 40);
// 				GetDlgItem(IDC_BONE_REVERSE)->MoveWindow(rtBtn2);
// 				rtBtn2.OffsetRect(0, 40);
// 				GetDlgItem(IDC_BONE_MAXAREA)->MoveWindow(rtBtn2);
// 				rtBtn2.OffsetRect(0, 160);
// 				GetDlgItem(IDC_BONE_CONFIRMREGION2)->MoveWindow(rtBtn2);

// 				CRect rtPart3(&rtShowPart3);
// 				rtPart3.OffsetRect(0, 320);
// 				GetDlgItem(IDC_BONE_OPENF)->MoveWindow(rtPart3);
// 				rtPart3.OffsetRect(0, 60);
// 				GetDlgItem(IDC_BONE_OPENE)->MoveWindow(rtPart3);

// 				CRect rtPart1(&rtShowPart1);
// 				CRect rtPart2(&rtShowPart2);
// 				rtPart1.OffsetRect(0, 320);
// 				rtPart2.OffsetRect(0, 320);
// 				GetDlgItem(IDC_STATIC_F_TYPE)->MoveWindow(rtPart1);
// 				GetDlgItem(IDC_COMBO_F_TYPE)->MoveWindow(rtPart2);
// 
// 				rtPart1.OffsetRect(0, 30);
// 				rtPart2.OffsetRect(0, 30);
// 				GetDlgItem(IDC_STATIC_F_SIZE)->MoveWindow(rtPart1);
// 				GetDlgItem(IDC_EDIT_FKERNEL)->MoveWindow(rtPart2);
// 
// 				rtPart1.OffsetRect(0, 30);
// 				rtPart2.OffsetRect(0, 30);
// 				GetDlgItem(IDC_STATIC_E_TYPE)->MoveWindow(rtPart1);
// 				GetDlgItem(IDC_COMBO_E_TYPE)->MoveWindow(rtPart2);
// 
// 				rtPart1.OffsetRect(0, 30);
// 				rtPart2.OffsetRect(0, 30);
// 				GetDlgItem(IDC_STATIC_E_SIZE)->MoveWindow(rtPart1);
// 				GetDlgItem(IDC_EDIT_EKERNEL)->MoveWindow(rtPart2);
 

// 



				GetDlgItem(IDC_BONE_LASTSTEP)->MoveWindow(rtLastStep);
				rtLastStep.OffsetRect(0, -40);
				GetDlgItem(IDC_BONE_SHOW_3D)->MoveWindow(rtLastStep);
				rtLastStep.OffsetRect(0, -40);
				if (m_pThread1 != NULL && m_pThread1->GetDlgWnd() != NULL)
				{
					GetDlgItem(IDC_BONE_PRINT_3D)->MoveWindow(rtLastStep);
					rtLastStep.OffsetRect(0, -40);
				}

				GetDlgItem(IDC_BONE_EXPORT_FILE_EX)->MoveWindow(rtLastStep);
				rtLastStep.OffsetRect(0, -40);
				GetDlgItem(IDC_BONE_EXPORT_FILE)->MoveWindow(rtLastStep);
				rtLastStep.OffsetRect(0, -40);
// 				CRect rtSf(rtLastStep);
// 				rtSf.top = rtLastStep.top - 40*2.5;
// 				m_GroupSave.MoveWindow(rtSf);
			}
			break;
		case SP_BINARY:
			{
				CRect rtBtn(rtShow);
				GetDlgItem(IDC_BONE_PARTBINARY)->MoveWindow(rtBtn);
				rtBtn.OffsetRect(0, 40);
				if (m_bBianry)
				{
					GetDlgItem(IDC_BONE_CALCULATE)->MoveWindow(rtBtn);
					rtBtn.OffsetRect(0, 40);
					GetDlgItem(IDC_EXPORT_EXCEL)->MoveWindow(rtBtn);
					rtBtn.OffsetRect(0, 40);
					GetDlgItem(IDC_EXPORT_PDF)->MoveWindow(rtBtn);
					rtBtn.OffsetRect(0, 40);
				}
				GetDlgItem(IDC_BONE_EXPORT_FILE)->MoveWindow(rtBtn);
				rtBtn.OffsetRect(0, 40);
				GetDlgItem(IDC_BONE_SHOW_3D)->MoveWindow(rtBtn);
				rtBtn.OffsetRect(0, 40);
				if (!m_bCaculateDirect)
				{
					GetDlgItem(IDC_BONE_LASTSTEP)->MoveWindow(rtLastStep);
					rtLastStep.OffsetRect(0, -40);
				}	
				GetDlgItem(IDC_BONE_SHOW_3D)->MoveWindow(rtLastStep);
				rtLastStep.OffsetRect(0, -40);
				if (m_pThread2 != NULL && m_pThread2->GetDlgWnd() != NULL)
				{
					GetDlgItem(IDC_BONE_PRINT_3D)->MoveWindow(rtLastStep);
					rtLastStep.OffsetRect(0, -40);
				}
				GetDlgItem(IDC_BONE_EXPORT_FILE_EX)->MoveWindow(rtLastStep);
				rtLastStep.OffsetRect(0, -40);
				GetDlgItem(IDC_BONE_EXPORT_FILE)->MoveWindow(rtLastStep);
				rtLastStep.OffsetRect(0, -40);


			}
			break;
// 		case SP_BONE:
// 			{
// 				CRect rt(rtShow);
// 				GetDlgItem(IDC_BONE_SELECTION)->MoveWindow(rt);
// 				rt.OffsetRect(0, 40);
// 				GetDlgItem(IDC_BONE_DENSITY)->MoveWindow(rt);	
// 				rt.OffsetRect(0, 40);
// 				GetDlgItem(IDC_BONE_PRINTREPORT)->MoveWindow(rt);
// // 				rt.OffsetRect(0, 40);
// // 				GetDlgItem(IDC_EDIT_CLOSE)->MoveWindow(rt);			
// // 				rt.OffsetRect(0, 40);
// // 				GetDlgItem(IDC_BONE_CLOSE)->MoveWindow(rt);
// 				rt.OffsetRect(0, 40);
// 				GetDlgItem(IDC_EXPORT_EXCEL)->MoveWindow(rt);
// 				if (!theBoneDensitySwapData.m_bCalculateDirect)
// 					GetDlgItem(IDC_BONE_LASTSTEP)->MoveWindow(rtLastStep);
// 			}
// 			break;
// 		case SP_DENSITY:
// 			{
// 				CRect rtShow(&rect);
// 				rtShow.bottom = rtShow.top + 40;
// 				rtShow.DeflateRect(4,2,4,2);
// 				GetDlgItem(IDC_BONE_SELECTION)->MoveWindow(rtShow);
// 
// 				rtShow.OffsetRect(0, 40);
// 				GetDlgItem(IDC_BONE_LOADMODEL)->MoveWindow(rtShow);
// 
// 				rtShow.top = rect.top + 80;
// 				rtShow.bottom = rtShow.top + 24;
// 				rtShow.left = rect.left;
// 				rtShow.right = rect.left + rect.Width()/3;
// 				rtShow.OffsetRect(rtShow.Width(), 0);
// 				rtShow.DeflateRect(0,1,1,1);
// 				GetDlgItem(IDC_STATIC_CTLEVEL)->MoveWindow(rtShow);
// 				rtShow.OffsetRect(rect.Width()/3, 0);
// 				GetDlgItem(IDC_STATIC_DENSITY)->MoveWindow(rtShow);
// 				rtShow.OffsetRect(-rect.Width()/3*2, 24);
// 				GetDlgItem(IDC_STATIC_LOWMODEL)->MoveWindow(rtShow);
// 				rtShow.OffsetRect(rect.Width()/3, 0);
// 				GetDlgItem(IDC_EDIT_LOWCTLEVEL)->MoveWindow(rtShow);
// 				rtShow.OffsetRect(rect.Width()/3, 0);
// 				GetDlgItem(IDC_EDIT_LOWDENSITY)->MoveWindow(rtShow);
// 				rtShow.OffsetRect(-rect.Width()/3*2, 24);
// 				GetDlgItem(IDC_STATIC_HIGHMODEL)->MoveWindow(rtShow);
// 				rtShow.OffsetRect(rect.Width()/3, 0);
// 				GetDlgItem(IDC_EDIT_HIGHCTLEVEL)->MoveWindow(rtShow);
// 				rtShow.OffsetRect(rect.Width()/3, 0);
// 				GetDlgItem(IDC_EDIT_HIGHDENSITY)->MoveWindow(rtShow);
// 				rtShow.left = rect.left;
// 				rtShow.right = rect.right;
// 				rtShow.bottom = rtShow.top + 40;
// 				rtShow.OffsetRect(0,24);
// 				rtShow.DeflateRect(4,2,4,2);
// 				GetDlgItem(IDC_BONE_DENSITYCALCU)->MoveWindow(rtShow);
// 				rtShow.OffsetRect(0, 40);
// 				GetDlgItem(IDC_BONE_PRINTREPORT)->MoveWindow(rtShow);
// 				rtShow.OffsetRect(0, 40);
// 				GetDlgItem(IDC_EXPORT_EXCEL)->MoveWindow(rtShow);
// 				if (!theBoneDensitySwapData.m_bCalculateDirect)
// 					GetDlgItem(IDC_BONE_LASTSTEP)->MoveWindow(rtLastStep);
// // 				rtShow.OffsetRect(0, 40);
// // 				GetDlgItem(IDC_EXPORT_EXCEL)->MoveWindow(rtShow);
// 
// 			}
// 			break;
// 		default:
// 			break;
 		}
	}
}


void SeBoneDensityCtrlDlg::OnBnClickedBoneStartdivision()
{
// 	// TODO: 在此添加控件通知处理程序代码
// //	theBoneDensitySwapData.CreatProDlg(theBoneDensitySwapData.m_pParentDlg);
// 	bool bEmpty  = true;
// 	for (int i = 0; i < theBoneDensitySwapData.m_pBoneDensityView->m_pProcessArray->GetZDcmPicCount(); i++)
// 		bEmpty = bEmpty && theBoneDensitySwapData.m_pBoneDensityView->m_pVecEdge[i].empty();
// 		//bEmpty = bEmpty && theBoneDensitySwapData.m_pBoneDensityView->m_pVecEdge[i].empty() && theBoneDensitySwapData.m_pBoneDensityView->m_pVecEdgeInside[i].empty();
// 	if (bEmpty)
// 	{
// 		AfxMessageBox("请先圈定ROI区域");
// 		return;
// 	}
// 
// 	theBoneDensitySwapData.m_pBoneDensityView->GetMaxRegion();
// 	theBoneDensitySwapData.m_pBoneDensityView->ClipAllImage();
// 	theBoneDensitySwapData.m_pBoneDensityView->Invalidate(FALSE);

}


void SeBoneDensityCtrlDlg::OnBnClickedBoneConfirmregion()
{

	BOOL bEmptyROI = theBoneDensitySwapData.m_pROIView->IsEmptyROI();
	if (bEmptyROI)
	{
		AfxMessageBox("请先圈定区域");
		return;
	}

	int layerCount = theBoneDensitySwapData.m_pROIView->GetDcmArray()->GetDcmArray().size();
	int startLayer = -1, endLayer = -1;
	for (int i = 0; i < layerCount; i++) {
		if (!theBoneDensitySwapData.m_pROIView->m_pVecEdge[i].empty()) {
			startLayer = i;
			break;
		}
	}
	for (int i = layerCount - 1; i >= 0; i--) {
		if (!theBoneDensitySwapData.m_pROIView->m_pVecEdge[i].empty()) {
			endLayer = i;
			break;
		}
	}
	CString roiMsg;
	roiMsg.Format(_T("[ROI确认] 层数: %d, 起始层: %d, 结束层: %d, 类型: 手动圈定"), layerCount, startLayer, endLayer);
	LOG_INFO(roiMsg);

	theBoneDensitySwapData.m_nStep++;
	g_pBoneDensityModule->CutImage(theBoneDensitySwapData.m_pROIView->m_pVecEdge, theBoneDensitySwapData.m_pROIView->m_pVecEdgeInside);
	theBoneDensitySwapData.m_pROIView->ResetROIPts();

	// 记录ROI操作日志 - 获取起始层和结束层
	

	theBoneDensitySwapData.m_pBinaryView->SetDcmArray(&(g_pBoneDensityModule->m_ROIArray));
	theBoneDensitySwapData.m_pBinaryView->SetShowNewMask(FALSE);
	SendMessage(WM_SIZE);
	g_pBoneDensityModule->GetUI()->SendMessage(WM_SIZE);




// 	// TODO: 在此添加控件通知处理程序代码
// 	bool bEmpty  = true;
// 	int nSize = theBoneDensitySwapData.m_pBoneDensityView->m_pProcessArray->GetZDcmPicCount();
// 	for (int i = 0; i < nSize; i++)
// 		bEmpty = bEmpty && theBoneDensitySwapData.m_pBoneDensityView->m_pVecEdge[i].empty();
// 		//bEmpty = bEmpty && theBoneDensitySwapData.m_pBoneDensityView->m_pVecEdge[i].empty() && theBoneDensitySwapData.m_pBoneDensityView->m_pVecEdgeInside[i].empty();
// 	if (bEmpty)
// 	{
// 		AfxMessageBox("请先圈定区域");
// 		return;
// 	}
// 	theBoneDensitySwapData.m_pBoneDensityView->GetMaxRegion();
// 	theBoneDensitySwapData.m_pBoneDensityView->ClipAllImage();
// 
// 
// 	theBoneDensitySwapData.m_pBoneDensityView->SaveROIData();
// 	theBoneDensitySwapData.m_pParentDlg->ReleaseRotateArray();
// 	theBoneDensitySwapData.m_pBoneAnalysisView->SetDcmArray(&theBoneDensitySwapData.m_pParentDlg->m_BoneArray);
// 	theBoneDensitySwapData.m_nStep = SP_BINARY;
// 	theBoneDensitySwapData.m_bCalculateDirect = false;
// 	SendMessage(WM_SIZE);
// 	theBoneDensitySwapData.m_pParentDlg->SendMessage(WM_SIZE);

}



void SeBoneDensityCtrlDlg::OnBnClickedBonePartbinary()
{
	CSeNewMaskDlg dlg(
		&theBoneDensitySwapData.m_Histogram[0],
		theBoneDensitySwapData.m_lMaxNumber, 
		theBoneDensitySwapData.m_nMaxValue, 
		theBoneDensitySwapData.m_nMinValue,
		theBoneDensitySwapData.m_nMaxValuePos,
		theBoneDensitySwapData.m_nMinValuePos,
		this);
	theBoneDensitySwapData.m_pBinaryView->SetShowNewMask(TRUE);
	if (dlg.DoModal() == IDOK)
	{
		// 记录二值化日志 - 最小值和最大值范围
		CString binaryMsg;
		binaryMsg.Format(_T("[二值化] 范围: %d - %d"), 
			dlg.GetMin(), 
			dlg.GetMax());
		LOG_INFO(binaryMsg);
		
		theBoneDensitySwapData.m_pBinaryView->Invalidate(FALSE);
		m_bBianry = TRUE;
		SendMessage(WM_SIZE);
	}
	else
	{
		theBoneDensitySwapData.m_pBinaryView->SetShowNewMask(FALSE);
		theBoneDensitySwapData.m_pBinaryView->Invalidate(FALSE);
	}
}

void SeBoneDensityCtrlDlg::OnBnClickedBoneParametercalcu()
{
	if(g_pBoneDensityModule->m_OriDcmArray.GetDcmArray().size() == 0)
	{
		AfxMessageBox("请先读入数据！");
		return;
	}
	if (IDNO == MessageBox("导入数据是否是已裁剪图像或二值图？", "图像类型判断：", MB_ICONWARNING|MB_YESNO))
		return;
	theBoneDensitySwapData.m_nStep = SP_BINARY;
	m_bCaculateDirect = TRUE;
	theBoneDensitySwapData.m_pBinaryView->SetDcmArray(&(g_pBoneDensityModule->m_OriDcmArray));

	SendMessage(WM_SIZE);
	g_pBoneDensityModule->GetUI()->SendMessage(WM_SIZE);
}


void SeBoneDensityCtrlDlg::OnBnClickedBoneDensity()
{
// 	// TODO: 在此添加控件通知处理程序代码
// 	theBoneDensitySwapData.m_nStep = SP_DENSITY;
// 	SendMessage(WM_SIZE);
// 	theBoneDensitySwapData.m_pParentDlg->SendMessage(WM_SIZE);
}


void SeBoneDensityCtrlDlg::OnBnClickedBoneLoadmodel()
{
	// TODO: 在此添加控件通知处理程序代码
	//theBoneDensitySwapData.m_pParentDlg->OpenModelFile();
}

// void SeBoneDensityCtrlDlg::SetHighCTLevel( int nLevel )
// {
// 	CString	csLevel;
// 	csLevel.Format("%d", nLevel);
// 	GetDlgItem(IDC_EDIT_HIGHCTLEVEL)->SetWindowText(csLevel);
// 	Invalidate(FALSE);
// }
// 
// void SeBoneDensityCtrlDlg::SetLowCTLevel( int nLevel )
// {
// 	CString	csLevel;
// 	csLevel.Format("%d", nLevel);
// 	GetDlgItem(IDC_EDIT_LOWCTLEVEL)->SetWindowText(csLevel);
// 	Invalidate(FALSE);
// }
// 
// void SeBoneDensityCtrlDlg::Reset()
// {
// 	m_bBoneParameterTest=false;
// 	theBoneDensitySwapData.m_nStep = SP_SELECTZ;
// 	SendMessage(WM_SIZE);
// 	theBoneDensitySwapData.m_pParentDlg->SendMessage(WM_SIZE);
// }

void SeBoneDensityCtrlDlg::OnBnClickedBoneDensitycalcu()
{
// 	CString CTlevelA;
// 	CString CTlevelB;
// 	CString DensityA;
// 	CString DensityB;
// 	GetDlgItem(IDC_EDIT_LOWCTLEVEL)->GetWindowTextA(CTlevelA);
// 	GetDlgItem(IDC_EDIT_HIGHCTLEVEL)->GetWindowTextA(CTlevelB);
// 
// 	GetDlgItem(IDC_EDIT_LOWDENSITY)->GetWindowTextA(DensityA);
// 	GetDlgItem(IDC_EDIT_HIGHDENSITY)->GetWindowTextA(DensityB);
// 	if (CTlevelA == "" || CTlevelB == "" || DensityA == "" || DensityB == "")
// 	{
// 		AfxMessageBox("请输入模体参数");
// 		return;
// 	}
// 	theBoneDensitySwapData.m_pBoneAnalysisView->OnBoneDensity(CTlevelA,CTlevelB,DensityA,DensityB);
// 	theBoneDensitySwapData.m_dlgResult->m_lstResult.DeleteAllItems();//删除listresult结果
// 	theBoneDensitySwapData.m_dlgResult->FillList();
}


void SeBoneDensityCtrlDlg::OnBnClickedBoneLaststep()
{
// 	// TODO: 在此添加控件通知处理程序代码
	if (m_bCaculateDirect) {
		m_bCaculateDirect = FALSE;
		theBoneDensitySwapData.m_nStep = SP_SELECTZ;
		SendMessage(WM_SIZE);
		g_pBoneDensityModule->GetUI()->SendMessage(WM_SIZE);
	}
	else {
		theBoneDensitySwapData.m_nStep--;
		ReturnToLastStep();
		SendMessage(WM_SIZE);
	}
// 	theBoneDensitySwapData.m_pParentDlg->SendMessage(WM_SIZE);
}


void SeBoneDensityCtrlDlg::OnBnClickedBonePrintreport()
{
// 	// TODO: 在此添加控件通知处理程序代码
// 	theBoneDensitySwapData.m_pBoneAnalysisView->ShowReport();
}


void SeBoneDensityCtrlDlg::OnBnClickedBoneDrawedge()
{
// 	// TODO: 在此添加控件通知处理程序代码
 	theBoneDensitySwapData.m_pROIView->SetMouseTool(MT_BONEZROI);
	theBoneDensitySwapData.m_pROIView->InitROIPts();
	theBoneDensitySwapData.m_pROIView->SetClipOutside(TRUE);
	m_nDeleteOutside = 0;
	UpdateData(FALSE);
	
}


void SeBoneDensityCtrlDlg::OnBnClickedBoneClearedge()
{
// 	// TODO: 在此添加控件通知处理程序代码
 	theBoneDensitySwapData.m_pROIView->ResetROIPts();
}

void SeBoneDensityCtrlDlg::OnBnClickedBoneParamSelect()
{
	// 	if ( SelectDialog.DoModal() == IDOK )
	// 	{
	// 		m_bBoneParameterTest=true;
	// 		theBoneDensitySwapData.m_pBoneAnalysisView->OnBoneanalysis(&(SelectDialog.ParamSelection),SelectDialog.GetProcessNum());
	// 		theBoneDensitySwapData.m_dlgResult->FillList();
	// 	}	
}


void SeBoneDensityCtrlDlg::OnBnClickedBoneRectconfirm()
{
// 	theBoneDensitySwapData.m_nStep = SP_BINARY;
// 	theBoneDensitySwapData.m_bCalculateDirect = true;
// 
// 	theBoneDensitySwapData.m_pBoneAnalysisView->SetDcmArray(&(theBoneDensitySwapData.m_pParentDlg->m_ZROIPicArray));
// 
// 	theBoneDensitySwapData.m_pBoneAnalysisView->SaveImageFile(false);
// 	SendMessage(WM_SIZE);
// 	theBoneDensitySwapData.m_pParentDlg->SendMessage(WM_SIZE);
}

void SeBoneDensityCtrlDlg::OnBnClickedBoneClose()
{
// 	// TODO: 在此添加控件通知处理程序代码
// 	UpdateData(TRUE);
// 	vector <CImageBase*> vecImg = theBoneDensitySwapData.m_pBoneAnalysisDoc->GetImageSet()->GetImageSet();
// 	
// 	theAppIVConfig.m_pILog->ProgressInit(vecImg.size());
// 	for (int i=0; i<vecImg.size(); i++)
// 	{
// 		CImageBase* pImg = vecImg[i];
// 		short* pData = (short*)pImg->GetData();
// 		Inflation(pData, pImg->GetWidth(), pImg->GetHeight(), _ttoi(m_nEditClose));
// 		Corrosion(pData, pImg->GetWidth(), pImg->GetHeight(), _ttoi(m_nEditClose));
// 		theAppIVConfig.m_pILog->ProgressStepIt();
// 	}
// 	theAppIVConfig.m_pILog->ProgressClose();
}

void SeBoneDensityCtrlDlg::OnBnClickedExportExcel()
{
// 	// TODO: 在此添加控件通知处理程序代码
// 	theBoneDensitySwapData.m_dlgResult->SendMessage(WM_EXPORT_EXCEL, 0, 0);
	if (m_pResult->m_vecResult.size() == 0)
	{
		MessageBoxTimeout(NULL, "         导出前请先计算参数！       ", "提示", MB_ICONINFORMATION, 0, 500);
		return;
	}
	
	BasicExcel e;
	e.New(1);
	BasicExcelWorksheet* sheet = e.GetWorksheet("Sheet1");
	if (sheet)
	{
		sheet->Cell(0,0)->SetWString("参数名称(Description)");
		sheet->Cell(0,1)->SetWString("参数名称(Description)");
		sheet->Cell(0,2)->SetWString("缩写(Abbreviation)");
		sheet->Cell(0,3)->SetWString("数值(Value)");
		sheet->Cell(0,4)->SetWString("单位(Unit)");

		for (int i=0; i<m_pResult->m_vecResult.size(); i++)
		{
			// 第一行写了头， 从第二行开始
			sheet->Cell(i + 1, 0)->SetWString(m_pResult->m_vecResult[i].strDescription);
			sheet->Cell(i + 1, 1)->SetWString(m_pResult->m_vecResult[i].strDescription2);
			sheet->Cell(i + 1, 2)->SetWString(m_pResult->m_vecResult[i].strAbbreviation);
			sheet->Cell(i + 1, 3)->SetDouble(m_pResult->m_vecResult[i].dValue); 
			sheet->Cell(i + 1, 4)->SetWString(m_pResult->m_vecResult[i].strUint); 
		}
		CString csPath;
		CString csName = theBoneDensitySwapData.m_csSeriesName;
		if (csName.GetLength() == 0)
			csName = "Result";
		csPath.Format("%s\\%s.xls", theBoneDensitySwapData.m_csFolderPath, csName);
		e.SaveAs(csPath); 
		
		// 记录日志
		CString exportMsg;
		exportMsg.Format(_T("[导出] Excel报告: %s"), csPath);
		LOG_INFO(exportMsg);
		
		MessageBoxTimeout(NULL, "         生成Excel成功！       ", "提示", MB_ICONINFORMATION, 0, 1000);
	}
	else
	{
		LOG_ERROR("生成Excel失败");
		MessageBoxTimeout(NULL, "         生成Excel失败！       ", "提示", MB_ICONINFORMATION, 0, 1000);
	}
}



void SeBoneDensityCtrlDlg::OnEnSetfocusXs()
{
	m_bSetValue = TRUE;
	m_nEditID = IDC_EDIT_XS;
}

void SeBoneDensityCtrlDlg::OnEnSetfocusXe()
{
	m_bSetValue = TRUE;
	m_nEditID = IDC_EDIT_XE;
}

void SeBoneDensityCtrlDlg::OnEnSetfocusYs()
{
	m_bSetValue = TRUE;
	m_nEditID = IDC_EDIT_YS;
}

void SeBoneDensityCtrlDlg::OnEnSetfocusYe()
{
	m_bSetValue = TRUE;
	m_nEditID = IDC_EDIT_YE;
}

void SeBoneDensityCtrlDlg::OnEnSetfocusZs()
{
	m_bSetValue = TRUE;
	m_nEditID = IDC_EDIT_ZS;
}

void SeBoneDensityCtrlDlg::OnEnSetfocusZe()
{
	m_bSetValue = TRUE;
	m_nEditID = IDC_EDIT_ZE;
}

void SeBoneDensityCtrlDlg::OnEnChangeEditXs()
{
	numOnly(IDC_EDIT_XS);
}


void SeBoneDensityCtrlDlg::OnEnChangeEditXe()
{
	numOnly(IDC_EDIT_XE);
}


void SeBoneDensityCtrlDlg::OnEnChangeEditYs()
{
	numOnly(IDC_EDIT_YS);
}


void SeBoneDensityCtrlDlg::OnEnChangeEditYe()
{
	numOnly(IDC_EDIT_YE);
}


void SeBoneDensityCtrlDlg::OnEnChangeEditZs()
{
	numOnly(IDC_EDIT_ZS);
}


void SeBoneDensityCtrlDlg::OnEnChangeEditZe()
{
	numOnly(IDC_EDIT_ZE);
}

void SeBoneDensityCtrlDlg::OnEnKillfocusXs()
{
	if(m_nEditID == IDC_EDIT_XS)
	{
		SeAPRView::m_nXStart = SetValue(m_nEditID);
	}
	theBoneDensitySwapData.m_pXOYView->UpdateOtherView();
	m_bSetValue = FALSE;
}

void SeBoneDensityCtrlDlg::OnEnKillfocusXe()
{
	if(m_nEditID == IDC_EDIT_XE)
	{
		SeAPRView::m_nXEnd = SetValue(m_nEditID);
	}
	theBoneDensitySwapData.m_pXOYView->UpdateOtherView();
	m_bSetValue = FALSE;
}

void SeBoneDensityCtrlDlg::OnEnKillfocusYs()
{
	if(m_nEditID == IDC_EDIT_YS)
	{
		SeAPRView::m_nYStart = SetValue(m_nEditID);
	}
	theBoneDensitySwapData.m_pXOYView->UpdateOtherView();
	m_bSetValue = FALSE;
}

void SeBoneDensityCtrlDlg::OnEnKillfocusYe()
{
	if(m_nEditID == IDC_EDIT_YE)
	{
		SeAPRView::m_nYEnd = SetValue(m_nEditID);
	}
	theBoneDensitySwapData.m_pXOYView->UpdateOtherView();
	m_bSetValue = FALSE;
}

void SeBoneDensityCtrlDlg::OnEnKillfocusZs()
{
	if(m_nEditID == IDC_EDIT_ZS)
	{
		SeAPRView::m_nZStart = SetValue(m_nEditID);
	}
	theBoneDensitySwapData.m_pXOYView->UpdateOtherView();
	m_bSetValue = FALSE;
}

void SeBoneDensityCtrlDlg::OnEnKillfocusZe()
{
	if(m_nEditID == IDC_EDIT_ZE)
	{
		SeAPRView::m_nZEnd = SetValue(m_nEditID);
	}
	theBoneDensitySwapData.m_pXOYView->UpdateOtherView();
	m_bSetValue = FALSE;
}

void SeBoneDensityCtrlDlg::numOnly(int nID)
{
	CEdit* pEdit;
	pEdit = (CEdit*) GetDlgItem(nID);
	CString str;
	pEdit->GetWindowText(str);

	if (str.SpanIncluding("-0123456789") != str)
	{
		pEdit->SetWindowText(str.Left(str.GetLength() - 1));
		int len = pEdit->GetWindowTextLength();
		pEdit->SetSel(len, len, FALSE);
		pEdit->SetFocus();
	}
}

inline void SeBoneDensityCtrlDlg::GetValue(int nID, int nValue)
{
	CEdit* pEdit;
	CString str;
	int na = theBoneDensitySwapData.m_nScreenWidth;
	int nb = theBoneDensitySwapData.m_nScreenHeight;
	int nc = theBoneDensitySwapData.m_nRotateDcmSideLength;
	if(nID == IDC_EDIT_XS || nID == IDC_EDIT_XE)
	{
		str.Format("%d", int((nValue - (na-nb)/2) * nc / nb));
	}
	else
	{
		str.Format("%d", int(nValue * nc / nb));
	}
	pEdit = (CEdit*)GetDlgItem(nID);
	pEdit->SetWindowText(str);
}

int SeBoneDensityCtrlDlg::SetValue(int nID)
{
	CEdit* pEdit;
	CString str;
	pEdit = (CEdit*)GetDlgItem(nID);
	pEdit->GetWindowText(str);
	int nValue = atoi(str);
	if((nID - IDC_EDIT_XS) % 2 != 0 )
	{
		pEdit = (CEdit*)GetDlgItem(nID - 1);
		pEdit->GetWindowText(str);
		nValue = (nValue > (atoi(str) + 10)) ? nValue : (atoi(str) + 10);
	}
	else
	{
		pEdit = (CEdit*)GetDlgItem(nID + 1);
		pEdit->GetWindowText(str);
		nValue = (nValue < (atoi(str) - 10)) ? nValue : (atoi(str) - 10);
	}
	int na = theBoneDensitySwapData.m_nScreenWidth;
	int nb = theBoneDensitySwapData.m_nScreenHeight;
	int nc = theBoneDensitySwapData.m_nRotateDcmSideLength;
	if(nID == IDC_EDIT_XS || nID == IDC_EDIT_XE)
	{
		nValue = int(nValue * nb / nc + (na - nb)/2);
	}
	else
	{
		nValue = int(nValue * nb / nc);
	}
	return nValue;
}


void SeBoneDensityCtrlDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if(nIDEvent == 1 && !m_bSetValue)
	{
		GetValue(IDC_EDIT_XS, SeAPRView::m_nXStart);
		GetValue(IDC_EDIT_XE, SeAPRView::m_nXEnd);
		GetValue(IDC_EDIT_YS, SeAPRView::m_nYStart);
		GetValue(IDC_EDIT_YE, SeAPRView::m_nYEnd);
		GetValue(IDC_EDIT_ZS, SeAPRView::m_nZStart);
		GetValue(IDC_EDIT_ZE, SeAPRView::m_nZEnd);
		SetLWH();
	}
	else if (nIDEvent == 2 && m_bSegFuncsUsed)
	{
		CRect rect;
		theBoneDensitySwapData.m_pROIView->GetWindowRect(&rect);
		CPoint point;
		GetCursorPos(&point);
		if (PtInRect(&rect,point))
		{
			AfxGetMainWnd()->EnableWindow(TRUE);
		}
		else
		{
			AfxGetMainWnd()->EnableWindow(FALSE);
		}
	}
	CSeDialogBase::OnTimer(nIDEvent);
}


void SeBoneDensityCtrlDlg::OnIdok()
{
	// TODO: 在此添加命令处理程序代码
	if(theBoneDensitySwapData.m_nStep == SP_ROTATEDATA)
	{
		int nID = m_nEditID;
		if(nID - IDC_EDIT_XS == 5)
			nID = IDC_EDIT_XS;
		else
			nID++;
		CEdit* pEdit;
		pEdit = (CEdit*) GetDlgItem(nID);
		int len = pEdit->GetWindowTextLength();
		pEdit->SetSel(len, len, FALSE);
		pEdit->SetFocus();
		SetLWH();
	}
}

void SeBoneDensityCtrlDlg::SetLWH()
{
	// 在控件属性里设置 readonly 失败，未知原因
	((CEdit*)GetDlgItem(IDC_EDIT_L))->SetReadOnly(FALSE);
	((CEdit*)GetDlgItem(IDC_EDIT_W))->SetReadOnly(FALSE);
	((CEdit*)GetDlgItem(IDC_EDIT_H))->SetReadOnly(FALSE);
	CString str;
	str.Format("%.2fmm", (float)(GetEditValue(IDC_EDIT_XE) - GetEditValue(IDC_EDIT_XS)) * theBoneDensitySwapData.m_dbXYSliceSpace);
	GetDlgItem(IDC_EDIT_L)->SetWindowText(str); 
	str.Format("%.2fmm", (float)(GetEditValue(IDC_EDIT_YE) - GetEditValue(IDC_EDIT_YS)) * theBoneDensitySwapData.m_dbXYSliceSpace);
	GetDlgItem(IDC_EDIT_W)->SetWindowText(str); 
	str.Format("%.2fmm", (float)(GetEditValue(IDC_EDIT_ZE) - GetEditValue(IDC_EDIT_ZS)) * theBoneDensitySwapData.m_dbZSliceSpace);
	GetDlgItem(IDC_EDIT_H)->SetWindowText(str); 

}

int SeBoneDensityCtrlDlg::GetEditValue(int nID)
{
	CEdit* pEdit;
	pEdit = (CEdit*) GetDlgItem(nID);
	CString str;
	pEdit->GetWindowText(str);
	return atoi(str);
}

void SeBoneDensityCtrlDlg::Corrosion(short *pData, int nWidth, int nHeight, int nCore)
{
	if (nCore == 0)
		return;
	int nValue = -600;
	if(nHeight <= 0 || nWidth <= 0)
		return;
	short* temp = new short[nWidth*nHeight];
	memcpy(temp, pData, nWidth*nHeight*sizeof(short));
	for (int i=1; i<nWidth-1; i++)
	{
		for(int j=1;j<nHeight-1;j++)
		{
			if (temp[i+j*nWidth] >= nValue)
			{
				if ((temp[i-1+(j+1)*nWidth] >= nValue
					&&temp[i+(j+1)*nWidth] >= nValue
					&&temp[i+1+(j+1)*nWidth] >= nValue
					&&temp[i+1+j*nWidth] >= nValue
					&&temp[i-1+j*nWidth] >= nValue
					&&temp[i-1+(j-1)*nWidth] >= nValue
					&&temp[i+(j-1)*nWidth] >= nValue
					&&temp[i+1+(j-1)*nWidth] >= nValue))
				{
					pData[i+j*nWidth] = nValue;
				}
				else 
					pData[i+j*nWidth]=-1000;
			}/*如果该点附近有点的值不等于最大值，将该点设为最小值*/
			else pData[i+j*nWidth]=-1000;
		}	
	}
	Safe_Delete(temp);
	nCore = nCore - 1; 
	Corrosion(pData, nWidth, nHeight, nCore);
}

void SeBoneDensityCtrlDlg::Inflation(short *pData, int nWidth, int nHeight, int nCore)
{
	if (nCore == 0)
		return;
	int nValue = -600;
	if(nHeight <= 0 || nWidth <= 0)
		return ;
	short* temp = new short[nWidth*nHeight];
	memcpy(temp, pData , nWidth*nHeight*sizeof(short));
	for (int i = 1 ; i < nWidth - 1; i++)
	{
		for (int j = 1 ; j<nHeight - 1; j++)
		{
			if(temp[i+j*nWidth] >= nValue)
			{
				pData[i-1+(j+1)*nWidth	] = nValue;
				pData[i+(j+1)*nWidth	] = nValue;
				pData[i+1+(j+1)*nWidth	] = nValue;
				pData[i+1+j*nWidth		] = nValue;
				pData[i-1+j*nWidth		] = nValue;
				pData[i-1+(j-1)*nWidth	] = nValue;
				pData[i+(j-1)*nWidth	] = nValue;
				pData[i+1+(j-1)*nWidth	] = nValue;
			}/*如果该点为最大值，将附近的点都设为最大值*/
		}
	}
	Safe_Delete(temp);
	nCore = nCore - 1; 
	Inflation(pData, nWidth, nHeight, nCore);
}

void SeBoneDensityCtrlDlg::Reset()
{
	m_bGetEage = FALSE;
	m_bCaculateDirect = FALSE; 
	m_bBianry = FALSE;
}



void SeBoneDensityCtrlDlg::OnBnClickedBoneCalculate()
{
	// TODO: 在此添加控件通知处理程序代码
	SeBoneParamSelection dlg;
	dlg.SetInfo(theBoneDensitySwapData.m_nSamllValue,
		theBoneDensitySwapData.m_nBigValue,
		theBoneDensitySwapData.m_fSmallDensity,
		theBoneDensitySwapData.m_fBigDensity);
	dlg.SetParent(this);
	if (IDOK == dlg.DoModal())
	{
		BoneParameter Param = dlg.GetParam();
		m_pResult->SetCaculateParam(
			Param.b_BasicParameter,
			Param.b_BoneThick,
			Param.b_SpaceThick,
			Param.b_BoneNumber,
			Param.b_Connectivity,
			Param.b_SMI,
			Param.b_DA,
			Param.b_TBPf,	
			Param.b_Density,
			Param.b_CorticalArea,
			Param.b_CorticalThick,
			Param.b_CorticalFraction,
			Param.b_MedullaryArea,
			Param.b_CorticalDensity,
			Param.densityInfo.nSmallValue,
			Param.densityInfo.nBigValue,
			Param.densityInfo.fSmallDensity,
			Param.densityInfo.fBigDensity
			);
		if(Param.b_Density)
			dlg.GetInfo(theBoneDensitySwapData.m_nSamllValue,
			theBoneDensitySwapData.m_nBigValue,
			theBoneDensitySwapData.m_fSmallDensity,
			theBoneDensitySwapData.m_fBigDensity);
		SeBinaryView* p = theBoneDensitySwapData.m_pBinaryView;
		m_pResult->SetEmptyVolume(static_cast<double>(theBoneDensitySwapData.m_tmpSize));
		m_pResult->CaculateChoicedParam(p->GetDcmArray(), p->GetMin(), p->GetMax(), theBoneDensitySwapData.m_nMinValue);

		// 记录计算日志 - 只记录计算了哪些参数类型
		CString calcParams;
		if (Param.b_BasicParameter) calcParams += _T("基础参数, ");
		if (Param.b_BoneThick) calcParams += _T("骨厚度, ");
		if (Param.b_SpaceThick) calcParams += _T("间隙厚度, ");
		if (Param.b_BoneNumber) calcParams += _T("骨数量, ");
		if (Param.b_Connectivity) calcParams += _T("连通性, ");
		if (Param.b_SMI) calcParams += _T("SMI, ");
		if (Param.b_DA) calcParams += _T("DA, ");
		if (Param.b_TBPf) calcParams += _T("TBPf, ");
		if (Param.b_Density) calcParams += _T("密度, ");
		if (Param.b_CorticalArea) calcParams += _T("皮质面积, ");
		if (Param.b_CorticalThick) calcParams += _T("皮质厚度, ");
		if (Param.b_CorticalFraction) calcParams += _T("皮质分数, ");
		if (Param.b_MedullaryArea) calcParams += _T("髓质面积, ");
		if (Param.b_CorticalDensity) calcParams += _T("皮质密度, ");
		if (!calcParams.IsEmpty()) {
			calcParams = calcParams.Left(calcParams.GetLength() - 2); // 去掉最后的", "
			CString msg;
			msg.Format(_T("[参数计算] 已选择: %s"), calcParams);
			LOG_INFO(msg);
		}

		MessageBoxTimeout(NULL, "         计算完成！        ", "提示", MB_ICONINFORMATION, 0, 1000);
	}
	
}

void SeBoneDensityCtrlDlg::ReturnToLastStep()
{
	switch(theBoneDensitySwapData.m_nStep)
	{
	case SP_SELECTZ:
		{
			theBoneDensitySwapData.m_pXOYView->SetAPRTool();
			theBoneDensitySwapData.m_pYOZView->SetAPRTool();
			theBoneDensitySwapData.m_pXOZView->SetAPRTool();

			theBoneDensitySwapData.m_pXOYView->InitView();
			theBoneDensitySwapData.m_pYOZView->InitView();
			theBoneDensitySwapData.m_pXOZView->InitView();
			break;
		}
	case SP_ROTATEDATA:
		{
			theBoneDensitySwapData.m_pROIView->SetShowNewMask(FALSE);
			SendMessage(WM_SIZE);
			g_pBoneDensityModule->GetUI()->SendMessage(WM_SIZE);
			break;
		}
	case SP_CALCU:
		{
			theBoneDensitySwapData.m_pROIView->SetShowNewMask(FALSE);
			theBoneDensitySwapData.m_pROIView->InitROIPts();
			SendMessage(WM_SIZE);
			g_pBoneDensityModule->GetUI()->SendMessage(WM_SIZE);
			break;
		}
	default:
		break;
	}
}

LRESULT SeBoneDensityCtrlDlg::OnAddMask(WPARAM wParam, LPARAM lParam)
{
	int nMin = (int)wParam;
	int nMax = (int)lParam;
	switch(theBoneDensitySwapData.m_nStep)
	{

	case SP_CALCU:
		{
			theBoneDensitySwapData.m_pROIView->SetInfo(nMin, nMax, RGB(255, 0, 0), 64);
			theBoneDensitySwapData.m_pROIView->Invalidate(FALSE);
			break;
		}
	case SP_BINARY:
		{
			theBoneDensitySwapData.m_pBinaryView->SetInfo(nMin, nMax, RGB(255, 0, 0), 64);
			theBoneDensitySwapData.m_pBinaryView->Invalidate(FALSE);
			break;
		}
	default:
		break;
	}

	return 0;
}


void SeBoneDensityCtrlDlg::OnBnClickedBoneExportFile()
{
	// TODO: 在此添加控件通知处理程序代码
	switch(theBoneDensitySwapData.m_nStep)
	{

	case SP_CALCU:
		{
			g_pBoneDensityModule->ExportDcmArray("RotateData", &g_pBoneDensityModule->m_SliceArray);
			break;
		}
	case SP_BINARY:
		{
			g_pBoneDensityModule->ExportDcmArray("BinaryData", &g_pBoneDensityModule->m_ROIArray);
			break;
		}
	default:
		break;
	}
	
}


void SeBoneDensityCtrlDlg::OnBnClickedBoneShow3d()
{
	// TODO: 在此添加控件通知处理程序代码
	switch(theBoneDensitySwapData.m_nStep)
	{

	case SP_CALCU:
		{
			CSeNewMaskDlg dlg(
				&theBoneDensitySwapData.m_Histogram[0],
				theBoneDensitySwapData.m_lMaxNumber, 
				theBoneDensitySwapData.m_nMaxValue, 
				theBoneDensitySwapData.m_nMinValue,
				theBoneDensitySwapData.m_nMaxValuePos,
				theBoneDensitySwapData.m_nMinValuePos,
				this);
			theBoneDensitySwapData.m_pROIView->SetShowNewMask(TRUE);
			if (dlg.DoModal() == IDOK)
			{
				theBoneDensitySwapData.m_pROIView->SetShowNewMask(FALSE);
				theBoneDensitySwapData.m_pROIView->Invalidate(FALSE);
				CRect rect;
				g_pBoneDensityModule->GetUI()->GetWindowRect(&rect);
				m_pThread1 = (CEasy3DViewThread*)AfxBeginThread(RUNTIME_CLASS(CEasy3DViewThread), THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
				ThreadInfoFor3D* pInfo = new ThreadInfoFor3D(
					(CWnd*)this,
					theBoneDensitySwapData.m_pROIView->GetDcmArray(),
					theBoneDensitySwapData.m_pROIView->GetMin(),
					theBoneDensitySwapData.m_pROIView->GetMax(),
					rect
					);
				m_pThread1->SetInfo(pInfo);
				m_pThread1->ResumeThread();
				break;
			}
			else
			{
				theBoneDensitySwapData.m_pROIView->SetShowNewMask(FALSE);
				theBoneDensitySwapData.m_pROIView->Invalidate(FALSE);
			}
			break;
		}
	case SP_BINARY:
		{
			CRect rect;
			g_pBoneDensityModule->GetUI()->GetWindowRect(&rect);
			m_pThread2 = (CEasy3DViewThread*)AfxBeginThread(RUNTIME_CLASS(CEasy3DViewThread), THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
			ThreadInfoFor3D* pInfo = new ThreadInfoFor3D(
				(CWnd*)this,
				theBoneDensitySwapData.m_pBinaryView->GetDcmArray(),
				theBoneDensitySwapData.m_pBinaryView->GetMin(),
				theBoneDensitySwapData.m_pBinaryView->GetMax(),
				rect
				);
			m_pThread2->SetInfo(pInfo);
			m_pThread2->ResumeThread();
			break;
		}
	default:
		break;
	}
	SendMessage(WM_SIZE);
}


void SeBoneDensityCtrlDlg::OnBnClickedBonePrint3d()
{
	// 自订消息使用 CString
	// 发送消息
	// CString csWPARAM=_T("ZHPC 连接服务器成功 ");
	// CString csLPARAM=_T("ZHPC 收到登录请求，收到公钥 随机数 ");
	// PostMessage(WM_PostMessage, (WPARAM)csWPARAM.AllocSysString(), (LPARAM)csLPARAM.AllocSysString());、

	//  消息响应函数内
	// 	CString csWPARAM=(CString)((BSTR)wParam);
	// 	CString csLPARAM=(CString)((BSTR)lParam);
	// 	SysFreeString((BSTR)wParam);
	// 	SysFreeString((BSTR)lParam);
	// TODO: 在此添加控件通知处理程序代码

	CString csPath = theBoneDensitySwapData.m_csFolderPath;
	switch(theBoneDensitySwapData.m_nStep)
	{
	case SP_CALCU:
		{
			if (m_pThread1 != NULL) 
 				m_pThread1->PostThreadMessage(WM_PRINT_3D, (WPARAM)csPath.AllocSysString(), 0);
			break;
		}
	case SP_BINARY:
		{
			if (m_pThread2 != NULL)
				m_pThread2->PostThreadMessage(WM_PRINT_3D, (WPARAM)csPath.AllocSysString(), 0);
			break;
		}
	default:
		break;
	}

}

void SeBoneDensityCtrlDlg::OnBnClickedBoneSegFunc()
{
// 	CSeBoneSegProcedureDlg dlg(&theBoneDensitySwapData.m_vecFuncList,&theBoneDensitySwapData.m_mapFunc,this);
// 	dlg.DoModal();
// 	SendMessage(WM_SIZE);
	theBoneDensitySwapData.m_bSeg2Cal = FALSE;
	m_bSegFuncsUsed = TRUE;
	CSeBoneSegProcedureDlg* dlg = new CSeBoneSegProcedureDlg(&theBoneDensitySwapData.m_vecFuncList,&theBoneDensitySwapData.m_mapFunc,this);
	dlg->Create(IDD_DIALOG_BONECALPROCEDURE,this); //创建一个非模态对话框
	dlg->ShowWindow(SW_SHOW); //显示非模态对话框
}


LRESULT SeBoneDensityCtrlDlg::OnBoneSegmentation(WPARAM wParam, LPARAM lParam)
{
	//皮质骨松质骨的自动分割

	//获得要操作的步骤
	//bool bOuter = 0;
	if (theBoneDensitySwapData.m_vecFuncList.empty())
	{
		return 0;
	}
	for (int i=0;i<theBoneDensitySwapData.m_vecFuncList.size();i++)
	{
// 		if (!theBoneDensitySwapData.m_vecFuncList[i].bSel)
// 		{
// 			continue;
// 		}
		CString strName = theBoneDensitySwapData.m_vecFuncList[i].strFuncName;
		CString strColor = theBoneDensitySwapData.m_vecFuncList[i].strColor;
		if (strName == "reverse")
		{
			if (strColor == "red")
			{
				theBoneDensitySwapData.m_pROIView->ReverseAllBone(&(g_pBoneDensityModule->m_MaskArray));
			}
			else
			{
				theBoneDensitySwapData.m_pROIView->ReverseAllBone(&(g_pBoneDensityModule->m_MaskOutArray));
			}
		}
		else if (strName == "gethole")
		{
			if (strColor == "red")
			{
				theBoneDensitySwapData.m_pROIView->GetHoleAllBone(&(g_pBoneDensityModule->m_MaskArray));
			}
			else
			{
				theBoneDensitySwapData.m_pROIView->GetHoleAllBone(&(g_pBoneDensityModule->m_MaskOutArray));
			}
			//theBoneDensitySwapData.m_pROIView->FloodfillAllBone(&(g_pBoneDensityModule->m_MaskArray),&(g_pBoneDensityModule->m_MaskOutArray));
			//theBoneDensitySwapData.m_pROIView->SetDcmArrayOutMask(&(g_pBoneDensityModule->m_MaskOutArray));
		}
		else if (strName == "fillhole")
		{
			if (strColor == "red")
			{
				theBoneDensitySwapData.m_pROIView->FillHoleAllBone(&(g_pBoneDensityModule->m_MaskArray));
			}
			else
			{
				theBoneDensitySwapData.m_pROIView->FillHoleAllBone(&(g_pBoneDensityModule->m_MaskOutArray));
			}
			
		}
		else if (strName == "eorrosion")
		{
			CString strType = theBoneDensitySwapData.m_vecFuncList[i].strKernelType;
			int nKernelSize = theBoneDensitySwapData.m_vecFuncList[i].nKernelSize;
			int nType = 0;
			if (strType == "circle")
			{
				nType = 1;
			}
			if (strColor == "red")
			{
				theBoneDensitySwapData.m_pROIView->BoneSegCorrosion(&(g_pBoneDensityModule->m_MaskArray),nType,nKernelSize);
			}
			else
			{
				theBoneDensitySwapData.m_pROIView->BoneSegCorrosion(&(g_pBoneDensityModule->m_MaskOutArray),nType,nKernelSize);
			}
			
		}
		else if (strName == "dilate")
		{
			CString strType = theBoneDensitySwapData.m_vecFuncList[i].strKernelType;
			int nKernelSize = theBoneDensitySwapData.m_vecFuncList[i].nKernelSize;
			int nType = 0;
			if (strType == "circle")
			{
				nType = 1;
			}
			if (strColor == "red")
			{
				theBoneDensitySwapData.m_pROIView->BoneSegInflation(&(g_pBoneDensityModule->m_MaskArray),nType,nKernelSize);
			}
			else
			{
				theBoneDensitySwapData.m_pROIView->BoneSegInflation(&(g_pBoneDensityModule->m_MaskOutArray),nType,nKernelSize);
			}
		}
		else if (strName == "open")
		{
			CString strType = theBoneDensitySwapData.m_vecFuncList[i].strKernelType;
			int nKernelSize = theBoneDensitySwapData.m_vecFuncList[i].nKernelSize;
			int nType = 0;
			if (strType == "circle")
			{
				nType = 1;
			}
			if (strColor == "red")
			{
				theBoneDensitySwapData.m_pROIView->BongSegOpen(&(g_pBoneDensityModule->m_MaskArray),nType,nKernelSize);
			}
			else
			{
				theBoneDensitySwapData.m_pROIView->BongSegOpen(&(g_pBoneDensityModule->m_MaskOutArray),nType,nKernelSize);
			}
		}
		else if (strName == "close")
		{
			CString strType = theBoneDensitySwapData.m_vecFuncList[i].strKernelType;
			int nKernelSize = theBoneDensitySwapData.m_vecFuncList[i].nKernelSize;
			int nType = 0;
			if (strType == "circle")
			{
				nType = 1;
			}
			if (strColor == "red")
			{
				theBoneDensitySwapData.m_pROIView->BongSegClose(&(g_pBoneDensityModule->m_MaskArray),nType,nKernelSize);
			}
			else
			{
				theBoneDensitySwapData.m_pROIView->BongSegClose(&(g_pBoneDensityModule->m_MaskOutArray),nType,nKernelSize);
			}
		}
		else
		{
			return 0;
		}
	}
// 	if (bOuter)
// 	{
// 		theBoneDensitySwapData.m_pROIView->BoneGetMaskOuter(&(g_pBoneDensityModule->m_MaskArray),&(g_pBoneDensityModule->m_MaskOutArray));
// 		theBoneDensitySwapData.m_pROIView->SetDcmArrayOutMask(&(g_pBoneDensityModule->m_MaskOutArray));
// 		theBoneDensitySwapData.m_pROIView->SetShowNewROIOutMask(1);
// 	}
	//theBoneDensitySwapData.m_pROIView->SetDcmArray(&(g_pBoneDensityModule->m_SliceArray));
	theBoneDensitySwapData.m_pROIView->SetDcmArrayMask(&(g_pBoneDensityModule->m_MaskArray));
	theBoneDensitySwapData.m_pROIView->SetDcmArrayOutMask(&(g_pBoneDensityModule->m_MaskOutArray));
	SendMessage(WM_SIZE);
	theBoneDensitySwapData.m_pROIView->Invalidate(FALSE);
	return 0;
}

LRESULT SeBoneDensityCtrlDlg::OnBoneSegReset(WPARAM wParam, LPARAM lParam)
{
	//返回二值化情况
	theBoneDensitySwapData.m_pROIView->SetInfo(theBoneDensitySwapData.m_nMinValuePos, theBoneDensitySwapData.m_nMaxValuePos, RGB(255, 0, 0), 64);
	theBoneDensitySwapData.m_pROIView->BinaryzationAllBone(&(g_pBoneDensityModule->m_SliceArray),&(g_pBoneDensityModule->m_MaskArray));
	//theBoneDensitySwapData.m_pROIView->SetDcmArray(&(g_pBoneDensityModule->m_SliceArray));
	theBoneDensitySwapData.m_pROIView->SetDcmArrayMask(&(g_pBoneDensityModule->m_MaskArray));
	theBoneDensitySwapData.m_pROIView->BoneSegDicomPicArrayClone(&(g_pBoneDensityModule->m_MaskArray),&(g_pBoneDensityModule->m_MaskOutArray));
	theBoneDensitySwapData.m_pROIView->SetShowNewROIMask(TRUE);
	theBoneDensitySwapData.m_pROIView->SetShowNewROIOutMask(FALSE);
	theBoneDensitySwapData.m_pROIView->SetShowNewMask(TRUE);
	theBoneDensitySwapData.m_pROIView->Invalidate(FALSE);
	SendMessage(WM_SIZE);
	return 0;
}

LRESULT SeBoneDensityCtrlDlg::OnBoneSegShowALL(WPARAM wParam, LPARAM lParam)
{
	//
	theBoneDensitySwapData.m_pROIView->SetShowNewMask(TRUE);
	theBoneDensitySwapData.m_pROIView->SetShowNewROIMask(TRUE);
	theBoneDensitySwapData.m_pROIView->SetShowNewROIOutMask(TRUE);
	theBoneDensitySwapData.m_pROIView->Invalidate(FALSE);
	return 0;
}

LRESULT SeBoneDensityCtrlDlg::OnBoneSegFinished(WPARAM wParam, LPARAM lParam)
{
	//改变状态
	m_bSegFuncsUsed = FALSE;
	theBoneDensitySwapData.m_bSeg2Cal = TRUE;
	return 1;
}

LRESULT SeBoneDensityCtrlDlg::OnBoneSetShowMaskInfer(WPARAM wParam, LPARAM lParam)
{
	//传递要显示的
	bool bRed = (bool)wParam;
	bool bGreen = (bool)lParam;
	theBoneDensitySwapData.m_pROIView->SetShowNewROIMask(bRed);
	theBoneDensitySwapData.m_pROIView->SetShowNewROIOutMask(bGreen);
	theBoneDensitySwapData.m_pROIView->Invalidate(FALSE);
	return 1;
}

void SeBoneDensityCtrlDlg::OnBnClickedBoneThre()
{
	//todo:二值化
	theBoneDensitySwapData.m_bSeg2Cal = FALSE;
	theBoneDensitySwapData.m_pROIView->SetShowNewMask(FALSE);
	theBoneDensitySwapData.m_pROIView->SetShowNewROIOutMask(FALSE);
	theBoneDensitySwapData.m_pROIView->SetShowNewROIMask(FALSE);
	theBoneDensitySwapData.m_bPosChanged = TRUE;
	CSeNewMaskDlg dlg(
		&theBoneDensitySwapData.m_Histogram[0],
		theBoneDensitySwapData.m_lMaxNumber, 
		theBoneDensitySwapData.m_nMaxValue, 
		theBoneDensitySwapData.m_nMinValue,
		theBoneDensitySwapData.m_nMaxValuePos,
		theBoneDensitySwapData.m_nMinValuePos,
		this);
	theBoneDensitySwapData.m_pROIView->SetShowNewMask(TRUE);
	if (dlg.DoModal() == IDOK)
	{
		theBoneDensitySwapData.m_nMaxValuePos = dlg.GetMax();
		theBoneDensitySwapData.m_nMinValuePos = dlg.GetMin();
		theBoneDensitySwapData.m_pROIView->BinaryzationAllBone(&(g_pBoneDensityModule->m_SliceArray),&(g_pBoneDensityModule->m_MaskArray));
		//theBoneDensitySwapData.m_pROIView->SetDcmArray(&(g_pBoneDensityModule->m_SliceArray));
		theBoneDensitySwapData.m_pROIView->SetDcmArrayMask(&(g_pBoneDensityModule->m_MaskArray));
		theBoneDensitySwapData.m_pROIView->BoneSegDicomPicArrayClone(&(g_pBoneDensityModule->m_MaskArray),&(g_pBoneDensityModule->m_MaskOutArray));
		theBoneDensitySwapData.m_pROIView->SetDcmArrayOutMask(&(g_pBoneDensityModule->m_MaskOutArray));
		
 		theBoneDensitySwapData.m_pROIView->SetShowNewMask(TRUE);
		theBoneDensitySwapData.m_pROIView->SetShowNewROIMask(TRUE);
		theBoneDensitySwapData.m_pROIView->SetShowNewROIOutMask(TRUE);
		theBoneDensitySwapData.m_pROIView->Invalidate(FALSE);
		SendMessage(WM_SIZE);
	}
	else
	{
		theBoneDensitySwapData.m_pROIView->SetShowNewMask(FALSE);
		theBoneDensitySwapData.m_pROIView->Invalidate(FALSE);
	}
}

void SeBoneDensityCtrlDlg::OnBnClickedBoneConfirmregion2()
{
	// TODO: 在此添加控件通知处理程序代码
	if (!theBoneDensitySwapData.m_bSeg2Cal)
	{
		return;
	}

	theBoneDensitySwapData.m_nStep++;
	//theBoneDensitySwapData.m_pROIView->ShowTrabecular(&(g_pBoneDensityModule->m_SliceArray),&(g_pBoneDensityModule->m_ROIArray),&(g_pBoneDensityModule->m_MaskArray));
	

	auto iiter = theBoneDensitySwapData.m_mapFunc.find(theBoneDensitySwapData.m_csSegPartName);
	if (iiter == theBoneDensitySwapData.m_mapFunc.end())
	{
		return;
	}

	theBoneDensitySwapData.m_vecFuncList.clear();

	if (theBoneDensitySwapData.m_csSegPartName == "trabecular")
	{
		//mask
		g_pBoneDensityModule->GetBoneSegInner();
		theBoneDensitySwapData.m_pBinaryView->SetDcmArray(&(g_pBoneDensityModule->m_ROIArray));
		theBoneDensitySwapData.m_pBinaryView->SetShowNewMask(FALSE);
		SendMessage(WM_SIZE);
		g_pBoneDensityModule->GetUI()->SendMessage(WM_SIZE);

	}
	else if (theBoneDensitySwapData.m_csSegPartName == "corticalbone")
	{
		//mask and mask out
		g_pBoneDensityModule->GetBoneSegOuter();
		theBoneDensitySwapData.m_pBinaryView->SetDcmArray(&(g_pBoneDensityModule->m_ROIArray));
		theBoneDensitySwapData.m_pBinaryView->SetShowNewMask(FALSE);
		SendMessage(WM_SIZE);
		g_pBoneDensityModule->GetUI()->SendMessage(WM_SIZE);
	}
	else{
		return;
	}


	
	

}

// void SeBoneDensityCtrlDlg::OnBnClickedBoneConfirmregionOuter()
// {
// 	// TODO: 在此添加控件通知处理程序代码
// 	if (!theBoneDensitySwapData.m_pROIView->GetShowNewROIOutMask())
// 	{
// 		return;
// 	}
// 	theBoneDensitySwapData.m_nStep++;
// 	
// 	g_pBoneDensityModule->GetBoneSegOuter();
// 	//theBoneDensitySwapData.m_pROIView->ShowTrabecular(&(g_pBoneDensityModule->m_SliceArray),&(g_pBoneDensityModule->m_ROIArray),&(g_pBoneDensityModule->m_MaskOutArray));
// 	theBoneDensitySwapData.m_pBinaryView->SetDcmArray(&(g_pBoneDensityModule->m_ROIArray));
// 	SendMessage(WM_SIZE);
// 	g_pBoneDensityModule->GetUI()->SendMessage(WM_SIZE);
// }

void SeBoneDensityCtrlDlg::OnBnClickedBoneExportFileEx()
{
	// TODO: 在此添加控件通知处理程序代码
	if(theBoneDensitySwapData.m_nStep == SP_CALCU)
	{
		theBoneDensitySwapData.m_pROIView->OnExportImage();
	}
	else if(theBoneDensitySwapData.m_nStep == SP_BINARY)
	{
		theBoneDensitySwapData.m_pBinaryView->OnExportImage();
	}
	
}

void SeBoneDensityCtrlDlg::OnBnClickedExportPdf()
{
	// 检查是否已计算参数
	if (m_pResult->m_vecResult.size() == 0)
	{
		MessageBoxTimeout(NULL, "         导出前请先计算参数！       ", "提示", MB_ICONINFORMATION, 0, 500);
		return;
	}

	try
	{
		LOG_INFO("开始生成PDF报告");
		
		// 准备参数数据（紧凑格式，类似Excel表格）
		std::vector<CString> params;
		for (int i = 0; i < m_pResult->m_vecResult.size(); i++)
		{
			CString paramStr;
			paramStr.Format(_T("%s (%s): %.4f %s"),
				m_pResult->m_vecResult[i].strDescription,
				m_pResult->m_vecResult[i].strAbbreviation,
				m_pResult->m_vecResult[i].dValue,
				m_pResult->m_vecResult[i].strUint);
			params.push_back(paramStr);
		}
		
		// 生成PDF报告（使用libharu，直接输出PDF，支持中文）
		CString pdfPath = theBoneDensitySwapData.m_csFolderPath + _T("\\骨密度分析报告.pdf");
		if (SePDFReportGenerator::GenerateReport(
			theBoneDensitySwapData.m_csFolderPath,
			params))
		{
			CString exportMsg;
			exportMsg.Format(_T("[导出] PDF报告: %s"), pdfPath);
			LOG_INFO(exportMsg);
			
			// 自动打开PDF
			ShellExecute(NULL, _T("open"), pdfPath, NULL, NULL, SW_SHOW);
			
			MessageBox(
				_T("PDF报告已生成！\n\n")
				_T("文件位置：\n") + pdfPath + _T("\n\n")
				_T("特点：\n")
				_T("- 紧凑布局，一页写完\n")
				_T("- 白底黑字，简洁清晰\n")
				_T("- 支持中文显示"),
				_T("导出成功"),
				MB_OK | MB_ICONINFORMATION
			);
		}
		else
		{
			LOG_ERROR("生成PDF报告失败，请检查SimSun.ttf字体文件是否存在");
			MessageBox(
				_T("生成PDF报告失败！\n\n")
				_T("可能原因：\n")
				_T("1. SimSun.ttf 字体文件不存在\n")
				_T("2. 输出路径无写权限\n")
				_T("3. 磁盘空间不足\n\n")
				_T("请将 SimSun.ttf 字体文件复制到程序目录或Bin目录下"),
				_T("导出失败"),
				MB_OK | MB_ICONERROR
			);
		}
	}
	catch (...)
	{
		LOG_ERROR("生成PDF报告时发生异常");
		MessageBoxTimeout(NULL, "         生成PDF报告异常！       ", "提示", MB_ICONERROR, 0, 1000);
	}
}
