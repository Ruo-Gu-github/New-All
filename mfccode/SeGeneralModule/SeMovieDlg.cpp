// SeMovieDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "SeGeneralModule.h"
#include "SeMovieDlg.h"
#include "afxdialogex.h"


// CSeMovieDlg 对话框

IMPLEMENT_DYNAMIC(CSeMovieDlg, CDialogEx)

CSeMovieDlg::CSeMovieDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSeMovieDlg::IDD, pParent)
	, m_nRTime(10)
	, m_dSStart(1.0)
	, m_dSEnd(2.0)
	, m_nSTime(2)
	, m_nCXStart(5000)
	, m_nCXEnd(10000)
	, m_nCTime(5)
	, m_nCYStart(5000)
	, m_nCYEnd(5000)
	, m_nCZStart(5000)
	, m_nCZEnd(5000)
	, m_nRXStart(0)
	, m_nRXEnd(360)
	, m_nRYStart(0)
	, m_nRYEnd(0)
	, m_nRZStart(0)
	, m_nRZEnd(0) 
	, m_nRunPos(0)
{
	m_pParentWnd = pParent;
}

CSeMovieDlg::~CSeMovieDlg()
{
}

void CSeMovieDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_ACTION, m_listctlAction);
	DDX_Text(pDX, IDC_EDIT_R_TIME, m_nRTime);
	DDV_MinMaxInt(pDX, m_nRTime, 1, 100);
	DDX_Text(pDX, IDC_EDIT_S_START, m_dSStart);
	DDV_MinMaxDouble(pDX, m_dSStart, 0.1, 5.0);
	DDX_Text(pDX, IDC_EDIT_S_END, m_dSEnd);
	DDV_MinMaxDouble(pDX, m_dSEnd, 0.1, 5.0);
	DDX_Text(pDX, IDC_EDIT_S_TIME, m_nSTime);
	DDV_MinMaxInt(pDX, m_nSTime, 1, 100);
	DDX_Text(pDX, IDC_EDIT_C_X_START, m_nCXStart);
	DDV_MinMaxInt(pDX, m_nCXStart, 0, 20000);
	DDX_Text(pDX, IDC_EDIT_C_X_END, m_nCXEnd);
	DDV_MinMaxInt(pDX, m_nCXEnd, 0, 20000);
	DDX_Text(pDX, IDC_EDIT_C_TIME, m_nCTime);
	DDV_MinMaxInt(pDX, m_nCTime, 1, 100);
	DDX_Text(pDX, IDC_EDIT_C_Y_START, m_nCYStart);
	DDV_MinMaxInt(pDX, m_nCYStart, 0, 20000);
	DDX_Text(pDX, IDC_EDIT_C_Y_END, m_nCYEnd);
	DDV_MinMaxInt(pDX, m_nCYEnd, 0, 20000);
	DDX_Text(pDX, IDC_EDIT_C_Z_START, m_nCZStart);
	DDV_MinMaxInt(pDX, m_nCZStart, 0, 20000);
	DDX_Text(pDX, IDC_EDIT_C_Z_END, m_nCZEnd);
	DDV_MinMaxInt(pDX, m_nCZEnd, 0, 20000);
	DDX_Text(pDX, IDC_EDIT_R_X_START, m_nRXStart);
	DDV_MinMaxInt(pDX, m_nRXStart, 0, 1080);
	DDX_Text(pDX, IDC_EDIT_R_X_END, m_nRXEnd);
	DDV_MinMaxInt(pDX, m_nRXEnd, 0, 1080);
	DDX_Text(pDX, IDC_EDIT_R_Y_START, m_nRYStart);
	DDV_MinMaxInt(pDX, m_nRYStart, 0, 1080);
	DDX_Text(pDX, IDC_EDIT_R_Y_END, m_nRYEnd);
	DDV_MinMaxInt(pDX, m_nRYEnd, 0, 1080);
	DDX_Text(pDX, IDC_EDIT_R_Z_START, m_nRZStart);
	DDV_MinMaxInt(pDX, m_nRZStart, 0, 1080);
	DDX_Text(pDX, IDC_EDIT_R_Z_END, m_nRZEnd);
	DDV_MinMaxInt(pDX, m_nRZEnd, 0, 1080);
}


BEGIN_MESSAGE_MAP(CSeMovieDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_R_ADD, &CSeMovieDlg::OnBnClickedButtonRAdd)
	ON_BN_CLICKED(IDC_BUTTON_S_ADD, &CSeMovieDlg::OnBnClickedButtonSAdd)
	ON_BN_CLICKED(IDC_BUTTON_C_ADD, &CSeMovieDlg::OnBnClickedButtonCAdd)
	ON_BN_CLICKED(IDC_BUTTON_D_ONESTEP, &CSeMovieDlg::OnBnClickedButtonDOnestep)
	ON_BN_CLICKED(IDC_BUTTON_D_RUN, &CSeMovieDlg::OnBnClickedButtonDRun)
	ON_BN_CLICKED(IDC_BUTTON_RECORD, &CSeMovieDlg::OnBnClickedButtonRecord)
	ON_BN_CLICKED(IDOK, &CSeMovieDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CSeMovieDlg::OnBnClickedCancel)
