#include "StdAfx.h"
#include "Resource.h"
#include "SeGeneralMouseTool.h"
#include "Measurement.h"
#include "SeMPRView.h"
#include <cmath>
#include <algorithm>
#include <climits>

namespace
{
	inline int RoundToInt(double value)
	{
		return static_cast<int>((value >= 0.0) ? value + 0.5 : value - 0.5);
	}

	void BuildRectanglePolygon(const CPoint& start, const CPoint& end, vector<CPoint>& pts, bool forceSquare)
	{
		int left = std::min(start.x, end.x);
		int right = std::max(start.x, end.x);
		int top = std::min(start.y, end.y);
		int bottom = std::max(start.y, end.y);

		int width = right - left;
		int height = bottom - top;

		if (forceSquare)
		{
			const int baseWidth = std::abs(end.x - start.x);
			const int baseHeight = std::abs(end.y - start.y);
			const int size = std::max(baseWidth, baseHeight);
			if (size == 0)
			{
				pts.push_back(start);
				return;
			}
			const bool startIsLeft = (start.x <= end.x);
			const bool startIsTop = (start.y <= end.y);
			if (startIsLeft)
				right = left + size;
			else
				left = right - size;

			if (startIsTop)
				bottom = top + size;
			else
				top = bottom - size;

			width = right - left;
			height = bottom - top;
		}

		if (width == 0 && height == 0)
		{
			pts.push_back(CPoint(left, top));
			return;
		}
		pts.push_back(CPoint(left, top));
		pts.push_back(CPoint(right, top));
		pts.push_back(CPoint(right, bottom));
		pts.push_back(CPoint(left, bottom));
	}

	void BuildCirclePolygon(const CPoint& start, const CPoint& end, vector<CPoint>& pts, bool forceCircle)
	{
		int left = std::min(start.x, end.x);
		int right = std::max(start.x, end.x);
		int top = std::min(start.y, end.y);
		int bottom = std::max(start.y, end.y);

		int width = right - left;
		int height = bottom - top;

		if (forceCircle)
		{
			const int baseWidth = std::abs(end.x - start.x);
			const int baseHeight = std::abs(end.y - start.y);
			const int size = std::max(baseWidth, baseHeight);
			if (size == 0)
			{
				pts.push_back(start);
				return;
			}
			const bool startIsLeft = (start.x <= end.x);
			const bool startIsTop = (start.y <= end.y);
			if (startIsLeft)
				right = left + size;
			else
				left = right - size;
			if (startIsTop)
				bottom = top + size;
			else
				top = bottom - size;
			width = right - left;
			height = bottom - top;
		}

		if (width <= 0 || height <= 0)
		{
			pts.push_back(start);
			return;
		}

		const double cx = (static_cast<double>(left) + static_cast<double>(right)) * 0.5;
		const double cy = (static_cast<double>(top) + static_cast<double>(bottom)) * 0.5;
		const double radiusX = static_cast<double>(width) * 0.5;
		const double radiusY = static_cast<double>(height) * 0.5;

		const int segments = 64;
		pts.reserve(segments);
		for (int i = 0; i < segments; ++i)
		{
			const double angle = 2.0 * M_PI * static_cast<double>(i) / static_cast<double>(segments);
			const int x = RoundToInt(cx + radiusX * std::cos(angle));
			const int y = RoundToInt(cy + radiusY * std::sin(angle));
			pts.push_back(CPoint(x, y));
		}
	}

	void BuildRoiShapePolygon(int shape, const CPoint& start, const CPoint& current, vector<CPoint>& pts, bool forceRegular = false)
	{
		pts.clear();
		switch (shape)
		{
		case ROI_SHAPE_RECT:
			BuildRectanglePolygon(start, current, pts, forceRegular);
			break;
		case ROI_SHAPE_ROUND:
			BuildCirclePolygon(start, current, pts, forceRegular);
			break;
		default:
			break;
		}
	}
}


SeROIMouseTool::SeROIMouseTool(void) : CMouseTool(MT_ROI)
{
	m_bClipInside = TRUE;
	m_bLBDown = FALSE;
	m_bShiftDown = FALSE;
	m_bAltDown = FALSE;
	m_ptStartImg = CPoint(0, 0);
	m_ptCurrentImg = CPoint(0, 0);
}


SeROIMouseTool::~SeROIMouseTool(void)
{

}

void SeROIMouseTool::Draw(CWnd* pWnd, CDC* pDC, CRect& rt)
{
	SeMPRView* pView = (SeMPRView*)pWnd;
	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if(pDcm == NULL)
		return;

	Graphics gc(pDC->m_hDC);
	gc.SetSmoothingMode(SmoothingModeHighQuality);
	Color	colorFocus(255, 255, 97, 0);

	DrawLine(m_vecPtsTmp, colorFocus, 2, &gc, pDcm);

	CRect   rect;
	pView->GetClientRect(&rect);
	rect.InflateRect(0, 0, -2, -1);
	Color	colorGreen(255, 0, 255, 0);
	Pen     penGreen(colorGreen, 2.0);
	gc.DrawRectangle(&penGreen, rect.left, rect.top, rect.Width(), rect.Height());

	Gdiplus::Font font(L"Segoe UI", 12.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
	SolidBrush textBrush(Color(230, 255, 255, 255));
	CStringW strCoord;
	strCoord.Format(L"XY: (%d, %d)", m_ptCurrentImg.x, m_ptCurrentImg.y);
	gc.DrawString(strCoord, strCoord.GetLength(), &font, PointF(6.0f, 21.0f), &textBrush);
	if (m_bLBDown)
	{
		CStringW strStart;
		strStart.Format(L"Start: (%d, %d)", m_ptStartImg.x, m_ptStartImg.y);
		gc.DrawString(strStart, strStart.GetLength(), &font, PointF(6.0f, 39.0f), &textBrush);
	}

	const int activeShape = SeMPRView::GetRoiShape();
	if (m_bLBDown && !m_vecPtsTmp.empty() && (activeShape == ROI_SHAPE_RECT || activeShape == ROI_SHAPE_ROUND))
	{
		int minX = INT_MAX;
		int minY = INT_MAX;
		int maxX = INT_MIN;
		int maxY = INT_MIN;
		for (size_t i = 0; i < m_vecPtsTmp.size(); ++i)
		{
			const CPoint& pt = m_vecPtsTmp[i];
			if (pt.x < minX) minX = pt.x;
			if (pt.x > maxX) maxX = pt.x;
			if (pt.y < minY) minY = pt.y;
			if (pt.y > maxY) maxY = pt.y;
		}
		if (minX <= maxX && minY <= maxY)
		{
			double widthPx = static_cast<double>(maxX - minX);
			double heightPx = static_cast<double>(maxY - minY);
			double pixelSize = pDcm->GetMMPerpixel();
			if (pixelSize <= 0.0)
				pixelSize = 1.0;
			CStringW strDim;
			if (activeShape == ROI_SHAPE_RECT)
			{
				strDim.Format(L"Width: %.2f mm (%.1f px)\nHeight: %.2f mm (%.1f px)",
					widthPx * pixelSize,
					widthPx,
					heightPx * pixelSize,
					heightPx);
			}
			else
			{
				double majorPx = std::max(widthPx, heightPx);
				double minorPx = std::min(widthPx, heightPx);
				strDim.Format(L"Major axis: %.2f mm (%.1f px)\nMinor axis: %.2f mm (%.1f px)",
					majorPx * pixelSize,
					majorPx,
					minorPx * pixelSize,
					minorPx);
			}
			CPoint imgTopLeft(minX, minY);
			CPoint screenTopLeft = pDcm->Image2Screen(&imgTopLeft);
			REAL textX = static_cast<REAL>(screenTopLeft.x + 5);
			REAL textY = static_cast<REAL>(screenTopLeft.y - 40);
			if (textY < 0)
				textY = static_cast<REAL>(screenTopLeft.y + 5);
			RectF layout(textX, textY, 240.0f, 70.0f);
			gc.DrawString(strDim, strDim.GetLength(), &font, layout, NULL, &textBrush);
		}
	}

	if(pView->m_pVecEdge != NULL)
	{
		int nPlanePos = pView->GetAxialPos();
		int nSize = (int)pView->m_pVecEdge[nPlanePos].size();
		if(!pView->m_pVecEdge[nPlanePos].empty())
		{
			DrawLine(pView->m_pVecEdge[nPlanePos], colorFocus, 1, &gc, pDcm);
			DrawRegion(pView->m_pVecEdge[nPlanePos], Color(30, 0, 255, 0), &gc, pDcm);
		}
	}
}

void SeROIMouseTool::OnLButtonDown(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	SeMPRView* pView = (SeMPRView*)pWnd;
	m_bLBDown = TRUE;

	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if (pDcm == NULL)
		return;

	int nPlanePos = pView->m_nCurrentFrame;

	const bool shiftHeld = ((nFlags & MK_SHIFT) == MK_SHIFT);
	const bool ctrlHeld = ((nFlags & MK_CONTROL) == MK_CONTROL);
	if(shiftHeld && !ctrlHeld)
	{
		m_bShiftDown = TRUE;
		pView->m_pVecEdge[nPlanePos].clear();
		nFlags  = nFlags& (~MK_SHIFT);
		pView->Invalidate(FALSE);
		pView->UpdateWindow();
	}
	else
	{
		m_bShiftDown = FALSE;
		pView->Invalidate(FALSE);
		pView->UpdateWindow();
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
	m_ptStartImg = pDcm->Screen2Image(&point);
	m_ptCurrentImg = m_ptStartImg;

}

void SeROIMouseTool::OnLButtonUp(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	SeMPRView* pView = (SeMPRView*)pWnd;
	CDcmPic *pDcm = (CDcmPic *)pView->GetDisplayMgr()->GetCurrentImage();
	if(pDcm == NULL)
		return;
	m_bLBDown = FALSE;
	m_ptCurrentImg = pDcm->Screen2Image(&point);
	const int shape = SeMPRView::GetRoiShape();
	if (shape != ROI_SHAPE_ANY)
	{
		BuildRoiShapePolygon(shape, m_ptStartImg, m_ptCurrentImg, m_vecPtsTmp);
	}
	if (m_vecPtsTmp.size() < 2)
	{
		m_vecPtsTmp.clear();
		pView->Invalidate(FALSE);
		pView->UpdateWindow();
		return;
	}
	if(pView->m_pVecEdge != NULL)
	{
		int nPlanePos = pView->GetAxialPos();
		pView->m_vecPoid.push_back(nPlanePos);
		if(m_bShiftDown || pView->m_pVecEdge[nPlanePos].size() == 0)//shift �ػ�
		{
			pView->m_pVecEdge[nPlanePos].insert(pView->m_pVecEdge[nPlanePos].begin(), m_vecPtsTmp.begin(), m_vecPtsTmp.end());
		}
		if(!m_bShiftDown && !m_bAltDown && m_ScopeTool.Intersection(pView->m_pVecEdge[nPlanePos], m_vecPtsTmp))//Ĭ������
		{
			vector <CPoint> pts;
			m_ScopeTool.Operation(UNION, pView->m_pVecEdge[nPlanePos], m_vecPtsTmp, pts);
			pView->m_pVecEdge[nPlanePos].clear();
			pView->m_pVecEdge[nPlanePos].insert(pView->m_pVecEdge[nPlanePos].begin(), pts.begin(), pts.end());
		}
		if(!m_bShiftDown && m_bAltDown && m_ScopeTool.Intersection(pView->m_pVecEdge[nPlanePos], m_vecPtsTmp))//alt ����
		{
			vector <CPoint> pts;
			m_ScopeTool.Operation(DIFF, pView->m_pVecEdge[nPlanePos], m_vecPtsTmp, pts);
			pView->m_pVecEdge[nPlanePos].clear();
			pView->m_pVecEdge[nPlanePos].insert(pView->m_pVecEdge[nPlanePos].begin(), pts.begin(), pts.end());
		}
	}
	m_vecPtsTmp.clear();
	pView->Invalidate(FALSE);
	pView->UpdateWindow();
}

void SeROIMouseTool::OnMouseMove(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	SeMPRView* pView = (SeMPRView*)pWnd;
	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if (pDcm != NULL)
	{
		CPoint ptImg = pDcm->Screen2Image(&point);
		m_ptCurrentImg = ptImg;
		if (m_bLBDown)
		{
			const int shape = SeMPRView::GetRoiShape();
			if (shape == ROI_SHAPE_ANY)
			{
				m_vecPtsTmp.push_back(ptImg);
			}
			else
			{
				const bool forceRegular = ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) && ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0);
				BuildRoiShapePolygon(shape, m_ptStartImg, m_ptCurrentImg, m_vecPtsTmp, forceRegular);
			}
		}
	}
	pView->Invalidate(FALSE);
	pView->UpdateWindow();
}

