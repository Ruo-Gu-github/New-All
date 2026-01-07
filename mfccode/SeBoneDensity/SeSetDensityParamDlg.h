#pragma once


// CSeSetDensityParamDlg 对话框

class CSeSetDensityParamDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSeSetDensityParamDlg)

public:
	CSeSetDensityParamDlg(CWnd* pParent = NULL);   // 标准构造函数
	CSeSetDensityParamDlg(int nSmall, int nBig, float fSamllDensity, float fBigDensity, CWnd* pParent = NULL);
	virtual ~CSeSetDensityParamDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG_SET_DENSITY };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnEnChangeEditLowValue();
	afx_msg void OnEnChangeEditLowDensity();
	afx_msg void OnEnChangeEditHighValue();
	afx_msg void OnEnChangeEditHighDensity();
	afx_msg void OnBnClickedOk();

	const int GetSmallValue(){return m_nSmallValue;}
	const int GetBigValue(){return m_nBigValue;}
	const float GetSmallDensity(){return m_fSmallDensity;}
	const float GetBigDensity(){return m_fBigDensity;}

	void SetInfo(int nSmall, int nBig, float fSmallDensity, float fBigDensity);

private:
	void					numOnly(int nID);

private:
	int m_nSmallValue;
	int m_nBigValue;
	float m_fSmallDensity;
	float m_fBigDensity;
public:
	virtual BOOL OnInitDialog();
};
