// SeMPRView.cpp : 实现文件


#include "stdafx.h"
#include <Windows.h>
#include <algorithm>
#include <memory>
#include <limits>
#include <vector>

#include "GeneralSwapData.h"
#include "SeGeneralModule.h"
#include "SeMPRView.h"
#include "SeROIData.h"
#include "Se3DView.h"
#include "Measurement.h"

#include "boost/thread/thread.hpp"

struct ProjectionWorkerArgs
{
	short*                     pDest;
	SIZE_T                     startIndex;
	SIZE_T                     endIndex;
	const short* const*        sliceArray;
	SIZE_T                     sliceCount;
	bool                       useMip;
};

UINT AFX_CDECL SeMPRView::ProjectionWorkerProc(LPVOID lpParam)
{
	ProjectionWorkerArgs* pArgs = static_cast<ProjectionWorkerArgs*>(lpParam);
	if (pArgs == NULL)
	{
		return 0;
	}
	short* pDest = pArgs->pDest;
	const short* const* sliceArray = pArgs->sliceArray;
	SIZE_T startIndex = pArgs->startIndex;
	SIZE_T endIndex = pArgs->endIndex;
	SIZE_T sliceCount = pArgs->sliceCount;
	bool useMip = pArgs->useMip;
	const short kMinValue = std::numeric_limits<short>::min();
	const short kMaxValue = std::numeric_limits<short>::max();
	if (pDest == NULL || sliceArray == NULL || sliceCount == 0 || endIndex <= startIndex)
	{
		delete pArgs;
		return 0;
	}
	for (SIZE_T idx = startIndex; idx < endIndex; ++idx)
	{
		short bestValue = useMip ? kMinValue : kMaxValue;
		bool hasSample = false;
		for (SIZE_T slice = 0; slice < sliceCount; ++slice)
		{
			const short* pSrc = sliceArray[slice];
			if (pSrc == NULL)
			{
				continue;
			}
			short sample = pSrc[idx];
			hasSample = true;
			if (useMip)
			{
				if (sample > bestValue)
				{
					bestValue = sample;
				}
			}
			else
			{
				if (sample < bestValue)
				{
					bestValue = sample;
				}
			}
		}
		pDest[idx] = hasSample ? bestValue : 0;
	}
	delete pArgs;
	return 0;
}


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

int				SeMPRView::m_nXPos = 0;
int				SeMPRView::m_nYPos = 0;
int				SeMPRView::m_nZPos = 0;
int             SeMPRView::m_nMin = 0;
int             SeMPRView::m_nMax = 65535;
COLORREF        SeMPRView::m_color = RGB(0, 0, 0);
DWORD           SeMPRView::m_alpha = 0;
int				SeMPRView::m_nOriImageWidth = 0;
int				SeMPRView::m_nOriImageHeight = 0;
int				SeMPRView::m_nOriImagePiece = 0;
int             SeMPRView::m_nRoiShape = ROI_SHAPE_ANY;
float           SeMPRView::m_fSharp = 0.0f;
vector<CMeasurement*> SeMPRView::m_vecRst;
BOOL            SeMPRView::m_bShowMask = TRUE;
int             SeMPRView::m_nMipThickness = 1;
int             SeMPRView::m_nMinIpThickness = 1;
int             SeMPRView::m_eProjectionMode = SeMPRView::PROJECTION_NONE;

int SeMPRView::GetRoiShape()
{
	return m_nRoiShape;
}

void SeMPRView::SetRoiShape(int nShape)
{
	if (nShape < ROI_SHAPE_ANY || nShape > ROI_SHAPE_ROUND)
	{
		m_nRoiShape = ROI_SHAPE_ANY;
	}
	else
	{
		m_nRoiShape = nShape;
	}
}

void SeMPRView::DrawLine(vector <CPoint> vecPts, Color color, int nWidth, Graphics* gc, CDcmPic* pDcm)
{
	int nSize = (int)vecPts.size();
	Pen		MarkPen(color,nWidth);
	Point*	pMarkpt = new Point[nSize];
	for (int i = 0 ; i < nSize ; i++)
	{
		CPoint ptMark = pDcm->Image2Screen(&vecPts[i]);
		pMarkpt[i].X = ptMark.x;
		pMarkpt[i].Y = ptMark.y;
	}
	gc->DrawLines(&MarkPen, pMarkpt, nSize);
	gc->DrawLine(&MarkPen, pMarkpt[0], pMarkpt[nSize - 1]);
	delete []pMarkpt;
}

void SeMPRView::DrawRegion(vector <CPoint> vecPts, SolidBrush brush, Graphics* gc, CDcmPic* pDcm)
{
	GraphicsPath path;
	int nSize = (int)vecPts.size();
	if(nSize > 0)
	{
		PointF* pPtF = new PointF[nSize];
		for (UINT i=0;i<nSize;i++)
		{
			CPoint pt = pDcm->Image2Screen(&vecPts[i]);
			pPtF[i].X = pt.x;
			pPtF[i].Y = pt.y;
		}
		path.AddLines(pPtF, nSize);
		path.CloseFigure();		
		delete[] pPtF;
	}
	Region region(&path);
	gc->FillRegion(&brush, &region);
}

CWnd*                 SeMPRView::m_wndResult = NULL;
CDcmPicArray*   SeMPRView::m_pDcmArray = NULL;
vector <CSeROIData*> SeMPRView::m_vecROIData;


//vector <MaskInfo> SeMPRView::m_vecMaskInfo;

CEvent				g_eventDrawdcm(FALSE, TRUE);
CEvent				g_eventReleasedcm(FALSE, TRUE);
//SeMPRView

IMPLEMENT_DYNCREATE(SeMPRView, CImageViewerView)

	SeMPRView::SeMPRView()
{
	m_sMinValue = -1000;
	m_bNewMask = FALSE;

	m_hDrawDcm = g_eventDrawdcm;
	m_hReleaseDcm = g_eventReleasedcm;
	m_pVecEdge = NULL;
	m_nPiece = 0;
	m_fSharp = 0.0f;
	m_bMbtnDown = FALSE;
	m_pColorImage = NULL;
}

SeMPRView::~SeMPRView()
{
	if (m_pVecEdge != NULL)
	{
		for (int i=0; i<m_nVecEageSize; i++)
		{
			m_pVecEdge[i].clear();
		}
		delete [] m_pVecEdge;
	}

}

BEGIN_MESSAGE_MAP(SeMPRView, CImageViewerView)
	ON_WM_VSCROLL()
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
END_MESSAGE_MAP()


//SeMPRView 绘图

void SeMPRView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();

	CRect			rtClient;
	GetClientRect(&rtClient);
	CZKMemDC			MemDC(pDC, rtClient, RGB(0,0,0));

	CImageViewerView::OnDraw(&MemDC);

	CImageBase* pImg = GetDisplayMgr()->GetCurrentImage();
	
	Graphics gc(MemDC.GetSafeHdc());

	

// DrawText
	FontFamily fontFamily(L"幼圆");
	Gdiplus::Font font(&fontFamily, 16, FontStyleRegular, UnitPixel);
	LOGFONT lf;
	::GetObject((HFONT)GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
	memcpy(lf.lfFaceName, "Arial", 5) ;
	lf.lfHeight = 25 ;
	SolidBrush brush(Color(255, 255, 255, 255));

	Pen pen(Color(255, 255, 255, 255), 1.0f);
	CStringW str = L"";
	switch(m_nPlaneNum)
	{
	case 1:
		str.Format(L"%d", m_nZPos);
		break;
	case 2:
		str.Format(L"%d", m_nXPos);
		break;
	case 3:
		str.Format(L"%d", m_nYPos);
		break;
	default:
		break;
	}
	
	gc.DrawString(str, -1, &font, PointF(5.0f, 5.0f), &brush);

	if (!m_bShowMask) return;
	if (pImg != NULL && m_bNewMask)
	{
		Bitmap* pBmp = GreatePng(pImg, m_nMin, m_nMax, m_color, m_alpha);
		// 		CRect rect = pImg->GetDrawRect();
		// 		Image* pMyImg = pBmp->GetThumbnailImage(rect.Width(), rect.Height());
		// 		gc.DrawImage(pMyImg, rect.left, rect.top, rect.Width(), rect.Height());
		// 		Safe_Delete(pBmp);
		// 		Safe_Delete(pMyImg);
		if (pBmp != NULL)
		{
			CRect rect = pImg->GetDrawRect();
			Image* pMyImg = pBmp->GetThumbnailImage(rect.Width(), rect.Height());
			gc.DrawImage(pMyImg, rect.left, rect.top, rect.Width(), rect.Height());
			Safe_Delete(pMyImg);
		}
		Safe_Delete(pBmp);
	}
	for (int i=0; i<m_vecROIData.size(); i++)
	{
		if (m_vecROIData[i]->IsVisible())
		{
			int pos = m_nCurrentFrame;
			if (m_nPlaneNum == 1)
				pos = m_nCurrentFrame * (theGeneralSwapData.m_dbXYSliceSpace / theGeneralSwapData.m_dbZSliceSpace);
			Bitmap* pBmp = CreatePng(m_nPlaneNum, pos, m_vecROIData[i], pImg->GetWidth(), pImg->GetHeight());
			if (pBmp != NULL)
			{
				CRect rect = pImg->GetDrawRect();
				Image* pMyImg = pBmp->GetThumbnailImage(rect.Width(), rect.Height());
				gc.DrawImage(pMyImg, rect.left, rect.top, rect.Width(), rect.Height());
				Safe_Delete(pMyImg);
			}
			Safe_Delete(pBmp);
		}
	}

}


//SeMPRView 诊断

#ifdef _DEBUG
void SeMPRView::AssertValid() const
{
	CImageViewerView::AssertValid();
}

#ifndef _WIN32_WCE
void SeMPRView::Dump(CDumpContext& dc) const
{
	CImageViewerView::Dump(dc);
}

//void SeMPRView::SetDcmArray(CDcmPicArray* pDcmArray)
//{
//	m_pDcmArray = pDcmArray;
//	Reset();
//	m_nPiece = pDcmArray->GetZDcmPicCount();
//	m_nOriImagePiece = pDcmArray->GetZDcmPicCount();
//	m_nOriImageWidth = pDcmArray->GetXDcmPicCount();
//	m_nOriImageHeight = pDcmArray->GetYDcmPicCount();
//	int nMax = m_nOriImagePiece > m_nOriImageWidth ? m_nOriImagePiece : m_nOriImageWidth;
//	nMax = nMax > m_nOriImageHeight ? nMax : m_nOriImageHeight;
//	m_nVecEageSize = nMax;
//	m_pVecEdge = new vector<CPoint>[m_nVecEageSize];
//	m_SeVisualMPR.SetDcmArray(pDcmArray);	
//	m_fSharp = 0.0f;
//}

void SeMPRView::SetDcmArray(CDcmPicArray* pDcmArray)
{
	m_pDcmArray = pDcmArray;
	Reset();

	// 修改这里：根据平面类型设置正确的层数
	m_nOriImagePiece = pDcmArray->GetZDcmPicCount();
	m_nOriImageWidth = pDcmArray->GetXDcmPicCount();
	m_nOriImageHeight = pDcmArray->GetYDcmPicCount();

	// 设置当前视图的层数
	switch(m_nPlaneNum)
	{
	case 1: // XOY平面
		m_nPiece = pDcmArray->GetZAxialPixelNumber(); // 使用Z轴实际像素数
		break;
	case 2: // YOZ平面
		m_nPiece = m_nOriImageWidth;
		break;
	case 3: // XOZ平面
		m_nPiece = m_nOriImageHeight;
		break;
	default:
		m_nPiece = m_nOriImagePiece;
		break;
	}

	int nMax = m_nOriImagePiece > m_nOriImageWidth ? m_nOriImagePiece : m_nOriImageWidth;
	nMax = nMax > m_nOriImageHeight ? nMax : m_nOriImageHeight;
	m_nVecEageSize = nMax;
	m_pVecEdge = new vector<CPoint>[m_nVecEageSize];
	m_SeVisualMPR.SetDcmArray(pDcmArray);	
	m_fSharp = 0.0f;

	// 更新滚动条
	m_nPicCount = m_nPiece;
	ResetVScrollBar();
}

void SeMPRView::UpdateImage()
{
	CDcmPic* pDcm = NULL;
	if (IsProjectionActive())
	{
		pDcm = CreateProjectionImage(m_nCurrentFrame);
		if (pDcm == NULL)
		{
			pDcm = m_SeVisualMPR.GetMPRImage();
		}
	}
	else
	{
		pDcm = m_SeVisualMPR.GetMPRImage();
	}
	if (pDcm == NULL)
		return;
	DrawImage(pDcm);
}

void SeMPRView::UpdateImage(int nPos)
{
	CDcmPic* pDcm = NULL;
	if (IsProjectionActive())
	{
		pDcm = CreateProjectionImage(nPos);
		if (pDcm == NULL)
		{
			int normalized = NormalizeSliceIndex(nPos);
			int targetPos = (normalized >= 0) ? normalized : nPos;
			m_nCurrentFrame = targetPos;
			pDcm = m_SeVisualMPR.GetMPRImage(targetPos);
		}
	}
	else
	{
		int normalized = NormalizeSliceIndex(nPos);
		int targetPos = (normalized >= 0) ? normalized : nPos;
		m_nCurrentFrame = targetPos;
		pDcm = m_SeVisualMPR.GetMPRImage(targetPos);
	}
	if (pDcm == NULL)
		return;
	DrawImage(pDcm);
}

