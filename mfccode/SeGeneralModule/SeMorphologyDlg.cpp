// MorphologyDlg.cpp : 实现文件
//


#include "stdafx.h"
#include "SeGeneralModule.h"
#include "SeMorphologyDlg.h"
#include "afxdialogex.h"


// CSeMorphologyDlg 对话框

IMPLEMENT_DYNAMIC(CSeMorphologyDlg, CDialogEx)

CSeMorphologyDlg::CSeMorphologyDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSeMorphologyDlg::IDD, pParent)
	, m_nKernel(1)
{
	m_pParent = pParent;
	m_bFloodFill = FALSE;
}

CSeMorphologyDlg::~CSeMorphologyDlg()
{
}

void CSeMorphologyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_KERNEL, m_nKernel);
	DDV_MinMaxUInt(pDX, m_nKernel, 1, 100);
}


BEGIN_MESSAGE_MAP(CSeMorphologyDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_EROSION, &CSeMorphologyDlg::OnBnClickedButtonErosion)
	ON_BN_CLICKED(IDC_BUTTON_DELITE, &CSeMorphologyDlg::OnBnClickedButtonDelite)
	ON_BN_CLICKED(IDC_BUTTON_CLOSE, &CSeMorphologyDlg::OnBnClickedButtonClose)
	ON_BN_CLICKED(IDC_BUTTON_OPEN, &CSeMorphologyDlg::OnBnClickedButtonOpen)
	ON_BN_CLICKED(IDC_BUTTON_FLOODFILL, &CSeMorphologyDlg::OnBnClickedButtonFloodfill)
	ON_BN_CLICKED(IDC_BUTTON_INVERSE, &CSeMorphologyDlg::OnBnClickedButtonInverse)
	ON_BN_CLICKED(IDOK, &CSeMorphologyDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CSeMorphologyDlg::OnBnClickedCancel)
END_MESSAGE_MAP()


// CSeMorphologyDlg 消息处理程序


void CSeMorphologyDlg::OnBnClickedButtonErosion()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	m_pParent->SendMessage(WM_MORPHYOLOGY_OPERATION, MORPHOLOGY_CORROSION, m_nKernel);
}


void CSeMorphologyDlg::OnBnClickedButtonDelite()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	m_pParent->SendMessage(WM_MORPHYOLOGY_OPERATION, MORPGOLOGY_DELITE, m_nKernel);
}


void CSeMorphologyDlg::OnBnClickedButtonClose()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	m_pParent->SendMessage(WM_MORPHYOLOGY_OPERATION, MORPHOLOGY_CLOSE, m_nKernel);
}


void CSeMorphologyDlg::OnBnClickedButtonOpen()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	m_pParent->SendMessage(WM_MORPHYOLOGY_OPERATION, MORPHOLOGY_OPEN, m_nKernel);
}


void CSeMorphologyDlg::OnBnClickedButtonFloodfill()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	m_pParent->SendMessage(WM_MORPHYOLOGY_OPERATION, MORPHOLOGY_FLOODFILL, 0);
	m_bFloodFill = TRUE;
}


void CSeMorphologyDlg::OnBnClickedButtonInverse()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	m_pParent->SendMessage(WM_MORPHYOLOGY_OPERATION, MORPHOLOGY_INVERSE, 0);
}


void CSeMorphologyDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_bFloodFill)
		m_pParent->SendMessage(WM_MORPHYOLOGY_OPERATION, MORPHOLOGY_EXECUTE_FLOODFILL, 0);
	m_bFloodFill = FALSE;
	CDialogEx::OnOK();
}


void CSeMorphologyDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_bFloodFill)
		m_pParent->SendMessage(WM_MORPHYOLOGY_OPERATION, MORPHOLOGY_QUIT_FLOODFILL, 0);
	m_bFloodFill = FALSE;
	CDialogEx::OnCancel();
}
