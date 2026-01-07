// SeTransFuncDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "SeGeneralModule.h"
#include "SeTransFuncDlg.h"
#include "afxdialogex.h"


// CSeTransFuncDlg 对话框

IMPLEMENT_DYNAMIC(CSeTransFuncDlg, CDialogEx)

CSeTransFuncDlg::CSeTransFuncDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSeTransFuncDlg::IDD, pParent)
{
	m_pParent = pParent;
	m_nMin = 0;
	m_nMax = 0;
	m_bLBtnDown = FALSE;
	m_state = NOTHIT;
	m_pHistogram = NULL;
}

CSeTransFuncDlg::CSeTransFuncDlg(LONG* pHistogram, LONG lMaxNumber, int nMax /*= 4096*/, int nMin /*= -1000*/, CWnd* pParent /*= NULL*/)
	: CDialogEx(CSeTransFuncDlg::IDD, pParent)
{
	m_pParent = pParent;
	m_nMin = nMin;
	m_nMax = nMax;
	m_pHistogram = pHistogram;
	m_lMaxNumber = lMaxNumber;
	m_bLBtnDown = FALSE;
	m_state = NOTHIT;
}

CSeTransFuncDlg::~CSeTransFuncDlg()
{
}

void CSeTransFuncDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSeTransFuncDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_SETCURSOR()
	ON_COMMAND(ID_DELETE_POINT, OnDeleteLinePoint)
	ON_COMMAND(ID_DELETE_POINT2, OnDeleteColorPoint)
	ON_COMMAND(ID_CHANGR_COLOR, OnChangmeColor)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON_SAVE, &CSeTransFuncDlg::OnBnClickedButtonSave)
	ON_BN_CLICKED(IDC_BUTTON_LOAD, &CSeTransFuncDlg::OnBnClickedButtonLoad)
	ON_EN_CHANGE(IDC_EDIT_TRANSFUNC_NAME, &CSeTransFuncDlg::OnEnChangeEditTransfuncName)
END_MESSAGE_MAP()


// CSeTransFuncDlg 消息处理程序


void CSeTransFuncDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: 在此处添加消息处理程序代码
	// 不为绘图消息调用 CDialogEx::OnPaint()
	CRect rect;
	GetClientRect(&rect);
	CZKMemDC memDC(&dc, rect, RGB(255,255,255));
	Graphics gc(memDC.m_hDC);
	Color crBlack(255, 0, 0, 0);
	Pen penBlack(crBlack, 1.0);
	SolidBrush brushBlack(crBlack);
	Color crWhite(255, 255, 255, 255);
	SolidBrush brushWhite(crWhite);
	Color crRed(255, 255, 0, 0);
	SolidBrush brushRed(crRed);
	Pen penRed(crRed, 1.0);

	gc.SetSmoothingMode(SmoothingModeHighQuality);
	if (m_pHistogram == NULL)
		return;

	// draw plane on the top
	CRect rtPoint = m_rtUpPlane;
	gc.FillRectangle(&brushWhite, rtPoint.left, rtPoint.top, rtPoint.Width(), rtPoint.Height());
	gc.DrawRectangle(&penBlack, rtPoint.left, rtPoint.top, rtPoint.Width(), rtPoint.Height());
	float dWidth = (float)rtPoint.Width();
	float dHeight = (float)rtPoint.Height();
	float dHistogramWidth = (float)(m_nMax - m_nMin + 1);
	float dHistogramHeight = log((float)m_lMaxNumber);
	float dScale = dHeight/dHistogramHeight;
	Color crGreen(64, 0, 255, 0);
	SolidBrush brushGreen(crGreen);
	Color crGray(255, 128, 128, 128);
	SolidBrush brushGray(crGray);
	PointF *pPts = new PointF[m_nMax - m_nMin + 3];
	for (int i = m_nMin; i <= m_nMax; i++)
	{
		float fHvalue = (float)m_pHistogram[i + 32768] != 0 ? log ((float)m_pHistogram[i + 32768]) : 0.0;
		pPts[i - m_nMin] = PointF(rtPoint.left + (i - m_nMin) * dWidth/dHistogramWidth, rtPoint.bottom - fHvalue * dScale);
	}
	pPts[m_nMax - m_nMin + 1] = PointF(rtPoint.right, rtPoint.bottom);
	pPts[m_nMax - m_nMin + 2] = PointF(rtPoint.left, rtPoint.bottom);
	gc.FillPolygon(&brushGray, pPts, m_nMax - m_nMin + 3);
	assert(m_vecLightnessCtlPoints.size() >= 2);
	for (int i=0; i<m_vecLightnessCtlPoints.size() - 1; i++)
	{
		gc.DrawLine(&penBlack, 
			m_vecLightnessCtlPoints[i].ptX, 
			m_vecLightnessCtlPoints[i].ptY, 
			m_vecLightnessCtlPoints[i+1].ptX, 
			m_vecLightnessCtlPoints[i+1].ptY
			);
		gc.FillRectangle(&brushBlack, 
			m_vecLightnessCtlPoints[i].ptX - 2.0, 
			m_vecLightnessCtlPoints[i].ptY - 2.0,
			4, 4);
		if (m_vecLightnessCtlPoints[i].hit)
		{
			gc.DrawRectangle(&penRed, 
				m_vecLightnessCtlPoints[i].ptX - 3.0, 
				m_vecLightnessCtlPoints[i].ptY - 3.0,
				6, 6);
		}

	}
	gc.FillRectangle(&brushBlack, 
		m_vecLightnessCtlPoints[m_vecLightnessCtlPoints.size() - 1].ptX - 2.0, 
		m_vecLightnessCtlPoints[m_vecLightnessCtlPoints.size() - 1].ptY - 2.0,
		4, 4);
	if (m_vecLightnessCtlPoints[m_vecLightnessCtlPoints.size() - 1].hit)
	{
		gc.DrawRectangle(&penRed, 
			m_vecLightnessCtlPoints[m_vecLightnessCtlPoints.size() - 1].ptX - 3.0, 
			m_vecLightnessCtlPoints[m_vecLightnessCtlPoints.size() - 1].ptY - 3.0,
			6, 6);
	}


	// draw plane on the bottom
	CRect rtColor = m_rtDownPlane;
	gc.FillRectangle(&brushWhite, rtColor.left, rtColor.top, rtColor.Width(), rtColor.Height());
	gc.DrawRectangle(&penBlack, rtColor.left, rtColor.top, rtColor.Width(), rtColor.Height());

	assert(m_vecColorCtlPoints.size() >= 2);
	for (int i=0; i<m_vecColorCtlPoints.size() - 1; i++)
	{
		Color cr1 = Color(255, 
			GetRValue(m_vecColorCtlPoints[i].color),
			GetGValue(m_vecColorCtlPoints[i].color),
			GetBValue(m_vecColorCtlPoints[i].color)
			);  //  第一种颜色

		Color cr2 = Color(255, 
			GetRValue(m_vecColorCtlPoints[i+1].color),
			GetGValue(m_vecColorCtlPoints[i+1].color),
			GetBValue(m_vecColorCtlPoints[i+1].color)
			);  //  第二种颜色

		LinearGradientBrush linGrBrush(Rect(m_vecColorCtlPoints[i].pos, 
			rtColor.top, 
			m_vecColorCtlPoints[i + 1].pos - m_vecColorCtlPoints[i].pos, 
			rtColor.Height()), cr1, cr2, (REAL)(0));

		gc.FillRectangle(&linGrBrush, Rect(m_vecColorCtlPoints[i].pos, 
			rtColor.top, 
			m_vecColorCtlPoints[i + 1].pos - m_vecColorCtlPoints[i].pos, 
			rtColor.Height()));

		SolidBrush brushCr1(cr1);
		gc.FillRectangle(&brushBlack, Rect(m_vecColorCtlPoints[i].pos - 2, 
			rtColor.top + 5, 
			4, 
			rtColor.Height() - 10));

		if (m_vecColorCtlPoints[i].hit)
		{
			gc.DrawRectangle(&penRed, Rect(m_vecColorCtlPoints[i].pos - 2, 
				rtColor.top, 
				6, 
				rtColor.Height()));
		}
	}

	Color cr = Color(255, 
		GetRValue(m_vecColorCtlPoints[m_vecColorCtlPoints.size() - 1].color),
		GetGValue(m_vecColorCtlPoints[m_vecColorCtlPoints.size() - 1].color),
		GetBValue(m_vecColorCtlPoints[m_vecColorCtlPoints.size() - 1].color)
		);  //  第一种颜色
	SolidBrush brushCr1(cr);
	gc.FillRectangle(&brushBlack, Rect(m_vecColorCtlPoints[m_vecColorCtlPoints.size() - 1].pos - 2, 
		rtColor.top + 5, 
		4, 
		rtColor.Height() - 10));

	if (m_vecColorCtlPoints[m_vecColorCtlPoints.size() - 1].hit)
	{
		gc.DrawRectangle(&penRed, Rect(m_vecColorCtlPoints[m_vecColorCtlPoints.size() - 1].pos - 2, 
			rtColor.top, 
			6, 
			rtColor.Height()));
	}
}

