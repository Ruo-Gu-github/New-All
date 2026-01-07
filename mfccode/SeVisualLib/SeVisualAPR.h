#pragma once



class SEV_EXT_CLASS SeVisualAPR
{
public:
	SeVisualAPR();
	~SeVisualAPR();

	void		SetMipThick(int nThick);
	void		SetDcmArray(CDcmPicArray* pArray);

	CDcmPic*	GetAPRImage(double dRotateMatrix[16], double dCenterX, double dCenterY, double dCenterZ, bool bMipDownSampling);
	
	Vector3D	GetAPRNormal();
	
	double*		GetAPRAngle(int nPlaneNum);
	bool		GetParallel(int nPlaneNum);

	Vector3D*	GetCuttingPlanePoint();
	int			GetCuttingPlaneWidth();
	int			GetCuttingPlaneHeight(); 
	void		GetSeriesData(double SliceSpace, CDcmPicArray* pDcmArray);
	
	void		Reset();
	void		SetDIProtateDown(int DownSalple);
	void		SetDIPInterplote(int Interpolation);
	void		RotateorTrans( bool bRotateorTrans );
	void		SetSlabMode(CString nMode); 

	void		SetPlaneNum(int nPlaneNum);
	int			GetPlaneNum();
	void		SetWinLevel(int nWinCenter, int nWinWidth);

private:
	void			ChangeCuttingPlane();
	void			SetCuttingPlane();
	void			ResetCuttingPlane();
	void			ComputeAPRNormal();
	void			Compute2DAPRLine();

	static UINT		__LinearInterpolation(void* lpVoid);
	void			CUDA_Interpolation();
	void			MIPLinearInterpolation(int i,int Interpolation);
	void			MinIPLinearInterpolation(int i,int Interpolation);
	void			AverageLinearInterpolation(int i,int Interpolation);

private:
	int				m_nOriImageWidth;
	int				m_nOriImageHeight;
	int				m_nOriImagePiece;
	int				m_nZPixelNum;
	int				m_nCuttingPlaneWidth;
	int				m_nCuttingPlaneHeight;
	double			m_dPixelperPiece;
	short			m_sInitValue;
	TPixelDataType	m_DataType;
	HANDLE			m_hStartThread;
	HANDLE			m_hDrawAPR;
	bool			m_bThreadEnd[20];
	bool			m_bMipDownSampling;
	bool			m_bRotateorTrans;

	CDcmPicArray*	m_pDcmArray;
	double			m_dRotateMatrix[16];

	double			m_dCenterX;
	double			m_dCenterY;
	double			m_dCenterZ;
	Vector3D*		m_pCuttingPlanePoint;
//	Vector3D*		m_pCuttingPlanePointOri;
	short*			m_sCuttingPlaneData;

	short**			m_sChipArray;

	double			m_dAngleXOY;
	double			m_dAngleYOZ;
	double			m_dAngleXOZ;

	
	float			m_fAPRNormal_X;
	float			m_fAPRNormal_Y;
	float			m_fAPRNormal_Z;

	int				m_nMIPPieceNum;
	int				m_nSampleNum;
	int				m_bMIPRotateDown;
	int				m_bInterplote;

	bool			m_bXOYParallel;
	bool			m_bYOZParallel;
	bool			m_bXOZParallel;

	CString			m_nSlabMode;
	int             m_nPlaneNum;

	// ´°¿í´°Î»
	static int       m_nWinCenter;
	static int       m_nWinWidth;
};