HCURSOR SeROIMouseTool::LoadCursor()
{
	return AfxGetApp()->LoadCursor(IDC_DRAWLINECOR);
}

// void SeROIMouseTool::DrawLine(vector <CPoint> vecPts, Color color, int nWidth, Graphics* gc, CDcmPic* pDcm)
// {
// 	int nSize = (int)vecPts.size();
// 	Pen		MarkPen(color,nWidth);
// 	Point*	pMarkpt = new Point[nSize];
// 	for (int i = 0 ; i < nSize ; i++)
// 	{
// 		CPoint ptMark = pDcm->Image2Screen(&vecPts[i]);
// 		pMarkpt[i].X = ptMark.x;
// 		pMarkpt[i].Y = ptMark.y;
// 	}
// 	gc->DrawLines(&MarkPen, pMarkpt, nSize);
// 	gc->DrawLine(&MarkPen, pMarkpt[0], pMarkpt[nSize - 1]);
// 	delete []pMarkpt;
// }
// 
// void SeROIMouseTool::DrawRegion(vector <CPoint> vecPts, SolidBrush brush, Graphics* gc, CDcmPic* pDcm)
// {
// 	GraphicsPath path;
// 	int nSize = (int)vecPts.size();
// 	if(nSize > 0)
// 	{
// 		PointF* pPtF = new PointF[nSize];
// 		for (UINT i=0;i<nSize;i++)
// 		{
// 			CPoint pt = pDcm->Image2Screen(&vecPts[i]);
// 			pPtF[i].X = pt.x;
// 			pPtF[i].Y = pt.y;
// 		}
// 		path.AddLines(pPtF, nSize);
// 		path.CloseFigure();		
// 		delete[] pPtF;
// 	}
// 	Region region(&path);
// 	gc->FillRegion(&brush, &region);
// }


SeMPRMouseTool::SeMPRMouseTool(): CMouseTool(MT_MPR)
{
	m_bShiftDown = FALSE;
	m_bLBDown = FALSE;
	
}

SeMPRMouseTool::~SeMPRMouseTool()
{

}

void SeMPRMouseTool::OnLButtonDown(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	SeMPRView* pView = (SeMPRView*)pWnd;
	m_nPlaneNum = pView->GetPlaneNum();

	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if (pDcm == NULL)
	{
		pView->SetViewPos(point);
		return;
	}
	CRect rect = pDcm->GetImageDispRect();
	if(!rect.PtInRect(point))
		return;
	m_bLBDown = TRUE;
}

void SeMPRMouseTool::OnLButtonUp(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	m_bLBDown = FALSE;
}

void SeMPRMouseTool::OnMouseMove(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	SeMPRView* pView = (SeMPRView*)pWnd;
	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if (pDcm != NULL && m_bLBDown/* && m_bShiftDown*/)
	{
		CPoint pt = pDcm->Screen2Image(&point);
		pView->SetViewPos(pt);
	}
}

HCURSOR SeMPRMouseTool::LoadCursor()
{
	return AfxGetApp()->LoadCursor(IDC_MOVECUR);
}