BOOL CSeTransFuncDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	CRect rect;
	GetClientRect(&rect);
	m_rtUpPlane = CRect(
		rect.left + 15,
		rect.top + 5,
		rect.right - 15,
		rect.bottom - 90
		);
	m_rtDownPlane = CRect(
		rect.left + 15,
		rect.bottom - 60,
		rect.right - 15,
		rect.bottom - 35);

	RefreshTransFuncSetting();

	CString strWorkPath = CSeToolKit::GetWorkPath();
	LoadTransFuncSetting(strWorkPath + "\\Config\\TransFunc.ini");

	CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_TRANSFUNC_SETTING);
	for(int i=0; i<m_vecSetting.size(); i++)
	{
		// 逆向顺序添加
		pCombo->AddString(m_vecSetting[i].csSettingName);
	}

	if (m_vecSetting.size() > 1)
	{
		pCombo->SetCurSel(0);
		CStringArray csaPoints;
		CStringArray csaColors;
		CSeToolKit::TokenizeString(m_vecSetting[0].csSetttingPoints, "|", csaPoints, TRUE, TRUE);
		CSeToolKit::TokenizeString(m_vecSetting[0].csSettingColors, "|", csaColors, TRUE, TRUE);

		// add Points
		CStringArray csaZeroValues;
		CSeToolKit::TokenizeString(csaPoints[0], ",", csaZeroValues, TRUE, TRUE);
		if (atoi(csaZeroValues[0]) > m_nMin)
			m_vecLightnessCtlPoints.push_back(TransFuncLightnessCtlPoint(m_rtUpPlane.left + 2.0, m_rtUpPlane.bottom - 2.0, FALSE));

		for (int i=0; i<csaPoints.GetSize(); i++)
		{
			CStringArray csaValues;
			CSeToolKit::TokenizeString(csaPoints[i], ",", csaValues, TRUE, TRUE);
			int valueX = atoi(csaValues[0]) > m_nMin ? atoi(csaValues[0]) : m_nMin;
			valueX = atoi(csaValues[0]) < m_nMax ? atoi(csaValues[0]) : m_nMax;
			int valueY = atoi(csaValues[1]) > 0 ? atoi(csaValues[1]) : 0;
			valueY = atoi(csaValues[1]) < 255 ? atoi(csaValues[1]) : 255;
			float fXpos = m_rtUpPlane.left + 2.0 + valueX * (m_rtUpPlane.Width() - 4.0) / (m_nMax - m_nMin);
			float fYpos = m_rtUpPlane.top + 2.0 + (256.0 - valueY) * (m_rtUpPlane.Height() - 4.0) / 256.0;

			m_vecLightnessCtlPoints.push_back(TransFuncLightnessCtlPoint(fXpos, fYpos , FALSE));
		}

		csaZeroValues.RemoveAll();
		CSeToolKit::TokenizeString(csaPoints[csaPoints.GetSize() - 1], ",", csaZeroValues, TRUE, TRUE);
		if (atoi(csaZeroValues[0]) < m_nMax)
			m_vecLightnessCtlPoints.push_back(TransFuncLightnessCtlPoint(m_rtUpPlane.right - 2.0, m_rtUpPlane.top + 2.0, FALSE));


		// add Colors
		csaZeroValues.RemoveAll();
		CSeToolKit::TokenizeString(csaPoints[0], ",", csaZeroValues, TRUE, TRUE);
		if (atoi(csaZeroValues[0]) > m_nMin)
			m_vecColorCtlPoints.push_back(TransFuncColorCtlPoint(m_rtDownPlane.left + 2.0, RGB(255, 255, 255), FALSE));

		for (int i=0; i<csaColors.GetSize(); i++)
		{
			CStringArray csaValues;
			CSeToolKit::TokenizeString(csaColors[i], ",", csaValues, TRUE, TRUE);
			int valueX = atoi(csaValues[0]) > m_nMin ? atoi(csaValues[0]) : m_nMin;
			valueX = atoi(csaValues[0]) < m_nMax ? atoi(csaValues[0]) : m_nMax;

			int valueR = atoi(csaValues[1]) > 0 ? atoi(csaValues[1]) : 0;
			valueR = atoi(csaValues[1]) < 255 ? atoi(csaValues[1]) : 255;
			int valueG = atoi(csaValues[2]) > 0 ? atoi(csaValues[2]) : 0;
			valueG = atoi(csaValues[2]) < 255 ? atoi(csaValues[2]) : 255;
			int valueB = atoi(csaValues[3]) > 0 ? atoi(csaValues[3]) : 0;
			valueB = atoi(csaValues[3]) < 255 ? atoi(csaValues[3]) : 255;

			float fPos = m_rtDownPlane.left + 2.0 + valueX * (m_rtDownPlane.Width() - 4.0) / (m_nMax - m_nMin);
			COLORREF color = RGB(valueR, valueG, valueB);
			m_vecColorCtlPoints.push_back(TransFuncColorCtlPoint(fPos, color, FALSE));
		}

		csaZeroValues.RemoveAll();
		CSeToolKit::TokenizeString(csaPoints[csaPoints.GetSize() - 1], ",", csaZeroValues, TRUE, TRUE);
		if (atoi(csaZeroValues[0]) < m_nMax)
			m_vecColorCtlPoints.push_back(TransFuncColorCtlPoint(m_rtDownPlane.right - 2.0, RGB(255, 255, 255), FALSE));
	}
	else
	{
		// 控制点宽度 4 个像素， 需要减掉
		m_vecLightnessCtlPoints.push_back(TransFuncLightnessCtlPoint(m_rtUpPlane.left + 2.0, m_rtUpPlane.bottom - 2.0, FALSE));
		m_vecLightnessCtlPoints.push_back(TransFuncLightnessCtlPoint(m_rtUpPlane.right - 2.0, m_rtUpPlane.top + 2.0, FALSE));

		m_vecColorCtlPoints.push_back(TransFuncColorCtlPoint(m_rtDownPlane.left + 2.0, RGB(255, 255, 255), FALSE));
		m_vecColorCtlPoints.push_back(TransFuncColorCtlPoint(m_rtDownPlane.right - 2.0, RGB(255, 255, 255), FALSE));
	}



	SetTimer(0, 50, NULL);
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CSeTransFuncDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_bLBtnDown = TRUE;
	switch(m_state)
	{
	case NOTHIT:
		break;
	case HITLINE:
		{
			for (int i=0; i<m_vecLightnessCtlPoints.size(); i++)
			{
				m_vecLightnessCtlPoints[i].hit = FALSE;
			}
			m_vecLightnessCtlPoints.insert(m_vecLightnessCtlPoints.begin() + m_hitIndex + 1, TransFuncLightnessCtlPoint(point.x, point.y, TRUE));
			break;
		}
	case HITPOINT:
		{
			for (int i=0; i<m_vecLightnessCtlPoints.size(); i++)
			{
				m_vecLightnessCtlPoints[i].hit = FALSE;
			}
			m_vecLightnessCtlPoints[m_hitIndex].hit = TRUE;
			break;
		}
	case HITCOLORLINE:
		{
			for (int i=0; i<m_vecColorCtlPoints.size(); i++)
			{
				m_vecColorCtlPoints[i].hit = FALSE;
			}
			m_vecColorCtlPoints.insert(m_vecColorCtlPoints.begin() + m_hitIndex + 1, TransFuncColorCtlPoint(point.x, 
				GetMixColor(m_vecColorCtlPoints[m_hitIndex], m_vecColorCtlPoints[m_hitIndex + 1], point.x),
				TRUE));
			break;
		}
	case HITCOLOR:
		{
			for (int i=0; i<m_vecColorCtlPoints.size(); i++)
			{
				m_vecColorCtlPoints[i].hit = FALSE;
			}
			m_vecColorCtlPoints[m_hitIndex].hit = TRUE;
			break;
		}
	default:
		{
			break;
		}
	}
	Invalidate(FALSE);
	CDialogEx::OnLButtonDown(nFlags, point);
}


void CSeTransFuncDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_bLBtnDown = FALSE;
	CDialogEx::OnLButtonUp(nFlags, point);
}


void CSeTransFuncDlg::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	
	CDialogEx::OnLButtonDblClk(nFlags, point);
}


void CSeTransFuncDlg::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CPoint pt;
	GetCursorPos(&pt);

	switch(m_state)
	{
	case NOTHIT:
		break;
	case HITLINE:
		break;
	case HITPOINT:
		{
			for (int i=0; i<m_vecLightnessCtlPoints.size(); i++)
			{
				m_vecLightnessCtlPoints[i].hit = FALSE;
			}
			m_vecLightnessCtlPoints[m_hitIndex].hit = TRUE;
			Invalidate(FALSE);
			CMenu menuLine;
			menuLine.CreatePopupMenu();
			menuLine.AppendMenu(MF_STRING,ID_DELETE_POINT,"Delete");
			menuLine.TrackPopupMenu(TPM_RIGHTBUTTON, pt.x, pt.y, this);
			menuLine.DestroyMenu();
			break;
		}
	case HITCOLORLINE:
		break;
	case HITCOLOR:
		{
			for (int i=0; i<m_vecColorCtlPoints.size(); i++)
			{
				m_vecColorCtlPoints[i].hit = FALSE;
			}
			m_vecColorCtlPoints[m_hitIndex].hit = TRUE;
			Invalidate(FALSE);
			CMenu menuColor;
			menuColor.CreatePopupMenu();
			menuColor.AppendMenu(MF_STRING, ID_DELETE_POINT2, "Delete");
			menuColor.AppendMenu(MF_STRING, ID_CHANGR_COLOR, "Change Color");
			menuColor.TrackPopupMenu(TPM_RIGHTBUTTON, pt.x, pt.y, this);
			menuColor.DestroyMenu();
			break;
		}
	default:
		break;
	}
	CDialogEx::OnRButtonDown(nFlags, point);
}


void CSeTransFuncDlg::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnRButtonUp(nFlags, point);
}


void CSeTransFuncDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CRect rect;
	GetClientRect(&rect);

	CRect rtLine = m_rtUpPlane;
	CRect rtColor = m_rtDownPlane;


