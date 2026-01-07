#pragma once
#include "afxcmn.h"
#include "resource.h"
#include "afxwin.h"
#include "BoneDensitySwapData.h"

class CSeBoneSegProcedureChangeDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSeBoneSegProcedureChangeDlg)

public:
	CSeBoneSegProcedureChangeDlg(CWnd* pParent = NULL);   // 标准构造函数
	CSeBoneSegProcedureChangeDlg(map<CString, int>* pmapFunc, CString* pFuncSingleChange, CWnd* pParent = NULL);
	virtual ~CSeBoneSegProcedureChangeDlg();

	// 对话框数据
	enum { IDD = IDD_DIALOG_BONECALPROCEDURE_CHANGE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();

	afx_msg void OnSelchangeComboFuncname();
	afx_msg void OnSelchangeComboKerneltype();

private:
	CWnd* m_pParent;

public:
	CComboBox m_cbFuncName;
	CComboBox m_cbKernelType;
	CEdit m_edKernelSize;

	map<CString, int>* m_pmapFunc;

	CString* m_pstrSingleFuncChange;
	CComboBox m_cbColor;
};
