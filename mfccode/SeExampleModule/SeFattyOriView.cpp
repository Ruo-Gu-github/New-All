// SeFattyOriView.cpp : 实现文件
//

#include "stdafx.h"
#include "SeExampleModule.h"
#include "ExampleModuleSwapData.h"
#include "SeFattyOriView.h"


// CSeFattyOriView

IMPLEMENT_DYNCREATE(CSeFattyOriView, CImageViewerView)

CSeFattyOriView::CSeFattyOriView()
{
	m_pDcmPicArray = NULL;
	m_bDrawWindowName = FALSE;
}

CSeFattyOriView::~CSeFattyOriView()
{
}

BEGIN_MESSAGE_MAP(CSeFattyOriView, CImageViewerView)
END_MESSAGE_MAP()


// CSeFattyOriView 绘图

void CSeFattyOriView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();

	CRect			rtClient;
	GetClientRect(&rtClient);
	CZKMemDC			MemDC(pDC, rtClient, RGB(0,0,0));

	CImageViewerView::OnDraw(&MemDC);
	CString strName = "原图";
	CFont fontname;
	fontname.CreatePointFont(100, "System", &MemDC); 
	MemDC.SelectObject(&fontname);
	MemDC.SetTextColor(RGB(0,255,0));
	MemDC.TextOut(5,5,strName);

	//DrawWindowName(&MemDC);
	//if (m_bDrawWindowName)
	//{
	//	DrawWindowName(&MemDC);
	//}
}


// CSeFattyOriView 诊断

#ifdef _DEBUG
void CSeFattyOriView::AssertValid() const
{
	CImageViewerView::AssertValid();
}

#ifndef _WIN32_WCE
void CSeFattyOriView::Dump(CDumpContext& dc) const
{
	CImageViewerView::Dump(dc);
}
#endif
#endif //_DEBUG


// CSeFattyOriView 消息处理程序

void CSeFattyOriView::OnInitialUpdate()
{
	CImageViewerView::OnInitialUpdate();
	SendMessage(WM_COMMAND, IDM_IMAGEVIEWER_LAYOUT_1X1);
	// TODO: 在此添加专用代码和/或调用基类
}

void CSeFattyOriView::SetDcmArray( CDcmPicArray* pDcmPicArray )
{
	m_pDcmPicArray = pDcmPicArray;
	CImageViewerDoc* pDoc = (CImageViewerDoc*)GetDocument();
	pDoc->ReleaseSeries();
	//int nCount = m_pDcmPicArray->GetZDcmPicCount();
	for (int i = 0; i < m_pDcmPicArray->GetZDcmPicCount(); i++)
	{
		//m_pDcmPicArray->GetDcmArray()[i]->SetDataInMem(true);
		pDoc->AddImage(m_pDcmPicArray->GetDcmArray()[i], -1, FALSE);
	}
}

const void CSeFattyOriView::Reset()
{
	CImageViewerDoc* pDoc = (CImageViewerDoc*)GetDocument();
	pDoc->ReleaseSeries();
}

void CSeFattyOriView::DrawWindowName(CDC* pDC)
{
	CDcmPic* pDcm = (CDcmPic*)GetDisplayMgr()->GetCurrentImage();
	if (pDcm == NULL)
		return;

	CString strName = "原图";
	CFont fontname;
	fontname.CreatePointFont(100, "System", pDC); 
	pDC->SelectObject(&fontname);
	pDC->SetTextColor(RGB(0,255,0));
	pDC->TextOut(5,5,strName);
}

HBITMAP CSeFattyOriView::GetScreenImage(LPRECT lpRect)
{
	HDC hScrDC, hMemDC;             // 屏幕和内存设备描述表  
	HBITMAP hBitmap, hOldBitmap;    // 位图句柄  
	int nX, nY, nX2, nY2;           // 选定区域坐标  
	int nWidth, nHeight;            // 位图宽度和高度  
	int xScrn, yScrn;               // 屏幕分辨率  

	if (IsRectEmpty(lpRect))  
		return NULL;  

	hScrDC = CreateDC("DISPLAY", NULL, NULL, NULL);     // 为屏幕创建设备描述表  

	hMemDC = CreateCompatibleDC(hScrDC);                // 为屏幕设备描述表创建兼容的内存设备描述表  

	nX = lpRect->left;  
	nY = lpRect->top;  
	nX2 = lpRect->right;  
	nY2 = lpRect->bottom;  

	xScrn = GetDeviceCaps(hScrDC, HORZRES); // 获得屏幕水平分辨率  
	yScrn = GetDeviceCaps(hScrDC, VERTRES);  

	if (nX < 0)  nX = 0;  
	if (nY < 0)  nY = 0;  
	if (nX2 > xScrn)  nX2 = xScrn;  
	if (nY2 > yScrn)  nY2 = yScrn;  
	nWidth = nX2 - nX;  
	nHeight = nY2 - nY;  

	hBitmap = CreateCompatibleBitmap(hScrDC, nWidth, nHeight);      // 创建一个与屏幕设备描述表兼容的位图  
	hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);            // 把新位图选到内存设备描述表中  
	BitBlt(hMemDC, 0, 0, nWidth, nHeight, hScrDC, nX, nY, SRCCOPY); // 把屏幕设备描述表拷贝到内存设备描述表中  
	hBitmap = (HBITMAP)SelectObject(hMemDC, hOldBitmap);            // 得到屏幕位图的句柄  

	DeleteDC(hScrDC);  
	DeleteDC(hMemDC);  

	return hBitmap;   
}