bool SeMPRView::IsProjectionActive() const
{
	if (m_nPlaneNum != 1)
		return false;
	switch (m_eProjectionMode)
	{
	case PROJECTION_MIP:
		return m_nMipThickness > 1;
	case PROJECTION_MINIP:
		return m_nMinIpThickness > 1;
	default:
		return false;
	}
}

int SeMPRView::NormalizeSliceIndex(int index) const
{
	if (m_pDcmArray == NULL)
		return -1;
	const std::vector<CDcmPic*>& images = m_pDcmArray->GetDcmArray();
	if (images.empty())
		return -1;
	if (index < 0)
		return 0;
	//if (index >= static_cast<int>(images.size()))
	//	return static_cast<int>(images.size()) - 1;
	return index;
}

CDcmPic* SeMPRView::CreateProjectionImage(int requestedIndex)
{
	if (!IsProjectionActive())
	{
		int targetIndex = requestedIndex;
		int normalized = NormalizeSliceIndex(targetIndex);
		if (normalized >= 0)
		{
			targetIndex = normalized;
		}
		m_nCurrentFrame = targetIndex;
		return m_SeVisualMPR.GetMPRImage(targetIndex);
	}
	if (m_pDcmArray == NULL)
	{
		return NULL;
	}
	int targetIndex = requestedIndex;
	int normalized = NormalizeSliceIndex(targetIndex);
	if (normalized >= 0)
	{
		targetIndex = normalized;
	}
	if (targetIndex < 0)
	{
		return NULL;
	}
	const std::vector<CDcmPic*>& images = m_pDcmArray->GetDcmArray();
	if (targetIndex >= static_cast<int>(images.size()))
	{
		return NULL;
	}
	CDcmPic* pTemplate = images[targetIndex];
	if (pTemplate == NULL)
	{
		return NULL;
	}
	const int width = pTemplate->GetWidth();
	const int height = pTemplate->GetHeight();
	const int pixelCount = width * height;
	if (pixelCount <= 0)
	{
		return NULL;
	}
	const int sliceCountTotal = m_pDcmArray->GetZDcmPicCount();
	int thickness = (m_eProjectionMode == PROJECTION_MIP) ? m_nMipThickness : m_nMinIpThickness;
	if (thickness <= 1)
	{
		m_nCurrentFrame = targetIndex;
		return m_SeVisualMPR.GetMPRImage(targetIndex);
	}
	bool useMip = (m_eProjectionMode == PROJECTION_MIP);
	int startSlice = 0;
	int endSlice = 0;
	ClampProjectionRange(targetIndex, thickness, sliceCountTotal, startSlice, endSlice);
	if (endSlice < startSlice)
	{
		m_nCurrentFrame = targetIndex;
		return m_SeVisualMPR.GetMPRImage(targetIndex);
	}
	const int slicesToProcess = endSlice - startSlice + 1;
	if (slicesToProcess <= 0)
	{
		m_nCurrentFrame = targetIndex;
		return m_SeVisualMPR.GetMPRImage(targetIndex);
	}
	std::vector<short*> slicePointers;
	slicePointers.reserve(slicesToProcess);
	for (int slice = startSlice; slice <= endSlice; ++slice)
	{
		CDcmPic* pSlice = m_pDcmArray->GetDcmArray()[slice];
		if (pSlice == NULL)
		{
			slicePointers.push_back(NULL);
			continue;
		}
		short* src = reinterpret_cast<short*>(pSlice->GetData());
		if (src == NULL)
		{
			slicePointers.push_back(NULL);
		}
		else
		{
			slicePointers.push_back(src);
		}
	}
	if (slicePointers.empty())
	{
		return NULL;
	}
	short* pResultBuffer = new short[pixelCount];
	const short* const* sliceArray = slicePointers.empty() ? NULL : &slicePointers[0];
	const SIZE_T sliceArrayCount = slicePointers.size();
	const int kMaxThreads = 20;
	int threadCount = kMaxThreads;
	if (pixelCount < threadCount)
	{
		threadCount = (pixelCount <= 0) ? 1 : pixelCount;
	}
	if (threadCount < 1)
	{
		threadCount = 1;
	}
	const SIZE_T totalPixels = static_cast<SIZE_T>(pixelCount);
	const SIZE_T chunkSize = (totalPixels + threadCount - 1) / threadCount;
	std::vector<CWinThread*> workers;
	workers.reserve(threadCount);
	for (int i = 0; i < threadCount; ++i)
	{
		SIZE_T startIndex = static_cast<SIZE_T>(i) * chunkSize;
		if (startIndex >= totalPixels)
			break;
		SIZE_T endIndex = startIndex + chunkSize;
		if (endIndex > totalPixels)
		{
			endIndex = totalPixels;
		}
		ProjectionWorkerArgs* args = new ProjectionWorkerArgs;
		args->pDest = pResultBuffer;
		args->startIndex = startIndex;
		args->endIndex = endIndex;
		args->sliceArray = sliceArray;
		args->sliceCount = sliceArrayCount;
		args->useMip = useMip;
		CWinThread* pThread = AfxBeginThread(ProjectionWorkerProc, args, THREAD_PRIORITY_NORMAL, 0, 0, NULL);
		if (pThread == NULL)
		{
			ProjectionWorkerProc(args);
			continue;
		}
		pThread->m_bAutoDelete = FALSE;
		workers.push_back(pThread);
	}
	for (size_t i = 0; i < workers.size(); ++i)
	{
		WaitForSingleObject(workers[i]->m_hThread, INFINITE);
		delete workers[i];
	}
	CDcmPic* pResult = pTemplate->CloneDcmPic();
	if (pResult == NULL)
	{
		delete [] pResultBuffer;
		return NULL;
	}
	pResult->SetPixelData(reinterpret_cast<BYTE*>(pResultBuffer), width, height);
	int winCenter = pTemplate->GetWinCenter();
	int winWidth = pTemplate->GetWinWidth();
	pResult->AdjustWin(winCenter, winWidth);
	m_nCurrentFrame = targetIndex;
	return pResult;
}

void SeMPRView::ClampProjectionRange(int center, int thickness, int limit, int& start, int& end)
{
	if (limit <= 0)
	{
		start = end = 0;
		return;
	}
	thickness = thickness < 1 ? 1 : thickness;
	if (thickness > limit)
		thickness = limit;
	if (center < 0)
		center = 0;
	else if (center >= limit)
		center = limit - 1;
	start = center - thickness / 2;
	end = start + thickness - 1;
	if (start < 0)
	{
		end += -start;
		start = 0;
	}
	if (end >= limit)
	{
		int diff = end - (limit - 1);
		start -= diff;
		if (start < 0)
			start = 0;
		end = limit - 1;
	}
}

void SeMPRView::ApplyProjection(CDcmPic* pDcm)
{
	if (pDcm == NULL || m_pDcmArray == NULL)
		return;
	if (!IsProjectionActive())
		return;
	int thickness = (m_eProjectionMode == PROJECTION_MIP) ? m_nMipThickness : m_nMinIpThickness;
	bool useMip = (m_eProjectionMode == PROJECTION_MIP);
	ApplyProjectionAlongZ(pDcm, thickness, useMip);
}

void SeMPRView::ApplyProjectionAlongZ(CDcmPic* pDcm, int thickness, bool useMip)
{
	const int width = pDcm->GetWidth();
	const int height = pDcm->GetHeight();
	const int pixelCount = width * height;
	if (pixelCount <= 0)
		return;
	const int sliceCount = m_pDcmArray->GetZDcmPicCount();
	if (sliceCount <= 0)
		return;
	std::vector<short> buffer(pixelCount);
	const short initValue = useMip ? std::numeric_limits<short>::lowest() : std::numeric_limits<short>::max();
	std::fill(buffer.begin(), buffer.end(), initValue);
	int start = 0;
	int end = 0;
	ClampProjectionRange(m_nCurrentFrame, thickness, sliceCount, start, end);
	for (int slice = start; slice <= end && slice < sliceCount; ++slice)
	{
		CDcmPic* pSlice = m_pDcmArray->GetDcmArray()[slice];
		if (pSlice == NULL)
			continue;
		pSlice->ReloadBuffer();
		short* src = reinterpret_cast<short*>(pSlice->GetData());
		if (src == NULL)
			continue;
		for (int idx = 0; idx < pixelCount; ++idx)
		{
			if (useMip)
			{
				buffer[idx] = std::max(buffer[idx], src[idx]);
			}
			else
			{
				buffer[idx] = std::min(buffer[idx], src[idx]);
			}
		}
		pSlice->SetDataInMem(false);
		pSlice->ReleaseBuffer();
	}
	short* dest = reinterpret_cast<short*>(pDcm->GetData());
	if (dest != NULL)
	{
		memcpy(dest, &buffer[0], sizeof(short) * buffer.size());
	}
}

void SeMPRView::ApplyProjectionAlongX(CDcmPic* pDcm, int thickness, bool useMip)
{
	const int dimY = pDcm->GetWidth();
	const int dimZ = pDcm->GetHeight();
	if (dimY <= 0 || dimZ <= 0)
		return;
	const int dimX = m_pDcmArray->GetXDcmPicCount();
	if (dimX <= 0)
		return;
	std::vector<short> buffer(dimY * dimZ);
	const short initValue = useMip ? std::numeric_limits<short>::lowest() : std::numeric_limits<short>::max();
	std::fill(buffer.begin(), buffer.end(), initValue);
	std::vector<short> column(dimY);
	int start = 0;
	int end = 0;
	ClampProjectionRange(m_nCurrentFrame, thickness, dimX, start, end);
	for (int slice = 0; slice < dimZ; ++slice)
	{
		CDcmPic* pSlice = m_pDcmArray->GetDcmArray()[slice];
		if (pSlice == NULL)
			continue;
		pSlice->ReloadBuffer();
		for (int x = start; x <= end; ++x)
		{
			pSlice->GetColumnData(&column[0], x);
			for (int y = 0; y < dimY; ++y)
			{
				const int idx = slice * dimY + y;
				if (useMip)
				{
					buffer[idx] = std::max(buffer[idx], column[y]);
				}
				else
				{
					buffer[idx] = std::min(buffer[idx], column[y]);
				}
			}
		}
			pSlice->SetDataInMem(false);
			pSlice->ReleaseBuffer();
	}
	short* dest = reinterpret_cast<short*>(pDcm->GetData());
	if (dest != NULL)
	{
		memcpy(dest, &buffer[0], sizeof(short) * buffer.size());
	}
}

void SeMPRView::ApplyProjectionAlongY(CDcmPic* pDcm, int thickness, bool useMip)
{
	const int dimX = pDcm->GetWidth();
	const int dimZ = pDcm->GetHeight();
	if (dimX <= 0 || dimZ <= 0)
		return;
	const int dimY = m_pDcmArray->GetYDcmPicCount();
	if (dimY <= 0)
		return;
	std::vector<short> buffer(dimX * dimZ);
	const short initValue = useMip ? std::numeric_limits<short>::lowest() : std::numeric_limits<short>::max();
	std::fill(buffer.begin(), buffer.end(), initValue);
	std::vector<short> row(dimX);
	int start = 0;
	int end = 0;
	ClampProjectionRange(m_nCurrentFrame, thickness, dimY, start, end);
	for (int slice = 0; slice < dimZ; ++slice)
	{
		CDcmPic* pSlice = m_pDcmArray->GetDcmArray()[slice];
		if (pSlice == NULL)
			continue;
		pSlice->ReloadBuffer();
		for (int y = start; y <= end; ++y)
		{
			pSlice->GetRowData(&row[0], y);
			for (int x = 0; x < dimX; ++x)
			{
				const int idx = slice * dimX + x;
				if (useMip)
				{
					buffer[idx] = std::max(buffer[idx], row[x]);
				}
				else
				{
					buffer[idx] = std::min(buffer[idx], row[x]);
				}
			}
		}
		pSlice->SetDataInMem(false);
		pSlice->ReleaseBuffer();
	}
	short* dest = reinterpret_cast<short*>(pDcm->GetData());
	if (dest != NULL)
	{
		memcpy(dest, &buffer[0], sizeof(short) * buffer.size());
	}
}

void SeMPRView::SetMipThickness(int thickness)
{
	m_nMipThickness = thickness < 1 ? 1 : thickness;
	if (m_nMipThickness > 1)
	{
		m_eProjectionMode = PROJECTION_MIP;
	}
	else if (m_nMinIpThickness > 1)
	{
		m_eProjectionMode = PROJECTION_MINIP;
	}
	else
	{
		m_eProjectionMode = PROJECTION_NONE;
	}
}

void SeMPRView::SetMinIpThickness(int thickness)
{
	m_nMinIpThickness = thickness < 1 ? 1 : thickness;
	if (m_nMinIpThickness > 1)
	{
		m_eProjectionMode = PROJECTION_MINIP;
	}
	else if (m_nMipThickness > 1)
	{
		m_eProjectionMode = PROJECTION_MIP;
	}
	else
	{
		m_eProjectionMode = PROJECTION_NONE;
	}
}

