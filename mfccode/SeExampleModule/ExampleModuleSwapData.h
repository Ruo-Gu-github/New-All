#pragma once
class SeExampleView;
class CSeFattyOriView;
class CSeFattyProjectionView;
class CSeFattyZSelectView;

// 所有视图类定义


struct ExampleModuleSwapData
{
public:
	ExampleModuleSwapData(void);
	~ExampleModuleSwapData(void);

public:
	SeExampleView*           m_pExampleView;
	CImageViewerDoc*         m_pExampleDoc;

	CSeFattyOriView*         m_pOriView;
	CImageViewerDoc*         m_pOriDoc;

	CSeFattyProjectionView*  m_pProjectionView;
	CImageViewerDoc*         m_pProjectionDoc;

	CSeFattyZSelectView*      m_pXOYView;
	CImageViewerDoc*         m_pXOYDoc;

	CSeFattyZSelectView*      m_pXOZView;
	CImageViewerDoc*         m_pXOZDoc;

	CSeFattyZSelectView*      m_pYOZView;
	CImageViewerDoc*         m_pYOZDoc;

	
	// 步骤
	int							m_nStep;

	// 图片信息
	int							m_nWidth;
	int							m_nHeight;
	int							m_nLength;
	int							m_nVoltage;
	int                         m_nCurrent;
	CString                     m_csFolderPath;
	double                      m_dbZSliceSpace;
	double                      m_dbXYSliceSpace;
	int							m_nWinWidth;
	int							m_nWinHeight;
	LONG						m_Histogram[65536];
	int							m_nMaxValue;
	int							m_nMinValue;
	LONG						m_lMaxNumber;
	int                         m_nScreenWidth;
	int                         m_nScreenHeight;
	CString                     m_csSeriesName;
	short*                      m_pProjectionData;
	short*                      m_pSrcData;
};

extern	ExampleModuleSwapData	theExampleModuleSwapData;
