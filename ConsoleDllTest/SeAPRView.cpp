// SeAPRView.cpp : ??????
//


#include "stdafx.h"
#include "BoneDensitySwapData.h"
#include "SeAPRView.h"

double				SeAPRView::m_dXALLTrans = 0;
double				SeAPRView::m_dYALLTrans = 0;
double				SeAPRView::m_dZALLTrans = 0;

int					SeAPRView::m_nXStart	= 0;
int					SeAPRView::m_nXEnd	= 0;
int					SeAPRView::m_nYStart = 0;
int					SeAPRView::m_nYEnd	= 0;
int					SeAPRView::m_nZStart = 0;
int					SeAPRView::m_nZEnd	= 0;


CEvent				Seg_eventDrawdcm(FALSE, TRUE);
CEvent				Seg_eventReleasedcm(FALSE, TRUE);

// SeAPRView

IMPLEMENT_DYNCREATE(SeAPRView, CImageViewerView)

SeAPRView::SeAPRView()
{
	m_hDrawDcm = Seg_eventDrawdcm;
	m_hReleaseDcm = Seg_eventReleasedcm;
	m_fZRotateLast = 0;
	m_nSelectedTool = 0;
}

SeAPRView::~SeAPRView()
{
}

BEGIN_MESSAGE_MAP(SeAPRView, CImageViewerView)
	ON_WM_MOUSEWHEEL()
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()


// SeAPRView ???

void SeAPRView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	CRect			rtClient;
	GetClientRect(&rtClient);
	WaitForSingleObject(m_hDrawDcm, INFINITE);      //???????????????????dcm??¦Ä??????releaseseries??????dcm??ãè???????read??????????????????     
	Seg_eventReleasedcm.ResetEvent();
	CPen pen(PS_SOLID, 1, afxGlobalData.clrBarShadow);

	CZKMemDC MemDC(pDC, RGB(255,0,0));
	CImageViewerView::OnDraw(&MemDC);
	if (m_nSelectedTool == 1)
	{
		APRDraw(&MemDC);
	}
	else if (m_nSelectedTool == 2)
	{
		APRRectDraw(&MemDC);
	}
	Seg_eventReleasedcm.SetEvent();
}


// SeAPRView ???

#ifdef _DEBUG
void SeAPRView::AssertValid() const
{
	CImageViewerView::AssertValid();
}

#ifndef _WIN32_WCE
void SeAPRView::Dump(CDumpContext& dc) const
{
	CImageViewerView::Dump(dc);
}

void SeAPRView::SetDcmArray(CDcmPicArray* pDcmArray)
{
	m_pDcmArray = pDcmArray;
	m_SeVisualAPR.SetDcmArray(pDcmArray);
}

void SeAPRView::UpdateImage()
{
	CDcmPic* pDcm = NULL;
	//pDcm = m_SeVisualAPR.GetAPRImage(m_RotateInfo.dModelView, m_dXALLTrans, m_dYALLTrans, m_dZALLTrans, m_bMIPDownSample);
// 	double db[16] = {1.0, 0.0, 0.0, 0.0,
// 					0.0, 1.0, 0.0, 0.0,
// 					0.0, 0.0, 1.0, 0.0,
// 					0.0, 0.0, 0.0, 0.0};
	pDcm = m_SeVisualAPR.GetAPRImage(m_RotateInfo.dModelView, 0.0, 0.0, 0.0, TRUE);
	if (pDcm != NULL)
	{
		DrawImage(pDcm);
	}
	
}

