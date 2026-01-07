#include "StdAfx.h"
#include "SeBoneDensityMouseTool.h"
#include "SeAPRView.h"
#include "SeROIView.h"


static SeROITool	s_ROITool;
static SeAPRMouseTool   s_APRTool;
static SeAPRRectMouseTool s_APRRecctTool;

SeROITool::SeROITool(void) : CMouseTool(MT_BONEZROI)
{
}


SeROITool::~SeROITool(void)
{

}

void SeROITool::Draw( CWnd* pWnd, CDC* pDC, CRect& rt )
{
	SeROIView* pView = (SeROIView*)pWnd;
	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetFirstDispImage();
	if (pDcm == NULL)
		return;

	Graphics gc(pDC->m_hDC);

	if (pView->m_pVecEdge != NULL)
	{
		int nPlanePos = pView->GetDisplayMgr()->GetFirstImage();
		if (!pView->m_pVecEdge[nPlanePos].empty())
		{
			DrawRegion(pView->m_pVecEdge[nPlanePos], Color(100, 0, 255, 0), &gc, pDcm);
		}
	}
	if (pView->m_pVecEdgeInside != NULL)
	{
 		int nPlanePos = pView->GetDisplayMgr()->GetFirstImage();
 		if (!pView->m_pVecEdgeInside[nPlanePos].empty())
 		{
			DrawRegion(pView->m_pVecEdgeInside[nPlanePos], Color(100, 255, 255, 0), &gc, pDcm);
 		}
	}

	if(pView->IsClipOutside())
	{
		Color	colorFocus(255,0,255,0);
		DrawLine(m_vecPtsTmp, colorFocus, &gc, pDcm);
	}
	else
	{
		Color	colorFocus(255,255,255,0);
		DrawLine(m_vecPtsTmp, colorFocus, &gc, pDcm);
	}
}

void SeROITool::DrawLine(vector <CPoint> vecPts, Color color, Graphics *gc, CDcmPic* pDcm)
{
	int nSize = (int)vecPts.size();
	Pen		MarkPen(color,2);
	Point*	pMarkpt = new Point[nSize];
	for (int i = 0 ; i < nSize ; i++)
	{
		CPoint ptMark = pDcm->Image2Screen(&vecPts[i]);
		pMarkpt[i].X = ptMark.x;
		pMarkpt[i].Y = ptMark.y;
	}
	gc->DrawLines(&MarkPen,pMarkpt,nSize);
	gc->DrawLine(&MarkPen, pMarkpt[0], pMarkpt[nSize - 1]);
	delete []pMarkpt;
}

