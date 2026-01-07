#pragma once
#include "afxcmn.h"
#include "resource.h"

// CSeNewMaskDlg 对话框

class CSeNewMaskDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSeNewMaskDlg)

public:
	CSeNewMaskDlg(CWnd* pParent = NULL);   // 标准构造函数
	CSeNewMaskDlg(LONG* pHistogram, LONG lMaxNumber, int nMax = 4096, int nMin = -1000, CWnd* pParent = NULL);
	virtual ~CSeNewMaskDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG_NEW_MASK };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnEnChangeEditMinValue();
	afx_msg void OnEnChangeEditMaxValue();

public:
	const int GetMin(){return m_nMin;}
	const int GetMax(){return m_nMax;}

private:
	void CSeNewMaskDlg::numOnly(int nID);

private:
	CSliderCtrl m_SliderMin;
	CSliderCtrl m_SliderMax;
	CWnd* m_pParent;
	int m_nMin;
	int m_nMax;
	LONG* m_pHistogram;
	LONG m_lMaxNumber;

};