void SeAPRView::UpdateImage(int nPlaneNum)
{
	CDcmPic* pDcm = NULL;
	MprRotateInfo info = theBoneDensitySwapData.m_pXOYView->m_RotateInfo; 
	double dX, dY, dZ;
	switch (nPlaneNum)
	{
	case 1:
		{
			dX = m_dZALLTrans*info.dWorldX_z;
			dY = m_dZALLTrans*info.dWorldY_z;
			dZ = m_dZALLTrans*info.dWorldZ_z;
			break;
		}
	case 2:
		{
			dX = m_dXALLTrans*info.dWorldX_x;
			dY = m_dXALLTrans*info.dWorldY_x;
			dZ = m_dXALLTrans*info.dWorldZ_x;
			break;
		}
	case 3:
		{
			dX = m_dYALLTrans*info.dWorldX_y;
			dY = m_dYALLTrans*info.dWorldY_y;
			dZ = m_dYALLTrans*info.dWorldZ_y;
			break;
		}
	default:
		break;
	}
	//pDcm = m_SeVisualAPR.GetAPRImage(m_RotateInfo.dModelView, dX, dY, dZ, m_bMIPDownSample);

// 	double db[16] = {1.0, 0.0, 0.0, 0.0,
// 		0.0, 1.0, 0.0, 0.0,
// 		0.0, 0.0, 1.0, 0.0,
// 		0.0, 0.0, 0.0, 0.0};
	pDcm = m_SeVisualAPR.GetAPRImage(m_RotateInfo.dModelView, dX, dY, dZ, TRUE);

	//pDcm = m_SeVisualAPR.GetAPRImage(m_RotateInfo.dModelView, dX, dY, dZ);
	if (pDcm != NULL)
	{
		DrawImage(pDcm);
	}
}


void SeAPRView::DrawImage(CDcmPic* pDcm)
{
	SeVisualLibDoc* pDoc = (SeVisualLibDoc*)GetDocument();
	WaitForSingleObject(m_hReleaseDcm, INFINITE);  //???????????????????dcm??¦Ä??????releaseseries??????dcm??ãè???????read??????????????????
	Seg_eventDrawdcm.ResetEvent();
	pDoc->ReleaseSeries();
	pDoc->AddImage(pDcm, 0);					   //???AddImage?????????????????true????????????¨²????????????pDcm????????????delete
	pDcm->SetDrawEdgeRectFlag(false);
	Seg_eventDrawdcm.SetEvent();
}

#endif
#endif //_DEBUG


// SeAPRView ??????????


void SeAPRView::OnInitialUpdate()
{
	CImageViewerView::OnInitialUpdate();
	SetLayoutFormat(1,1);
	AfxBeginThread(__UpdateImage, this);
	Seg_eventDrawdcm.SetEvent();
	Seg_eventReleasedcm.SetEvent();
}

UINT SeAPRView::__UpdateImage(void* lpVoid)
{
	SeAPRView*  pThis = (SeAPRView*)lpVoid;

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
			pThis->UpdateImage();
			break;
		case WAIT_OBJECT_0 + 1:
			bDone = true;
			break;
		}
	}

	return 0;
}


void SeAPRView::SetAPRTool()
{
	SetMouseTool(MT_APRTOOL);
	m_nSelectedTool = 1;
}

void SeAPRView::SetAPRRectTool()
{
	SetMouseTool(MT_APRRECT);
	m_nSelectedTool = 2;
}

void SeAPRView::InitView()
{
	m_dZALLTrans = m_dYALLTrans = m_dXALLTrans = 0;
	m_nXStart = m_nXEnd = m_nYStart = m_nYEnd = m_nZStart = m_nZEnd = 0;
	CDcmPic* pDcm = NULL;
	//pDcm = m_SeVisualAPR.GetAPRImage(m_RotateInfo.dModelView, 0, 0, 0, m_bMIPDownSample);
	pDcm = m_SeVisualAPR.GetAPRImage(m_RotateInfo.dModelView,  0, 0, 0, m_bMIPDownSample);
	if(pDcm != NULL)
		theBoneDensitySwapData.m_nRotateDcmSideLength = pDcm->GetWidth();

	switch(m_nPlaneNum)
	{
	case 1:
		{
			m_RotateInfo.ResetAPR(1);
			UpdateImage();
			break;
		}
	case 2:
		{
			m_RotateInfo.ResetAPR(1);
			m_RotateInfo = m_SeVisualRotate.ChangeRotateAngle_Current(false, 0, 90, m_RotateInfo);
			UpdateImage();
			break;
		}
	case 3:
		{
			m_RotateInfo.ResetAPR(1);
			m_RotateInfo = m_SeVisualRotate.ChangeRotateAngle_Current(false, 90, 0, m_RotateInfo);
			UpdateImage();
			break;
		}
	default:
		break;
	}
}

