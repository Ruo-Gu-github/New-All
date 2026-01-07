// SeFattyZSelectView.cpp : Êµï¿½ï¿½ï¿½Ä¼ï¿½
//

#include "stdafx.h"
#include "SeFattyZSelectView.h"
#include "SeROIData.h"
#include "FatSeprater.h"
#include "ExampleModuleSwapData.h"

// CSeFattyZSelectView

std::vector<CSeROIData*> CSeFattyZSelectView::m_vecROIData;

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


IMPLEMENT_DYNCREATE(CSeFattyZSelectView, CImageViewerView)

CSeFattyZSelectView::CSeFattyZSelectView()
{
	m_pDcmPicArray = NULL;
	m_nPlaneNum = 0;
	m_nStartEnd = 1;
	m_nWidth = m_nOffSet = 0;
	m_bMPR = false;
	m_nFatMin = 0;
	m_nFatMax  = 0;
	m_nLungMin = 0;
	m_nLungMax = 0;
	m_nBoneMin = 0;
	m_nBoneMax = 0;
	m_pFatInside = NULL;
	m_pFatOutside = NULL;
	m_pLungInside = NULL;
	m_pLungOutside = NULL;
	m_pLung = NULL;
	m_pBone = NULL;
}

CSeFattyZSelectView::~CSeFattyZSelectView()
{
}

BEGIN_MESSAGE_MAP(CSeFattyZSelectView, CImageViewerView)
	ON_WM_VSCROLL()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()


// CSeFattyZSelectView ï¿½ï¿½Í¼

