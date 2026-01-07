// SeLightDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "SeGeneralModule.h"
#include "SeLightDlg.h"
#include "afxdialogex.h"

// load image from source
// bool Load( CImage * pimage, LPCTSTR lpszResourceName, HINSTANCE hinstRes)
// {
// 	if (hinstRes == NULL)
// 	{
// 		hinstRes = AfxFindResourceHandle(lpszResourceName, _T("PNG") );
// 	}
// 
// 	HRSRC hRsrc = ::FindResource(hinstRes, lpszResourceName, _T("PNG") );
// 	if (hRsrc == NULL)
// 	{
// 		return false;
// 	}
// 
// 	HGLOBAL hGlobal = LoadResource(hinstRes, hRsrc);
// 	if (hGlobal == NULL)
// 	{
// 		return false;
// 	}
// 
// 	LPBYTE lpBuffer = (LPBYTE) ::LockResource(hGlobal);
// 	if (lpBuffer == NULL)
// 	{
// 		FreeResource(hGlobal);
// 		return false;
// 	}
// 
// 	bool bRes = false;
// 	{
// 		UINT uiSize = ::SizeofResource(hinstRes, hRsrc);
// 
// 		HGLOBAL hRes = ::GlobalAlloc(GMEM_MOVEABLE, uiSize);
// 		if (hRes != NULL)
// 		{
// 			IStream* pStream = NULL;
// 			LPVOID lpResBuffer = ::GlobalLock(hRes);
// 			ASSERT (lpResBuffer != NULL);
// 
// 			memcpy(lpResBuffer, lpBuffer, uiSize);
// 
// 			HRESULT hResult = ::CreateStreamOnHGlobal(hRes, TRUE, &pStream);
// 
// 			if( hResult == S_OK)
// 			{
// 				pimage->Load(pStream);
// 				pStream->Release();
// 				bRes= true;
// 			}
// 		}
// 	}
// 
// 	UnlockResource(hGlobal);
// 	FreeResource(hGlobal);
// 
// 	return bRes;
// }


// CSeLightDlg 对话框

IMPLEMENT_DYNAMIC(CSeLightDlg, CDialogEx)

CSeLightDlg::CSeLightDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSeLightDlg::IDD, pParent)
{
	m_pParentWnd = pParent;
	m_LightInfo.emission = 0.5f;
	m_LightInfo.diffuse = 0.6f;
	m_LightInfo.reflect = 0.3f;
	m_LightInfo.specular = 32.0f;
	m_LightInfo.LightColor = RGB(255, 255, 255);
	m_LightInfo.MaterialColor = RGB(255, 255, 255);
	m_LightInfo.lightPosition = POSITION_3D(2.0f, 2.0f, 3.0f);
	m_LightInfo.shadowD = 0.1f;
	m_LightInfo.light = false;
	m_LightInfo.shadow = false;

	m_bLightMoving = FALSE;
	m_ptLightPos = CPoint(0,0);
	m_rectLightArea = CRect(0,0,0,0);
}

CSeLightDlg::~CSeLightDlg()
{
}

void CSeLightDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSeLightDlg, CDialogEx)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER_EMISSION, &CSeLightDlg::OnNMCustomdrawSliderEmission)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER_DIFFUSE, &CSeLightDlg::OnNMCustomdrawSliderDiffuse)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER_REFLECT, &CSeLightDlg::OnNMCustomdrawSliderReflect)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER_SPECULAR, &CSeLightDlg::OnNMCustomdrawSliderSpecular)
	ON_BN_CLICKED(IDC_COLOR_LIGHT, &CSeLightDlg::OnBnClickedColorLight)
	ON_BN_CLICKED(IDC_COLOR_MATERIAL, &CSeLightDlg::OnBnClickedColorMaterial)
	ON_BN_CLICKED(IDC_CHECK_LIGHT, &CSeLightDlg::OnBnClickedCheckLight)
	ON_BN_CLICKED(IDC_CHECK_SHADOW, &CSeLightDlg::OnBnClickedCheckShadow)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER_SHADOW, &CSeLightDlg::OnNMCustomdrawSliderShadow)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CSeLightDlg 消息处理程序


void CSeLightDlg::OnNMCustomdrawSliderEmission(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	CSliderCtrl* pSlidCtrl = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_EMISSION);
	int nPos = pSlidCtrl->GetPos();
	m_LightInfo.emission = static_cast<float>(nPos) / 100.0f;

	m_pParentWnd->SendMessage(WM_SET_3D_LIGHT, (WPARAM)&m_LightInfo, 0);

	*pResult = 0;
}