void SeMPRMouseTool::Draw(CWnd* pWnd, CDC *pDC, CRect& rt)
{
	SeMPRView* pView = (SeMPRView*)pWnd;
	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if(pDcm == NULL)
		return;

	Graphics gc(pDC->m_hDC);
	gc.SetSmoothingMode(SmoothingModeHighQuality);

	CRect   rect;
	pView->GetClientRect(&rect);
	Color	colorRed(255, 255, 0, 0);
	Pen     penRed(colorRed, 1.0);
	Color	colorGreen(255, 0, 255, 0);
	Pen     penGreen(colorGreen, 1.0);
	Color	colorBlue(255, 0, 0, 255);
	Pen     penBlue(colorBlue, 1.0);
	Color	colorYellow(255, 255, 255, 0);
	Pen     penYellow(colorYellow, 1.0);
	CRect	rtDisplay = pDcm->GetImageDispRect();
	

	switch(pView->GetPlaneNum())
	{
	case 1:
		{
			CPoint ptScreen = pDcm->Image2Screen(&CPoint(pView->m_nXPos, pView->m_nYPos));
			gc.DrawRectangle(&penBlue, rtDisplay.left, rtDisplay.top, rtDisplay.Width(), rtDisplay.Height());
			gc.DrawLine(&penRed,	Point(ptScreen.x, rtDisplay.top),	Point(ptScreen.x, rtDisplay.bottom));
			gc.DrawLine(&penGreen,	Point(rtDisplay.left, ptScreen.y),	Point(rtDisplay.right, ptScreen.y));
			break;
		}
	case 2:
		{
			CPoint ptScreen = pDcm->Image2Screen(&CPoint(pView->m_nYPos, pView->m_nZPos));
			gc.DrawRectangle(&penRed, rtDisplay.left, rtDisplay.top, rtDisplay.Width(), rtDisplay.Height());
			gc.DrawLine(&penGreen,	Point(ptScreen.x, rtDisplay.top),	Point(ptScreen.x, rtDisplay.bottom));
			gc.DrawLine(&penBlue,	Point(rtDisplay.left, ptScreen.y),	Point(rtDisplay.right, ptScreen.y));			
			break;
		}
	case 3:
		{
			CPoint ptScreen = pDcm->Image2Screen(&CPoint(pView->m_nXPos, pView->m_nZPos));
			gc.DrawRectangle(&penGreen, rtDisplay.left, rtDisplay.top, rtDisplay.Width(), rtDisplay.Height());
			gc.DrawLine(&penBlue,	Point(rtDisplay.left,ptScreen.y),	Point(rtDisplay.right, ptScreen.y));
			gc.DrawLine(&penRed,	Point(ptScreen.x, rtDisplay.top),	Point(ptScreen.x, rtDisplay.bottom));
			break;
		}
	default:
		{
			break;
		}
	}
}

SeFloodFillTool::SeFloodFillTool() : CMouseTool(MT_FLOODFILL)
{
}

SeFloodFillTool::~SeFloodFillTool()
{

}

void SeFloodFillTool::Draw(CWnd* pWnd, CDC *pDC, CRect& rt)
{

}

void SeFloodFillTool::OnLButtonDown(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	SeMPRView* pView = (SeMPRView*)pWnd;
	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if (pDcm == NULL)
		return;
	CPoint pt = pDcm->Screen2Image(&point);
	pView->SetPosition(pt);
	pView->FloodFill();
}

void SeFloodFillTool::OnLButtonUp(CWnd* pWnd, UINT nFlags, CPoint& point)
{

}

void SeFloodFillTool::OnMouseMove(CWnd* pWnd, UINT nFlags, CPoint& point)
{

}

HCURSOR SeFloodFillTool::LoadCursor()
{
	return AfxGetApp()->LoadCursor(IDC_CROSS);
}


SeMeasureLineTool::SeMeasureLineTool() : CMouseTool(MT_MEASURE_LINE)
{
	m_bLBDown = FALSE;
}

SeMeasureLineTool::~SeMeasureLineTool()
{

}

void SeMeasureLineTool::Draw(CWnd* pWnd, CDC *pDC, CRect& rt)
{
	DrawTools(pWnd, pDC, rt);
	SeMPRView* pView = (SeMPRView*)pWnd;
	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if(pDcm == NULL || !m_bLBDown)
		return;

	Graphics gc(pDC->m_hDC);
	gc.SetSmoothingMode(SmoothingModeHighQuality);
	Color	colorFocus(255, 255, 97, 0);
	Color   colorYellow(255, 255, 255, 0);
	Color	colorGreen(255, 0, 255, 0);
	Pen     penGreen(colorGreen, 1.0);
	SolidBrush   brushYellow(colorYellow);
	gc.DrawLine(&penGreen, PointF(m_ptStart.x, m_ptStart.y), PointF(m_ptEnd.x, m_ptEnd.y));
	gc.FillRectangle(&brushYellow, m_ptStart.x - 2, m_ptStart.y - 2, 5, 5);
	gc.FillRectangle(&brushYellow, m_ptEnd.x - 2, m_ptEnd.y - 2, 5, 5);
}

void SeMeasureLineTool::OnLButtonDown(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	SeMPRView* pView = (SeMPRView*)pWnd;
	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if (pDcm == NULL)
		return;

	m_bLBDown = TRUE;
	CRect rect;
	pWnd->GetWindowRect(&rect);
	ClipCursor(rect);

	int nPlanePos = pView->m_nCurrentFrame;
	m_ptStart = point;
}

void SeMeasureLineTool::OnLButtonUp(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	SeMPRView* pView = (SeMPRView*)pWnd;
	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if (pDcm == NULL)
		return;

	m_bLBDown = FALSE;
	ClipCursor(NULL);
	CMeaLine* line = new CMeaLine();
	line->m_nSliceNumber = pView->m_nCurrentFrame;
	line->m_nSelected = FALSE;
	line->m_dPixelSize = pDcm->GetMMPerpixel();
	line->m_ptStart = pDcm->Screen2Image(&m_ptStart);
	line->m_ptEnd = pDcm->Screen2Image(&m_ptEnd);
	line->pDcm = pDcm;
	line->GetMeasurement();
	pView->AddResult(dynamic_cast<CMeasurement*>(line));
/*	pView->UpdateResult();*/
	pView->SetSelectTool();
	pView->Invalidate(FALSE);
	pView->UpdateWindow();
}

void SeMeasureLineTool::OnMouseMove(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	SeMPRView* pView = (SeMPRView*)pWnd;
	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if (pDcm != NULL && m_bLBDown)
	{
		m_ptEnd = point;
		pView->Invalidate(FALSE);
		pView->UpdateWindow();
	}
}

HCURSOR SeMeasureLineTool::LoadCursor()
{
	return AfxGetApp()->LoadCursor(IDC_DRAWLINECOR);
}

SeMeasureAngleTool::SeMeasureAngleTool() : CMouseTool(MT_MEASURE_ANGLE)
{
	m_bLBDown = FALSE;
	m_bFirstClick = FALSE;
	m_bSecondClick = FALSE;
	m_bThirdClick = FALSE;
}

SeMeasureAngleTool::~SeMeasureAngleTool()
{

}

void SeMeasureAngleTool::Draw(CWnd* pWnd, CDC *pDC, CRect& rt)
{
	DrawTools(pWnd, pDC, rt);
	SeMPRView* pView = (SeMPRView*)pWnd;
	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if(pDcm == NULL)
		return;

	Graphics gc(pDC->m_hDC);
	gc.SetSmoothingMode(SmoothingModeHighQuality);
	Color	colorFocus(255, 255, 97, 0);
	Color   colorYellow(255, 255, 255, 0);
	Color	colorGreen(255, 0, 255, 0);
	Pen     penGreen(colorGreen, 1.0);
	SolidBrush   brushYellow(colorYellow);

	if (m_bFirstClick) {
		gc.FillRectangle(&brushYellow, m_ptStart.x - 2, m_ptStart.y - 2, 5, 5);
	}
	else if(m_bSecondClick) {
		gc.DrawLine(&penGreen, PointF(m_ptStart.x, m_ptStart.y), PointF(m_ptAngle.x, m_ptAngle.y));
		gc.FillRectangle(&brushYellow, m_ptStart.x - 2, m_ptStart.y - 2, 5, 5);
		gc.FillRectangle(&brushYellow, m_ptAngle.x - 2, m_ptAngle.y - 2, 5, 5);
	}
	else if (m_bThirdClick) {
		gc.DrawLine(&penGreen, PointF(m_ptStart.x, m_ptStart.y), PointF(m_ptAngle.x, m_ptAngle.y));
		gc.DrawLine(&penGreen, PointF(m_ptAngle.x, m_ptAngle.y), PointF(m_ptEnd.x, m_ptEnd.y));
		gc.FillRectangle(&brushYellow, m_ptStart.x - 2, m_ptStart.y - 2, 5, 5);
		gc.FillRectangle(&brushYellow, m_ptAngle.x - 2, m_ptAngle.y - 2, 5, 5);
		gc.FillRectangle(&brushYellow, m_ptEnd.x - 2, m_ptEnd.y - 2, 5, 5);
	}
}

void SeMeasureAngleTool::OnLButtonDown(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	SeMPRView* pView = (SeMPRView*)pWnd;
	m_bLBDown = TRUE;
	CRect rect;
	pWnd->GetWindowRect(&rect);
	ClipCursor(rect);

	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if (pDcm == NULL)
		return;
	int nPlanePos = pView->m_nCurrentFrame;
	if (!m_bFirstClick && !m_bSecondClick && ! m_bThirdClick)
		m_bFirstClick = TRUE;
// 	if(m_bFirstClick)
// 		m_ptStart = point;
// 	else if(m_bSecondClick)
// 		m_ptAngle = point;
// 	else 
// 		m_ptEnd = point;
}

