#pragma once
#include "resource.h"
#include "afxcmn.h"

// CSeMovieDlg 对话框

class CSeMovieDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSeMovieDlg)

public:
	CSeMovieDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CSeMovieDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG_MOVIE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:

	afx_msg void OnBnClickedButtonRAdd();
	afx_msg void OnBnClickedButtonSAdd();
	afx_msg void OnBnClickedButtonCAdd();
	afx_msg void OnBnClickedButtonDOnestep();
	afx_msg void OnBnClickedButtonDRun();
	afx_msg void OnBnClickedButtonRecord();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

private:
	void ChangeColumns(CString type);
	void MoveUpColumns();
	void MoveDownColumns();
	void DeleteColumns();
	void ParseAction(CString type, CString start, CString end, CString during);
	void ParseRotate(CString start, CString end, CString during);
	void ParseScale(CString start, CString end, CString during);
	void ParseCut(CString start, CString end, CString during);

private:
	CWnd*       m_pParentWnd;
	CListCtrl m_listctlAction;
	int m_nRunPos;
	int m_nRTime;
	double m_dSStart;
	double m_dSEnd;
	int m_nSTime;
	int m_nCXStart;
	int m_nCXEnd;
	int m_nCTime;
	int m_nCYStart;
	int m_nCYEnd;
	int m_nCZStart;
	int m_nCZEnd;
	int m_nRXStart;
	int m_nRXEnd;
	int m_nRYStart;
	int m_nRYEnd;
	int m_nRZStart;
	int m_nRZEnd;
};