END_MESSAGE_MAP()


// CSeMovieDlg 消息处理程序
BOOL CSeMovieDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	DWORD dwStyle = m_listctlAction.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;// 选中某行使整行高亮（只适用与 report 风格的 listctrl）
	dwStyle |= LVS_EX_CHECKBOXES;//item 前生成 checkbox 控件
	m_listctlAction.SetExtendedStyle(dwStyle); // 设置扩展风格

	m_listctlAction.InsertColumn(0, "动作", LVCFMT_LEFT, 60);// 插入列
	m_listctlAction.InsertColumn(1, "起始", LVCFMT_LEFT, 200);
	m_listctlAction.InsertColumn(2, "结束", LVCFMT_LEFT, 200);
	m_listctlAction.InsertColumn(3, "用时", LVCFMT_LEFT, 60);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CSeMovieDlg::OnBnClickedButtonRAdd()
{
	m_nRunPos = 0;
	UpdateData(TRUE);
	int nXStart = m_nRXStart;
	int nXEnd = m_nRXEnd;
	int nYStart = m_nRYStart;
	int nYEnd = m_nRYEnd;
	int nZStart = m_nRZStart;
	int nZEnd = m_nRZEnd;
	int nTime = m_nRTime;

	int nRow = m_listctlAction.InsertItem(m_listctlAction.GetItemCount(), " ");// 插入行
	m_listctlAction.SetItemText(nRow, 0, "旋转");// 设置数据
	CString str;
	str.Format("X:%d Y:%d Z:%d", nXStart, nYStart, nZStart);
	m_listctlAction.SetItemText(nRow, 1, str);// 设置数据
	str.Format("X:%d Y:%d Z:%d", nXEnd, nYEnd, nZEnd);
	m_listctlAction.SetItemText(nRow, 2, str);// 设置数据
	str.Format("%d", nTime);
	m_listctlAction.SetItemText(nRow, 3, str);// 设置数据
	m_listctlAction.SetCheck(nRow, TRUE);
	m_listctlAction.SetItemState(nRow, LVNI_FOCUSED | LVIS_SELECTED, LVNI_FOCUSED | LVIS_SELECTED);
}


void CSeMovieDlg::OnBnClickedButtonSAdd()
{
	m_nRunPos = 0;
	UpdateData(TRUE);
	// TODO: 在此添加控件通知处理程序代码
	double dStart = m_dSStart;
	double dEnd = m_dSEnd;
	int nTime = m_nSTime;

	int nRow = m_listctlAction.InsertItem(m_listctlAction.GetItemCount(), " ");// 插入行
	m_listctlAction.SetItemText(nRow, 0, "缩放");// 设置数据
	CString str;
	str.Format("%.2f", dStart);
	m_listctlAction.SetItemText(nRow, 1, str);// 设置数据
	str.Format("%.2f", dEnd);
	m_listctlAction.SetItemText(nRow, 2, str);// 设置数据
	str.Format("%d", nTime);
	m_listctlAction.SetItemText(nRow, 3, str);// 设置数据
	m_listctlAction.SetCheck(nRow, TRUE);
	m_listctlAction.SetItemState(nRow, LVNI_FOCUSED | LVIS_SELECTED, LVNI_FOCUSED | LVIS_SELECTED);
}


void CSeMovieDlg::OnBnClickedButtonCAdd()
{
	m_nRunPos = 0;
	UpdateData(TRUE);
	// TODO: 在此添加控件通知处理程序代码
	int nXStart = m_nCXStart;
	int nXEnd = m_nCXEnd;
	int nYStart = m_nCYStart;
	int nYEnd = m_nCYEnd;
	int nZStart = m_nCZStart;
	int nZEnd = m_nCZEnd;
	int nTime = m_nCTime;

	int nRow = m_listctlAction.InsertItem(m_listctlAction.GetItemCount(), " ");// 插入行
	m_listctlAction.SetItemText(nRow, 0, "裁切");// 设置数据
	CString str;
	str.Format("X:%d Y:%d Z:%d", nXStart, nYStart, nZStart);
	m_listctlAction.SetItemText(nRow, 1, str);// 设置数据
	str.Format("X:%d Y:%d Z:%d", nXEnd, nYEnd, nZEnd);
	m_listctlAction.SetItemText(nRow, 2, str);// 设置数据
	str.Format("%d", nTime);
	m_listctlAction.SetItemText(nRow, 3, str);// 设置数据
	m_listctlAction.SetCheck(nRow, TRUE);
	m_listctlAction.SetItemState(nRow, LVNI_FOCUSED | LVIS_SELECTED, LVNI_FOCUSED | LVIS_SELECTED);
}