void SeAPRView::UpdateOtherView(MprRotateInfo rotateInfor)
{
	MprRotateInfo info;
	switch (m_nPlaneNum)
	{
	case 1:
		{
			info = rotateInfor;
			break;
		}
	case 2:
		{
			info = theBoneDensitySwapData.m_pYOZView->GetRotate()->ChangeRotateAngle_Current(false, 0, -90, rotateInfor);
			break;
		}
	case 3:
		{
			info = theBoneDensitySwapData.m_pXOZView->GetRotate()->ChangeRotateAngle_Current(false, -90, 0, rotateInfor);
			break;
		}
	default:
		break;
	}


	theBoneDensitySwapData.m_pXOYView->SetRotateInfor(info);
	theBoneDensitySwapData.m_pXOYView->UpdateImage(1);

	theBoneDensitySwapData.m_pXOZView->SetRotateInfor(theBoneDensitySwapData.m_pXOZView->GetRotate()->ChangeRotateAngle_Current(false, 0, 90, info));
	theBoneDensitySwapData.m_pXOZView->UpdateImage(2);

	theBoneDensitySwapData.m_pYOZView->SetRotateInfor(theBoneDensitySwapData.m_pYOZView->GetRotate()->ChangeRotateAngle_Current(false, 90, 0, info));	
	theBoneDensitySwapData.m_pYOZView->UpdateImage(3);

	theBoneDensitySwapData.m_pXOYView->Invalidate(FALSE);
	theBoneDensitySwapData.m_pXOYView->UpdateWindow();
	theBoneDensitySwapData.m_pXOZView->Invalidate(FALSE);
	theBoneDensitySwapData.m_pXOZView->UpdateWindow();
	theBoneDensitySwapData.m_pYOZView->Invalidate(FALSE);
	theBoneDensitySwapData.m_pYOZView->UpdateWindow();
}

void SeAPRView::UpdateOtherView()
{
	theBoneDensitySwapData.m_pXOYView->Invalidate(FALSE);
	theBoneDensitySwapData.m_pXOYView->UpdateWindow();
	theBoneDensitySwapData.m_pXOZView->Invalidate(FALSE);
	theBoneDensitySwapData.m_pXOZView->UpdateWindow();
	theBoneDensitySwapData.m_pYOZView->Invalidate(FALSE);
	theBoneDensitySwapData.m_pYOZView->UpdateWindow();
}


BOOL SeAPRView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: ????????????????????/?????????
	int nDcmWidth = theBoneDensitySwapData.m_nRotateDcmSideLength;
	switch (m_nPlaneNum)
	{
	case 1:
		m_dZALLTrans = m_dZALLTrans - int(zDelta/120);
		m_dZALLTrans = m_dZALLTrans < -nDcmWidth/2 ? -nDcmWidth/2 : m_dZALLTrans;
		m_dZALLTrans = m_dZALLTrans > nDcmWidth/2 ? nDcmWidth/2 : m_dZALLTrans;
		break;
	case 2:
		m_dXALLTrans = m_dXALLTrans - int(zDelta/120);
		m_dXALLTrans = m_dXALLTrans < -nDcmWidth/2 ? -nDcmWidth/2 : m_dXALLTrans;
		m_dXALLTrans = m_dXALLTrans > nDcmWidth/2 ? nDcmWidth/2 : m_dXALLTrans;
		break;
	case 3:
		m_dYALLTrans = m_dYALLTrans - int(zDelta/120);
		m_dYALLTrans = m_dYALLTrans < -nDcmWidth/2 ? -nDcmWidth/2 : m_dYALLTrans;
		m_dYALLTrans = m_dYALLTrans > nDcmWidth/2 ? nDcmWidth/2 : m_dYALLTrans;
		break;
	default:
		break;
	}
	theBoneDensitySwapData.m_pXOYView->UpdateImage(1);
	theBoneDensitySwapData.m_pYOZView->UpdateImage(3);
	theBoneDensitySwapData.m_pXOZView->UpdateImage(2);
	UpdateOtherView();
	return CImageViewerView::OnMouseWheel(nFlags, zDelta, pt);
}

