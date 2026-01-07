#pragma once

#define nTypeSquare 0
#define nTypeCircle 1


// SeROIView 视图

class SeROIView : public CImageViewerView
{
	DECLARE_DYNCREATE(SeROIView)

protected:
	SeROIView();           // 动态创建所使用的受保护的构造函数
	virtual ~SeROIView();

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
	CDcmPicArray*		m_pDcmPicArray;
	CDcmPicArray*		m_pDcmPicArrayMask;
	CDcmPicArray*		m_pDcmPicArrayOutMask;
	void				SetDcmArray(CDcmPicArray* pDcmArray);
	void				SetDcmArrayMask(CDcmPicArray* pDcmPicArrayMask);
	void				SetDcmArrayOutMask(CDcmPicArray* pDcmPicArrayOutMask);

public:
	vector<CPoint>*		m_pVecEdgeInside;
	vector<CPoint>*		m_pVecEdge;
	vector<int>         m_vecPoidInside;
	vector<int>         m_vecPoid;


	void                FillMidLayer();
	void                InitROIPts();
	void                ReleaseROIPts();
	void                ResetROIPts();
	void                OnExportImage();
	const  BOOL         IsClipOutside(){return m_bClipOutside;}
	const  void         SetClipOutside(BOOL b){m_bClipOutside = b;}
	const  BOOL         IsEmptyROI();
	const  void         Reset();
	              
	const  void			SetShowNewMask(BOOL b){m_bNewMask = b;}
	const  void			SetShowNewROIMask(BOOL b){m_bNewROIMask = b;}
	const  void			SetShowNewROIOutMask(BOOL b){m_bNewROIOutMask = b;}
	const  void			SetInfo(int n1, int n2, DWORD n3, int n4){m_nMin = n1; m_nMax = n2; m_color = n3, m_alpha = n4;}

	CDcmPicArray*		GetDcmArray(){return m_pDcmPicArray;}
	const  int          GetMin(){return m_nMin;}
	const  int          GetMax(){return m_nMax;}
	const  bool			GetShowNewROIOutMask(){return m_bNewROIOutMask;}
	const  bool			GetShowNewROIMask(){return m_bNewROIMask;}

	//松质骨
	void				BinaryzationAllBone(CDcmPicArray* pDcmPicArray,CDcmPicArray* pDcmPicArray_Mask);//,CDcmPicArray* pDcmPicArray_ROI);
	void				ReverseAllBone(CDcmPicArray* pDcmPicArray_Mask);
	void				FloodfillAllBone(CDcmPicArray* pDcmPicArray_Mask,CDcmPicArray* pDcmPicArray_MaskOut);
	void				FillHoleAllBone(CDcmPicArray* pDcmPicArray);
	void				GetHoleAllBone(CDcmPicArray* pDcmPicArray);
	void				OnlyGetMaxArea(CDcmPicArray* pDcmPicArray_Mask,CDcmPicArray* pDcmPicArray_MaskOut);
	void				floodFillScanline(short* pData, int x, int y, int newColor, int oldColor, int nWidth, int nHeight);
	void				BoneSegCorrosion(CDcmPicArray* pDcmPicArray_Mask, int nType, int nKernelSize);
	void				BoneSegInflation(CDcmPicArray* pDcmPicArray_Mask, int nType, int nKernelSize);
	void				BongSegOpen(CDcmPicArray* pDcmPicArray_Mask, int nType, int nKernelSize);
	void				BongSegClose(CDcmPicArray* pDcmPicArray_Mask, int nType, int nKernelSize);

	void				BoneSegDicomPicArrayClone(CDcmPicArray* pDcmPicArray_In,CDcmPicArray* pDcmPicArray_Out);

	void				BoneGetMaskOuter(CDcmPicArray* pDcmPicArray_Mask, CDcmPicArray* pDcmPicArray_Out);



	void				ShowTrabecular(CDcmPicArray* pDcmPicArray_Ori,CDcmPicArray* pDcmPicArray_Out,CDcmPicArray* pDcmPicArray_Mask);
	


	void				MaxAreaAllBone(CDcmPicArray* pDcmPicArray_Mask,CDcmPicArray* pDcmPicArray_ROI);
	void				BoneOpenF(CDcmPicArray* pDcmPicArray_Mask, CString str, int nType);
	void				BoneOpenE(CDcmPicArray* pDcmPicArray_Mask, CString str, int nType);

	
	bool				IsSame(int* pData1,int* pData2,int nlength);
	int					CalAddPoints(int* pData1,int* pData2,int nlength);
	void				Corrosion(int *pData, int nWidth, int nHeight,int kernelsize, int nType, int nCore);
	void				Corrosion(short *pData, int nWidth, int nHeight,int kernelsize, int nType, int nCore);
	void				Inflation(short *pData, int nWidth, int nHeight,int kernelsize, int nType,  int nCore);
	void				Inflation(int *pData, int nWidth, int nHeight,int kernelsize, int nType,  int nCore);
	void				BitwithAnd(int *pDataMask, short *pData, int *pDataOut,int nWidth, int nHeight);
	void				BitwithAnd(int *pDataMask, short *pData, short *pDataOut,int nWidth, int nHeight);
	void				ReverseMask(int *pDataMask, int nWidth, int nHeight);
	void				Mask2Data(int *pDataMask, short *pData, int nWidth, int nHeight);
	void				Data2Mask(int *pDataMask, short *pData, int nWidth, int nHeight);
	void				MaskCopy(int *pMask1,int *pMask1Copy,int nWidth, int nHeight);
	double				DistancePoints(int i1, int j1,int i2, int j2);
	void				shrink(int *pDataMask, int n, int nWidth, int nHeight);
	static void			shrink_look_up(int *img,int *res,int nWidth, int nHeight);
	static void			shrink_mask_and(int *img, int *m, int *sub, int nWidth, int nHeight);
	static void			shrink_copy_quarter_data(int *src, int *dst, int rs, int cs, int nWidth, int nHeight);

	

	

private:
	void				Interpolate( int nStart, int nEnd, vector <CPoint>* pVecEdge );
	CRect				GetROIRect(vector <int> vecPoid);

	int					Otsu(short *pData, int nWidth, int nHeight);

	Bitmap*				GreatePng(CImageBase* pImg, int nMin, int nMax, COLORREF color, DWORD alpha);
	Bitmap*				GreatePngWithMask(CImageBase* pImg, int PlanePos);
	//Bitmap*				GreatePngWithMaskOut(CImageBase* pImg, int PlanePos);
	
	CScopeTool          m_ScopeTool;
	CRect               m_rtROI;
	BOOL				m_bClipOutside;
	BOOL                m_bNewMask;
	BOOL				m_bNewROIMask;
	BOOL				m_bNewROIOutMask;

	int                 m_nMin;
	int                 m_nMax;
	DWORD               m_color;
	int                 m_alpha;

// 	vector<vector<int > > m_vecImgInt;
// 	vector<int >		m_vecImgLine;
};


