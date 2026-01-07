// SeBoneDensityDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "SeExampleModule.h"
#include "SeExampleModuleDlg.h"
#include "ExampleModuleSwapData.h"
#include "SeExampleView.h"
#include "SeFattyOriView.h"
#include "SeProjectionView.h"
#include "SeFattyZSelectView.h"
// SeExampleModuleDlg 对话框

IMPLEMENT_DYNAMIC(SeExampleModuleDlg, CDialog)
SeExampleModuleDlg::SeExampleModuleDlg(CWnd* pParent /*=NULL*/)
	: CDialog(SeExampleModuleDlg::IDD, pParent)
{
	TRACE("SeExampleModuleDlg ctor: this=%p\n", this);
}

SeExampleModuleDlg::~SeExampleModuleDlg()
{
	TRACE("SeExampleModuleDlg dtor: this=%p\n", this);
	m_pOriDcmArray = NULL;
}

void SeExampleModuleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(SeExampleModuleDlg, CDialog)
	ON_WM_ERASEBKGND()
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()

BOOL SeExampleModuleDlg::OnCmdMsg( UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	return TRUE;
}

// SeExampleModuleDlg 消息处理程序


BOOL SeExampleModuleDlg::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CRect rect;
	GetClientRect(&rect);
	CZKMemDC memDC(pDC);
	memDC.FillSolidRect(rect,RGB(94,94,94));//34,33,73
	return TRUE;
}


int SeExampleModuleDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;
	//  创建 View 窗口

	//m_wndExampleCtrl.Create(this, 0, 
	//	RUNTIME_CLASS(CImageViewerDoc),
	//	RUNTIME_CLASS(CImageViewerFrame),
	//	RUNTIME_CLASS(SeExampleView),
	//	WS_CHILD | WS_VISIBLE, 0);
	//theExampleModuleSwapData.m_pExampleView = (SeExampleView*)m_wndExampleCtrl.GetView();
	//theExampleModuleSwapData.m_pExampleDoc = (CImageViewerDoc*)theExampleModuleSwapData.m_pExampleView->GetDocument();
	//theExampleModuleSwapData.m_pExampleView->SendMessage(WM_COMMAND, IDM_IMAGEVIEWER_LAYOUT_1X1);

	//m_wndOriCtrl.Create(this, 0, 
	//	RUNTIME_CLASS(CImageViewerDoc),
	//	RUNTIME_CLASS(CImageViewerFrame),
	//	RUNTIME_CLASS(CSeFattyOriView),
	//	WS_CHILD | WS_VISIBLE, 0);
	//theExampleModuleSwapData.m_pOriView = (CSeFattyOriView*)m_wndOriCtrl.GetView();
	//theExampleModuleSwapData.m_pOriDoc = (CImageViewerDoc*)theExampleModuleSwapData.m_pOriView->GetDocument();
	//theExampleModuleSwapData.m_pOriView->SendMessage(WM_COMMAND, IDM_IMAGEVIEWER_LAYOUT_1X1);

	m_ProjectionCtrl.Create(this, 0, 
		RUNTIME_CLASS(CImageViewerDoc),
		RUNTIME_CLASS(CImageViewerFrame),
		RUNTIME_CLASS(CSeFattyProjectionView),
		WS_CHILD | WS_VISIBLE, 0);
	theExampleModuleSwapData.m_pProjectionView = (CSeFattyProjectionView*)m_ProjectionCtrl.GetView();
	theExampleModuleSwapData.m_pProjectionDoc = (CImageViewerDoc*)theExampleModuleSwapData.m_pProjectionView->GetDocument();
	theExampleModuleSwapData.m_pProjectionView->SendMessage(WM_COMMAND, IDM_IMAGEVIEWER_LAYOUT_1X1);

	m_wndXOYViewCtrl.Create(this, 0, 
		RUNTIME_CLASS(CImageViewerDoc),
		RUNTIME_CLASS(CImageViewerFrame),
		RUNTIME_CLASS(CSeFattyZSelectView),
		WS_CHILD | WS_VISIBLE, 0);
	theExampleModuleSwapData.m_pXOYView = (CSeFattyZSelectView*)m_wndXOYViewCtrl.GetView();
	theExampleModuleSwapData.m_pXOYDoc = (CImageViewerDoc*)theExampleModuleSwapData.m_pXOYView->GetDocument();
	theExampleModuleSwapData.m_pXOYView->SendMessage(WM_COMMAND, IDM_IMAGEVIEWER_LAYOUT_1X1);

	m_wndXOZViewCtrl.Create(this, 0, 
		RUNTIME_CLASS(CImageViewerDoc),
		RUNTIME_CLASS(CImageViewerFrame),
		RUNTIME_CLASS(CSeFattyZSelectView),
		WS_CHILD | WS_VISIBLE, 0);
	theExampleModuleSwapData.m_pXOZView = (CSeFattyZSelectView*)m_wndXOZViewCtrl.GetView();
	theExampleModuleSwapData.m_pXOZDoc = (CImageViewerDoc*)theExampleModuleSwapData.m_pXOZView->GetDocument();
	theExampleModuleSwapData.m_pXOZView->SendMessage(WM_COMMAND, IDM_IMAGEVIEWER_LAYOUT_1X1);

	m_wndYOZViewCtrl.Create(this, 0, 
		RUNTIME_CLASS(CImageViewerDoc),
		RUNTIME_CLASS(CImageViewerFrame),
		RUNTIME_CLASS(CSeFattyZSelectView),
		WS_CHILD | WS_VISIBLE, 0);
	theExampleModuleSwapData.m_pYOZView = (CSeFattyZSelectView*)m_wndYOZViewCtrl.GetView();
	theExampleModuleSwapData.m_pYOZDoc = (CImageViewerDoc*)theExampleModuleSwapData.m_pYOZView->GetDocument();
	theExampleModuleSwapData.m_pYOZView->SendMessage(WM_COMMAND, IDM_IMAGEVIEWER_LAYOUT_1X1);

	return 0;
}


void SeExampleModuleDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	CRect rect;
	CRect rtHide(0,0,0,0);
	GetClientRect(&rect);
	//// 默认隐藏
	//m_wndExampleCtrl.MoveWindow(rtHide);
	//m_wndOriCtrl.MoveWindow(rtHide);
	m_ProjectionCtrl.MoveWindow(rtHide);
	m_wndXOYViewCtrl.MoveWindow(rtHide);
	m_wndXOZViewCtrl.MoveWindow(rtHide);
	m_wndYOZViewCtrl.MoveWindow(rtHide);
	switch(theExampleModuleSwapData.m_nStep)
	{
	case 0:
		{
			//m_wndOriCtrl.MoveWindow(rect);
			// m_wndExampleCtrl.MoveWindow(rect);
			rect.right = rect.left + rect.Width() / 2;
			rect.bottom = rect.top + rect.Height() / 2;
			m_wndXOYViewCtrl.MoveWindow(rect);
			rect.OffsetRect(rect.Width(), 0);
			m_wndXOZViewCtrl.MoveWindow(rect);
			rect.OffsetRect(-rect.Width(), rect.Height());
			m_wndYOZViewCtrl.MoveWindow(rect);
			rect.OffsetRect(rect.Width(), 0);
			m_ProjectionCtrl.MoveWindow(rect);
			break;
		}
	default:
		break;
	}
}

void SeExampleModuleDlg::Reset()
{
	//if (theExampleModuleSwapData.m_pExampleView != NULL)
	//	theExampleModuleSwapData.m_pExampleView->Reset();
	//if (theExampleModuleSwapData.m_pOriView != NULL)
	//	theExampleModuleSwapData.m_pOriView->Reset();
	if (theExampleModuleSwapData.m_pProjectionView != NULL)
		theExampleModuleSwapData.m_pProjectionView->Reset();
	if (theExampleModuleSwapData.m_pXOYView != NULL)
		theExampleModuleSwapData.m_pXOYView->Reset();
	if (theExampleModuleSwapData.m_pXOZView != NULL)
		theExampleModuleSwapData.m_pXOZView->Reset();
	if (theExampleModuleSwapData.m_pYOZView != NULL)
		theExampleModuleSwapData.m_pYOZView->Reset();
}

void SeExampleModuleDlg::ShowFirstView()
{
	TRACE("ShowFirstView: this=%p, m_pOriDcmArray=%p\n", this, m_pOriDcmArray);
	if (!m_pOriDcmArray) {
		AfxMessageBox(_T("m_pOriDcmArray is NULL!"));
		return;
	}
	//theExampleModuleSwapData.m_pExampleView->SetDcmArray(m_pOriDcmArray);
	//theExampleModuleSwapData.m_pExampleView->Invalidate(FALSE);
 //	theExampleModuleSwapData.m_pOriView->SetDcmArray(m_pOriDcmArray);
	//theExampleModuleSwapData.m_pOriView->Invalidate(FALSE);
	
	theExampleModuleSwapData.m_pProjectionView->SetDcmArray(m_pProjectionArray);
	theExampleModuleSwapData.m_pProjectionView->Invalidate(FALSE);

	int x, y, z;
	m_pOriDcmArray->GetDcmPicSizeXOY(x, y);
	m_pOriDcmArray->GetDcmPicSizeXOZ(x, z);
	theExampleModuleSwapData.m_pXOYView->SetPicCount(z);
	theExampleModuleSwapData.m_pXOYView->SetCurrentFrame(z/2);
	theExampleModuleSwapData.m_pYOZView->SetPicCount(x);
	theExampleModuleSwapData.m_pYOZView->SetCurrentFrame(x/2);
	theExampleModuleSwapData.m_pXOZView->SetPicCount(y);
	theExampleModuleSwapData.m_pXOZView->SetCurrentFrame(y/2);

	theExampleModuleSwapData.m_pXOYView->SetPlaneNum(1);
	theExampleModuleSwapData.m_pXOYView->SetDcmArray(m_pOriDcmArray);	
	theExampleModuleSwapData.m_pXOYView->UpdateImage();

	theExampleModuleSwapData.m_pXOZView->SetPlaneNum(2);
	theExampleModuleSwapData.m_pXOZView->SetDcmArray(m_pOriDcmArray);
	theExampleModuleSwapData.m_pXOZView->UpdateImage();

	theExampleModuleSwapData.m_pYOZView->SetPlaneNum(3);
	theExampleModuleSwapData.m_pYOZView->SetDcmArray(m_pOriDcmArray);	
	theExampleModuleSwapData.m_pYOZView->UpdateImage();

	//theExampleModuleSwapData.m_pXOYView->SetDcmArray(m_pOriDcmArray);
	theExampleModuleSwapData.m_pXOYView->Invalidate(FALSE);

	//theExampleModuleSwapData.m_pXOZView->SetDcmArray(m_pOriDcmArray);
	theExampleModuleSwapData.m_pXOZView->Invalidate(FALSE);

	//theExampleModuleSwapData.m_pYOZView->SetDcmArray(m_pOriDcmArray);
	theExampleModuleSwapData.m_pYOZView->Invalidate(FALSE);
}