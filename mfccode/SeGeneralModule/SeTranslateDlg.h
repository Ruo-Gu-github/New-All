#pragma once
#include "resource.h"

// CSeTranslateDlg 对话框

class CSeTranslateDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSeTranslateDlg)

public:
	CSeTranslateDlg(CWnd* pParent = NULL);   // 标准构造函数
	CSeTranslateDlg(CWnd* pParent, int nRow, int nTranslate);
	virtual ~CSeTranslateDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG_TRANSLATE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnEnKillfocusEdit1();


private:
	int m_nTranslate;
	int m_nTranslateEdit;
	int m_nTranValue;
	int m_nRow;
	CWnd* m_pParent;
};
