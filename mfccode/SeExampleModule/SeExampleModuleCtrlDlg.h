#pragma once
#include "resource.h"
#include "afxwin.h"
#include "afxcmn.h"

// SeBoneDensityCtrlDlg 对话框

class SeExampleModuleCtrlDlg : public CSeDialogBase
{
	DECLARE_DYNAMIC(SeExampleModuleCtrlDlg)
public:
	SeExampleModuleCtrlDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~SeExampleModuleCtrlDlg();

	// 对话框数据
	enum { IDD = IDD_SEEXAMPLEMODULECTRLDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg BOOL			OnEraseBkgnd(CDC* pDC);
	afx_msg HBRUSH			OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void			OnSize(UINT nType, int cx, int cy);
	afx_msg int				OnCreate(LPCREATESTRUCT lpCreateStruct);

	void                    Reset();
private: 
	HBRUSH                  m_brush;
public:
	afx_msg void OnBnClickedButtonExample1();
	afx_msg void OnBnClickedButtonExample2();
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedButtonMinip2();
	afx_msg void OnBnClickedButtonMinip3();
	afx_msg void OnBnClickedButtonMinip4();
	afx_msg void OnBnClickedButtonMinip5();
	afx_msg void OnBnClickedButtonMip();
	afx_msg void OnBnClickedButtonMinip();
	afx_msg LRESULT         OnAddMask(WPARAM wParam, LPARAM lParam);


private:
	int m_nSelect;
	int m_nFatMin;
	int m_nFatMax;
	int m_nLungMin;
	int m_nLungMax;
	int m_nBoneMin;
	int m_nBoneMax;
	int m_nNowPos;
public:
	
};