// 	// MAKEINTRESOURCE(32515) 十字光标
// 	// MAKEINTRESOURCE(32512) 箭头光标
// 	BOOL bHit = FALSE;
// 	if (PtInRect(&rtLine, point))
// 	{
// 		for (int i=0; i<m_vecLightnessCtlPoints.size() - 1; i++)
// 		{
// 			CPoint pt1(m_vecLightnessCtlPoints[i].ptX, m_vecLightnessCtlPoints[i].ptY);
// 			CPoint pt2(m_vecLightnessCtlPoints[i+1].ptX, m_vecLightnessCtlPoints[i+1].ptY);
// 			CRgn rgn;
// 			CPoint pt[4];
// 			pt[0].x = m_vecLightnessCtlPoints[i].ptX;
// 			pt[0].y = m_vecLightnessCtlPoints[i].ptY - 4;
// 			pt[1].x = m_vecLightnessCtlPoints[i].ptX;
// 			pt[1].y = m_vecLightnessCtlPoints[i].ptY + 4;
// 			pt[2].x = m_vecLightnessCtlPoints[i+1].ptX;
// 			pt[2].y = m_vecLightnessCtlPoints[i+1].ptY + 4;
// 			pt[3].x = m_vecLightnessCtlPoints[i+1].ptX;
// 			pt[3].y = m_vecLightnessCtlPoints[i+1].ptY - 4;
// 			rgn.CreatePolygonRgn (pt,4,WINDING);// 注意先后 
// 			if(PtInRegion(rgn, point.x, point.y))
// 			{
// 				HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(32515));
// 				::SetCursor(hCur);
// 				m_state = HITLINE;
// 				m_hitIndex = i;
// 				bHit = TRUE;
// 			}
// 		}
// 
// 		for (int i=0; i<m_vecLightnessCtlPoints.size(); i++)
// 		{
// 			if (abs(m_vecLightnessCtlPoints[i].ptX - point.x) <= 4 && abs(m_vecLightnessCtlPoints[i].ptY - point.y) <= 4)
// 			{
// 				HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(32649));
// 				::SetCursor(hCur);
// 				m_state = HITPOINT;
// 				m_hitIndex = i;
// 			}
// 		}
// 	}
// 	else if(PtInRect(&rtColor, point))
// 	{
// 		for (int i=0; i<m_vecColorCtlPoints.size() - 1; i++)
// 		{
// 			CRect rtColorSlice(m_vecColorCtlPoints[i].pos, rtColor.top, m_vecColorCtlPoints[i+1].pos, rtColor.bottom);
// 			if (PtInRect(&rtColorSlice, point))
// 			{
// 				m_state = HITCOLORLINE;
// 				m_hitIndex = i;
// 			}
// 		}
// 		for (int i=0; i<m_vecColorCtlPoints.size(); i++)
// 		{
// 			if (abs(m_vecColorCtlPoints[i].pos - point.x) <= 5)
// 			{
// 				HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(32515));
// 				::SetCursor(hCur);
// 				bHit = TRUE;
// 				m_state = HITCOLOR;
// 				m_hitIndex = i;
// 			}
// 		}
// 	}
// 
// 	if (!bHit)
// 	{
// 		HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(32512));
// 		::SetCursor(hCur);
// 	}

	if (m_bLBtnDown && (m_state == HITPOINT || m_state == HITCOLOR))
	{
		switch(m_state)
		{
		case NOTHIT:
			break;
		case HITLINE:
			{
// 				if (point.x > m_vecLightnessCtlPoints[m_hitIndex].ptX && point.x < m_vecLightnessCtlPoints[m_hitIndex +2 ].ptX)
// 					m_vecLightnessCtlPoints[m_hitIndex+1].ptX = point.x;
// 				if (point.y >  rtLine.top && point.y < rtLine.bottom)
// 					m_vecLightnessCtlPoints[m_hitIndex+1].ptY = point.y;
// 				Invalidate(FALSE);
				break;
			}
		case HITPOINT:
			{
				if (m_hitIndex == 0 || m_hitIndex == m_vecLightnessCtlPoints.size() - 1)
				{
					if (point.y >  rtLine.top && point.y < rtLine.bottom)
						m_vecLightnessCtlPoints[m_hitIndex].ptY = point.y;
				}
				else
				{
					if (point.x > m_vecLightnessCtlPoints[m_hitIndex - 1].ptX && point.x < m_vecLightnessCtlPoints[m_hitIndex + 1].ptX)
						m_vecLightnessCtlPoints[m_hitIndex].ptX = point.x;
					if (point.y >  rtLine.top && point.y < rtLine.bottom)
						m_vecLightnessCtlPoints[m_hitIndex].ptY = point.y;
				}
				m_state = HITPOINT;
				Invalidate(FALSE);
				break;
			}
		case HITCOLORLINE:
			{
// 				if (point.x > m_vecColorCtlPoints[m_hitIndex].pos && point.x < m_vecColorCtlPoints[m_hitIndex + 2].pos)
// 					m_vecColorCtlPoints[m_hitIndex+1].pos = point.x;
// 				Invalidate(FALSE);
				break;
			}
		case HITCOLOR:
			{
				if (m_hitIndex == 0 || m_hitIndex == m_vecColorCtlPoints.size() - 1)
				{

				}
				else
				{
					if (point.x > m_vecColorCtlPoints[m_hitIndex - 1].pos && point.x < m_vecColorCtlPoints[m_hitIndex + 1].pos)
						m_vecColorCtlPoints[m_hitIndex].pos = point.x;
				}
				m_state = HITCOLOR;
				Invalidate(FALSE);
				break;
			}
		default:
			break;
		}
	}
	else
	{
		// MAKEINTRESOURCE(32515) 十字光标
		// MAKEINTRESOURCE(32512) 箭头光标
		BOOL bHit = FALSE;
		if (PtInRect(&rtLine, point))
		{
			for (int i=0; i<m_vecLightnessCtlPoints.size() - 1; i++)
			{
				CPoint pt1(m_vecLightnessCtlPoints[i].ptX, m_vecLightnessCtlPoints[i].ptY);
				CPoint pt2(m_vecLightnessCtlPoints[i+1].ptX, m_vecLightnessCtlPoints[i+1].ptY);
				CRgn rgn;
				CPoint pt[4];
				pt[0].x = m_vecLightnessCtlPoints[i].ptX;
				pt[0].y = m_vecLightnessCtlPoints[i].ptY - 4;
				pt[1].x = m_vecLightnessCtlPoints[i].ptX;
				pt[1].y = m_vecLightnessCtlPoints[i].ptY + 4;
				pt[2].x = m_vecLightnessCtlPoints[i+1].ptX;
				pt[2].y = m_vecLightnessCtlPoints[i+1].ptY + 4;
				pt[3].x = m_vecLightnessCtlPoints[i+1].ptX;
				pt[3].y = m_vecLightnessCtlPoints[i+1].ptY - 4;
				rgn.CreatePolygonRgn (pt,4,WINDING);// 注意先后 
				if(PtInRegion(rgn, point.x, point.y))
				{
					HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(32515));
					::SetCursor(hCur);
					m_state = HITLINE;
					m_hitIndex = i;
					bHit = TRUE;
				}
			}

			for (int i=0; i<m_vecLightnessCtlPoints.size(); i++)
			{
				if (abs(m_vecLightnessCtlPoints[i].ptX - point.x) <= 4 && abs(m_vecLightnessCtlPoints[i].ptY - point.y) <= 4)
				{
					HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(32649));
					::SetCursor(hCur);
					m_state = HITPOINT;
					m_hitIndex = i;
				}
			}
		}
		else if(PtInRect(&rtColor, point))
		{
			for (int i=0; i<m_vecColorCtlPoints.size() - 1; i++)
			{
				CRect rtColorSlice(m_vecColorCtlPoints[i].pos, rtColor.top, m_vecColorCtlPoints[i+1].pos, rtColor.bottom);
				if (PtInRect(&rtColorSlice, point))
				{
					HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(32512));
					::SetCursor(hCur);
					bHit = TRUE;
					m_state = HITCOLORLINE;
					m_hitIndex = i;
				}
			}
			for (int i=0; i<m_vecColorCtlPoints.size(); i++)
			{
				if (abs(m_vecColorCtlPoints[i].pos - point.x) <= 5)
				{
					HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(32515));
					::SetCursor(hCur);
					bHit = TRUE;
					m_state = HITCOLOR;
					m_hitIndex = i;
				}
			}
		}
			
		if (!bHit)
		{
			HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(32512));
			::SetCursor(hCur);
			m_state = NOTHIT;
		}
	}


	CDialogEx::OnMouseMove(nFlags, point);
}





