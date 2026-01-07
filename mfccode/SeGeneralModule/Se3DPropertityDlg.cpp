// Se3DPropertityDlg.cpp : 实现文件
#include "stdafx.h"
#include "SeGeneralModule.h"
#include "Se3DPropertityDlg.h"
#include "afxdialogex.h"
#include "resource.h"


// CSe3DPropertityDlg 对话框

IMPLEMENT_DYNAMIC(CSe3DPropertityDlg, CDialogEx)

CSe3DPropertityDlg::CSe3DPropertityDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSe3DPropertityDlg::IDD, pParent)
{
// 	m_matRotation = glm::mat4(1.0f);
// 	m_vec3RotateCenter = glm::vec3(-0.5f);
// 	m_fViewAngle = 45.0f;
// 	m_nMoveSpeed = 4;
// 	m_nRotateSpeed = 4;
// 	m_nScaleSpeed = 4;
// 	m_vec3LightColor = glm::vec3(1.0f);
// 	m_vec3CameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
// 	m_vec3LightPos = glm::vec3(1.2f, 1.0f, 2.0f);

	// 初始化总设置信息
	m_pParentWnd = pParent;
	m_volumeInfo.nMoveSpeed = 50;
	m_volumeInfo.nRotateSpeed = 50;
	m_volumeInfo.nScaleSpeed = 50;
	// m_volumeInfo.lightPosition = POSITION_3D(0.0f, 0.0f, 3.0f);
	// m_volumeInfo.ViewPosition = POSITION_3D(0.0f, 0.0f, 3.0f);
	m_volumeInfo.color = RGB(0, 0, 0);


}

CSe3DPropertityDlg::~CSe3DPropertityDlg()
{
}

void CSe3DPropertityDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSe3DPropertityDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CSe3DPropertityDlg::OnBnClickedOk)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER_MOVESPEED, &CSe3DPropertityDlg::OnNMCustomdrawSliderMovespeed)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER_ROTATESPEED, &CSe3DPropertityDlg::OnNMCustomdrawSliderRotatespeed)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER_SCALESPEED, &CSe3DPropertityDlg::OnNMCustomdrawSliderScalespeed)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER_QUALITY, &CSe3DPropertityDlg::OnNMCustomdrawSliderQuality)

// 	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_LIGHT_POS_X, &CSe3DPropertityDlg::OnDeltaposSpinLightPosX)
// 	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_LIGHT_POS_Y, &CSe3DPropertityDlg::OnDeltaposSpinLightPosY)
// 	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_LIGHT_POS_Z, &CSe3DPropertityDlg::OnDeltaposSpinLightPosZ)
// 	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_VIEW_POS_X, &CSe3DPropertityDlg::OnDeltaposSpinViewPosX)
// 	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_VIEW_POS_Y, &CSe3DPropertityDlg::OnDeltaposSpinViewPosY)
// 	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_VIEW_POS_Z, &CSe3DPropertityDlg::OnDeltaposSpinViewPosZ)
	ON_BN_CLICKED(IDC_MFCCOLORBUTTON_BK_GND, &CSe3DPropertityDlg::OnBnClickedMfccolorbuttonBkGnd)
END_MESSAGE_MAP()


// CSe3DPropertityDlg 消息处理程序


void CSe3DPropertityDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
}


BOOL CSe3DPropertityDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	// TODO:  在此添加额外的初始化

	// 初始化控制条
	CSliderCtrl* pSlidCtrl = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_MOVESPEED);
	pSlidCtrl->SetRange(1,100,TRUE);// 设置滑动条范围
	pSlidCtrl->SetPos(50);// 设置滑动条位置

	pSlidCtrl = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_ROTATESPEED);
	pSlidCtrl->SetRange(1,100,TRUE);// 设置滑动条范围
	pSlidCtrl->SetPos(50);// 设置滑动条位置

	pSlidCtrl = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_SCALESPEED);
	pSlidCtrl->SetRange(1,100,TRUE);// 设置滑动条范围
	pSlidCtrl->SetPos(50);// 设置滑动条位置

	pSlidCtrl = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_QUALITY);
	pSlidCtrl->SetRange(1,100,TRUE);// 设置滑动条范围
	pSlidCtrl->SetPos(50);// 设置滑动条位置

	// 初始化 编辑框
