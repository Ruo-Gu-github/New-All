#pragma once
#include "Resource.h"
#include "afxwin.h"

// CSeROIEditDlg 对话框

class CSeROIEditDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSeROIEditDlg)

public:
	CSeROIEditDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CSeROIEditDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG_ROI_EDIT_OPERATION };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedButtonRoiClear();
	afx_msg void OnBnClickedButtonMidLayer();
	afx_msg void OnBnClickedRadioFront();
	afx_msg void OnBnClickedRadioRoiAdd();

	const int GetPlaneNumNow();
	const int GetOperation();
    const int GetShape() const { return m_nShape; }

private:
	int m_nOperation;
	int m_nDirection;
	int m_nShape;
	CWnd* m_pParent;
public:
	afx_msg void OnClose();
	afx_msg void OnBnClickedRadioRoiShape();
};
