#pragma once


struct SeedPosition
{
	int nXpos;
	int nYpos;
	int nZpos;
	SeedPosition(int x, int y, int z)
	{
		nXpos = x;
		nYpos = y;
		nZpos = z;
	}

	SeedPosition Offset(int nx, int ny, int nz)
	{
		return SeedPosition(nXpos + nx, nYpos + ny, nZpos + nz);
	}
};

struct DcmInfo
{
	CDcmPic* pDcm;
	int      nMark;

	DcmInfo(CDcmPic* p, int n)
	{
		pDcm = p;
		nMark = n;
	}
};

struct ThreadInfo_0
{
	queue <DcmInfo> infoList;
	BYTE*           pData;
	int             nWidth;
	int             nHeight;
	int             nLength;
	int             nLow;
	int             nHigh;
	int             nThreadAlive;
	ThreadInfo_0(queue <DcmInfo> info, BYTE* p, int n0, int n1, int n2, int n3, int n4, int n5)
	{
		infoList = info;
		pData = p;
		nWidth = n0;
		nHeight = n1;
		nLength = n2;
		nLow    = n3;
		nHigh   = n4;
		nThreadAlive = n5;
	}
};

struct ThreadInfo_1
{
	BYTE*                 pData;
	int                   nWidth;
	int                   nHeight;
	int                   nLength;
	int                   nKernel;
	int                   nThreadAlive;
	ThreadInfo_1(BYTE* p, int n0, int n1, int n2, int n3, int n4)
	{
		pData    = p;
		nWidth   = n0;
		nHeight  = n1;
		nLength  = n2;
		nKernel  = n3;
		nThreadAlive = n4;
	}
};


class CSeROIData
{
public:
	CSeROIData(void);
	~CSeROIData(void);

	CSeROIData(int nWidth, int nHeight, int nLength, CDcmPicArray* pDcmArray, int nLow, int nHigh, COLORREF color, DWORD alpha, BOOL bShow = TRUE);
	CSeROIData(CSeROIData* pMaskA, CSeROIData* pMaskB, BOOLEAN_OPERATION operation, COLORREF color, DWORD alpha, BOOL bShow = TRUE);
	CSeROIData(CSeROIData* pSrc, COLORREF color, DWORD alpha, BOOL bShow = TRUE);
	CSeROIData(CString strFile, int nWidth, int nHeight, int nLength, COLORREF color, DWORD alpha, BOOL bShow = TRUE);

	BYTE* GetSliceData(int nLayer, int nPlane);
	void  SetSliceData(BYTE* pSliceData, int nLayer, int nPlane);

	// 3 维方向 形态学 未开放
	void Close(int nKernel);
	void Open(int nKernel);
	void Corrosion(int nKernel);
	void Dilate(int nKernel);

	// 2 维方向 形态学 目前只使用 nPlane == 1;
	void Close(int nKernel, int nPlane);
	void Open(int nKernel, int nPlane);
	void Corrosion(int nKernel, int nPlane);
	void Dilate(int nKernel, int nPlane);

	void FloodFill( int nXpos, int nYpos, int nZpos);
	void Inverse();

	void ROI(vector <CPoint>* pVecEdge, int nPlane, ROI_OPERATION op = ROI_ADD);

	
	const int GetWidth(){return m_nWidth;}
	const int GetHeight(){return m_nHeight;}
	const int GetLength(){return m_nLength;}
	const COLORREF GetColor(){return m_color;}
	const DWORD GetAlpha(){return m_alpha;}
	const BOOL IsVisible(){return m_bShow;}
	BYTE* GetData(){return m_pData;}

	const void SetVisible(BOOL bShow){m_bShow = bShow;}
	const void ChangeColor(COLORREF color){m_color = color;}
	const void SetAlpha(DWORD nAlpha){m_alpha = nAlpha;}

	
private:
	LONGLONG m_nWidth;
	LONGLONG m_nHeight;
	LONGLONG m_nLength;
	COLORREF m_color;
	DWORD  m_alpha;
	BOOL m_bShow;
	BYTE* m_pData;

	CSeROIData* m_pFloodFillSrc;
	BYTE* m_pSliceData;
	inline void SearchOnePoint(queue <SeedPosition>& vecSeeds, SeedPosition seed, BYTE* pData, BYTE* pUsed);
	SeedPosition GetRealPosition(SeedPosition pos, int nPlane);

	enum POINTSTATE{EMPTY = 0, IN_QUEUE, ADDED_TO_REGION};

//  多线程相关
	static UINT                 __ConvertImagesWay0(LPVOID pParam);
	static UINT                 __Corrosion(LPVOID pParam);
	static UINT                 __Dilate(LPVOID pParam); 

	static void Dilate(BYTE *pData, int nWidth, int nHeight, int nKernel);
	static void Corrosion(BYTE *pData, int nWidth, int nHeight, int nKerne);

};