void SeROITool::DrawRegion(vector <CPoint> vecPts, SolidBrush brush, Graphics *gc, CDcmPic* pDcm)
{
	GraphicsPath path;
	Pen pen(Color(255, 0, 0, 255), 2.0);
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

void SeROITool::OnLButtonDown( CWnd* pWnd, UINT nFlags, CPoint& point )
{
	SeROIView* pView = (SeROIView*)pWnd;

	m_bLBDown = TRUE;

	m_nPosition = pView->GetDisplayMgr()->GetFirstImage();

	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if (pDcm == NULL)
		return;

	if((nFlags & MK_SHIFT) == MK_SHIFT)
	{
		m_bShiftDown = TRUE;
		if(pView->IsClipOutside())
		{
			pView->m_pVecEdge[m_nPosition].clear();
		}
		else
		{
			pView->m_pVecEdgeInside[m_nPosition].clear();
		}
		nFlags  = nFlags& (~MK_SHIFT);
		pView->Invalidate(FALSE);
		pView->UpdateWindow();
	}
	else
	{
		m_bShiftDown = FALSE;
	}

	if(GetAsyncKeyState(VK_MENU))
	{
		m_bAltDown = TRUE;
		m_bShiftDown = FALSE;
	}
	else
	{
		m_bAltDown = FALSE;
	}

	m_vecPtsTmp.clear();

}

void SeROITool::OnLButtonUp( CWnd* pWnd, UINT nFlags, CPoint& point )
{
	SeROIView* pView = (SeROIView*)pWnd;
	m_bLBDown = FALSE;
	if(pView->IsClipOutside())
	{
		pView->m_vecPoid.push_back(m_nPosition);
		if(m_bShiftDown || pView->m_pVecEdge[m_nPosition].size() == 0)
		{
			pView->m_pVecEdge[m_nPosition].insert(pView->m_pVecEdge[m_nPosition].begin(), m_vecPtsTmp.begin(), m_vecPtsTmp.end());
		}
		if(!m_bShiftDown && !m_bAltDown && m_ScopeTool.Intersection(pView->m_pVecEdge[m_nPosition], m_vecPtsTmp))
		{
			vector <CPoint> pts;
			m_ScopeTool.Operation(UNION, pView->m_pVecEdge[m_nPosition], m_vecPtsTmp, pts);
			pView->m_pVecEdge[m_nPosition].clear();
			pView->m_pVecEdge[m_nPosition].insert(pView->m_pVecEdge[m_nPosition].begin(), pts.begin(), pts.end());
		}
		if(!m_bShiftDown && m_bAltDown && m_ScopeTool.Intersection(pView->m_pVecEdge[m_nPosition], m_vecPtsTmp))
		{
			vector <CPoint> pts;
			m_ScopeTool.Operation(DIFF, pView->m_pVecEdge[m_nPosition], m_vecPtsTmp, pts);
			pView->m_pVecEdge[m_nPosition].clear();
			pView->m_pVecEdge[m_nPosition].insert(pView->m_pVecEdge[m_nPosition].begin(), pts.begin(), pts.end());
		}
	}
	else
	{
		pView->m_vecPoidInside.push_back(m_nPosition);
		if(m_bShiftDown || pView->m_pVecEdgeInside[m_nPosition].size() == 0)
		{
			pView->m_pVecEdgeInside[m_nPosition].insert(pView->m_pVecEdgeInside[m_nPosition].begin(), m_vecPtsTmp.begin(), m_vecPtsTmp.end());
		}
		if(!m_bShiftDown && !m_bAltDown && m_ScopeTool.Intersection(pView->m_pVecEdgeInside[m_nPosition], m_vecPtsTmp))
		{
			vector <CPoint> pts;
			m_ScopeTool.Operation(UNION, pView->m_pVecEdgeInside[m_nPosition], m_vecPtsTmp, pts);
			pView->m_pVecEdgeInside[m_nPosition].clear();
			pView->m_pVecEdgeInside[m_nPosition].insert(pView->m_pVecEdgeInside[m_nPosition].begin(), pts.begin(), pts.end());
		}
		if(!m_bShiftDown && m_bAltDown && m_ScopeTool.Intersection(pView->m_pVecEdgeInside[m_nPosition], m_vecPtsTmp))
		{
			vector <CPoint> pts;
			m_ScopeTool.Operation(DIFF, pView->m_pVecEdgeInside[m_nPosition], m_vecPtsTmp, pts);
			pView->m_pVecEdgeInside[m_nPosition].clear();
			pView->m_pVecEdgeInside[m_nPosition].insert(pView->m_pVecEdgeInside[m_nPosition].begin(), pts.begin(), pts.end());
		}
 	}

	m_vecPtsTmp.clear();
	pView->Invalidate(FALSE);
}

void SeROITool::OnMouseMove( CWnd* pWnd, UINT nFlags, CPoint& point )
{
	SeROIView* pView = (SeROIView*)pWnd;
	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if (pDcm != NULL && m_bLBDown)
	{
 		CPoint ptImg = pDcm->Screen2Image(&point);
		m_vecPtsTmp.push_back(ptImg);
	}
	pView->Invalidate(FALSE);
}

HCURSOR SeROITool::LoadCursor()
{
	return AfxGetApp()->LoadCursor(IDC_DRAWLINECOR);
}

SeAPRMouseTool::SeAPRMouseTool(): CMouseTool(MT_APRTOOL)
{
	m_bShiftDown = FALSE;
	m_bLBDown = FALSE;
	m_bNewAngle = FALSE;
	m_fAbsoluteAngle = 0.0;
	m_fAngle = 0.0;
}

SeAPRMouseTool::~SeAPRMouseTool()
{

}

void SeAPRMouseTool::OnLButtonDown(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	SeAPRView* pView = (SeAPRView*)pWnd;
	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if(pDcm == NULL)
		return;
	m_bLBDown = TRUE;
	m_bNewAngle = TRUE;
	m_ptLBDown = point;

	if((nFlags & MK_SHIFT) == MK_SHIFT)
	{
		m_bShiftDown = TRUE;
		nFlags  = nFlags& (~MK_SHIFT);		
	}
	pView->Invalidate(FALSE);
	pView->UpdateWindow();
}

void SeAPRMouseTool::OnLButtonUp(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	SeAPRView* pView = (SeAPRView*)pWnd;
	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	m_bLBDown = FALSE;
	m_bShiftDown = FALSE;
}

void SeAPRMouseTool::OnMouseMove(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	SeAPRView* pView = (SeAPRView*)pWnd;
	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();

	if((nFlags & MK_SHIFT) == MK_SHIFT)
	{
		m_bShiftDown = TRUE;
		nFlags  = nFlags& (~MK_SHIFT);
		LoadCursor();
	}
	else
	{
		m_bShiftDown = FALSE;
		LoadCursor();
	}

	if (pDcm != NULL && m_bLBDown && m_bShiftDown)
	{
		m_fZRotateLast = m_fZRotateCurrent;
		m_fZRotateCurrent = pView->GetAngle(point);
		if(m_bNewAngle)
		{
			m_fZRotateLast = m_fZRotateCurrent;
			m_bNewAngle = FALSE;
		}
		float fAngle = m_fZRotateCurrent - m_fZRotateLast;	
		pView->SetRotateInfor(pView->GetRotate()->ChangeRotateAngle_CurrentZ(false, fAngle, pView->GetRotateInfor()));
		pView->UpdateOtherView(pView->GetRotateInfor());
		pView->RotatePosLine(-fAngle);
	}
	else if(pDcm != NULL && m_bLBDown)
	{
		CPoint pt = pDcm->Screen2Image(&point);
		pt.Offset(-pDcm->GetWidth()/2, -pDcm->GetHeight()/2);
		pView->SetPos(pt);
	}
}

HCURSOR SeAPRMouseTool::LoadCursor()
{
	if(m_bShiftDown)
		return AfxGetApp()->LoadCursor(IDC_ROTATECUR);
	return AfxGetApp()->LoadCursor(IDC_MOVECUR);
}

void SeAPRMouseTool::Draw(CWnd* pWnd, CDC *pDC, CRect& rt)
{
	SeAPRView* pView = (SeAPRView*)pWnd;
	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if(pDcm == NULL)
		return;

	Graphics gc(pDC->m_hDC);
	gc.SetSmoothingMode(SmoothingModeHighQuality);

	CRect   rect;
	pView->GetClientRect(&rect);
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
	pView->SetScreenWidth(rect.Width());
	pView->SetScreenHeight(rect.Height());

	switch(pView->GetPlaneNum())
	{
	case 1:
		{
			CPoint pt(pView->m_dXALLTrans, pView->m_dYALLTrans);
			pt.Offset(pDcm->GetWidth()/2, pDcm->GetHeight()/2);
			CPoint ptScreen = pDcm->Image2Screen(&pt);
			gc.DrawLine(&penBlue, Point(ptScreen.x, rtDisplay.top), Point(ptScreen.x, rtDisplay.bottom) );
			gc.DrawLine(&penGreen, Point(rtDisplay.left, ptScreen.y), Point(rtDisplay.right, ptScreen.y));
			gc.DrawRectangle(&penRed, rtDisplay.left, rtDisplay.top, rtDisplay.Width(), rtDisplay.Height());
			break;
		}
	case 2:
		{
			CPoint pt(-pView->m_dZALLTrans, pView->m_dYALLTrans);
			pt.Offset(pDcm->GetWidth()/2, pDcm->GetHeight()/2);
			CPoint ptScreen = pDcm->Image2Screen(&pt);
			gc.DrawLine(&penRed, Point(ptScreen.x, rtDisplay.top), Point(ptScreen.x, rtDisplay.bottom));
			gc.DrawLine(&penGreen, Point(rtDisplay.left, ptScreen.y), Point(rtDisplay.right, ptScreen.y));
			gc.DrawRectangle(&penBlue, rtDisplay.left, rtDisplay.top, rtDisplay.Width(), rtDisplay.Height());
			break;
		}
	case 3:
		{
			CPoint pt(pView->m_dXALLTrans, pView->m_dZALLTrans);
			pt.Offset(pDcm->GetWidth()/2, pDcm->GetHeight()/2);
			CPoint ptScreen = pDcm->Image2Screen(&pt);
			gc.DrawLine(&penBlue, Point(ptScreen.x, rtDisplay.top), Point(ptScreen.x, rtDisplay.bottom) );
			gc.DrawLine(&penRed, Point(rtDisplay.left,ptScreen.y), Point(rtDisplay.right, ptScreen.y));
			gc.DrawRectangle(&penGreen, rtDisplay.left, rtDisplay.top, rtDisplay.Width(), rtDisplay.Height());
			break;
		}
	default:
		{
			break;
		}
	}
// 	CString str;
// 	str.Format("%d %d %d", int(pView->m_dXALLTrans), int(pView->m_dYALLTrans), int(pView->m_dZALLTrans));
// 	pDC->TextOutA(0, 0, str);
}





SeAPRRectMouseTool::SeAPRRectMouseTool(): CMouseTool(MT_APRRECT)
{
	CRect m_rtBorder[8];
	for(int i=0; i<8; i++)
		m_rtBorder[i] = CRect(0,0,0,0);
	m_nMouseState = CUR_OUT;
	m_bLBdown = FALSE;
	m_nHitWhiceOne = -1;
}

SeAPRRectMouseTool::~SeAPRRectMouseTool()
{

}

void SeAPRRectMouseTool::Draw(CWnd* pWnd, CDC *pDC, CRect& rt)
{
	SeAPRView* pView = (SeAPRView*)pWnd;
	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if(pDcm == NULL)
		return;

	Graphics gc(pDC->m_hDC);
	gc.SetSmoothingMode(SmoothingModeHighQuality);
	CRect	rtDisplay = pDcm->GetImageDispRect();
	if(pView->m_nXStart + pView->m_nYStart + pView->m_nZStart +  pView->m_nXEnd + pView->m_nYEnd + pView->m_nZEnd == 0)
	{
		pView->m_nXStart =  rtDisplay.left + rtDisplay.Width()/5;
		pView->m_nYStart = rtDisplay.top + rtDisplay.Height()/5;
		pView->m_nZStart = rtDisplay.top + rtDisplay.Height()/10;
		pView->m_nXEnd = rtDisplay.left + rtDisplay.Width()/5*4;
		pView->m_nYEnd = rtDisplay.top + rtDisplay.Height()/5*4;
		pView->m_nZEnd = rtDisplay.top + rtDisplay.Height()/10*9;
	}
	Color	color(255, 0, 255, 255);
	Pen     pen(color, 1.0);
	int nOffset = pDcm->GetImageDispRect().left;
	CRect rtWidth;
	pView->GetClientRect(&rtWidth);
	int nWidth = rtWidth.Width();
	Rect rect;
	Rect rtBorders[8];
	switch(pView->GetPlaneNum())
	{
	case 1:
		rect =  Rect(pView->m_nXStart, pView->m_nYStart, pView->m_nXEnd - pView->m_nXStart, pView->m_nYEnd - pView->m_nYStart);
		gc.DrawRectangle(&pen, rect);
		break;
	case 2:
		rect =  Rect(nWidth - pView->m_nZEnd - nOffset, pView->m_nYStart, pView->m_nZEnd - pView->m_nZStart, pView->m_nYEnd - pView->m_nYStart);
		gc.DrawRectangle(&pen, rect);
		break;
	case 3:
		rect =  Rect(pView->m_nXStart, pView->m_nZStart, pView->m_nXEnd - pView->m_nXStart, pView->m_nZEnd - pView->m_nZStart);
		gc.DrawRectangle(&pen, rect);
		break;
	default:
		break;
	}
	pView->GetHitPoints(&rtBorders[0], rect.GetLeft(), rect.GetTop(), rect.GetRight(), rect.GetBottom());
	gc.DrawRectangles(&pen, &rtBorders[0], 8);
}

void SeAPRRectMouseTool::OnLButtonDown(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	SeAPRView* pView = (SeAPRView*)pWnd;
	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if(pDcm == NULL)
		return;
	m_bLBdown = TRUE;
	m_ptStart = point;
	HitTest(point);
}

void SeAPRRectMouseTool::OnLButtonUp(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	m_bLBdown = FALSE;
}

void SeAPRRectMouseTool::OnMouseMove(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	SeAPRView* pView = (SeAPRView*)pWnd;
	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if(pDcm == NULL)
		return;
	int nOffset = pDcm->GetImageDispRect().left;
	CRect rtWidth;
	pView->GetClientRect(&rtWidth);
	int nWidth = rtWidth.Width();
	SetPoint(pView, nWidth, nOffset);
	if(!m_bLBdown)
	{
		SetMouseState(point);
	}
	else
	{
		MovePoint(point);
		int nOffset = pDcm->GetImageDispRect().left;
		CRect rtWidth;
		pView->GetClientRect(&rtWidth);
		int nWidth = rtWidth.Width();
		DeSetPoint(pView, nWidth, nOffset);
		pView->Invalidate(FALSE);
		pView->UpdateOtherView();
	}
}

HCURSOR SeAPRRectMouseTool::LoadCursor()
{
	switch (m_nMouseState)
	{
	case 0:
		{
			return AfxGetApp()->LoadCursor(IDC_MOVECUR);
			break;
		}
	case 1:
		{
			return AfxGetApp()->LoadCursor(IDC_MYCROSS);
			break;
		}
	case 2:
		{
			return AfxGetApp()->LoadCursor(IDC_POINTER);
			break;
		}
	default:
		break;
	}
	return AfxGetApp()->LoadCursor(IDC_POINTER); 
}

void SeAPRRectMouseTool::SetPoint(CWnd* pWnd, int nWidth, int nOffset)
{
	SeAPRView* pView = (SeAPRView*)pWnd;
	switch(pView->GetPlaneNum())
	{
	case 1:
		{
			m_ptLT = CPoint(pView->m_nXStart, pView->m_nYStart);
			m_ptRB = CPoint(pView->m_nXEnd, pView->m_nYEnd);
			break;
		}
	case 2:
		{
			m_ptLT = CPoint(nWidth - pView->m_nZEnd - nOffset, pView->m_nYStart);
			m_ptRB = CPoint(nWidth - pView->m_nZStart - nOffset, pView->m_nYEnd);
			break;
		}
	case 3:
		{
			m_ptLT = CPoint(pView->m_nXStart, pView->m_nZStart);
			m_ptRB = CPoint(pView->m_nXEnd, pView->m_nZEnd);
			break;
		}
	default:
		break;
	}
}

void SeAPRRectMouseTool::SetMouseState(CPoint point)
{
	CRect rect(m_ptLT.x, m_ptLT.y, m_ptRB.x, m_ptRB.y);
	rect.NormalizeRect();	
	CRect rect2(rect);
	rect.InflateRect(5,5);
	if ((abs(point.x - rect.left) < 5 || abs(point.x - (rect.right + rect.left)/2) < 5 || abs(point.x - rect.right) < 5) &&
		(abs(point.y - rect.top) < 5 || abs(point.y - (rect.top + rect.bottom)/2) < 5 || abs(point.y - rect.bottom) < 5) &&
		!(abs(point.x - (rect.left + rect.right)/2) < 5 && abs(point.y - (rect.top + rect.bottom)/2) < 5))
	{
		m_nMouseState = CUR_ON;
	}	
	else if (PtInRect(&rect2, point))
	{
		m_nMouseState = CUR_IN;
	}
	else
	{
		m_nMouseState = CUR_OUT;
	}
	LoadCursor();
}

void SeAPRRectMouseTool::MovePoint(CPoint point)
{
	if(m_nMouseState == CUR_IN)
	{
		m_ptLT.Offset(point - m_ptStart);
		m_ptRB.Offset(point - m_ptStart);
		m_ptStart = point;
	}
	else if(m_nMouseState == CUR_ON)
	{
		switch(m_nHitWhiceOne)
		{
		case 0:
			{
				m_ptLT = point;
				break;
			}
		case 1:
			{
				m_ptLT.y = point.y;
				break;
			}
		case 2:
			{
				m_ptRB.x = point.x;
				m_ptLT.y = point.y;
				break;
			}
		case 4:
			{
				m_ptLT.x = point.x;
				break;
			}
		case 6:
			{
				m_ptRB.x = point.x;
				break;
			}
		case 8:
			{
				m_ptLT.x = point.x;
				m_ptRB.y = point.y;
				break;
			}
		case 9:
			{
				m_ptRB.y = point.y;
				break;
			}
		case 10:
			{
				m_ptRB = point;
				break;
			}
		default:
			break;
		}
		CRect rect(m_ptLT, m_ptRB);
		rect.NormalizeRect();
		m_ptLT = rect.TopLeft();
		m_ptRB = rect.BottomRight();
	}
}

void SeAPRRectMouseTool::DeSetPoint(CWnd* pWnd, int nWidth, int nOffset)
{
	SeAPRView* pView = (SeAPRView*)pWnd;
	CRect rect(m_ptLT.x, m_ptLT.y, m_ptRB.x, m_ptRB.y);
	rect.NormalizeRect();
	switch(pView->GetPlaneNum())
	{
	case 1:
		pView->m_nXStart = rect.left;
		pView->m_nXEnd   = rect.right;
		pView->m_nYStart = rect.top;
		pView->m_nYEnd   = rect.bottom;
		break;
	case 3:
		pView->m_nXStart = rect.left;
		pView->m_nXEnd   = rect.right;
		pView->m_nZStart = rect.top;
		pView->m_nZEnd   = rect.bottom;
		break;
	case 2:
		pView->m_nZStart = nWidth - rect.right - nOffset;
		pView->m_nZEnd   = nWidth - rect.left - nOffset;
		pView->m_nYStart = rect.top;
		pView->m_nYEnd   = rect.bottom;
		break;
	default:
		break;
	}
}

void SeAPRRectMouseTool::HitTest(CPoint point)
{
	if(m_nMouseState == CUR_ON && abs(m_ptLT.x - m_ptRB.x) > 8 && abs(m_ptLT.y - m_ptRB.y) > 8)
	{
		CPoint ptCenter((m_ptLT.x + m_ptRB.x)/2, (m_ptLT.y + m_ptRB.y)/2);
		m_nHitWhiceOne = 0;
		if(point.x - ptCenter.x < -4)
		{
			m_nHitWhiceOne = 0; 
		}
		else if (point.x - ptCenter.x > 4)
		{
			m_nHitWhiceOne = 2; 
		}
		else
		{
			m_nHitWhiceOne = 1;
		}
		if(point.y - ptCenter.y < -4)
		{
			m_nHitWhiceOne += 0;
		}
		else if(point.y - ptCenter.y > 4)
		{
			m_nHitWhiceOne += 8;
		}
		else
		{
			m_nHitWhiceOne += 4;
		}
	}
}