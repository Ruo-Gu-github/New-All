// SeGeneralModuleDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "SeGeneralModule.h"
#include "SeGeneralModuleDlg.h"
#include "SeGeneralModuleCtrlDlg.h"
#include "afxdialogex.h"
#include "GeneralSwapData.h"
#include "Se3DView.h"
#include "SeMPRView.h"

// SeGeneralModuleDlg 对话框

IMPLEMENT_DYNAMIC(SeGeneralModuleDlg, CDialog)

SeGeneralModuleDlg::SeGeneralModuleDlg(CWnd* pParent /*=NULL*/)
	: CDialog(SeGeneralModuleDlg::IDD, pParent)
{
	m_pOriPicArray = NULL;
	m_showState = SHOW_ALL;
}

SeGeneralModuleDlg::~SeGeneralModuleDlg()
{
	m_pOriPicArray = NULL;
}

void SeGeneralModuleDlg::Reset()
{
	m_pOriPicArray = NULL;
}

void SeGeneralModuleDlg::ShowWnds()
{ 
	SendMessage(WM_SIZE);
	InitMPRPos();
	Init3D();
}

void SeGeneralModuleDlg::ResetWnds(CDcmPicArray * pDcmArray)
{

}

void SeGeneralModuleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(SeGeneralModuleDlg, CDialog)
	ON_WM_ERASEBKGND()
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_MESSAGE(WM_3D_STATE, &SeGeneralModuleDlg::OnChangeState)
END_MESSAGE_MAP()


// SeGeneralModuleDlg 消息处理程序


BOOL SeGeneralModuleDlg::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CRect		rtClient;
	GetClientRect(&rtClient);

	CZKMemDC		MemDC(pDC, afxGlobalData.clrWindow);

	return true;
}


int SeGeneralModuleDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;
	// TODO:  在此添加您专用的创建代码
//  MPR
	m_wndXOY.Create(this, 0, 
		RUNTIME_CLASS(SeVisualLibDoc),
		RUNTIME_CLASS(CImageViewerFrame),
		RUNTIME_CLASS(SeMPRView),
		WS_CHILD | WS_VISIBLE, 0);
	theGeneralSwapData.m_pXOYView = (SeMPRView*)m_wndXOY.GetView();
	theGeneralSwapData.m_pXOYDoc = (SeVisualLibDoc*)theGeneralSwapData.m_pXOYView->GetDocument();
	theGeneralSwapData.m_pXOYView->SetPlaneNum(1);

	m_wndXOZ.Create(this, 0, 
		RUNTIME_CLASS(SeVisualLibDoc),
		RUNTIME_CLASS(CImageViewerFrame),
		RUNTIME_CLASS(SeMPRView),
		WS_CHILD | WS_VISIBLE, 0);
	theGeneralSwapData.m_pXOZView = (SeMPRView*)m_wndXOZ.GetView();
	theGeneralSwapData.m_pXOZDoc = (SeVisualLibDoc*)theGeneralSwapData.m_pXOZView->GetDocument();
	theGeneralSwapData.m_pXOZView->SetPlaneNum(2);

	m_wndYOZ.Create(this, 0, 
		RUNTIME_CLASS(SeVisualLibDoc),
		RUNTIME_CLASS(CImageViewerFrame),
		RUNTIME_CLASS(SeMPRView),
		WS_CHILD | WS_VISIBLE, 0);
	theGeneralSwapData.m_pYOZView = (SeMPRView*)m_wndYOZ.GetView();
	theGeneralSwapData.m_pYOZDoc = (SeVisualLibDoc*)theGeneralSwapData.m_pYOZView->GetDocument();
	theGeneralSwapData.m_pYOZView->SetPlaneNum(3);

// 3D 
	m_wnd3D.Create(this, 0, 
		RUNTIME_CLASS(CImageDisplayDoc),
		RUNTIME_CLASS(CImageViewerFrame),
		RUNTIME_CLASS(Se3DView),
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, 0);
	theGeneralSwapData.m_p3DView = (Se3DView*)m_wnd3D.GetView();

	return 0;
}


void SeGeneralModuleDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	CRect rtHide(0, 0, 0, 0);
	CRect rtClient;
	GetClientRect(&rtClient);
	m_wndXOY.MoveWindow(rtHide);
	m_wndXOZ.MoveWindow(rtHide);
	m_wndYOZ.MoveWindow(rtHide);
	m_wnd3D.MoveWindow(rtHide);
	if (m_wndXOY.GetSafeHwnd() != NULL)
	{
		CRect rtShow(rtClient);
		rtShow.right = rtShow.left + rtClient.Width();	
		CRect rtTopLeft(rtShow.left,rtShow.top, rtShow.left + rtShow.Width()/2, rtShow.top + rtShow.Height()/2);
		CRect rtTopRight(rtShow.left + rtShow.Width()/2, rtShow.top, rtShow.right, rtShow.top + rtShow.Height()/2);
		CRect rtBottomLeft(rtShow.left, rtShow.top + rtShow.Height()/2, rtShow.left + rtShow.Width()/2, rtShow.bottom);
		CRect rtBottomRight(rtShow.left + rtShow.Width()/2, rtShow.top + rtShow.Height()/2, rtShow.right, rtShow.bottom);

		switch (m_showState)
		{
		case SHOW_ALL:
			{
				m_wndXOZ.MoveWindow(rtTopLeft);
				m_wndXOY.MoveWindow(rtTopRight);
				m_wndYOZ.MoveWindow(rtBottomLeft);
				m_wnd3D.MoveWindow(rtBottomRight);
				break;
			}
		case SHOW_FRONT:
			{
				m_wndXOZ.MoveWindow(rtShow);
				break;
			}
		case SHOW_LEFT:
			{
				m_wndYOZ.MoveWindow(rtShow);
				break;
			}
		case SHOW_TOP:
			{
				m_wndXOY.MoveWindow(rtShow);
				break;
			}
		case SHOW_3D:
			{
				m_wnd3D.MoveWindow(rtShow);
				break;
			}
// 		default:
// 			{
// 				m_wndXOZ.MoveWindow(rtTopLeft);
// 				m_wndXOY.MoveWindow(rtTopRight);
// 				m_wndYOZ.MoveWindow(rtBottomLeft);
// 				m_wnd3D.MoveWindow(rtBottomRight);
// 				break;
// 			}
		}
	}
}

void SeGeneralModuleDlg::InitMPRPos()
{
	int x, y, z;
	m_pOriPicArray->GetDcmPicSizeXOY(x, y);
	m_pOriPicArray->GetDcmPicSizeXOZ(x, z);
	theGeneralSwapData.m_pXOYView->SetPicCount(z);
	theGeneralSwapData.m_pXOYView->SetCurrentFrame(z/2);
	theGeneralSwapData.m_pYOZView->SetPicCount(x);
	theGeneralSwapData.m_pYOZView->SetCurrentFrame(x/2);
	theGeneralSwapData.m_pXOZView->SetPicCount(y);
	theGeneralSwapData.m_pXOZView->SetCurrentFrame(y/2);

	theGeneralSwapData.m_pXOYView->SetPlaneNum(1);
	theGeneralSwapData.m_pXOYView->SetDcmArray(m_pOriPicArray);	
	theGeneralSwapData.m_pXOYView->UpdateImage();

	theGeneralSwapData.m_pXOZView->SetPlaneNum(2);
	theGeneralSwapData.m_pXOZView->SetDcmArray(m_pOriPicArray);
	theGeneralSwapData.m_pXOZView->UpdateImage();

	theGeneralSwapData.m_pYOZView->SetPlaneNum(3);
	theGeneralSwapData.m_pYOZView->SetDcmArray(m_pOriPicArray);	
	theGeneralSwapData.m_pYOZView->UpdateImage();



	theGeneralSwapData.m_pXOYView->SetMPRTool();
	theGeneralSwapData.m_pYOZView->SetMPRTool();
	theGeneralSwapData.m_pXOZView->SetMPRTool();


	theGeneralSwapData.m_pXOZView->SendMessage(WM_LBUTTONDOWN, MK_LBUTTON, (LPARAM)MAKELONG(x/2, z/2));
	theGeneralSwapData.m_pYOZView->SendMessage(WM_LBUTTONDOWN, MK_LBUTTON, (LPARAM)MAKELONG(y/2, z/2));
	theGeneralSwapData.m_pXOYView->SendMessage(WM_LBUTTONDOWN, MK_LBUTTON, (LPARAM)MAKELONG(x/2, y/2));

}

LRESULT SeGeneralModuleDlg::OnChangeState(WPARAM wParam, LPARAM lParam)
{
	m_showState = (SHOW_STATE)wParam;
	SendMessage(WM_SIZE);
	return 0;
}

void SeGeneralModuleDlg::Init3D()
{
	theGeneralSwapData.m_p3DView->Reset();
}



// LRESULT SeGeneralModuleDlg::OnScaleOneView(WPARAM wParam, LPARAM lParam)
// {
// 	switch ()
// }