#pragma once
#include "Resource.h"
// SeGeneralModuleDlg 对话框

#include "SeGeneralModuleCtrlDlg.h"

class SeGeneralModuleDlg : public CDialog
{
	DECLARE_DYNAMIC(SeGeneralModuleDlg)

public:
	SeGeneralModuleDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~SeGeneralModuleDlg();

// 对话框数据
	enum { IDD = IDD_SeGeneralModuleDlg };

public:
	CDcmPicArray*				m_pOriPicArray;
	CImageViewerCtrl			m_wndXOY;
	CImageViewerCtrl			m_wndYOZ;
	CImageViewerCtrl			m_wndXOZ;
	CImageViewerCtrl            m_wnd3D;
	SeGeneralModuleCtrlDlg      m_wndCtrl;

public:
	void						Reset();
	void						ShowWnds();
	void						ResetWnds(CDcmPicArray * pDcmArray);
	void						InitMPRPos();
	void                        Init3D();
	
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg BOOL		OnEraseBkgnd(CDC* pDC);
	afx_msg int			OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void		OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT     OnChangeState(WPARAM wParam, LPARAM lParam);

private:
	SHOW_STATE					m_showState;
};