BOOL CSeTransFuncDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	//return CDialogEx::OnSetCursor(pWnd, nHitTest, message);
	return TRUE;
}

COLORREF CSeTransFuncDlg::GetMixColor(TransFuncColorCtlPoint start, TransFuncColorCtlPoint end, int pos)
{
	float fScale = (pos - start.pos) / (end.pos - start.pos);
	COLORREF rstColor = RGB(
		(GetRValue(end.color) - GetRValue(start.color)) * fScale +  GetRValue(start.color),
		(GetGValue(end.color) - GetGValue(start.color)) * fScale +  GetGValue(start.color),
		(GetBValue(end.color) - GetBValue(start.color)) * fScale +  GetBValue(start.color)
		);
	return rstColor;
}

void CSeTransFuncDlg::OnDeleteLinePoint()
{
	if (m_hitIndex != m_vecLightnessCtlPoints.size() - 1 || m_hitIndex != 0)
		m_vecLightnessCtlPoints.erase(m_vecLightnessCtlPoints.begin() + m_hitIndex, m_vecLightnessCtlPoints.begin() + m_hitIndex + 1);
	Invalidate(FALSE);
}

void CSeTransFuncDlg::OnDeleteColorPoint()
{
	if (m_hitIndex != m_vecColorCtlPoints.size() - 1 || m_hitIndex != 0)
		m_vecColorCtlPoints.erase(m_vecColorCtlPoints.begin() + m_hitIndex, m_vecColorCtlPoints.begin() + m_hitIndex + 1);
	Invalidate(FALSE);
}

void CSeTransFuncDlg::OnChangmeColor()
{
	CColorDialog dlg(0, 0, NULL);
	if (IDOK == dlg.DoModal())
	{
		COLORREF color = dlg.GetColor();
		m_vecColorCtlPoints[m_hitIndex].color = color;
	}
	Invalidate(FALSE);

}


void CSeTransFuncDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_pParent->SendMessage(WM_TRANSFUNC, (WPARAM)&m_vecColorCtlPoints, (LPARAM)&m_vecLightnessCtlPoints);
	CDialogEx::OnTimer(nIDEvent);
}


void CSeTransFuncDlg::OnBnClickedButtonSave()
{
	// TODO: 在此添加控件通知处理程序代码
	CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_TRANSFUNC_NAME);
	CString str;
	pEdit->GetWindowText(str);
	if (str.IsEmpty())
	{
		AfxMessageBox(_T("请填写保存模板的名称"));
		return;
	}
	for (int i=0; i<m_vecSetting.size(); i++)
	{
		if (m_vecSetting[i].csSettingName == str)
		{
			if (IDNO == AfxMessageBox(_T("已存在相同名称配置，是否替换"), MB_YESNO))
				return;
		}
	}
	CString strWorkPath = CSeToolKit::GetWorkPath();
	SaveTransFuncSetting(strWorkPath + "\\Config\\TransFunc.ini", str);
}


void CSeTransFuncDlg::OnBnClickedButtonLoad()
{
	// TODO: 在此添加控件通知处理程序代码
	CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_TRANSFUNC_SETTING);
	int nSelect = pCombo->GetCurSel();
	if (nSelect == -1)
		return;
	CString str;
	pCombo->GetLBText(nSelect, str);
	SetNewTransFuncSetting(str);
}

void CSeTransFuncDlg::LoadTransFuncSetting(CString csFile)
{
	CIniFile ini(csFile);
	CStringArray csaTransFuncSettings;
	if (ini.ReadFile())
	{
		CString str = ini.GetValue(_T("参数列表"), "SettingName");
		CSeToolKit::TokenizeString(str, ",", csaTransFuncSettings, TRUE, TRUE);
	}
	else
	{
		AfxMessageBox(_T("读取传递函数文件失败"));
	}

 	for (int i=0; i<csaTransFuncSettings.GetSize(); i++)
 	{
		CString strPts = ini.GetValue(csaTransFuncSettings[i], "Point");
		CString strClrs = ini.GetValue(csaTransFuncSettings[i], "Color");
		if (!strPts.IsEmpty() && !strClrs.IsEmpty() && !csaTransFuncSettings[i].IsEmpty())
			m_vecSetting.push_back(TransFuncSetting(csaTransFuncSettings[i], strPts,strClrs));
// 		CStringArray csaTransFuncPoints;
// 		CStringArray csaTransFuncColors;
// 		CSeToolKit::TokenizeString(strPts, "|", csaTransFuncPoints, TRUE, TRUE);
// 		CSeToolKit::TokenizeString(strClrs, "|", csaTransFuncColors, TRUE, TRUE);
// 		for(int j=0; j<csaTransFuncPoints.GetSize(); j++)
// 		{
// 			CStringArray values;
// 			CSeToolKit::TokenizeString(csaTransFuncPoints[j], ",", values, TRUE, TRUE);
// 			m_vecLightnessCtlPoints.push_back(TransFuncLightnessCtlPoint(atof(values[0]),atof(values[1]),FALSE));
// 		}
 	}
}