// 	CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_LIGHT_POS_X);
// 	pEdit->SetWindowTextA("0.0");
// 
// 	pEdit = (CEdit*)GetDlgItem(IDC_EDIT_LIGHT_POS_Y);
// 	pEdit->SetWindowTextA("0.0");
// 
// 	pEdit = (CEdit*)GetDlgItem(IDC_EDIT_LIGHT_POS_Z);
// 	pEdit->SetWindowTextA("3.0");
// 
// 	pEdit = (CEdit*)GetDlgItem(IDC_EDIT_VIEW_POS_X);
// 	pEdit->SetWindowTextA("1.2");
// 
// 	pEdit = (CEdit*)GetDlgItem(IDC_EDIT_VIEW_POS_Y);
// 	pEdit->SetWindowTextA("1.0");
// 
// 	pEdit = (CEdit*)GetDlgItem(IDC_EDIT_VIEW_POS_Z);
// 	pEdit->SetWindowTextA("2.0");


	// 初始化颜色控制
	CMFCColorButton* pClrBtn = (CMFCColorButton*)GetDlgItem(IDC_MFCCOLORBUTTON_BK_GND);
	pClrBtn->SetColor(m_volumeInfo.color);

	// 绑定 Spin 控件到 Edit 控件
// 	CSpinButtonCtrl* pCtrl = (CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_LIGHT_POS_X); 
// 	pCtrl->SetBuddy(GetDlgItem(IDC_EDIT_LIGHT_POS_X)); 
// 
// 	pCtrl = (CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_LIGHT_POS_Y); 
// 	pCtrl->SetBuddy(GetDlgItem(IDC_EDIT_LIGHT_POS_Y)); 
// 
// 	pCtrl = (CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_LIGHT_POS_Z); 
// 	pCtrl->SetBuddy(GetDlgItem(IDC_EDIT_LIGHT_POS_Z)); 
// 
// 	pCtrl = (CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_VIEW_POS_X); 
// 	pCtrl->SetBuddy(GetDlgItem(IDC_EDIT_VIEW_POS_X)); 
// 
// 	pCtrl = (CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_VIEW_POS_Y); 
// 	pCtrl->SetBuddy(GetDlgItem(IDC_EDIT_VIEW_POS_Y)); 
// 
// 	pCtrl = (CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_VIEW_POS_Z); 
// 	pCtrl->SetBuddy(GetDlgItem(IDC_EDIT_VIEW_POS_Z)); 
 
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CSe3DPropertityDlg::OnNMCustomdrawSliderMovespeed(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码

	CSliderCtrl* pSlidCtrl = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_MOVESPEED);
	int nPos = pSlidCtrl->GetPos();
	m_volumeInfo.nMoveSpeed = nPos;

	m_pParentWnd->SendMessage(WM_SET_3D_PERFORMANCE, (WPARAM)&m_volumeInfo, 0);
	*pResult = 0;
}


void CSe3DPropertityDlg::OnNMCustomdrawSliderRotatespeed(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	CSliderCtrl* pSlidCtrl = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_ROTATESPEED);
	int nPos = pSlidCtrl->GetPos();
	m_volumeInfo.nRotateSpeed = nPos;

	m_pParentWnd->SendMessage(WM_SET_3D_PERFORMANCE, (WPARAM)&m_volumeInfo, 0);
	*pResult = 0;
}


void CSe3DPropertityDlg::OnNMCustomdrawSliderScalespeed(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	CSliderCtrl* pSlidCtrl = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_SCALESPEED);
	int nPos = pSlidCtrl->GetPos();
	m_volumeInfo.nScaleSpeed = nPos;

	m_pParentWnd->SendMessage(WM_SET_3D_PERFORMANCE, (WPARAM)&m_volumeInfo, 0);
	*pResult = 0;
}


void CSe3DPropertityDlg::OnNMCustomdrawSliderQuality(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	CSliderCtrl* pSlidCtrl = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_QUALITY);
	int nPos = pSlidCtrl->GetPos();
	m_volumeInfo.nQuality = nPos;

	m_pParentWnd->SendMessage(WM_SET_3D_PERFORMANCE, (WPARAM)&m_volumeInfo, 0);
	*pResult = 0;
}


