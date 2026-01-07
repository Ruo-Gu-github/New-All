// SeROIView.cpp : 实现文件
//

#include "stdafx.h"
#include "SeROIView.h"
#include "BoneDensitySwapData.h"
#include "SeBoneDensity.h"
#include <stack>

const int	dx[4] = {1, 0, 0, -1};
const int	dy[4] = {0, 1, -1, 0};

IMPLEMENT_DYNCREATE(SeROIView, CImageViewerView)

SeROIView::SeROIView()
{
	m_pDcmPicArray = NULL;
	m_pVecEdgeInside = NULL;
	m_pVecEdge = NULL;
	m_bClipOutside = TRUE;
	m_bNewMask = FALSE;
	m_bNewROIMask = FALSE;
	m_bNewROIOutMask = FALSE;
}

SeROIView::~SeROIView()
{
}

BEGIN_MESSAGE_MAP(SeROIView, CImageViewerView)

END_MESSAGE_MAP()


// SeROIView 绘图

void SeROIView::OnDraw(CDC* pDC)
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
		Bitmap* pBmp;
		if (m_bNewROIMask || m_bNewROIOutMask)
		{
			int nPlanePos = GetDisplayMgr()->GetFirstImage();
			pBmp = GreatePngWithMask(pImg, nPlanePos);
// 			if (m_bNewROIOutMask)
// 			{
// 				pBmp = GreatePngWithMaskOut(pImg, nPlanePos);
// 			}
// 			else{
// 				pBmp = GreatePngWithMask(pImg, nPlanePos);
// 			}
			
		}
		else
		{
			pBmp = GreatePng(pImg, m_nMin, m_nMax, m_color, m_alpha);
		}
		
		CRect rect = pImg->GetDrawRect();
		Image* pMyImg = pBmp->GetThumbnailImage(rect.Width(), rect.Height());
		gc.DrawImage(pMyImg, rect.left, rect.top, rect.Width(), rect.Height());
		Safe_Delete(pBmp);
		Safe_Delete(pMyImg);
	}
}


// SeROIView 诊断

#ifdef _DEBUG
void SeROIView::AssertValid() const
{
	CImageViewerView::AssertValid();
}

#ifndef _WIN32_WCE

static int shrink_table[] = {
	0, 0,  0,  0, 0,   0, 0,  0, 0,  0,  0, 0,
	0, 0,  0,  0, 0,   1, 1,  1, 1,  0,  1, 1,
	1, 1,  1,  1, 0,   0, 1,  1, 0,  0,  0, 0,
	0, 0,  0,  0, 0,   0, 0,  0, 0,  0,  0, 0,
	1, 0,  1,  1, 1,   0, 1,  1, 0,  0,  1, 1,
	0, 0,  1,  1, 0,   0, 0,  0, 0,  0,  0, 0,
	0, 0,  0,  0, 0,   0, 0,  0, 1,  0,  0, 0,
	0, 0,  0,  0, 1,   1, 1,  1, 0,  0,  1, 1,
	0, 0,  0,  0, 0,   0, 0,  0, 0,  0,  0, 0,
	0, 0,  0,  0, 0,   0, 0,  0, 0,  0,  0, 0,
	0, 0,  1,  1, 0,   0, 1,  1, 0,  0,  0, 0,
	0, 0,  0,  0, 0,   0, 0,  0, 0,  0,  0, 0,
	1, 0,  0,  0, 0,   0, 0,  0, 1,  1,  1, 1,
	0, 0,  1,  1, 0,   0, 0,  0, 0,  0,  0, 0,
	0, 0,  0,  0, 0,   0, 0,  0, 1,  0,  1, 1,
	1, 0,  1,  1, 1,   1, 0,  0, 1,  1,  0, 0,
	0, 0,  0,  0, 0,   0, 0,  0, 0,  0,  0, 0,
	0, 0,  0,  0, 1,   0, 0,  0, 0,  0,  0, 0,
	1, 1,  1,  1, 0,   0, 1,  1, 0,  0,  0, 0,
	0, 0,  0,  0, 0,   0, 0,  0, 0,  0,  0, 0,
	1, 0,  1,  1, 1,   0, 1,  1, 1,  1,  0, 0,
	1, 1,  0,  0, 0,   0, 0,  0, 0,  0,  0, 0,
	0, 0,  0,  0, 0,   0, 0,  0, 1,  0,  0, 0,
	0, 0,  0,  0, 0,   0, 0,  0, 0,  0,  0, 0,
	0, 0,  0,  0, 0,   0, 0,  0, 0,  0,  0, 0,
	0, 0,  0,  0, 1,   0, 1,  1, 1,  0,  1, 1,
	0, 0,  1,  1, 0,   0, 1,  1, 0,  0,  0, 0,
	0, 0,  0,  0, 0,   0, 0,  0, 0,  0,  0, 0,
	0, 0,  0,  0, 0,   0, 0,  0, 0,  0,  0, 0,
	0, 0,  0,  0, 0,   0, 0,  0, 0,  0,  0, 0,
	0, 0,  0,  0, 0,   0, 0,  0, 0,  0,  0, 0,
	0, 0,  0,  0, 0,   0, 1,  1, 0,  0,  1, 1,
	0, 0,  0,  0, 0,   0, 0,  0, 0,  0,  0, 0,
	0, 0,  0,  0, 1,   0, 0,  0, 0,  0,  0, 0,
	1, 1,  1,  1, 0,   0, 1,  1, 0,  0,  0, 0,
	0, 0,  0,  0, 0,   0, 0,  0, 0,  0,  0, 0,
	1, 0,  1,  1, 1,   0, 1,  1, 1,  1,  0, 0,
	1, 1,  0,  0, 0,   0, 0,  0, 0,  0,  0, 0,
	0, 0,  0,  0, 0,   0, 0,  0, 1,  0,  0, 0,
	0, 0,  0,  0, 1,   1, 1,  1, 0,  0,  1, 1,
	0, 0,  0,  0, 0,   0, 0,  0, 0,  0,  0, 0,
	0, 0,  0,  0, 1,   0, 1,  1, 1,  0,  1, 1,
	1, 1,  0,  0, 1,   1, 0,  0
};

void SeROIView::Dump(CDumpContext& dc) const
{
	CImageViewerView::Dump(dc);
}

void SeROIView::SetDcmArray(CDcmPicArray* pDcmArray)
{
	ReleaseROIPts();
	m_pDcmPicArray = pDcmArray;
	CImageViewerDoc* pDoc = (CImageViewerDoc*)GetDocument();
	pDoc->ReleaseSeries();
	for (int i = 0; i < m_pDcmPicArray->GetZDcmPicCount(); i++)
	{
		m_pDcmPicArray->GetDcmArray()[i]->SetDataInMem(true);
		pDoc->AddImage(m_pDcmPicArray->GetDcmArray()[i], -1, FALSE);
	}
	InitROIPts();
}

void SeROIView::SetDcmArrayMask(CDcmPicArray* pDcmArray)
{
	m_pDcmPicArrayMask = pDcmArray;
}

void SeROIView::SetDcmArrayOutMask(CDcmPicArray* pDcmPicArrayOutMask)
{
	m_pDcmPicArrayOutMask = pDcmPicArrayOutMask;
}

void SeROIView::InitROIPts()
{
	if (m_pVecEdgeInside == NULL) 
		m_pVecEdgeInside = new vector <CPoint> [m_pDcmPicArray->GetDcmArray().size()];
	if (m_pVecEdge == NULL)
		m_pVecEdge = new vector <CPoint> [m_pDcmPicArray->GetDcmArray().size()];
}

void SeROIView::ReleaseROIPts()
{
	if (m_pVecEdgeInside == NULL || m_pVecEdge == NULL)
		return;
	for (int i=0; i<m_pVecEdgeInside->size(); i++)
	{
		
		m_pVecEdgeInside[i].clear();
	}
	delete []m_pVecEdgeInside;
	m_pVecEdgeInside = NULL;
	m_vecPoidInside.clear();
	for (int i=0; i<m_pVecEdge->size(); i++)
	{
		m_pVecEdge[i].clear();
	}
	delete []m_pVecEdge;
	m_pVecEdge = NULL;
	m_vecPoid.clear();
}