void CSeMovieDlg::OnBnClickedButtonDOnestep()
{
	// TODO: 在此添加控件通知处理程序代码
	m_listctlAction.SetItemState(-1, 0, LVIS_SELECTED);

	int nTotal = m_listctlAction.GetItemCount();
	int nStart = m_nRunPos;
	if (nTotal <= nStart) return;

	CString strSel, strNext;
	for (int i = nStart; i < nTotal; i++)
	{
		BOOL checked = m_listctlAction.GetCheck(i);
		if (!checked)
		{
			m_nRunPos++;
			continue;
		}
		m_listctlAction.SetItemState(i, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		CString action = m_listctlAction.GetItemText(i, 0);
		CString start = m_listctlAction.GetItemText(i, 1);
		CString end = m_listctlAction.GetItemText(i, 2);
		CString during = m_listctlAction.GetItemText(i, 3);
		ParseAction(action, start, end, during);
		m_nRunPos++;
		break;
	}
}


void CSeMovieDlg::OnBnClickedButtonDRun()
{
	// TODO: 在此添加控件通知处理程序代码
	m_nRunPos = 0;
	CString strSel, strNext;
	for (int i = 0; i < m_listctlAction.GetItemCount(); i++)
	{
		BOOL checked = m_listctlAction.GetCheck(i);
		if (!checked) continue;
		CString action = m_listctlAction.GetItemText(i, 0);
		CString start = m_listctlAction.GetItemText(i, 1);
		CString end = m_listctlAction.GetItemText(i, 2);
		CString during = m_listctlAction.GetItemText(i, 3);
		ParseAction(action, start, end, during);
	}
}


void CSeMovieDlg::OnBnClickedButtonRecord()
{
	// TODO: 在此添加控件通知处理程序代码
	m_nRunPos = 0;
	ShowWindow(SW_HIDE);
	CString strSel, strNext;
	for (int i = 0; i < m_listctlAction.GetItemCount(); i++)
	{
		BOOL checked = m_listctlAction.GetCheck(i);
		if (!checked) continue;
		DWORD_PTR data = m_listctlAction.GetItemData(i);
		CString action = m_listctlAction.GetItemText(i, 0);
		CString start = m_listctlAction.GetItemText(i, 1);
		CString end = m_listctlAction.GetItemText(i, 2);
		CString during = m_listctlAction.GetItemText(i, 3);
		ParseAction(action, start, end, during);
	}
	ShowWindow(SW_SHOW);
}


void CSeMovieDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
}


void CSeMovieDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnCancel();
}





BOOL CSeMovieDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 在此添加专用代码和/或调用基类
	// 删除 上移 下移 消息
	if(GetDlgItem(IDC_LIST_ACTION) != GetFocus()) return CDialogEx::PreTranslateMessage(pMsg);
	if (pMsg->message == WM_KEYDOWN)
	{
		if(pMsg->wParam == VK_DELETE)
		{
			ChangeColumns("delete");
		}
		else if(pMsg->wParam == VK_UP)
		{
			ChangeColumns("moveup");
		} 
		else if(pMsg->wParam == VK_DOWN)
		{
			ChangeColumns("movedown");
		}
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}

void CSeMovieDlg::ChangeColumns(CString type)
{
	m_nRunPos = 0;
	if (type == "delete")
	{
		DeleteColumns();
	} 
	else if (type == "moveup")
	{
		MoveUpColumns();
	}
	else if (type == "movedown")
	{
		MoveDownColumns();
	}
}

void CSeMovieDlg::MoveUpColumns()
{
	//UINT i;
	//UINT uSelectedCount = m_listctlAction.GetSelectedCount();
	//int  nItem;
	//if (uSelectedCount > 0)
	//{
	//	for (i=0; i < uSelectedCount; i++)
	//	{   
	//		nItem = m_listctlAction.GetNextItem(-1, LVNI_SELECTED);
	//		ASSERT(nItem != -1);
	//		m_listctlAction.DeleteItem(nItem); 
	//	}
	//}
	BOOL bUp = TRUE;
	if (1 != m_listctlAction.GetSelectedCount())
		return;
	int sel = m_listctlAction.GetNextItem(-1, LVNI_SELECTED); 
	// Move up or down
	int next = bUp ? sel - 1 : sel + 1;
	if (next < 0 || next >= m_listctlAction.GetItemCount())
		return;
	CString strSel, strNext;
	for (int i = 0; i < m_listctlAction.GetHeaderCtrl()->GetItemCount(); i++)
	{
		strSel = m_listctlAction.GetItemText(sel, i);
		strNext = m_listctlAction.GetItemText(next, i);
		m_listctlAction.SetItemText(sel, i, strNext);
		m_listctlAction.SetItemText(next, i, strSel);
	}
	// Move selection
	m_listctlAction.SetItemState(sel, ~LVNI_SELECTED, LVIS_SELECTED);
	m_listctlAction.SetItemState(next, LVNI_SELECTED, LVIS_SELECTED);
	m_listctlAction.SetSelectionMark(next);
}