// void CSe3DPropertityDlg::OnDeltaposSpinLightPosX(NMHDR *pNMHDR, LRESULT *pResult)
// {
// 	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
// 	// TODO: 在此添加控件通知处理程序代码
// 	UpdateData(true);
// 	if(pNMUpDown->iDelta == 1) // 如果此值为-1 , 说明点击了Spin的往下的箭头
// 	{
// 		CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_LIGHT_POS_X);
// 		CString str;
// 		GetDlgItemTextA(IDC_EDIT_LIGHT_POS_X, str);
// 		double value = _ttof(str);
// 		value -= 0.1;
// 		str.Format("%.1f", value);
// 		SetDlgItemTextA(IDC_EDIT_LIGHT_POS_X, str);
// 		m_volumeInfo.lightPosition.fXpos = value;
// 	}
// 	else if(pNMUpDown->iDelta == -1) // 如果此值为1, 说明点击了Spin的往上的箭头
// 	{
// 		CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_LIGHT_POS_X);
// 		CString str;
// 		GetDlgItemTextA(IDC_EDIT_LIGHT_POS_X, str);
// 		double value = _ttof(str);
// 		value += 0.1;
// 		str.Format("%.1f", value);
// 		SetDlgItemTextA(IDC_EDIT_LIGHT_POS_X, str);
// 		m_volumeInfo.lightPosition.fXpos = value;
// 	}
// 	UpdateData(false);
// 
// 	m_pParentWnd->SendMessage(WM_SET_3D_PERFORMANCE, (WPARAM)&m_volumeInfo, 0);
// 	*pResult = 0;
// }
// 
// 
// void CSe3DPropertityDlg::OnDeltaposSpinLightPosY(NMHDR *pNMHDR, LRESULT *pResult)
// {
// 	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
// 	// TODO: 在此添加控件通知处理程序代码
// 	UpdateData(true);
// 	if(pNMUpDown->iDelta == 1) // 如果此值为-1 , 说明点击了Spin的往下的箭头
// 	{
// 		CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_LIGHT_POS_Y);
// 		CString str;
// 		GetDlgItemTextA(IDC_EDIT_LIGHT_POS_Y, str);
// 		double value = _ttof(str);
// 		value -= 0.1;
// 		str.Format("%.1f", value);
// 		SetDlgItemTextA(IDC_EDIT_LIGHT_POS_Y, str);
// 		m_volumeInfo.lightPosition.fYpos = value;
// 	}
// 	else if(pNMUpDown->iDelta == -1) // 如果此值为1, 说明点击了Spin的往上的箭头
// 	{
// 		CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_LIGHT_POS_Y);
// 		CString str;
// 		GetDlgItemTextA(IDC_EDIT_LIGHT_POS_Y, str);
// 		double value = _ttof(str);
// 		value += 0.1;
// 		str.Format("%.1f", value);
// 		SetDlgItemTextA(IDC_EDIT_LIGHT_POS_Y, str);
// 		m_volumeInfo.lightPosition.fYpos = value;
// 	}
// 	UpdateData(false);
// 
// 	m_pParentWnd->SendMessage(WM_SET_3D_PERFORMANCE, (WPARAM)&m_volumeInfo, 0);
// 	*pResult = 0;
// }
// 
// 
// void CSe3DPropertityDlg::OnDeltaposSpinLightPosZ(NMHDR *pNMHDR, LRESULT *pResult)
// {
// 	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
// 	// TODO: 在此添加控件通知处理程序代码
// 	UpdateData(true);
// 	if(pNMUpDown->iDelta == 1) // 如果此值为-1 , 说明点击了Spin的往下的箭头
// 	{
// 		CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_LIGHT_POS_Z);
// 		CString str;
// 		GetDlgItemTextA(IDC_EDIT_LIGHT_POS_Z, str);
// 		double value = _ttof(str);
// 		value -= 0.1;
// 		str.Format("%.1f", value);
// 		SetDlgItemTextA(IDC_EDIT_LIGHT_POS_Z, str);
// 		m_volumeInfo.lightPosition.fZpos = value;
// 	}
// 	else if(pNMUpDown->iDelta == -1) // 如果此值为1, 说明点击了Spin的往上的箭头
// 	{
// 		CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_LIGHT_POS_Z);
// 		CString str;
// 		GetDlgItemTextA(IDC_EDIT_LIGHT_POS_Z, str);
// 		double value = _ttof(str);
// 		value += 0.1;
// 		str.Format("%.1f", value);
// 		SetDlgItemTextA(IDC_EDIT_LIGHT_POS_Z, str);
// 		m_volumeInfo.lightPosition.fZpos = value;
// 	}
// 	UpdateData(false);
// 
// 	m_pParentWnd->SendMessage(WM_SET_3D_PERFORMANCE, (WPARAM)&m_volumeInfo, 0);
// 	*pResult = 0;
// }
// 
// 
// void CSe3DPropertityDlg::OnDeltaposSpinViewPosX(NMHDR *pNMHDR, LRESULT *pResult)
// {
// 	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
// 	// TODO: 在此添加控件通知处理程序代码
// 	UpdateData(true);
// 	if(pNMUpDown->iDelta == 1) // 如果此值为-1 , 说明点击了Spin的往下的箭头
// 	{
// 		CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_VIEW_POS_X);
// 		CString str;
// 		GetDlgItemTextA(IDC_EDIT_VIEW_POS_X, str);
// 		double value = _ttof(str);
// 		value -= 0.1;
// 		str.Format("%.1f", value);
// 		SetDlgItemTextA(IDC_EDIT_VIEW_POS_X, str);
// 		m_volumeInfo.ViewPosition.fXpos = value;
// 	}
// 	else if(pNMUpDown->iDelta == -1) // 如果此值为1, 说明点击了Spin的往上的箭头
// 	{
// 		CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_VIEW_POS_X);
// 		CString str;
// 		GetDlgItemTextA(IDC_EDIT_VIEW_POS_X, str);
// 		double value = _ttof(str);
// 		value += 0.1;
// 		str.Format("%.1f", value);
// 		SetDlgItemTextA(IDC_EDIT_VIEW_POS_X, str);
// 		m_volumeInfo.ViewPosition.fXpos = value;
// 	}
// 	UpdateData(false);
// 
// 	m_pParentWnd->SendMessage(WM_SET_3D_PERFORMANCE, (WPARAM)&m_volumeInfo, 0);
// 	*pResult = 0;
// }
// 
// 
// void CSe3DPropertityDlg::OnDeltaposSpinViewPosY(NMHDR *pNMHDR, LRESULT *pResult)
// {
// 	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
// 	// TODO: 在此添加控件通知处理程序代码
// 	UpdateData(true);
// 	if(pNMUpDown->iDelta == 1) // 如果此值为-1 , 说明点击了Spin的往下的箭头
// 	{
// 		CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_VIEW_POS_Y);
// 		CString str;
// 		GetDlgItemTextA(IDC_EDIT_VIEW_POS_Y, str);
// 		double value = _ttof(str);
// 		value -= 0.1;
// 		str.Format("%.1f", value);
// 		SetDlgItemTextA(IDC_EDIT_VIEW_POS_Y, str);
// 		m_volumeInfo.ViewPosition.fYpos = value;
// 	}
// 	else if(pNMUpDown->iDelta == -1) // 如果此值为1, 说明点击了Spin的往上的箭头
// 	{
// 		CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_VIEW_POS_Y);
// 		CString str;
// 		GetDlgItemTextA(IDC_EDIT_VIEW_POS_Y, str);
// 		double value = _ttof(str);
// 		value += 0.1;
// 		str.Format("%.1f", value);
// 		SetDlgItemTextA(IDC_EDIT_VIEW_POS_Y, str);
// 		m_volumeInfo.ViewPosition.fYpos = value;
// 	}
// 	UpdateData(false);
// 
// 	m_pParentWnd->SendMessage(WM_SET_3D_PERFORMANCE, (WPARAM)&m_volumeInfo, 0);
// 	*pResult = 0;
// }
// 
// 
// void CSe3DPropertityDlg::OnDeltaposSpinViewPosZ(NMHDR *pNMHDR, LRESULT *pResult)
// {
// 	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
// 	// TODO: 在此添加控件通知处理程序代码
// 	UpdateData(true);
// 	if(pNMUpDown->iDelta == 1) // 如果此值为-1 , 说明点击了Spin的往下的箭头
// 	{
// 		CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_VIEW_POS_Z);
// 		CString str;
// 		GetDlgItemTextA(IDC_EDIT_VIEW_POS_Z, str);
// 		double value = _ttof(str);
// 		value -= 0.1;
// 		str.Format("%.1f", value);
// 		SetDlgItemTextA(IDC_EDIT_VIEW_POS_Z, str);
// 		m_volumeInfo.ViewPosition.fZpos = value;
// 	}
// 	else if(pNMUpDown->iDelta == -1) // 如果此值为1, 说明点击了Spin的往上的箭头
// 	{
// 		CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_VIEW_POS_Z);
// 		CString str;
// 		GetDlgItemTextA(IDC_EDIT_VIEW_POS_Z, str);
// 		double value = _ttof(str);
// 		value += 0.1;
// 		str.Format("%.1f", value);
// 		SetDlgItemTextA(IDC_EDIT_VIEW_POS_Z, str);
// 		m_volumeInfo.ViewPosition.fZpos = value;
// 	}
// 	UpdateData(false);
// 
// 	m_pParentWnd->SendMessage(WM_SET_3D_PERFORMANCE, (WPARAM)&m_volumeInfo, 0);
// 	*pResult = 0;
// }





void CSe3DPropertityDlg::OnBnClickedMfccolorbuttonBkGnd()
{
	// TODO: 在此添加控件通知处理程序代码
	CMFCColorButton* pClrBtn = (CMFCColorButton*)GetDlgItem(IDC_MFCCOLORBUTTON_BK_GND);
	m_volumeInfo.color = pClrBtn->GetColor();
	m_pParentWnd->SendMessage(WM_SET_3D_PERFORMANCE, (WPARAM)&m_volumeInfo, 0);
}

