#pragma once
#include "Resource.h"
#include "afxcmn.h"
#include "afxcolorbutton.h"

// CSeLightDlg 对话框

class CSeLightDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSeLightDlg)

public:
	CSeLightDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CSeLightDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG_LIGHT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnNMCustomdrawSliderEmission(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMCustomdrawSliderDiffuse(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMCustomdrawSliderReflect(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMCustomdrawSliderSpecular(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedColorLight();
	afx_msg void OnBnClickedColorMaterial();
	afx_msg void OnBnClickedCheckLight();
	afx_msg void OnBnClickedCheckShadow();

private:
	LIGHT_INFO	m_LightInfo;
	CWnd*       m_pParentWnd;
	CPoint      m_ptLightPos;
	CRect       m_rectLightArea;
	BOOL        m_bLightMoving;
	// CPngImage   m_imgLight;
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnNMCustomdrawSliderShadow(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnPaint();

private:
	void  DrawLight(CPaintDC* pDC, CPoint ptCenter, CRect rectValid);
	void  LoadLightSetting(CString csFile);
	void  SaveLightSetting(CString csFile);
public:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