void CSeTransFuncDlg::SetNewTransFuncSetting(CString strSettingName)
{

// 	m_vecLightnessCtlPoints.push_back(TransFuncLightnessCtlPoint(m_rtUpPlane.left + 2.0, m_rtUpPlane.bottom - 2.0, FALSE));
// 	m_vecLightnessCtlPoints.push_back(TransFuncLightnessCtlPoint(m_rtUpPlane.right - 2.0, m_rtUpPlane.top + 2.0, FALSE));
// 
// 	m_vecColorCtlPoints.push_back(TransFuncColorCtlPoint(m_rtDownPlane.left + 2.0, RGB(255, 255, 255), FALSE));
// 	m_vecColorCtlPoints.push_back(TransFuncColorCtlPoint(m_rtDownPlane.right - 2.0, RGB(255, 255, 255), FALSE));
	for (int j=0; j<m_vecSetting.size(); j++)
	{
		if (m_vecSetting[j].csSettingName == strSettingName)
		{
			m_vecLightnessCtlPoints.clear();
			m_vecColorCtlPoints.clear();
			CStringArray csaPoints;
			CStringArray csaColors;
			CSeToolKit::TokenizeString(m_vecSetting[j].csSetttingPoints, "|", csaPoints, TRUE, TRUE);
			CSeToolKit::TokenizeString(m_vecSetting[j].csSettingColors, "|", csaColors, TRUE, TRUE);

			// add Points
			CStringArray csaZeroValues;
			CSeToolKit::TokenizeString(csaPoints[0], ",", csaZeroValues, TRUE, TRUE);
			if (atoi(csaZeroValues[0]) > m_nMin)
				m_vecLightnessCtlPoints.push_back(TransFuncLightnessCtlPoint(m_rtUpPlane.left + 2.0, m_rtUpPlane.bottom - 2.0, FALSE));

			for (int i=0; i<csaPoints.GetSize(); i++)
			{
				CStringArray csaValues;
				CSeToolKit::TokenizeString(csaPoints[i], ",", csaValues, TRUE, TRUE);
				int valueX = atoi(csaValues[0]) > m_nMin ? atoi(csaValues[0]) : m_nMin;
				valueX = atoi(csaValues[0]) < m_nMax ? atoi(csaValues[0]) : m_nMax;
				int valueY = atoi(csaValues[1]) > 0 ? atoi(csaValues[1]) : 0;
				valueY = atoi(csaValues[1]) < 255 ? atoi(csaValues[1]) : 255;
				float fXpos = m_rtUpPlane.left + 2.0 + valueX * (m_rtUpPlane.Width() - 4.0) / (m_nMax - m_nMin);
				float fYpos = m_rtUpPlane.top + 2.0 + (256.0 - valueY) * (m_rtUpPlane.Height() - 4.0) / 256.0;
				m_vecLightnessCtlPoints.push_back(TransFuncLightnessCtlPoint(fXpos, fYpos , FALSE));
			}

			csaZeroValues.RemoveAll();
			CSeToolKit::TokenizeString(csaPoints[csaPoints.GetSize() - 1], ",", csaZeroValues, TRUE, TRUE);
			if (atoi(csaZeroValues[0]) < m_nMax)
				m_vecLightnessCtlPoints.push_back(TransFuncLightnessCtlPoint(m_rtUpPlane.right - 2.0, m_rtUpPlane.top + 2.0, FALSE));


			// add Colors
			csaZeroValues.RemoveAll();
			CSeToolKit::TokenizeString(csaPoints[0], ",", csaZeroValues, TRUE, TRUE);
			if (atoi(csaZeroValues[0]) > m_nMin)
				m_vecColorCtlPoints.push_back(TransFuncColorCtlPoint(m_rtDownPlane.left + 2.0, RGB(255, 255, 255), FALSE));

			for (int i=0; i<csaColors.GetSize(); i++)
			{
				CStringArray csaValues;
				CSeToolKit::TokenizeString(csaColors[i], ",", csaValues, TRUE, TRUE);
				int valueX = atoi(csaValues[0]) > m_nMin ? atoi(csaValues[0]) : m_nMin;
				valueX = atoi(csaValues[0]) < m_nMax ? atoi(csaValues[0]) : m_nMax;

				int valueR = atoi(csaValues[1]) > 0 ? atoi(csaValues[1]) : 0;
				valueR = atoi(csaValues[1]) < 255 ? atoi(csaValues[1]) : 255;
				int valueG = atoi(csaValues[2]) > 0 ? atoi(csaValues[2]) : 0;
				valueG = atoi(csaValues[2]) < 255 ? atoi(csaValues[2]) : 255;
				int valueB = atoi(csaValues[3]) > 0 ? atoi(csaValues[3]) : 0;
				valueB = atoi(csaValues[3]) < 255 ? atoi(csaValues[3]) : 255;

				float fPos = m_rtDownPlane.left + 2.0 + valueX * (m_rtDownPlane.Width() - 4.0) / (m_nMax - m_nMin);
				COLORREF color = RGB(valueR, valueG, valueB);
				m_vecColorCtlPoints.push_back(TransFuncColorCtlPoint(fPos, color, FALSE));
			}

			csaZeroValues.RemoveAll();
			CSeToolKit::TokenizeString(csaPoints[csaPoints.GetSize() - 1], ",", csaZeroValues, TRUE, TRUE);
			if (atoi(csaZeroValues[0]) < m_nMax)
				m_vecColorCtlPoints.push_back(TransFuncColorCtlPoint(m_rtDownPlane.right - 2.0, RGB(255, 255, 255), FALSE));

			Invalidate(FALSE);
			return;
		}
	}
}