void CSeLightDlg::OnNMCustomdrawSliderDiffuse(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	CSliderCtrl* pSlidCtrl = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_DIFFUSE);
	int nPos = pSlidCtrl->GetPos();
	m_LightInfo.diffuse = static_cast<float>(nPos) / 100.0f;

	m_pParentWnd->SendMessage(WM_SET_3D_LIGHT, (WPARAM)&m_LightInfo, 0);

	*pResult = 0;
}


void CSeLightDlg::OnNMCustomdrawSliderReflect(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	CSliderCtrl* pSlidCtrl = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_REFLECT);
	int nPos = pSlidCtrl->GetPos();
	m_LightInfo.reflect = static_cast<float>(nPos) / 100.0f;

	m_pParentWnd->SendMessage(WM_SET_3D_LIGHT, (WPARAM)&m_LightInfo, 0);

	*pResult = 0;
}


void CSeLightDlg::OnNMCustomdrawSliderSpecular(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	CSliderCtrl* pSlidCtrl = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_SPECULAR);
	int nPos = pSlidCtrl->GetPos();
	m_LightInfo.specular = static_cast<float>(nPos);

	m_pParentWnd->SendMessage(WM_SET_3D_LIGHT, (WPARAM)&m_LightInfo, 0);

	*pResult = 0;
}


void CSeLightDlg::OnBnClickedColorLight()
{
	// TODO: 在此添加控件通知处理程序代码
	CMFCColorButton* pClrBtn = (CMFCColorButton*)GetDlgItem(IDC_COLOR_LIGHT);
	m_LightInfo.LightColor = pClrBtn->GetColor();
	m_pParentWnd->SendMessage(WM_SET_3D_LIGHT, (WPARAM)&m_LightInfo, 0);
}


void CSeLightDlg::OnBnClickedColorMaterial()
{
	// TODO: 在此添加控件通知处理程序代码
	CMFCColorButton* pClrBtn = (CMFCColorButton*)GetDlgItem(IDC_COLOR_MATERIAL);
	m_LightInfo.MaterialColor = pClrBtn->GetColor();
	m_pParentWnd->SendMessage(WM_SET_3D_LIGHT, (WPARAM)&m_LightInfo, 0);
}


void CSeLightDlg::OnBnClickedCheckLight()
{
	// TODO: 在此添加控件通知处理程序代码
	CButton* pCheckButton = (CButton*)GetDlgItem(IDC_CHECK_LIGHT);
	if(pCheckButton->GetCheck())
	{
		m_LightInfo.light = true;
	}
	else
	{
		m_LightInfo.light = false;
	}
	m_pParentWnd->SendMessage(WM_SET_3D_LIGHT, (WPARAM)&m_LightInfo, 0);
}


void CSeLightDlg::OnBnClickedCheckShadow()
{
	// TODO: 在此添加控件通知处理程序代码
	CButton* pCheckButton = (CButton*)GetDlgItem(IDC_CHECK_SHADOW);
	if(pCheckButton->GetCheck())
	{
		m_LightInfo.shadow = true;
	}
	else
	{
		m_LightInfo.shadow = false;
	}
	m_pParentWnd->SendMessage(WM_SET_3D_LIGHT, (WPARAM)&m_LightInfo, 0);
}