int SeMPRView::GetMipThickness()
{
	return m_nMipThickness;
}

int SeMPRView::GetMinIpThickness()
{
	return m_nMinIpThickness;
}

void SeMPRView::RefreshAllViews()
{
	if (theGeneralSwapData.m_pXOYView != NULL)
	{
		theGeneralSwapData.m_pXOYView->UpdateImage();
		theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	}
}


void SeMPRView::DrawImage(CDcmPic* pDcm)
{
	SeVisualLibDoc* pDoc = (SeVisualLibDoc*)GetDocument();
	WaitForSingleObject(m_hReleaseDcm, INFINITE);  //与主线程同步，防止绘制dcm尚未完成时，releaseseries图像导致读dcm内存报错（不能read），图像较大时报错频率高
	g_eventDrawdcm.ResetEvent();
	pDoc->ReleaseSeries();
	pDoc->AddImage(pDcm, -1);
	//pDcm->SetSharpenDepth(m_fSharp);
	Sharp(pDcm, m_fSharp);
	pDcm->SetDrawEdgeRectFlag(false);
	g_eventDrawdcm.SetEvent();
}

UINT SeMPRView::__UpdateImage(void* lpVoid)
{
	SeMPRView*  pThis = (SeMPRView*)lpVoid;

	HANDLE		hds[2];
	hds[0] = pThis->m_eventChange;
	hds[1] = pThis->m_eventExit;

	bool		bDone = false;

	while (!bDone)
	{
		DWORD dwRet = WaitForMultipleObjects(2, hds, false, INFINITE);
		switch (dwRet)
		{
		case WAIT_OBJECT_0:
			{
				pThis->UpdateImage();
			}
			break;
		case WAIT_OBJECT_0 + 1:
			bDone = true;
			break;
		}
	}
	return 0;
}

#endif
#endif //_DEBUG


//SeMPRView 消息处理程序


void SeMPRView::OnInitialUpdate()
{
	CImageViewerView::OnInitialUpdate();
	SetLayoutFormat(1,1);
	AfxBeginThread(__UpdateImage, this);
	g_eventDrawdcm.SetEvent();
	g_eventReleasedcm.SetEvent();
}

void SeMPRView::SetPlaneNum(int nPlaneNum)
{
	m_SeVisualMPR.SetPlaneNum(nPlaneNum);
	m_nPlaneNum = nPlaneNum;
}

int SeMPRView::GetPlaneNum()
{
	return m_nPlaneNum;
}

void SeMPRView::SetMPRTool()
{
	SetMouseTool(MT_MPR);
}


void SeMPRView::SetViewPos(CPoint pt)
{
	switch(m_nPlaneNum)
	{
	case 1:
		{
			m_nXPos = pt.x;
			m_nYPos = pt.y;
			break;
		}
	case 3:
		{
			m_nXPos = pt.x;
			m_nZPos = pt.y;
			break;
		}
	case 2:
		{
			m_nYPos = pt.x;
			m_nZPos = pt.y;
		}
	}

	theGeneralSwapData.m_pXOYView->UpdateImage(m_nZPos);
	theGeneralSwapData.m_pYOZView->UpdateImage(m_nYPos);
	theGeneralSwapData.m_pXOZView->UpdateImage(m_nXPos);

	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);

	theGeneralSwapData.m_pXOYView->UpdateWindow();
	theGeneralSwapData.m_pXOZView->UpdateWindow();
	theGeneralSwapData.m_pYOZView->UpdateWindow();	
}

//
//void SeMPRView::ResetVScrollBar()
//{
//	if (m_nOriImagePiece == 0)
//	{
//		EnableScrollBarCtrl(SB_VERT,FALSE);
//		return;
//	}
//
//	SCROLLINFO	si;
//	si.cbSize = sizeof(SCROLLINFO);
//	si.fMask  = SIF_ALL;
//	si.nMin   = 0;
//	si.nMax   = m_nPicCount - 1;
//	si.nPage  = 3;
//	si.nPos	  = m_nCurrentFrame;
//
//	SetScrollInfo(SB_VERT,&si);
//	EnableScrollBarCtrl(SB_VERT, TRUE);
//}

void SeMPRView::ResetVScrollBar()
{
	if (m_nOriImagePiece == 0)
	{
		EnableScrollBarCtrl(SB_VERT,FALSE);
		return;
	}

	SCROLLINFO	si;
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask  = SIF_ALL;
	si.nMin   = 0;

	// 修改这里：根据不同平面使用不同的最大值
	switch(m_nPlaneNum)
	{
	case 1: // XOY平面，使用实际的Z轴像素数
		si.nMax = m_SeVisualMPR.GetMPRPiece() - 1;
		break;
	case 2: // YOZ平面，使用X方向的像素数
		si.nMax = m_SeVisualMPR.GetMPRPiece() - 1;
		break;
	case 3: // XOZ平面，使用Y方向的像素数
		si.nMax = m_SeVisualMPR.GetMPRPiece() - 1;
		break;
	default:
		si.nMax = m_nPicCount - 1;
		break;
	}

	si.nPage  = 3;
	si.nPos	  = m_nCurrentFrame;

	SetScrollInfo(SB_VERT,&si);
	EnableScrollBarCtrl(SB_VERT, TRUE);
}


void SeMPRView::SetWinLevel(int nWinCenter, int nWinWidth)
{
	CDcmPic* pDcm = (CDcmPic *)GetDisplayMgr()->GetFirstDispImage();
	if (pDcm != NULL)
	{
		// pDcm->AdjustWin(nWinCenter, nWinWidth);
		m_SeVisualMPR.SetWinLevel(nWinCenter, nWinWidth);
		UpdateImage();
	}

	
}

void SeMPRView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	SCROLLINFO		si;
	si.cbSize = sizeof(si);
	si.fMask  = SIF_ALL;
	VERIFY(GetScrollInfo(SB_VERT,&si));

	int		nMinPos , nMaxPos;
	nMinPos = si.nMin;
	nMaxPos = si.nMax;

	int nVertPos = si.nPos;

	if (nSBCode == SB_LINEDOWN)
	{
		++nVertPos;
	}
	else if (nSBCode == SB_LINEUP)
	{
		--nVertPos;
	}
	else if (nSBCode == SB_PAGEDOWN)
	{
		nVertPos += si.nPage;
	}
	else if (nSBCode == SB_PAGEUP)
	{
		nVertPos -= si.nPage;
	}
	else if (nSBCode == SB_THUMBPOSITION ||nSBCode == SB_THUMBTRACK)
	{
		nVertPos = nPos;
	}

	if (nVertPos < nMinPos)
	{
		nVertPos = nMinPos;
	}

	if (nVertPos > nMaxPos)
	{
		nVertPos = nMaxPos;
	}

	if (si.nPos == nVertPos)
	{
		return;
	}
	si.nPos = nVertPos;
	m_nCurrentFrame = nVertPos;
	if (m_nPlaneNum == 1)
		m_nZPos = m_nCurrentFrame;
	else if (m_nPlaneNum == 2)
		m_nXPos = m_nCurrentFrame;
	else if (m_nPlaneNum == 3)
		m_nYPos = m_nCurrentFrame;
	SetScrollInfo(SB_VERT,&si);
	// 	CDcmPic* pDcm = m_SeVisualMPR.GetMPRImage(nVertPos);
	// 	if (pDcm!=NULL)
	// 		DrawImage(pDcm);
	// 	Invalidate(FALSE);
	// 	UpdateWindow();

	theGeneralSwapData.m_pXOYView->UpdateImage(m_nZPos);
	theGeneralSwapData.m_pYOZView->UpdateImage(m_nYPos);
	theGeneralSwapData.m_pXOZView->UpdateImage(m_nXPos);

	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);

	theGeneralSwapData.m_pXOYView->UpdateWindow();
	theGeneralSwapData.m_pXOZView->UpdateWindow();
	theGeneralSwapData.m_pYOZView->UpdateWindow();	
	//CImageViewerView::OnVScroll(nSBCode, nPos, pScrollBar);
}


BOOL SeMPRView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (zDelta > 0 && m_nCurrentFrame > 0)
	{
		int nTmp = m_nCurrentFrame - int(abs(zDelta)/120);
		m_nCurrentFrame = nTmp >= 0 ? nTmp : 0;
	}
	else if(zDelta < 0 && m_nCurrentFrame < m_nPicCount - 1)
	{
		int nTmp = m_nCurrentFrame + int(abs(zDelta)/120);
		m_nCurrentFrame = nTmp <= m_nPicCount - 1 ? nTmp : m_nPicCount - 1; 
	}
	if (m_nPlaneNum == 1)
		m_nZPos = m_nCurrentFrame;
	else if (m_nPlaneNum == 2)
		m_nXPos = m_nCurrentFrame;
	else if (m_nPlaneNum == 3)
		m_nYPos = m_nCurrentFrame;

	// 	CDcmPic* pDcm = m_SeVisualMPR.GetMPRImage(m_nCurrentFrame);
	// 	if (pDcm!=NULL)
	// 		DrawImage(pDcm);
	theGeneralSwapData.m_pXOYView->UpdateImage(m_nZPos);
	theGeneralSwapData.m_pYOZView->UpdateImage(m_nYPos);
	theGeneralSwapData.m_pXOZView->UpdateImage(m_nXPos);

	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);

	theGeneralSwapData.m_pXOYView->UpdateWindow();
	theGeneralSwapData.m_pXOZView->UpdateWindow();
	theGeneralSwapData.m_pYOZView->UpdateWindow();	

	return CImageViewerView::OnMouseWheel(nFlags, zDelta, pt);
}

Bitmap* SeMPRView::GreatePng(CImageBase* pImg, int nMin, int nMax, COLORREF color, DWORD alpha)
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
			if (tmp >= nMin && tmp <= nMax)
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


void SeMPRView::SetPngInfo(int nMin, int nMax, COLORREF color, DWORD alpha)
{
	m_nMin = nMin;
	m_nMax = nMax;
	m_color = color;
	m_alpha = alpha;
}


void SeMPRView::Reset()
{
	m_vecPoid.clear();
	for (int i=0; i<m_vecROIData.size(); i++)
	{
		Safe_Delete(m_vecROIData[i]);
	}
	m_vecROIData.clear();
	if(m_pVecEdge != NULL)
	{
		for (int i=0; i<m_nVecEageSize; i++)
		{
			m_pVecEdge[i].clear();
		}
	}
	delete [] m_pVecEdge;
	m_pVecEdge = NULL;

}

void SeMPRView::Interpolate(int nStart, int nEnd, vector <CPoint>* pVecEdge)
{
	int nSize = nEnd - nStart;
	m_ScopeTool.InterpolatePrePare(pVecEdge[nStart], pVecEdge[nEnd], MAX_LENGTH);
	for(int i=1; i<nSize; i++)
	{
		m_ScopeTool.InterpolatePoints(pVecEdge[nStart], pVecEdge[nEnd],(float)i/nSize, pVecEdge[nStart + i]);
	}
}



void SeMPRView::DeleteROI()
{
	m_vecPoid.clear();
	if(m_pVecEdge != NULL)
	{
		for (int i=0; i<m_nVecEageSize; i++)
		{
			m_pVecEdge[i].clear();
		}
	}
}

void SeMPRView::FillMidLayer()
{
	if(m_vecPoid.size()<2)
		return;

	sort(m_vecPoid.begin(), m_vecPoid.end());
	m_vecPoid.erase(unique(m_vecPoid.begin(), m_vecPoid.end()), m_vecPoid.end());
	for(int i=0; i<m_vecPoid.size() -1; i++)
	{
		Interpolate(m_vecPoid[i], m_vecPoid[i + 1], m_pVecEdge);
	}
	m_rtROI = GetROIRect(m_vecPoid);
}

int SeMPRView::GetAxialPos()
{
	if (m_nPlaneNum == 1)
		return (int)(m_nCurrentFrame * (theGeneralSwapData.m_dbXYSliceSpace / theGeneralSwapData.m_dbZSliceSpace));
	else 
		return m_nCurrentFrame;
}

