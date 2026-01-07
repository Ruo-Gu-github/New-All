#pragma once

class CSeROIData;
class CMeasurement;
// SeMPRView 视图

struct MaskInfo
{
	int					nMin;
	int					nMax;
	COLORREF			color;
	DWORD				alpha;
	BOOL                bShow;

	MaskInfo(int nMinValue, int nMaxValue, COLORREF colorValue, DWORD alphaValue, BOOL bShowHit)
	{
		nMin = nMinValue;
		nMax = nMaxValue;
		color = colorValue;
		alpha = alphaValue;
		bShow = bShowHit;
	}
};


struct VolTexInfo
{
	int nWidth;
	int nHelght;
	int nLength;
	BYTE* pData;
};

struct CutInfo
{
	float nXstart;
	float nXend;
	float nYstart;
	float nYend;
	float nZstart;
	float nZend;
};


class SeMPRView : public CImageViewerView
{
	DECLARE_DYNCREATE(SeMPRView)

protected:
	SeMPRView();           // 动态创建所使用的受保护的构造函数
	virtual ~SeMPRView();

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
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);

public:
	void				SetDcmArray(CDcmPicArray* pDcmArray);
	void			    SetPicCount(int nCount){m_nPicCount = nCount;}
	void				SetCurrentFrame(int nCurrent){m_nCurrentFrame = nCurrent;}
	void				SetPlaneNum(int nPlaneNum);
	int					GetPlaneNum();
	void				Reset();


	void				UpdateImage();
	void				UpdateImage(int nPos);

	void				DrawImage(CDcmPic* pDcm);
	void				SetMPRTool();
	void				SetViewPos(CPoint pt);
	void                ResetVScrollBar();
	void                SetWinLevel(int nWinCenter, int nWinWidth);


	static  UINT	    __UpdateImage(void* lpVoid);
	HANDLE				m_hDrawDcm;
	HANDLE				m_hReleaseDcm;
	CEvent				m_eventChange;
	CEvent				m_eventExit;


private:
	int                 m_sMinValue;
	int					m_nPlaneNum;
	int					m_nOffSet;

public:
	BOOL                m_bNewMask;
	int					m_nPicCount;
	int					m_nCurrentFrame;


	static int			m_nXPos;
	static int          m_nYPos;
	static int          m_nZPos;

	SeVisualMPR				 m_SeVisualMPR;
	static vector<CSeROIData*> m_vecROIData;

private:
	static CDcmPicArray* m_pDcmArray;
	static int			m_nMin;
	static int			m_nMax;
	static COLORREF		m_color;
	static DWORD		m_alpha;

	static int          m_nOriImagePiece;
	static int			m_nOriImageWidth;
	static int			m_nOriImageHeight;
	static int          m_nRoiShape;

	bool                IsProjectionActive() const;
	int                 NormalizeSliceIndex(int index) const;
	static UINT AFX_CDECL ProjectionWorkerProc(LPVOID lpParam);
	CDcmPic*            CreateProjectionImage(int requestedIndex);

public:
	static BOOL         m_bShowMask;
	enum { PROJECTION_NONE = 0, PROJECTION_MIP, PROJECTION_MINIP };
	static int          m_nMipThickness;
	static int          m_nMinIpThickness;
	static int          m_eProjectionMode;
	static void         ClampProjectionRange(int center, int thickness, int limit, int& start, int& end);
	void                ApplyProjection(CDcmPic* pDcm);
	void                ApplyProjectionAlongZ(CDcmPic* pDcm, int thickness, bool useMip);
	void                ApplyProjectionAlongX(CDcmPic* pDcm, int thickness, bool useMip);
	void                ApplyProjectionAlongY(CDcmPic* pDcm, int thickness, bool useMip);

public:
	static void         SetMipThickness(int thickness);
	static void         SetMinIpThickness(int thickness);
	static int          GetMipThickness();
	static int          GetMinIpThickness();
	static void         RefreshAllViews();
	static int          GetRoiShape();
	static void         SetRoiShape(int nShape);



public:
	static void			ConvertDcms2VolTex(MaskInfo info);
	static Bitmap*		CreatePng(int nPlane, int nLayer, CSeROIData* data, int nWidth, int nHeight);
	static VolTexInfo   ConvertDcms2VolTex(int nRow);
	static Bitmap*		GreatePng(CImageBase* pImg, int nMin, int nMax, COLORREF color, DWORD alpha);
	static void         SetPngInfo(int nMin, int nMax, COLORREF color, DWORD alpha);
	static int          GetOriImagePiece(){return m_nOriImagePiece;} 
	static void         GetBasicInfo(int nRow);





