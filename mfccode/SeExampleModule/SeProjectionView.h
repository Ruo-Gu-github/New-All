#pragma once
#define CLAMP(x, low, high) ((x) < (low) ? (low) : ((x) > (high) ? (high) : (x)))


// CSeFattyZSelectView 视图

class CSeFattyProjectionView : public CImageViewerView
{
	DECLARE_DYNCREATE(CSeFattyProjectionView)

protected:
	CSeFattyProjectionView();           // 动态创建所使用的受保护的构造函数
	virtual ~CSeFattyProjectionView();

public:
	CDcmPicArray*	m_pDcmPicArray;

private:
	bool	m_bMPR;
	int		m_nPlaneNum;
	int		m_nStartPos;
	int     m_nEndPos;
	int		m_nWidthZ;
	int		m_nHeightZ;
	int		m_nOffSet;
	CRect   m_imageRect;  // 映射后的图像区域（居中）
	bool    m_bDraggingRed;
	bool    m_bDraggingGreen;
	CPoint  m_lastMousePoint;
	int LINE_HIT_THRESHOLD;

public:
	void	SetDcmArray(CDcmPicArray* pDcmPicArray);
	const  void     Reset();
	int     ImageYToClientY(int y);
	int     ClientYToImageY(int y);
	BOOL	IsNearLine(int clientY, int lineY);
	void	UpdateImageRect();
	int     GetStartPos() {return m_nHeightZ - m_nEndPos;}
	int     GetEndPos() {return m_nHeightZ - m_nStartPos;}
	
	//void	UpdateImage();
	//void	SetMPRMode(bool	bMPR);
	//void	SetPlaneNum(int nPlaneNum);
	//void	SetStartEnd(int nStartOrEnd);
	//void	CreateDcmPic();
	//void	RefreshZRegion();
	///////////////////////////////////////////////////////

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
	virtual void OnInitialUpdate();
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
};


