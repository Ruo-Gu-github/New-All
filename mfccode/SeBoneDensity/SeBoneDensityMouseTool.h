#pragma once
class SeROITool : public CMouseTool
{
public:
	SeROITool(void);
	~SeROITool(void);
public:
	virtual void		Draw( CWnd* pWnd, CDC* pDC, CRect& rt );
	virtual void		OnLButtonDown( CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual void		OnLButtonUp( CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual void		OnMouseMove(  CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual HCURSOR		LoadCursor();


	void				DrawLine(vector <CPoint> vecPts, Color color, Graphics *gc, CDcmPic* pDcm);
	void				DrawRegion(vector <CPoint> vecPts, SolidBrush brush, Graphics *gc, CDcmPic* pDcm);

public:
	int					m_nPosition;
	BOOL				m_bLBDown;
	BOOL				m_bShiftDown;
	BOOL				m_bAltDown;
	vector <CPoint>		m_vecPtsTmp;

private:
	CScopeTool          m_ScopeTool;
};


class SeAPRMouseTool : public CMouseTool
{
public:
	friend class CDrawBase;
	SeAPRMouseTool();
	virtual ~SeAPRMouseTool();
public:
	virtual void		Draw(CWnd* pWnd, CDC *pDC, CRect& rt);
	virtual void		OnLButtonDown(CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual void		OnLButtonUp(CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual void		OnMouseMove(CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual HCURSOR		LoadCursor();


public:
	BOOL				m_bShiftDown;
	BOOL                m_bLBDown;
	int                 m_nPlaneNum;

	CPoint              m_ptLBDown;
	CPoint              m_ptMove;


	float               m_fAngle;
	float               m_fLength;
	float               m_fAbsoluteAngle;
	float               m_fZRotateLast;
	float               m_fZRotateCurrent;
	BOOL                m_bNewAngle;
};

class SeAPRRectMouseTool :public CMouseTool
{
public:
	friend class CDrawBase;
	SeAPRRectMouseTool();
	virtual ~SeAPRRectMouseTool();
public:
	virtual void		Draw(CWnd* pWnd, CDC *pDC, CRect& rt);
	virtual void		OnLButtonDown(CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual void		OnLButtonUp(CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual void		OnMouseMove(CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual HCURSOR		LoadCursor();

private:
	void                SetPoint(CWnd* pWnd, int nWidth, int nOffset);
	void                SetMouseState(CPoint point);
	void                MovePoint(CPoint point);
	void                DeSetPoint(CWnd* pWnd, int nWidth, int nOffset);
	void                HitTest(CPoint point);

	int					m_nMouseState;
	CRect				m_rtBorder[8];
	CPoint				m_ptLT;
	CPoint				m_ptRB;
	BOOL				m_bLBdown;
	CPoint				m_ptStart;
	int					m_nHitWhiceOne;
};