void SeMPRView::GetInfo(vector <CPoint> pts)
{
	CDcmPic* pDcm = (CDcmPic *)GetDisplayMgr()->GetFirstDispImage();
	GraphicsPath gcPath;
	const int nCount = (int)pts.size();
	PointF *ptFCurve = new PointF[nCount];
	for(int i=0; i<nCount; i++)
	{
		ptFCurve[i].X = pts[i].x;
		ptFCurve[i].Y = pts[i].y;
	}
	gcPath.AddClosedCurve(ptFCurve, (INT)pts.size());
	Region intersectRegion(&gcPath);  
	Matrix matrix; 
	int count = 0;  
	long PixelDataTotle = 0;
	vector<int> vecPixelList;
	count = intersectRegion.GetRegionScansCount(&matrix); 
	Rect* rects = NULL;
	rects = (Rect*)malloc(count*sizeof(Rect));  
	intersectRegion.GetRegionScans(&matrix, rects, &count);  
	for(int j=0; j<count; j++)  
	{
		int nWidth = rects[j].Width;
		for(int m=0; m<nWidth; m++)
		{
			int nHeight = rects[j].Height;
			for(int k=0; k<nHeight; k++)
			{
				if(rects[j].X + m >= 0 && rects[j].X + m <= pDcm->GetWidth() && rects[j].Y + k >=0 && rects[j].Y + k < pDcm->GetHeight())
				{
					int pixel = pDcm->GetPixelData(rects[j].X + m, rects[j].Y + k);
					PixelDataTotle += pixel;
					vecPixelList.push_back(pixel);
				}
			}
		}
	}

	free(rects); 


	//返回面积值  
	m_fROIArea = vecPixelList.size() * pDcm->GetMMPerpixel() * pDcm->GetMMPerpixel();   
	//返回平均值
	m_fAverageCTValue = (double)PixelDataTotle/vecPixelList.size();
	//返回标准差
	double sum = 0.0;
	for(int i = 0; i< vecPixelList.size(); i++)
	{
		sum += (vecPixelList[i] - m_fAverageCTValue)*(vecPixelList[i] - m_fAverageCTValue);
	}
	m_fVariance = sqrt(sum/(double)vecPixelList.size());
}

void SeMPRView::GetBasicInfo(int nRow)
{
	if (nRow < 0 || nRow >= static_cast<int>(m_vecROIData.size()))
		return;

	CSeROIData* pRoi = m_vecROIData[nRow];
	if (pRoi == NULL)
		return;

	if (m_nOriImageWidth <= 0 || m_nOriImageHeight <= 0 || m_nOriImagePiece <= 0)
		return;

	BYTE* pMask = pRoi->GetData();
	if (pMask == NULL)
		return;

	const size_t sliceSize = static_cast<size_t>(m_nOriImageWidth) * static_cast<size_t>(m_nOriImageHeight);
	const int histogramSize = SHORT_MAX - SHORT_MIN + 1;
	vector<LONGLONG> histogram(histogramSize, 0);

	LONGLONG voxelCount = 0;
	LONGLONG sumIntensity = 0;
	long minValue = LONG_MAX;
	long maxValue = LONG_MIN;
	long double sumSquares = 0.0L;

	for (int slice = 0; slice < m_nOriImagePiece; ++slice)
	{
		short* pSliceData = (short*)m_pDcmArray->GetDcmArray()[slice]->GetData();
		BYTE* pMaskSlice = pMask + sliceSize * static_cast<size_t>(slice);
		for (size_t idx = 0; idx < sliceSize; ++idx)
		{
			if (pMaskSlice[idx] != 0)
			{
				const short value = pSliceData[idx];
				voxelCount++;
				sumIntensity += value;
				sumSquares += static_cast<long double>(value) * static_cast<long double>(value);
				if (value < minValue)
					minValue = value;
				if (value > maxValue)
					maxValue = value;
				histogram[value - SHORT_MIN]++;
			}
		}
	}

	if (voxelCount == 0)
	{
		MessageBoxTimeout(NULL, _T("         ROI 中没有有效体素！       "), _T("提示"), MB_ICONINFORMATION, 0, 1000);
		return;
	}

	CDcmPic* pDcm = m_pDcmArray->GetDcmArray()[0];
	double pixelSize = pDcm->GetMMPerpixel();
	if (pixelSize <= 0.0)
		pixelSize = 1.0;
	double sliceSpace = pDcm->GetSliceSpace();
	if (sliceSpace <= 0.0)
		sliceSpace = 1.0;
	const double voxelVolume = pixelSize * pixelSize * sliceSpace;
	const double volumeMm3 = static_cast<double>(voxelCount) * voxelVolume;
	const double volumeMl = volumeMm3 / 1000.0;

	const double mean = static_cast<double>(sumIntensity) / static_cast<double>(voxelCount);
	const double meanSquare = static_cast<double>(sumSquares) / static_cast<double>(voxelCount);
	double variance = meanSquare - mean * mean;
	if (variance < 0.0)
		variance = 0.0;
	const double stdDeviation = sqrt(variance);

	const LONGLONG totalCount = voxelCount;
	const LONGLONG medianRank = (totalCount + 1) / 2;
	const LONGLONG p10Rank = (totalCount * 10 + 99) / 100;
	const LONGLONG p90Rank = (totalCount * 90 + 99) / 100;

	int p10Value = static_cast<int>(minValue);
	int medianValue = static_cast<int>(minValue);
	int p90Value = static_cast<int>(maxValue);
	bool foundP10 = false;
	bool foundMedian = false;
	bool foundP90 = false;
	LONGLONG cumulative = 0;
	for (int i = 0; i < histogramSize; ++i)
	{
		cumulative += histogram[i];
		if (!foundP10 && cumulative >= p10Rank)
		{
			p10Value = SHORT_MIN + i;
			foundP10 = true;
		}
		if (!foundMedian && cumulative >= medianRank)
		{
			medianValue = SHORT_MIN + i;
			foundMedian = true;
		}
		if (!foundP90 && cumulative >= p90Rank)
		{
			p90Value = SHORT_MIN + i;
			foundP90 = true;
		}
		if (foundP10 && foundMedian && foundP90)
			break;
	}

	CString str;
	str.Format(_T("体素数：%I64d\n体积：%.2f mm^3 (%.3f mL)\n最小值：%d HU\n最大值：%d HU\n平均值：%.2f HU\n中位数：%d HU\n10%% 分位数：%d HU\n90%% 分位数：%d HU\n标准差：%.2f HU\n方差：%.2f"),
		voxelCount,
		volumeMm3,
		volumeMl,
		static_cast<int>(minValue),
		static_cast<int>(maxValue),
		mean,
		medianValue,
		p10Value,
		p90Value,
		stdDeviation,
		variance);

	::MessageBox(NULL,str, _T("ROI统计"), MB_ICONINFORMATION);
}

void SeMPRView::ClipAllImage()
{
	if (m_pVecEdge == NULL)
		return;

	theAppIVConfig.m_pILog->ProgressInit(m_nOriImagePiece);
	CImageViewerDoc* pDoc = (CImageViewerDoc*)GetDocument();
	pDoc->ReleaseSeries();
	CDcmPicArray DcmArray;
	for(int i = 0; i < m_nOriImagePiece; i++)
	{
		if (!m_pVecEdge[i].empty())
		{
			m_pDcmArray->GetDcmArray()[i]->ReloadBuffer();
			short* pTempData = new short[m_nOriImageWidth*m_nOriImageHeight];
			memcpy(pTempData, (short*)m_pDcmArray->GetDcmArray()[i]->GetData(),m_nOriImageWidth*m_nOriImageHeight*sizeof(short));
			CRgn rgnROI;
			int nSize = (int)m_pVecEdge[i].size();
			CPoint *ptEdge = new CPoint[nSize];
			for (int k = 0; k < nSize; k++)
			{
				ptEdge[k] = m_pVecEdge[i][k];
			}
			rgnROI.CreatePolygonRgn(ptEdge, (int)m_pVecEdge[i].size(), ALTERNATE);
			Safe_Delete(ptEdge);
			for (int m = 0; m < m_nOriImageHeight; m++)
			{
				for (int n = 0; n < m_nOriImageWidth; n++)
				{
					CPoint pt(n,m);
					if (!rgnROI.PtInRegion(pt)/* == !theBoneDensitySwapData.m_bClipOutside*/)
					{
						pTempData[m_nOriImageWidth*m + n] = m_sMinValue;
					}
				}
			}
			CDcmPic* pDcm = m_pDcmArray->GetDcmArray()[i]->CloneDcmPic();
			pDcm->SetPixelData((BYTE*)pTempData, m_nOriImageWidth, m_nOriImageHeight);	
			pDoc->AddImage(pDcm, -1, FALSE);
		} 	
		/*		Safe_Delete(m_pDcmArray->GetDcmArray()[i]);*/
		theAppIVConfig.m_pILog->ProgressStepIt();
	}
	//  	m_pDcmArray->ReleaseArray();
	for (int j = 0;j <pDoc->GetImageSet()->GetSize();j++)
	{
		DcmArray.AddDcmImage((CDcmPic*)pDoc->GetImageSet()->GetImage(j));
	}
	m_pDcmArray = &DcmArray;
	DeleteROI();
	m_nOriImagePiece = m_pDcmArray->GetZDcmPicCount();

	theAppIVConfig.m_pILog->ProgressClose();
}

void SeMPRView::ClipAllImage(CRect rect, int nStartPiece, int nEndPiece)
{
	if(rect.IsRectEmpty() || nEndPiece < nStartPiece)
		return;
	theAppIVConfig.m_pILog->ProgressInit(m_nOriImagePiece);
	for(int i=nStartPiece-1; i<nEndPiece; i++)
	{
		m_pDcmArray->GetDcmArray()[i]->ReloadBuffer();
		short* pOriData = new short[m_nOriImageWidth*m_nOriImageHeight];
		short* pTempData = new short[m_nOriImageWidth*m_nOriImageHeight];
		for(int k=0; k<m_nOriImageWidth*m_nOriImageHeight; k++)
			*pTempData++ = m_sMinValue;
		//		memcpy(pTempData, (short*)m_pDcmArray->GetDcmArray()[i]->GetData(),m_nOriImageWidth*m_nOriImageHeight*sizeof(short));
		pTempData -= m_nOriImageWidth * (m_nOriImageHeight - rect.top);
		pOriData += m_nOriImageWidth * rect.top;
		for(int j=rect.top; j<=rect.bottom; j++)
		{
			pTempData += rect.left;
			pOriData += rect.left;
			memcpy(pTempData, pOriData, sizeof(short)*rect.Width());
			pTempData += (rect.Width() - rect.right);
			pOriData += (rect.Width() - rect.right);

		}
		delete [] pOriData;
		//		CDcmPic* pDcm = m_pDcmArray->GetDcmArray()[i]->CloneDcmPic();
		m_pDcmArray->GetDcmArray()[i]->SetPixelData((BYTE*)pTempData, m_nOriImageWidth, m_nOriImageHeight);
		//		Safe_Delete(pDcm);
		theAppIVConfig.m_pILog->ProgressStepIt();
	}

	theAppIVConfig.m_pILog->ProgressClose();
}

void SeMPRView::SaveAllImage()
{
	CString csFolderPath = theGeneralSwapData.m_csFolderPath + "\\ROI";
	CreateFolder(csFolderPath);
	theAppIVConfig.m_pILog->ProgressInit(m_nOriImagePiece);
	for(int nIndex=0; nIndex<m_nOriImagePiece; nIndex++)
	{
		CString csName;
		CString csIndex;
		csIndex.Format("%d", nIndex);
		if (nIndex<10)
		{
			csName = "Hiscan_0000" + csIndex;
		}
		else if (nIndex>=10 && nIndex<100)
		{
			csName = "Hiscan_000" + csIndex;
		}
		else if (nIndex>=100)
		{
			csName = "Hiscan_00" + csIndex;
		}
		else
			csName = "Hiscan_0" + csIndex;
		CDcmPic* pDcm = m_pDcmArray->GetDcmArray()[nIndex]->CloneDcmPic();
		CDcmElement* pEle = pDcm->GetDcmElemList().FindElem(0x0020, 0x0013);
		pEle->ValueAdd(csName);
		CString csFilePath = csFolderPath + "\\" + csName + ".dcm";
		pDcm->ExportDcm(csFilePath);
		Safe_Delete(pDcm);
		theAppIVConfig.m_pILog->ProgressStepIt();
	}
	theAppIVConfig.m_pILog->ProgressClose();
}

void SeMPRView::CreateFolder(CString csPath)
{
	if (!::CreateDirectoryA(csPath, NULL))
	{
		CFileFind   filefind;
		CString     csfilePath = csPath + "\\*.dcm";
		BOOL	bFind = filefind.FindFile(csfilePath); 
		while(bFind)
		{
			bFind = filefind.FindNextFile();
			if(filefind.IsDots())
				continue;
			else if(filefind.IsDirectory())
				continue;		
			else
			{
				CString   csfilename = filefind.GetFilePath();
				::DeleteFile(csfilename);
			}
		}
	}
}

CRect SeMPRView::GetROIRect(vector <int> vecPoid)
{
	CRect rect(m_nOriImageWidth,m_nOriImageHeight, 0, 0);
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
	if(rect.right < m_nOriImageWidth - 10) rect.right += 10;
	if(rect.top > 10) rect.top -= 10;
	if(rect.bottom < m_nOriImageHeight - 10) rect.bottom += 10;
	return rect;
}

VolTexInfo SeMPRView::ConvertDcms2VolTex(int nRow)
{
	VolTexInfo info;
	info.nWidth = m_nOriImageWidth;
	info.nHelght = m_nOriImageHeight;
	info.nLength = m_nOriImagePiece;
	BYTE* pData = new BYTE[m_nOriImageWidth * m_nOriImageHeight * m_nOriImagePiece];
	BYTE* pDatePoint = pData;
	memset(pData, 0, sizeof(BYTE) * m_nOriImageWidth * m_nOriImageHeight * m_nOriImagePiece);
	return info;
}