void CSeTransFuncDlg::OnEnChangeEditTransfuncName()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	CEdit* pEdit;
	pEdit = (CEdit*) GetDlgItem(IDC_EDIT_TRANSFUNC_NAME);
	CString str;
	pEdit->GetWindowText(str);

	if (str.Find(",", 0) != -1 || str.Find("|", 0) != -1)
	{
		pEdit->SetWindowText(str.Left(str.GetLength() - 1));
		int len = pEdit->GetWindowTextLength();
		pEdit->SetSel(len, len, FALSE);
		pEdit->SetFocus();
	}
}

void CSeTransFuncDlg::SaveTransFuncSetting(CString csFile, CString strSettingName)
{
	CString strPts;
	CString strClrs;
	
	for (int i=0; i<m_vecLightnessCtlPoints.size(); i++)
	{
		CString strTmp;
		float fx = m_vecLightnessCtlPoints[i].ptX;
		float fy = m_vecLightnessCtlPoints[i].ptY;

// 		float fXpos = m_rtUpPlane.left + 2.0 + valueX * (m_rtUpPlane.Width() - 4.0) / (m_nMax - m_nMin);
// 		float fYpos = m_rtUpPlane.top + 2.0 + (256.0 - valueY) * (m_rtUpPlane.Height() - 4.0) / 256.0;
		int x = int((fx - m_rtUpPlane.left - 2.0) * (m_nMax - m_nMin) / (m_rtUpPlane.Width() - 4.0));
		int y = 256 - int((fy - m_rtUpPlane.top - 2.0) * 256.0 / (m_rtUpPlane.Height() - 4.0));
		if (strPts.IsEmpty())
			strPts.Format("%d,%d", x, y);
		else
		{
			strTmp.Format("|%d,%d", x, y);
			strPts += strTmp;
		}
	}

	for (int i=0; i<m_vecColorCtlPoints.size(); i++)
	{
		CString strTmp;
		float pos = m_vecColorCtlPoints[i].pos;
		COLORREF color = m_vecColorCtlPoints[i].color;

		int x = int((pos - m_rtDownPlane.left - 2.0) * (m_nMax - m_nMin) / (m_rtDownPlane.Width() - 4.0));
		
		if (strClrs.IsEmpty())
			strClrs.Format("%d,%d,%d,%d", x, GetRValue(color), GetGValue(color), GetBValue(color));
		else
		{
			strTmp.Format("|%d,%d,%d,%d", x, GetRValue(color), GetGValue(color), GetBValue(color));
			strClrs += strTmp;
		}
	}

	CIniFile ini(csFile);
	CStringArray csaTransFuncSettings;
	if (ini.ReadFile())
	{
		CString str = ini.GetValue(_T("参数列表"), "SettingName");
		CSeToolKit::TokenizeString(str, ",", csaTransFuncSettings, TRUE, TRUE);
	}
	else
	{
		AfxMessageBox(_T("读取传递函数文件失败"));
	}

	BOOL bReplace = FALSE;
	int nIndex = -1;
	for (int i=0; i<csaTransFuncSettings.GetSize(); i++)
	{
		if (csaTransFuncSettings[i] == strSettingName)
		{
			bReplace = TRUE;
			nIndex = i;
			break;
		}
	}

	if (bReplace && nIndex != -1)
	{
		ini.SetValue(csaTransFuncSettings[nIndex], "Point", strPts);
		ini.SetValue(csaTransFuncSettings[nIndex], "Color", strClrs);
		m_vecSetting[nIndex].csSettingColors = strClrs;
		m_vecSetting[nIndex].csSetttingPoints =strPts;
	}
	else
	{
		CString strSettingNames = ini.GetValue(_T("参数列表"), "SettingName");
		strSettingNames = strSettingNames + "," + strSettingName;
		ini.SetValue(_T("参数列表"), "SettingName", strSettingNames);
		ini.SetValue(strSettingName, "Point", strPts);
		ini.SetValue(strSettingName, "Color", strClrs);
		if (!strSettingName.IsEmpty() && !strPts.IsEmpty() && !strClrs.IsEmpty())
			m_vecSetting.push_back(TransFuncSetting(strSettingName, strPts, strClrs));
		CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_TRANSFUNC_SETTING);
		pCombo->AddString(strSettingName);
	}

	ini.WriteFile();
}

void CSeTransFuncDlg::RefreshTransFuncSetting()
{
	SetDlgItemText(IDC_STATIC_MIN, CSeToolKit::num2CStr(m_nMin));
	SetDlgItemText(IDC_STATIC_MAX, CSeToolKit::num2CStr(m_nMax));
}
