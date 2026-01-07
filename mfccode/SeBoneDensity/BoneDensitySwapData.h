#pragma once

#ifndef PI
#define PI 3.14159265358979323846
#endif

// class	SeBoneDensityDlg;
// class	SeBoneDensityCtrlDlg;
// class	SeROIView;
// class	SeBoneDensity3DView;
// class	SeBoneDensityView;
// class	SeBoneAnalysisView;
// class	SeBoneModelView;
// class	SeBoneProgressDlg;
// class	SeBoneDensityDoc;
// class	SeBoneDensityResultDlg;
// class   SeAPRView;

class SeAPRView;
class SeROIView;
class SeBinaryView;

struct FuncSingle
{
	CString strColor;
	CString strFuncName;
	CString strKernelType;
	int		nKernelSize;
};

struct BoneDensitySwapData
{
public:
	BoneDensitySwapData(void);
	~BoneDensitySwapData(void);

public:

// 	MPRPObserverHost m_MPROberverHost;
// 
// 	CSize m_szDelta;
// 
// 	int						m_nWinCenter;
// 	int						m_nWinWidth;
// 
// 	img_process				m_ip;
// ////////////////////////////////////////////////////////////////////////
// 	SeBoneDensityDlg*		m_pParentDlg;
// 	SeBoneDensityCtrlDlg*	m_pCtrlDlg;
// 	SeBoneDensity3DView*	m_p3DView;
// 	SeBoneDensityView*		m_pBoneDensityView;
// 	SeBoneDensityDoc*		m_pBoneDensityDoc;
// 	SeBoneAnalysisView*		m_pBoneAnalysisView;
// 	CImageViewerDoc*		m_pBoneAnalysisDoc;
// 	SeBoneModelView*		m_pModelView;
// 	SeBoneDensityDoc*		m_pModelDoc;
// 	SeBoneProgressDlg*		m_dlgProgress;
// 
// 	SeAPRView*					m_pXOYView;
// 	SeVisualLibDoc*					m_pXOYDoc;
// 	SeAPRView*					m_pYOZView;
// 	SeVisualLibDoc*					m_pYOZDoc;
// 	SeAPRView*					m_pXOZView;
// 	SeVisualLibDoc*					m_pXOZDoc;
// 
// 
// 	CImageViewerView*				m_pOriImageView;
// 	CImageViewerDoc*				m_pOriImageDoc;

// 	int						m_nXstart;
// 	int						m_nYstart;
// 	int						m_nZstart;
// 	int						m_nWidth;
// 	int						m_nHeight;
// 	int						m_nZpiece;
// 	int						m_nCuttingPlaneWidth;
// 	int						m_nCuttingPlaneHeight;
// 	double					m_dPixelperPiece;
// 	double                  m_dbXYSliceSpace;
// 	double                  m_dbZSliceSpace;
// 	int						m_nStep;
// 	bool					m_bCalculateDirect;
// 	BOOL					m_bClipOutside;
// 	short					m_sMinValue;
// 
// 	CString					m_csFolderPath;
// 
// 	SeBoneDensityResultDlg*	m_dlgResult;
// public:
// 	void		CreateFolder( CString csPath );
// 	void		CreatProDlg( CWnd * pWnd);
// 	void		SetProgress( int nPos );



	
	SeAPRView*					m_pXOYView;
	SeVisualLibDoc*				m_pXOYDoc;
	SeAPRView*					m_pXOZView;
	SeVisualLibDoc*				m_pXOZDoc;
	SeAPRView*					m_pYOZView;
	SeVisualLibDoc*				m_pYOZDoc;
	SeROIView*					m_pROIView;
	CImageViewerDoc*            m_pROIDoc;
	SeBinaryView*				m_pBinaryView;
	CImageViewerDoc*            m_pBinaryDoc;




	int							m_nStep;
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
	int                         m_nRotateDcmSideLength;
	int                         m_nScreenWidth;
	int                         m_nScreenHeight;
	CString                     m_csSeriesName;

	int                         m_nSamllValue;
	int                         m_nBigValue;
	float                       m_fSmallDensity;
	float                       m_fBigDensity;

	//boneseg
	vector<FuncSingle>			m_vecFuncList;
	map<CString, int>			m_mapFunc;
	int							m_nMinValuePos;
	int							m_nMaxValuePos;
	bool						m_bPosChanged;

	CString						m_csSegPartName;
	bool						m_bSeg2Cal;
	

	// temporary
	LONGLONG                    m_tmpSize;

};

extern	BoneDensitySwapData	theBoneDensitySwapData;