void SeMPRView::ConvertDcms2VolTex(MaskInfo info)
{
	CSeROIData* pData = new CSeROIData(m_nOriImageWidth, m_nOriImageHeight, m_nOriImagePiece, m_pDcmArray, info.nMin, info.nMax, info.color, info.alpha, info.bShow);
	m_vecROIData.push_back(pData);
}

Bitmap* SeMPRView::CreatePng(int nPlane, int nLayer, CSeROIData* data, int nWidth, int nHeight)
{
	BYTE* pData = data->GetSliceData(nLayer, nPlane);
	if (pData == NULL)
		return NULL;

	// 获取数据的实际尺寸
	int nDataWidth = 0;
	int nDataHeight = 0;
	switch(nPlane)
	{
	case 1: // XOY平面
		nDataWidth = data->GetWidth();
		nDataHeight = data->GetHeight();
		break;
	case 2: // YOZ平面
		nDataWidth = data->GetHeight();
		nDataHeight = data->GetLength();
		break;
	case 3: // XOZ平面
		nDataWidth = data->GetWidth();
		nDataHeight = data->GetLength();
		break;
	default:
		return NULL;
	}

	// 如果尺寸为0，返回NULL
	if (nDataWidth <= 0 || nDataHeight <= 0)
		return NULL;

	// 创建目标位图
	PixelFormat pixelFormat = PixelFormat32bppARGB;
	Bitmap myBmp(nWidth, nHeight, pixelFormat);
	BitmapData myData;
	Rect rect(0, 0, nWidth, nHeight);
	myBmp.LockBits(&rect, ImageLockModeWrite, pixelFormat, &myData);

	DWORD* pMyData = (DWORD*)myData.Scan0;

	// 获取颜色信息
	DWORD r = GetRValue(data->GetColor());
	DWORD g = GetGValue(data->GetColor());
	DWORD b = GetBValue(data->GetColor());
	DWORD rgba = (data->GetAlpha() << 24) | (r << 16) | (g << 8) | b;

	// 计算缩放比例
	float scaleX = static_cast<float>(nDataWidth) / static_cast<float>(nWidth);
	float scaleY = static_cast<float>(nDataHeight) / static_cast<float>(nHeight);

	// 使用最近邻插值进行缩放
	for(int i = 0; i < nHeight; i++)
	{
		for(int j = 0; j < nWidth; j++)
		{
			// 计算在原始数据中的位置
			int srcX = static_cast<int>(j * scaleX);
			int srcY = static_cast<int>(i * scaleY);

			// 边界检查
			if (srcX >= nDataWidth) srcX = nDataWidth - 1;
			if (srcY >= nDataHeight) srcY = nDataHeight - 1;
			if (srcX < 0) srcX = 0;
			if (srcY < 0) srcY = 0;

			// 获取源数据
			BYTE tmp = pData[srcY * nDataWidth + srcX];

			if (tmp != 0)
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

//Bitmap* SeMPRView::CreatePng(int nPlane, int nLayer, CSeROIData* data, int nWidth, int nHeight)
//{
//// 	BYTE* pData;
//// 	try
//// 	{
//// 		pData = data->GetSliceData(nLayer, nPlane);
//// 	}
//// 	catch (CException* e)
//// 	{
//// 		pData = NULL;
//// 	}
//	BYTE* pData = data->GetSliceData(nLayer, nPlane);
//	if (pData == NULL)
//		return NULL;
//	BYTE* pFirstValue = pData;
//	pData = pFirstValue;
//	PixelFormat pixelFormat = PixelFormat32bppARGB;
//	Bitmap myBmp(nWidth, nHeight, pixelFormat);
//	BitmapData myData;
//	Rect rect(0, 0, nWidth, nHeight);
//	myBmp.LockBits(&rect, ImageLockModeWrite, pixelFormat, &myData);
//	int nStride = myData.Stride;
//	DWORD* pMyData = (DWORD*)myData.Scan0; 
//	DWORD r = GetRValue(data->GetColor());
//	DWORD g = GetGValue(data->GetColor());
//	DWORD b = GetBValue(data->GetColor());
//
//	DWORD rgba = (data->GetAlpha() << 24) | (r << 16) | (g << 8) | b  ;
//
//	for(int i=0; i<nHeight; i++)
//	{
//		for(int j=0; j<nWidth; j++)
//		{
//			BYTE tmp = *pData++;
//			if (tmp != 0)
//			{
//				*pMyData++ = rgba;
//
//			}
//			else
//			{
//				*pMyData++ = 0;
//			}
//		}
//	}
//	myBmp.UnlockBits(&myData); 
//
//	return myBmp.Clone(0, 0, nWidth, nHeight, pixelFormat);
//}


void SeMPRView::ROI(int nPos, ROI_OPERATION op)
{
	m_vecROIData[nPos]->ROI(m_pVecEdge, GetPlaneNum(), op);
}


// unused
BYTE* SeMPRView::ConvertOneDcm(CImageBase* pImg, MaskInfo info)
{
	if(pImg == NULL)
		return NULL;
	int nWidth = pImg->GetWidth();
	int nHeight = pImg->GetHeight();
	short* pData = (short*)pImg->GetData();
	short* pFirstValue = pData;
	BYTE* pMyData = new BYTE[nWidth * nHeight];
	BYTE* pMyDataPoint = pMyData;
	memset(pMyData, 0, sizeof(BYTE) * nWidth * nHeight);
	for(int i=0; i<nHeight; i++)
	{
		for(int j=0; j<nWidth; j++)
		{
			short tmp = *pData++;
			if (tmp > info.nMin && tmp < info.nMax)
				*pMyDataPoint++ = 255;
			else
				*pMyDataPoint++ = 0;
		}
	}
	return pMyData;
}

void SeMPRView::MultiThreadFreecut(float xStart, float xEnd, float yStart, float yEnd, float zStart, float zEnd, glm::mat4 mat1, glm::mat4 mat2, glm::mat4 mat3, glm::mat4 mat4)
{
	const int nSize = static_cast<int>(m_pDcmArray->GetDcmArray().size());
	const int  nWidth = m_pDcmArray->GetDcmArray()[0]->GetWidth();
	const int  nHeight = m_pDcmArray->GetDcmArray()[0]->GetHeight();
	CString strFolder = theGeneralSwapData.m_csFolderPath;
	int pos = strFolder.ReverseFind('\\');
	CString newstrFolder = strFolder.Left(pos) + "\\Freecut" + strFolder.Right(strFolder.GetLength() - pos - 1);
	CSeToolKit::CreateDeepDirectory(newstrFolder);

	glm::mat4 matrix_total(1.0f);

	float fLength = static_cast<float>(nSize);
	float fWidth = static_cast<float>(nWidth);
	float fHeight = static_cast<float>(nHeight);

	matrix_total *= glm::inverse(glm::scale(mat1, glm::vec3(1.0f, fHeight/fWidth, fLength/fWidth))) * 
		glm::inverse(mat2) * 
		mat3 * 
		glm::scale(mat4, glm::vec3(1.0f, fHeight/fWidth, fLength/fWidth));

	int thread_number = 12;

	theAppIVConfig.m_pILog->ProgressInit(static_cast<int>(nSize));

	size_t size = nSize / thread_number;
	size_t last_size = size + nSize % thread_number;
	CutInfo info;
	info.nXstart = xStart;
	info.nXend = xEnd;
	info.nYstart = yStart;
	info.nYend = yEnd;
	info.nZstart = zStart;
	info.nZend = zEnd;

	boost::thread_group grp;

	for (size_t i=0; i<thread_number; i++) {
		int nStart = static_cast<int>(size * i);
		int nEnd = nStart;
		if (i != thread_number - 1) {
			 nEnd = static_cast<int>(size * i + size);
		}
		else {
			nEnd = static_cast<int>(nSize);
		}
		grp.create_thread(boost::bind(boost::mem_fn(&SeMPRView::__Freecut), this, nStart, nEnd, newstrFolder, info, matrix_total));
	}

	grp.join_all();

	MessageBoxTimeout(NULL, _T("         导出文件完成！       "), _T("提示"), MB_ICONINFORMATION, 0, 300);
	theAppIVConfig.m_pILog->ProgressClose();
}

void SeMPRView::__Freecut(const int nStart, const int nEnd, const CString folder, const CutInfo info, glm::mat4 matrix_total)
{
	for (int k=nStart; k<nEnd; k++)
	{
		int nIndex = k;
		short* pData =  (short*)m_pDcmArray->GetDcmArray()[nIndex]->GetData();
		int nWidth = m_pDcmArray->GetDcmArray()[nIndex]->GetWidth();
		int nHeight = m_pDcmArray->GetDcmArray()[nIndex]->GetHeight();
	int nSize = static_cast<int>(m_pDcmArray->GetDcmArray().size());
		short* pNewData = new short[nWidth * nHeight];
		memset(pNewData, 0, sizeof(short) * nWidth * nHeight);
		short* pHead = pData;
		short* pNewHead = pNewData;
		for (int j=0; j<nHeight; j++)
		{
			for(int i=0; i<nWidth; i++) {

				glm::vec4 pos((static_cast<float>(i) + 0.5f)/static_cast<float>(nWidth), (static_cast<float>(j) + 0.5f)/static_cast<float>(nHeight), (static_cast<float>(k) + 0.5f)/static_cast<float>(nSize), 1.0f);
				glm::vec4 range = matrix_total * pos;
				if (range.x >= info.nXstart && range.x <= info.nXend && range.y >= info.nYstart && range.y <= info.nYend && range.z >= info.nZstart && range.z <= info.nZend) {
					*pNewHead = *pHead;	
				}
				pNewHead++;
				pHead++;
			}
		}
		CDcmPic* pDcm = m_pDcmArray->GetDcmArray()[nIndex]->CloneDcmPic();
		pDcm->SetPixelData((BYTE*)pNewData, nWidth, nHeight);

		CString csName;
		CString csIndex;
		csIndex.Format("%d", nIndex);
		if (nIndex<10)
		{
			csName = "Hiscan_0000" + csIndex;
		}
		else if (nIndex>=10 && nIndex<100)
		{
			csName = "Hiscan_000" + csIndex;
		}
		else if (nIndex>=100)
		{
			csName = "Hiscan_00" + csIndex;
		}
		else
			csName = "Hiscan_0" + csIndex;

		CString strFile = folder + "\\" + csName + _T(".dcm");
		CDcmElement* pEle = pDcm->GetDcmElemList().FindElem(0x0020, 0x0013);
		pEle->ValueAdd(csName);
		pDcm->ExportDcm(strFile);
		Safe_Delete(pDcm);
		theAppIVConfig.m_pILog->ProgressStepIt();
	}
}

void SeMPRView::ExportImages(BYTE* pMask)
{
	const auto nSize = m_pDcmArray->GetDcmArray().size();
	const int  nWidth = m_pDcmArray->GetDcmArray()[0]->GetWidth();
	const int  nHeight = m_pDcmArray->GetDcmArray()[0]->GetHeight();
	CString strFolder = theGeneralSwapData.m_csFolderPath;
	int pos = strFolder.ReverseFind('\\');
	CString newstrFolder = strFolder.Left(pos) + "\\Part" + strFolder.Right(strFolder.GetLength() - pos - 1);
	CSeToolKit::CreateDeepDirectory(newstrFolder);
	BYTE* pMaskHead = pMask;
	theAppIVConfig.m_pILog->ProgressInit(static_cast<int>(nSize));
 	for (auto i=0; i<nSize; i++)
 	{
 		short* pData =  (short*)m_pDcmArray->GetDcmArray()[i]->GetData();
 		short* pNewData = new short[nWidth * nHeight];
 		memcpy(pNewData, pData, sizeof(short) * nWidth * nHeight);
 		short* pHead = pNewData;
 		for (int j=0; j<nWidth*nHeight; j++)
 		{
 			if (*pMaskHead == 0)
 				*pHead = 0;
// 			else
// 				*pHead = 1;
 			pMaskHead++;
 			pHead++;
 		}
 		CDcmPic* pDcm = m_pDcmArray->GetDcmArray()[i]->CloneDcmPic();
 		pDcm->SetPixelData((BYTE*)pNewData, nWidth, nHeight);
 
 		CString csName;
 		CString csIndex;
 		int nIndex = i;
 		csIndex.Format("%d", nIndex);
 		if (nIndex<10)
 		{
 			csName = "Hiscan_0000" + csIndex;
		}
 		else if (nIndex>=10 && nIndex<100)
 		{
 			csName = "Hiscan_000" + csIndex;
 		}
 		else if (nIndex>=100)
 		{
 			csName = "Hiscan_00" + csIndex;
 		}
 		else
 			csName = "Hiscan_0" + csIndex;
 		
 		CString strFile = newstrFolder + "\\" + csName + _T(".dcm");
 		CDcmElement* pEle = pDcm->GetDcmElemList().FindElem(0x0020, 0x0013);
 		pEle->ValueAdd(csName);
 		pDcm->ExportDcm(strFile);
 		Safe_Delete(pDcm);
		theAppIVConfig.m_pILog->ProgressStepIt();
 	}
	MessageBoxTimeout(NULL, _T("         导出文件完成！       "), _T("提示"), MB_ICONINFORMATION, 0, 300);
	theAppIVConfig.m_pILog->ProgressClose();

// 	CDcmPicArray* pDcmArray = m_pDcmArray;
// 	CString csFullPath = newstrFolder + "\\Part" + strFolder.Right(strFolder.GetLength() - pos - 1);
// 	CSeToolKit::CreateDeepDirectory(csFullPath);
// 	for (int nIndex=0; nIndex<nSize; nIndex++)
// 	{
// 		CString csName;
// 		CString csIndex;
// 		csIndex.Format("%d", nIndex);
// 		if (nIndex<10)
// 		{
// 			csName = "Hiscan_0000" + csIndex;
// 		}
// 		else if (nIndex>=10 && nIndex<100)
// 		{
// 			csName = "Hiscan_000" + csIndex;
// 		}
// 		else if (nIndex>=100)
// 		{
// 			csName = "Hiscan_00" + csIndex;
// 		}
// 		else
// 			csName = "Hiscan_0" + csIndex;
// 		CDcmPic* pDcm = pDcmArray->GetDcmArray()[nIndex]->CloneDcmPic();
// 		short* pTmp = new short[pDcm->GetWidth() * pDcm->GetHeight()];
// 		memset(pTmp, 0, sizeof(short) * pDcm->GetWidth() * pDcm->GetHeight());
// 		
// 
// 
// 
// 
// 		pDcm->SetPixelData((BYTE*)pTmp, 
// 			pDcmArray->GetDcmArray()[nIndex]->GetWidth(), 
// 			pDcmArray->GetDcmArray()[nIndex]->GetHeight());
// 		CDcmElement* pEle = pDcm->GetDcmElemList().FindElem(0x0020, 0x0013);
// 		pEle->RemoveAllValue();
// 		pEle->ValueAdd(csIndex);
// 
// 		// 		pDcm->SetImageNumber(nIndex);
// 		// 		pDcm->ReloadImage();
// 		CString csFilePath = csFullPath + "\\" + csName + _T(".dcm");;
// 		pDcm->ExportDcm(csFilePath);
// 		Safe_Delete(pDcm);
// 	}
}

void SeMPRView::ExportImagesWithMask()
{
	const auto nSize = m_pDcmArray->GetDcmArray().size();
	const int  nWidth = m_pDcmArray->GetDcmArray()[0]->GetWidth();
	const int  nHeight = m_pDcmArray->GetDcmArray()[0]->GetHeight();
	const int  nBmpLineWidth = ((nWidth * 3 + 2) >> 2 << 2);
	const int  nW = m_SeVisualMPR.GetWinWidth();
	const int  nL = m_SeVisualMPR.GetWinCenter();
	CString strFolder = theGeneralSwapData.m_csFolderPath;
	int pos = strFolder.ReverseFind('\\');
	CString newstrFolder = strFolder.Left(pos) + "\\Mask" + strFolder.Right(strFolder.GetLength() - pos - 1);
	CSeToolKit::CreateDeepDirectory(newstrFolder);


	vector<BYTE*> vecMaskPtr;
	vector<COLORREF> vecMaskColor;
	for(int i=0; i<m_vecROIData.size(); i++)
	{
		if (m_vecROIData[i]->IsVisible())
		{
			vecMaskPtr.push_back(m_vecROIData[i]->GetData());
			vecMaskColor.push_back(m_vecROIData[i]->GetColor());
		}
	}

	// bmp 4 字节对齐, 每行可能多需要 1，2，3个字节
	BYTE* pBmpHead = new BYTE[nBmpLineWidth * nHeight];
	

	theAppIVConfig.m_pILog->ProgressInit(static_cast<int>(nSize));
	for (auto i=0; i<nSize; i++)
	{
		BYTE* pData =  m_pDcmArray->GetDcmArray()[i]->GetData();
		memset(pBmpHead, 0, nBmpLineWidth * nHeight);
		Dcm2RGB(pBmpHead, pData, nWidth, nBmpLineWidth, nHeight, nW, nL);

		for(auto j=0; j<vecMaskPtr.size(); j++)
		{
			ReplaceRGB(pBmpHead, vecMaskPtr[j] + nWidth * nHeight * i, nWidth, nBmpLineWidth, nHeight, vecMaskColor[j]);
		}

		CString csName;
		CString csIndex;
		int nIndex = i;
		csIndex.Format("%d", nIndex);
		if (nIndex<10)
		{
			csName = "Hiscan_0000" + csIndex;
		}
		else if (nIndex>=10 && nIndex<100)
		{
			csName = "Hiscan_000" + csIndex;
		}
		else if (nIndex>=100)
		{
			csName = "Hiscan_00" + csIndex;
		}
		else
			csName = "Hiscan_0" + csIndex;

		CString strFile = newstrFolder + "\\" + csName + _T(".bmp");

		UpDownReplace(pBmpHead, nBmpLineWidth, nHeight);
		SaveBitmapToFile(pBmpHead, nWidth, nHeight, 24, nBmpLineWidth - nWidth * 3, strFile);
		theAppIVConfig.m_pILog->ProgressStepIt();
	}
	MessageBoxTimeout(NULL, _T("         导出文件完成！       "), _T("提示"), MB_ICONINFORMATION, 0, 300);
	theAppIVConfig.m_pILog->ProgressClose();
}

void SeMPRView::ExportCurrentImageWithMask()
{
	CDcmPic* pDcm = NULL;
	pDcm = m_SeVisualMPR.GetMPRImage(m_nCurrentFrame);
	const int nWidth = pDcm->GetWidth();
	const int nHeight = pDcm->GetHeight();
	const int  nBmpLineWidth = ((nWidth * 3 + 2) >> 2 << 2);
	const int  nW = m_SeVisualMPR.GetWinWidth();
	const int  nL = m_SeVisualMPR.GetWinCenter();
	CString strFolder = theGeneralSwapData.m_csFolderPath;
	int pos = strFolder.ReverseFind('\\');
	CString newstrFolder = strFolder.Left(pos) + "\\Mask" + strFolder.Right(strFolder.GetLength() - pos - 1);
	CSeToolKit::CreateDeepDirectory(newstrFolder);


	vector<BYTE*> vecMaskPtr;
	vector<COLORREF> vecMaskColor;
	for(int i=0; i<m_vecROIData.size(); i++)
	{
		if (m_vecROIData[i]->IsVisible())
		{
			vecMaskPtr.push_back(m_vecROIData[i]->GetSliceData(m_nCurrentFrame, m_nPlaneNum));
			vecMaskColor.push_back(m_vecROIData[i]->GetColor());
		}
	}

	// bmp 4 字节对齐, 每行可能多需要 1，2，3个字节
	BYTE* pBmpHead = new BYTE[nBmpLineWidth * nHeight];

	BYTE* pData =  pDcm->GetData();
	memset(pBmpHead, 0, nBmpLineWidth * nHeight);
	Dcm2RGB(pBmpHead, pData, nWidth, nBmpLineWidth, nHeight, nW, nL);

	for(auto j=0; j<vecMaskPtr.size(); j++)
	{
		ReplaceRGB(pBmpHead, vecMaskPtr[j], nWidth, nBmpLineWidth, nHeight, vecMaskColor[j]);
	}

	CString strFile;
	strFile.Format("%s\\%d%d.bmp", newstrFolder, m_nPlaneNum, m_nCurrentFrame);

	UpDownReplace(pBmpHead, nBmpLineWidth, nHeight);
	SaveBitmapToFile(pBmpHead, nWidth, nHeight, 24, nBmpLineWidth - nWidth * 3, strFile);
}

void SeMPRView::ExportMask(BYTE* pMask)
{
	const auto nSize = m_pDcmArray->GetDcmArray().size();
	const int  nWidth = m_pDcmArray->GetDcmArray()[0]->GetWidth();
	const int  nHeight = m_pDcmArray->GetDcmArray()[0]->GetHeight();
	CString strFolder = theGeneralSwapData.m_csFolderPath + "\\Mask.dat";

	ofstream ouF;
	ouF.open(strFolder, std::ofstream::binary);
	ouF.write(reinterpret_cast<const char*>(pMask), sizeof(BYTE) * nWidth * nHeight * nSize);
	ouF.close();
	MessageBoxTimeout(NULL, _T("         导出Mask完成！       "), _T("提示"), MB_ICONINFORMATION, 0, 1000);
}


void SeMPRView::LoadMask(CString strFileName, MaskInfo info)
{
	const auto nSize = m_pDcmArray->GetDcmArray().size();
	const int  nWidth = m_pDcmArray->GetDcmArray()[0]->GetWidth();
	const int  nHeight = m_pDcmArray->GetDcmArray()[0]->GetHeight();

	CSeROIData* pData = new CSeROIData(strFileName, m_nOriImageWidth, m_nOriImageHeight, m_nOriImagePiece, info.color, info.alpha, info.bShow);
	m_vecROIData.push_back(pData);

	MessageBoxTimeout(NULL, _T("         加载Mask完成！       "), _T("提示"), MB_ICONINFORMATION, 0, 1000);
}

void SeMPRView::ExportFreeCutImages(float xStart, float xEnd, float yStart, float yEnd, float zStart, float zEnd, glm::mat4 mat1, glm::mat4 mat2, glm::mat4 mat3, glm::mat4 mat4)
{
	const auto nSize = m_pDcmArray->GetDcmArray().size();
	const int  nWidth = m_pDcmArray->GetDcmArray()[0]->GetWidth();
	const int  nHeight = m_pDcmArray->GetDcmArray()[0]->GetHeight();
	CString strFolder = theGeneralSwapData.m_csFolderPath;
	int pos = strFolder.ReverseFind('\\');
	CString newstrFolder = strFolder.Left(pos) + "\\Freecut" + strFolder.Right(strFolder.GetLength() - pos - 1);
	CSeToolKit::CreateDeepDirectory(newstrFolder);


// 	glm::mat4 model, view;
// 	model = glm::mat4(1.0f);//m_pGLMatrix->GetMatrix();
// 	glm::vec4 move = m_pGLCamera->GetFreeCutMove();
// 	model = glm::translate(model, glm::vec3(-0.5f + move.x, -0.5f + move.y, -0.5f + move.z));
// 
// 	model = glm::scale(model, glm::vec3(1.0f, /fWidth, fLength/fWidth));
// 	view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
// 
// 	view *= m_pGLCamera->GetFreeCutMatrix();
// 
// 	glm::mat4 model, view;
// 	model = glm::mat4(1.0f);//m_pGLMatrix->GetMatrix();
// 	glm::vec4 move = m_pGLCamera->GetFreeCutMove();
// 	model = glm::translate(model, glm::vec3(-0.5f + move.x, -0.5f + move.y, -0.5f + move.z));
// 
// 	model = glm::scale(model, glm::vec3(1.0f, /fWidth, fLength/fWidth));
// 	view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
// 
// 	view *= m_pGLCamera->GetFreeCutMatrix();

	glm::mat4 matrix_total(1.0f);

	float fLength = static_cast<float>(nSize);
	float fWidth = static_cast<float>(nWidth);
	float fHeight = static_cast<float>(nHeight);

	matrix_total *= glm::inverse(glm::scale(mat1, glm::vec3(1.0f, fHeight/fWidth, fLength/fWidth))) * 
		glm::inverse(mat2) * 
		mat3 * 
		glm::scale(mat4, glm::vec3(1.0f, fHeight/fWidth, fLength/fWidth));


	theAppIVConfig.m_pILog->ProgressInit(static_cast<int>(nSize));
	for (auto k=0; k<nSize; k++)
	{
		short* pData =  (short*)m_pDcmArray->GetDcmArray()[k]->GetData();
		short* pNewData = new short[nWidth * nHeight];
		memset(pNewData, 0, sizeof(short) * nWidth * nHeight);
		short* pHead = pData;
		short* pNewHead = pNewData;
		for (int j=0; j<nHeight; j++)
		{
			for(int i=0; i<nWidth; i++) {
				
				glm::vec4 pos((static_cast<float>(i) + 0.5f)/static_cast<float>(nWidth), (static_cast<float>(j) + 0.5f)/static_cast<float>(nHeight), (static_cast<float>(k) + 0.5f)/static_cast<float>(nSize), 1.0f);
				glm::vec4 range = matrix_total * pos;
				if (range.x >= xStart && range.x <= xEnd && range.y >= yStart && range.y <= yEnd && range.z >= zStart && range.z <= zEnd) {
					*pNewHead = *pHead;	
				}
				pNewHead++;
				pHead++;
			}
		}
		CDcmPic* pDcm = m_pDcmArray->GetDcmArray()[k]->CloneDcmPic();
		pDcm->SetPixelData((BYTE*)pNewData, nWidth, nHeight);

		CString csName;
		CString csIndex;
		int nIndex = k;
		csIndex.Format("%d", nIndex);
		if (nIndex<10)
		{
			csName = "Hiscan_0000" + csIndex;
		}
		else if (nIndex>=10 && nIndex<100)
		{
			csName = "Hiscan_000" + csIndex;
		}
		else if (nIndex>=100)
		{
			csName = "Hiscan_00" + csIndex;
		}
		else
			csName = "Hiscan_0" + csIndex;

		CString strFile = newstrFolder + "\\" + csName + _T(".dcm");
		CDcmElement* pEle = pDcm->GetDcmElemList().FindElem(0x0020, 0x0013);
		pEle->ValueAdd(csName);
		pDcm->ExportDcm(strFile);
		Safe_Delete(pDcm);
		theAppIVConfig.m_pILog->ProgressStepIt();
	}
	MessageBoxTimeout(NULL, _T("         导出文件完成！       "), _T("提示"), MB_ICONINFORMATION, 0, 300);
	theAppIVConfig.m_pILog->ProgressClose();
}

void SeMPRView::AddResult(CMeasurement* pMeasurement)
{
	if (pMeasurement != NULL){
		pMeasurement->m_nPlaneNumber = m_nPlaneNum;
		m_vecRst.push_back(pMeasurement);
		m_wndResult->SendMessage(WM_ADD_RESULT, (WPARAM)pMeasurement);
	}
}

void SeMPRView::CleanResult(CMeasurement* pMeasurement)
{
	for(int i=0; i<m_vecRst.size(); i++) {
		if(m_vecRst[i] == NULL) continue;
		CString strClassName = m_vecRst[i]->m_strToolName;
		if (strClassName == "Line") {
			CMeaLine* pTool = dynamic_cast<CMeaLine*>(m_vecRst[i]);
			assert(pTool != NULL);
			delete pTool;
		}
		else if (strClassName == "Angle") {
			CMeaAngle* pTool = dynamic_cast<CMeaAngle*>(m_vecRst[i]);
			assert(pTool != NULL);
			delete pTool;
		}
		else if (strClassName == "Shape") {
			CMeaShape* pTool = dynamic_cast<CMeaShape*>(m_vecRst[i]);
			assert(pTool != NULL);
			delete pTool;
		}
		else if (strClassName == "CT") {
			CMeaCT* pTool = dynamic_cast<CMeaCT*>(m_vecRst[i]);
			assert(pTool != NULL);
			delete pTool;
		}
		else if (strClassName == "Area") {
			CMeaArea* pTool = dynamic_cast<CMeaArea*>(m_vecRst[i]);
			assert(pTool != NULL);
			delete pTool;
		}
	}
}



void SeMPRView::UpdateResult(int nPos)
{
	m_wndResult->SendMessage(WM_UPDATE_RESULT, (WPARAM)&m_vecRst, (LPARAM)nPos);
}

void SeMPRView::SetSelectTool()
{	
	theGeneralSwapData.m_pXOYView->SetMouseTool(MT_MEASURE_SELECT);
	theGeneralSwapData.m_pXOZView->SetMouseTool(MT_MEASURE_SELECT);
	theGeneralSwapData.m_pYOZView->SetMouseTool(MT_MEASURE_SELECT);


	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);
}


void SeMPRView::MoveToToolPos(int nPos)
{
	assert(nPos < m_vecRst.size() && nPos >= 0);
	CMeasurement* pMeasurement = m_vecRst[nPos];
	int nPlaneNumber = pMeasurement->m_nPlaneNumber;
	int nSliceNumber = pMeasurement->m_nSliceNumber;
	switch (nPlaneNumber) 
	{
	case 1:
		m_nZPos = nSliceNumber;
		break;
	case 2:
		m_nXPos = nSliceNumber;
		break;
	case 3:
		m_nYPos = nSliceNumber;
		break;
	default:
		break;
	}

	theGeneralSwapData.m_pXOYView->UpdateImage(m_nZPos);
	theGeneralSwapData.m_pYOZView->UpdateImage(m_nYPos);
	theGeneralSwapData.m_pXOZView->UpdateImage(m_nXPos);

	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);

	theGeneralSwapData.m_pXOYView->UpdateWindow();
	theGeneralSwapData.m_pXOZView->UpdateWindow();
	theGeneralSwapData.m_pYOZView->UpdateWindow();	
}


void SeMPRView::UpdateView()
{
	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);

	theGeneralSwapData.m_pXOYView->UpdateWindow();
	theGeneralSwapData.m_pXOZView->UpdateWindow();
	theGeneralSwapData.m_pYOZView->UpdateWindow();	
}

