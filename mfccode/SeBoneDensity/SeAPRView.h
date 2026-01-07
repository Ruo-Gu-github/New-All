#pragma once
// SeAPRView 视图

class SeAPRView : public CImageViewerView
{
	DECLARE_DYNCREATE(SeAPRView)

protected:
	SeAPRView();           // 动态创建所使用的受保护的构造函数
	virtual ~SeAPRView();

public:
	virtual void OnDraw(CDC* pDC);      // 重写以绘制该视图
#ifdef _DEBUG
	virtual void AssertValid() const;
#ifndef _WIN32_WCE
	virtual void Dump(CDumpContext& dc) const;
#endif
#endif

protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual void		OnInitialUpdate();
	afx_msg BOOL		OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void		OnMouseMove(UINT nFlags, CPoint point);

public:
	void				SetDcmArray(CDcmPicArray* pDcmArray);
	void                Reset();
	
	void				SetPlaneNum(int nPlaneNum)      {m_nPlaneNum = nPlaneNum;}
	int					GetPlaneNum()					{return m_nPlaneNum;}

	void                InitView();

	void				UpdateImage();
	void				UpdateImage(int nPlaneNum);
	static	UINT		__UpdateImage(void* lpVoid);
	void				DrawImage(CDcmPic* pDcm);

	void                UpdateOtherView(MprRotateInfo rotateInfor);
	void                UpdateOtherView();

	void				RotatePosLine(float fAngle);
	void				SetPos(CPoint point);

	void				SetAPRTool();
	void                SetAPRRectTool();

	void				SetRotateInfor(MprRotateInfo rotateInfor)	{m_RotateInfo = rotateInfor;}
	MprRotateInfo		GetRotateInfor()							{return m_RotateInfo;}

	float				GetAngle(CPoint point);
	void				GetHitPoints(Rect* rect, int nl, int nt, int nr, int nb);
#ifdef CUDA_AVAILABLE
	SeVisualAPR_with_CUDA*        GetAPR(){return &m_SeVisualAPR;}
#else
	SeVisualAPR*		GetAPR(){return &m_SeVisualAPR;}
#endif
	SeVisualRotate*     GetRotate(){return &m_SeVisualRotate;}

	void                SetScreenWidth(int nWidth);
	void				SetScreenHeight(int nHeight);

	void              SetWinLevel(int nWinCenter, int nWinWidth);

private:
	void                APRDraw(CZKMemDC* pDC);
	void                APRRectDraw(CZKMemDC* pDC);

private:
	CDcmPicArray*		m_pDcmArray;
	SeVisualRotate      m_SeVisualRotate;
	MprRotateInfo		m_RotateInfo;
#ifdef CUDA_AVAILABLE
	SeVisualAPR_with_CUDA	m_SeVisualAPR;
#else
	SeVisualAPR			m_SeVisualAPR;
#endif
	bool                m_bMIPDownSample;
	int                 m_nPlaneNum;
	int                 m_nSelectedTool;
	int                 m_nDcmWidth;
	double              m_fZRotateLast;

	HANDLE				m_hDrawDcm;
	HANDLE				m_hReleaseDcm;

	CEvent				m_eventChange;
	CEvent				m_eventExit;

public:
	// 标志线
	static double		m_dXALLTrans;
	static double		m_dYALLTrans;
	static double		m_dZALLTrans;

	// 画框
	static int			m_nXStart;
	static int			m_nXEnd;
	static int			m_nYStart;
	static int			m_nYEnd;
	static int			m_nZStart;
	static int			m_nZEnd;

};