BOOL CSeLightDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CString strWorkPath = CSeToolKit::GetWorkPath();
	LoadLightSetting(strWorkPath + "\\Config\\Light.ini");

	// TODO:  在此添加额外的初始化
	CSliderCtrl* pSlidCtrl = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_EMISSION);
	pSlidCtrl->SetRange(1,100,TRUE);// 设置滑动条范围
	pSlidCtrl->SetPos(m_LightInfo.emission * 100);// 设置滑动条位置

	pSlidCtrl = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_DIFFUSE);
	pSlidCtrl->SetRange(1,100,TRUE);// 设置滑动条范围
	pSlidCtrl->SetPos(m_LightInfo.diffuse * 100);// 设置滑动条位置

	pSlidCtrl = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_REFLECT);
	pSlidCtrl->SetRange(1,100,TRUE);// 设置滑动条范围
	pSlidCtrl->SetPos(m_LightInfo.reflect * 100);// 设置滑动条位置

	pSlidCtrl = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_SPECULAR);
	pSlidCtrl->SetRange(1,64,TRUE);// 设置滑动条范围
	pSlidCtrl->SetPos(m_LightInfo.specular);// 设置滑动条位置

	pSlidCtrl = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_SHADOW);
	pSlidCtrl->SetRange(1,100,TRUE);// 设置滑动条范围
	pSlidCtrl->SetPos(m_LightInfo.shadowD * 100);// 设置滑动条位置

	CMFCColorButton* pClrBtn = (CMFCColorButton*)GetDlgItem(IDC_COLOR_LIGHT);
	pClrBtn->SetColor(m_LightInfo.LightColor);

	pClrBtn = (CMFCColorButton*)GetDlgItem(IDC_COLOR_MATERIAL);
	pClrBtn->SetColor(m_LightInfo.MaterialColor);

	CButton* pCheckButton = (CButton*)GetDlgItem(IDC_CHECK_LIGHT);
	pCheckButton->SetCheck(m_LightInfo.light);

	pCheckButton = (CButton*)GetDlgItem(IDC_CHECK_SHADOW);
	pCheckButton->SetCheck(m_LightInfo.shadow);

	// m_imgLight.Load(IDB_PNG_LIGHT, AfxGetResourceHandle());
	SetTimer(0, 1000, NULL);
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CSeLightDlg::OnNMCustomdrawSliderShadow(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	CSliderCtrl* pSlidCtrl = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_SHADOW);
	int nPos = pSlidCtrl->GetPos();
	m_LightInfo.shadowD = static_cast<float>(nPos) / 100.0f;

	m_pParentWnd->SendMessage(WM_SET_3D_LIGHT, (WPARAM)&m_LightInfo, 0);

	*pResult = 0;
}


void CSeLightDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: 在此处添加消息处理程序代码
	// 不为绘图消息调用 CDialogEx::OnPaint()
	CStatic* pPicture = (CStatic*)GetDlgItem(IDC_PICTURE_LIGHT_POS);	
	if (pPicture != NULL)
	{
		CRect rect;
		CRect rectMain;
		this->GetWindowRect(&rectMain);
		pPicture->GetWindowRect(&rect);
		CPoint ptBR = rect.BottomRight() - rectMain.TopLeft() - CPoint(3, 26);
		CPoint ptTL = rect.TopLeft() - rectMain.TopLeft() - CPoint(3, 26);
		m_rectLightArea = CRect(ptTL, ptBR);

		DrawLight(&dc, m_ptLightPos, m_rectLightArea);
	}
	
}

void CSeLightDlg::DrawLight(CPaintDC* pDC, CPoint ptCenter, CRect rectValid)
{
	rectValid.DeflateRect(12, 9, 12 ,9);
	if (!rectValid.PtInRect(ptCenter))
	{
		if (ptCenter.x > rectValid.right)
		{
			ptCenter.x = rectValid.right;
		}
		else if(ptCenter.x < rectValid.left)
		{
			ptCenter.x = rectValid.left;
		}
		if (ptCenter.y > rectValid.bottom)
		{
			ptCenter.y = rectValid.bottom;
		}
		else if (ptCenter.y < rectValid.top)
		{
			ptCenter.y = rectValid.top;
		}
	}
	
	CBrush brush;
	brush.CreateSolidBrush(RGB(255,201,14));
	pDC->SelectObject(&brush);
	CPoint ptTopLeft = ptCenter - CPoint(10, 7);
	CPoint ptTopCenter = ptCenter - CPoint(0, 7);
	CPoint ptTopRight = ptCenter + CPoint(10, -7);
	CPoint ptBottomRight = ptCenter + CPoint(10, 7);
	CPoint ptRightCenter = ptCenter + CPoint(10, 0);
	CPoint ptLightTop = ptCenter + CPoint(3, -4);
	CPoint ptLightCenter = ptCenter + CPoint(3, 0);
	CPoint ptLightBottom = ptCenter + CPoint(3, 4);
	CRect rectEllipse(ptTopLeft, ptBottomRight);
	pDC->Ellipse(rectEllipse);
	CRect rectRemove(ptTopCenter, ptBottomRight);
	brush.DeleteObject();
	brush.CreateSolidBrush(RGB(240, 240, 240));
	pDC->SelectObject(&brush);
	pDC->FillRect(rectRemove, &brush);
	CPen pen;
	pen.CreatePen(PS_SOLID, 3, RGB(255,201,14));
	pDC->SelectObject(&pen);
	pDC->MoveTo(ptLightTop);
	pDC->LineTo(ptTopRight);
	pDC->MoveTo(ptLightCenter);
	pDC->LineTo(ptRightCenter);
	pDC->MoveTo(ptLightBottom);
	pDC->LineTo(ptBottomRight);

	m_LightInfo.lightPosition.fXpos = ((ptCenter - rectValid.TopLeft()).cx - rectValid.Width()/ 2) * 5.0f / rectValid.Width();
	m_LightInfo.lightPosition.fYpos = ((ptCenter - rectValid.TopLeft()).cy - rectValid.Height()/ 2) * -5.0f / rectValid.Height();
	m_LightInfo.lightPosition.fZpos = 3.0f;
	m_pParentWnd->SendMessage(WM_SET_3D_LIGHT, (WPARAM)&m_LightInfo, 0);
}