Bitmap* CSeFattyZSelectView::CreatePng(int nPlane, int nLayer, CSeROIData* data, int nWidth, int nHeight)
{
	BYTE* pData = data->GetSliceData(nLayer, nPlane);
	if (pData == NULL)
		return NULL;
	BYTE* pFirstValue = pData;
	pData = pFirstValue;
	PixelFormat pixelFormat = PixelFormat32bppARGB;
	Bitmap myBmp(nWidth, nHeight, pixelFormat);
	BitmapData myData;
	Rect rect(0, 0, nWidth, nHeight);
	myBmp.LockBits(&rect, ImageLockModeWrite, pixelFormat, &myData);
	int nStride = myData.Stride;
	DWORD* pMyData = (DWORD*)myData.Scan0; 
	DWORD r = GetRValue(data->GetColor());
	DWORD g = GetGValue(data->GetColor());
	DWORD b = GetBValue(data->GetColor());

	DWORD rgba = (data->GetAlpha() << 24) | (r << 16) | (g << 8) | b  ;

	for(int i=0; i<nHeight; i++)
	{
		for(int j=0; j<nWidth; j++)
		{
			BYTE tmp = *pData++;
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

void CSeFattyZSelectView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: ï¿½Ú´ï¿½ï¿½ï¿½ï¿½Ó»ï¿½ï¿½Æ´ï¿½ï¿½ï¿½
	CRect			rtClient;
	GetClientRect(&rtClient);
	CZKMemDC			MemDC(pDC, rtClient, RGB(0,0,0));

	CImageViewerView::OnDraw(&MemDC);


	CImageBase* pImg = GetDisplayMgr()->GetFirstDispImage();
	Graphics gc(MemDC.GetSafeHdc());
	if (pImg != NULL && m_nSelect != 0)
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
			min = m_nBoneMin;
			max = m_nBoneMax;
			color = RGB(255, 0, 0);
		}
		Bitmap* pBmp = GreatePng(pImg, min, max, color , 64);
		CRect rect = pImg->GetDrawRect();
		Image* pMyImg = pBmp->GetThumbnailImage(rect.Width(), rect.Height());
		gc.DrawImage(pMyImg, rect.left, rect.top, rect.Width(), rect.Height());
		Safe_Delete(pBmp);
		Safe_Delete(pMyImg);
	}

	for (int i=0; i<m_vecROIData.size(); i++)
	{
		if (m_vecROIData[i]->IsVisible())
		{
			Bitmap* pBmp = CreatePng(m_nPlaneNum, m_nCurrentFrame, m_vecROIData[i], pImg->GetWidth(), pImg->GetHeight());
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

	CString strInfo;
	if (m_nPlaneNum == 1)
	{
		strInfo.Format(_T("%d\nÖ¬·¾ãÐÖµ%d - %d\n·Î²¿ãÐÖµ%d - %d\n¹Ç÷ÀãÐÖµ%d - %d"),
			m_nCurrentFrame,
			m_nFatMin, m_nFatMax,
			m_nLungMin, m_nLungMax,
			m_nBoneMin, m_nBoneMax);
	}
	else
	{
		strInfo.Format(_T("%d"),m_nCurrentFrame);
	}


	// ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½É«ï¿½Í±ï¿½ï¿½ï¿½Í¸ï¿½ï¿½
	MemDC.SetTextColor(RGB(255, 255, 255)); // ï¿½ï¿½É«ï¿½ï¿½ï¿½ï¿½
	MemDC.SetBkMode(TRANSPARENT);

	// ï¿½è¶¨ï¿½Ä±ï¿½ï¿½ï¿½Ê¾ï¿½ï¿½ï¿½ï¿½
	CRect textRect(10, 10, 300, 100); // ï¿½ß¶ï¿½ï¿½Êµï¿½ï¿½ï¿½ï¿½ï¿½

	// Ò»ï¿½ï¿½ï¿½Ô»ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ä±ï¿½
	MemDC.DrawText(strInfo, textRect, DT_LEFT | DT_TOP | DT_WORDBREAK);

}


// CSeFattyZSelectView ï¿½ï¿½ï¿?

#ifdef _DEBUG
void CSeFattyZSelectView::AssertValid() const
{
	CImageViewerView::AssertValid();
}

#ifndef _WIN32_WCE
void CSeFattyZSelectView::Dump(CDumpContext& dc) const
{
	CImageViewerView::Dump(dc);
}
#endif
#endif //_DEBUG


// CSeFattyZSelectView ï¿½ï¿½Ï¢ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
void CSeFattyZSelectView::OnInitialUpdate()
{
	CImageViewerView::OnInitialUpdate();
	SendMessage(WM_COMMAND, IDM_IMAGEVIEWER_LAYOUT_1X1);
	// TODO: ï¿½Ú´ï¿½ï¿½ï¿½ï¿½ï¿½×¨ï¿½Ã´ï¿½ï¿½ï¿½ï¿?ï¿½ï¿½ï¿½ï¿½Ã»ï¿½ï¿½ï¿?
}

void CSeFattyZSelectView::SetDcmArray( CDcmPicArray* pDcmPicArray )
{
	m_pDcmPicArray = pDcmPicArray;
	m_SeVisualMPR.SetDcmArray(pDcmPicArray);	
}

void CSeFattyZSelectView::Reset()
{
	for (int i=0; i<m_vecROIData.size(); i++) {
		BYTE * pdata = m_vecROIData[i]->GetData();
		delete [] pdata;
	}
	m_vecROIData.clear();
	m_pFatInside = NULL;
	m_pFatOutside = NULL;
	m_pLungInside = NULL;
	m_pLungOutside = NULL;
	m_pLung = NULL;
	m_pBone = NULL;
}

void CSeFattyZSelectView::UpdateImage()
{
	CDcmPic* pDcm = NULL;
	pDcm = m_SeVisualMPR.GetMPRImage();

	if (pDcm != NULL)
	{
		CImageViewerDoc* pDoc = (CImageViewerDoc*)GetDocument();
		pDoc->ReleaseSeries();
		pDoc->AddImage(pDcm, 0);
		Invalidate(false);
	}
}

void CSeFattyZSelectView::UpdateImage(int nPos)
{
	CDcmPic* pDcm = NULL;
	pDcm = m_SeVisualMPR.GetMPRImage(nPos);
	
	m_nCurrentFrame = nPos;
	if (pDcm != NULL)
	{
		CImageViewerDoc* pDoc = (CImageViewerDoc*)GetDocument();
		pDoc->ReleaseSeries();
		pDoc->AddImage(pDcm, 0);
		Invalidate(false);
	}
}

void CSeFattyZSelectView::SetMPRMode( bool bMPR )
{
	m_bMPR = bMPR;
}

void CSeFattyZSelectView::SetPlaneNum( int nPlaneNum )
{
	m_SeVisualMPR.SetPlaneNum(nPlaneNum);
	m_nPlaneNum = nPlaneNum;
}

void CSeFattyZSelectView::SetStartEnd( int nStartOrEnd )
{
	m_nStartEnd = nStartOrEnd;
}

void CSeFattyZSelectView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: ï¿½Ú´ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ï¢ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½/ï¿½ï¿½ï¿½ï¿½ï¿½Ä¬ï¿½ï¿½Ö?
	CImageViewerView::OnVScroll(nSBCode, nPos, pScrollBar);
}

BOOL CSeFattyZSelectView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
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

	UpdateImage(m_nCurrentFrame);
	Invalidate(FALSE);
	UpdateWindow();
	// TODO: ï¿½Ú´ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ï¢ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½/ï¿½ï¿½ï¿½ï¿½ï¿½Ä¬ï¿½ï¿½Ö?
	return CImageViewerView::OnMouseWheel(nFlags, zDelta, pt);
}

