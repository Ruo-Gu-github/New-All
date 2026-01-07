// SetDensityParam.cpp : 实现文件
//

#include "stdafx.h"
#include "SeBoneDensity.h"
#include "SeSetDensityParamDlg.h"
#include "afxdialogex.h"

// 添加MessageBoxTimeout支持
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

// CSeSetDensityParamDlg 对话框

IMPLEMENT_DYNAMIC(CSeSetDensityParamDlg, CDialogEx)

CSeSetDensityParamDlg::CSeSetDensityParamDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSeSetDensityParamDlg::IDD, pParent)
{
	m_nSmallValue = 0;
	m_nBigValue = 0;
	m_fSmallDensity = 0.0;
	m_fBigDensity = 0.0;
}

CSeSetDensityParamDlg::CSeSetDensityParamDlg(int nSmall, int nBig, float fSamllDensity, float fBigDensity, CWnd* pParent /*= NULL*/)
	: CDialogEx(CSeSetDensityParamDlg::IDD, pParent)
{
	m_nSmallValue = 0;
	m_nBigValue = 0;
	m_fSmallDensity = 0.0;
	m_fBigDensity = 0.0;
// 	SetDlgItemText(IDC_EDIT_LOW_VALUE, CSeToolKit::num2CStr(nSmall));
// 	SetDlgItemText(IDC_EDIT_HIGH_VALUE, CSeToolKit::num2CStr(nBig));
// 	SetDlgItemText(IDC_EDIT_LOW_DENSITY, CSeToolKit::float2CStr(fSamllDensity));
// 	SetDlgItemText(IDC_EDIT_HIGH_DENSITY, CSeToolKit::float2CStr(fBigDensity));
}

CSeSetDensityParamDlg::~CSeSetDensityParamDlg()
{
}

void CSeSetDensityParamDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSeSetDensityParamDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CSeSetDensityParamDlg::OnBnClickedOk)
	ON_EN_CHANGE(IDC_EDIT_LOW_VALUE, &CSeSetDensityParamDlg::OnEnChangeEditLowValue)
	ON_EN_CHANGE(IDC_EDIT_LOW_DENSITY, &CSeSetDensityParamDlg::OnEnChangeEditLowDensity)
	ON_EN_CHANGE(IDC_EDIT_HIGH_VALUE, &CSeSetDensityParamDlg::OnEnChangeEditHighValue)
	ON_EN_CHANGE(IDC_EDIT_HIGH_DENSITY, &CSeSetDensityParamDlg::OnEnChangeEditHighDensity)
END_MESSAGE_MAP()


// CSeSetDensityParamDlg 消息处理程序


void CSeSetDensityParamDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CString str0, str1, str2, str3;
	GetDlgItemText(IDC_EDIT_LOW_VALUE, str0);
	GetDlgItemText(IDC_EDIT_HIGH_VALUE, str1);
	GetDlgItemText(IDC_EDIT_LOW_DENSITY, str2);
	GetDlgItemText(IDC_EDIT_HIGH_DENSITY, str3);
	if (str0.GetLength() == 0 ||
		str1.GetLength() == 0 ||
		str2.GetLength() == 0 ||
		str3.GetLength() == 0
		)
	{
		MessageBoxTimeout(NULL, "      请填写所有空白的编辑框！     ", "提示", MB_ICONINFORMATION, 0, 300);
		return;
	}
	m_nSmallValue = CSeToolKit::CStr2num(str0);
	m_nBigValue	  =	CSeToolKit::CStr2num(str1);
	m_fSmallDensity = CSeToolKit::CStr2Float(str2);
	m_fBigDensity   = CSeToolKit::CStr2Float(str3);
	CDialogEx::OnOK();
}


void CSeSetDensityParamDlg::OnEnChangeEditLowValue()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	numOnly(IDC_EDIT_LOW_VALUE);
}


void CSeSetDensityParamDlg::OnEnChangeEditLowDensity()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	numOnly(IDC_EDIT_LOW_DENSITY);
}


void CSeSetDensityParamDlg::OnEnChangeEditHighValue()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	numOnly(IDC_EDIT_HIGH_VALUE);
}


void CSeSetDensityParamDlg::OnEnChangeEditHighDensity()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	numOnly(IDC_EDIT_HIGH_DENSITY);
}

void CSeSetDensityParamDlg::numOnly(int nID)
{
	CEdit* pEdit;
	pEdit = (CEdit*) GetDlgItem(nID);
	CString str;
	pEdit->GetWindowText(str);

	if (str.SpanIncluding("-0123456789.") != str)
	{
		pEdit->SetWindowText(str.Left(str.GetLength() - 1));
		int len = pEdit->GetWindowTextLength();
		pEdit->SetSel(len, len, FALSE);
		pEdit->SetFocus();
	}
}

void CSeSetDensityParamDlg::SetInfo(int nSmall, int nBig, float fSmallDensity, float fBigDensity)
{
// 	CEdit* pEdit;
// 	pEdit = (CEdit*)GetDlgItem(IDC_EDIT_LOW_VALUE);
// 	pEdit->SetWindowText(CSeToolKit::num2CStr(nSmall));
// 	pEdit = (CEdit*)GetDlgItem(IDC_EDIT_HIGH_VALUE);
// 	pEdit->SetWindowText(CSeToolKit::num2CStr(nBig));
// 	pEdit = (CEdit*)GetDlgItem(IDC_EDIT_LOW_DENSITY);
// 	pEdit->SetWindowText(CSeToolKit::float2CStr(fSmallDensity));
// 	pEdit = (CEdit*)GetDlgItem(IDC_EDIT_HIGH_DENSITY);
// 	pEdit->SetWindowText(CSeToolKit::float2CStr(fBigDensity));

// 	SetDlgItemText(IDC_EDIT_LOW_VALUE, CSeToolKit::num2CStr(nSmall));
// 	SetDlgItemText(IDC_EDIT_HIGH_VALUE, CSeToolKit::num2CStr(nBig));
// 	SetDlgItemText(IDC_EDIT_LOW_DENSITY, CSeToolKit::float2CStr(fSmallDensity));
// 	SetDlgItemText(IDC_EDIT_HIGH_DENSITY, CSeToolKit::float2CStr(fBigDensity));

	m_nSmallValue = nSmall;
	m_nBigValue = nBig;
	m_fSmallDensity = fSmallDensity;
	m_fBigDensity = fBigDensity;
}


BOOL CSeSetDensityParamDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	if (m_nSmallValue !=0 || m_nBigValue != 0 || m_fSmallDensity > 0.0 || m_fBigDensity > 0.0)
	{
		SetDlgItemText(IDC_EDIT_LOW_VALUE, CSeToolKit::num2CStr(m_nSmallValue));
		SetDlgItemText(IDC_EDIT_HIGH_VALUE, CSeToolKit::num2CStr(m_nBigValue));
		SetDlgItemText(IDC_EDIT_LOW_DENSITY, CSeToolKit::float2CStr(m_fSmallDensity));
		SetDlgItemText(IDC_EDIT_HIGH_DENSITY, CSeToolKit::float2CStr(m_fBigDensity));
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}