void SeROIView::ResetROIPts()
{
	for (int i=0; i<m_pDcmPicArray->GetDcmArray().size(); i++)
	{
		m_pVecEdgeInside[i].clear();
	}
	m_vecPoidInside.clear();
	for (int i=0; i<m_pDcmPicArray->GetDcmArray().size(); i++)
	{
		m_pVecEdge[i].clear();
	}
	m_vecPoid.clear();
}

void SeROIView::FillMidLayer()
{
	CRect rtIn, rtOut;
	if (m_vecPoidInside.size() >= 2)
	{
		sort(m_vecPoidInside.begin(), m_vecPoidInside.end());
		m_vecPoidInside.erase(unique(m_vecPoidInside.begin(), m_vecPoidInside.end()), m_vecPoidInside.end());
		for(int i=0; i<m_vecPoidInside.size() -1; i++)
		{
			Interpolate(m_vecPoidInside[i], m_vecPoidInside[i + 1], m_pVecEdgeInside);
		}
		rtIn = GetROIRect(m_vecPoidInside);
	}

	if (m_vecPoid.size() >= 2)
	{
		sort(m_vecPoid.begin(), m_vecPoid.end());
		m_vecPoid.erase(unique(m_vecPoid.begin(), m_vecPoid.end()), m_vecPoid.end());
		for(int i=0; i<m_vecPoid.size() -1; i++)
		{
			Interpolate(m_vecPoid[i], m_vecPoid[i + 1], m_pVecEdge);
		}
		rtOut = GetROIRect(m_vecPoid);
	}


	m_rtROI.left = rtOut.left < rtIn.left ? rtOut.left : rtIn.left;
	m_rtROI.right = rtOut.right > rtIn.right ? rtOut.right : rtIn.right;
	m_rtROI.top = rtOut.top < rtIn.top ? rtOut.top : rtIn.top;
	m_rtROI.bottom = rtOut.bottom > rtIn.bottom ? rtOut.bottom : rtIn.bottom;

}

void SeROIView::Interpolate(int nStart, int nEnd, vector <CPoint>* pVecEdge)
{
	int nSize = nEnd - nStart;
	m_ScopeTool.InterpolatePrePare(pVecEdge[nStart], pVecEdge[nEnd], MAX_LENGTH);
	for(int i=1; i<nSize; i++)
	{
		m_ScopeTool.InterpolatePoints(pVecEdge[nStart], pVecEdge[nEnd],(float)i/nSize, pVecEdge[nStart + i]);
	}
}

CRect SeROIView::GetROIRect(vector <int> vecPoid)
{
	CRect rect(theBoneDensitySwapData.m_nWidth,theBoneDensitySwapData.m_nHeight, 0, 0);
	sort(vecPoid.begin(), vecPoid.end());
	m_vecPoid.erase(unique(vecPoid.begin(), vecPoid.end()), vecPoid.end());

	for(int i=0; i<vecPoid.size(); i++)
	{
		if(!m_pVecEdge[vecPoid[i]].empty())
		{
			int n = vecPoid[i];
			for(int j = 0; j < m_pVecEdge[vecPoid[i]].size(); j++)
			{
				rect.left = (rect.left > m_pVecEdge[n][j].x) ? m_pVecEdge[n][j].x : rect.left;
				rect.right = (rect.right < m_pVecEdge[n][j].x) ? m_pVecEdge[n][j].x : rect.right;
				rect.top = (rect.top > m_pVecEdge[n][j].y) ? m_pVecEdge[n][j].y : rect.top;
				rect.bottom = (rect.bottom < m_pVecEdge[n][j].y) ? m_pVecEdge[n][j].y : rect.bottom;
			}
		}
	}
	if(rect.left > 10) rect.left -= 10;
	if(rect.right < theBoneDensitySwapData.m_nWidth - 10) rect.right += 10;
	if(rect.top > 10) rect.top -= 10;
	if(rect.bottom < theBoneDensitySwapData.m_nHeight - 10) rect.bottom += 10;
	return rect;
}

const BOOL SeROIView::IsEmptyROI()
{
	for (int i=0; i<m_pDcmPicArray->GetDcmArray().size(); i++)
	{
		if (!m_pVecEdge[i].empty())
			return FALSE;
	}
	return TRUE;
}

Bitmap* SeROIView::GreatePng(CImageBase* pImg, int nMin, int nMax, COLORREF color, DWORD alpha)
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

Bitmap* SeROIView::GreatePngWithMask(CImageBase* pImg, int PlanePos)
{
	int n = m_pDcmPicArrayMask->GetZDcmPicCount();
	CImageBase* pDcmMask = m_pDcmPicArrayMask->GetDcmArray()[PlanePos];
	CImageBase* pDcmMaskOut = m_pDcmPicArrayOutMask->GetDcmArray()[PlanePos];
	if(pImg == NULL || pDcmMask==NULL || pDcmMaskOut==NULL)
		return NULL;

	int nWidth = pImg->GetWidth();
	int nHeight = pImg->GetHeight();
	short* pData = (short*)pImg->GetData();
	short* pFirstValue = pData;
	short* pDataMask = (short*)pDcmMask->GetData();
	short* pDataOutMask = (short*)pDcmMaskOut->GetData();
	pData = pFirstValue;
	PixelFormat pixelFormat = PixelFormat32bppARGB;
	Bitmap myBmp(nWidth, nHeight, pixelFormat);
	BitmapData myData;
	Rect rect(0, 0, nWidth, nHeight);
	myBmp.LockBits(&rect, ImageLockModeWrite, pixelFormat, &myData);
	int nStride = myData.Stride;
	DWORD* pMyData = (DWORD*)myData.Scan0; 
	COLORREF color = RGB(255, 0, 0);
	DWORD r = GetRValue(color);
	DWORD g = GetGValue(color);
	DWORD b = GetBValue(color);
	DWORD alpha = 64;
	DWORD rgba = (alpha << 24) | (r << 16) | (g << 8) | b  ;

	color = RGB(0, 255, 0);
	r = GetRValue(color);
	g = GetGValue(color);
	b = GetBValue(color);
	DWORD rgbaOut = (alpha << 24) | (r << 16) | (g << 8) | b  ;

	color = RGB(255, 255, 0);
	r = GetRValue(color);
	g = GetGValue(color);
	b = GetBValue(color);
	DWORD rgbaBoth = (alpha << 24) | (r << 16) | (g << 8) | b  ;

	for(int i=0; i<nHeight; i++)
	{
		for(int j=0; j<nWidth; j++)
		{
			short tmp = pData[i*nWidth+j];
			short tmpMask = pDataMask[i*nWidth+j];
			short tmpMaskOut = pDataOutMask[i*nWidth+j];
			if (m_bNewROIMask && !m_bNewROIOutMask)
			{
				if (tmpMask>1)
				{
					pMyData[i*nWidth+j] = rgba;
				}
				else{
					pMyData[i*nWidth+j] = 0;
				}
			}
			else if (!m_bNewROIMask && m_bNewROIOutMask)
			{
				if (tmpMaskOut>1)
				{
					pMyData[i*nWidth+j] = rgbaOut;
				}
				else{
					pMyData[i*nWidth+j] = 0;
				}
			}
			else if (m_bNewROIMask && m_bNewROIOutMask)
			{
				if (tmpMaskOut>1 && tmpMask>1)
				{
					pMyData[i*nWidth+j] = rgbaBoth;
				}
				else if (tmpMaskOut>1 && tmpMask<1)
				{
					pMyData[i*nWidth+j] = rgbaOut;
				}
				else if (tmpMaskOut<1 && tmpMask>1)
				{
					pMyData[i*nWidth+j] = rgba;
				}
				else{
					pMyData[i*nWidth+j] = 0;
				}
			}
			else{
				pMyData[i*nWidth+j] = 0;
			}
		}
	}
	myBmp.UnlockBits(&myData); 

	return myBmp.Clone(0, 0, nWidth, nHeight, pixelFormat);
}