void SeMeasureAngleTool::OnLButtonUp(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	SeMPRView* pView = (SeMPRView*)pWnd;
	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if(pDcm == NULL || !m_bLBDown)
		return;

	if(m_bFirstClick)
	{
		m_ptStart = point;
		m_bSecondClick = TRUE;
		m_bFirstClick = FALSE;
	}
	else if (m_bSecondClick) 
	{
		m_ptAngle = point;
		m_bSecondClick = FALSE;
		m_bThirdClick = TRUE;
	}
	else if (m_bThirdClick)
	{
		m_ptEnd = point;
		m_bLBDown = FALSE;
		m_bThirdClick = FALSE;

		CMeaAngle* angle = new CMeaAngle();
		angle->m_nSliceNumber = pView->m_nCurrentFrame;
		angle->m_nSelected = FALSE;
		angle->m_dPixelSize = pDcm->GetMMPerpixel();
		angle->m_ptStart = pDcm->Screen2Image(&m_ptStart);
		angle->m_ptAngle = pDcm->Screen2Image(&m_ptAngle);
		angle->m_ptEnd = pDcm->Screen2Image(&m_ptEnd);
		angle->GetMeasurement();
		pView->AddResult(dynamic_cast<CMeasurement*>(angle));
/*		pView->UpdateResult();*/
		pView->SetSelectTool();
		ClipCursor(NULL);
		pView->Invalidate(FALSE);
		pView->UpdateWindow();
	}
}

void SeMeasureAngleTool::OnMouseMove(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	SeMPRView* pView = (SeMPRView*)pWnd;
	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if (pDcm != NULL && m_bLBDown)
	{
		if(m_bFirstClick)
			m_ptStart = point;
		else if (m_bSecondClick)
			m_ptAngle = point;
		else if(m_bThirdClick)
			m_ptEnd = point;
		pView->Invalidate(FALSE);
		pView->UpdateWindow();
	}
}

HCURSOR SeMeasureAngleTool::LoadCursor()
{
	return AfxGetApp()->LoadCursor(IDC_DRAWANGLECUR);
}

SeMeasureShapeTool::SeMeasureShapeTool() : CMouseTool(MT_MEASURE_SHAPE)
{

}

SeMeasureShapeTool::~SeMeasureShapeTool()
{

}

void SeMeasureShapeTool::Draw(CWnd* pWnd, CDC *pDC, CRect& rt)
{
	DrawTools(pWnd, pDC, rt);
	SeMPRView* pView = (SeMPRView*)pWnd;
	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if(pDcm == NULL || !m_bLBDown)
		return;

	Graphics gc(pDC->m_hDC);
	gc.SetSmoothingMode(SmoothingModeHighQuality);
	Color	colorFocus(255, 255, 97, 0);
	Color   colorYellow(255, 255, 255, 0);
	Color	colorGreen(255, 0, 255, 0);
	Pen     penGreen(colorGreen, 1.0);
	SolidBrush   brushYellow(colorYellow);
	int x = m_ptStart.x < m_ptEnd.x ? m_ptStart.x : m_ptEnd.x;
	int y = m_ptStart.y < m_ptEnd.y ? m_ptStart.y : m_ptEnd.y;
	gc.DrawRectangle(&penGreen, x, y, abs(m_ptStart.x - m_ptEnd.x), abs(m_ptStart.y - m_ptEnd.y));
	gc.FillRectangle(&brushYellow, m_ptStart.x - 2, m_ptStart.y - 2, 5, 5);
	gc.FillRectangle(&brushYellow, m_ptEnd.x - 2, m_ptEnd.y - 2, 5, 5);
	gc.FillRectangle(&brushYellow, m_ptStart.x - 2, m_ptEnd.y - 2, 5, 5);
	gc.FillRectangle(&brushYellow, m_ptEnd.x - 2, m_ptStart.y - 2, 5, 5);
}

void SeMeasureShapeTool::OnLButtonDown(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	SeMPRView* pView = (SeMPRView*)pWnd;
	m_bLBDown = TRUE;

	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if (pDcm == NULL)
		return;
	int nPlanePos = pView->m_nCurrentFrame;
	m_ptStart = point;

	CRect rect;
	pWnd->GetWindowRect(&rect);
	ClipCursor(rect);
}

void SeMeasureShapeTool::OnLButtonUp(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	SeMPRView* pView = (SeMPRView*)pWnd;
	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if (pDcm == NULL)
		return;

	m_bLBDown = FALSE;
	ClipCursor(NULL);

	CMeaShape* rect = new CMeaShape();
	rect->m_nSliceNumber = pView->m_nCurrentFrame;
	rect->m_nSelected = FALSE;
	rect->m_dPixelSize = pDcm->GetMMPerpixel();
	rect->m_ptTopLeft = pDcm->Screen2Image(&m_ptStart);
	rect->m_ptBottonRight = pDcm->Screen2Image(&m_ptEnd);
	rect->pDcm = pDcm;
	rect->GetMeasurement();
	pView->AddResult(dynamic_cast<CMeasurement*>(rect));
/*	pView->UpdateResult();*/
	pView->SetSelectTool();
	pView->Invalidate(FALSE);
	pView->UpdateWindow();
}

void SeMeasureShapeTool::OnMouseMove(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	SeMPRView* pView = (SeMPRView*)pWnd;
	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if (pDcm != NULL && m_bLBDown)
	{
		m_ptEnd = point;
		pView->Invalidate(FALSE);
		pView->UpdateWindow();
	}
}

HCURSOR SeMeasureShapeTool::LoadCursor()
{
	return AfxGetApp()->LoadCursor(IDC_DRAWRECTCUR);
}

SeMeasureCTTool::SeMeasureCTTool() : CMouseTool(MT_MEASURE_CT)
{

}

SeMeasureCTTool::~SeMeasureCTTool()
{
	m_nPlanenumber = -1;
}

void SeMeasureCTTool::Draw(CWnd* pWnd, CDC *pDC, CRect& rt)
{
	DrawTools(pWnd, pDC, rt);
	SeMPRView* pView = (SeMPRView*)pWnd;
	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if(pDcm == NULL && m_nPlanenumber == pView->GetPlaneNum())
		return;

	CRect rect;
	pWnd->GetWindowRect(&rect);
	CPoint ptCursor;
	GetCursorPos(&ptCursor);
	if(!rect.PtInRect(ptCursor))
		return;

	Graphics gc(pDC->m_hDC);
	gc.SetSmoothingMode(SmoothingModeHighQuality);
	Color	colorFocus(255, 255, 97, 0);
	Color   colorYellow(255, 255, 255, 0);
	Color	colorGreen(255, 0, 255, 0);
	Color   colorLightYellow(255, 248, 243, 205);
	Pen     penGreen(colorGreen, 1.0);
	SolidBrush   brushYellow(colorYellow);
	SolidBrush   brushLightYellow(colorLightYellow);

	FontFamily fontFamily(L"��Բ");
	Gdiplus::Font font(&fontFamily, 16, FontStyleRegular, UnitPixel);
	SolidBrush brush(Color(255, 0, 0, 0));



	int xPos = m_ptPosition.x < rect.Width() - 110 ? m_ptPosition.x : m_ptPosition.x - 130; 
	int yPos = m_ptPosition.y < rect.Height() - 40 ? m_ptPosition.y : m_ptPosition.y - 50;

	Rect rectShow(xPos + 10, yPos + 5, 110, 35);
	gc.FillRectangle(&brushLightYellow, rectShow);
	CPoint pt = pDcm->Screen2Image(&m_ptPosition);
	int nWidth = pDcm->GetWidth();
	int nHeight = pDcm->GetHeight();
	int value = pDcm->GetPixelData(pt.x, pt.y);
	if (pt.x < 0 || pt.y < 0 || pt.x >= nWidth || pt.y >= nHeight)
		value = 0;	

	CStringW str;
	str.Format(L"X:%d Y:%d\r\nValue:%d", ptCursor.x, ptCursor.y, value);
	gc.DrawString(str, -1, &font, PointF(xPos + 10, yPos + 5), &brush);
}

void SeMeasureCTTool::OnLButtonDown(CWnd* pWnd, UINT nFlags, CPoint& point)
{

}

void SeMeasureCTTool::OnLButtonUp(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	SeMPRView* pView = (SeMPRView*)pWnd;
	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if (pDcm == NULL)
		return;

	CMeaCT* CT= new CMeaCT();
	CT->m_nSliceNumber = pView->m_nCurrentFrame;
	CT->m_nSelected = FALSE;
	CT->m_dPixelSize = pDcm->GetMMPerpixel();
	CT->m_ptPostion = pDcm->Screen2Image(&m_ptPosition);
	CT->pDcm = pDcm;
	CT->GetMeasurement();
	pView->AddResult(dynamic_cast<CMeasurement*>(CT));
	/*	pView->UpdateResult();*/
	pView->SetSelectTool();
	pView->Invalidate(FALSE);
	pView->UpdateWindow();
}

