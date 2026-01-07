#pragma once
#include "Resource.h"
#include "afxwin.h"
#include "afxcmn.h"

// SeGeneralModuleCtrlDlg 对话框


class CSeMorphologyDlg;
class CSeBooleanDlg;
class CSeROIEditDlg;
class CSeROIData;
class CSe3DPropertityDlg;
class CSeTransFuncDlg;
class CSeFreeCutDlg;
class CSeResultDlg;
class CSeLightDlg;
class CSeMovieDlg;

class SeGeneralModuleCtrlDlg : public CSeDialogBase
{
	DECLARE_DYNAMIC(SeGeneralModuleCtrlDlg)

public:
	SeGeneralModuleCtrlDlg(CWnd* pParent = NULL);   // 标准构造函数 
	virtual ~SeGeneralModuleCtrlDlg();

// 对话框数据
	enum { IDD = IDD_SeGeneralModuleCtrlDlg };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

	afx_msg void OnBnClickedChoiceHandle();
	afx_msg void OnBnClickedButton2dAdd();
	afx_msg void OnBnClickedButton2dRemove();
	afx_msg void OnBnClickedButton2dColor();
	afx_msg void OnBnClickedButton2dInfo();
	afx_msg void OnBnClickedButton3dAdd();
	afx_msg void OnBnClickedButton3dRemove();
	afx_msg void OnBnClickedButton3dColor();
	afx_msg void OnBnClickedButton3dTranslate();
	afx_msg void OnBnClickedMorphology();
	afx_msg void OnBnClickedRoiEdit();
	afx_msg void OnBnClickedBoolean();
	afx_msg void OnBnClicked3dControl();
	afx_msg void OnBnClickedPrintscreen3d();
	afx_msg void OnBnClickedRenderMode();
	afx_msg void OnBnClickedTransFunc();
	afx_msg void OnBnClickedFreeCut();
	afx_msg void OnBnClickedExportFromMask();

	afx_msg void OnNMClickList2dPlane(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMClickList3dPlane(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMClickList3dPlane2(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMCustomdrawList2dPlane(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMCustomdrawList3dPlane(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMCustomdrawList3dPlane2(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkList2dPlane(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkList3dPlane(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkList3dPlane2(NMHDR *pNMHDR, LRESULT *pResult);

// 自定消息
	afx_msg LRESULT OnAddMask(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAddMaskItem(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAddMaskItemFalse(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnMorphologyOperation(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnBooleanOperation(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnChangeROIEditPlane(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnChangeROIShape(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDeleteROI(WPARAM wPAram, LPARAM lParam);
	afx_msg LRESULT OnMidLayer(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnExecuteROI(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSetMouseTool(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSet3DState(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSetLightState(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSetTransFunc(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAdjustFreeCut(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnCloseFreeCutWindow(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnChangeTranslate(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnCloseAllWindow(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnRotateAction(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnScaleAction(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnCutAction(WPARAM wParam, LPARAM lParam);
	void            Reset();



private:
	CGroupControl		m_Group;
	CGroupControl		m_Group2DPlane;
	CGroupControl		m_Group3DPlane;
	CGroupControl       m_Group3DPlane2;
	BOOL                m_bRayCasting;
	BOOL				m_bBorder;
	BOOL				m_bSetValue;
	BOOL                m_bFloodFill;
	int					m_nEditID;
	int					m_nColorIndex;
	int					m_nPlaneNumNow;

	CSeMorphologyDlg*	m_pMorPhologyDlg;
	CSeBooleanDlg*		m_pBooleanDlg;
	CSeROIEditDlg*		m_pROIEditDlg;
	CSe3DPropertityDlg* m_p3DProrertityDlg;
	CSeTransFuncDlg*    m_pTransFuncDlg;
	CSeFreeCutDlg*      m_pFreeCutDlg;
	CSeResultDlg*       m_pResultDlg;
	CSeLightDlg*        m_pLightDlg;
	CSeMovieDlg*        m_pMovieDlg;

	CListCtrl			m_lst2DPlane;
	CListCtrl			m_lst3DPlane;
	CListCtrl           m_lst3DPlane2;

	COLORREF			m_ColorLst[8];
	
	float               m_fFreeCutRange[6];
	BOOL                m_bMaskTool;
	int                 m_nMipThickness;
	int                 m_nMinIpThickness;
	void                UpdateProjectionSliderText();

public:
	
	afx_msg void OnBnClickedRandomPick();
	afx_msg void OnBnClicked3dOutput();
	CComboBox m_templateCtl;
	afx_msg void OnCbnSelchangeTemplate();
	afx_msg void OnBnClickedToolLine();
	afx_msg void OnBnClickedToolAngle();
	afx_msg void OnBnClickedToolShape();
	afx_msg void OnBnClickedToolValue();
	afx_msg void OnBnClickedToolArea();
	afx_msg void OnBnClickedToolSelect();
	afx_msg void OnBnClickedExportWithMask();
	CSliderCtrl m_sliderStepScale;
	CSliderCtrl m_sliderOffsetScale;
	afx_msg void OnNMCustomdrawSliderStepScale(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMCustomdrawSliderOffsetScale(NMHDR *pNMHDR, LRESULT *pResult);

	afx_msg void OnBnClicked3dLight();
	afx_msg void OnBnClickedExportSharp();
	afx_msg void OnBnClickedButton2dSave();
	afx_msg void OnBnClickedButton2dLoad();
	afx_msg void OnBnClicked3dLine();
	afx_msg void OnBnClickedToolScale();
	afx_msg void OnBnClickedToolReset();
	afx_msg void OnBnClickedMovieRecord();
	afx_msg void OnBnClickedButtonSaveMask();
	afx_msg void OnBnClickedButtonLoadMask();

	afx_msg void OnBnClickedButton3dAdd2();
	afx_msg void OnBnClickedButton3dRemove2();
	afx_msg void OnBnClickedButton3dColor2();
	afx_msg void OnBnClickedButton3dTranslate2();

// 不应该在这 临时
	BOOL						GetFilesName(CStringArray& csaFiles);

	afx_msg void OnBnClickedToolEllipse();
};