// Bitmap* SeROIView::GreatePngWithMaskOut(CImageBase* pImg, int PlanePos)
// {
// 	int n = m_pDcmPicArrayMask->GetZDcmPicCount();
// 	CImageBase* pDcmMask = m_pDcmPicArrayMask->GetDcmArray()[PlanePos];
// 	CImageBase* pDcmMaskOut = m_pDcmPicArrayOutMask->GetDcmArray()[PlanePos];
// 
// 	if(pImg == NULL || pDcmMask==NULL|| pDcmMaskOut==NULL)
// 		return NULL;
// 
// 	int nWidth = pImg->GetWidth();
// 	int nHeight = pImg->GetHeight();
// 	short* pData = (short*)pImg->GetData();
// 	short* pFirstValue = pData;
// 	short* pDataMask = (short*)pDcmMask->GetData();
// 	short* pDataMaskOut = (short*)pDcmMaskOut->GetData();
// 	pData = pFirstValue;
// 	PixelFormat pixelFormat = PixelFormat32bppARGB;
// 	Bitmap myBmp(nWidth, nHeight, pixelFormat);
// 	BitmapData myData;
// 	Rect rect(0, 0, nWidth, nHeight);
// 	myBmp.LockBits(&rect, ImageLockModeWrite, pixelFormat, &myData);
// 	int nStride = myData.Stride;
// 	DWORD* pMyData = (DWORD*)myData.Scan0; 
// 	COLORREF color = RGB(255, 0, 0);
// 	DWORD r = GetRValue(color);
// 	DWORD g = GetGValue(color);
// 	DWORD b = GetBValue(color);
// 	DWORD alpha = 64;
// 	DWORD rgba = (alpha << 24) | (r << 16) | (g << 8) | b  ;
// 
// 	color = RGB(0, 255, 0);
// 	r = GetRValue(color);
// 	g = GetGValue(color);
// 	b = GetBValue(color);
// 	DWORD rgbaOut = (alpha << 24) | (r << 16) | (g << 8) | b  ;
// 
// 	for(int i=0; i<nHeight; i++)
// 	{
// 		for(int j=0; j<nWidth; j++)
// 		{
// 			short tmp = pData[i*nWidth+j];
// 			short tmpMask = pDataMask[i*nWidth+j];
// 			short tmpMaskOut = pDataMaskOut[i*nWidth+j];
// 			if (tmpMask>1)
// 			{
// 				pMyData[i*nWidth+j]= rgba;
// 			}
// 			else if (tmpMaskOut>1)
// 			{
// 				pMyData[i*nWidth+j] = rgbaOut;
// 			}
// 			else
// 			{
// 				pMyData[i*nWidth+j] = 0;
// 			}
// 		}
// 	}
// 	myBmp.UnlockBits(&myData); 
// 
// 	return myBmp.Clone(0, 0, nWidth, nHeight, pixelFormat);
// }

const void SeROIView::Reset()
{
	CImageViewerDoc* pDoc = (CImageViewerDoc*)GetDocument();
	pDoc->ReleaseSeries();
	ReleaseROIPts();
	SetMouseTool(MT_Flipping);
}

int SeROIView::Otsu(short *pData, int nWidth, int nHeight)
{
	int threshold = 0;
	short* pDatatemp = new short[nWidth*nHeight];
	memcpy(pDatatemp, pData, nWidth*nHeight*sizeof(short));
	//init the parameters
	int nMaxGray = 4096;
	int nMinGray = 1000;
	int nSumPix[3096];
	float nProDis[3096];
	for (int i = 0; i < (nMaxGray-nMinGray); i++)
	{
		nSumPix[i] = 0;
		nProDis[i] = 0;
	}

	//统计灰度集中每个像素在整幅图像中的个数
	int nPicSum=0;
	for (int i = 0; i < nHeight; i++)
	{
		for (int j = 0; j < nWidth; j++)
		{
			int temp = pDatatemp[i * nWidth + j];
			if (temp>=nMinGray && temp< nMaxGray)
			{
				temp = temp-nMinGray;
				nSumPix[temp]++;
				nPicSum++;
			}
			
		}
	}

	//计算每个灰度级占图像中的概率分布
	for (int i = 0; i < (nMaxGray-nMinGray); i++)
	{
		nProDis[i] = nSumPix[i]*1.0 / (nPicSum);
	}

	//遍历灰度级【0，nMaxGray】，计算出最大类间方差下的阈值

	float w0, w1, u0_temp, u1_temp, u0, u1, delta_temp;
	double delta_max = 0.0;
	for (int i = 0; i < (nMaxGray-nMinGray); i++)
	{
		//初始化相关参数
		w0 = w1 = u0 = u1 = u0_temp = u1_temp = delta_temp = 0;
		for (int j = 0; j < (nMaxGray-nMinGray); j++)
		{
			//背景部分
			if (j <= i)
			{
				w0 += nProDis[j];
				u0_temp += j * nProDis[j];
			}
			//前景部分
			else
			{
				w1 += nProDis[j];
				u1_temp += j * nProDis[j];
			}
		}
		//计算两个分类的平均灰度
		u0 = u0_temp / w0;
		u1 = u1_temp / w1;
		//依次找到最大类间方差下的阈值
		delta_temp = (float)(w0*w1*pow((u0 - u1), 2)); //前景与背景之间的方差(类间方差)
		if (delta_temp > delta_max)
		{
			delta_max = delta_temp;
			threshold = i;
		}
	}
	return threshold;
}

void SeROIView::BinaryzationAllBone(CDcmPicArray* pDcmPicArray,CDcmPicArray* pDcmPicArray_Mask)
{
	for (int i = 0; i < pDcmPicArray->GetZDcmPicCount(); i++)
	{
		CImageBase* pDcm = pDcmPicArray->GetDcmArray()[i];
		CImageBase* pDcmMask = pDcmPicArray_Mask->GetDcmArray()[i];
/*		CImageBase* pDcmROI = pDcmPicArray_ROI->GetDcmArray()[i];*/
		if(pDcm == NULL)
			return;
		int nWidth = pDcm->GetWidth();
		int nHeight = pDcm->GetHeight();
		short* pData = (short*)pDcm->GetData();
		short* pDataMask = (short*)pDcmMask->GetData();
/*		short* pDataROI = (short*)pDcmROI->GetData();*/
		//int nThred = Otsu(pData,nWidth,nHeight);
		for (int k=0;k<nWidth*nHeight;k++)
		{
			short pTemp = pData[k];
			if(pTemp>=m_nMin && pTemp<=m_nMax){
				pDataMask[k] = 4096;
/*				pDataROI[k]=0;*/
			}
			else{
				pDataMask[k] = 0;
/*				pDataROI[k] = 4096;*/
			}
		}
	}
	
}

void SeROIView::ReverseAllBone(CDcmPicArray* pDcmPicArray_Mask)
{
	for (int i = 0; i < pDcmPicArray_Mask->GetZDcmPicCount(); i++)
	{
		CImageBase* pDcm = pDcmPicArray_Mask->GetDcmArray()[i];
		if(pDcm == NULL)
			return;
		int nWidth = pDcm->GetWidth();
		int nHeight = pDcm->GetHeight();
		short* pData = (short*)pDcm->GetData();
		for (int k=0;k<nWidth*nHeight;k++)
		{
			short pTemp = pData[k];
			if(pTemp==4096){
				pData[k] = 0;
			}
			else{
				pData[k] = 4096;
			}
		}
	}
}

