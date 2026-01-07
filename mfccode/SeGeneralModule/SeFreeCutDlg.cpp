// SeFreeCutDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "SeGeneralModule.h"
#include "SeFreeCutDlg.h"
#include "afxdialogex.h"
#include "Resource.h"


// CSeFreeCutDlg 对话框

IMPLEMENT_DYNAMIC(CSeFreeCutDlg, CDialogEx)

CSeFreeCutDlg::CSeFreeCutDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSeFreeCutDlg::IDD, pParent)
	, m_edit_xs(0)
	, m_edit_xe(0)
	, m_edit_ys(0)
	, m_edit_ye(0)
	, m_edit_zs(0)
	, m_edit_ze(0)
{
	m_pParent = pParent;
}

CSeFreeCutDlg::~CSeFreeCutDlg()
{
}

void CSeFreeCutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SLIDER_FC_XS, m_SliderXStart);
	DDX_Control(pDX, IDC_SLIDER_FC_XE, m_SliderXEnd);
	DDX_Control(pDX, IDC_SLIDER_FC_YS, m_SliderYStart);
	DDX_Control(pDX, IDC_SLIDER_FC_YE, m_SliderYEnd);
	DDX_Control(pDX, IDC_SLIDER_FC_ZS, m_SliderZStart);
	DDX_Control(pDX, IDC_SLIDER_FC_ZE, m_SliderZEnd);
	DDX_Text(pDX, IDC_EDIT_XS, m_edit_xs);
	DDV_MinMaxInt(pDX, m_edit_xs, 0, 20000);
	DDX_Text(pDX, IDC_EDIT_XE, m_edit_xe);
	DDV_MinMaxInt(pDX, m_edit_xe, 0, 20000);
	DDX_Text(pDX, IDC_EDIT_YS, m_edit_ys);
	DDV_MinMaxInt(pDX, m_edit_ys, 0, 20000);
	DDX_Text(pDX, IDC_EDIT_YE, m_edit_ye);
	DDV_MinMaxInt(pDX, m_edit_ye, 0, 20000);
	DDX_Text(pDX, IDC_EDIT_ZS, m_edit_zs);
	DDV_MinMaxInt(pDX, m_edit_zs, 0, 20000);
	DDX_Text(pDX, IDC_EDIT_ZE, m_edit_ze);
	DDV_MinMaxInt(pDX, m_edit_ze, 0, 20000);
}


BEGIN_MESSAGE_MAP(CSeFreeCutDlg, CDialogEx)
	ON_WM_HSCROLL()
	ON_WM_CLOSE()
	ON_EN_CHANGE(IDC_EDIT_XS, &CSeFreeCutDlg::OnEnChangeEditXs)
	ON_EN_CHANGE(IDC_EDIT_XE, &CSeFreeCutDlg::OnEnChangeEditXe)
	ON_EN_CHANGE(IDC_EDIT_YS, &CSeFreeCutDlg::OnEnChangeEditYs)
	ON_EN_CHANGE(IDC_EDIT_YE, &CSeFreeCutDlg::OnEnChangeEditYe)
	ON_EN_CHANGE(IDC_EDIT_ZS, &CSeFreeCutDlg::OnEnChangeEditZs)
	ON_EN_CHANGE(IDC_EDIT_ZE, &CSeFreeCutDlg::OnEnChangeEditZe)
END_MESSAGE_MAP()


// CSeFreeCutDlg 消息处理程序


void CSeFreeCutDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (pScrollBar->GetDlgCtrlID() == IDC_SLIDER_FC_XS)
	{
		int nPos = m_SliderXStart.GetPos() < m_SliderXEnd.GetPos() ? m_SliderXStart.GetPos() : m_SliderXEnd.GetPos() - 1;
		m_SliderXStart.SetPos(nPos);
		m_edit_xs = nPos;
	}
	else if (pScrollBar->GetDlgCtrlID() == IDC_SLIDER_FC_XE)
	{
		int nPos = m_SliderXEnd.GetPos() > m_SliderXStart.GetPos() ? m_SliderXEnd.GetPos() : m_SliderXStart.GetPos() + 1;
		m_SliderXEnd.SetPos(nPos);
		m_edit_xe = nPos;
	}
	else if (pScrollBar->GetDlgCtrlID() == IDC_SLIDER_FC_YS)
	{
		int nPos = m_SliderYStart.GetPos() < m_SliderYEnd.GetPos() ? m_SliderYStart.GetPos() : m_SliderYEnd.GetPos() - 1;
		m_SliderYStart.SetPos(nPos);
		m_edit_ys = nPos;
	}
	else if (pScrollBar->GetDlgCtrlID() == IDC_SLIDER_FC_YE)
	{
		int nPos = m_SliderYEnd.GetPos() > m_SliderYStart.GetPos() ? m_SliderYEnd.GetPos() : m_SliderYStart.GetPos() + 1;
		m_SliderYEnd.SetPos(nPos);
		m_edit_ye = nPos;
	}
	else if (pScrollBar->GetDlgCtrlID() == IDC_SLIDER_FC_ZS)
	{
		int nPos = m_SliderZStart.GetPos() < m_SliderZEnd.GetPos() ? m_SliderZStart.GetPos() : m_SliderZEnd.GetPos() - 1;
		m_SliderZStart.SetPos(nPos);
		m_edit_zs = nPos;
	}
	else if (pScrollBar->GetDlgCtrlID() == IDC_SLIDER_FC_ZE)
	{
		int nPos = m_SliderZEnd.GetPos() > m_SliderZStart.GetPos() ? m_SliderZEnd.GetPos() : m_SliderZStart.GetPos() + 1;
		m_SliderZEnd.SetPos(nPos);
		m_edit_ze = nPos;
	}

	int nRangers[6];

	nRangers[0] = m_SliderXStart.GetPos();
	nRangers[1] = m_SliderXEnd.GetPos();
	nRangers[2] = m_SliderYStart.GetPos();
	nRangers[3] = m_SliderYEnd.GetPos();
	nRangers[4] = m_SliderZStart.GetPos();
	nRangers[5] = m_SliderZEnd.GetPos();

	m_pParent->SendMessage(WM_FREECUT, (WPARAM)&nRangers[0], 6);
	Invalidate(FALSE);
	UpdateData(FALSE); 
	CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}


BOOL CSeFreeCutDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	m_SliderXStart.SetRange(0,20000);
	m_SliderYStart.SetRange(0,20000);
	m_SliderZStart.SetRange(0,20000);
	m_SliderXEnd.SetRange(0,20000);
	m_SliderYEnd.SetRange(0,20000);
	m_SliderZEnd.SetRange(0,20000);

	m_SliderXStart.SetPos(5000);
	m_SliderXEnd.SetPos(15000);
	m_SliderYStart.SetPos(5000);
	m_SliderYEnd.SetPos(15000);
	m_SliderZStart.SetPos(5000);
	m_SliderZEnd.SetPos(15000);

	m_edit_xs = 5000;
	m_edit_xe = 15000;
	m_edit_ys = 5000;
	m_edit_ye = 15000;
	m_edit_zs = 5000;
	m_edit_ze = 15000;
	UpdateData(FALSE); 

	int nRangers[6];

	nRangers[0] = m_edit_xs;
	nRangers[1] = m_edit_xe;
	nRangers[2] = m_edit_ys;
	nRangers[3] = m_edit_ye;
	nRangers[4] = m_edit_zs;
	nRangers[5] = m_edit_ze;

	m_pParent->SendMessage(WM_FREECUT, (WPARAM)&nRangers[0], 6);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CSeFreeCutDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_pParent->SendMessage(WM_ClOSE_FCWND);
	CDialogEx::OnClose();
}


void CSeFreeCutDlg::OnEnChangeEditXs()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData(TRUE);
	int nPos = m_edit_xs;
	if (nPos > m_SliderXEnd.GetPos())
		m_SliderXStart.SetPos(m_SliderXEnd.GetPos());
	else
		m_SliderXStart.SetPos(nPos);

	int nRangers[6];

	nRangers[0] = m_SliderXStart.GetPos();
	nRangers[1] = m_SliderXEnd.GetPos();
	nRangers[2] = m_SliderYStart.GetPos();
	nRangers[3] = m_SliderYEnd.GetPos();
	nRangers[4] = m_SliderZStart.GetPos();
	nRangers[5] = m_SliderZEnd.GetPos();

	m_pParent->SendMessage(WM_FREECUT, (WPARAM)&nRangers[0], 6);
	Invalidate(FALSE);

}


