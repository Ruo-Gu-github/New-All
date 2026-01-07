#include "StdAfx.h"
#include "SeVisualAPR_with_CUDA.h"

#ifdef CUDA_AVAILABLE


// all parameters used by cuda only have one copy
int* SeVisualAPR_with_CUDA::dev_vec_in_2 = 0;
float* SeVisualAPR_with_CUDA::dev_matrix_in = 0;
short* SeVisualAPR_with_CUDA::dev_vec_out = 0;
float* SeVisualAPR_with_CUDA::dev_vec_in = 0;
short* SeVisualAPR_with_CUDA::dev_data_in = 0;

int* SeVisualAPR_with_CUDA::host_vec_in_2 = 0;
float* SeVisualAPR_with_CUDA::host_matrix_in = 0;
short* SeVisualAPR_with_CUDA::host_vec_out = 0;
float* SeVisualAPR_with_CUDA::host_vec_in = 0;
short** SeVisualAPR_with_CUDA::host_data_in = 0;

int SeVisualAPR_with_CUDA::imagesize = 0;
int SeVisualAPR_with_CUDA::slice = 0;
int SeVisualAPR_with_CUDA::sliceSize = 0;
int SeVisualAPR_with_CUDA::pitch_out = 0;

BOOL SeVisualAPR_with_CUDA::m_bInit = FALSE;

LONGLONG  SeVisualAPR_with_CUDA::m_lMaxDataSize = 0;

int SeVisualAPR_with_CUDA::m_nWinCenter = 2048;
int SeVisualAPR_with_CUDA::m_nWinWidth = 4096;

extern "C"
	cudaError_t InterpolationWithCUDA(short *host_vec_out, float *host_matrix_in, float* host_vec_in, int *host_vec_in_2,
									short *dev_vec_out, float *dev_vec_in, short *dev_data_in, float *dev_matrix_in, int *dev_vec_in_2,
									int size, int slice, int sliceSize, int pitch_out, int width, int height, int length, int min);
extern "C"
	cudaError_t CopyDataToDevice(short *host_vec_out, float *host_vec_in, short** host_data_in, float *host_matrix_in, int *host_vec_in_2, 
									short *&dev_vec_out, float *&dev_vec_in, short *&dev_data_in, float *&dev_matrix_in, int *&dev_vec_in_2, 
									int imageSize, int slice, int sliceSize, int* pitch_out);
extern "C"
	cudaError_t ReleaseDataFromDevice(short *dev_vec_out, float *dev_vec_in, short* dev_data_in, float *dev_matrix_in, int *dev_vec_in_2);

SeVisualAPR_with_CUDA::SeVisualAPR_with_CUDA(void)
{
	m_pOriginalData = NULL;
	m_pResultData = NULL;
	m_pSourcePosition = NULL;
	m_pDcmInfo = NULL;
	m_bCUDASupport = FALSE;
	m_lMaxDataSize = 0;
	m_sMinValue = 0;
	for (int i=0; i<16; i++)
		m_dRotateMatrix[i] = 0.0;
	for (int i=0; i<3; i++)
		m_dPos[i] = 0.0;
	m_nRstWidth = 0;
	m_nRstHeight = 0;
	m_nOriWidth = 0;
	m_nOriHeight = 0;
	m_nOriLength = 0;
	m_lDataSize = 0;
}


SeVisualAPR_with_CUDA::~SeVisualAPR_with_CUDA(void)
{
	//Safe_Delete(m_pOriginalData);
	Safe_Delete(m_pResultData);
	Safe_Delete(m_pSourcePosition);
}

void SeVisualAPR_with_CUDA::SetDcmArray(CDcmPicArray* pArray)
{
	
	const int nSize = (int)pArray->GetDcmArray().size();
	assert(nSize > 0);
	CDcmPic* pDcm = pArray->GetDcmArray()[0];
	m_pDcmInfo = pArray->GetDcmArray()[0];
	m_nOriLength = nSize;
	m_nOriWidth = pDcm->GetWidth();
	m_nOriHeight = pDcm->GetHeight();
	m_fPixelSpacing = pDcm->GetSliceSpace();
	m_fPixelSize = pDcm->GetMMPerpixel();

	m_lDataSize = (LONGLONG)m_nOriHeight * m_nOriWidth * m_nOriLength * sizeof(short);

	// 只需要判断一次就好
	static BOOL b = IsCUDAAvailable();
	// 图像总大小最大支持显存的 80% 容量
	if (m_lDataSize > m_lMaxDataSize * 0.80 || !b)
		m_bCUDASupport = FALSE;
	else
		m_bCUDASupport = TRUE;
	assert(m_nOriWidth > 0 && m_nOriHeight > 0 && m_nOriLength > 0);
	int m_nScaleLength = static_cast<int>(m_nOriLength * m_fPixelSpacing / m_fPixelSize);
	m_nRstWidth = m_nRstHeight = sqrt(double(m_nOriHeight*m_nOriHeight + m_nOriHeight*m_nOriHeight + m_nScaleLength*m_nScaleLength));

	m_pResultData = new short[m_nRstWidth * m_nRstHeight];
	memset(m_pResultData, 0, sizeof(short) * m_nRstWidth * m_nRstHeight);

	m_pOriginalData = new short*[nSize];
	memset(m_pOriginalData, 0, sizeof(short*) * nSize);
	for (int i=0; i<nSize; i++)
	{
		m_pOriginalData[i] = (short*)pArray->GetDcmArray()[i]->GetData();
	}

	InitPlanePos();
	// only initialize at the first time 
	if (!m_bInit && m_bCUDASupport)
	{
		m_bCUDASupport = InitCUDAMemory();
		m_bInit = TRUE;
	}
	this;
}