void SeROIView::FloodfillAllBone(CDcmPicArray* pDcmPicArray_Mask, CDcmPicArray* pDcmPicArray_MaskOut)
{
	for (int k = 0;k<pDcmPicArray_Mask->GetZDcmPicCount(); k++)
	{
		CImageBase* pDcm = pDcmPicArray_Mask->GetDcmArray()[k];
		if(pDcm == NULL)
			return;
		int nWidth = pDcm->GetWidth();
		int nHeight = pDcm->GetHeight();
		short* pData = (short*)pDcm->GetData();

		CImageBase* pDcmOut = pDcmPicArray_MaskOut->GetDcmArray()[k];
		short* pDataOut = (short*)pDcmOut->GetData();

		floodFillScanline(pData, 0, 0, 2, 4096,nWidth,nHeight);

		for(int i=0;i<nHeight;i++)
		{
			for (int j=0;j<nWidth;j++)
			{
				int temp = pData[i*nWidth+j];
				if (temp == 2)
				{
					//外面被fill的地方
					pData[i*nWidth+j] = 0;
					pDataOut[i*nWidth+j] = 0;
				}
				else{
					pData[i*nWidth+j]=4096-pDataOut[i*nWidth+j];
					pDataOut[i*nWidth+j] = 4096;
				}

			}
		}

		//
// 
		int nMark = 0;
		for (int i=0;i<nHeight;i++)
		{
			for(int j=0;j<nWidth;j++)
			{
				int temp = pDataOut[i*nWidth+j];
				if (temp == 4096)
				{
					nMark++;
					floodFillScanline(pDataOut, i, j, nMark, 4096,nWidth,nHeight);
				}
			}
		}

		if (nMark<2)
		{
			continue;
		}

		int* pSum = new int[nMark+1];
		memset(pSum,0,sizeof(int)*(nMark+1));	

		for (int i=0;i<nHeight;i++)
		{
			for(int j=0;j<nWidth;j++)
			{
				int temp = pDataOut[i*nWidth+j];
				if (temp>nMark)
				{
					int m =1;
					return;
				}
				pSum[temp] = pSum[temp]+1;
			}
		}

		int nMax = 0;
		int nMaxPos = 0;
		for(int i=1;i<=nMark;i++)
		{
			if(pSum[i]>nMax){nMax = pSum[i]; nMaxPos = i;}
		}

		for(int i=0;i<nHeight;i++)
		{
			for (int j=0;j<nWidth;j++)
			{
				int tempOut = pDataOut[i*nWidth+j];
				if (tempOut == nMaxPos)
				{
					pDataOut[i*nWidth+j] = 4096;
				}
				else
				{
					pData[i*nWidth+j]=0;
					pDataOut[i*nWidth+j]=0;
				}
			}
		}
		delete []pSum;
	}
}

void SeROIView::FillHoleAllBone(CDcmPicArray* pDcmPicArray)
{
	for (int k = 0;k<pDcmPicArray->GetZDcmPicCount(); k++)
	{
		CImageBase* pDcm = pDcmPicArray->GetDcmArray()[k];
		if(pDcm == NULL)
			return;
		int nWidth = pDcm->GetWidth();
		int nHeight = pDcm->GetHeight();
		short* pData = (short*)pDcm->GetData();

		floodFillScanline(pData, 0, 0, 2, 4096,nWidth,nHeight);

// 		short* pDataOut = new short[nHeight*nWidth];
// 		memset(pDataOut, 0 , sizeof(short)*nHeight*nWidth);

		for(int i=0;i<nHeight;i++)
		{
			for (int j=0;j<nWidth;j++)
			{
				int temp = pData[i*nWidth+j];
				if (temp == 2)
				{
					//外面被fill的地方
					pData[i*nWidth+j] = 0;
				}
				else{
					pData[i*nWidth+j] = 4096;
				}

			}
		}

		//
		// 
		int nMark = 0;
		for (int i=0;i<nHeight;i++)
		{
			for(int j=0;j<nWidth;j++)
			{
				int temp = pData[i*nWidth+j];
				if (temp == 4096)
				{
					nMark++;
					floodFillScanline(pData, i, j, nMark, 4096,nWidth,nHeight);
				}
			}
		}

		if (nMark<2)
		{
			for(int i=0;i<nHeight;i++)
			{
				for (int j=0;j<nWidth;j++)
				{
					int temp = pData[i*nWidth+j];
					if (temp > 0)
					{
						pData[i*nWidth+j] = 4096;
					}
					else
					{
						pData[i*nWidth+j]=0;
					}
				}
			}
			continue;
		}

		int* pSum = new int[nMark+1];
		memset(pSum,0,sizeof(int)*(nMark+1));	

		for (int i=0;i<nHeight;i++)
		{
			for(int j=0;j<nWidth;j++)
			{
				int temp = pData[i*nWidth+j];
				if (temp>nMark)
				{
					int m =1;
					return;
				}
				pSum[temp] = pSum[temp]+1;
			}
		}

		int nMax = 0;
		int nMaxPos = 0;
		for(int i=1;i<=nMark;i++)
		{
			if(pSum[i]>nMax){nMax = pSum[i]; nMaxPos = i;}
		}

		for(int i=0;i<nHeight;i++)
		{
			for (int j=0;j<nWidth;j++)
			{
				int temp = pData[i*nWidth+j];
				if (temp == nMaxPos)
				{
					pData[i*nWidth+j] = 4096;
				}
				else
				{
					pData[i*nWidth+j]=0;
				}
			}
		}
		if (pSum != NULL)
		{
			delete []pSum;
		}
	}
}

void SeROIView::GetHoleAllBone(CDcmPicArray* pDcmPicArray)
{
	for (int k = 0;k<pDcmPicArray->GetZDcmPicCount(); k++)
	{
		CImageBase* pDcm = pDcmPicArray->GetDcmArray()[k];
		if(pDcm == NULL)
			return;
		int nWidth = pDcm->GetWidth();
		int nHeight = pDcm->GetHeight();
		short* pData = (short*)pDcm->GetData();

		short* pDataOut = new short[nHeight*nWidth];
		memcpy(pDataOut, pData , sizeof(short)*nHeight*nWidth);

		floodFillScanline(pDataOut, 0, 0, 2, 4096,nWidth,nHeight);

		for(int i=0;i<nHeight;i++)
		{
			for (int j=0;j<nWidth;j++)
			{
				int temp = pDataOut[i*nWidth+j];
				if (temp == 2)
				{
					//外面被fill的地方
					pData[i*nWidth+j] = 0;
					pDataOut[i*nWidth+j] = 0;
				}
				else{
					//pData[i*nWidth+j]=4096-pDataOut[i*nWidth+j];
					pDataOut[i*nWidth+j] = 4096;
				}

			}
		}

		//
		// 
		int nMark = 0;
		for (int i=0;i<nHeight;i++)
		{
			for(int j=0;j<nWidth;j++)
			{
				int temp = pDataOut[i*nWidth+j];
				if (temp == 4096)
				{
					nMark++;
					floodFillScanline(pDataOut, i, j, nMark, 4096,nWidth,nHeight);
				}
			}
		}

		if (nMark<2)
		{
			if (pDataOut != NULL)
			{
				delete []pDataOut;
			}
			continue;
		}

		int* pSum = new int[nMark+1];
		memset(pSum,0,sizeof(int)*(nMark+1));	

		for (int i=0;i<nHeight;i++)
		{
			for(int j=0;j<nWidth;j++)
			{
				int temp = pDataOut[i*nWidth+j];
				if (temp>nMark)
				{
					int m = 1;
					return;
				}
				pSum[temp] = pSum[temp]+1;
			}
		}

		int nMax = 0;
		int nMaxPos = 0;
		for(int i=1;i<=nMark;i++)
		{
			if(pSum[i]>nMax){nMax = pSum[i]; nMaxPos = i;}
		}

		for(int i=0;i<nHeight;i++)
		{
			for (int j=0;j<nWidth;j++)
			{
				int tempOut = pDataOut[i*nWidth+j];
				if (tempOut != nMaxPos)
				{
					pData[i*nWidth+j]=0;
				}
			}
		}
		if (pSum != NULL)
		{
			delete []pSum;
		}
		if (pDataOut != NULL)
		{
			delete []pDataOut;
		}
	}
}

