#pragma once
#include "afxcmn.h"
#include "resource.h"
#include "Measurement.h"
#include <vector>

// CSePicture 对话框

class CSePicture : public CDialogEx
{
	DECLARE_DYNAMIC(CSePicture)

public:
	CSePicture(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CSePicture();

// 对话框数据
	enum { IDD = IDD_DIALOG_PICTURE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	BOOL SetMeasurement(CMeasurement* pMeasurement);

protected:
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();

private:
	void DrawProfile(CDC* pDC, const CRect& rcPlotArea);
	void DrawHistogram(CDC* pDC, const CRect& rcPlotArea);
	void BuildHistogram(const std::vector<int>& samples);

private:
	CMeasurement* m_pMeasurement;
	bool m_bHistogram;
	bool m_bDataReady;
	CString m_strChartTitle;
	std::vector<double> m_profileAxis;
	std::vector<double> m_profileValues;
	std::vector<double> m_histBins;
	std::vector<int> m_histCounts;
	int m_nHistMaxCount;
	double m_histMinValue;
	double m_histMaxValue;
};