void SeMeasureCTTool::OnMouseMove(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	SeMPRView* pView = (SeMPRView*)pWnd;
	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if (pDcm == NULL)
		return;
	m_ptPosition = point;
	if(m_nPlanenumber != pView->GetPlaneNum())
	{
		m_nPlanenumber = pView->GetPlaneNum();
		pView->UpdateView();
	}
	else
	{
		pView->Invalidate(FALSE);
		pView->UpdateWindow();
	}	
}

HCURSOR SeMeasureCTTool::LoadCursor()
{
	return AfxGetApp()->LoadCursor(IDC_CROSS);
}

SeMeasureAreaTool::SeMeasureAreaTool() : CMouseTool(MT_MEASURE_AREA)
{
		m_bLBDown = FALSE;
}

SeMeasureAreaTool::~SeMeasureAreaTool()
{

}

void SeMeasureAreaTool::Draw(CWnd* pWnd, CDC *pDC, CRect& rt)
{
	DrawTools(pWnd, pDC, rt);
	SeMPRView* pView = (SeMPRView*)pWnd;
	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if(pDcm == NULL)
		return;

	Graphics gc(pDC->m_hDC);
	gc.SetSmoothingMode(SmoothingModeHighQuality);
	Color	colorFocus(255, 255, 97, 0);

	DrawLine(m_vecPtsTmp, colorFocus, 1, &gc, pDcm);
	DrawRegion(m_vecPtsTmp, Color(30, 0, 255, 0), &gc, pDcm);

	CRect   rect;
	pView->GetClientRect(&rect);
	rect.InflateRect(0, 0, -2, -1);
	Color	colorGreen(255, 0, 255, 0);
	Pen     penGreen(colorGreen, 2.0);
	gc.DrawRectangle(&penGreen, rect.left, rect.top, rect.Width(), rect.Height());
}

void SeMeasureAreaTool::OnLButtonDown(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	SeMPRView* pView = (SeMPRView*)pWnd;
	m_bLBDown = TRUE;

	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if (pDcm == NULL)
		return;

	m_vecPtsTmp.clear();
}

void SeMeasureAreaTool::OnLButtonUp(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	SeMPRView* pView = (SeMPRView*)pWnd;
	CDcmPic *pDcm = (CDcmPic *)pView->GetDisplayMgr()->GetCurrentImage();
	if(pDcm == NULL)
		return;

	CMeaArea* area = new CMeaArea();
	area->m_nSliceNumber = pView->m_nCurrentFrame;
	area->m_nSelected = FALSE;
	area->m_dPixelSize = pDcm->GetMMPerpixel();
	area->m_vecPoints.insert(area->m_vecPoints.begin(), m_vecPtsTmp.begin(), m_vecPtsTmp.end());
	area->pDcm = pDcm;
	area->GetMeasurement();
	pView->AddResult(dynamic_cast<CMeasurement*>(area));
/*	pView->UpdateResult();*/
	m_vecPtsTmp.clear();
	m_bLBDown = FALSE;
	pView->SetSelectTool();
	pView->Invalidate(FALSE);
	pView->UpdateWindow();
}

void SeMeasureAreaTool::OnMouseMove(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	SeMPRView* pView = (SeMPRView*)pWnd;
	CDcmPic* pDcm = (CDcmPic*)pView->GetDisplayMgr()->GetCurrentImage();
	if (pDcm != NULL && m_bLBDown)
	{
		CPoint ptImg = pDcm->Screen2Image(&point);
		m_vecPtsTmp.push_back(ptImg);
		pView->Invalidate(FALSE);
		pView->UpdateWindow();
	}
}

HCURSOR SeMeasureAreaTool::LoadCursor()
{
	return AfxGetApp()->LoadCursor(IDC_DRAWPOLYGONECUR);
}

// void SeMeasureAreaTool::DrawLine(vector <CPoint> vecPts, Color color, int nWidth, Graphics* gc, CDcmPic* pDcm)
// {
// 	int nSize = (int)vecPts.size();
// 	Pen		MarkPen(color,nWidth);
// 	Point*	pMarkpt = new Point[nSize];
// 	for (int i = 0 ; i < nSize ; i++)
// 	{
// 		CPoint ptMark = pDcm->Image2Screen(&vecPts[i]);
// 		pMarkpt[i].X = ptMark.x;
// 		pMarkpt[i].Y = ptMark.y;
// 	}
// 	gc->DrawLines(&MarkPen, pMarkpt, nSize);
// 	gc->DrawLine(&MarkPen, pMarkpt[0], pMarkpt[nSize - 1]);
// 	delete []pMarkpt;
// }
// 
// void SeMeasureAreaTool::DrawRegion(vector <CPoint> vecPts, SolidBrush brush, Graphics* gc, CDcmPic* pDcm)
// {
// 	GraphicsPath path;
// 	int nSize = (int)vecPts.size();
// 	if(nSize > 0)
// 	{
// 		PointF* pPtF = new PointF[nSize];
// 		for (UINT i=0;i<nSize;i++)
// 		{
// 			CPoint pt = pDcm->Image2Screen(&vecPts[i]);
// 			pPtF[i].X = pt.x;
// 			pPtF[i].Y = pt.y;
// 		}
// 		path.AddLines(pPtF, nSize);
// 		path.CloseFigure();		
// 		delete[] pPtF;
// 	}
// 	Region region(&path);
// 	gc->FillRegion(&brush, &region);
// }

SeEllipseTool::SeEllipseTool() : CMouseTool(MT_MEASURE_ELLIPSE)
{
	m_bLBDown = FALSE;
	m_bShiftDown = FALSE;
}

SeEllipseTool::~SeEllipseTool()
{
}

void SeEllipseTool::Draw(CWnd* pWnd, CDC* pDC, CRect& rt)
{
	DrawTools(pWnd, pDC, rt);
	SeMPRView* pView = static_cast<SeMPRView*>(pWnd);
	CDcmPic* pDcm = static_cast<CDcmPic*>(pView->GetDisplayMgr()->GetCurrentImage());
	if (pDcm == NULL || !m_bLBDown)
		return;

	Graphics gc(pDC->m_hDC);
	gc.SetSmoothingMode(SmoothingModeHighQuality);

	Color colorGreen(255, 0, 255, 0);
	Color colorYellow(255, 255, 255, 0);
	Pen penGreen(colorGreen, 2.0f);
	SolidBrush brushYellow(colorYellow);

	CPoint ptStart = m_ptStart;
	CPoint ptEnd = m_ptEnd;
	if (m_bShiftDown)
	{
		const int dx = ptEnd.x - ptStart.x;
		const int dy = ptEnd.y - ptStart.y;
		const int radius = min(abs(dx), abs(dy));
		ptEnd.x = ptStart.x + (dx >= 0 ? radius : -radius);
		ptEnd.y = ptStart.y + (dy >= 0 ? radius : -radius);
	}

	const int left = min(ptStart.x, ptEnd.x);
	const int top = min(ptStart.y, ptEnd.y);
	const int width = abs(ptEnd.x - ptStart.x);
	const int height = abs(ptEnd.y - ptStart.y);

	if (width <= 0 || height <= 0)
		return;

	gc.DrawEllipse(&penGreen, left, top, width, height);
	gc.FillRectangle(&brushYellow, ptStart.x - 2, ptStart.y - 2, 5, 5);
	gc.FillRectangle(&brushYellow, ptEnd.x - 2, ptEnd.y - 2, 5, 5);
}

void SeEllipseTool::OnLButtonDown(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	SeMPRView* pView = static_cast<SeMPRView*>(pWnd);
	CDcmPic* pDcm = static_cast<CDcmPic*>(pView->GetDisplayMgr()->GetCurrentImage());
	if (pDcm == NULL)
		return;

	m_bLBDown = TRUE;
	m_ptStart = point;
	m_ptEnd = point;
	m_bShiftDown = (nFlags & MK_SHIFT) == MK_SHIFT;

	CRect rect;
	pWnd->GetWindowRect(&rect);
	ClipCursor(rect);
}

