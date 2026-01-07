#pragma once
    
class SeBoneAnalysisView;

class SeBoneConnectivity
{
public:
	SeBoneConnectivity(void);
	SeBoneConnectivity(CDcmPicArray* p, int nMinValue, int nMin, int nMax, int nWidth, int nHeight, int nSize);
	~SeBoneConnectivity(void);
public:
	typedef struct ParamConnect
	{
		int nThread;
		SeBoneConnectivity* pSeBone;
	  //CRITICAL_SECTION*	pCriticalLock;
	}ParamConnectivity;

	//CRITICAL_SECTION pLock;

public:
	double		getSumEuler();
	static UINT _threadGetEuler(LPVOID pVoid);
	void		fillEulerLUT2( int* LUT );
	int*		getOctant( int x, int y, int z );
	int			getPixel( int x, int y, int z );
	int			getDeltaEuler( int* octant, int* LUT );
	double		getDeltaChi( double sumEuler );
	double		correctForEdges();
	long		getEdgeVertices();
	long		getFaceEdges();
	long		getFaceVertices();
	long		getStackFaces();
	long		getStackEdges();
	long		getStackVertices();

private:
	int*				m_eulerLUT;
	int*				m_sumEulerInt;
	CDcmPicArray*       m_pProcessArray;
	int                 m_nMinValue;
	int                 m_nMin;
	int                 m_nMax;
	int                 m_nWidth;
	int                 m_nHeight;
	int                 m_nOriImagePiece;
};
 