void SeROIView::OnlyGetMaxArea(CDcmPicArray* pDcmPicArray_Mask,CDcmPicArray* pDcmPicArray_MaskOut)
{//只保留最大联通区域
	for (int k = 0;k<pDcmPicArray_Mask->GetZDcmPicCount(); k++)
	{
		CImageBase* pDcm = pDcmPicArray_Mask->GetDcmArray()[k];
		if(pDcm == NULL)
			return;
		int nWidth = pDcm->GetWidth();
		int nHeight = pDcm->GetHeight();
		short* pData = (short*)pDcm->GetData();

		CImageBase* pDcmOut = pDcmPicArray_MaskOut->GetDcmArray()[k];
		short* pDataOut = (short*)pDcmOut->GetData();

		int nMark = 0;
		for (int i=0;i<nHeight;i++)
		{
			for(int j=0;j<nWidth;j++)
			{
				int temp = pDataOut[i*nWidth+j];
				if (temp == 4096)
				{
					nMark++;
					floodFillScanline(pDataOut, i, j, nMark, 4096,nWidth,nHeight);
				}
			}
		}

		if (nMark<2)
		{
			return;
		}

		int* pSum = new int[nMark+1];
		memset(pSum,0,sizeof(int)*nMark);	

		for (int i=0;i<nHeight;i++)
		{
			for(int j=0;j<nWidth;j++)
			{
				int temp = pDataOut[i*nWidth+j];
				if (temp>nMark)
				{
					int m =1;
					return;
				}
				pSum[temp] = pSum[temp]+1;
			}
		}

		int nMax = 0;
		int nMaxPos = 0;
		for(int i=1;i<=nMark;i++)
		{
			if(pSum[i]>nMax){nMax = pSum[i]; nMaxPos = i;}
		}

		for(int i=0;i<nHeight;i++)
		{
			for (int j=0;j<nWidth;j++)
			{
				int tempOut = pDataOut[i*nWidth+j];
				if (tempOut == 0)
				{
					pData[i*nWidth+j]=0;
					pDataOut[i*nWidth+j]=0;
				}
				else if (tempOut == nMaxPos)
				{
					pDataOut[i*nWidth+j] = 4096;
				}
				else
				{
					pData[i*nWidth+j]=0;
					pDataOut[i*nWidth+j]=0;
				}
			}
		}
		delete pSum;
	}
}

void SeROIView::floodFillScanline(short* pData, int x, int y, int newColor, int oldColor, int nWidth, int nHeight)
{
	if(newColor==oldColor) return;
	if(pData[x*nWidth+y]!=oldColor) return;
	int y1=y;
	while(y1<nWidth&&pData[x*nWidth+y1]==oldColor){
		pData[x*nWidth+y1]=newColor;
		y1++;
	}
	y1=y-1;
	while(y1>=0&&pData[x*nWidth+y1]==oldColor){
		pData[x*nWidth+y1]=newColor;
		y1--;
	}
	y1=y;
	while(y1<nWidth&&pData[x*nWidth+y1]==newColor){
		if(x<nHeight-1&&pData[(x+1)*nWidth+y1]==oldColor) floodFillScanline(pData, x+1,y1,newColor,oldColor,nWidth,nHeight);
		y1++;
	}
	y1=y-1;
	while(y1>0&&pData[x*nWidth+y1]==newColor){
		if(x>0&&pData[(x-1)*nWidth+y1]==oldColor) floodFillScanline(pData,x-1,y1,newColor,oldColor,nWidth,nHeight);
		y1--;
	}

	y1=y;
	while(y1<nWidth&&pData[x*nWidth+y1]==newColor){
		if(x>0&&pData[(x-1)*nWidth+y1]==oldColor) floodFillScanline(pData,x-1,y1,newColor,oldColor,nWidth,nHeight);
		y1++;
	}
	y1=y-1;
	while(y1>0&&pData[x*nWidth+y1]==newColor){
		if(x<nHeight-1&&pData[(x+1)*nWidth+y1]==oldColor) floodFillScanline(pData, x+1,y1,newColor,oldColor,nWidth,nHeight);
		y1--;
	}

}

void SeROIView::BoneSegCorrosion(CDcmPicArray* pDcmPicArray_Mask, int nType, int nKernelSize)
{
	for (int k = 0; k < pDcmPicArray_Mask->GetZDcmPicCount(); k++)
	{
		CImageBase* pDcm = pDcmPicArray_Mask->GetDcmArray()[k];
		if(pDcm == NULL)
			return;
		int nWidth = pDcm->GetWidth();
		int nHeight = pDcm->GetHeight();
		short* pData = (short*)pDcm->GetData();

		Corrosion(pData,nWidth,nHeight,nKernelSize,nType,1);
	}
}

void SeROIView::BoneSegInflation(CDcmPicArray* pDcmPicArray_Mask, int nType, int nKernelSize)
{
	for (int k = 0; k < pDcmPicArray_Mask->GetZDcmPicCount(); k++)
	{
		CImageBase* pDcm = pDcmPicArray_Mask->GetDcmArray()[k];
		if(pDcm == NULL)
			return;
		int nWidth = pDcm->GetWidth();
		int nHeight = pDcm->GetHeight();
		short* pData = (short*)pDcm->GetData();

		Inflation(pData,nWidth,nHeight,nKernelSize,nType,1);
	}
}

void SeROIView::BongSegOpen(CDcmPicArray* pDcmPicArray_Mask, int nType, int nKernelSize)
{//消除杂点
	for (int k = 0; k < pDcmPicArray_Mask->GetZDcmPicCount(); k++)
	{
		CImageBase* pDcm = pDcmPicArray_Mask->GetDcmArray()[k];
		if(pDcm == NULL)
			return;
		int nWidth = pDcm->GetWidth();
		int nHeight = pDcm->GetHeight();
		short* pData = (short*)pDcm->GetData();

		Corrosion(pData,nWidth,nHeight,nKernelSize,nType,1);
		Inflation(pData,nWidth,nHeight,nKernelSize,nType,1);

	}
}

void SeROIView::BongSegClose(CDcmPicArray* pDcmPicArray_Mask, int nType, int nKernelSize)
{
	for (int k = 0; k < pDcmPicArray_Mask->GetZDcmPicCount(); k++)
	{
		CImageBase* pDcm = pDcmPicArray_Mask->GetDcmArray()[k];
		if(pDcm == NULL)
			return;
		int nWidth = pDcm->GetWidth();
		int nHeight = pDcm->GetHeight();
		short* pData = (short*)pDcm->GetData();

		Inflation(pData,nWidth,nHeight,nKernelSize,nType,1);
		Corrosion(pData,nWidth,nHeight,nKernelSize,nType,1);

	}
}

void SeROIView::BoneSegDicomPicArrayClone(CDcmPicArray* pDcmPicArray_In,CDcmPicArray* pDcmPicArray_Out)
{
	for (int k = 0; k < pDcmPicArray_In->GetZDcmPicCount(); k++)
	{
		CImageBase* pDcm = pDcmPicArray_In->GetDcmArray()[k];
		CImageBase* pDcmOut = pDcmPicArray_Out->GetDcmArray()[k];
		if(pDcm == NULL)
			return;
		int nWidth = pDcm->GetWidth();
		int nHeight = pDcm->GetHeight();
		short* pData = (short*)pDcm->GetData();
		short* pDataOut = (short*)pDcmOut->GetData();

		for (int i = 0; i<(nWidth*nHeight);i++)
		{
			pDataOut[i] = pData[i];
		}
	}
}

void SeROIView::BoneGetMaskOuter(CDcmPicArray* pDcmPicArray_Mask, CDcmPicArray* pDcmPicArray_Out)
{
	for (int k = 0; k < pDcmPicArray_Mask->GetZDcmPicCount(); k++)
	{
		CImageBase* pDcm = pDcmPicArray_Mask->GetDcmArray()[k];
		CImageBase* pDcmOut = pDcmPicArray_Out->GetDcmArray()[k];
		if(pDcm == NULL || pDcm == NULL)
			return;
		int nWidth = pDcm->GetWidth();
		int nHeight = pDcm->GetHeight();
		short* pData = (short*)pDcm->GetData();
		short* pDataOut = (short*)pDcmOut->GetData();

		for (int i=0;i<nWidth*nHeight;i++)
		{
			if (pData[i])
			{
				pDataOut[i] = 0;
			}
		}
	}
}


