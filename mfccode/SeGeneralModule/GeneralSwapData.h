#pragma once

class SeGeneralModuleDlg;
class SeGeneralModuleCtrlDlg;
class Se3DView;
class SeMPRView;

struct GeneralSwapData
{
public:
	GeneralSwapData();
	~GeneralSwapData();


 	SeMPRView*							m_pXOYView;
 	SeVisualLibDoc*						m_pXOYDoc;
 	SeMPRView*							m_pYOZView;
 	SeVisualLibDoc*						m_pYOZDoc;
 	SeMPRView*							m_pXOZView;
 	SeVisualLibDoc*						m_pXOZDoc;

	Se3DView*							m_p3DView;

	CString                             m_csFolderPath;
	int                                 m_nWidth;
	int									m_nHeight;
	int                                 m_nDepth;
	double                              m_dbZSliceSpace;
	double                              m_dbXYSliceSpace;

	LONG                                m_Histogram[65536];
	int                                 m_nMaxValue;
	int                                 m_nMinValue;
	LONG                                m_lMaxNumber;

};

extern GeneralSwapData	theGeneralSwapData;