void SeEllipseTool::OnLButtonUp(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	SeMPRView* pView = static_cast<SeMPRView*>(pWnd);
	CDcmPic* pDcm = static_cast<CDcmPic*>(pView->GetDisplayMgr()->GetCurrentImage());
	if (pDcm == NULL)
		return;

	m_bLBDown = FALSE;
	ClipCursor(NULL);

	m_bShiftDown = (nFlags & MK_SHIFT) == MK_SHIFT;

	CPoint ptStartScreen = m_ptStart;
	CPoint ptEndScreen = m_ptEnd;
	if (m_bShiftDown)
	{
		const int dx = ptEndScreen.x - ptStartScreen.x;
		const int dy = ptEndScreen.y - ptStartScreen.y;
		const int radius = min(abs(dx), abs(dy));
		ptEndScreen.x = ptStartScreen.x + (dx >= 0 ? radius : -radius);
		ptEndScreen.y = ptStartScreen.y + (dy >= 0 ? radius : -radius);
	}

	CMeaEllipse* ellipse = new CMeaEllipse();
	ellipse->m_nSliceNumber = pView->m_nCurrentFrame;
	ellipse->m_nSelected = FALSE;
	ellipse->m_dPixelSize = pDcm->GetMMPerpixel();
	ellipse->m_bIsCircle = m_bShiftDown;
	ellipse->m_ptStart = pDcm->Screen2Image(&ptStartScreen);
	ellipse->m_ptEnd = pDcm->Screen2Image(&ptEndScreen);
	ellipse->pDcm = pDcm;
	ellipse->GetMeasurement();

	pView->AddResult(dynamic_cast<CMeasurement*>(ellipse));
	pView->SetSelectTool();
	pView->Invalidate(FALSE);
	pView->UpdateWindow();
}

void SeEllipseTool::OnMouseMove(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	if (!m_bLBDown)
		return;

	m_ptEnd = point;
	m_bShiftDown = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

	SeMPRView* pView = static_cast<SeMPRView*>(pWnd);
	pView->Invalidate(FALSE);
	pView->UpdateWindow();
}

HCURSOR SeEllipseTool::LoadCursor()
{
	return AfxGetApp()->LoadCursor(IDC_DRAWELLIPSECUR);
}

SeMeasureSelectTool::SeMeasureSelectTool() : CMouseTool(MT_MEASURE_SELECT)
{
	m_bLBDown = FALSE;
	m_bPointSelected = FALSE;
	m_bRectSelected = FALSE;
	m_ptrSelectedTool = NULL;
	m_nSelectedIndex = -1;

}

SeMeasureSelectTool::~SeMeasureSelectTool()
{

}

void SeMeasureSelectTool::Draw(CWnd* pWnd, CDC *pDC, CRect& rt)
{
	DrawTools(pWnd, pDC, rt);
}

void SeMeasureSelectTool::OnLButtonDown(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	SeMPRView* pView = (SeMPRView*)pWnd;
	CDcmPic *pDcm = (CDcmPic *)pView->GetDisplayMgr()->GetCurrentImage();
	if(pDcm == NULL)
		return;

	CRect rect;
	pWnd->GetWindowRect(&rect);
	ClipCursor(rect);

	m_bLBDown = TRUE;
	int nCurrentFrame = pView->m_nCurrentFrame;
	vector<CMeasurement*> vecTools = pView->GetResult();
	const int nToolCount = static_cast<int>(vecTools.size());
	for (int i = nToolCount - 1; i >= 0; --i)
	{
		if (vecTools[i] == NULL || vecTools[i]->m_nPlaneNumber != pView->GetPlaneNum() || vecTools[i]->m_nSliceNumber != nCurrentFrame) continue;
		CString strClassName = vecTools[i]->m_strToolName;
		if (strClassName == "Line") {
			CMeaLine* pTool = dynamic_cast<CMeaLine*>(vecTools[i]);
			CPoint ptStart = pDcm->Image2Screen(&pTool->m_ptStart);
			CPoint ptEnd = pDcm->Image2Screen(&pTool->m_ptEnd);
			CRect rect(ptStart, ptEnd);
			rect.NormalizeRect();
			if (abs(point.x - ptStart.x) < 5 && abs(point.y - ptStart.y) < 5)
			{
				m_bPointSelected = TRUE;
				m_ptrSelectedTool = pTool;
				m_pSelectedPoint = &pTool->m_ptStart;
				m_nSelectedIndex = i;
				return;
			}
			else if (abs(point.x - ptEnd.x) < 5 && abs(point.y - ptEnd.y) < 5)
			{
				m_bPointSelected = TRUE;
				m_ptrSelectedTool = pTool;
				m_pSelectedPoint = &pTool->m_ptEnd;
				m_nSelectedIndex = i;
				return;
			}
			else if (rect.PtInRect(point))
			{
				m_bRectSelected = TRUE;
				m_ptrSelectedTool = pTool;
				m_ptOri = point;
				m_nSelectedIndex = i;
				return;
			}
		}
		else if (strClassName == "Angle") {
			CMeaAngle* pTool = dynamic_cast<CMeaAngle*>(vecTools[i]);
			CPoint ptStart = pDcm->Image2Screen(&pTool->m_ptStart);
			CPoint ptAngle = pDcm->Image2Screen((&pTool->m_ptAngle));
			CPoint ptEnd = pDcm->Image2Screen(&pTool->m_ptEnd);
			int xMin = ptStart.x, xMax = ptStart.x, yMin = ptStart.y, yMax = ptStart.y;
			if (ptAngle.x < xMin) xMin = ptAngle.x;
			else if(ptAngle.x > xMax) xMax = ptAngle.x;
			if (ptEnd.x < xMin) xMin = ptEnd.x;
			else if (ptEnd.x > xMax) xMax = ptEnd.x;
			if (ptAngle.y < yMin) yMin = ptAngle.y;
			else if(ptAngle.y > yMax) yMax = ptAngle.y;
			if (ptEnd.y < yMin) yMin = ptEnd.y;
			else if (ptEnd.y > yMax) yMax = ptEnd.y;
			CRect rect(xMin, yMin, xMax, yMax);
			rect.NormalizeRect();
			if (abs(point.x - ptStart.x) < 5 && abs(point.y - ptStart.y) < 5)
			{
				m_bPointSelected = TRUE;
				m_ptrSelectedTool = pTool;
				m_pSelectedPoint = &pTool->m_ptStart;
				m_nSelectedIndex = i;
				return;
			}
			else if (abs(point.x - ptAngle.x) < 5 && abs(point.y - ptAngle.y) < 5)
			{
				m_bPointSelected = TRUE;
				m_ptrSelectedTool = pTool;
				m_pSelectedPoint = &pTool->m_ptAngle;
				m_nSelectedIndex = i;
				return;
			}
			else if (abs(point.x - ptEnd.x) < 5 && abs(point.y - ptEnd.y) < 5)
			{
				m_bPointSelected = TRUE;
				m_ptrSelectedTool = pTool;
				m_pSelectedPoint = &pTool->m_ptEnd;
				m_nSelectedIndex = i;
				return;
			}
			else if (rect.PtInRect(point))
			{
				m_bRectSelected = TRUE;
				m_ptrSelectedTool = pTool;
				m_ptOri = point;
				m_nSelectedIndex = i;
				return;
			}
		}
		else if (strClassName == "Shape") {
			CMeaShape* pTool = dynamic_cast<CMeaShape*>(vecTools[i]);
			CPoint ptStart = pDcm->Image2Screen(&pTool->m_ptTopLeft);
			CPoint ptEnd = pDcm->Image2Screen(&pTool->m_ptBottonRight);
			CRect rect(ptStart, ptEnd);
			rect.NormalizeRect();
			if (abs(point.x - ptStart.x) < 5 && abs(point.y - ptStart.y) < 5)
			{
				m_bPointSelected = TRUE;
				m_ptrSelectedTool = pTool;
				m_pSelectedPoint = &pTool->m_ptTopLeft;
				m_nSelectedIndex = i;
				return;
			}
			else if (abs(point.x - ptEnd.x) < 5 && abs(point.y - ptEnd.y) < 5)
			{
				m_bPointSelected = TRUE;
				m_ptrSelectedTool = pTool;
				m_pSelectedPoint = &pTool->m_ptBottonRight;
				m_nSelectedIndex = i;
				return;
			}
			else if (abs(point.x - ptEnd.x) < 5 && abs(point.y - ptStart.y) < 5)
			{
				CPoint ptTmp = pTool->m_ptTopLeft;		
				pTool->m_ptTopLeft.y = pTool->m_ptBottonRight.y;
				pTool->m_ptBottonRight.y = ptTmp.y;

				m_bPointSelected = TRUE;
				m_ptrSelectedTool = pTool;
				m_pSelectedPoint = &pTool->m_ptBottonRight;
				m_nSelectedIndex = i;
				return;
			}
			else if (abs(point.x - ptStart.x) < 5 && abs(point.y - ptEnd.y) < 5)
			{
				CPoint ptTmp = pTool->m_ptTopLeft;		
				pTool->m_ptTopLeft.y = pTool->m_ptBottonRight.y;
				pTool->m_ptBottonRight.y = ptTmp.y;

				m_bPointSelected = TRUE;
				m_ptrSelectedTool = pTool;
				m_pSelectedPoint = &pTool->m_ptTopLeft;
				m_nSelectedIndex = i;
				return;
			}
			else if (rect.PtInRect(point))
			{
				m_bRectSelected = TRUE;
				m_ptrSelectedTool = pTool;
				m_ptOri = point;
				m_nSelectedIndex = i;
				return;
			}
		}
		else if (strClassName == "CT") {
			CMeaCT* pTool = dynamic_cast<CMeaCT*>(vecTools[i]);
			CPoint ptPosition = pDcm->Image2Screen(&pTool->m_ptPostion);
			if (abs(point.x - ptPosition.x) < 5 && abs(point.y - ptPosition.y) < 5)
			{
				m_bPointSelected = TRUE;
				m_ptrSelectedTool = pTool;
				m_pSelectedPoint = &pTool->m_ptPostion;
				m_nSelectedIndex = i;
			}
		}
	}
}

