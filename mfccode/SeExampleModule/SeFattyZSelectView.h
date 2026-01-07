#pragma once
class CSeROIData;

// CSeFattyZSelectView 视图

class CSeFattyZSelectView : public CImageViewerView
{
	DECLARE_DYNCREATE(CSeFattyZSelectView)

protected:
	CSeFattyZSelectView();           // 动态创建所使用的受保护的构造函数
	virtual ~CSeFattyZSelectView();

public:
	CDcmPicArray*	m_pDcmPicArray;

private:
	bool	m_bMPR;
	int		m_nPlaneNum;
	int		m_nStartEnd;
	int		m_nWidth;
	int		m_nHeight;
	int		m_nOffSet;

public:
	void	SetDcmArray(CDcmPicArray* pDcmPicArray);
	void    Reset();
	void	UpdateImage();
	void	UpdateImage(int nPos);
	void	SetMPRMode(bool	bMPR);
	void	SetPlaneNum(int nPlaneNum);
	void	SetStartEnd(int nStartOrEnd);
	void    SetPicCount(int nCount){m_nPicCount = nCount;}
	void    SetCurrentFrame(int nCurrent){m_nCurrentFrame = nCurrent;}
	static Bitmap*		CreatePng(int nPlane, int nLayer, CSeROIData* data, int nWidth, int nHeight);
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

	SeVisualMPR			m_SeVisualMPR;

	virtual void OnInitialUpdate();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);


	Bitmap*				GreatePng(CImageBase* pImg, int nMin, int nMax, COLORREF color, DWORD alpha);
	void                SetInfo(int n1, int n2, DWORD n3, int n4, int n5);
	const  void         SetSelect(int n) { m_nSelect = n;}
	int m_nPicCount;
	int m_nCurrentFrame;
	static vector<CSeROIData*> m_vecROIData;


	int                 m_nSelect;
	int                 m_nFatMin;
	int                 m_nFatMax;
	int                 m_nLungMin;
	int                 m_nLungMax;
	int                 m_nBoneMin;
	int                 m_nBoneMax;
	int                 m_nNowPos;

	void                SeprateFat();
	void                SeprateLung();

	CSeROIData*         m_pFatInside;
	CSeROIData*         m_pFatOutside;
	CSeROIData*         m_pLungInside;
	CSeROIData*         m_pLungOutside;
	CSeROIData*         m_pLung;
	CSeROIData*         m_pBone;
	

	void               ExpertFats();
};


