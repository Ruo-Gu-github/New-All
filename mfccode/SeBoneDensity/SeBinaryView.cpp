// SeBinaryView.cpp : 实现文件
//

#include "stdafx.h"
#include "SeBoneDensity.h"
#include "SeBinaryView.h"


// SeBinaryView

IMPLEMENT_DYNCREATE(SeBinaryView, CImageViewerView)

SeBinaryView::SeBinaryView()
{
	m_pDcmPicArray = NULL;
	m_bNewMask = FALSE;
}

SeBinaryView::~SeBinaryView()
{
	m_pDcmPicArray = NULL;
}

BEGIN_MESSAGE_MAP(SeBinaryView, CImageViewerView)
END_MESSAGE_MAP()


// SeBinaryView 绘图

void SeBinaryView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();

	CRect			rtClient;
	GetClientRect(&rtClient);
	CZKMemDC			MemDC(pDC, rtClient, RGB(0,0,0));

	CImageViewerView::OnDraw(&MemDC);

	if (!m_bNewMask)
		return;
	CImageBase* pImg = GetDisplayMgr()->GetFirstDispImage();
	Graphics gc(MemDC.GetSafeHdc());
	if (pImg != NULL)
	{
		Bitmap* pBmp = GreatePng(pImg, m_nMin, m_nMax, m_color, m_alpha);
		CRect rect = pImg->GetDrawRect();
		Image* pMyImg = pBmp->GetThumbnailImage(rect.Width(), rect.Height());
		gc.DrawImage(pMyImg, rect.left, rect.top, rect.Width(), rect.Height());
		Safe_Delete(pBmp);
		Safe_Delete(pMyImg);
	}
}


// SeBinaryView 诊断

#ifdef _DEBUG
void SeBinaryView::AssertValid() const
{
	CImageViewerView::AssertValid();
}

#ifndef _WIN32_WCE
void SeBinaryView::Dump(CDumpContext& dc) const
{
	CImageViewerView::Dump(dc);
}

void SeBinaryView::OnExportImage()
{
	CImageViewerView::OnImageviewerExport();
}

void SeBinaryView::SetDcmArray(CDcmPicArray* pDcmArray)
{
	m_pDcmPicArray = pDcmArray;
	CImageViewerDoc* pDoc = (CImageViewerDoc*)GetDocument();
	pDoc->ReleaseSeries();
	for (int i = 0; i < m_pDcmPicArray->GetZDcmPicCount(); i++)
	{
		m_pDcmPicArray->GetDcmArray()[i]->SetDataInMem(true);
		pDoc->AddImage(m_pDcmPicArray->GetDcmArray()[i], -1, FALSE);
	}
}

Bitmap* SeBinaryView::GreatePng(CImageBase* pImg, int nMin, int nMax, COLORREF color, DWORD alpha)
{
	if(pImg == NULL)
		return NULL;
	int nWidth = pImg->GetWidth();
	int nHeight = pImg->GetHeight();
	short* pData = (short*)pImg->GetData();
	short* pFirstValue = pData;
	int nSpace = (nMax - nMin)/256 + 1;
	pData = pFirstValue;
	PixelFormat pixelFormat = PixelFormat32bppARGB;
	Bitmap myBmp(nWidth, nHeight, pixelFormat);
	BitmapData myData;
	Rect rect(0, 0, nWidth, nHeight);
	myBmp.LockBits(&rect, ImageLockModeWrite, pixelFormat, &myData);
	int nStride = myData.Stride;
	DWORD* pMyData = (DWORD*)myData.Scan0; 
	DWORD r = GetRValue(color);
	DWORD g = GetGValue(color);
	DWORD b = GetBValue(color);

	DWORD rgba = (alpha << 24) | (r << 16) | (g << 8) | b  ;

	for(int i=0; i<nHeight; i++)
	{
		for(int j=0; j<nWidth; j++)
		{
			short tmp = *pData++;
			if (tmp > nMin && tmp < nMax)
			{
				*pMyData++ = rgba;

			}
			else
			{
				*pMyData++ = 0;
			}
		}
	}
	myBmp.UnlockBits(&myData); 

	return myBmp.Clone(0, 0, nWidth, nHeight, pixelFormat);
}

const void SeBinaryView::Reset()
{
	m_nMax = 0;
	m_nMin = 0;
	m_bNewMask = FALSE;
	CImageViewerDoc* pDoc = (CImageViewerDoc*)GetDocument();
	pDoc->ReleaseSeries();
}



#endif
#endif //_DEBUG


// SeBinaryView 消息处理程序