void SeMeasureSelectTool::OnLButtonUp(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	m_bPointSelected = FALSE;
	m_bRectSelected = FALSE;
	m_bLBDown = FALSE;
	m_ptrSelectedTool = NULL;
	ClipCursor(NULL);
}

void SeMeasureSelectTool::OnMouseMove(CWnd* pWnd, UINT nFlags, CPoint& point)
{
	SeMPRView* pView = (SeMPRView*)pWnd;
	CDcmPic *pDcm = (CDcmPic *)pView->GetDisplayMgr()->GetCurrentImage();
	if(pDcm == NULL)
		return;

	if(m_bPointSelected && m_bLBDown) {
		m_pSelectedPoint->x = pDcm->Screen2Image(&point).x;
		m_pSelectedPoint->y = pDcm->Screen2Image(&point).y;
		CString strToolName = m_ptrSelectedTool->m_strToolName;
		if (strToolName == "Line")
		{
			CMeaLine* pLineTool = dynamic_cast<CMeaLine*>(m_ptrSelectedTool);
			if (pLineTool != NULL)
				pLineTool->pDcm = pDcm;
		}
		else if (strToolName == "Area")
		{
			CMeaArea* pAreaTool = dynamic_cast<CMeaArea*>(m_ptrSelectedTool);
			if (pAreaTool != NULL)
				pAreaTool->pDcm = pDcm;
		}
		else if (strToolName == "Ellipse")
		{
			CMeaEllipse* pEllipseTool = dynamic_cast<CMeaEllipse*>(m_ptrSelectedTool);
			if (pEllipseTool != NULL)
				pEllipseTool->pDcm = pDcm;
		}
		else if (strToolName == "Shape")
		{
			CMeaShape* pShapeTool = dynamic_cast<CMeaShape*>(m_ptrSelectedTool);
			if (pShapeTool != NULL)
				pShapeTool->pDcm = pDcm;
		}
		m_ptrSelectedTool->GetMeasurement();
		pView->UpdateResult(m_nSelectedIndex);
		pView->Invalidate(FALSE);
		pView->UpdateWindow();
		return;
	}
	else if (m_bRectSelected && m_bLBDown) {
		CPoint ptMove = pDcm->Screen2Image(&point);
		CPoint ptOri = pDcm->Screen2Image(&m_ptOri);		
		CRect rect;
		pView->GetClientRect(&rect);
		m_ptrSelectedTool->UpdateRect(ptMove - ptOri, pDcm->Screen2Image(&rect));
		m_ptOri = point;
		CString strToolName = m_ptrSelectedTool->m_strToolName;
		if (strToolName == "Line")
		{
			CMeaLine* pLineTool = dynamic_cast<CMeaLine*>(m_ptrSelectedTool);
			if (pLineTool != NULL)
				pLineTool->pDcm = pDcm;
		}
		else if (strToolName == "Area")
		{
			CMeaArea* pAreaTool = dynamic_cast<CMeaArea*>(m_ptrSelectedTool);
			if (pAreaTool != NULL)
				pAreaTool->pDcm = pDcm;
		}
		else if (strToolName == "Ellipse")
		{
			CMeaEllipse* pEllipseTool = dynamic_cast<CMeaEllipse*>(m_ptrSelectedTool);
			if (pEllipseTool != NULL)
				pEllipseTool->pDcm = pDcm;
		}
		else if (strToolName == "Shape")
		{
			CMeaShape* pShapeTool = dynamic_cast<CMeaShape*>(m_ptrSelectedTool);
			if (pShapeTool != NULL)
				pShapeTool->pDcm = pDcm;
		}
		m_ptrSelectedTool->GetMeasurement();
		pView->UpdateResult(m_nSelectedIndex);
		pView->Invalidate(FALSE);
		pView->UpdateWindow();
		return;
	}

	int nCurrentFrame = pView->m_nCurrentFrame;
	vector<CMeasurement*> vecTools = pView->GetResult();
	const int nToolCount = static_cast<int>(vecTools.size());
	for (int i = nToolCount - 1; i >= 0; --i)
	{
		if (vecTools[i] == NULL || vecTools[i]->m_nPlaneNumber != pView->GetPlaneNum() || vecTools[i]->m_nSliceNumber != nCurrentFrame) continue;
		CString strClassName = vecTools[i]->m_strToolName;
		if (strClassName == "Line") {
			CMeaLine* pTool = dynamic_cast<CMeaLine*>(vecTools[i]);
			CPoint ptStart = pDcm->Image2Screen(&pTool->m_ptStart);
			CPoint ptEnd = pDcm->Image2Screen(&pTool->m_ptEnd);
			CRect rect(ptStart, ptEnd);
			rect.NormalizeRect();
			if (abs(point.x - ptStart.x) < 5 && abs(point.y - ptStart.y) < 5)
				m_nMouseState = 2;
			else if (abs(point.x - ptEnd.x) < 5 && abs(point.y - ptEnd.y) < 5)
				m_nMouseState = 2;
			else if (rect.PtInRect(point))
				m_nMouseState = 1;
			else
				m_nMouseState = 0;
		}
		else if (strClassName == "Angle") {
			CMeaAngle* pTool = dynamic_cast<CMeaAngle*>(vecTools[i]);
			CPoint ptStart = pDcm->Image2Screen(&pTool->m_ptStart);
			CPoint ptAngle = pDcm->Image2Screen((&pTool->m_ptAngle));
			CPoint ptEnd = pDcm->Image2Screen(&pTool->m_ptEnd);
			int xMin = ptStart.x, xMax = ptStart.x, yMin = ptStart.y, yMax = ptStart.y;
			if (ptAngle.x < xMin) xMin = ptAngle.x;
			else if(ptAngle.x > xMax) xMax = ptAngle.x;
			if (ptEnd.x < xMin) xMin = ptEnd.x;
			else if (ptEnd.x > xMax) xMax = ptEnd.x;
			if (ptAngle.y < yMin) yMin = ptAngle.y;
			else if(ptAngle.y > yMax) yMax = ptAngle.y;
			if (ptEnd.y < yMin) yMin = ptEnd.y;
			else if (ptEnd.y > yMax) yMax = ptEnd.y;
			CRect rect(xMin, yMin, xMax, yMax);
			rect.NormalizeRect();
			if (abs(point.x - ptStart.x) < 5 && abs(point.y - ptStart.y) < 5)
				m_nMouseState = 2;
			else if (abs(point.x - ptAngle.x) < 5 && abs(point.y - ptAngle.y) < 5)
				m_nMouseState = 2;
			else if (abs(point.x - ptEnd.x) < 5 && abs(point.y - ptEnd.y) < 5)
				m_nMouseState = 2;
			else if (rect.PtInRect(point))
				m_nMouseState = 1;
			else
				m_nMouseState = 0;
		}
		else if (strClassName == "Shape") {
			CMeaShape* pTool = dynamic_cast<CMeaShape*>(vecTools[i]);
			CPoint ptStart = pDcm->Image2Screen(&pTool->m_ptTopLeft);
			CPoint ptEnd = pDcm->Image2Screen(&pTool->m_ptBottonRight);
			CRect rect(ptStart, ptEnd);
			rect.NormalizeRect();
			if (abs(point.x - ptStart.x) < 5 && abs(point.y - ptStart.y) < 5)
				m_nMouseState = 2;
			else if (abs(point.x - ptEnd.x) < 5 && abs(point.y - ptEnd.y) < 5)
				m_nMouseState = 2;
			else if (abs(point.x - ptEnd.x) < 5 && abs(point.y - ptStart.y) < 5)
				m_nMouseState = 2;
			else if (abs(point.x - ptStart.x) < 5 && abs(point.y - ptEnd.y) < 5)
				m_nMouseState = 2;
			else if (rect.PtInRect(point))
				m_nMouseState = 1;
			else
				m_nMouseState = 0;
		}
		else if (strClassName == "CT") {
			CMeaCT* pTool = dynamic_cast<CMeaCT*>(vecTools[i]);
			CPoint ptPosition = pDcm->Image2Screen(&pTool->m_ptPostion);
			if (abs(point.x - ptPosition.x) < 5 && abs(point.y - ptPosition.y) < 5)
				m_nMouseState = 2;
			else
				m_nMouseState = 0;
		}

		if (m_nMouseState != 0) return;
	}
}