void SeAPRView::SetPos(CPoint point)
{
	MprRotateInfo info = theBoneDensitySwapData.m_pXOYView->m_RotateInfo;
	switch (m_nPlaneNum)
	{
	case 1:
		{
			m_dXALLTrans = point.x;
			m_dYALLTrans = point.y;
			break;
		}
	case 2:
		{
			m_dZALLTrans = -point.x;
			m_dYALLTrans = point.y;
			break;
		}
	case 3:
		{
			m_dXALLTrans = point.x;
			m_dZALLTrans = point.y;
			break;
		}
	default:
		break;
	}
	theBoneDensitySwapData.m_pXOYView->UpdateImage(1);
	theBoneDensitySwapData.m_pYOZView->UpdateImage(3);
	theBoneDensitySwapData.m_pXOZView->UpdateImage(2);

	UpdateOtherView();
}

void SeAPRView::RotatePosLine(float fAngle)
{
	fAngle = fAngle*PI/180;
	double dTmpX = m_dXALLTrans;
	double dTmpY = m_dYALLTrans;
	double dTmpZ = m_dZALLTrans;
	switch (m_nPlaneNum)
	{
	case 1:
		{
			m_dXALLTrans = dTmpX*cos(fAngle) - dTmpY*sin(fAngle);
			m_dYALLTrans = dTmpX*sin(fAngle) + dTmpY*cos(fAngle);
			break;
		}
	case 2:
		{
			m_dZALLTrans = dTmpZ*cos(-fAngle) - dTmpY*sin(-fAngle);
			m_dYALLTrans = dTmpZ*sin(-fAngle) + dTmpY*cos(-fAngle);
			break;
		}
	case 3:
		{
			m_dXALLTrans = dTmpX*cos(fAngle) - dTmpZ*sin(fAngle);
			m_dZALLTrans = dTmpX*sin(fAngle) + dTmpZ*cos(fAngle);
			break;
		}
	default:
		break;
	}
}

float SeAPRView::GetAngle(CPoint point)
{
	CRect	rtClient;
	GetClientRect(&rtClient);
	int nCenterX = (rtClient.left + rtClient.right)/2;
	int nCenterY = (rtClient.top + rtClient.bottom)/2;
	double dDeltX = point.x - nCenterX;
	double dDeltY = -(point.y - nCenterY);
	double dZRotateCurrent_Radial = atan2(dDeltY, dDeltX);
	float fAngle = dZRotateCurrent_Radial/PI*180.0;
	return fAngle;
}

void SeAPRView::GetHitPoints(Rect* rect, int nl, int nt, int nr, int nb)
{
	rect[0] = Rect(nl-2, nt-2, 5, 5);
	rect[1] = Rect(nl-2 + (nr-nl)/2, nt-2, 5, 5);
	rect[2] = Rect(nr-1, nt-1, 5, 5);
	rect[3] = Rect(nr-2, nt-2 + (nb - nt)/2, 5, 5);
	rect[4] = Rect(nr-2, nb-2, 5, 5);
	rect[5] = Rect(nl-2 + (nr-nl)/2, nb-2, 5, 5);
	rect[6] = Rect(nl-2, nb-2, 5, 5);
	rect[7] = Rect(nl-2, nt-2 + (nb - nt)/2, 5, 5);
}

