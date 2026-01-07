#include "StdAfx.h"
#include "SeProjectionView.h"
#include "SeFattyZSelectView.h"
#include "ExampleModuleSwapData.h"


// CSeFattyProjectionView

IMPLEMENT_DYNCREATE(CSeFattyProjectionView, CImageViewerView)

	CSeFattyProjectionView::CSeFattyProjectionView()
{
	m_pDcmPicArray = NULL;
	m_nPlaneNum = 0;
	m_nStartPos = 0;
	m_nEndPos = 0;
	m_nWidthZ = 0;
	m_nHeightZ = 0;
	m_bDraggingRed = FALSE;
	m_bDraggingGreen = FALSE;
	m_bMPR = false;
	LINE_HIT_THRESHOLD = 20;
}

CSeFattyProjectionView::~CSeFattyProjectionView()
{
}

BEGIN_MESSAGE_MAP(CSeFattyProjectionView, CImageViewerView)
	ON_WM_VSCROLL()
	ON_WM_MOUSEWHEEL()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()


// CSeFattyProjectionView 绘图

void CSeFattyProjectionView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	if (!pDoc) return;

	CRect rtClient;
	GetClientRect(&rtClient);

	// 双缓冲
	CZKMemDC MemDC(pDC, rtClient, RGB(0, 0, 0));

	// 先调用父类绘图（例如图像）
	CImageViewerView::OnDraw(&MemDC);
	
	// ===== 添加画线逻辑 =====

	// 假设你已经设置好 m_imageWidth, m_imageHeight（图像实际像素大小）
	// 假设你计算出了 m_imageRect（图像在屏幕上的显示位置，考虑了缩放和居中）
	// 如果你还没算，请告诉我图像缩放显示的方式，我来帮你算

	if (m_nWidthZ == 0 || m_nHeightZ <= 0)
		return;
	UpdateImageRect();
	int redY = ImageYToClientY(m_nStartPos);
	int greenY = ImageYToClientY(m_nEndPos);

	// 画红线
	CPen redPen(PS_SOLID, 1, RGB(255, 0, 0));
	CPen greenPen(PS_SOLID, 1, RGB(0, 255, 0));
	CPen* pOldPen = MemDC.SelectObject(&redPen);
	MemDC.MoveTo(0, redY);
	MemDC.LineTo(rtClient.right, redY);

	// 画绿线
	MemDC.SelectObject(&greenPen);
	MemDC.MoveTo(0, greenY);
	MemDC.LineTo(rtClient.right, greenY);

	// 恢复原来的笔
	MemDC.SelectObject(pOldPen);

	CString strInfo;
	strInfo.Format(_T("Start: %d\nEnd: %d\nGap: %d"), m_nStartPos, m_nEndPos, m_nEndPos - m_nStartPos);

	// 设置文字颜色和背景透明
	MemDC.SetTextColor(RGB(255, 255, 255)); // 白色文字
	MemDC.SetBkMode(TRANSPARENT);

	// 设定文本显示区域（左上角）
	CRect textRect = CRect(10, 10, 300, 60); // 宽高根据需要调整

	// 画文本
	MemDC.DrawText(strInfo, textRect, DT_LEFT | DT_TOP | DT_WORDBREAK);
}



// CSeFattyProjectionView 诊断

#ifdef _DEBUG
void CSeFattyProjectionView::AssertValid() const
{
	CImageViewerView::AssertValid();
}

#ifndef _WIN32_WCE
void CSeFattyProjectionView::Dump(CDumpContext& dc) const
{
	CImageViewerView::Dump(dc);
}
#endif
#endif //_DEBUG


// CSeFattyProjectionView 消息处理程序


void CSeFattyProjectionView::OnInitialUpdate()
{
	CImageViewerView::OnInitialUpdate();
	SendMessage(WM_COMMAND, IDM_IMAGEVIEWER_LAYOUT_1X1);
	// TODO: 在此添加专用代码和/或调用基类
}

void CSeFattyProjectionView::SetDcmArray( CDcmPicArray* pDcmPicArray )
{
	m_pDcmPicArray = pDcmPicArray;
	CImageViewerDoc* pDoc = (CImageViewerDoc*)GetDocument();
	pDoc->ReleaseSeries();
	for (int i = 0; i < m_pDcmPicArray->GetZDcmPicCount(); i++)
	{
		m_pDcmPicArray->GetDcmArray()[i]->SetDataInMem(true);
		pDoc->AddImage(m_pDcmPicArray->GetDcmArray()[i], -1, FALSE);
	}
	m_nWidthZ = m_pDcmPicArray->GetDcmArray()[0]->GetWidth();
	m_nHeightZ = m_pDcmPicArray->GetDcmArray()[0]->GetHeight();
	if (m_nHeightZ > 300) {
		m_nStartPos = 50;
		m_nEndPos = m_nHeightZ - 50;
	} else {
		m_nStartPos = 1;
		m_nEndPos = m_nHeightZ - 1;
	}	
}

