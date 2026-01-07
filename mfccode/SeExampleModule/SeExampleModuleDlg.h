#pragma once
#include "resource.h"
// SeExampleModuleDlg 对话框

class SeExampleModuleDlg : public CDialog
{
	DECLARE_DYNAMIC(SeExampleModuleDlg)

public:
	SeExampleModuleDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~SeExampleModuleDlg();

	// 对话框数据
	enum { IDD = IDD_SEEXAMPLEMODULEDLG };

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

private:
	//CImageViewerCtrl	m_wndExampleCtrl;
	//CImageViewerCtrl    m_wndOriCtrl;

	CImageViewerCtrl    m_ProjectionCtrl;

	CImageViewerCtrl    m_wndXOYViewCtrl;
	CImageViewerCtrl    m_wndXOZViewCtrl;
	CImageViewerCtrl    m_wndYOZViewCtrl;
public:
	CDcmPicArray*      m_pOriDcmArray;
	CDcmPicArray*      m_pProjectionArray;
};
