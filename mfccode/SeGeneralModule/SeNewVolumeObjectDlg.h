#pragma once
#include "resource.h"
#include "afxcmn.h"


// CSeNewVolumeObjectDlg 对话框

class CSeNewVolumeObjectDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSeNewVolumeObjectDlg)

public:
	CSeNewVolumeObjectDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CSeNewVolumeObjectDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG_NEW_VOLUME_OBJECT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	CListCtrl m_lstVolumeObject;

	CListCtrl* m_pLstSource;
	vector <COLORREF> m_colorList;
	CWnd*      m_pParent;
	int m_nSelect;
	afx_msg void OnNMCustomdrawListVolumeObject(NMHDR *pNMHDR, LRESULT *pResult);
};
