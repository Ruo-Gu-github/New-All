#pragma once
#include "Resource.h"

// CSeFreeCutDlg 对话框

class CSeFreeCutDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSeFreeCutDlg)

public:
	CSeFreeCutDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CSeFreeCutDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG_FREE_CUT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnClose();

private:
	CSliderCtrl m_SliderXStart;
	CSliderCtrl m_SliderXEnd;
	CSliderCtrl m_SliderYStart;
	CSliderCtrl m_SliderYEnd;
	CSliderCtrl m_SliderZStart;
	CSliderCtrl m_SliderZEnd;
	CWnd* m_pParent;

public:
	afx_msg void OnEnChangeEditXs();
	afx_msg void OnEnChangeEditXe();
	afx_msg void OnEnChangeEditYs();
	afx_msg void OnEnChangeEditYe();
	afx_msg void OnEnChangeEditZs();
	afx_msg void OnEnChangeEditZe();
	int m_edit_xs;
	int m_edit_xe;
	int m_edit_ys;
	int m_edit_ye;
	int m_edit_zs;
	int m_edit_ze;
};