void SeVisualAPR_with_CUDA::ChangeMatrix(double dRotateMatrix[16], double dCenterX, double dCenterY, double dCenterZ)
{
	for(int i=0; i<16; i++)
	{
		m_dRotateMatrix[i] = (float)dRotateMatrix[i]; 
	}
	m_dPos[0] = (float)dCenterX;
	m_dPos[1] = (float)dCenterY;
	m_dPos[2] = (float)dCenterZ;
}

CDcmPic* SeVisualAPR_with_CUDA::GetAPRImage(double dRotateMatrix[16], double dCenterX, double dCenterY, double dCenterZ, bool b)
{
	ChangeMatrix(dRotateMatrix, dCenterX, dCenterY, dCenterZ);
	short* pDataCpy = new short[m_nRstHeight * m_nRstWidth];
	memset(pDataCpy, 0, m_nRstHeight * m_nRstWidth * sizeof(short));
	// 不想写pixelSpacing等于pixelSize时候的插值CUDA算法,直接改CPU Version
	if (m_fPixelSize != m_fPixelSpacing)
	{
		m_bCUDASupport = FALSE;
	}
	if (m_bCUDASupport)
	{
		m_bCUDASupport = InterpolationByCUDA();
		memcpy(pDataCpy, host_vec_out, m_nRstHeight * m_nRstWidth * sizeof(short));
	}
	else
	{
		short* pRst = InterpolationByCPU();
		memcpy(pDataCpy, pRst, m_nRstHeight * m_nRstWidth * sizeof(short));
	}

	CDcmPic* pDcm = m_pDcmInfo->CloneDcmPic();
	pDcm->SetPixelData((BYTE*)pDataCpy, m_nRstWidth, m_nRstHeight); //注意：这里的数据区要求在外部copy，而不是单纯的指针
	pDcm->AdjustWin(m_nWinCenter, m_nWinWidth);
	return pDcm;
}

void SeVisualAPR_with_CUDA::Reset()
{
	Safe_Delete(m_pOriginalData);
	Safe_Delete(m_pResultData);
	m_pDcmInfo = NULL;
	m_dPos[0] = 0.0;
	m_dPos[1] = 0.0;
	m_dPos[2] = 0.0;
	for (int i=0; i<16; i++)
		m_dRotateMatrix[i] = 0.0;
	m_sMinValue = 0;

	m_nRstWidth = 0;
	m_nRstHeight = 0;
	m_nOriWidth = 0;
	m_nOriHeight = 0;
	m_nOriLength = 0;
	m_lDataSize = 0;
	if (m_bInit && m_bCUDASupport)
	{
		m_bCUDASupport = ReleaseCUDAMemory();
		m_bInit = FALSE;
	}
	m_bCUDASupport = FALSE;
}

BOOL SeVisualAPR_with_CUDA::IsCUDAAvailable()
{
	int nDeviceCount = 0;
	int nDriverVersion = 0;
	cudaError_t error_id = cudaGetDeviceCount(&nDeviceCount);
	// 读取显卡失败或者没有 Nvidia 显卡存在
	if (nDeviceCount == 0 || error_id != cudaSuccess)
		return FALSE;

	// 使用 设备 0，只使用一块显卡
	cudaSetDevice(0);
	cudaDeviceProp deviceProp;
	cudaGetDeviceProperties(&deviceProp, 0);
	cudaDriverGetVersion(&nDriverVersion);
	// 需要驱动支持 cuda 8.0 以上版本
	if (nDriverVersion <= 8000)
		return FALSE;
	m_lMaxDataSize = (LONGLONG)deviceProp.totalGlobalMem;

	error_id = cudaDeviceReset();
	if (error_id != cudaSuccess) {
		return FALSE;
	}

	return TRUE;
}