void SeMPRView::Dcm2RGB(BYTE* pBmpData, BYTE* pDcmData, int nWidth, int bmpLineWidth, int nHeight, int nW, int nL)
{
	short* pHead = (short*)pDcmData;
	const int size = nWidth * nHeight;
	for(int i=0; i<nHeight; i++)
	{
		BYTE* pTmpHead = pBmpData + bmpLineWidth * i;
		for(int j=0; j<nWidth; j++)
		{
			//const short pixel = pHead[nWidth* (nHeight - i - 1) + j];
			const short pixel = *pHead++;
			if (pixel < nL - (nW>>1))
			{
				*pTmpHead++ = 0;
				*pTmpHead++ = 0;
				*pTmpHead++ = 0;
			}
			else if (pixel > nL + (nW>>1))
			{
				*pTmpHead++ = 255;
				*pTmpHead++ = 255;
				*pTmpHead++ = 255;
			}
			else
			{
				const BYTE data = ((pixel - (nL - (nW>>1))) << 8) / nW;
				*pTmpHead++ = data;
				*pTmpHead++ = data;
				*pTmpHead++ = data;
			}
		}
	}
}

void SeMPRView::ReplaceRGB(BYTE* pBmpData, BYTE* pMaskData, int nWidth, int bmpLineWidth, int nHeight, Color color)
{
	const int size = nWidth * nHeight;
	for(int i=0; i<nHeight; i++)
	{
		BYTE* pTmpHead = pBmpData + bmpLineWidth * i;
		for(int j=0; j<nWidth; j++)
		{
			if(*pMaskData++ != 0)
			{
				BYTE temp = *pTmpHead / 2;
				float strength = static_cast<float>(*pTmpHead) / 256.0f * 0.5f;
				*pTmpHead++ = static_cast<BYTE>(static_cast<float>(color.GetRed()) * strength) + temp;
				*pTmpHead++ = static_cast<BYTE>(static_cast<float>(color.GetGreen()) * strength) + temp;
				*pTmpHead++ = static_cast<BYTE>(static_cast<float>(color.GetBlue()) * strength) + temp;
			}
			else
			{
				*pTmpHead++;
				*pTmpHead++;
				*pTmpHead++;
			}
		}
	}
}