Bitmap* CSeFattyZSelectView::GreatePng(CImageBase* pImg, int nMin, int nMax, COLORREF color, DWORD alpha)
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

void CSeFattyZSelectView::SetInfo(int n1, int n2, DWORD n3, int n4, int n5)
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

void CSeFattyZSelectView::SeprateFat()
{
	if(m_nFatMin == 0 ||  m_nFatMax == 0)
	{
		AfxMessageBox("ÇëÉèÖÃÖ¬·¾ãÐÖµ");
		return;
	}
	int width = theExampleModuleSwapData.m_nWidth;
	int height = theExampleModuleSwapData.m_nHeight;
	int depth = theExampleModuleSwapData.m_nLength;

	auto result = FatSeprater::SeprateFat(
		m_pDcmPicArray,
		width,
		height,
		depth,
		m_nCurrentFrame,
		m_nFatMin,
		m_nFatMax
		);
	BYTE* visceralFatMaskFinal = result.first;
	BYTE* finalSubcutaneousFat = result.second;

	CSeROIData* oldROIs[2] = { m_pFatInside, m_pFatOutside };
	for (std::vector<CSeROIData*>::iterator it = m_vecROIData.begin(); it != m_vecROIData.end(); ) {
		bool needDelete = false;
		for (int i = 0; i < 2; ++i) {
			if (*it == oldROIs[i] && oldROIs[i] != NULL) {
				needDelete = true;
				break;
			}
		}
		if (needDelete) {
			BYTE* pData = (*it)->GetData();
			if (pData) {
				delete[] pData;
			}
			delete *it;
			it = m_vecROIData.erase(it);
		} else {
			++it;
		}
	}

	CSeROIData* roiSubcutaneousFat = new CSeROIData(finalSubcutaneousFat, width, height, depth, RGB(255, 0, 0), 64, TRUE);
	m_vecROIData.push_back(roiSubcutaneousFat);

	CSeROIData* roiVisceralFat = new CSeROIData(visceralFatMaskFinal, width, height, depth, RGB(0, 255, 0), 64, TRUE);
	m_vecROIData.push_back(roiVisceralFat);

	m_pFatInside = roiSubcutaneousFat;
	m_pFatOutside = roiVisceralFat;

	delete [] visceralFatMaskFinal;
	delete [] finalSubcutaneousFat;

}

