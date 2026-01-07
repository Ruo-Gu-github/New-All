#pragma once
#include "resource.h"
#include "afxcmn.h"
#include "afxcolorbutton.h"

// CSe3DPropertityDlg 对话框



class CSe3DPropertityDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSe3DPropertityDlg)

public:
	CSe3DPropertityDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CSe3DPropertityDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG_VOLUME_PROPERTIES };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedMfccolorbuttonBkGnd();
	afx_msg void OnNMCustomdrawSliderMovespeed(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMCustomdrawSliderRotatespeed(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMCustomdrawSliderScalespeed(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMCustomdrawSliderQuality(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposSpinLightPosX(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposSpinLightPosY(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposSpinLightPosZ(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposSpinViewPosX(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposSpinViewPosY(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposSpinViewPosZ(NMHDR *pNMHDR, LRESULT *pResult);
	


private:
	VOLUME_INFO m_volumeInfo;
	CWnd*       m_pParentWnd;
	COLORREF    m_color;
};