void CSeLightDlg::LoadLightSetting(CString csFile)
{
	CIniFile ini(csFile);
	CStringArray csaTransFuncSettings;
	if (ini.ReadFile())
	{
		CString csEmission = ini.GetValue(_T("Emission"), "value");
		CString csDiffuse = ini.GetValue(_T("Diffuse"), "value");
		CString csReflect = ini.GetValue(_T("Reflect"), "value");
		CString csSpecular = ini.GetValue(_T("Specular"), "value");
		CString csShadow = ini.GetValue(_T("Shadow"), "value");
		CString csLightOn = ini.GetValue(_T("LightOn"), "value");
		CString csShadowOn = ini.GetValue(_T("ShadowOn"), "value");
		CString csLightColor = ini.GetValue(_T("LightColor"), "value");
		CString csMateiralColor = ini.GetValue(_T("MateiralColor"), "value");
		CString csLightX = ini.GetValue(_T("LightX"), "value");
		CString csLightY = ini.GetValue(_T("LightY"), "value");

		m_LightInfo.emission = atof(csEmission) / 100.0;
		m_LightInfo.diffuse = atof(csDiffuse) / 100.0;
		m_LightInfo.reflect = atof(csReflect) / 100.0;
		m_LightInfo.specular = atof(csSpecular);
		m_LightInfo.shadowD = atof(csShadow) / 100.0;
		if (atoi(csLightOn) == 1)
			m_LightInfo.light = true;
		else
			m_LightInfo.light = false;
		if (atoi(csShadowOn) == 1)
			m_LightInfo.shadow = true;
		else
			m_LightInfo.shadow = false;

		m_LightInfo.LightColor = atoi(csLightColor);
		m_LightInfo.MaterialColor = atoi(csMateiralColor);
	}
}



void CSeLightDlg::SaveLightSetting(CString csFile)
{
	CIniFile ini(csFile);
	CStringArray csaTransFuncSettings;
	if (ini.ReadFile())
	{
		ini.SetValueI(_T("Emission"), "value", static_cast<int>(m_LightInfo.emission * 100));
		ini.SetValueI(_T("Diffuse"), "value", static_cast<int>(m_LightInfo.diffuse * 100));
		ini.SetValueI(_T("Reflect"), "value", static_cast<int>(m_LightInfo.reflect * 100));
		ini.SetValueI(_T("Specular"), "value", static_cast<int>(m_LightInfo.specular));
		ini.SetValueI(_T("Shadow"), "value", static_cast<int>(m_LightInfo.shadowD * 100));
		if (m_LightInfo.light)
			ini.SetValueI(_T("LightOn"), "value", 1);
		else
			ini.SetValueI(_T("LightOn"), "value", 0);
		if (m_LightInfo.shadow)
			ini.SetValueI(_T("ShadowOn"), "value", 1);
		else
			ini.SetValueI(_T("ShadowOn"), "value", 0);

		ini.SetValueI(_T("LightColor"), "value", static_cast<int>(m_LightInfo.LightColor));
		ini.SetValueI(_T("MateiralColor"), "value", static_cast<int>(m_LightInfo.MaterialColor));
		ini.SetValueI(_T("LightX"), "value", static_cast<int>(m_LightInfo.lightPosition.fXpos * 100));
		ini.SetValueI(_T("LightY"), "value", static_cast<int>(m_LightInfo.lightPosition.fYpos * 100));
	}
	ini.WriteFile();
}

void CSeLightDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_bLightMoving = TRUE;
	CDialogEx::OnLButtonDown(nFlags, point);
}


void CSeLightDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_bLightMoving = FALSE;
	CDialogEx::OnLButtonUp(nFlags, point);
}


void CSeLightDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if(m_bLightMoving)
	{
		if (m_rectLightArea.PtInRect(point))
		{
			m_ptLightPos = point;
			InvalidateRect(m_rectLightArea, TRUE);
		}
	}
	CDialogEx::OnMouseMove(nFlags, point);
}





void CSeLightDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nIDEvent == 0)
	{
		CString strWorkPath = CSeToolKit::GetWorkPath();
		SaveLightSetting(strWorkPath + "\\Config\\Light.ini");
	}
	CDialogEx::OnTimer(nIDEvent);
}
