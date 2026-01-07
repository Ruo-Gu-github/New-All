#pragma once

#ifndef min_Value
#define  choosemin(a,b) (((a)>(b))?(b):(a))
#endif


struct ThreadInfo_ForBoneThickness
{
	int nThreadNum;
	int nCurrentThread;
	int nWidth;
	int nHeight;
	int nSize;
	int nMin;
	int nMax;
	BOOL bFinished;
	BOOL bInverse;
	CDcmPicArray* pDcmArray;
	FLOAT** s;

	ThreadInfo_ForBoneThickness(int n1, int n2, int n3, int n4, int n5, int n6, int n7, BOOL b, BOOL b2, CDcmPicArray*p, FLOAT** s0)
	{
		nThreadNum = n1;
		nCurrentThread = n2;
		nWidth = n3;
		nHeight = n4;
		nSize = n5;
		nMin = n6;
		nMax = n7;
		bInverse = b;
		bFinished = b2;
		pDcmArray = p;
		s = s0;
	}
};

struct ThreadInfo_ForBoneThickness2
{
	int nThreadNum;
	int nCurrentThread;
	int nWidth;
	int nHeight;
	int nSize;
	FLOAT** s;
	BOOL bFinished;
	ThreadInfo_ForBoneThickness2(int n1, int n2, int n3, int n4, int n5, FLOAT** s0, BOOL b)
	{
		nThreadNum = n1;
		nCurrentThread = n2;
		nWidth = n3;
		nHeight = n4;
		nSize = n5;
		s = s0;
		bFinished = b;
	}
};

struct ThreadInfo_ForBoneThickness3
{
	int nThreadNum;
	int nCurrentThread;
	int nWidth;
	int nHeight;
	int nSize;
	FLOAT** s;
	int* nRidge;
	int** iRidge; 
	int** jRidge;
	FLOAT** rRidge;
	BOOL bFinished;

	ThreadInfo_ForBoneThickness3(int n1, int n2, int n3, int n4, int n5, FLOAT** ps1, int* pn1, int **pn2, int** pn3, FLOAT** ps2, BOOL b)
	{
		nThreadNum = n1;
		nCurrentThread = n2;
		nWidth = n3;
		nHeight = n4;
		nSize = n5;
		s = ps1;
		nRidge = pn1;
		iRidge = pn2;
		jRidge = pn3;
		rRidge = ps2;
		bFinished = b;
	}
};


class SeBoneThickness
{
public:
	SeBoneThickness(void);
	SeBoneThickness(CDcmPicArray* p, int nMinValue, int nMin, int nMax, int nWidth, int nHeight, int nSize);
	~SeBoneThickness(void);

// public:
// 	typedef struct ParamThickness
// 	{
// 		int nThread;
// 		int* nRidge;
// 		int** iRidge;
// 		int** jRidge;
// 		float** rRidge;
// 		float** Edtdate;
// 		SeBoneThickness* pSeBone;
// 		CRITICAL_SECTION*	pCriticalLock;
// 	}PrameterThick;
// 
// 	CRITICAL_SECTION pLock;

// public:
// 	bool*		m_edgePoints;
// 	float**		EDT(BOOL bSpace = FALSE); 
// 	double		BoneJThickness(float** s);
// 	void		DistanceMaptoDistanceRidge(float** s);
// 	void		DistanceRidgetoLocalThickness(float** s);
// 	float*		CleanedUpLocalThickness(float** s);
// 	int**       createTemplate(int* distSqValues);
// 	int*		scanCube(int dx,int dy, int dz,int* distSqValues);
// 	float		setFlag(float** s, int i, int j, int k, int w, int h, int d);
// 	float		look(float** s, int i, int j, int k, int w, int h, int d);
// 	float		averageInteriorNeighbors(float** s, int i, int j, int k, int w,int h, int d);
// 	float		lookNew(int i, int j, int k, int w, int h, int d);

private:
	CDcmPicArray*       m_pProcessArray;
	int                 m_nMinValue;
	int                 m_nMin;
	int                 m_nMax;
	int                 m_nWidth;
	int                 m_nHeight;
	int                 m_nOriImagePiece;
// 
// 	int*				m_eulerLUT;
// 	int*				m_sumEulerInt;
// 	bool**				m_pImage;
// 
// 	static UINT __BonejThicknessThread(LPVOID pParam);
// 
// 
// 	int			m_numRadii;
// 	float		m_average;
// 	float**		m_sNew;

public:
	FLOAT*	GetLocalThickness(BOOL bInverse);

private:
	FLOAT** GeometryToDistanceMap(BOOL bInverse);
	void    DistanceMaptoDistanceRidge(FLOAT** s);
	void    DistanceRidgetoLocalThickness(FLOAT** s);
	FLOAT*  LocalThicknesstoCleanedUpLocalThickness(FLOAT** s);
	FLOAT*  MeanStdDev(FLOAT* sNew);

	inline int**    CreateTemplate(int* distSqValues, const int size);
	inline int*     ScanCube(const int dx, const int dy, const int dz, int* distSqValues, const int size);
	inline FLOAT	SetFlag(float** s, const int i, const int j, const int k, const int w, const int h, const int d);
	inline FLOAT	Look(float** s, const int i, const int j, const int k, const int w, const int h, const int d);
	inline FLOAT    AverageInteriorNeighbors(FLOAT** s, FLOAT* sNew, const int i, const int j, const int k, const int w, const int h, const int d);
	inline FLOAT    LookNew(FLOAT* sNew, const int i, const int j, const int k, const int w, const int h, const int d);


public:
	static UINT __Step1Thread(LPVOID pParam);
	static UINT __Step2Thread(LPVOID pParam);
	static UINT __Step3Thread(LPVOID pParam);

	static UINT __LTThread(LPVOID pParam);
};