const void CSeFattyProjectionView::Reset()
{
	CImageViewerDoc* pDoc = (CImageViewerDoc*)GetDocument();
	pDoc->ReleaseSeries();
}

int CSeFattyProjectionView::ImageYToClientY(int y)
{
	return m_imageRect.top + (int)((double)y / m_nHeightZ * m_imageRect.Height());
}


int CSeFattyProjectionView::ClientYToImageY(int y)
{
	return (int)(((double)(y - m_imageRect.top)) / m_imageRect.Height() * m_nHeightZ);
}


BOOL CSeFattyProjectionView::IsNearLine(int clientY, int lineY)
{
	return abs(clientY - lineY) <= LINE_HIT_THRESHOLD;
}

void CSeFattyProjectionView::UpdateImageRect()
{
	CRect client;
	GetClientRect(&client);

	double clientRatio = (double)client.Width() / client.Height();
	double imageRatio = (double)m_nWidthZ / m_nHeightZ;

	if (imageRatio > clientRatio) {
		int imgW = client.Width();
		int imgH = (int)(imgW / imageRatio);
		int yOffset = (client.Height() - imgH) / 2;
		m_imageRect.SetRect(0, yOffset, imgW, yOffset + imgH);
	} else {
		int imgH = client.Height();
		int imgW = (int)(imgH * imageRatio);
		int xOffset = (client.Width() - imgW) / 2;
		m_imageRect.SetRect(xOffset, 0, xOffset + imgW, imgH);
	}

}

BOOL CSeFattyProjectionView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	ScreenToClient(&pt);

	int redY = ImageYToClientY(m_nStartPos);
	int greenY = ImageYToClientY(m_nEndPos);

	int delta = zDelta > 0 ? -1 : 1;

	if (IsNearLine(pt.y, redY)) {
		m_nStartPos = CLAMP(m_nStartPos + delta, 0, m_nHeightZ - 1);
		theExampleModuleSwapData.m_pXOYView->UpdateImage(m_nHeightZ - m_nStartPos);
		theExampleModuleSwapData.m_pXOYView->Invalidate(FALSE);
		theExampleModuleSwapData.m_pXOYView->UpdateWindow();
		Invalidate();
		return TRUE;
	} else if (IsNearLine(pt.y, greenY)) {
		m_nEndPos = CLAMP(m_nEndPos + delta, 0, m_nHeightZ - 1);
		theExampleModuleSwapData.m_pXOYView->UpdateImage(m_nHeightZ - m_nEndPos);
		theExampleModuleSwapData.m_pXOYView->Invalidate(FALSE);
		theExampleModuleSwapData.m_pXOYView->UpdateWindow();
		Invalidate();
		return TRUE;
	}


	return CView::OnMouseWheel(nFlags, zDelta, pt);
}



void CSeFattyProjectionView::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (m_bDraggingRed || m_bDraggingGreen) {
		int deltaY = point.y - m_lastMousePoint.y;
		int deltaImageY = (int)((double)deltaY / m_imageRect.Height() * m_nHeightZ);

		if (m_bDraggingRed) {
			m_nStartPos = CLAMP(m_nStartPos + deltaImageY, 0, m_nHeightZ - 1);
			theExampleModuleSwapData.m_pXOYView->UpdateImage(m_nHeightZ - m_nStartPos);
			theExampleModuleSwapData.m_pXOYView->Invalidate(FALSE);
			theExampleModuleSwapData.m_pXOYView->UpdateWindow();
		} else if (m_bDraggingGreen) {
			m_nEndPos = CLAMP(m_nEndPos + deltaImageY, 0, m_nHeightZ - 1);
			theExampleModuleSwapData.m_pXOYView->UpdateImage(m_nHeightZ - m_nEndPos);
			theExampleModuleSwapData.m_pXOYView->Invalidate(FALSE);
			theExampleModuleSwapData.m_pXOYView->UpdateWindow();
		}

		m_lastMousePoint = point;
		Invalidate();
	}
	CImageViewerView::OnMouseMove(nFlags, point);
}


void CSeFattyProjectionView::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (!m_imageRect.PtInRect(point)) {
		CImageViewerView::OnLButtonDown(nFlags, point);
		return;
	}
	int redY = ImageYToClientY(m_nStartPos);
	int greenY = ImageYToClientY(m_nEndPos);

	if (IsNearLine(point.y, redY)) {
		m_bDraggingRed = true;
	} else if (IsNearLine(point.y, greenY)) {
		m_bDraggingGreen = true;
	}

	m_lastMousePoint = point;
	SetCapture();
	
	CImageViewerView::OnLButtonDown(nFlags, point);
}


void CSeFattyProjectionView::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (m_bDraggingRed || m_bDraggingGreen) {
		m_bDraggingRed = false;
		m_bDraggingGreen = false;
		ReleaseCapture();
    }
	CImageViewerView::OnLButtonUp(nFlags, point);
}