void SeMPRView::UpDownReplace(BYTE* pSrc, int nWidth, int nHeight)
{
	BYTE* pTmp = new BYTE[nWidth * nHeight];
	memcpy(pTmp, pSrc, nWidth * nHeight);
	for (int i=0; i<nHeight; i++)
	{
		for(int j=0; j<nWidth; j++)
		{
			//pHead[nWidth* (nHeight - i - 1) + j]
			pSrc[nWidth * i + j] = pTmp[nWidth* (nHeight - i - 1) + j];
// 			pSrc[(nWidth * i + j)*3 + 1] = pTmp[(nWidth* (nHeight - i - 1) + j)*3 + 1];
// 			pSrc[(nWidth * i + j)*3 + 2] = pTmp[(nWidth* (nHeight - i - 1) + j)*3 + 2];
		}
	}
	delete [] pTmp;
}

// void SeMPRView::CreateAndSaveBmp(CString filename, BYTE* pData, int nWidth, int nHeight)
// {
// 	CBitmap *bitmap;
// 	bitmap->CreateBitmap(nWidth, nHeight, 1, 24, )
// 	CImage img;
// 	img.Create(nWidth, nHeight, 24);
//  	BYTE* pRst = (BYTE*)img.GetBits();
// // 	memcpy(pRst, pData, 3 * nWidth * nHeight);
// 	img.Save(filename, Gdiplus::ImageFormatBMP);
// 	SaveBitmapToFile();
// }

void SeMPRView::CreateDcmPic()
{
	CDcmPicArray* m_pDcmPicArray = m_pDcmArray;
	int m_nWidth = m_nOriImageWidth;
	int m_nHeight = m_nOriImageHeight;
	if (!m_pDcmPicArray || m_nPlaneNum < 1)
		return;

	switch(m_nPlaneNum)
	{
	case 1:
		{
			m_nWidth = m_pDcmPicArray->GetDcmArray()[0]->GetHeight();
			m_nOffSet = m_pDcmPicArray->GetDcmArray()[0]->GetWidth()/2;
		}
		break;
	case 2:
		{

			m_nWidth = m_pDcmPicArray->GetDcmArray()[0]->GetWidth();
			m_nOffSet = m_pDcmPicArray->GetDcmArray()[0]->GetHeight()/2;

		}
		break;
	default:
		break;
	}
	m_nHeight = m_pDcmPicArray->GetZAxialPixelNumber();
	int nOriZ = m_pDcmPicArray->GetZDcmPicCount();
	short* sOri = new short[m_nWidth*nOriZ];
	for (int i = 0; i < nOriZ; i++)
	{
		theAppIVConfig.m_pILog->ProgressStepIt();
		m_pDcmPicArray->GetDcmArray()[i]->ReloadBuffer();
		switch(m_nPlaneNum)
		{
		case 1:
			m_pDcmPicArray->GetDcmArray()[i]->GetColumnData(&sOri[i*m_nWidth], m_nOffSet);
			break;
		case 2:
			m_pDcmPicArray->GetDcmArray()[i]->GetRowData(&sOri[i*m_nWidth], m_nOffSet);
			break;
		default:
			break;
		}
		m_pDcmPicArray->GetDcmArray()[i]->SetDataInMem(false);
		m_pDcmPicArray->GetDcmArray()[i]->ReleaseBuffer();
	}

	short* sInterport = new short[m_nWidth*m_nHeight];
	double dRate = (double)(nOriZ)/m_nHeight;
	for(int i = 0; i < m_nHeight; i++)
	{
		for (int j = 0; j < m_nWidth; j++)
		{
			double dY = i*dRate;
			if(dY < 0 || dY > nOriZ - 1)
				sInterport[i*m_nWidth +j] = 0;
			else
			{
				int nY1 = floor(dY);
				int nY2 = ceil(dY);
				double dY1 = dY - nY1;
				double dY2 = 1 - dY1;
				sInterport[i*m_nWidth + j] = dY2*sOri[nY1*m_nWidth + j] + dY1*sOri[nY2*m_nWidth + j];
			}
		}
	}
	CDcmPic* pDcmPic = m_pDcmPicArray->GetDcmArray()[0]->CloneDcmPic();
	pDcmPic->SetPixelData((BYTE*)sInterport, m_nWidth, m_nHeight);
	CImageViewerDoc* pDoc = (CImageViewerDoc*)GetDocument();
	pDoc->ReleaseSeries();
	pDoc->AddImage(pDcmPic, -1);
	Safe_Delete(sOri);
	Invalidate(FALSE);
}


void SeMPRView::OnLButtonDblClk(UINT nHitTest, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	static BOOL bShowAll = TRUE;
	if (bShowAll)
	{
		switch (m_nPlaneNum)
		{
		case 1:
			GetParent()->GetParent()->SendMessage(WM_3D_STATE, SHOW_TOP, 0);
			break;
		case 2:
			GetParent()->GetParent()->SendMessage(WM_3D_STATE, SHOW_FRONT, 0);
			break;
		case 3:
			GetParent()->GetParent()->SendMessage(WM_3D_STATE, SHOW_LEFT, 0);
			break;
		default:
			break;
		}
	}
	else
	{
		GetParent()->GetParent()->SendMessage(WM_3D_STATE, SHOW_ALL, 0);
	}
	bShowAll = !bShowAll;
	CImageViewerView::OnNcLButtonDblClk(nHitTest, point);
}

void SeMPRView::FloodFill()
{
	CPoint pt = m_ptPosition;
	int n = GetAxialPos();
	switch (m_nPlaneNum)
	{
	case 1:
		{
			m_vecROIData[m_vecROIData.size() - 1]->FloodFill(pt.x, pt.y, n);
			break;
		}
	case 2:
		{
			m_vecROIData[m_vecROIData.size() - 1]->FloodFill(n, pt.x, pt.y);
			break;
		}
	case 3:
		{
			m_vecROIData[m_vecROIData.size() - 1]->FloodFill(pt.x, n, pt.y);
			break;
		}
	default:
		{
			break;
		}
	}
	theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
	theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
	theGeneralSwapData.m_pYOZView->Invalidate(FALSE);
}


void SeMPRView::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CDcmPic* pDcm = (CDcmPic *)GetDisplayMgr()->GetFirstDispImage();
	if (pDcm != NULL)
		if (!m_bMbtnDown)
		{
			m_SeVisualMPR.SetWinLevel(pDcm->GetWinCenter(), pDcm->GetWinWidth());
			CImageViewerView::OnMouseMove(nFlags, point);
		}
		else
		{
			HCURSOR hCur = AfxGetApp()->LoadCursor(IDC_SHARPENCUR);
			::SetCursor(hCur);
			int offsetX = point.x - m_ptOld.x;
			if (offsetX != 0) 
				m_fSharp += offsetX * 0.01f;
			m_fSharp = m_fSharp > 5.0f ? 5.0f : m_fSharp;
			m_fSharp = m_fSharp < -5.0f ? -5.0f : m_fSharp;
			m_ptOld = point;

			theGeneralSwapData.m_pXOYView->UpdateImage();
			theGeneralSwapData.m_pYOZView->UpdateImage();
			theGeneralSwapData.m_pXOZView->UpdateImage();

			//theGeneralSwapData.m_pXOYView->Invalidate(FALSE);
			//theGeneralSwapData.m_pXOZView->Invalidate(FALSE);
			//theGeneralSwapData.m_pYOZView->Invalidate(FALSE);

			//theGeneralSwapData.m_pXOYView->UpdateWindow();
			//theGeneralSwapData.m_pXOZView->UpdateWindow();
			//theGeneralSwapData.m_pYOZView->UpdateWindow();
		}
}