void CSeFreeCutDlg::OnEnChangeEditXe()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData(TRUE); 
	int nPos = m_edit_xe;
	m_SliderXEnd.SetPos(nPos);

	if (nPos < m_SliderXStart.GetPos())
		m_SliderXEnd.SetPos(m_SliderXStart.GetPos());
	else
		m_SliderXEnd.SetPos(nPos);

	int nRangers[6];

	nRangers[0] = m_SliderXStart.GetPos();
	nRangers[1] = m_SliderXEnd.GetPos();
	nRangers[2] = m_SliderYStart.GetPos();
	nRangers[3] = m_SliderYEnd.GetPos();
	nRangers[4] = m_SliderZStart.GetPos();
	nRangers[5] = m_SliderZEnd.GetPos();

	m_pParent->SendMessage(WM_FREECUT, (WPARAM)&nRangers[0], 6);
	Invalidate(FALSE);
}


void CSeFreeCutDlg::OnEnChangeEditYs()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData(TRUE);
	int nPos = m_edit_ys;
 
	if (nPos > m_SliderYEnd.GetPos())
		m_SliderYStart.SetPos(m_SliderYEnd.GetPos());
	else
		m_SliderYStart.SetPos(nPos);

	int nRangers[6];

	nRangers[0] = m_SliderXStart.GetPos();
	nRangers[1] = m_SliderXEnd.GetPos();
	nRangers[2] = m_SliderYStart.GetPos();
	nRangers[3] = m_SliderYEnd.GetPos();
	nRangers[4] = m_SliderZStart.GetPos();
	nRangers[5] = m_SliderZEnd.GetPos();

	m_pParent->SendMessage(WM_FREECUT, (WPARAM)&nRangers[0], 6);
	Invalidate(FALSE);
}


void CSeFreeCutDlg::OnEnChangeEditYe()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData(TRUE);
	int nPos = m_edit_ye;

	if (nPos > m_SliderYStart.GetPos())
		m_SliderYEnd.SetPos(m_SliderYStart.GetPos());
	else
		m_SliderYEnd.SetPos(nPos);


	int nRangers[6];

	nRangers[0] = m_SliderXStart.GetPos();
	nRangers[1] = m_SliderXEnd.GetPos();
	nRangers[2] = m_SliderYStart.GetPos();
	nRangers[3] = m_SliderYEnd.GetPos();
	nRangers[4] = m_SliderZStart.GetPos();
	nRangers[5] = m_SliderZEnd.GetPos();

	m_pParent->SendMessage(WM_FREECUT, (WPARAM)&nRangers[0], 6);
	Invalidate(FALSE);
}


void CSeFreeCutDlg::OnEnChangeEditZs()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData(TRUE);
	int nPos = m_edit_zs;


	if (nPos > m_SliderZEnd.GetPos())
		m_SliderZStart.SetPos(m_SliderZEnd.GetPos());
	else
		m_SliderZStart.SetPos(nPos);

	int nRangers[6];

	nRangers[0] = m_SliderXStart.GetPos();
	nRangers[1] = m_SliderXEnd.GetPos();
	nRangers[2] = m_SliderYStart.GetPos();
	nRangers[3] = m_SliderYEnd.GetPos();
	nRangers[4] = m_SliderZStart.GetPos();
	nRangers[5] = m_SliderZEnd.GetPos();

	m_pParent->SendMessage(WM_FREECUT, (WPARAM)&nRangers[0], 6);
	Invalidate(FALSE);
}


void CSeFreeCutDlg::OnEnChangeEditZe()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData(TRUE);
	int nPos = m_edit_ze;

	if (nPos < m_SliderZStart.GetPos())
		m_SliderZEnd.SetPos(m_SliderZStart.GetPos());
	else
		m_SliderZEnd.SetPos(nPos);

	int nRangers[6];

	nRangers[0] = m_SliderXStart.GetPos();
	nRangers[1] = m_SliderXEnd.GetPos();
	nRangers[2] = m_SliderYStart.GetPos();
	nRangers[3] = m_SliderYEnd.GetPos();
	nRangers[4] = m_SliderZStart.GetPos();
	nRangers[5] = m_SliderZEnd.GetPos();

	m_pParent->SendMessage(WM_FREECUT, (WPARAM)&nRangers[0], 6);
	Invalidate(FALSE);
}
