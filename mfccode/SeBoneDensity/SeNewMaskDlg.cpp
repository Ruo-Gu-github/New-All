// SeNewMaskDlg.cpp : 实现文件

#include "stdafx.h"
#include "SeNewMaskDlg.h"
#include "afxdialogex.h"


// CSeNewMaskDlg 对话框

IMPLEMENT_DYNAMIC(CSeNewMaskDlg, CDialogEx)

	CSeNewMaskDlg::CSeNewMaskDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSeNewMaskDlg::IDD, pParent)
{
	m_pParent = pParent;
	m_nMin = 0;
	m_nMax = 0;
}

CSeNewMaskDlg::CSeNewMaskDlg(LONG* pHistogram, LONG lMaxNumber, int nMax, int nMin, int nMaxPos, int nMinPos, CWnd* pParent /*= NULL*/)
	: CDialogEx(CSeNewMaskDlg::IDD, pParent)
{
	m_pParent = pParent;
	m_nMin = nMin;
	m_nMax = nMax;
	m_nMinPos = nMinPos;
	m_nMaxPos = nMaxPos;
	m_pHistogram = pHistogram;
	m_lMaxNumber = lMaxNumber;
}

CSeNewMaskDlg::~CSeNewMaskDlg()
{
}

void CSeNewMaskDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SLIDER_MIN_VALUE, m_SliderMin);
	DDX_Control(pDX, IDC_SLIDER_MAX_VALUE, m_SliderMax);
}


BEGIN_MESSAGE_MAP(CSeNewMaskDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CSeNewMaskDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CSeNewMaskDlg::OnBnClickedCancel)
	ON_WM_HSCROLL()
	ON_WM_PAINT()
	ON_EN_CHANGE(IDC_EDIT_MIN_VALUE, &CSeNewMaskDlg::OnEnChangeEditMinValue)
	ON_EN_CHANGE(IDC_EDIT_MAX_VALUE, &CSeNewMaskDlg::OnEnChangeEditMaxValue)
END_MESSAGE_MAP()


// CSeNewMaskDlg 消息处理程序


void CSeNewMaskDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	//m_pParent->SendMessage(WM_MASK_ITEM, m_nMin, m_nMax);
	m_nMinPos = m_SliderMin.GetPos();
	m_nMaxPos = m_SliderMax.GetPos();
	CDialogEx::OnOK();
}


void CSeNewMaskDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	//m_pParent->SendMessage(WM_MASK_ITEM, 0, 0);
	CDialogEx::OnCancel();
}


BOOL CSeNewMaskDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	m_SliderMin.SetRange(m_nMin, m_nMax);
	m_SliderMax.SetRange(m_nMin, m_nMax);
	m_SliderMin.SetPos(m_nMinPos);
	m_SliderMax.SetPos(m_nMaxPos);
	CString str;
	str.Format("%d", m_nMinPos);
	GetDlgItem(IDC_EDIT_MIN_VALUE)->SetWindowText(str);
	str.Format("%d", m_nMaxPos);
	GetDlgItem(IDC_EDIT_MAX_VALUE)->SetWindowText(str);
	m_pParent->SendMessage(WM_MASK, m_nMinPos, m_nMaxPos);
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}





void CSeNewMaskDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (pScrollBar->GetDlgCtrlID() == IDC_SLIDER_MIN_VALUE)
	{
		int nPos = m_SliderMin.GetPos() < m_SliderMax.GetPos() ? m_SliderMin.GetPos() : m_SliderMax.GetPos() - 1;
		m_SliderMin.SetPos(nPos);
		CString str;
		str.Format("%d", nPos);
		GetDlgItem(IDC_EDIT_MIN_VALUE)->SetWindowText(str);
	}
	if (pScrollBar->GetDlgCtrlID() == IDC_SLIDER_MAX_VALUE)
	{
		int nPos = m_SliderMax.GetPos() > m_SliderMin.GetPos() ? m_SliderMax.GetPos() : m_SliderMin.GetPos() + 1;
		m_SliderMax.SetPos(nPos);
		CString str;
		str.Format("%d", nPos);
		GetDlgItem(IDC_EDIT_MAX_VALUE)->SetWindowText(str);
	}
	WPARAM nMinPos = m_SliderMin.GetPos();
	LPARAM nMaxPos = m_SliderMax.GetPos();

	m_pParent->SendMessage(WM_MASK, nMinPos, nMaxPos);
	Invalidate(FALSE);
	CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}


void CSeNewMaskDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: 在此处添加消息处理程序代码
	// 不为绘图消息调用 CDialogEx::OnPaint()
	CRect rect;
	GetClientRect(&rect);
	rect.DeflateRect(15, 15, 15, 120);
	CZKMemDC memDC(&dc, rect, RGB(255,255,255));
	Graphics gc(memDC.m_hDC);
	gc.SetSmoothingMode(SmoothingModeHighQuality);

	if (m_pHistogram == NULL)
		return;


	Color crBlack(255, 0, 0, 0);
	Pen penBlack(crBlack, 1.0);
	Color crWhite(255, 255, 255, 255);
	SolidBrush brushWhite(crWhite);
	gc.FillRectangle(&brushWhite, rect.left, rect.top, rect.Width(), rect.Height());
	gc.DrawRectangle(&penBlack, rect.left, rect.top, rect.Width(), rect.Height());
	float dWidth = (float)rect.Width();
	float dHeight = (float)rect.Height();
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
		pPts[i - m_nMin] = PointF(rect.left + (i - m_nMin) * dWidth/dHistogramWidth, rect.bottom - fHvalue * dScale);
	}
	pPts[m_nMax - m_nMin + 1] = PointF(rect.right, rect.bottom);
	pPts[m_nMax - m_nMin + 2] = PointF(rect.left, rect.bottom);
	gc.FillPolygon(&brushGray, pPts, m_nMax - m_nMin + 3);

	int nMinPos = m_SliderMin.GetPos();
	int nMaxPos = m_SliderMax.GetPos();

	gc.FillRectangle(&brushGreen, (REAL)rect.left + (nMinPos - m_nMin) * dWidth/dHistogramWidth, (REAL)rect.top, (REAL)(nMaxPos - nMinPos) * dWidth/dHistogramWidth, (REAL)rect.Height());

	Safe_Delete(pPts);
}

void CSeNewMaskDlg::OnEnChangeEditMinValue()
{	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	numOnly(IDC_EDIT_MIN_VALUE);

	int tmp = GetDlgItemInt(IDC_EDIT_MIN_VALUE);
	m_SliderMin.SetPos(tmp);

	WPARAM nMinPos = m_SliderMin.GetPos();
	LPARAM nMaxPos = m_SliderMax.GetPos();

	m_pParent->SendMessage(WM_MASK, nMinPos, nMaxPos);
	Invalidate(FALSE);
}


void CSeNewMaskDlg::OnEnChangeEditMaxValue()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	numOnly(IDC_EDIT_MAX_VALUE);

	int tmp = GetDlgItemInt(IDC_EDIT_MAX_VALUE);
	m_SliderMax.SetPos(tmp);

	WPARAM nMinPos = m_SliderMin.GetPos();
	LPARAM nMaxPos = m_SliderMax.GetPos();

	m_pParent->SendMessage(WM_MASK, nMinPos, nMaxPos);
	Invalidate(FALSE);
}

void CSeNewMaskDlg::numOnly(int nID)
{
	CEdit* pEdit;
	pEdit = (CEdit*) GetDlgItem(nID);
	CString str;
	pEdit->GetWindowText(str);

	if (str.SpanIncluding("-0123456789") != str)
	{
		pEdit->SetWindowText(str.Left(str.GetLength() - 1));
		int len = pEdit->GetWindowTextLength();
		pEdit->SetSel(len, len, FALSE);
		pEdit->SetFocus();
	}
}