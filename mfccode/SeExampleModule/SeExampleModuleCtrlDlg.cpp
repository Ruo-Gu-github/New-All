// SeExampleModuleCtrlDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "SeExampleModule.h"
#include "SeExampleModuleDlg.h"
#include "SeExampleModuleCtrlDlg.h"
#include "ExampleModuleSwapData.h"
#include "SeExampleView.h"
#include "SeNewMaskDlg.h"
#include "SeFattyZSelectView.h"
#include "SeResultDlg.h"
#include "SeROIData.h"

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

// SeExampleModuleCtrlDlg 对话框

IMPLEMENT_DYNAMIC(SeExampleModuleCtrlDlg, CSeDialogBase)

	SeExampleModuleCtrlDlg::SeExampleModuleCtrlDlg(CWnd* pParent /*=NULL*/)
	: CSeDialogBase(SeExampleModuleCtrlDlg::IDD, pParent)
{
	m_nFatMin = 0;
	m_nFatMax = 0;
	m_nLungMin = 0;
	m_nLungMax = 0;
	m_nBoneMin = 0;
	m_nBoneMax = 0;
	m_nNowPos = 0;
	m_nSelect = 0;
}

SeExampleModuleCtrlDlg::~SeExampleModuleCtrlDlg()
{

}

void SeExampleModuleCtrlDlg::DoDataExchange(CDataExchange* pDX)
{
	CSeDialogBase::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(SeExampleModuleCtrlDlg, CSeDialogBase)
	ON_WM_ERASEBKGND()
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_BUTTON_EXAMPLE_1, &SeExampleModuleCtrlDlg::OnBnClickedButtonExample1)
	ON_BN_CLICKED(IDC_BUTTON_EXAMPLE_2, &SeExampleModuleCtrlDlg::OnBnClickedButtonExample2)
	ON_BN_CLICKED(IDC_BUTTON_MINIP2, &SeExampleModuleCtrlDlg::OnBnClickedButtonMinip2)
	ON_BN_CLICKED(IDC_BUTTON_MINIP3, &SeExampleModuleCtrlDlg::OnBnClickedButtonMinip3)
	ON_BN_CLICKED(IDC_BUTTON_MINIP4, &SeExampleModuleCtrlDlg::OnBnClickedButtonMinip4)
	ON_BN_CLICKED(IDC_BUTTON_MIP, &SeExampleModuleCtrlDlg::OnBnClickedButtonMip)
	ON_BN_CLICKED(IDC_BUTTON_MINIP, &SeExampleModuleCtrlDlg::OnBnClickedButtonMinip)
	ON_MESSAGE(WM_MASK, &SeExampleModuleCtrlDlg::OnAddMask) 
	ON_BN_CLICKED(IDC_BUTTON_MINIP5, &SeExampleModuleCtrlDlg::OnBnClickedButtonMinip5)
END_MESSAGE_MAP()


// SeExampleModuleCtrlDlg 消息处理程序


BOOL SeExampleModuleCtrlDlg::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CRect		rtClient;
	GetClientRect(&rtClient);
	CRect rtShow(rtClient);
	CZKMemDC		MemDC(pDC, rtShow,RGB(94,94,94)/*afxGlobalData.clrWindow*/);
	return TRUE;
}


int SeExampleModuleCtrlDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	return 0;
}

HBRUSH SeExampleModuleCtrlDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
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

