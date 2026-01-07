// SeNewVolumeObject.cpp : 实现文件
//

#include "stdafx.h"
#include "SeGeneralModule.h"
#include "SeNewVolumeObjectDlg.h"
#include "afxdialogex.h"


// CSeNewVolumeObjectDlg 对话框

IMPLEMENT_DYNAMIC(CSeNewVolumeObjectDlg, CDialogEx)

CSeNewVolumeObjectDlg::CSeNewVolumeObjectDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSeNewVolumeObjectDlg::IDD, pParent)
{
	m_nSelect = -1;
	m_pLstSource = NULL;
	m_pParent = NULL;
}

CSeNewVolumeObjectDlg::~CSeNewVolumeObjectDlg()
{
}

void CSeNewVolumeObjectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_VOLUME_OBJECT, m_lstVolumeObject);
}


BEGIN_MESSAGE_MAP(CSeNewVolumeObjectDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CSeNewVolumeObjectDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CSeNewVolumeObjectDlg::OnBnClickedCancel)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST_VOLUME_OBJECT, &CSeNewVolumeObjectDlg::OnNMCustomdrawListVolumeObject)
END_MESSAGE_MAP()


// CSeNewVolumeObjectDlg 消息处理程序


BOOL CSeNewVolumeObjectDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	DWORD dwStyle = m_lstVolumeObject.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;// 选中某行使整行高亮（只适用与 report 风格的 listctrl）
	m_lstVolumeObject.SetExtendedStyle(dwStyle); // 设置扩展风格

	m_lstVolumeObject.InsertColumn(0, "Color", LVCFMT_LEFT, 100);// 插入列
	m_lstVolumeObject.InsertColumn(1, "Visible", LVCFMT_LEFT, 100);
	m_lstVolumeObject.InsertColumn(2, "Min", LVCFMT_LEFT, 100);
	m_lstVolumeObject.InsertColumn(3, "Max", LVCFMT_LEFT, 100);

	// TODO:  在此添加额外的初始化
	int nCount = m_pLstSource->GetItemCount();
	for (int i=0; i<nCount; i++)
	{
		int nRow = m_lstVolumeObject.InsertItem(0, m_pLstSource->GetItemText(nCount - i - 1, 0));// 插入行
		m_lstVolumeObject.SetItemText(nRow, 1, m_pLstSource->GetItemText(nCount - i - 1, 1));// 设置数据
		m_lstVolumeObject.SetItemText(nRow, 2, m_pLstSource->GetItemText(nCount - i - 1, 2));// 设置数据
		m_lstVolumeObject.SetItemText(nRow, 3, m_pLstSource->GetItemText(nCount - i - 1, 3));// 设置数据
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CSeNewVolumeObjectDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	m_nSelect = (int)m_lstVolumeObject.GetFirstSelectedItemPosition() - 1;
	CDialogEx::OnOK();
}


void CSeNewVolumeObjectDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnCancel();
}


void CSeNewVolumeObjectDlg::OnNMCustomdrawListVolumeObject(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );

	*pResult = 0;
	// If this is the beginning of the control's paint cycle, request
	// notifications for each item.

	if ( CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	else if ( CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		// This is the pre-paint stage for an item.  We need to make another
		// request to be notified during the post-paint stage.

		*pResult = CDRF_NOTIFYPOSTPAINT;
	}
	else if ( CDDS_ITEMPOSTPAINT == pLVCD->nmcd.dwDrawStage )
	{
		// If this item is selected, re-draw the icon in its normal
		// color (not blended with the highlight color).
		LVITEM rItem;
		int    nItem = static_cast<int>( pLVCD->nmcd.dwItemSpec );
		ZeroMemory ( &rItem, sizeof(LVITEM) );
		rItem.mask  = LVIF_IMAGE | LVIF_STATE;
		rItem.iItem = nItem;
		rItem.stateMask = LVIS_SELECTED;
		m_lstVolumeObject.GetItem ( &rItem );

		CDC*  pDC = CDC::FromHandle ( pLVCD->nmcd.hdc );
		CRect rcIcon;
		// Get the rect that holds the item's icon.
		m_lstVolumeObject.GetSubItemRect(nItem, 0, LVIR_LABEL, rcIcon);
		rcIcon.right = rcIcon.left + rcIcon.Height();
		rcIcon.DeflateRect(2,2,2,2);

		COLORREF crBkgnd = m_colorList[nItem];
		// Draw the icon.
		pDC->FillSolidRect(rcIcon, crBkgnd);

		*pResult = CDRF_SKIPDEFAULT;

	}
}
