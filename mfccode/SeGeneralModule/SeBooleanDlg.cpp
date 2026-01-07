// SeBOOLeanOperation.cpp : 实现文件
//

#include "stdafx.h"
#include "SeGeneralModule.h"
#include "SeBooleanDlg.h"
#include "afxdialogex.h"


// CSeBOOLeanOperation 对话框

IMPLEMENT_DYNAMIC(CSeBooleanDlg, CDialogEx)

CSeBooleanDlg::CSeBooleanDlg(CWnd* pParent/* =NULL*/)
	: CDialogEx(CSeBooleanDlg::IDD, pParent)
{
	m_pParent = pParent;
}

CSeBooleanDlg::~CSeBooleanDlg()
{
}

void CSeBooleanDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO2, m_cbOperation);
	DDX_Control(pDX, IDC_COMBO1, m_cbMaskA);
	DDX_Control(pDX, IDC_COMBO3, m_cbMaskB);
}


BEGIN_MESSAGE_MAP(CSeBooleanDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CSeBooleanDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CSeBooleanDlg::OnBnClickedCancel)
END_MESSAGE_MAP()


// CSeBOOLeanOperation 消息处理程序


void CSeBooleanDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	ShowWindow(SW_HIDE);
	UpdateData(TRUE);
// 	int nMaskA = m_cbMaskA.GetCurSel();
// 	int nOperation = m_cbOperation.GetCurSel();
// 	int nMaskB = m_cbMaskB.GetCurSel();
	
	int nParams[3];
	nParams[0] = m_cbMaskA.GetCurSel();
	nParams[1] = m_cbOperation.GetCurSel();
	nParams[2] = m_cbMaskB.GetCurSel();
	m_pParent->SendMessage(WM_BOOLEAN_OPERATION, (WPARAM)&nParams[0], 0);
	CDialogEx::OnOK();
}


void CSeBooleanDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnCancel();
}




BOOL CSeBooleanDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CSeBooleanDlg::UpdateCBData()
{
	m_cbMaskA.ResetContent();
	m_cbMaskB.ResetContent();
	m_cbOperation.ResetContent();
// 	m_colorList.clear();
// 	m_colorList.push_back(RGB(255,0,0));
// 	m_colorList.push_back(RGB(255,255,0));
// 	m_colorList.push_back(RGB(0,255,255));
// 	m_colorList.push_back(RGB(0,255,0));
// 	m_colorList.push_back(RGB(0,0,255));
	const int nSize = m_colorList.size();
	for (int i=0; i<nSize; i++)
	{
		m_cbMaskA.AddItem("", m_colorList[i]);	
		m_cbMaskB.AddItem("", m_colorList[i]);	
	}
	m_cbOperation.AddString("Union");
	m_cbOperation.AddString("Xor");
	m_cbOperation.AddString("Intersect");

	m_cbMaskA.SetCurSel(0);
	m_cbMaskB.SetCurSel(0);
	m_cbOperation.SetCurSel(0);

}
