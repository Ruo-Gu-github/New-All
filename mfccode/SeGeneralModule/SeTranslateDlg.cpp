// SeTranslateDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "SeGeneralModule.h"
#include "SeTranslateDlg.h"
#include "afxdialogex.h"


// CSeTranslateDlg 对话框

IMPLEMENT_DYNAMIC(CSeTranslateDlg, CDialogEx)

CSeTranslateDlg::CSeTranslateDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSeTranslateDlg::IDD, pParent)
	, m_nTranslate(0)
	, m_nTranslateEdit(0)
	, m_nTranValue(0)
{
	m_pParent = pParent;
}

CSeTranslateDlg::CSeTranslateDlg(CWnd* pParent /*=NULL*/, int nRow, int nTranslate)
	: CDialogEx(CSeTranslateDlg::IDD, pParent)
	, m_nTranslate(0)
	, m_nTranslateEdit(0)
	, m_nTranValue(0)
{
	m_pParent = pParent;
	m_nRow = nRow;
	m_nTranValue = m_nTranslate = m_nTranslateEdit = (100 - nTranslate);
}

CSeTranslateDlg::~CSeTranslateDlg()
{
}

void CSeTranslateDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Slider(pDX, IDC_SLIDER1, m_nTranslate);
	DDV_MinMaxInt(pDX, m_nTranslate, 0, 100);
	DDX_Text(pDX, IDC_EDIT1, m_nTranslateEdit);
	DDV_MinMaxInt(pDX, m_nTranslateEdit, 0, 100);
}


BEGIN_MESSAGE_MAP(CSeTranslateDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CSeTranslateDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CSeTranslateDlg::OnBnClickedCancel)
	ON_EN_CHANGE(IDC_EDIT1, &CSeTranslateDlg::OnEnChangeEdit1)
	ON_WM_HSCROLL()
	ON_EN_KILLFOCUS(IDC_EDIT1, &CSeTranslateDlg::OnEnKillfocusEdit1)
END_MESSAGE_MAP()


// CSeTranslateDlg 消息处理程序


void CSeTranslateDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	if (m_nTranslateEdit > 100)
		m_nTranslateEdit = 100;
	m_nTranslate = m_nTranslateEdit;
	m_pParent->SendMessage(WM_TRANSLATE, (WPARAM)(100 - m_nTranslate), m_nRow);
	UpdateData(FALSE);
	if(GetDlgItem(IDOK)==GetFocus())
	{
		m_nTranValue = m_nTranslate;
		m_pParent->SendMessage(WM_TRANSLATE, (WPARAM)(100 - m_nTranValue), m_nRow);
		CDialog::OnOK();    
	}
}


void CSeTranslateDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	m_pParent->SendMessage(WM_TRANSLATE, (WPARAM)(100 - m_nTranValue), m_nRow);
	CDialogEx::OnCancel();
}


void CSeTranslateDlg::OnEnChangeEdit1()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
}




void CSeTranslateDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);

	if ((GetDlgItem(IDC_SLIDER1)) ==(CSliderCtrl *) pScrollBar)         // mSlider 为你的slider控件
	{
		UpdateData(TRUE);
		m_nTranslateEdit = m_nTranslate;
		m_pParent->SendMessage(WM_TRANSLATE, (WPARAM)(100 - m_nTranslateEdit), m_nRow);
		UpdateData(FALSE);
	}


}


void CSeTranslateDlg::OnEnKillfocusEdit1()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	if (m_nTranslateEdit > 100)
		m_nTranslateEdit = 100;
	m_nTranValue = m_nTranslate = m_nTranslateEdit;
	m_pParent->SendMessage(WM_TRANSLATE, (WPARAM)(100 - m_nTranValue), m_nRow);
	UpdateData(FALSE);
}