void SeExampleModuleCtrlDlg::OnSize(UINT nType, int cx, int cy)
{
	CSeDialogBase::OnSize(nType, cx, cy);
	// 获得第一个控件的指针确认 dlg 和控件创建完成
	//CWnd* pWnd = GetDlgItem(IDC_BONE_CHIPTOOL);
	CWnd* pWnd = GetDlgItem(IDC_BUTTON_EXAMPLE_1);

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
		// 默认隐藏所有按钮
		GetDlgItem(IDC_BUTTON_EXAMPLE_1        )->MoveWindow(rtHide);
		GetDlgItem(IDC_BUTTON_EXAMPLE_2        )->MoveWindow(rtHide);
		GetDlgItem(IDC_BUTTON_MIP        )->MoveWindow(rtHide);
		GetDlgItem(IDC_BUTTON_MINIP        )->MoveWindow(rtHide);
		GetDlgItem(IDC_BUTTON_MINIP2        )->MoveWindow(rtHide);
		GetDlgItem(IDC_BUTTON_MINIP3       )->MoveWindow(rtHide);
		GetDlgItem(IDC_BUTTON_MINIP4        )->MoveWindow(rtHide);	
		GetDlgItem(IDC_BUTTON_MINIP5        )->MoveWindow(rtHide);
		switch(theExampleModuleSwapData.m_nStep)
		{
		case 0:
			{
				// 从上往下的按钮
				CRect rtBtn(rtShow);
				GetDlgItem(IDC_BUTTON_EXAMPLE_1)->MoveWindow(rtBtn);
				rtBtn.OffsetRect(0, 40);
				GetDlgItem(IDC_BUTTON_EXAMPLE_2)->MoveWindow(rtBtn);
				rtBtn.OffsetRect(0, 40);
				GetDlgItem(IDC_BUTTON_MIP)->MoveWindow(rtBtn);
				rtBtn.OffsetRect(0, 40);
				GetDlgItem(IDC_BUTTON_MINIP)->MoveWindow(rtBtn);
				rtBtn.OffsetRect(0, 40);
				GetDlgItem(IDC_BUTTON_MINIP2)->MoveWindow(rtBtn);
				rtBtn.OffsetRect(0, 40);
				GetDlgItem(IDC_BUTTON_MINIP3)->MoveWindow(rtBtn);
				rtBtn.OffsetRect(0, 40);
				GetDlgItem(IDC_BUTTON_MINIP4)->MoveWindow(rtBtn);
				rtBtn.OffsetRect(0, 40);
				GetDlgItem(IDC_BUTTON_MINIP5)->MoveWindow(rtBtn);
				break;
			}
		default:
			break;
		}

	}
}

void SeExampleModuleCtrlDlg::Reset()
{
	theExampleModuleSwapData.m_pXOYView->Reset();
}