short* SeVisualAPR_with_CUDA::InterpolationByCPU()
{
	const int width = m_nOriWidth;
	const int height = m_nOriHeight;
	const int length = m_nOriLength;
	const int rstWidth = m_nRstWidth;
	const int rstHeight = m_nRstHeight;
	const float pixelScale = m_fPixelSpacing / m_fPixelSize;
	const int min = 0;
	float* matrix = &m_dRotateMatrix[0];
	float* pos = &m_dPos[0];
	int* srcPos = m_pSourcePosition;
	float* rstPos = new float[rstWidth * rstHeight * 3];
	short** srcData = m_pOriginalData;
	short* rstData = m_pResultData;

	memset(rstPos, 0, sizeof(short) * rstWidth * rstHeight);

	GetTransfromedPos(srcPos, rstPos, matrix, pos, rstWidth, rstHeight, pixelScale);
	CaculateRstData(srcData, rstData, rstPos, rstWidth, rstHeight, width, height, length, min, pixelScale);

	Safe_Delete(rstPos);
	return rstData;
}

BOOL SeVisualAPR_with_CUDA::InterpolationByCUDA()
{
	host_matrix_in = &m_dRotateMatrix[0];

	host_vec_in = &m_dPos[0];
	int width = m_nOriWidth;
	int height = m_nOriHeight;
	int length = m_nOriLength;
	int min = 0;
	// transform vectors in parallel.
	host_vec_in_2 = m_pSourcePosition;
	host_vec_out = m_pResultData;
	host_data_in;
	cudaError_t cudaStatus = InterpolationWithCUDA(host_vec_out, host_matrix_in, host_vec_in, host_vec_in_2,
												dev_vec_out, dev_vec_in, dev_data_in, dev_matrix_in, dev_vec_in_2,
												imagesize, slice, sliceSize, pitch_out, width, height, length, min);
	if (cudaStatus != cudaSuccess) {
		return FALSE;
	}

	return TRUE;
}

void SeVisualAPR_with_CUDA::InitPlanePos()
{
	// 2 (x, y) values for every position
	m_pSourcePosition = new int[m_nRstWidth * m_nRstHeight * 2];
	memset(m_pSourcePosition, 0, sizeof(int) * m_nRstWidth * m_nRstHeight * 2);
	for (int i = -m_nRstHeight/2; i < m_nRstHeight/2; i++)
	{
		for (int j = -m_nRstWidth/2; j < m_nRstWidth/2; j++)
		{
			int nNum = (i + m_nRstHeight/2)*m_nRstWidth + (j + m_nRstWidth/2);
			m_pSourcePosition[nNum * 2] = j;
			m_pSourcePosition[nNum * 2 + 1] = i;
		}
	}
}

BOOL SeVisualAPR_with_CUDA::ReleaseCUDAMemory()
{
	// transform vectors in parallel.
	cudaError_t cudaStatus = ReleaseDataFromDevice(dev_vec_out, dev_vec_in, dev_data_in, dev_matrix_in, dev_vec_in_2);
	if (cudaStatus != cudaSuccess) {
		return FALSE;
	}

	// cudaDeviceReset must be called before exiting in order for profiling and
	// tracing tools such as Nsight and Visual Profiler to show complete traces.
	cudaStatus = cudaDeviceReset();
	if (cudaStatus != cudaSuccess) {
		return FALSE;
	}

	return TRUE;
}

BOOL SeVisualAPR_with_CUDA::InitCUDAMemory()
{
	// cuda device memory point
	dev_vec_in_2 = NULL;
	dev_matrix_in = NULL;
	dev_vec_out = NULL;
	dev_vec_in = NULL;
	dev_data_in = NULL;

	host_vec_in_2 = m_pSourcePosition;
	host_matrix_in = NULL;
	host_vec_out = m_pResultData;
	host_vec_in = NULL;
	host_data_in = m_pOriginalData;

	imagesize = m_nRstWidth * m_nRstHeight;
	slice = m_nOriLength;
	sliceSize = m_nOriHeight * m_nOriWidth;
	pitch_out = 0;
	// transform vectors in parallel.
	cudaError_t cudaStatus = CopyDataToDevice(host_vec_out, host_vec_in, host_data_in, host_matrix_in, host_vec_in_2, 
		dev_vec_out, dev_vec_in, dev_data_in, dev_matrix_in, dev_vec_in_2, 
		imagesize, slice, sliceSize, &pitch_out);
	if (cudaStatus != cudaSuccess) {
		return FALSE;
	}
	return TRUE;
}