HCURSOR SeMeasureSelectTool::LoadCursor()
{
	switch (m_nMouseState)
	{
	case 0:
		{
			return AfxGetApp()->LoadCursor(IDC_POINTER);
			break;
		}
	case 1:
		{
			return AfxGetApp()->LoadCursor(IDC_CROSS);
			break;
		}
	case 2:
		{
			return AfxGetApp()->LoadCursor(IDC_MOVECUR);
			break;
		}
	default:
		break;
	}
	return AfxGetApp()->LoadCursor(IDC_POINTER); 
}

void DrawTools(CWnd* pWnd, CDC *pDC, CRect& rt)
{
	SeMPRView* pView = (SeMPRView*)pWnd;
	CDcmPic *pDcm = (CDcmPic *)pView->GetDisplayMgr()->GetCurrentImage();
	if(pDcm == NULL)
		return;
	int nCurrentFrame = pView->m_nCurrentFrame;
	vector<CMeasurement*> vecTools = pView->GetResult();
	const int nToolCount = static_cast<int>(vecTools.size());
	for (int i = nToolCount - 1; i >= 0; --i)
	{
		if (vecTools[i] == NULL || vecTools[i]->m_nPlaneNumber != pView->GetPlaneNum() || vecTools[i]->m_nSliceNumber != nCurrentFrame) continue;

		Graphics gc(pDC->m_hDC);
		gc.SetSmoothingMode(SmoothingModeHighQuality);
		Color	colorFocus(255, 255, 97, 0);
		Color   colorYellow(255, 255, 255, 0);
		Color	colorGreen(255, 0, 255, 0);
		Pen     penGreen(colorGreen, 1.0);
		SolidBrush   brushYellow(colorYellow);
		Pen     penYellow(colorYellow, 2);

		CString strClassName = vecTools[i]->m_strToolName;
		if (strClassName == "Line") {
			CMeaLine* pTool = dynamic_cast<CMeaLine*>(vecTools[i]);
			CPoint ptStart = pDcm->Image2Screen(&pTool->m_ptStart);
			CPoint ptEnd = pDcm->Image2Screen(&pTool->m_ptEnd);
			gc.DrawLine(&penGreen, PointF(ptStart.x, ptStart.y), PointF(ptEnd.x, ptEnd.y));
			gc.FillRectangle(&brushYellow, ptStart.x - 2, ptStart.y - 2, 5, 5);
			gc.FillRectangle(&brushYellow, ptEnd.x - 2, ptEnd.y - 2, 5, 5);
		}
		else if (strClassName == "Angle") {
			CMeaAngle* pTool = dynamic_cast<CMeaAngle*>(vecTools[i]);
			CPoint ptStart = pDcm->Image2Screen(&pTool->m_ptStart);
			CPoint ptAngle = pDcm->Image2Screen((&pTool->m_ptAngle));
			CPoint ptEnd = pDcm->Image2Screen(&pTool->m_ptEnd);
			gc.DrawLine(&penGreen, PointF(ptStart.x, ptStart.y), PointF(ptAngle.x, ptAngle.y));
			gc.DrawLine(&penGreen, PointF(ptAngle.x, ptAngle.y), PointF(ptEnd.x, ptEnd.y));
			gc.FillRectangle(&brushYellow, ptStart.x - 2, ptStart.y - 2, 5, 5);
			gc.FillRectangle(&brushYellow, ptAngle.x - 2, ptAngle.y - 2, 5, 5);
			gc.FillRectangle(&brushYellow, ptEnd.x - 2, ptEnd.y - 2, 5, 5);
		}
		else if (strClassName == "Shape") {
			CMeaShape* pTool = dynamic_cast<CMeaShape*>(vecTools[i]);
			CPoint ptStart = pDcm->Image2Screen(&pTool->m_ptTopLeft);
			CPoint ptEnd = pDcm->Image2Screen(&pTool->m_ptBottonRight);
			int x = ptStart.x < ptEnd.x ? ptStart.x : ptEnd.x;
			int y = ptStart.y < ptEnd.y ? ptStart.y : ptEnd.y;
			gc.DrawRectangle(&penGreen, x, y, abs(ptStart.x - ptEnd.x), abs(ptStart.y - ptEnd.y));
			gc.FillRectangle(&brushYellow, ptStart.x - 2, ptStart.y - 2, 5, 5);
			gc.FillRectangle(&brushYellow, ptEnd.x - 2, ptEnd.y - 2, 5, 5);
			gc.FillRectangle(&brushYellow, ptStart.x - 2, ptEnd.y - 2, 5, 5);
			gc.FillRectangle(&brushYellow, ptEnd.x - 2, ptStart.y - 2, 5, 5);
		}
		else if (strClassName == "CT") {
			CMeaCT* pTool = dynamic_cast<CMeaCT*>(vecTools[i]);
			CPoint ptPostion = pDcm->Image2Screen(&pTool->m_ptPostion);
			gc.DrawLine(&penYellow, PointF(ptPostion.x - 10, ptPostion.y), PointF(ptPostion.x + 10, ptPostion.y));
			gc.DrawLine(&penYellow, PointF(ptPostion.x, ptPostion.y - 10), PointF(ptPostion.x, ptPostion.y + 10));
		}
		else if (strClassName == "Area") {
			CMeaArea* pTool = dynamic_cast<CMeaArea*>(vecTools[i]);

			vector<CPoint>& vecPts = pTool->m_vecPoints;

			DrawLine(vecPts, colorFocus, 1, &gc, pDcm);
			DrawRegion(vecPts, Color(30, 0, 255, 0), &gc, pDcm);

			CRect   rect;
			pView->GetClientRect(&rect);
			rect.InflateRect(0, 0, -2, -1);
			Color	colorGreen(255, 0, 255, 0);
			Pen     penGreen(colorGreen, 2.0);
			gc.DrawRectangle(&penGreen, rect.left, rect.top, rect.Width(), rect.Height());
		}
		else if (strClassName == "Ellipse") {
			CMeaEllipse* pTool = dynamic_cast<CMeaEllipse*>(vecTools[i]);
			if (pTool == NULL)
				continue;

			pTool->pDcm = pDcm;
			CPoint ptStart = pDcm->Image2Screen(&pTool->m_ptStart);
			CPoint ptEnd = pDcm->Image2Screen(&pTool->m_ptEnd);

			if (pTool->m_bIsCircle)
			{
				const int dx = ptEnd.x - ptStart.x;
				const int dy = ptEnd.y - ptStart.y;
				const int radius = min(abs(dx), abs(dy));
				ptEnd.x = ptStart.x + (dx >= 0 ? radius : -radius);
				ptEnd.y = ptStart.y + (dy >= 0 ? radius : -radius);
			}

			const int left = min(ptStart.x, ptEnd.x);
			const int top = min(ptStart.y, ptEnd.y);
			const int width = abs(ptEnd.x - ptStart.x);
			const int height = abs(ptEnd.y - ptStart.y);
			if (width <= 0 || height <= 0)
				continue;

			gc.DrawEllipse(&penGreen, left, top, width, height);
			gc.FillRectangle(&brushYellow, ptStart.x - 2, ptStart.y - 2, 5, 5);
			gc.FillRectangle(&brushYellow, ptEnd.x - 2, ptEnd.y - 2, 5, 5);
		}
	}
}

void DrawLine(vector <CPoint> vecPts, Color color, int nWidth, Graphics* gc, CDcmPic* pDcm)
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

void DrawRegion(vector <CPoint> vecPts, SolidBrush brush, Graphics* gc, CDcmPic* pDcm)
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