void SeAPRView::APRDraw(CZKMemDC* pDC)
{
	CDcmPic* pDcm = (CDcmPic*)GetDisplayMgr()->GetCurrentImage();
	if(pDcm == NULL)
		return;

	Graphics gc(pDC->m_hDC);
	gc.SetSmoothingMode(SmoothingModeHighQuality);

	CRect   rect;
	GetClientRect(&rect);
	CPoint ptCenter(rect.left + rect.Width()/2, rect.top + rect.Height()/2);
	Color	colorRed(255, 255, 0, 0);
	Pen     penRed(colorRed, 1.0);
	Color	colorGreen(255, 0, 255, 0);
	Pen     penGreen(colorGreen, 1.0);
	Color	colorBlue(255, 0, 0, 255);
	Pen     penBlue(colorBlue, 1.0);
	Color	colorYellow(255, 255, 255, 0);
	Pen     penYellow(colorYellow, 1.0);
	CRect	rtDisplay = pDcm->GetImageDispRect();

	// DrawText
	FontFamily fontFamily(L"???");
	Gdiplus::Font font(&fontFamily, 16, FontStyleRegular, UnitPixel);
	LOGFONT lf;
	::GetObject((HFONT)GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
	memcpy(lf.lfFaceName, "Arial", 5) ;
	lf.lfHeight = 25 ;
	SolidBrush brush(Color(255, 255, 255, 255));
	CStringW str = L"";
	int nDcmWidth = theBoneDensitySwapData.m_nRotateDcmSideLength;
	switch(GetPlaneNum())
	{
	case 1:
		{
			CPoint pt(m_dXALLTrans, m_dYALLTrans);
			pt.Offset(pDcm->GetWidth()/2, pDcm->GetHeight()/2);
			CPoint ptScreen = pDcm->Image2Screen(&pt);
			gc.DrawLine(&penBlue, Point(ptScreen.x, rtDisplay.top), Point(ptScreen.x, rtDisplay.bottom) );
			gc.DrawLine(&penGreen, Point(rtDisplay.left, ptScreen.y), Point(rtDisplay.right, ptScreen.y));
			gc.DrawRectangle(&penRed, rtDisplay.left, rtDisplay.top, rtDisplay.Width(), rtDisplay.Height());
			str.Format(L"Z:%d", static_cast<int>(m_dZALLTrans + nDcmWidth/2));
			gc.DrawString(str, -1, &font, PointF(5.0f, 5.0f), &brush);
			break;
		}
	case 2:
		{
			CPoint pt(-m_dZALLTrans, m_dYALLTrans);
			pt.Offset(pDcm->GetWidth()/2, pDcm->GetHeight()/2);
			CPoint ptScreen = pDcm->Image2Screen(&pt);
			gc.DrawLine(&penRed, Point(ptScreen.x, rtDisplay.top), Point(ptScreen.x, rtDisplay.bottom));
			gc.DrawLine(&penGreen, Point(rtDisplay.left, ptScreen.y), Point(rtDisplay.right, ptScreen.y));
			gc.DrawRectangle(&penBlue, rtDisplay.left, rtDisplay.top, rtDisplay.Width(), rtDisplay.Height());
			str.Format(L"X:%d", static_cast<int>(m_dXALLTrans + nDcmWidth/2));
			gc.DrawString(str, -1, &font, PointF(5.0f, 5.0f), &brush);
			break;
		}
	case 3:
		{
			CPoint pt(m_dXALLTrans, m_dZALLTrans);
			pt.Offset(pDcm->GetWidth()/2, pDcm->GetHeight()/2);
			CPoint ptScreen = pDcm->Image2Screen(&pt);
			gc.DrawLine(&penBlue, Point(ptScreen.x, rtDisplay.top), Point(ptScreen.x, rtDisplay.bottom) );
			gc.DrawLine(&penRed, Point(rtDisplay.left,ptScreen.y), Point(rtDisplay.right, ptScreen.y));
			gc.DrawRectangle(&penGreen, rtDisplay.left, rtDisplay.top, rtDisplay.Width(), rtDisplay.Height());
			str.Format(L"Y:%d", static_cast<int>(m_dYALLTrans + nDcmWidth/2));
			gc.DrawString(str, -1, &font, PointF(5.0f, 5.0f), &brush);
			break;
		}
	default:
		{
			break;
		}
	}
}

void SeAPRView::APRRectDraw(CZKMemDC* pDC)
{
	CDcmPic* pDcm = (CDcmPic*)GetDisplayMgr()->GetCurrentImage();
	if(pDcm == NULL)
		return;

	Graphics gc(pDC->m_hDC);
	gc.SetSmoothingMode(SmoothingModeHighQuality);
	CRect	rtDisplay = pDcm->GetImageDispRect();
	if(m_nXStart + m_nYStart + m_nZStart +  m_nXEnd + m_nYEnd + m_nZEnd == 0)
	{
		m_nXStart =  rtDisplay.left + rtDisplay.Width()/5;
		m_nYStart = rtDisplay.top + rtDisplay.Height()/5;
		m_nZStart = rtDisplay.top + rtDisplay.Height()/10;
		m_nXEnd = rtDisplay.left + rtDisplay.Width()/5*4;
		m_nYEnd = rtDisplay.top + rtDisplay.Height()/5*4;
		m_nZEnd = rtDisplay.top + rtDisplay.Height()/10*9;
	}
	Color	color(255, 0, 255, 255);
	Pen     pen(color, 1.0);
	int nOffset = pDcm->GetImageDispRect().left;
	CRect rtWidth;
	GetClientRect(&rtWidth);
	int nWidth = rtWidth.Width();
	Rect rect;
	Rect rtBorders[8];


	// DrawText
	FontFamily fontFamily(L"???");
	Gdiplus::Font font(&fontFamily, 16, FontStyleRegular, UnitPixel);
	LOGFONT lf;
	::GetObject((HFONT)GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
	memcpy(lf.lfFaceName, "Arial", 5) ;
	lf.lfHeight = 25 ;
	SolidBrush brush(Color(255, 255, 255, 255));
	CStringW str = L"";
	int nDcmWidth = theBoneDensitySwapData.m_nRotateDcmSideLength;
	switch(GetPlaneNum())
	{
	case 1:
		rect =  Rect(m_nXStart, m_nYStart, m_nXEnd - m_nXStart, m_nYEnd - m_nYStart);
		gc.DrawRectangle(&pen, rect);
		str.Format(L"Z:%d", static_cast<int>(m_dZALLTrans + nDcmWidth/2));
		gc.DrawString(str, -1, &font, PointF(5.0f, 5.0f), &brush);
		break;
	case 2:
		rect =  Rect(nWidth - m_nZEnd - nOffset, m_nYStart, m_nZEnd - m_nZStart, m_nYEnd - m_nYStart);
		gc.DrawRectangle(&pen, rect);
		str.Format(L"X:%d", static_cast<int>(m_dXALLTrans + nDcmWidth/2));
		gc.DrawString(str, -1, &font, PointF(5.0f, 5.0f), &brush);
		break;
	case 3:
		rect =  Rect(m_nXStart, m_nZStart, m_nXEnd - m_nXStart, m_nZEnd - m_nZStart);
		gc.DrawRectangle(&pen, rect);
		str.Format(L"Y:%d", static_cast<int>(m_dYALLTrans + nDcmWidth/2));
		gc.DrawString(str, -1, &font, PointF(5.0f, 5.0f), &brush);
		break;
	default:
		break;
	}
	GetHitPoints(&rtBorders[0], rect.GetLeft(), rect.GetTop(), rect.GetRight(), rect.GetBottom());
	gc.DrawRectangles(&pen, &rtBorders[0], 8);
}

void SeAPRView::SetScreenWidth(int nWidth)
{
	theBoneDensitySwapData.m_nScreenWidth = nWidth;
}

void SeAPRView::SetScreenHeight(int nHeight)
{
	theBoneDensitySwapData.m_nScreenHeight = nHeight;
}



void SeAPRView::SetWinLevel(int nWinCenter, int nWinWidth)
{
	CDcmPic* pDcm = (CDcmPic *)GetDisplayMgr()->GetFirstDispImage();
	if (pDcm != NULL)
		pDcm->AdjustWin(nWinCenter, nWinWidth);
	Invalidate(FALSE);
}

void SeAPRView::Reset()
{
	m_SeVisualAPR.Reset();
}

void SeAPRView::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: ????????????????????/?????????
	CDcmPic* pDcm = (CDcmPic *)GetDisplayMgr()->GetFirstDispImage();
	if (pDcm != NULL)
		m_SeVisualAPR.SetWinLevel(pDcm->GetWinCenter(), pDcm->GetWinWidth());
	CImageViewerView::OnMouseMove(nFlags, point);
}










