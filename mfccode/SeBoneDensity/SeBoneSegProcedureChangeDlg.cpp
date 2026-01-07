// CSeBoneSegProcedureChangeDlg.cpp : 实现文件

#include "stdafx.h"
#include "SeBoneSegProcedureChangeDlg.h"
#include "afxdialogex.h"


// CSeNewMaskDlg 对话框

IMPLEMENT_DYNAMIC(CSeBoneSegProcedureChangeDlg, CDialogEx)
	CSeBoneSegProcedureChangeDlg::CSeBoneSegProcedureChangeDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSeBoneSegProcedureChangeDlg::IDD, pParent)
{
	m_pParent = pParent;
}

CSeBoneSegProcedureChangeDlg::CSeBoneSegProcedureChangeDlg(map<CString, int>* pmapFunc, CString* pFuncSingleChange, CWnd* pParent/*= NULL*/)
	: CDialogEx(CSeBoneSegProcedureChangeDlg::IDD, pParent)
{
	m_pParent = pParent;
	m_pmapFunc = pmapFunc;
	m_pstrSingleFuncChange = pFuncSingleChange;
}

CSeBoneSegProcedureChangeDlg::~CSeBoneSegProcedureChangeDlg()
{
}

void CSeBoneSegProcedureChangeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_FUNCNAME, m_cbFuncName);
	DDX_Control(pDX, IDC_COMBO_KERNELTYPE, m_cbKernelType);
	DDX_Control(pDX, IDC_EDIT_KERNELSIZE, m_edKernelSize);
	DDX_Control(pDX, IDC_COMBO_COLOR, m_cbColor);
}

BEGIN_MESSAGE_MAP(CSeBoneSegProcedureChangeDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CSeBoneSegProcedureChangeDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CSeBoneSegProcedureChangeDlg::OnBnClickedCancel)
	ON_CBN_SELCHANGE(IDC_COMBO_FUNCNAME, &CSeBoneSegProcedureChangeDlg::OnSelchangeComboFuncname)
END_MESSAGE_MAP()

// CSeBoneSegProcedureChangeDlg 消息处理程序

BOOL CSeBoneSegProcedureChangeDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	m_cbColor.InsertString(0,"red");
	m_cbColor.InsertString(1,"green");
	m_cbColor.SetCurSel(m_pmapFunc->at(m_pstrSingleFuncChange[0]));

	m_cbFuncName.InsertString(0,"reverse");
	m_cbFuncName.InsertString(1,"gethole");
	m_cbFuncName.InsertString(2,"fillhole");
	m_cbFuncName.InsertString(3,"corrosion");
	m_cbFuncName.InsertString(4,"dilate");
	m_cbFuncName.InsertString(5,"open");
	m_cbFuncName.InsertString(6,"close");
	m_cbFuncName.SetCurSel(m_pmapFunc->at(m_pstrSingleFuncChange[1]));

	m_cbKernelType.InsertString(0,"circle");
	m_cbKernelType.InsertString(1,"square");
	m_cbKernelType.InsertString(2,"none");
	m_cbKernelType.SetCurSel(m_pmapFunc->at(m_pstrSingleFuncChange[2]));

	m_edKernelSize.SetWindowTextA(m_pstrSingleFuncChange[3]);


	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CSeBoneSegProcedureChangeDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strColor;
	CString strSelFuncName;
	CString strSelKernelType;
	CString strSelKernelSize;
	m_cbColor.GetLBText(m_cbColor.GetCurSel(),strColor);
	m_cbFuncName.GetLBText(m_cbFuncName.GetCurSel(),strSelFuncName);
	m_cbKernelType.GetLBText(m_cbKernelType.GetCurSel(),strSelKernelType);
	m_edKernelSize.GetWindowTextA(strSelKernelSize);


	if (m_cbFuncName.GetCurSel() == 0 || m_cbFuncName.GetCurSel() == 1 || m_cbFuncName.GetCurSel() == 2)
	{
		if (strSelKernelSize!="")
		{
			return;
		}
		if (m_cbKernelType.GetCurSel() != 2)
		{
			return;
		}
	}
	else{
		if (strSelKernelSize=="")
		{
			return;
		}
		if (m_cbKernelType.GetCurSel() == 2)
		{
			return;
		}
	}
	m_pstrSingleFuncChange[0] = strColor;
	m_pstrSingleFuncChange[1] = strSelFuncName;
	m_pstrSingleFuncChange[2] = strSelKernelType;
	m_pstrSingleFuncChange[3] = strSelKernelSize;

	m_pParent->SendMessage(WM_BONE_CALCHANGE,1,1);
	CDialogEx::OnOK();
	return;
}


void CSeBoneSegProcedureChangeDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnCancel();
}

void CSeBoneSegProcedureChangeDlg::OnSelchangeComboFuncname()
{
	// TODO: 在此添加控件通知处理程序代码
	int nselcur = m_cbFuncName.GetCurSel();
	if (nselcur == 0 || nselcur == 1 || nselcur == 2)
	{
		//后面两个没有
		m_cbKernelType.SetCurSel(2);
		m_edKernelSize.SetWindowTextA("");
		//m_bChangeLastTwo = 0;
	}
	else
	{
		CString csKernelType;
		m_cbKernelType.GetLBText(m_cbKernelType.GetCurSel(),csKernelType);
		if (csKernelType == "none")
		{
			m_cbKernelType.SetCurSel(0);
			m_edKernelSize.SetWindowTextA("9");
		}
		//m_bChangeLastTwo = 1;
	}
}