void SeROIView::ShowTrabecular(CDcmPicArray* pDcmPicArray_Ori,CDcmPicArray* pDcmPicArray_Out,CDcmPicArray* pDcmPicArray_Mask)
{
	int nMinValue = theBoneDensitySwapData.m_nMinValue;

	for (int k = 0; k < pDcmPicArray_Mask->GetZDcmPicCount(); k++)
	{
		CImageBase* pDcmMask = pDcmPicArray_Mask->GetDcmArray()[k];
		if(pDcmMask == NULL)
			return;
		int nWidth = pDcmMask->GetWidth();
		int nHeight = pDcmMask->GetHeight();
		short* pDataMask = (short*)pDcmMask->GetData();

		CImageBase* pDcmOri = pDcmPicArray_Ori->GetDcmArray()[k];
		short* pDataOri = (short*)pDcmOri->GetData();

		CImageBase* pDcmOut = pDcmPicArray_Out->GetDcmArray()[k];
		short* pDataOut = (short*)pDcmOut->GetData();

		for (int i=0;i<(nWidth*nHeight);i++)
		{
			if (pDataMask[i]==0)
			{
				pDataOut[i] = nMinValue;
			}
			else{
				if (pDataOri[i] == nMinValue)
				{
					pDataOut[i] = nMinValue+1;
				}
				else{
					pDataOut[i] = pDataOri[i];
				}
			}
		}
	}
}


void SeROIView::MaxAreaAllBone(CDcmPicArray* pDcmPicArray_Mask, CDcmPicArray* pDcmPicArray_ROI)
{
	for (int k = 0; k < pDcmPicArray_Mask->GetZDcmPicCount(); k++)
	{
		CImageBase* pDcm = pDcmPicArray_Mask->GetDcmArray()[k];
		if(pDcm == NULL)
			return;
		int nWidth = pDcm->GetWidth();
		int nHeight = pDcm->GetHeight();
		short* pData = (short*)pDcm->GetData();


		CImageBase* pDcmROI = pDcmPicArray_ROI->GetDcmArray()[k];
		short* pDataROI = (short*)pDcmROI->GetData();

// 		int xs=2;
// 		int xe=nWidth-2;
// 		int ys=2;
// 		int ye=nHeight-2;
// 		short* pDataCopy = pData;
// 		for (int i=2;i<nHeight;i++)
// 		{
// 			int nsum = 0;
// 			for (int j=0;j<nWidth;j++)
// 			{
// 				nsum = nsum+pDataCopy[i*nWidth+j];
// 			}
// 			if (nsum<nWidth*4096)
// 			{
// 				ys = i-1;
// 				break;
// 			}
// 		}
// 		for (int i=(nHeight-2);i>=0;i--)
// 		{
// 			int nsum = 0;
// 			for (int j=0;j<nWidth;j++)
// 			{
// 				nsum = nsum+pDataCopy[i*nWidth+j];
// 			}
// 			if (nsum<nWidth*4096)
// 			{
// 				ye = i+1;
// 				break;
// 			}
// 		}
// 		for (int i=2;i<nWidth;i++)
// 		{
// 			int nsum = 0;
// 			for (int j=0;j<nHeight;j++)
// 			{
// 				nsum = nsum+pDataCopy[j*nWidth+i]/4096;
// 			}
// 			if (nsum<nHeight)
// 			{
// 				xs = i-1;
// 				break;
// 			}
// 		}
// 		for (int i=(nWidth-2);i>=0;i--)
// 		{
// 			int nsum = 0;
// 			for (int j=0;j<nHeight;j++)
// 			{
// 				nsum = nsum+pDataCopy[j*nWidth+i]/4096;
// 			}
// 			if (nsum<nHeight)
// 			{
// 				xe = i+1;
// 				break;
// 			}
// 		}


		int* pMaska = new int[nWidth*nHeight];
		int* pMaskb = new int[nWidth*nHeight];
		for (int i=0;i<nHeight;i++)
		{	for (int j=0;j<nWidth;j++)
			{
				pMaska[i*nWidth+j] = 0;
				if (i<3 || j<3 || i>(nWidth-3) || j>(nHeight-3))
				{
					pMaskb[i*nWidth+j] = 1;
				}
				else{
					pMaskb[i*nWidth+j] = 0;
				}
			}
		}

		while (!IsSame(pMaska,pMaskb,(nWidth*nHeight)))
		{
			MaskCopy(pMaskb,pMaska,nWidth,nHeight);
			Inflation(pMaskb,nWidth,nHeight,3,nTypeSquare,1);
			BitwithAnd(pMaskb,pData,pMaskb,nWidth,nHeight);
		}
		ReverseMask(pMaskb,nWidth,nHeight);
		BitwithAnd(pMaskb,pDataROI,pMaskb,nWidth,nHeight);
		Mask2Data(pMaskb,pData,nWidth,nHeight);
		
		delete pMaska;
		delete pMaskb;
	}
}

void SeROIView::BoneOpenF(CDcmPicArray* pDcmPicArray_Mask, CString str, int nType)
{
	int kernelsize = _ttoi(str);
	for (int k = 0; k < pDcmPicArray_Mask->GetZDcmPicCount(); k++)
	{
		CImageBase* pDcm = pDcmPicArray_Mask->GetDcmArray()[k];
		if(pDcm == NULL)
			return;
		int nWidth = pDcm->GetWidth();
		int nHeight = pDcm->GetHeight();
		short* pData = (short*)pDcm->GetData();
		Inflation(pData, nWidth, nHeight, kernelsize, nType, 1);
	}
}

void SeROIView::BoneOpenE(CDcmPicArray* pDcmPicArray_Mask, CString str, int nType)
{

	int kernelsize = _ttoi(str);
	for (int k = 0; k < pDcmPicArray_Mask->GetZDcmPicCount(); k++)
	{
		CImageBase* pDcm = pDcmPicArray_Mask->GetDcmArray()[k];
		if(pDcm == NULL)
			return;
		int nWidth = pDcm->GetWidth();
		int nHeight = pDcm->GetHeight();
		short* pData = (short*)pDcm->GetData();
		Corrosion(pData, nWidth, nHeight, kernelsize,nType, 1);
	}
}


bool SeROIView::IsSame(int* pData1,int* pData2,int nlength){
	for (int i =0;i<nlength;i++)
	{
		if (pData1[i]!=pData2[i])
		{
			return FALSE;
		}
	}
	return TRUE;
}

int SeROIView::CalAddPoints(int* pData1,int* pData2,int nlength)
{
	int nsum = 0;
	for (int i=0;i<nlength;i++)
	{
		nsum = nsum+pData2[i]-pData1[i];
	}
	return nsum;
}

void SeROIView::Corrosion(int *pData, int nWidth, int nHeight, int kernelsize, int nType, int nCore)
{
	if (nCore == 0)
		return;
	if (kernelsize<3)
		return;
	if (kernelsize%2==0)
		kernelsize--;
	int halfkernel = (kernelsize-1)/2;
	int nValue = 1;
	if(nHeight <= 0 || nWidth <= 0)
		return;
	int* temp = new int[nWidth*nHeight];
	memcpy(temp, pData, nWidth*nHeight*sizeof(int));

	switch(nType){
	case 0:
		for (int i=halfkernel; i<nWidth-halfkernel; i++)
		{
			for(int j=halfkernel;j<nHeight-halfkernel;j++)
			{
				if (temp[i+j*nWidth] >= nValue)
				{
					bool bsumkernel = 1;
					for (int ii=(i-halfkernel);ii<=(i+halfkernel);ii++)
					{
						for (int jj=(j-halfkernel);jj<=(j+halfkernel);jj++)
						{
							if (temp[ii+jj*nWidth] < nValue)
							{
								bsumkernel=0;
							}
						}
					}
					if (bsumkernel)
					{
						pData[i+j*nWidth] = nValue;
					}
					else
					{
						pData[i+j*nWidth]=0;
					}

				}
				else
				{
					pData[i+j*nWidth]=0;
				}
			}	
		}
		break;
	case 1:
		//cricle
		for (int i=halfkernel; i<nWidth-halfkernel; i++)
		{
			for(int j=halfkernel;j<nHeight-halfkernel;j++)
			{
				if (temp[i+j*nWidth] >= nValue)
				{
					bool bsumkernel = 1;
					for (int ii=(i-halfkernel);ii<=(i+halfkernel);ii++)
					{
						for (int jj=(j-halfkernel);jj<=(j+halfkernel);jj++)
						{
							if (DistancePoints(ii,jj,i,j)<=halfkernel)
							{
								if (temp[ii+jj*nWidth] < nValue)
								{
									bsumkernel=0;
								}
							}
							
						}
					}
					if (bsumkernel)
					{
						pData[i+j*nWidth] = nValue;
					}
					else
					{
						pData[i+j*nWidth]=0;
					}
				}
				else
				{
					pData[i+j*nWidth]=0;
				}
			}	
		}
		break;
	default:
		return;
	}
	Safe_Delete(temp);
	nCore = nCore - 1; 
	Corrosion(pData, nWidth, nHeight, kernelsize, nType, nCore);
}

