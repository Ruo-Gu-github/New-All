#pragma once
#define  CUDA_AVAILABLE
#ifdef CUDA_AVAILABLE

#define SUPPORT_OLDEST_CUDA_VISION  8.0

class SEV_EXT_CLASS SeVisualAPR_with_CUDA
{
public:
	SeVisualAPR_with_CUDA(void);
	~SeVisualAPR_with_CUDA(void);

	void		SetDcmArray(CDcmPicArray* pArray);
	CDcmPic*	GetAPRImage(double dRotateMatrix[16], double dCenterX, double dCenterY, double dCenterZ, bool b=FALSE);
	void		Reset();
	void        SetMinValue(short sMin){m_sMinValue = sMin;}
	BOOL        IsCUDASupport(){return m_bCUDASupport;}
	void        SetWinLevel(int nWinCenter, int nWinHeight);

private:
	inline void ChangeMatrix(double dRotateMatrix[16], double dCenterX, double dCenterY, double dCenterZ);
	void        InitPlanePos();

	BOOL        IsCUDAAvailable();
	BOOL        InitCUDAMemory();
	BOOL        ReleaseCUDAMemory();
	BOOL		InterpolationByCUDA();

	short* 		InterpolationByCPU();
	void        GetTransfromedPos(int* srcPos, float* rstPos, float* matrix, float* pos, int rstWidth, int rstHeight, float pixelScale);
	void        CaculateRstData(short** srcData, short* rstData, float* rstPos, int rstWidth, int rstHeight, int width, int height, int length, short min, float pixelScale);


private:
	short**   m_pOriginalData;
	short*    m_pResultData;

	CDcmPic*  m_pDcmInfo;

	// 每个位置是两个 int
	int*      m_pSourcePosition;

	// 载入原图像的长宽高
	int       m_nOriWidth;
	int       m_nOriHeight;
	int       m_nOriLength;

	// pixelSpacing & pixelSize
	float    m_fPixelSpacing;
	float    m_fPixelSize;

	short     m_sMinValue;

	// 计算后 APR 图像的长宽
	int       m_nRstWidth;
	int       m_nRstHeight;

	// 窗宽窗位
	static int       m_nWinCenter;
	static int       m_nWinWidth;

	// 图像的中心点位置
	float    m_dPos[3];


	// 旋转 4元数 矩阵
	float    m_dRotateMatrix[16];

	BOOL      m_bCUDASupport;

	LONGLONG  m_lDataSize;

	static BOOL      m_bInit;

	static LONGLONG  m_lMaxDataSize;


// 	float *host_vec_out, float *host_vec_in, short** host_data_in, float *host_matrix_in, int *host_vec_in_2, 
// 	float *dev_vec_out, float *dev_vec_in, short* dev_data_in, float *dev_matrix_in, int *dev_vec_in_2, 
// 	int imageSize, int slice, int sliceSize, int* pitch_out

	// cuda device memory point
	static int* dev_vec_in_2;
	static float* dev_matrix_in;
	static short* dev_vec_out;
	static float* dev_vec_in;
	static short* dev_data_in;

	static int* host_vec_in_2;
	static float* host_matrix_in;
	static short* host_vec_out;
	static float* host_vec_in;
	static short** host_data_in;

	static int imagesize;
	static int slice;
	static int sliceSize;
	static int pitch_out;
};
#endif //CUDA_AVAILABLE 