void CSeFattyZSelectView::SeprateLung()
{
	if(m_nFatMin == 0 || m_nFatMax == 0  || m_nLungMax == 0 || m_nBoneMin == 0 ||  m_nBoneMax ==  0)
	{
		AfxMessageBox("ÇëÉèÖÃÖ¬·¾, ·Î²¿, ¹Ç÷ÀµÄãÐÖµ");
		return;
	}

	int width = theExampleModuleSwapData.m_nWidth;
	int height = theExampleModuleSwapData.m_nHeight;
	int depth = theExampleModuleSwapData.m_nLength;

	auto result = FatSeprater::SeprateLung(
		m_pDcmPicArray,
		width,
		height,
		depth,
		m_nFatMin,
		m_nFatMax,
		m_nLungMin,
		m_nLungMax,
		m_nBoneMin,
		m_nBoneMax
		);

	BYTE* visceralFatMaskFinal = result[0];
	BYTE* finalSubcutaneousFat = result[1];
	BYTE* lungMask = result[2];
	BYTE* boneMask = result[3];

	CSeROIData* oldROIs[4] = { m_pLungInside, m_pLungOutside, m_pLung, m_pBone };
	for (std::vector<CSeROIData*>::iterator it = m_vecROIData.begin(); it != m_vecROIData.end(); ) {
		bool needDelete = false;
		for (int i = 0; i < 4; ++i) {
			if (*it == oldROIs[i] && oldROIs[i] != NULL) {
				needDelete = true;
				break;
			}
		}
		if (needDelete) {
			BYTE* pData = (*it)->GetData();
			if (pData) {
				delete[] pData;
			}
			delete *it;
			it = m_vecROIData.erase(it);
		} else {
			++it;
		}
	}

	CSeROIData* roiSubcutaneousFat = new CSeROIData(finalSubcutaneousFat, width, height, depth, RGB(0, 0, 255), 64, TRUE);
	m_vecROIData.push_back(roiSubcutaneousFat);

	CSeROIData* roiVisceralFat = new CSeROIData(visceralFatMaskFinal, width, height, depth, RGB(255, 255, 0), 64, TRUE);
	m_vecROIData.push_back(roiVisceralFat);

	CSeROIData* roiLung = new CSeROIData(lungMask, width, height, depth, RGB(0, 255, 255), 64, TRUE);
	m_vecROIData.push_back(roiLung);

	CSeROIData* roiBone = new CSeROIData(boneMask, width, height, depth, RGB(255, 0, 255), 64, TRUE);
	m_vecROIData.push_back(roiBone);

	m_pLungInside = roiSubcutaneousFat;
	m_pLungOutside = roiVisceralFat;
	m_pLung = roiLung;
	m_pBone = roiBone;

	delete [] visceralFatMaskFinal;
	delete [] finalSubcutaneousFat;
	delete [] lungMask;
	delete [] boneMask;
}

void CSeFattyZSelectView::ExpertFats()
{
	CString strFolder = theExampleModuleSwapData.m_csFolderPath; // µ¼³öÄ¿Â¼£¬¿É¸ù¾ÝÐèÒªÐÞ¸Ä
	int width = theExampleModuleSwapData.m_nWidth;
	int height = theExampleModuleSwapData.m_nHeight;
	int depth = theExampleModuleSwapData.m_nLength;

	struct MaskInfo {
		CSeROIData* pROI;
		CString name;
	} masks[6] = {
		{ m_pFatInside,   _T("¸¹Ç»ÄÚÔàÖ¬·¾") },
		{ m_pFatOutside,  _T("¸¹Ç»Æ¤ÏÂÖ¬·¾") },
		{ m_pLungInside,  _T("·Î²¿ÄÚÔàÖ¬·¾") },
		{ m_pLungOutside, _T("·Î²¿Æ¤ÏÂÖ¬·¾") },
		{ m_pLung,        _T("·Î²¿") },
		{ m_pBone,        _T("¹Ç÷À") }
	};

	for (int i = 0; i < 6; ++i) {
		if (masks[i].pROI != NULL) {
			CString strFile = strFolder + "\\" + masks[i].name + _T(".dat");
			BYTE* pMask = masks[i].pROI->GetData();
			std::ofstream ouF(strFile, std::ofstream::binary);
			if (ouF.is_open()) {
				ouF.write(reinterpret_cast<const char*>(pMask), width * height * depth);
				ouF.close();
			}
		}
	}

	MessageBoxTimeout(NULL, _T("         µ¼³öMaskÍê³É£¡       "), _T("ÌáÊ¾"), MB_ICONINFORMATION, 0, 1000);
}