void SeVisualAPR_with_CUDA::GetTransfromedPos(int* srcPos, float* rstPos, float* matrix, float* pos, int rstWidth, int rstHeight, float pixelScale)
{
	for (int i=0; i<rstHeight * rstWidth; i++)
	{
			int x = *srcPos++;
			int y = *srcPos++;
			*rstPos++ = matrix[0] * x + matrix[4] * y + pos[0];
			*rstPos++ = matrix[1] * x + matrix[5] * y + pos[1];
			*rstPos++ = (matrix[2] * x + matrix[6] * y + pos[2]) / pixelScale;
	}
}

void SeVisualAPR_with_CUDA::CaculateRstData(short** srcData, short* rstData, float* rstPos, int rstWidth, int rstHeight, int width, int height, int length, short min, float pixelScale)
{
	long long x0, x1, y0, y1, z0, z1;
	float deltaX0, deltaX1, deltaY0, deltaY1, deltaZ0, deltaZ1;
	float valueX0, valueX1, valueX2, valueX3, valueY0, valueY1, valueZ0;
	float offsetX, offsetY, offsetZ;
	for (int i=0; i<rstWidth * rstHeight; i++)
	{
		if (rstPos[i*3] < -width/2 || rstPos[i*3] >= width/2 - 1 || 
			rstPos[i*3 + 1] < -height/2 || rstPos[i*3 + 1] >= height/2 - 1 || 
			rstPos[i*3 + 2] < -length/2 || rstPos[i*3 + 2] >= length/2 - 1)
			*rstData++ = min;
		else
		{
			offsetX = rstPos[i*3] + width/2;
			offsetY = rstPos[i*3 + 1] + height/2;
			offsetZ = rstPos[i*3 + 2] + length/2;
			// cell 向上取整 floor 向下取整
			x0 = ceil(offsetX);
			x1 = floor(offsetX);
			y0 = ceil(offsetY);
			y1 = floor(offsetY);
			z0 = ceil(offsetZ);
			z1 = floor(offsetZ);

			deltaX0 = x0 - offsetX;
			deltaX1 = 1.0 - deltaX0;
			deltaY0 = y0 - offsetY;
			deltaY1 = 1.0 - deltaY0;
			deltaZ0 = z0 - offsetZ;
			deltaZ1 = 1.0 - deltaZ0;

			// pitch is imagesize counted by byte not short.

			valueX0 = srcData[z0][y0 * width + x0] * deltaX1 + srcData[z0][y0 * width + x1] * deltaX0;
			valueX1 = srcData[z0][y1 * width + x0] * deltaX1 + srcData[z0][y1 * width + x1] * deltaX0;
			valueX2 = srcData[z1][y0 * width + x0] * deltaX1 + srcData[z1][y0 * width + x1] * deltaX0;
			valueX3 = srcData[z1][y1 * width + x0] * deltaX1 + srcData[z1][y1 * width + x1] * deltaX0;

			valueY0 = valueX0 * deltaY1 + valueX1 * deltaY0;
			valueY1 = valueX2 * deltaY1 + valueX3 * deltaY0;

			valueZ0 = valueY0*deltaZ1 + valueY1*deltaZ0;

			*rstData++ = short(valueZ0);
		}
	}
}

void SeVisualAPR_with_CUDA::SetWinLevel(int nWinCenter, int nWinWidth)
{
	m_nWinCenter = nWinCenter;
	m_nWinWidth = nWinWidth;
}
#endif // CUDA_AVAILABLE



// #include <iostream>
// #include "cuda_runtime.h"
// #include "device_launch_parameters.h"
// using namespace std;
// 
// extern "C"
// 	cudaError_t addWithCuda(int *c, const int *a, const int *b, unsigned int size);
// int main(int argc,char **argv)
// {
// 	const int arraySize = 5;
// 	const int a[arraySize] = { 1, 2, 3, 4, 5 };
// 	const int b[arraySize] = { 10, 20, 30, 40, 50 };
// 	int c[arraySize] = { 0 };
// 
// 	// Add vectors in parallel.
// 	cudaError_t cudaStatus = addWithCuda(c, a, b, arraySize);
// 	if (cudaStatus != cudaSuccess) {
// 		fprintf(stderr, "addWithCuda failed!");
// 		return 1;
// 	}
// 
// 	cout<<"{1,2,3,4,5} + {10,20,30,40,50} = {"<<c[0]<<','<<c[1]<<','<<c[2]<<','<<c[3]<<'}'<<endl;
// 	printf("cpp工程中调用cu成功！\n");
// 
// 	// cudaDeviceReset must be called before exiting in order for profiling and
// 	// tracing tools such as Nsight and Visual Profiler to show complete traces.
// 	cudaStatus = cudaDeviceReset();
// 	if (cudaStatus != cudaSuccess) {
// 		fprintf(stderr, "cudaDeviceReset failed!");
// 		return 1;
// 	}
// 	system("pause"); //here we want the console to hold for a while
// 	return 0;
// }