void SeROIView::Corrosion(short *pData, int nWidth, int nHeight, int kernelsize, int nType, int nCore)
{
	if (nCore == 0)
		return;
	if (kernelsize<3)
		return;
	if (kernelsize%2==0)
		kernelsize--;
	int halfkernel = (kernelsize-1)/2;
	short nValue = 4096;
	if(nHeight <= 0 || nWidth <= 0)
		return;
	short* temp = new short[nWidth*nHeight];
	memcpy(temp, pData, nWidth*nHeight*sizeof(short));

	switch(nType){
	case 0:
		//square
		for (int i=halfkernel; i<nWidth-halfkernel; i++)
		{
			for(int j=halfkernel;j<nHeight-halfkernel;j++)
			{
				if (temp[i+j*nWidth] >= nValue)
				{
					bool bsumkernel = 1;
					for (int ii=(i-halfkernel);ii<=(i+halfkernel);ii++)
					{
						for (int jj=(j-halfkernel);jj<=(j+halfkernel);jj++)
						{
							if (temp[ii+jj*nWidth] < nValue)
							{
								bsumkernel=0;
							}
						}
					}
					if (bsumkernel)
					{
						pData[i+j*nWidth] = nValue;
					}
					else
					{
						pData[i+j*nWidth]=0;
					}

				}
				else
				{
					pData[i+j*nWidth]=0;
				}
			}	
		}
		break;
	case 1:
		//cricle
		for (int i=halfkernel; i<nWidth-halfkernel; i++)
		{
			for(int j=halfkernel;j<nHeight-halfkernel;j++)
			{
				if (temp[i+j*nWidth] >= nValue)
				{
					bool bsumkernel = 1;
					for (int ii=(i-halfkernel);ii<=(i+halfkernel);ii++)
					{
						for (int jj=(j-halfkernel);jj<=(j+halfkernel);jj++)
						{
							if (DistancePoints(ii,jj,i,j)<=halfkernel)
							{
								if (temp[ii+jj*nWidth] < nValue)
								{
									bsumkernel=0;
								}
							}

						}
					}
					if (bsumkernel)
					{
						pData[i+j*nWidth] = nValue;
					}
					else
					{
						pData[i+j*nWidth]=0;
					}
				}
				else
				{
					pData[i+j*nWidth]=0;
				}
			}	
		}
		break;
	default:
		return;
	}
	Safe_Delete(temp);
	nCore = nCore - 1; 
	Corrosion(pData, nWidth, nHeight, kernelsize, nType, nCore);
}

// void SeROIView::Corrosion(short *pData, int nWidth, int nHeight, int kernelsize, int nType, int nCore)
// {
// 	if (nCore == 0)
// 		return;
// 	if (kernelsize<3)
// 		return;
// 	if (kernelsize%2==0)
// 		kernelsize--;
// 	int halfkernel = (kernelsize-1)/2;
// 	int nValue = 4096;
// 	if(nHeight <= 0 || nWidth <= 0)
// 		return;
// 	short* temp = new short[nWidth*nHeight];
// 	memcpy(temp, pData, nWidth*nHeight*sizeof(short));
// 
// 	switch(nType){
// 	case 0:
// 		for (int i=halfkernel; i<nWidth-halfkernel; i++)
// 		{
// 			for(int j=halfkernel;j<nHeight-halfkernel;j++)
// 			{
// 				int nsumkernel1 = 0;
// 				int nsumkernel2 = 0;
// 				for (int ii=(i-halfkernel);ii<=(i+halfkernel);ii++)
// 				{
// 					for (int jj=(j-halfkernel);jj<=(j+halfkernel);jj++)
// 					{
// 						nsumkernel1=nsumkernel1+temp[ii+jj*nWidth];
// 						nsumkernel2=nsumkernel2+1;
// 					}
// 				}
// 				if (nsumkernel1==(nsumkernel2*nValue))
// 				{
// 					pData[i+j*nWidth] = nValue;
// 				}
// 				else
// 				{
// 					pData[i+j*nWidth]=0;
// 				}
// 			}	
// 		}
// 		break;
// 	case 1:
// 		//cricle
// 		for (int i=halfkernel; i<nWidth-halfkernel; i++)
// 		{
// 			for(int j=halfkernel;j<nHeight-halfkernel;j++)
// 			{
// 				int nsumkernel1 = 0;
// 				int nsumkernel2 = 0;
// 				for (int ii=(i-halfkernel);ii<=(i+halfkernel);ii++)
// 				{
// 					for (int jj=(j-halfkernel);jj<=(j+halfkernel);jj++)
// 					{
// 						if (DistancePoints(ii,jj,i,j)<=halfkernel)
// 						{
// 							nsumkernel1=nsumkernel1+temp[ii+jj*nWidth];
// 							nsumkernel2=nsumkernel2+1;
// 						}
// 
// 					}
// 				}
// 				if (nsumkernel1==(nsumkernel2*nValue))
// 				{
// 					pData[i+j*nWidth] = nValue;
// 				}
// 				else
// 				{
// 					pData[i+j*nWidth]=0;
// 				}
// 			}	
// 		}
// 		break;
// 	default:
// 		return;
// 	}
// 	
// 	Safe_Delete(temp);
// 	nCore = nCore - 1; 
// 	Corrosion(pData, nWidth, nHeight, kernelsize,nType, nCore);
// }

void SeROIView::Inflation(short *pData, int nWidth, int nHeight, int kernelsize, int nType, int nCore)
{
	if (nCore == 0)
		return;
	if (kernelsize<3)
		return;
	if (kernelsize%2==0)
		kernelsize--;
	int halfkernel = (kernelsize-1)/2;
	int nValue = 4096;
	if(nHeight <= 0 || nWidth <= 0)
		return ;
	short* temp = new short[nWidth*nHeight];
	memcpy(temp, pData , nWidth*nHeight*sizeof(short));

	switch(nType){
	case 0://square
		for (int i = halfkernel ; i < nWidth - halfkernel; i++)
		{
			for (int j = halfkernel ; j<nHeight - halfkernel; j++)
			{
				if(temp[i+j*nWidth] >= nValue)
				{
					for (int ii=(i-halfkernel);ii<=(i+halfkernel);ii++)
					{
						for (int jj=(j-halfkernel);jj<=(j+halfkernel);jj++)
						{
							pData[ii+(jj)*nWidth] = nValue;
						}
					}
				}
			}
		}
		break;
	case 1://cricle
		for (int i = halfkernel ; i < nWidth - halfkernel; i++)
		{
			for (int j = halfkernel ; j<nHeight - halfkernel; j++)
			{
				if(temp[i+j*nWidth] >= nValue)
				{
					for (int ii=(i-halfkernel);ii<=(i+halfkernel);ii++)
					{
						for (int jj=(j-halfkernel);jj<=(j+halfkernel);jj++)
						{
							if (DistancePoints(ii,jj,i,j)<=halfkernel)
							{
								pData[ii+(jj)*nWidth] = nValue;
							}
						}
					}
				}
			}
		}
		break;
	default:
		return;
	}
	
	Safe_Delete(temp);
	nCore = nCore - 1; 
	Inflation(pData, nWidth, nHeight, kernelsize,nType, nCore);
}

