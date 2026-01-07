#pragma once
#include "afxcmn.h"
#include "resource.h"
#include "afxwin.h"
#include "BoneDensitySwapData.h"

// CSeBoneSegProcedureDlg 对话框

// struct FuncSingle
// {
// 	CString strFuncName;
// 	CString strKernelType;
// 	int		nKernelSize;
// };

class CSeBoneSegProcedureDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSeBoneSegProcedureDlg)

public:
	CSeBoneSegProcedureDlg(CWnd* pParent = NULL);   // 标准构造函数
	CSeBoneSegProcedureDlg(vector<FuncSingle> *pvecFuncs, map<CString, int> *m_pmapFunc, CWnd* pParent = NULL);
	virtual ~CSeBoneSegProcedureDlg();

	// 对话框数据
	enum { IDD = IDD_DIALOG_BONECALPROCEDURE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnClickListBoneCalProcedure(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDblclkListBoneCalProcedure(NMHDR *pNMHDR, LRESULT *pResult);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	
	//自定消息
	afx_msg LRESULT         OnDblckChange(WPARAM wParam, LPARAM lParam);

private:
	CWnd* m_pParent;

public:
	CListCtrl m_ListCtrBoneSeg;

	CComboBox m_cbFuncName;
	CComboBox m_cbKernelType;
	CEdit m_edKernelSize;

	//CComboBox m_cbSegName;

	bool m_bChange;
	bool m_bChangeLastTwo;
	int  m_nSelRow;
	int  m_nSelCol;
	CString m_strSelColor;
	CString m_strSelFuncName;
	CString m_strSelKernelSize;
	CString m_strSelKernelType;

	CListCtrl* pmyListCtrl;

	map<CString, int>* m_pmapFunc;

	vector<FuncSingle>* m_pvecFuncList;

	CString strChangeSingleFun[4];

	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnSelchangeComboFuncname();
	afx_msg void OnSelchangeComboKerneltype();
	afx_msg void OnBnClickedButtonRun();
	afx_msg void OnBnClickedButtonReset();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnBnClickedButtonLoad();
	afx_msg void OnBnClickedButtonSave();

	//
	void CSeBoneSegProcedureDlg::ReadFile(const char *pFileName, char *pFileContent);
	CComboBox m_cbColor;
	//CComboBox m_cbShowColorInfer;

	CColorComboBox m_cbShowColorInferColor;
	CColorComboBox m_cbColorColor;

	afx_msg void OnSelchangeComboShowcolorInfer();
	CEdit m_edSegName;
};