// For ROIEdit
public:
	vector<CPoint>*		m_pVecEdge;
	vector<int>         m_vecPoid;
	int                 m_nVecEageSize;

	void				SetRoiTool();
	void				SetRectRoiTool();
	void				DeleteROI();     
	void				FillMidLayer();
	int                 GetAxialPos();
	void                ROI(int nPos, ROI_OPERATION op);
	void                ClipAllImage();
	void				ClipAllImage( CRect rect, int nStartPiece, int nEndPiece );
	void				SaveAllImage();

private:
	void				Interpolate( int nStart, int nEnd, vector <CPoint>* pVecEdge );
	void				GetInfo( vector <CPoint> pts );
	void				CreateFolder( CString csPath );
	CRect				GetROIRect(vector <int> vecPoid);

	CScopeTool          m_ScopeTool;

	float               m_fROIArea;
	float               m_fROIVolume;
	float               m_fAverageCTValue;
	float				m_fVariance;
	CRect               m_rtROI;
	int                 m_nPiece;

// For FloodFill
private:
	CPoint              m_ptPosition; 

public:
	void                SetPosition(CPoint pt){m_ptPosition = pt;}
	void                FloodFill();


// For MeasureTool
private:
	static vector<CMeasurement*> m_vecRst;
	void				DrawLine(vector <CPoint> vecPts, Color color, int nWidth, Graphics* gc, CDcmPic* pDcm);
	void				DrawRegion(vector <CPoint> vecPts, SolidBrush brush, Graphics* gc, CDcmPic* pDcm);

public:
	static CWnd*        m_wndResult;
	void                AddResult(CMeasurement* pMeasurement);
	void                CleanResult(CMeasurement* pMeasurement);
	void                UpdateResult(int nPos);
	void                SetSelectTool();
	static vector<CMeasurement*>&  GetResult(){return m_vecRst;}
	static void         MoveToToolPos(int nPos);
	void                UpdateView();


// export images
public:
	void                ExportImages(BYTE* pMask);
	void                ExportImagesWithMask();
	void                ExportCurrentImageWithMask();
	void                ExportFreeCutImages(float xStart, float xEnd, float yStart, float yEnd, float zStart, float zEnd, glm::mat4 mat1, glm::mat4 mat2, glm::mat4 mat3, glm::mat4 mat4);

private:
	void               Dcm2RGB(BYTE* pBmpData, BYTE* pDcmData, int nWidth, int bmpLineWidth, int nHeight, int nW, int nL);
	void               ReplaceRGB(BYTE* pBmpData, BYTE* pMaskData, int nWidth, int bmpLineWidth, int nHeight, Color color);
	void               UpDownReplace(BYTE* pSrc, int nWidth, int nHeight);

	void			   SaveBitmapToFile( BYTE* pBitmapBits,  LONG lWidth,  LONG lHeight,  WORD wBitsPerPixel, const unsigned long& padding_size, LPCTSTR lpszFileName );  
	BYTE*			   LoadBMP ( int* width, int* height, unsigned long* size, LPCTSTR bmpfile );
	std::unique_ptr<BYTE[]> CreateNewBuffer( unsigned long& padding, BYTE* pmatrix, const int& width, const int& height );

	// unused 
	void				CreateDcmPic();
	static BYTE*        ConvertOneDcm(CImageBase* pImg, MaskInfo info);

	void                Dcm2Byte(BYTE* pBmpData, BYTE* pDcmData, int nWidth, int nHeight, int nW, int nL);


public:
	// save & load mask
	void                ExportMask(BYTE* pMask);
	void                LoadMask(CString strFileName, MaskInfo info);

// for sharp image and export
private:
	BOOL                m_bMbtnDown;
	static float        m_fSharp;
	CPoint              m_ptOld;
	void				Sharp(CDcmPic* pImg, float fSharp);
public:
	void                ExportSharpImages();
	

private:
	BYTE*               m_pColorImage;
public:
	// multiply thread free cut caculator
	void MultiThreadFreecut(float xStart, float xEnd, float yStart, float yEnd, float zStart, float zEnd, glm::mat4 mat1, glm::mat4 mat2, glm::mat4 mat3, glm::mat4 mat4);
private:
	void __Freecut(const int nStart, const int nSize, const CString folder, const CutInfo info, glm::mat4 matrix_total);
public:
};