void SeROIView::Inflation(int *pData, int nWidth, int nHeight, int kernelsize, int nType, int nCore)
{
	if (nCore == 0)
		return;
	if (kernelsize<3)
		return;
	if (kernelsize%2==0)
		kernelsize--;
	int halfkernel = (kernelsize-1)/2;
	int nValue = 1;
	if(nHeight <= 0 || nWidth <= 0)
		return ;
	int* temp = new int[nWidth*nHeight];
	memcpy(temp, pData , nWidth*nHeight*sizeof(int));

	switch(nType){
	case 0://square
		for (int i = halfkernel ; i < nWidth - halfkernel; i++)
		{
			for (int j = halfkernel ; j<nHeight - halfkernel; j++)
			{
				if(temp[i+j*nWidth] >= nValue)
				{
					for (int ii=(i-halfkernel);ii<=(i+halfkernel);ii++)
					{
						for (int jj=(j-halfkernel);jj<=(j+halfkernel);jj++)
						{
							pData[ii+(jj)*nWidth] = nValue;
						}
					}
				}
			}
		}
		break;
	case 1://cricle
		for (int i = halfkernel ; i < nWidth - halfkernel; i++)
		{
			for (int j = halfkernel ; j<nHeight - halfkernel; j++)
			{
				if(temp[i+j*nWidth] >= nValue)
				{
					for (int ii=(i-halfkernel);ii<=(i+halfkernel);ii++)
					{
						for (int jj=(j-halfkernel);jj<=(j+halfkernel);jj++)
						{
							if (DistancePoints(ii,jj,i,j)<=halfkernel)
							{
								pData[ii+(jj)*nWidth] = nValue;
							}
						}
					}
				}
			}
		}
		break;
	default:
		return;
	}
	Safe_Delete(temp);
	nCore = nCore - 1; 
	Inflation(pData, nWidth, nHeight, kernelsize, nType, nCore);
}

void SeROIView::BitwithAnd(int *pDataMask, short *pData, int*pDataOut, int nWidth, int nHeight)
{
	for (int i=0;i<(nWidth*nHeight);i++)
	{
		pDataOut[i] = pDataMask[i] && pData[i];
	}
	return;
}

void SeROIView::BitwithAnd(int *pDataMask, short *pData, short *pDataOut,int nWidth, int nHeight)
{
	for (int i=0;i<(nWidth*nHeight);i++)
	{
		pDataOut[i] = pDataMask[i] * pData[i];
	}
	return;
}

void SeROIView::ReverseMask(int *pDataMask, int nWidth, int nHeight)
{
	for (int i=0;i<(nWidth*nHeight);i++)
	{
		pDataMask[i] = 1-pDataMask[i];
	}
}


void SeROIView::Mask2Data(int *pDataMask, short *pData, int nWidth, int nHeight)
{
	for (int i=0;i<(nWidth*nHeight);i++)
	{
		pData[i] = pDataMask[i]*4096;
	}
}

void SeROIView::Data2Mask(int *pDataMask, short *pData, int nWidth, int nHeight)
{
	for (int i=0;i<(nWidth*nHeight);i++)
	{
		if (pData[i]>1)
		{
			pDataMask[i] = 1;
		}
		else
		{
			pDataMask[i] = 0;
		}
	}

}

void SeROIView::MaskCopy(int *pMask1,int *pMask1Copy, int nWidth, int nHeight)
{
	for (int i=0;i<(nWidth*nHeight);i++)
	{
		pMask1Copy[i]=pMask1[i];
	}
}

double SeROIView::DistancePoints(int i1, int j1,int i2, int j2)
{
	return sqrt(pow((i1-i2)*1.0,2)+pow((j1-j2)*1.0,2));
}

void SeROIView::shrink(int *pDataMask, int n, int nWidth, int nHeight)
{
	if (!pDataMask)return;

	int iter = 0;
	int done = 0;

	int* m = new int[nWidth*nHeight];
	int* sub = new int[nWidth*nHeight];	
	int* last_img = new int[nWidth*nHeight];
	memset(m, 0 , nWidth*nHeight*sizeof(int));	
	memset(sub, 0 , nWidth*nHeight*sizeof(int));
	memset(last_img, 0 , nWidth*nHeight*sizeof(int));

	while (!done)
	{
		// 一次shrink操作，4次查找表处理
		MaskCopy(pDataMask, last_img, nWidth, nHeight);

		shrink_look_up(pDataMask, m, nWidth, nHeight);
		shrink_mask_and(pDataMask, m, sub, nWidth, nHeight);
		shrink_copy_quarter_data(sub, pDataMask, 0, 0, nWidth, nHeight);

		shrink_look_up(pDataMask, m, nWidth, nHeight);
		shrink_mask_and(pDataMask, m, sub, nWidth, nHeight);
		shrink_copy_quarter_data(sub, pDataMask, 1, 1, nWidth, nHeight);

		shrink_look_up(pDataMask, m, nWidth, nHeight);
		shrink_mask_and(pDataMask, m, sub, nWidth, nHeight);
		shrink_copy_quarter_data(sub, pDataMask, 0, 1, nWidth, nHeight);

		shrink_look_up(pDataMask, m, nWidth, nHeight);
		shrink_mask_and(pDataMask, m, sub, nWidth, nHeight);
		shrink_copy_quarter_data(sub, pDataMask, 1, 0, nWidth, nHeight);

		done = 1;

// 		//达到设定次数或者当前二值图与上一次shrink操作结果相同，结束shrink操作
// 		if (iter++ >= n || IsSame(last_img, pDataMask, (nWidth*nHeight)))
// 		{
// 			done = 1;
// 		}
	}
}

void SeROIView::shrink_look_up(int *img,int *res,int nWidth, int nHeight)
{
	if (!img || !res)return;
	int rows = nHeight;
	int cols = nWidth;
	int i, j;
	int index;

	rows--;
	cols--;
	assert(rows > 1 && cols > 1);

	for (i = 1; i < rows; i++)
	{
		for (j = 1; j < cols; j++)
		{
// 			index = (arr[i - 1][j - 1] << 8) + (arr[i][j - 1] << 7) +\
// 				(arr[i + 1][j - 1] << 6) + (arr[i - 1][j] << 5) + \
// 				(arr[i][j] << 4) + (arr[i + 1][j] << 3) + \
// 				(arr[i - 1][j + 1] << 2) + (arr[i][j + 1] << 1) + \
// 				arr[i + 1][j + 1];
			int mmm = 1;
			int mm = mmm << 8;
			index = (img[(i-1)*cols+(j-1)] << 8) 
				+ (img[(i)*cols+(j-1)] << 7) 
				+ (img[(i+1)*cols+(j-1)] << 6) 
				+ (img[(i-1)*cols+(j)] << 5) 
				+ (img[(i)*cols+(j)] << 4) 
				+ (img[(i+1)*cols+(j)] << 3) 
				+ (img[(i-1)*cols+(j+1)]<< 2) 
				+ (img[(i)*cols+(j+1)] << 1) 
				+ (img[(i+1)*cols+(j+1)]);

			assert(index < 512);
			res[(i)*cols+(j)] = shrink_table[index];
		}
	}

}

void SeROIView::shrink_mask_and(int *img, int *m, int *sub, int nWidth, int nHeight)
{
	for (int i=0;i<(nWidth*nHeight);i++)
	{
		sub[i] = img[i] && m[i];
	}
}

void SeROIView::shrink_copy_quarter_data(int *src, int *dst, int rs, int cs, int nWidth, int nHeight)
{
	if (!src || !dst)return;
	int rows = nHeight;
	int cols = nWidth;
	int i, j;

	for (i = rs; i < rows; i+=2)
	{
		for (j = cs; j < cols; j+=2)
		{
			dst[i * cols + j] = src[i * cols + j];
		}
	}
}

void SeROIView::OnExportImage()
{
	CImageViewerView::OnImageviewerExport();
}

#endif
#endif //_DEBUG
// SeROIView 消息处理程序







