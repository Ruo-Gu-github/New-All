#pragma once
#include "resource.h"
// #include "SeBoneDensityResultDlg.h"
// #include "SeRotateBoxDlg.h"
// SeBoneDensityDlg 对话框

class SeBoneDensityDlg : public CDialog
{
	DECLARE_DYNAMIC(SeBoneDensityDlg)

public:
	SeBoneDensityDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~SeBoneDensityDlg();

// 对话框数据
	enum { IDD = IDD_SEBONEDENSITYDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	DECLARE_MESSAGE_MAP()
public:

	void    Reset();
	void    ShowFirstView();

// 	void	ShowImage();
// 	void	GetRoiRegion();
// 	void	GetSeries();
// 	void    GetROIArray();
// 	void	ReleaseRotateArray();
// 	void	ReleaseBoneArray();
// 	void	OpenModelFile();
// 	void	ReturnToLastStep();
// 	void	ReLoadBeforeRoiSelectData();
// 	void	ReLoadBeforeBinaryData();
// 	void	Reset();
// 
//  private:
// 	void	InitAPRPos();
// 	void	InitRoiParamter();
// 	void	Load2DData();
// 	void	LoadSingleDcm(CString path, int index);
// 	void	Load3DData();
// 	void	SaveDcmArray(CDcmPicArray* pDcmArray, CString path);

public:
	CDcmPicArray*	m_pOriPicArray;
// 	CDcmPicArray	m_ZROIPicArray;
// 	CDcmPicArray	m_RotateArray;
// 	CDcmPicArray	m_BoneArray;
// 	CDcmPicArray	m_ModelArray;

//	SeVisualAPR			m_SeViualAPR;

	CImageViewerCtrl	m_wndAPRXOYCtrl;
	CImageViewerCtrl	m_wndAPRYOZCtrl;
	CImageViewerCtrl	m_wndAPRXOZCtrl;
	CImageViewerCtrl	m_wndROICtrl;
	CImageViewerCtrl	m_wndBinaryCtrl;



/*	SeBoneDensityResultDlg m_resultDlg;*/
/*	SeRotateBoxDlg		m_RotateBoxDlg;*/

private:
	int					m_nTimes;
};
