// SeExampleView.cpp : 实现文件
//

#include "stdafx.h"
#include "SeExampleModule.h"
#include "ExampleModuleSwapData.h"
#include "SeExampleView.h"


// SeExampleView

IMPLEMENT_DYNCREATE(SeExampleView, CImageViewerView)

	SeExampleView::SeExampleView()
{
	m_pDcmPicArray = NULL;
	m_nFatMin = 0;
	m_nFatMax = 0;
	m_nLungMin = 0;
	m_nLungMax = 0;
	m_nBoneMin = 0;
	m_nBoneMax = 0;
	m_nNowPos = 0;
	m_nSelect = 0;
}

SeExampleView::~SeExampleView()
{
	m_pDcmPicArray = NULL;
}

BEGIN_MESSAGE_MAP(SeExampleView, CImageViewerView)
END_MESSAGE_MAP()


// SeExampleView 绘图

void SeExampleView::OnDraw(CDC* pDC)
{
	
	CDocument* pDoc = GetDocument();

	CRect			rtClient;
	GetClientRect(&rtClient);
	CZKMemDC			MemDC(pDC, rtClient, RGB(0,0,0));

	CImageViewerView::OnDraw(&MemDC);

	if (m_nSelect == 0)
	{
		return;
	}

	CImageBase* pImg = GetDisplayMgr()->GetFirstDispImage();
	Graphics gc(MemDC.GetSafeHdc());
	if (pImg != NULL)
	{
		int min = 0;
		int max = 0;
		COLORREF color = RGB(0, 0, 0);
		if (m_nSelect == 1) 
		{
			min = m_nFatMin;
			max = m_nFatMax;
			color = RGB(0, 255, 0);
		}
		else if (m_nSelect == 2)
		{
			min = m_nLungMin;
			max = m_nLungMax;
			color = RGB(0, 0, 255);
		}
		else if (m_nSelect == 3)
		{
			min = m_nLungMin;
			max = m_nLungMax;
			color = RGB(255, 0, 0);
		}
		Bitmap* pBmp = GreatePng(pImg, min, max, color , 64);
		CRect rect = pImg->GetDrawRect();
		Image* pMyImg = pBmp->GetThumbnailImage(rect.Width(), rect.Height());
		gc.DrawImage(pMyImg, rect.left, rect.top, rect.Width(), rect.Height());
		Safe_Delete(pBmp);
		Safe_Delete(pMyImg);
	}
}


// SeExampleView 诊断

#ifdef _DEBUG
void SeExampleView::AssertValid() const
{
	CImageViewerView::AssertValid();
}

#ifndef _WIN32_WCE
void SeExampleView::Dump(CDumpContext& dc) const
{
	CImageViewerView::Dump(dc);
}


#endif
#endif //_DEBUG


// SeExampleView 消息处理程序
void SeExampleView::SetDcmArray(CDcmPicArray* pDcmArray)
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

const void SeExampleView::Reset()
{
	CImageViewerDoc* pDoc = (CImageViewerDoc*)GetDocument();
	pDoc->ReleaseSeries();
}

void SeExampleView::SetInfo(int n1, int n2, DWORD n3, int n4, int n5)
{
	m_nSelect = n5;
	if (n5 == 1) 
	{
		m_nFatMin =  n1;
		m_nFatMax = n2;
	}
	else if (n5 == 2)
	{
		m_nLungMin = n1;
		m_nLungMax = n2;
	}
	else if (n5 == 3)
	{
		m_nBoneMin = n1;
		m_nBoneMax = n2;
	}
}

Bitmap* SeExampleView::GreatePng(CImageBase* pImg, int nMin, int nMax, COLORREF color, DWORD alpha)
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