void SeMPRView::OnLButtonDown(UINT nFlags, CPoint point)
{
// 	CDcmPic* pDcm = (CDcmPic *)GetDisplayMgr()->GetFirstDispImage();
// 	if (pDcm != NULL)
	CImageViewerView::OnLButtonDown(nFlags, point);
}

void SeMPRView::OnLButtonUp(UINT nFlags, CPoint point)
{
	CImageViewerView::OnLButtonUp(nFlags, point);
}



void SeMPRView::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CImageViewerView::OnRButtonDown(nFlags, point);
}


void SeMPRView::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	// only when m_bRButtonMove is false, can show menu.
	this->m_bRButtonMove = true;
	CImageViewerView::OnRButtonUp(nFlags, point);
}




// Save the bitmap to a bmp file  
void SeMPRView::SaveBitmapToFile(BYTE* pBitmapBits, LONG lWidth, LONG lHeight, WORD wBitsPerPixel, const unsigned long& padding_size, LPCTSTR lpszFileName)
{
	// Some basic bitmap parameters  
	unsigned long headers_size = sizeof( BITMAPFILEHEADER ) +  
		sizeof( BITMAPINFOHEADER );  

	unsigned long pixel_data_size = lHeight * ( ( lWidth * ( wBitsPerPixel / 8 ) ) + padding_size );  

	BITMAPINFOHEADER bmpInfoHeader = {0};  

	// Set the size  
	bmpInfoHeader.biSize = sizeof(BITMAPINFOHEADER);  

	// Bit count  
	bmpInfoHeader.biBitCount = wBitsPerPixel;  

	// Use all colors  
	bmpInfoHeader.biClrImportant = 0;  

	// Use as many colors according to bits per pixel  
	bmpInfoHeader.biClrUsed = 0;  

	// Store as un Compressed  
	bmpInfoHeader.biCompression = BI_RGB;  

	// Set the height in pixels  
	bmpInfoHeader.biHeight = lHeight;  

	// Width of the Image in pixels  
	bmpInfoHeader.biWidth = lWidth;  

	// Default number of planes  
	bmpInfoHeader.biPlanes = 1;  

	// Calculate the image size in bytes  
	bmpInfoHeader.biSizeImage = pixel_data_size;  

	BITMAPFILEHEADER bfh = {0};  

	// This value should be values of BM letters i.e 0x4D42  
	// 0x4D = M 0×42 = B storing in reverse order to match with endian  
	bfh.bfType = 0x4D42;  
	//bfh.bfType = 'B'+('M' << 8); 

	// <<8 used to shift ‘M’ to end  */  

	// Offset to the RGBQUAD  
	bfh.bfOffBits = headers_size;  

	// Total size of image including size of headers  
	bfh.bfSize =  headers_size + pixel_data_size;  

	// Create the file in disk to write  
	HANDLE hFile = CreateFile( lpszFileName,  
		GENERIC_WRITE,  
		0,  
		NULL,  
		CREATE_ALWAYS,  
		FILE_ATTRIBUTE_NORMAL,  
		NULL );  

	// Return if error opening file  
	if( !hFile ) return;  

	DWORD dwWritten = 0;  

	// Write the File header  
	WriteFile( hFile,  
		&bfh,  
		sizeof(bfh),  
		&dwWritten ,  
		NULL );  

	// Write the bitmap info header  
	WriteFile( hFile,  
		&bmpInfoHeader,  
		sizeof(bmpInfoHeader),  
		&dwWritten,  
		NULL );  

	// Write the RGB Data  
	WriteFile( hFile,  
		pBitmapBits,  
		bmpInfoHeader.biSizeImage,  
		&dwWritten,  
		NULL );  

	// Close the file handle  
	CloseHandle( hFile );  
}

BYTE* LoadBMP ( int* width, int* height, unsigned long* size, LPCTSTR bmpfile )
{
	BITMAPFILEHEADER bmpheader;
	BITMAPINFOHEADER bmpinfo;
	// value to be used in ReadFile funcs
	DWORD bytesread;
	// open file to read from
	HANDLE file = CreateFile ( bmpfile , 
		GENERIC_READ, 
		FILE_SHARE_READ,
		NULL, 
		OPEN_EXISTING, 
		FILE_FLAG_SEQUENTIAL_SCAN, 
		NULL );
	if ( NULL == file )
		return NULL;

	if ( ReadFile ( file, &bmpheader, sizeof ( BITMAPFILEHEADER ), &bytesread, NULL ) == false )
	{
		CloseHandle ( file );
		return NULL;
	}

	// Read bitmap info
	if ( ReadFile ( file, &bmpinfo, sizeof ( BITMAPINFOHEADER ), &bytesread, NULL ) == false )
	{
		CloseHandle ( file );
		return NULL;
	}

	// check if file is actually a bmp
	if ( bmpheader.bfType != 'MB' )
	{
		CloseHandle ( file );
		return NULL;
	}

	// get image measurements
	*width   = bmpinfo.biWidth;
	*height  = abs ( bmpinfo.biHeight );

	// Check if bmp iuncompressed
	if ( bmpinfo.biCompression != BI_RGB )
	{
		CloseHandle ( file );
		return NULL;
	}

	// Check if we have 24 bit bmp
	if ( bmpinfo.biBitCount != 24 )
	{
		CloseHandle ( file );
		return NULL;
	}

	// create buffer to hold the data
	*size = bmpheader.bfSize - bmpheader.bfOffBits;
	BYTE* Buffer = new BYTE[ *size ];
	// move file pointer to start of bitmap data
	SetFilePointer ( file, bmpheader.bfOffBits, NULL, FILE_BEGIN );
	// read bmp data
	if ( ReadFile ( file, Buffer, *size, &bytesread, NULL ) == false )
	{
		delete [] Buffer;
		CloseHandle ( file );
		return NULL;
	}

	// everything successful here: close file and return buffer

	CloseHandle ( file );

	return Buffer;
}

std::unique_ptr<BYTE[]> CreateNewBuffer( unsigned long& padding,
	BYTE* pmatrix, 
	const int& width,
	const int& height )
{
	padding = ( 4 - ( ( width * 3 ) % 4 ) ) % 4;  
	int scanlinebytes = width * 3;
	int total_scanlinebytes = scanlinebytes + padding;
	long newsize = height * total_scanlinebytes;
	std::unique_ptr<BYTE[]> newbuf( new BYTE[ newsize ] );

	// Fill new array with original buffer, pad remaining with zeros
	std::fill( &newbuf[ 0 ], &newbuf[ newsize ], 0 );
	long bufpos = 0;   
	long newpos = 0;
	for ( int y = 0; y < height; y++ )
	{
		for ( int x = 0; x < 3 * width; x+=3 )
		{
			// Determine positions in original and padded buffers
			bufpos = y * 3 * width + ( 3 * width - x );     
			newpos = ( height - y - 1 ) * total_scanlinebytes + x; 

			// Swap R&B, G remains, swap B&R
			newbuf[ newpos ] = pmatrix[ bufpos + 2 ]; 
			newbuf[ newpos + 1 ] = pmatrix[ bufpos + 1 ];  
			newbuf[ newpos + 2 ] = pmatrix[ bufpos ];       
		}
	}

	return newbuf;
}

// int main()
// {
// 	int imageWidth = 0;
// 	int imageHeight = 0;
// 	unsigned long imageSize = 0;
// 	unsigned long padding = 0;
// 
// 	// Load the bitmap file, amd put its data part into the BYTE array
// 	BYTE* bytes = LoadBMP( &imageWidth, &imageHeight, &imageSize, "C:\\MyStuff\\shaunak.BMP" );
// 	std::reverse( bytes, bytes + imageSize );
// 
// 	// Determine amount of padding required, if any, and create a new BYTE array from this
// 	std::unique_ptr<BYTE[]> newbuf2 = CreateNewBuffer( padding, bytes, imageWidth, imageHeight );
// 
// 	// Use the new array data to create the new bitmap file
// 	SaveBitmapToFile( (BYTE*) &newbuf2[ 0 ],  
// 		imageWidth,  
// 		imageHeight,  
// 		24,  
// 		padding,
// 		"C:\\MyStuff\\new_image.bmp" );  
// 
// 	return 0;
// }

void SeMPRView::OnMButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_bMbtnDown = TRUE;
	m_ptOld = point;
	HCURSOR hCur = AfxGetApp()->LoadCursor(IDC_SHARPENCUR);
	::SetCursor(hCur);
	// CImageViewerView::OnMButtonDown(nFlags, point);
}


void SeMPRView::OnMButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_bMbtnDown = FALSE;
	HCURSOR hCur = AfxGetApp()->LoadCursor(IDC_MOVECUR);
	::SetCursor(hCur);
	// CImageViewerView::OnMButtonUp(nFlags, point);
}

void SeMPRView::Sharp(CDcmPic* pImg, float fSharp)
{
	if (abs(fSharp - 1.0f) < 0.01f) 
		return;
	const int nWidth = pImg->GetWidth();
	const int nHeight = pImg->GetHeight();
	short* pTempData = new short[nWidth * nHeight];
	short* pOriData = (short*)pImg->GetData();
	memset(pTempData, 0, sizeof(short) * nWidth * nHeight);
	memcpy(pTempData, pOriData, sizeof(short) * nWidth * nHeight);
	int n = 3;
	int model[9] = {0,-1, 0, -1, 5, -1, 0, -1, 0};
	for (int j=1; j<nHeight-1; j++)
	{
		for (int i=1; i<nWidth-1; i++)
		{
			int temp = 0;
			temp += static_cast<int>(pOriData[(j-1) * nWidth + (i-1)]) * model[0];
			temp += static_cast<int>(pOriData[(j-1) * nWidth + (i  )]) * model[1];
			temp += static_cast<int>(pOriData[(j-1) * nWidth + (i+1)]) * model[2];
			temp += static_cast<int>(pOriData[(j  ) * nWidth + (i-1)]) * model[3];
			temp += static_cast<int>(pOriData[(j  ) * nWidth + (i  )]) * model[4];
			temp += static_cast<int>(pOriData[(j  ) * nWidth + (i+1)]) * model[5];
			temp += static_cast<int>(pOriData[(j+1) * nWidth + (i-1)]) * model[6];
			temp += static_cast<int>(pOriData[(j+1) * nWidth + (i  )]) * model[7];
			temp += static_cast<int>(pOriData[(j+1) * nWidth + (i+1)]) * model[8];

			pTempData[j * nWidth + i] = static_cast<short>((static_cast<short>(temp) - pTempData[j * nWidth + i]) * fSharp) + pTempData[j * nWidth + i];
			//if (pTempData[j * nWidth + i] < 0)
			//	pTempData[j * nWidth + i] = 0; 
		}
	}
	pImg->SetPixelData((BYTE*) pTempData, nWidth, nHeight);
}

void SeMPRView::ExportSharpImages()
{
	const float fSharp = m_fSharp;
	const auto nSize = m_pDcmArray->GetDcmArray().size();
	const int  nWidth = m_pDcmArray->GetDcmArray()[0]->GetWidth();
	const int  nHeight = m_pDcmArray->GetDcmArray()[0]->GetHeight();
	CString strFolder = theGeneralSwapData.m_csFolderPath;
	int pos = strFolder.ReverseFind('\\');
	CString newstrFolder = strFolder.Left(pos) + "\\Sharp" + strFolder.Right(strFolder.GetLength() - pos - 1);
	CSeToolKit::CreateDeepDirectory(newstrFolder);
	theAppIVConfig.m_pILog->ProgressInit(static_cast<int>(nSize));
	for (auto i=0; i<nSize; i++)
	{

		CDcmPic* pDcm = m_pDcmArray->GetDcmArray()[i]->CloneDcmPic();
		Sharp(pDcm, fSharp);
		CString csName;
		CString csIndex;
		int nIndex = i;
		csIndex.Format("%d", nIndex);
		if (nIndex<10)
		{
			csName = "Hiscan_0000" + csIndex;
		}
		else if (nIndex>=10 && nIndex<100)
		{
			csName = "Hiscan_000" + csIndex;
		}
		else if (nIndex>=100)
		{
			csName = "Hiscan_00" + csIndex;
		}
		else
			csName = "Hiscan_0" + csIndex;

		CString strFile = newstrFolder + "\\" + csName + _T(".dcm");
		CDcmElement* pEle = pDcm->GetDcmElemList().FindElem(0x0020, 0x0013);
		pEle->ValueAdd(csName);
		pDcm->ExportDcm(strFile);
		Safe_Delete(pDcm);
		theAppIVConfig.m_pILog->ProgressStepIt();
	}
	MessageBoxTimeout(NULL, _T("         导出文件完成！       "), _T("提示"), MB_ICONINFORMATION, 0, 300);
	theAppIVConfig.m_pILog->ProgressClose();
}
