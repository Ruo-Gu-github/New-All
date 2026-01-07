#pragma once
#include "resource.h"

// CSeMorphologyDlg 对话框

class CSeMorphologyDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSeMorphologyDlg)

public:
	CSeMorphologyDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CSeMorphologyDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG_MORPHOLOGY_OPERATION };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonErosion();
	afx_msg void OnBnClickedButtonDelite();
	afx_msg void OnBnClickedButtonClose();
	afx_msg void OnBnClickedButtonOpen();
	afx_msg void OnBnClickedButtonFloodfill();
	afx_msg void OnBnClickedButtonInverse();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();

private:
	UINT m_nKernel;
	CWnd* m_pParent;
	BOOL  m_bFloodFill;
};
