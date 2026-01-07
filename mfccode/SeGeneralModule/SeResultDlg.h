#pragma once
#include "resource.h"
#include "Measurement.h"
#include "afxcmn.h"

// SeResultDlg 对话框

class CSeResultDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSeResultDlg)

public:
	CSeResultDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CSeResultDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG_MEASURE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

public:
	void UpdateTable(vector<CMeasurement*>& vecResult, int nPos);
	void AddTable(CMeasurement* Result);
	void RefreshTable(vector<CMeasurement*>& vecResult);
	afx_msg LRESULT OnUpdateResult(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAddResult(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnRefreshResult(WPARAM wParam, LPARAM lParam);
	CListCtrl m_ResultList;
	virtual BOOL OnInitDialog();
	afx_msg void OnHdnItemdblclickListMeasure(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnNMDblclkListMeasure(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButtonShow();
	afx_msg void OnBnClickedButtonOutput();
	afx_msg void OnBnClickedButtonSaveMark();
	afx_msg void OnBnClickedButtonLoadMark();

private:
	BOOL ExportResultsToCsv(const CString& strFilePath);
	CString BuildCsvContent() const;
	BOOL SaveMarksToFile(const CString& strFilePath);
	BOOL LoadMarksFromFile(const CString& strFilePath);
	void ClearAllMeasurements();
	void AttachCurrentDcm(CMeasurement* pMeasurement) const;
};