BOOL SeExampleModuleCtrlDlg::OnInitDialog()
{
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void SeExampleModuleCtrlDlg::OnBnClickedButtonExample1()
{
	m_nSelect = 1;
	// 设定脂肪阈值范围
	CSeNewMaskDlg dlg(
		&theExampleModuleSwapData.m_Histogram[0],
		theExampleModuleSwapData.m_lMaxNumber, 
		theExampleModuleSwapData.m_nMaxValue, 
		theExampleModuleSwapData.m_nMinValue,
		theExampleModuleSwapData.m_nMaxValue,
		theExampleModuleSwapData.m_nMinValue,
		this);
	if (dlg.DoModal() == IDOK)
	{
		theExampleModuleSwapData.m_pXOYView->Invalidate(FALSE);
		theExampleModuleSwapData.m_pXOZView->Invalidate(FALSE);
		theExampleModuleSwapData.m_pYOZView->Invalidate(FALSE);
		SendMessage(WM_SIZE);
	}
	else
	{
		m_nSelect = 0;
		theExampleModuleSwapData.m_pXOYView->SetSelect(m_nSelect);
		theExampleModuleSwapData.m_pXOYView->Invalidate(FALSE);

		theExampleModuleSwapData.m_pXOZView->SetSelect(m_nSelect);
		theExampleModuleSwapData.m_pXOZView->Invalidate(FALSE);

		theExampleModuleSwapData.m_pYOZView->SetSelect(m_nSelect);
		theExampleModuleSwapData.m_pYOZView->Invalidate(FALSE);
	}
}


void SeExampleModuleCtrlDlg::OnBnClickedButtonExample2()
{
	m_nSelect = 2;
	// 设定肺阈值范围
	CSeNewMaskDlg dlg(
		&theExampleModuleSwapData.m_Histogram[0],
		theExampleModuleSwapData.m_lMaxNumber, 
		theExampleModuleSwapData.m_nMaxValue, 
		theExampleModuleSwapData.m_nMinValue,
		theExampleModuleSwapData.m_nMaxValue,
		theExampleModuleSwapData.m_nMinValue,
		this);
	if (dlg.DoModal() == IDOK)
	{
		theExampleModuleSwapData.m_pXOYView->Invalidate(FALSE);
		theExampleModuleSwapData.m_pXOZView->Invalidate(FALSE);
		theExampleModuleSwapData.m_pYOZView->Invalidate(FALSE);
		SendMessage(WM_SIZE);
	}
	else
	{
		m_nSelect = 0;
		theExampleModuleSwapData.m_pXOYView->SetSelect(m_nSelect);
		theExampleModuleSwapData.m_pXOYView->Invalidate(FALSE);

		theExampleModuleSwapData.m_pXOZView->SetSelect(m_nSelect);
		theExampleModuleSwapData.m_pXOZView->Invalidate(FALSE);

		theExampleModuleSwapData.m_pYOZView->SetSelect(m_nSelect);
		theExampleModuleSwapData.m_pYOZView->Invalidate(FALSE);
	}
}

void SeExampleModuleCtrlDlg::OnBnClickedButtonMip()
{
	// TODO: 在此添加控件通知处理程序代码
	// 设置骨头阈值范围
	m_nSelect = 3;
	// 设定肺阈值范围
	CSeNewMaskDlg dlg(
		&theExampleModuleSwapData.m_Histogram[0],
		theExampleModuleSwapData.m_lMaxNumber, 
		theExampleModuleSwapData.m_nMaxValue, 
		theExampleModuleSwapData.m_nMinValue,
		theExampleModuleSwapData.m_nMaxValue,
		theExampleModuleSwapData.m_nMinValue,
		this);
	if (dlg.DoModal() == IDOK)
	{
		theExampleModuleSwapData.m_pXOYView->Invalidate(FALSE);
		theExampleModuleSwapData.m_pXOZView->Invalidate(FALSE);
		theExampleModuleSwapData.m_pYOZView->Invalidate(FALSE);
		SendMessage(WM_SIZE);
	}
	else
	{
		m_nSelect = 0;
		theExampleModuleSwapData.m_pXOYView->SetSelect(m_nSelect);
		theExampleModuleSwapData.m_pXOYView->Invalidate(FALSE);

		theExampleModuleSwapData.m_pXOZView->SetSelect(m_nSelect);
		theExampleModuleSwapData.m_pXOZView->Invalidate(FALSE);

		theExampleModuleSwapData.m_pYOZView->SetSelect(m_nSelect);
		theExampleModuleSwapData.m_pYOZView->Invalidate(FALSE);
	}
}


void SeExampleModuleCtrlDlg::OnBnClickedButtonMinip2()
{
	// TODO: 在此添加控件通知处理程序代码
	// 智能识别肺部脂肪
	m_nSelect = 0;

	theExampleModuleSwapData.m_pXOYView->SetSelect(m_nSelect);
	theExampleModuleSwapData.m_pXOYView->Invalidate(FALSE);

	theExampleModuleSwapData.m_pXOZView->SetSelect(m_nSelect);
	theExampleModuleSwapData.m_pXOZView->Invalidate(FALSE);

	theExampleModuleSwapData.m_pYOZView->SetSelect(m_nSelect);
	theExampleModuleSwapData.m_pYOZView->Invalidate(FALSE);

	theExampleModuleSwapData.m_pXOYView->SeprateLung();
}


void SeExampleModuleCtrlDlg::OnBnClickedButtonMinip3()
{
	// TODO: 在此添加控件通知处理程序代码
	// 导出范围内脂肪
	m_nSelect = 0;
	theExampleModuleSwapData.m_pXOYView->SetSelect(m_nSelect);
	theExampleModuleSwapData.m_pXOYView->Invalidate(FALSE);

	theExampleModuleSwapData.m_pXOZView->SetSelect(m_nSelect);
	theExampleModuleSwapData.m_pXOZView->Invalidate(FALSE);

	theExampleModuleSwapData.m_pYOZView->SetSelect(m_nSelect);
	theExampleModuleSwapData.m_pYOZView->Invalidate(FALSE);

	theExampleModuleSwapData.m_pXOYView->ExpertFats();
}


void SeExampleModuleCtrlDlg::OnBnClickedButtonMinip4()
{
	// TODO: 在此添加控件通知处理程序代码
	m_nSelect = 0;
	theExampleModuleSwapData.m_pXOYView->SetSelect(m_nSelect);
	theExampleModuleSwapData.m_pXOYView->Invalidate(FALSE);

	theExampleModuleSwapData.m_pXOZView->SetSelect(m_nSelect);
	theExampleModuleSwapData.m_pXOZView->Invalidate(FALSE);

	theExampleModuleSwapData.m_pYOZView->SetSelect(m_nSelect);
	theExampleModuleSwapData.m_pYOZView->Invalidate(FALSE);

	CSeResultDlg dlg;

	// 获取原图数据和参数
	short* pImage = theExampleModuleSwapData.m_pSrcData;
	int width = theExampleModuleSwapData.m_nWidth;
	int height = theExampleModuleSwapData.m_nHeight;
	int depth = theExampleModuleSwapData.m_nLength;
	double xySpacing = theExampleModuleSwapData.m_dbXYSliceSpace;
	double zSpacing = theExampleModuleSwapData.m_dbZSliceSpace;
	double density = 0.9196; // 这里填脂肪密度，单位自定

	struct MaskInfo {
		CSeROIData* pROI;
		CString name;
	} masks[6] = {
		{ theExampleModuleSwapData.m_pXOYView->m_pFatInside,   _T("腹腔内脏脂肪") },
		{ theExampleModuleSwapData.m_pXOYView->m_pFatOutside,  _T("腹腔皮下脂肪") },
		{ theExampleModuleSwapData.m_pXOYView->m_pLungInside,  _T("肺部内脏脂肪") },
		{ theExampleModuleSwapData.m_pXOYView->m_pLungOutside, _T("肺部皮下脂肪") },
		{ theExampleModuleSwapData.m_pXOYView->m_pLung,        _T("肺部") },
		{ theExampleModuleSwapData.m_pXOYView->m_pBone,        _T("骨骼") }
	};

	for (int i = 0; i < 6; ++i) {
		if (masks[i].pROI != NULL) {
			BYTE* pMask = masks[i].pROI->GetData();
			dlg.AddMaskStat(
				pMask,
				pImage,
				width, height, depth,
				xySpacing, zSpacing, density,
				masks[i].name
				);
		}
	}

	dlg.DoModal();
}



void SeExampleModuleCtrlDlg::OnBnClickedButtonMinip()
{
	// TODO: 在此添加控件通知处理程序代码
	// 智能识别腹腔脂肪
	m_nSelect = 0;

	theExampleModuleSwapData.m_pXOYView->SetSelect(m_nSelect);
	theExampleModuleSwapData.m_pXOYView->Invalidate(FALSE);

	theExampleModuleSwapData.m_pXOZView->SetSelect(m_nSelect);
	theExampleModuleSwapData.m_pXOZView->Invalidate(FALSE);

	theExampleModuleSwapData.m_pYOZView->SetSelect(m_nSelect);
	theExampleModuleSwapData.m_pYOZView->Invalidate(FALSE);

	theExampleModuleSwapData.m_pXOYView->SeprateFat();
}

LRESULT SeExampleModuleCtrlDlg::OnAddMask(WPARAM wParam, LPARAM lParam)
{
	int nMin = (int)wParam;
	int nMax = (int)lParam;
	if (m_nSelect == 1)
	{
		m_nFatMin = nMin;
		m_nFatMax = nMax;
	}
	else if (m_nSelect == 2) 
	{
		m_nLungMin = nMin;
		m_nLungMax = nMax;
	}
	else if (m_nSelect == 3)
	{
		m_nBoneMin = nMin;
		m_nBoneMax = nMax;
	}
	theExampleModuleSwapData.m_pXOYView->SetInfo(nMin, nMax, RGB(255, 0, 0), 64, m_nSelect);
	theExampleModuleSwapData.m_pXOYView->Invalidate(FALSE);

	theExampleModuleSwapData.m_pXOZView->SetInfo(nMin, nMax, RGB(255, 0, 0), 64, m_nSelect);
	theExampleModuleSwapData.m_pXOZView->Invalidate(FALSE);

	theExampleModuleSwapData.m_pYOZView->SetInfo(nMin, nMax, RGB(255, 0, 0), 64, m_nSelect);
	theExampleModuleSwapData.m_pYOZView->Invalidate(FALSE);

	return 0;
}


void SeExampleModuleCtrlDlg::OnBnClickedButtonMinip5()
{
	// TODO: 在此添加控件通知处理程序代码
	Reset();
	theExampleModuleSwapData.m_pXOYView->Invalidate(FALSE);
	theExampleModuleSwapData.m_pXOZView->Invalidate(FALSE);
	theExampleModuleSwapData.m_pYOZView->Invalidate(FALSE);
	
}
