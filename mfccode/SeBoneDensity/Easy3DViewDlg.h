#pragma once


// CEasy3DViewDlg 对话框

class CEasy3DViewDlg : public CDialog
{
	DECLARE_DYNAMIC(CEasy3DViewDlg)

public:
	CEasy3DViewDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CEasy3DViewDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG_EASY_3D };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	
public:
	void InitData(CDcmPicArray* pDcmArray, int nMin, int nMax, CRect rt);
	void Reset();

	// 截图相关
public:
	void                GetPrintScreen(CString strPath);
private:
	HBITMAP				GetScreenImage(LPRECT lpRect);
	BOOL				SaveImageToFile( HBITMAP hBitmap, LPCTSTR lpFileName );
	void				CreateFolder(CString csPath );
	void				DeliteData(BYTE* pSrc, BYTE* pRst, int nWidth, int nHeight, int nLength);

public:
	HDC				mhContext;
	COpenGLMain		m_glMainHandle;
	COpenGLShader*  m_pGLShader;
	COpenGLDataMgr* m_pGLDataMgr;
	COpenGLCamera*	m_pGLCamera;
	CPoint			m_ptOri;
	CPoint          m_ptRightOri;
	BOOL			m_bOpened;
	BYTE*           m_pData;

private:
	CClientDC*      m_pDc;	

};
