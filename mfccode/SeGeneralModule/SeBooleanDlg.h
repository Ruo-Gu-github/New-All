#pragma once
#include "resource.h"
#include "afxwin.h"

// CSeBOOLeanOperation 对话框

class CSeBooleanDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSeBooleanDlg)

public:
	CSeBooleanDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CSeBooleanDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG_BOOLEAN_OPERATION };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	CComboBox m_cbOperation;
	CColorComboBox m_cbMaskA;
	CColorComboBox m_cbMaskB;
	virtual BOOL OnInitDialog();
	CWnd* m_pParent;
	vector <COLORREF> m_colorList;

public: 
	void UpdateCBData();
};
