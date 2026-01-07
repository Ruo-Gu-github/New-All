#pragma once

class CMeasurement;

class SeROIMouseTool : public CMouseTool
{
public:
	friend class CDrawBase;
	SeROIMouseTool();
	virtual ~SeROIMouseTool();
public:
	virtual void		Draw( CWnd* pWnd, CDC* pDC, CRect& rt );
	virtual void		OnLButtonDown( CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual void		OnLButtonUp( CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual void		OnMouseMove(  CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual HCURSOR		LoadCursor();

private:
// 	void				DrawLine(vector <CPoint> vecPts, Color color, int nWidth, Graphics* gc, CDcmPic* pDcm);
// 	void				DrawRegion(vector <CPoint> vecPts, SolidBrush brush, Graphics* gc, CDcmPic* pDcm);
	CScopeTool			m_ScopeTool;

public:
	BOOL                m_bClipInside;
	BOOL				m_bLBDown;
	BOOL				m_bAltDown;
	BOOL				m_bShiftDown;
	vector <CPoint>		m_vecPtsTmp;
    CPoint             m_ptStartImg;
    CPoint             m_ptCurrentImg;
};


class SeMPRMouseTool : public CMouseTool
{
public:
	friend class CDrawBase;
	SeMPRMouseTool();
	virtual ~SeMPRMouseTool();
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

};

class SeFloodFillTool : public CMouseTool
{
public:
	friend class CDrawBase;
	SeFloodFillTool();
	virtual ~SeFloodFillTool();
public:
	virtual void		Draw(CWnd* pWnd, CDC *pDC, CRect& rt);
	virtual void		OnLButtonDown(CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual void		OnLButtonUp(CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual void		OnMouseMove(CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual HCURSOR		LoadCursor();
};

//class SeSharpTool : public CMouseTool
//{
//public:
//	friend class CDrawBase;
//	SeSharpTool();
//	virtual ~SeSharpTool();
//public:
//	virtual void      Draw(CWnd* pWnd, CDC *pDC, CRect& rt);
//	virtual void      OnLButtonDown(CWnd* pWnd, UINT nFlags, CPoint& point);
//	virtual void      OnLButtonUp(CWnd* pWnd, UINT nFlags, CPoint& point);
//	virtual void      OnMouseMove(CWnd* pWnd, UINT nFlags, CPoint& point);
//	virtual HCURSOR   LoadCursor();
//};

class SeMeasureLineTool : public CMouseTool
{
public:
	friend class CDrawBase;
	SeMeasureLineTool();
	virtual ~SeMeasureLineTool();
public:
	virtual void		Draw(CWnd* pWnd, CDC *pDC, CRect& rt);
	virtual void		OnLButtonDown(CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual void		OnLButtonUp(CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual void		OnMouseMove(CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual HCURSOR		LoadCursor();

private:
	BOOL				m_bLBDown;
	CPoint				m_ptStart;
	CPoint              m_ptEnd;
};

class SeMeasureAngleTool : public CMouseTool
{
public:
	friend class CDrawBase;
	SeMeasureAngleTool();
	virtual ~SeMeasureAngleTool();
public:
	virtual void		Draw(CWnd* pWnd, CDC *pDC, CRect& rt);
	virtual void		OnLButtonDown(CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual void		OnLButtonUp(CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual void		OnMouseMove(CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual HCURSOR		LoadCursor();

private:
	BOOL				m_bLBDown;
	BOOL                m_bFirstClick;
	BOOL                m_bSecondClick;
	BOOL                m_bThirdClick;
	CPoint				m_ptStart;
	CPoint              m_ptAngle;
	CPoint              m_ptEnd;

};

class SeMeasureShapeTool : public CMouseTool
{
public:
	friend class CDrawBase;
	SeMeasureShapeTool();
	virtual ~SeMeasureShapeTool();
public:
	virtual void		Draw(CWnd* pWnd, CDC *pDC, CRect& rt);
	virtual void		OnLButtonDown(CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual void		OnLButtonUp(CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual void		OnMouseMove(CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual HCURSOR		LoadCursor();

private:
	BOOL				m_bLBDown;
	CPoint				m_ptStart;
	CPoint              m_ptEnd;
};

class SeMeasureCTTool : public CMouseTool
{
public:
	friend class CDrawBase;
	SeMeasureCTTool();
	virtual ~SeMeasureCTTool();
public:
	virtual void		Draw(CWnd* pWnd, CDC *pDC, CRect& rt);
	virtual void		OnLButtonDown(CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual void		OnLButtonUp(CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual void		OnMouseMove(CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual HCURSOR		LoadCursor();
private:
	CPoint              m_ptPosition;
	int                 m_nPlanenumber;
};

class SeMeasureAreaTool : public CMouseTool
{
public:
	friend class CDrawBase;
	SeMeasureAreaTool();
	virtual ~SeMeasureAreaTool();
public:
	virtual void		Draw(CWnd* pWnd, CDC *pDC, CRect& rt);
	virtual void		OnLButtonDown(CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual void		OnLButtonUp(CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual void		OnMouseMove(CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual HCURSOR		LoadCursor();

private:
// 	void				DrawLine(vector <CPoint> vecPts, Color color, int nWidth, Graphics* gc, CDcmPic* pDcm);
// 	void				DrawRegion(vector <CPoint> vecPts, SolidBrush brush, Graphics* gc, CDcmPic* pDcm);

public:
	BOOL				m_bLBDown;
	vector <CPoint>		m_vecPtsTmp;
};

class SeMeasureSelectTool : public CMouseTool
{
public:
	friend class CDrawBase;
	SeMeasureSelectTool();
	~SeMeasureSelectTool();

public:
	virtual void		Draw(CWnd* pWnd, CDC *pDC, CRect& rt);
	virtual void		OnLButtonDown(CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual void		OnLButtonUp(CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual void		OnMouseMove(CWnd* pWnd, UINT nFlags, CPoint& point);
	virtual HCURSOR		LoadCursor();

private:
	BOOL                m_bLBDown;
	BOOL                m_bPointSelected;
	BOOL                m_bRectSelected;
	CPoint				m_ptOri;
	CPoint*             m_pSelectedPoint;
	CMeasurement*       m_ptrSelectedTool;
	int					m_nMouseState;
	int                 m_nSelectedIndex;
// 	void				DrawLine(vector <CPoint> vecPts, Color color, int nWidth, Graphics* gc, CDcmPic* pDcm);
// 	void				DrawRegion(vector <CPoint> vecPts, SolidBrush brush, Graphics* gc, CDcmPic* pDcm);
};

static void DrawTools(CWnd* pWnd, CDC *pDC, CRect& rt);

static void DrawLine(vector <CPoint> vecPts, Color color, int nWidth, Graphics* gc, CDcmPic* pDcm);

static void DrawRegion(vector <CPoint> vecPts, SolidBrush brush, Graphics* gc, CDcmPic* pDcm);

// class SeAPRMouseTool : public CMouseTool
// {
// public:
// 	friend class CDrawBase;
// 	SeAPRMouseTool();
// 	virtual ~SeAPRMouseTool();
// public:
// 	virtual void		Draw(CWnd* pWnd, CDC *pDC, CRect& rt);
// 	virtual void		OnLButtonDown(CWnd* pWnd, UINT nFlags, CPoint& point);
// 	virtual void		OnLButtonUp(CWnd* pWnd, UINT nFlags, CPoint& point);
// 	virtual void		OnMouseMove(CWnd* pWnd, UINT nFlags, CPoint& point);
// 	virtual HCURSOR		LoadCursor();
// 
// 
// public:
// 	BOOL				m_bShiftDown;
// 	BOOL                m_bLBDown;
// 	int                 m_nPlaneNum;
// 
// 	CPoint              m_ptLBDown;
// 	CPoint              m_ptMove;
// 
// 
// 	float               m_fAngle;
// 	float               m_fLength;
// 	float               m_fAbsoluteAngle;
// 	float               m_fZRotateLast;
// 	float               m_fZRotateCurrent;
// 	BOOL                m_bNewAngle;
// };

// class SeAPRRectMouseTool :public CMouseTool
// {
// public:
// 	friend class CDrawBase;
// 	SeAPRRectMouseTool();
// 	virtual ~SeAPRRectMouseTool();
// public:
// 	virtual void		Draw(CWnd* pWnd, CDC *pDC, CRect& rt);
// 	virtual void		OnLButtonDown(CWnd* pWnd, UINT nFlags, CPoint& point);
// 	virtual void		OnLButtonUp(CWnd* pWnd, UINT nFlags, CPoint& point);
// 	virtual void		OnMouseMove(CWnd* pWnd, UINT nFlags, CPoint& point);
// 	virtual HCURSOR		LoadCursor();
// 
// private:
// 	void                SetPoint(CWnd* pWnd, int nWidth, int nOffset);
// 	void                SetMouseState(CPoint point);
// 	void                MovePoint(CPoint point);
// 	void                DeSetPoint(CWnd* pWnd, int nWidth, int nOffset);
// 	void                HitTest(CPoint point);
// 
// 	int    m_nMouseState;
// 	CRect  m_rtBorder[8];
// 	CPoint m_ptLT;
// 	CPoint m_ptRB;
// 	BOOL   m_bLBdown;
// 	CPoint m_ptStart;
// 	int    m_nHitWhiceOne;
// };

class SeEllipseTool : public CMouseTool
{
public:
    friend class CDrawBase;
    SeEllipseTool();
    virtual ~SeEllipseTool();

public:
    virtual void        Draw(CWnd* pWnd, CDC* pDC, CRect& rt);
    virtual void        OnLButtonDown(CWnd* pWnd, UINT nFlags, CPoint& point);
    virtual void        OnLButtonUp(CWnd* pWnd, UINT nFlags, CPoint& point);
    virtual void        OnMouseMove(CWnd* pWnd, UINT nFlags, CPoint& point);
    virtual HCURSOR     LoadCursor();

private:
    BOOL                m_bLBDown;
    BOOL                m_bShiftDown;
    CPoint              m_ptStart;
    CPoint              m_ptEnd;
};

static SeROIMouseTool	s_ROITool;
static SeMPRMouseTool   s_MPRTool;
static SeFloodFillTool  s_FloodDillTool;
// static SeAPRMouseTool   s_APRTool;
// static SeAPRRectMouseTool s_APRRecctTool;

static SeMeasureLineTool s_LineTool;
static SeMeasureAngleTool s_AngleTool;
static SeEllipseTool     s_EllipseTool;
static SeMeasureShapeTool s_ShapeTool;
static SeMeasureCTTool s_CTTool;
static SeMeasureAreaTool s_AreaTool;
static SeMeasureSelectTool s_SelectTool;