void CSeMovieDlg::MoveDownColumns()
{
	//UINT i;
	//UINT uSelectedCount = m_listctlAction.GetSelectedCount();
	//int  nItem;
	//if (uSelectedCount > 0)
	//{
	//	for (i=0; i < uSelectedCount; i++)
	//	{   
	//		nItem = m_listctlAction.GetNextItem(-1, LVNI_SELECTED);
	//		ASSERT(nItem != -1);
	//		m_listctlAction.DeleteItem(nItem); 
	//	}
	//}
	BOOL bUp = FALSE;
	if (1 != m_listctlAction.GetSelectedCount())
		return;
	int sel = m_listctlAction.GetNextItem(-1, LVNI_SELECTED); 
	// Move up or down
	int next = bUp ? sel - 1 : sel + 1;
	if (next < 0 || next >= m_listctlAction.GetItemCount())
		return;
	CString strSel, strNext;
	for (int i = 0; i < m_listctlAction.GetHeaderCtrl()->GetItemCount(); i++)
	{
		strSel = m_listctlAction.GetItemText(sel, i);
		strNext = m_listctlAction.GetItemText(next, i);
		m_listctlAction.SetItemText(sel, i, strNext);
		m_listctlAction.SetItemText(next, i, strSel);
	}
	// Move selection
	m_listctlAction.SetItemState(sel, ~LVNI_SELECTED, LVIS_SELECTED);
	m_listctlAction.SetItemState(next, LVNI_SELECTED, LVIS_SELECTED);
	m_listctlAction.SetSelectionMark(next);
}

void CSeMovieDlg::DeleteColumns()
{
	UINT i;
	UINT uSelectedCount = m_listctlAction.GetSelectedCount();
	int  nItem;
	if (uSelectedCount > 0)
	{
		for (i=0; i < uSelectedCount; i++)
		{   
			nItem = m_listctlAction.GetNextItem(-1, LVNI_SELECTED);
			ASSERT(nItem != -1);
			m_listctlAction.DeleteItem(nItem); 
		}
	}
}

void CSeMovieDlg::ParseAction(CString type, CString start, CString end, CString during)
{
	if(type == "旋转")
	{
		ParseRotate(start, end, during);
	}
	else if (type == "缩放")
	{
		ParseScale(start, end, during);
	}
	else if (type == "裁切")
	{
		ParseCut(start, end, during);
	}
}

void CSeMovieDlg::ParseRotate(CString start, CString end, CString during)
{
	int pos[6];
	int nTokenPos = 0;
	
	for(int i=0; i<3; i++)
	{
		CString strToken = start.Tokenize(" ", nTokenPos);
		pos[i] = atoi(strToken.Mid(2));
	}
	nTokenPos = 0;
	for(int i=3; i<6; i++)
	{
		CString strToken = end.Tokenize(" ", nTokenPos);
		pos[i] = atoi(strToken.Mid(2));
	}
	int nDuring = atoi(during);
	m_pParentWnd->SendMessage(WM_ROTATE_ACTION, (WPARAM)&pos[0], nDuring);
}

void CSeMovieDlg::ParseScale(CString start, CString end, CString during)
{
	double pos[2];
	pos[0] = atof(start);
	pos[1] = atof(end);
	int nDuring = atoi(during);
	m_pParentWnd->SendMessage(WM_SCALE_ACTION, (WPARAM)&pos[0], nDuring);
}

void CSeMovieDlg::ParseCut(CString start, CString end, CString during)
{
	int pos[6];
	int nTokenPos = 0;

	for(int i=0; i<3; i++)
	{
		CString strToken = start.Tokenize(" ", nTokenPos);
		pos[i] = atoi(strToken.Mid(2));
	}
	nTokenPos = 0;
	for(int i=3; i<6; i++)
	{
		CString strToken = end.Tokenize(" ", nTokenPos);
		pos[i] = atoi(strToken.Mid(2));
	}
	int nDuring = atoi(during);
	m_pParentWnd->SendMessage(WM_CUT_ACTION, (WPARAM)&pos[0], nDuring);
}
