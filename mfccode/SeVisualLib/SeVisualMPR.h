#pragma once


class SEV_EXT_CLASS SeVisualMPR
{
public:
	SeVisualMPR();
	~SeVisualMPR();

public:
	//void		SetCuttingPlane(int nCuttingPlane);
	void		SetPlaneNum(int nPlaneNum);
	int			GetPlaneNum();

	void		SetDcmArray(CDcmPicArray* pArray);
	void		SetDcmArray(CDcmPicArray* pArray, short** pChipArray, int nOriWidth, int nOriHeight, int nOriPiece, int nZPixelNum, TPixelDataType DataType);
	void        SetWinLevel(int nWinCenter, int nWinwidth);
	CDcmPicArray*	GetDcmArray();
	short**		GetChipArray();
	bool		GetChipArrayBool();
	int			GetMPRPosition();

	void		MoveCuttingPlane(int nDis);
	void		ResetCuttingPlane();

	CDcmPic*	GetMPRImage();
	CDcmPic*	GetMPRImage(int nPlanePos);

	int			GetMPRWidth();
	int			GetMPRHeight();
	int			GetMPRPiece();
	int			GetCuttingPlaneWidth();
	int			GetCuttingPlaneHeight();
	Vector3D*	GetCuttingPlanePoint();

	void		Reset();

	void        Smooth();

	int         GetWinWidth(){return m_nWinWidth;}
	int         GetWinCenter(){return m_nWinCenter;}


private:
	void		SetCuttingPlane( bool bDcmArray = true);
	void		ChangeCuttingPlane();
	void			LinearInterpolation(int i);
	static	UINT  __LinearInterpolation( void* lpVoid );

public:

	
private:
	Vector3D*			m_pCuttingPlanePoint;
//	Vector3D*			m_pCuttingPlanePointOri;
	short*				m_sCuttingPlaneData;
	short**				m_sChipArray;
	int					m_nPlaneNum;
	
	CDcmPicArray*		m_pDcmArray;

	int					m_nOriImageWidth;
	int					m_nOriImageHeight;
	int					m_nOriImagePiece;
	int					m_nZPixelNum;
	TPixelDataType		m_DataType;
	double				m_dPixelperPiece;

	int					m_nWidth;
	int					m_nHeight;
	int					m_nPiece;
	int					m_nCuttingPlaneWidth;
	int					m_nCuttingPlaneHeight;
	bool				m_bDeleteChipArray;
	int					m_nCuttingPlanePos;

	HANDLE			m_MPRStartThread;
	HANDLE			m_hDrawMPR;


	bool			m_MPRThreadEnd[20];

	static int            m_nWinWidth;
	static int            m_nWinCenter;
};

