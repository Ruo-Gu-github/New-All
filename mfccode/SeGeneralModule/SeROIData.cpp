#include "StdAfx.h"
#include "SeROIData.h"

CCriticalSection g_criSection_0;

CSeROIData::CSeROIData(void)
{
	m_pData = NULL;
	m_pSliceData = NULL;
	m_pFloodFillSrc = NULL;
	m_bShow = FALSE;
	m_nWidth = 0;
	m_nHeight = 0;
	m_nLength = 0;
	m_color = RGB(0, 0, 0);
	m_alpha = 0;
}

CSeROIData::CSeROIData(int nWidth, int nHeight, int nLength, CDcmPicArray* pDcmArray, int nLow, int nHigh, COLORREF color, DWORD alpha, BOOL bShow/* = TRUE*/)
{
	m_pData = new BYTE[(ULONG)nWidth * (ULONG)nHeight * (ULONG)nLength];
	memset(m_pData, 0, sizeof(BYTE) * nWidth * nHeight * nLength);
	BYTE* pHeadPoint = m_pData;

// 	// 多线程
// 	queue <DcmInfo> infoList;
// 	for (int i=0; i<pDcmArray->GetDcmArray().size(); i++)
// 	{
// 		DcmInfo info(pDcmArray->GetDcmArray()[i], i);
// 		infoList.push(info);
// 	}
// 	static int nThreadAlive = THREAD_NUMBER;
// 	nThreadAlive = THREAD_NUMBER;
// 
// 	ThreadInfo_0* pThreadInfo = new ThreadInfo_0(infoList, pHeadPoint, nWidth, nHeight, nLength, nLow, nHigh, nThreadAlive);
// 
// 	for (int i=0; i<THREAD_NUMBER; i++)
// 	{
// 		AfxBeginThread(__ConvertImagesWay0, pThreadInfo);
// 	}
// 
// 	while (pThreadInfo->nThreadAlive > 0)
// 	{
// 		int x = 0;
// 	}

	// 单线程
	for (int i=0; i<nLength; i++)
	{
		short* pOriData = (short*)pDcmArray->GetDcmArray()[i]->GetData();
		for (int j=0; j<nHeight * nWidth; j++)
		{
			short OriData = *pOriData++;
			if (OriData >= nLow && OriData <= nHigh)
				*pHeadPoint = 255;
			pHeadPoint++;
		}
	}

	m_pSliceData = NULL;
	m_pFloodFillSrc = NULL;
	m_nWidth = nWidth;
	m_nHeight = nHeight;
	m_nLength = nLength;
	m_bShow = bShow;
	m_color = color;
	m_alpha = alpha;
}

CSeROIData::CSeROIData(CString strFile, int nWidth, int nHeight, int nLength, COLORREF color, DWORD alpha, BOOL bShow /*= TRUE*/)
{
	m_pData = new BYTE[nWidth * nHeight * nLength];
	ifstream inF;
	inF.open(strFile, std::ifstream::binary);
	inF.read(reinterpret_cast<char*>(m_pData), sizeof(BYTE) * nWidth * nHeight * nLength);
	inF.close();

	m_pSliceData = NULL;
	m_pFloodFillSrc = NULL;
	m_nWidth = nWidth;
	m_nHeight = nHeight;
	m_nLength = nLength;
	m_bShow = bShow;
	m_color = color;
	m_alpha = alpha;
}


UINT CSeROIData::__ConvertImagesWay0(LPVOID pParam)
{
	ThreadInfo_0* pInfo = (ThreadInfo_0*) pParam;
	BYTE* pData = pInfo->pData;
	int nWidth = pInfo->nWidth;
	int nHeight = pInfo->nHeight;
	int nLength = pInfo->nLength;
	int nLow    = pInfo->nLow;
	int nHigh   = pInfo->nHigh;
	bool bKillThread = false;
	while(TRUE)
	{
		g_criSection_0.Lock();
		if (pInfo->infoList.empty())
		{
			// 等待其他线程读取完成
			g_criSection_0.Unlock();
			if (bKillThread == false)
			{
				pInfo->nThreadAlive -= 1;
				bKillThread = true;
				continue;
			}	
			if (pInfo->nThreadAlive == 0)
				return 0;
		}
		else
		{
			DcmInfo info = pInfo->infoList.front();
			pInfo->infoList.pop();
			g_criSection_0.Unlock();

			short* pOriData = (short*)info.pDcm->GetData();
			short* pHeadData = pOriData;
			BYTE* pTmp = new BYTE[nWidth * nHeight];
			BYTE* pHeadTmp = pTmp;
			memset(pTmp, 0, sizeof(BYTE) * nWidth * nHeight);
			
			for (int j=0; j<nHeight * nWidth; j++)
			{
				short OriData = *pHeadData++;
				if (OriData >= nLow && OriData <= nHigh)
					*pHeadTmp = 255;
				pHeadTmp++;
			}
			BYTE* pTmpData = pData;
			pTmpData += info.nMark * nWidth * nHeight;
			g_criSection_0.Lock();
			memcpy(pTmpData, pTmp, sizeof(BYTE) * nWidth * nHeight);
			g_criSection_0.Unlock();
		}
	}
	return 0;
}

CSeROIData::CSeROIData(CSeROIData* pMaskA, CSeROIData* pMaskB, BOOLEAN_OPERATION operation, COLORREF color, DWORD alpha, BOOL bShow/* = TRUE*/)
{
	if (pMaskA == NULL || pMaskB == NULL)
	{
		m_pData = NULL;
		m_pSliceData = NULL;
		return;
	}


	m_nWidth = pMaskA->GetWidth();
	m_nHeight = pMaskA->GetHeight();
	m_nLength = pMaskA->GetLength();
	m_bShow = bShow;
	m_color = color;
	m_alpha = alpha;
	m_pData = new BYTE[m_nWidth * m_nHeight * m_nLength];
	m_pSliceData = NULL;
	m_pFloodFillSrc = NULL;
	memset(m_pData, 0, sizeof(BYTE) * m_nWidth * m_nHeight * m_nLength);
	BYTE* pHeadPoint = m_pData;
	BYTE* pMaskAHead = pMaskA->GetData();
	BYTE* pMaskBHead = pMaskB->GetData();

	switch (operation)
	{
	case BOOL_INTEREST:
		{
			for (int i=0; i<m_nWidth * m_nHeight * m_nLength; i++)
			{
				*pHeadPoint++ = (*pMaskAHead++) & (*pMaskBHead++);
			}
			break;
		}
	case BOOL_UNION:
		{
			for (int i=0; i<m_nWidth * m_nHeight * m_nLength; i++)
			{
				*pHeadPoint++ = (*pMaskAHead++) | (*pMaskBHead++);
			}
			break;
		}
	case BOOL_XOR:
		{
			for (int i=0; i<m_nWidth * m_nHeight * m_nLength; i++)
			{
				*pHeadPoint++ = (*pMaskAHead) & ((*pMaskAHead) ^ (*pMaskBHead));
				pMaskAHead++;
				pMaskBHead++;
			}
			break;
		}
	default:
		break;
	}
}

CSeROIData::CSeROIData(CSeROIData* pSrc, COLORREF color, DWORD alpha, BOOL bShow/* = TRUE*/)
{
	if (pSrc == NULL)
	{
		m_pData = NULL;
		m_pSliceData = NULL;
		m_pFloodFillSrc = NULL;
		return;
	}
	
	m_nWidth = pSrc->GetWidth();
	m_nHeight = pSrc->GetHeight();
	m_nLength = pSrc->GetLength();
	m_color = color;
	m_alpha = alpha;
	m_bShow = bShow;
	m_pData = new BYTE[m_nWidth * m_nHeight * m_nLength];
	m_pSliceData = NULL;
	m_pFloodFillSrc = pSrc;
	memset(m_pData, 0, sizeof(BYTE) * m_nWidth * m_nHeight * m_nLength);
	BYTE* pHeadPoint = m_pData;

}


CSeROIData::~CSeROIData(void)
{
	if (m_pData != NULL)
		delete [] m_pData;
	m_pData = NULL;

	if (m_pSliceData != NULL)
		delete [] m_pSliceData;
	m_pSliceData = NULL;
}



BYTE* CSeROIData::GetSliceData(int nLayer, int nPlane)
{
	if (nLayer < 0 || 
		(nPlane == 1 && nLayer >= m_nLength) || 
		(nPlane == 2 && nLayer >= m_nWidth) ||
		(nPlane == 3 && nLayer >= m_nHeight)
		)
		return NULL;
	if (m_pSliceData != NULL)
		delete [] m_pSliceData;
	m_pSliceData = NULL;


	if (nPlane == 1)
	{
		m_pSliceData = new BYTE[m_nWidth * m_nHeight];
		memset(m_pSliceData, 0, sizeof(BYTE) * m_nWidth * m_nHeight);
		BYTE* pHeadPoint = m_pData;
		pHeadPoint += nLayer * m_nWidth * m_nHeight;
		memcpy(m_pSliceData, pHeadPoint, sizeof(BYTE) * m_nWidth * m_nHeight);
	}
	else if (nPlane == 3)
	{
		m_pSliceData = new BYTE[m_nWidth * m_nLength];
		memset(m_pSliceData, 0, sizeof(BYTE) * m_nWidth * m_nLength);
		BYTE* pHeadPoint = m_pData;
		BYTE* pSliceHead = m_pSliceData;
		pHeadPoint += m_nWidth * nLayer;
		for (int i=0; i<m_nLength; i++)
		{
			memcpy(pSliceHead, pHeadPoint, sizeof(BYTE) * m_nWidth);
			pHeadPoint += m_nWidth * m_nHeight;
			pSliceHead += m_nWidth;
		}
	}
	else if (nPlane == 2)
	{
		m_pSliceData = new BYTE[m_nHeight * m_nLength];
		memset(m_pSliceData, 0, sizeof(BYTE) * m_nHeight * m_nLength);
		BYTE* pHeadPoint = m_pData;
		BYTE* pSliceHead = m_pSliceData;
		pHeadPoint += nLayer;
		for (int i=0; i<m_nLength * m_nHeight; i++)
		{
			*pSliceHead++ = *pHeadPoint;
			pHeadPoint += m_nWidth;
		}
	}
	return m_pSliceData;
}



void CSeROIData::SetSliceData(BYTE* pSliceData, int nLayer, int nPlane)
{
	if (nPlane == 1)
	{
		BYTE* pHeadPoint = m_pData;
		pHeadPoint += nLayer * m_nWidth * m_nHeight;
		memcpy(pHeadPoint, pSliceData, sizeof(BYTE) * m_nWidth * m_nHeight);
	}
	else if (nPlane == 3)
	{
		BYTE* pHeadPoint = m_pData;
		BYTE* pSliceHead = pSliceData;
		pHeadPoint += m_nWidth * nLayer;
		for (int i=0; i<m_nLength; i++)
		{
			memcpy(pHeadPoint, pSliceHead, sizeof(BYTE) * m_nWidth);
			pHeadPoint += m_nWidth * m_nHeight;
			pSliceHead += m_nWidth;
		}
	}
	else if (nPlane == 2)
	{
		BYTE* pHeadPoint = m_pData;
		BYTE* pSliceHead = pSliceData;
		pHeadPoint += nLayer;
		for (int i=0; i<m_nLength * m_nHeight; i++)
		{
			*pHeadPoint = *pSliceHead++;
			pHeadPoint += m_nWidth;
		}
	}
}



void CSeROIData::Close(int nKernel)
{
	Dilate(nKernel);
	Corrosion(nKernel);
}

void CSeROIData::Close(int nKernel, int nPlane)
{
	Dilate(nKernel, nPlane);
	Corrosion(nKernel, nPlane);
}

void CSeROIData::Open(int nKernel)
{
	Corrosion(nKernel);
	Dilate(nKernel);
}

void CSeROIData::Open(int nKernel, int nPlane)
{
	Corrosion(nKernel, nPlane);
	Dilate(nKernel, nPlane);
}

void CSeROIData::Corrosion(int nKernel)
{
	for (int n = 0; n<nKernel; n++)
	{
		BYTE* pTmpData = new BYTE[m_nWidth * m_nHeight * m_nLength];
		memcpy(pTmpData, m_pData, sizeof(BYTE) * m_nWidth * m_nHeight * m_nLength);
		for (LONGLONG i = 0; i < m_nLength; i++)
		{
			for (LONGLONG j = 0; j < m_nHeight; j++)
			{
				for (LONGLONG k = 0; k < m_nWidth; k ++)
				{
					if (i == 0 || i == m_nLength - 1 || j == 0 || j == m_nHeight - 1 || k ==0 || k == m_nWidth - 1)
					{
						m_pData[i * m_nWidth * m_nHeight + j * m_nWidth + k] = 0;
					}
					else if (
						pTmpData[(i-1) * m_nWidth * m_nHeight + (j-1) * m_nWidth + (k-1)] == 0 ||
						pTmpData[(i-1) * m_nWidth * m_nHeight + (j-1) * m_nWidth + k    ] == 0 ||
						pTmpData[(i-1) * m_nWidth * m_nHeight + (j-1) * m_nWidth + (k+1)] == 0 ||
						pTmpData[(i-1) * m_nWidth * m_nHeight + j	  * m_nWidth + (k-1)] == 0 ||
						pTmpData[(i-1) * m_nWidth * m_nHeight + j	  * m_nWidth + k    ] == 0 ||
						pTmpData[(i-1) * m_nWidth * m_nHeight + j	  * m_nWidth + (k+1)] == 0 ||
						pTmpData[(i-1) * m_nWidth * m_nHeight + (j+1) * m_nWidth + (k-1)] == 0 ||
						pTmpData[(i-1) * m_nWidth * m_nHeight + (j+1) * m_nWidth + k    ] == 0 ||
						pTmpData[(i-1) * m_nWidth * m_nHeight + (j+1) * m_nWidth + (k+1)] == 0 ||
						pTmpData[i	   * m_nWidth * m_nHeight + (j-1) * m_nWidth + (k-1)] == 0 ||
						pTmpData[i	   * m_nWidth * m_nHeight + (j-1) * m_nWidth + k    ] == 0 ||
						pTmpData[i	   * m_nWidth * m_nHeight + (j-1) * m_nWidth + (k+1)] == 0 ||
						pTmpData[i	   * m_nWidth * m_nHeight + j	  * m_nWidth + (k-1)] == 0 ||
						pTmpData[i	   * m_nWidth * m_nHeight + j	  * m_nWidth + (k+1)] == 0 ||
						pTmpData[i	   * m_nWidth * m_nHeight + (j+1) * m_nWidth + (k-1)] == 0 ||
						pTmpData[i	   * m_nWidth * m_nHeight + (j+1) * m_nWidth + k    ] == 0 ||
						pTmpData[i	   * m_nWidth * m_nHeight + (j+1) * m_nWidth + (k+1)] == 0 ||
						pTmpData[(i+1) * m_nWidth * m_nHeight + (j-1) * m_nWidth + (k-1)] == 0 ||
						pTmpData[(i+1) * m_nWidth * m_nHeight + (j-1) * m_nWidth + k    ] == 0 ||
						pTmpData[(i+1) * m_nWidth * m_nHeight + (j-1) * m_nWidth + (k+1)] == 0 ||
						pTmpData[(i+1) * m_nWidth * m_nHeight + j	  * m_nWidth + (k-1)] == 0 ||
						pTmpData[(i+1) * m_nWidth * m_nHeight + j	  * m_nWidth + k    ] == 0 ||
						pTmpData[(i+1) * m_nWidth * m_nHeight + j	  * m_nWidth + (k+1)] == 0 ||
						pTmpData[(i+1) * m_nWidth * m_nHeight + (j+1) * m_nWidth + (k-1)] == 0 ||
						pTmpData[(i+1) * m_nWidth * m_nHeight + (j+1) * m_nWidth + k    ] == 0 ||
						pTmpData[(i+1) * m_nWidth * m_nHeight + (j+1) * m_nWidth + (k+1)] == 0
						)
						m_pData[i * m_nWidth * m_nHeight + j * m_nWidth + k] = 0;
					else
						m_pData[i * m_nWidth * m_nHeight + j * m_nWidth + k] = 255;
				}
			}
		}
		delete [] pTmpData;
	}

}

void CSeROIData::Corrosion(int nKernel, int nPlane)
{
	if (nPlane != 1)
		return;

	// 多线程
	BYTE* pData = m_pData;
	static int nThreadAlive = THREAD_NUMBER;
	nThreadAlive = THREAD_NUMBER;

	static int nLength = m_nLength; // 多线程需要同步， 声明 length 为 static 并赋值。
	nLength = m_nLength;


	ThreadInfo_1* ThreadInfo = new ThreadInfo_1(pData, m_nWidth, m_nHeight, nLength, nKernel, nThreadAlive);

	for (int i=0; i<THREAD_NUMBER; i++)
	{
		AfxBeginThread(__Corrosion, ThreadInfo);
	}

	while (ThreadInfo->nThreadAlive > 0)
	{
		int x = 0;
	}


//  // 单线程
// 	int nLength = 0, nWidth = 0, nHeight = 0;
// 	if (nPlane == 1)
// 	{
// 		nWidth = m_nWidth;
// 		nHeight = m_nHeight;
// 		nLength = m_nLength;
// 	}
// 	else if (nPlane == 2)
// 	{
// 		nWidth = m_nHeight;
// 		nHeight = m_nLength;
// 		nLength = m_nWidth;
// 	}
// 	else if (nPlane == 3)
// 	{
// 		nWidth = m_nWidth;
// 		nHeight = m_nLength;
// 		nLength = m_nHeight;
// 	}
// 	for (int j=0; j<nLength; j++)
// 	{
// 		BYTE* pSlice = GetSliceData(j, nPlane);
// 		Corrosion(pSlice, nWidth, nHeight, nKernel);
// 		SetSliceData(pSlice, j, nPlane);
// 	}
}

UINT CSeROIData::__Corrosion(LPVOID pParam)
{
	ThreadInfo_1* pInfo = (ThreadInfo_1*) pParam;
	BYTE*         pData = pInfo->pData;
	int           nWidth = pInfo->nWidth;
	int           nHeight = pInfo->nHeight;
	int           nKernel = pInfo->nKernel;
	int           nPlane =  pInfo->nKernel;
	bool		  bKillThread = false;
	while(TRUE)
	{
		g_criSection_0.Lock();
		if (pInfo->nLength == 0)
		{
			// 等待其他线程读取完成
			g_criSection_0.Unlock();
			if (bKillThread == false)
			{
				pInfo->nThreadAlive -= 1;
				bKillThread = true;
				continue;
			}	
			if (pInfo->nThreadAlive == 0)
				return 0;
		}
		else
		{
			int nSlice = pInfo->nLength;
			pInfo->nLength -= 1;
			g_criSection_0.Unlock();
 
			BYTE* pSlice = pData + nWidth * nHeight * (nSlice - 1);
			Corrosion(pSlice, nWidth, nHeight, nKernel);
		}
	}
 	return 0;
}


void CSeROIData::Corrosion(BYTE *pData, int nWidth, int nHeight, int nKernel)
{
	if (nKernel == 0)
		return;
	int nValue = 255;
	if(nHeight <= 0 || nWidth <= 0)
		return;
	BYTE* temp = new BYTE[nWidth*nHeight];
	memcpy(temp, pData, nWidth*nHeight*sizeof(BYTE));
	for (int i=1; i<nWidth-1; i++)
	{
		for(int j=1;j<nHeight-1;j++)
		{
			if (temp[i+j*nWidth] >= nValue)
			{
				if ((temp[i-1+(j+1)*nWidth] == nValue
					&&temp[i+(j+1)*nWidth] == nValue
					&&temp[i+1+(j+1)*nWidth] == nValue
					&&temp[i+1+j*nWidth] == nValue
					&&temp[i-1+j*nWidth] == nValue
					&&temp[i-1+(j-1)*nWidth] == nValue
					&&temp[i+(j-1)*nWidth] == nValue
					&&temp[i+1+(j-1)*nWidth] == nValue))
				{
					pData[i+j*nWidth] = nValue;
				}
				else 
					pData[i+j*nWidth] = 0;
			}/*如果该点附近有点的值不等于最大值，将该点设为最小值*/
			else pData[i+j*nWidth] = 0;
		}	
	}
	Safe_Delete(temp);
	nKernel = nKernel - 1; 
	Corrosion(pData, nWidth, nHeight, nKernel);
}

void CSeROIData::Dilate(int nKernel)
{
	for (int n = 0; n<nKernel; n++)
	{
		BYTE* pTmpData = new BYTE[m_nWidth * m_nHeight * m_nLength];
		memcpy(pTmpData, m_pData, sizeof(BYTE) * m_nWidth * m_nHeight * m_nLength);
		for (LONGLONG i = 1; i < m_nLength - 1; i++)
		{
			for (LONGLONG j = 1; j < m_nHeight - 1; j++)
			{
				for (LONGLONG k = 1; k < m_nWidth - 1; k ++)
				{
					if (pTmpData[i * m_nWidth * m_nHeight + j * m_nWidth + k] == 255)
					{
						m_pData[(i-1) * m_nWidth * m_nHeight + (j-1) * m_nWidth + (k-1)] = 255;
						m_pData[(i-1) * m_nWidth * m_nHeight + (j-1) * m_nWidth + k	   ] = 255;
						m_pData[(i-1) * m_nWidth * m_nHeight + (j-1) * m_nWidth + (k+1)] = 255;
						m_pData[(i-1) * m_nWidth * m_nHeight + j	 * m_nWidth + (k-1)] = 255;
						m_pData[(i-1) * m_nWidth * m_nHeight + j	 * m_nWidth + k	   ] = 255;
						m_pData[(i-1) * m_nWidth * m_nHeight + j	 * m_nWidth + (k+1)] = 255;
						m_pData[(i-1) * m_nWidth * m_nHeight + (j+1) * m_nWidth + (k-1)] = 255;
						m_pData[(i-1) * m_nWidth * m_nHeight + (j+1) * m_nWidth + k    ] = 255;
						m_pData[(i-1) * m_nWidth * m_nHeight + (j+1) * m_nWidth + (k+1)] = 255;
						m_pData[i	  * m_nWidth * m_nHeight + (j-1) * m_nWidth + (k-1)] = 255;
						m_pData[i	  * m_nWidth * m_nHeight + (j-1) * m_nWidth + k    ] = 255;
						m_pData[i	  * m_nWidth * m_nHeight + (j-1) * m_nWidth + (k+1)] = 255;
						m_pData[i	  * m_nWidth * m_nHeight + j	 * m_nWidth + (k-1)] = 255;
						m_pData[i	  * m_nWidth * m_nHeight + j	 * m_nWidth + (k+1)] = 255;
						m_pData[i	  * m_nWidth * m_nHeight + (j+1) * m_nWidth + (k-1)] = 255;
						m_pData[i	  * m_nWidth * m_nHeight + (j+1) * m_nWidth + k    ] = 255;
						m_pData[i	  * m_nWidth * m_nHeight + (j+1) * m_nWidth + (k+1)] = 255;
						m_pData[(i+1) * m_nWidth * m_nHeight + (j-1) * m_nWidth + (k-1)] = 255;
						m_pData[(i+1) * m_nWidth * m_nHeight + (j-1) * m_nWidth + k    ] = 255;
						m_pData[(i+1) * m_nWidth * m_nHeight + (j-1) * m_nWidth + (k+1)] = 255;
						m_pData[(i+1) * m_nWidth * m_nHeight + j	 * m_nWidth + (k-1)] = 255;
						m_pData[(i+1) * m_nWidth * m_nHeight + j	 * m_nWidth + k    ] = 255;
						m_pData[(i+1) * m_nWidth * m_nHeight + j	 * m_nWidth + (k+1)] = 255;
						m_pData[(i+1) * m_nWidth * m_nHeight + (j+1) * m_nWidth + (k-1)] = 255;
						m_pData[(i+1) * m_nWidth * m_nHeight + (j+1) * m_nWidth + k    ] = 255;
						m_pData[(i+1) * m_nWidth * m_nHeight + (j+1) * m_nWidth + (k+1)] = 255;
					}
					else
						m_pData[i * m_nWidth * m_nHeight + j * m_nWidth + k] = 0;
				}
			}
		}
		delete [] pTmpData;
	}
}

void CSeROIData::Dilate(int nKernel, int nPlane)
{
	if (nPlane != 1)
		return;

	// 多线程
	BYTE* pData = m_pData;
	static int nThreadAlive = THREAD_NUMBER;
	nThreadAlive = THREAD_NUMBER;

	static int nLength = m_nLength; // 多线程需要同步， 声明 length 为 static 并赋值。
	nLength = m_nLength;


	ThreadInfo_1* ThreadInfo = new ThreadInfo_1(pData, m_nWidth, m_nHeight, nLength, nKernel, nThreadAlive);

	for (int i=0; i<THREAD_NUMBER; i++)
	{
		AfxBeginThread(__Dilate, ThreadInfo);
	}

	while (ThreadInfo->nThreadAlive > 0)
	{
		int x = 0;
	}

//  // 单线程
// 	int nLength = 0, nWidth = 0, nHeight = 0;
// 	if (nPlane == 1)
// 	{
// 		nWidth = m_nWidth;
// 		nHeight = m_nHeight;
// 		nLength = m_nLength;
// 	}
// 	else if (nPlane == 2)
// 	{
// 		nWidth = m_nHeight;
// 		nHeight = m_nLength;
// 		nLength = m_nWidth;
// 	}
// 	else if (nPlane == 3)
// 	{
// 		nWidth = m_nWidth;
// 		nHeight = m_nLength;
// 		nLength = m_nHeight;
// 	}
// 
// 	for (int j=0; j<nLength; j++)
// 	{
// 		BYTE* pSlice = GetSliceData(j, nPlane);
// 		Dilate(pSlice, nWidth, nHeight, nKernel);
// 		SetSliceData(pSlice, j, nPlane);
// 	}
}

UINT CSeROIData::__Dilate(LPVOID pParam)
{
	ThreadInfo_1* pInfo = (ThreadInfo_1*) pParam;
	BYTE*         pData = pInfo->pData;
	int           nWidth = pInfo->nWidth;
	int           nHeight = pInfo->nHeight;
	int           nKernel = pInfo->nKernel;
	int           nPlane =  pInfo->nKernel;
	bool		  bKillThread = false;
	while(TRUE)
	{
		g_criSection_0.Lock();
		if (pInfo->nLength == 0)
		{
			// 等待其他线程读取完成
			g_criSection_0.Unlock();
			if (bKillThread == false)
			{
				pInfo->nThreadAlive -= 1;
				bKillThread = true;
				continue;
			}	
			if (pInfo->nThreadAlive == 0)
				return 0;
		}
		else
		{
			int nSlice = pInfo->nLength;
			pInfo->nLength -= 1;
			g_criSection_0.Unlock();

			BYTE* pSlice = pData + nWidth * nHeight * (nSlice - 1);	ThreadInfo_1* pInfo = (ThreadInfo_1*) pParam;
			BYTE*         pData = pInfo->pData;
			int           nWidth = pInfo->nWidth;
			int           nHeight = pInfo->nHeight;
			int           nKernel = pInfo->nKernel;
			int           nPlane =  pInfo->nKernel;
			bool		  bKillThread = false;
			while(TRUE)
			{
				g_criSection_0.Lock();
				if (pInfo->nLength == 0)
				{
					// 等待其他线程读取完成
					g_criSection_0.Unlock();
					if (bKillThread == false)
					{
						pInfo->nThreadAlive -= 1;
						bKillThread = true;
						continue;
					}	
					if (pInfo->nThreadAlive == 0)
						return 0;
				}
				else
				{
					int nSlice = pInfo->nLength;
					pInfo->nLength -= 1;
					g_criSection_0.Unlock();

					BYTE* pSlice = pData + nWidth * nHeight * (nSlice - 1);
					Dilate(pSlice, nWidth, nHeight, nKernel);
				}
			}
			return 0;
			Dilate(pSlice, nWidth, nHeight, nKernel);
		}
	}
	return 0;
}

void CSeROIData::Dilate(BYTE *pData, int nWidth, int nHeight, int nKernel)
{
	if (nKernel == 0)
		return;
	int nValue = 255;
	if(nHeight <= 0 || nWidth <= 0)
		return ;
	BYTE* temp = new BYTE[nWidth*nHeight];
	memcpy(temp, pData , nWidth*nHeight*sizeof(BYTE));
	for (int i = 1 ; i < nWidth - 1; i++)
	{
		for (int j = 1 ; j<nHeight - 1; j++)
		{
			if(temp[i+j*nWidth] == nValue)
			{
				pData[i-1+(j+1)*nWidth	] = nValue;
				pData[i+(j+1)*nWidth	] = nValue;
				pData[i+1+(j+1)*nWidth	] = nValue;
				pData[i+1+j*nWidth		] = nValue;
				pData[i-1+j*nWidth		] = nValue;
				pData[i-1+(j-1)*nWidth	] = nValue;
				pData[i+(j-1)*nWidth	] = nValue;
				pData[i+1+(j-1)*nWidth	] = nValue;
			}/*如果该点为最大值，将附近的点都设为最大值*/
		}
	}
	Safe_Delete(temp);
	nKernel = nKernel - 1; 
	Dilate(pData, nWidth, nHeight, nKernel);
}

void CSeROIData::Inverse()
{
	BYTE* pHeadPoint = m_pData;
	BYTE* pTmpData = new BYTE[m_nWidth * m_nHeight * m_nLength];
	BYTE* pTmpHead = pTmpData;
	memcpy(pTmpData, m_pData, sizeof(BYTE) * m_nWidth * m_nHeight * m_nLength);
	for (LONGLONG i=0; i<m_nWidth * m_nHeight * m_nLength; i++)
	{
		if(*pTmpHead++ == 0)
			*pHeadPoint++ = 255;
		else
			*pHeadPoint++ = 0; 
	}
	delete [] pTmpData;
}

void CSeROIData::FloodFill(int nXpos, int nYpos, int nZpos)
{
	if (m_pFloodFillSrc == NULL)
		return;
	CSeROIData *pSrc = m_pFloodFillSrc;
	if (pSrc->m_pData[nZpos * m_nWidth * m_nHeight + nYpos * m_nWidth + nXpos] == 0)
		return;
	if (m_pData[nZpos * m_nWidth * m_nHeight + nYpos * m_nWidth + nXpos] == 255)
		return;

	BYTE* pPointsState = new BYTE[m_nWidth * m_nHeight * m_nLength];
	memcpy(pPointsState, pSrc->m_pData, sizeof(BYTE) * m_nWidth * m_nHeight * m_nLength);

	int pointsInQueue = 0;
	int SizeofQueue = 1024;
	LONGLONG* pQueue = new LONGLONG[SizeofQueue];
	memset(pQueue, 0, sizeof(LONGLONG) * SizeofQueue);
	LONGLONG index = nZpos * m_nWidth * m_nHeight + nYpos * m_nWidth + nXpos;
	pQueue[pointsInQueue++] = index;

	int pointsInRegion = 0;

	while (pointsInQueue > 0)
	{
		LONGLONG nextIndex = pQueue[--pointsInQueue];

		LONGLONG currentPointStateIndex = nextIndex;

		pPointsState[nextIndex] = 0;

		LONGLONG z = nextIndex / (m_nWidth * m_nHeight);
		LONGLONG currentSliceIndex = nextIndex % (m_nWidth * m_nHeight); 
		LONGLONG y = currentSliceIndex / m_nWidth;
		LONGLONG x = currentSliceIndex % m_nWidth;

		m_pData[currentPointStateIndex] = 255;

		++pointsInRegion;
		LONGLONG pos[6];
		pos[0] = z * m_nWidth * m_nHeight + y * m_nWidth + (x - 1);
		pos[1] = z * m_nWidth * m_nHeight + y * m_nWidth + (x + 1);
		pos[2] = z * m_nWidth * m_nHeight + (y - 1) * m_nWidth + x;
		pos[3] = z * m_nWidth * m_nHeight + (y + 1) * m_nWidth + x;
		pos[4] = (z - 1) * m_nWidth * m_nHeight + y * m_nWidth + x;
		pos[5] = (z + 1) * m_nWidth * m_nHeight + y * m_nWidth + x;

		if (x == 0)
			pos[0] = -1;
		else if (x == (m_nWidth - 1))
			pos[1] = -1;
		if (y == 0)
			pos[2] = -1;
		else if (y == (m_nHeight - 1))
			pos[3] = -1;
		if (z == 0)
			pos[4] = -1;
		else if (z == (m_nLength - 1))
			pos[5] = -1;
		
		if (pos[0] != -1)
		{
			if (pPointsState[pos[0]] != 0)
			{
				m_pData[pos[0]] = 255; 
				if (pointsInQueue == SizeofQueue)
				{
					int newSizeofQueue = SizeofQueue * 1.5;
					LONGLONG* pQueuetmp = new LONGLONG[newSizeofQueue];
					memset(pQueuetmp, 0, sizeof(LONGLONG) * newSizeofQueue);
					memcpy(pQueuetmp, pQueue, sizeof(LONGLONG) * SizeofQueue);
					Safe_Delete(pQueue);
					pQueue = pQueuetmp;
					SizeofQueue = newSizeofQueue;
				}
				pQueue[pointsInQueue++] = pos[0];
			}
		}

		if (pos[1] != -1)
		{
			if (pPointsState[pos[1]] != 0)
			{
				m_pData[pos[1]] = 255; 
				if (pointsInQueue == SizeofQueue)
				{
					int newSizeofQueue = SizeofQueue * 1.5;
					LONGLONG* pQueuetmp = new LONGLONG[newSizeofQueue];
					memset(pQueuetmp, 0, sizeof(LONGLONG) * newSizeofQueue);
					memcpy(pQueuetmp, pQueue, sizeof(LONGLONG) * SizeofQueue);
					Safe_Delete(pQueue);
					pQueue = pQueuetmp;
					SizeofQueue = newSizeofQueue;
				}
				pQueue[pointsInQueue++] = pos[1];
			}
		}

		if (pos[2] != -1)
		{
			if (pPointsState[pos[2]] != 0)
			{
				m_pData[pos[2]] = 255; 
				if (pointsInQueue == SizeofQueue)
				{
					int newSizeofQueue = SizeofQueue * 1.5;
					LONGLONG* pQueuetmp = new LONGLONG[newSizeofQueue];
					memset(pQueuetmp, 0, sizeof(LONGLONG) * newSizeofQueue);
					memcpy(pQueuetmp, pQueue, sizeof(LONGLONG) * SizeofQueue);
					Safe_Delete(pQueue);
					pQueue = pQueuetmp;
					SizeofQueue = newSizeofQueue;
				}
				pQueue[pointsInQueue++] = pos[2];
			}
		}

		if (pos[3] != -1)
		{
			if (pPointsState[pos[3]] != 0)
			{
				m_pData[pos[3]] = 255; 
				if (pointsInQueue == SizeofQueue)
				{
					int newSizeofQueue = SizeofQueue * 1.5;
					LONGLONG* pQueuetmp = new LONGLONG[newSizeofQueue];
					memset(pQueuetmp, 0, sizeof(LONGLONG) * newSizeofQueue);
					memcpy(pQueuetmp, pQueue, sizeof(LONGLONG) * SizeofQueue);
					Safe_Delete(pQueue);
					pQueue = pQueuetmp;
					SizeofQueue = newSizeofQueue;
				}
				pQueue[pointsInQueue++] = pos[3];
			}
		}

		if (pos[4] != -1)
		{
			if (pPointsState[pos[4]] != 0)
			{
				m_pData[pos[4]] = 255; 
				if (pointsInQueue == SizeofQueue)
				{
					int newSizeofQueue = SizeofQueue * 1.5;
					LONGLONG* pQueuetmp = new LONGLONG[newSizeofQueue];
					memset(pQueuetmp, 0, sizeof(LONGLONG) * newSizeofQueue);
					memcpy(pQueuetmp, pQueue, sizeof(LONGLONG) * SizeofQueue);
					Safe_Delete(pQueue);
					pQueue = pQueuetmp;
					SizeofQueue = newSizeofQueue;
				}
				pQueue[pointsInQueue++] = pos[4];
			}
		}

		if (pos[5] != -1)
		{
			if (pPointsState[pos[5]] != 0)
			{
				m_pData[pos[5]] = 255; 
				if (pointsInQueue == SizeofQueue)
				{
					int newSizeofQueue = SizeofQueue * 1.5;
					LONGLONG* pQueuetmp = new LONGLONG[newSizeofQueue];
					memset(pQueuetmp, 0, sizeof(LONGLONG) * newSizeofQueue);
					memcpy(pQueuetmp, pQueue, sizeof(LONGLONG) * SizeofQueue);
					Safe_Delete(pQueue);
					pQueue = pQueuetmp;
					SizeofQueue = newSizeofQueue;
				}
				pQueue[pointsInQueue++] = pos[5];
			}
		}
	}

	Safe_Delete(pPointsState);

// 	CStdioFile file("timelog.txt", CFile::modeWrite);
// 	DWORD start0 = GetTickCount();
// 	for (int i=0; i<m_nLength; i++)
// 	{
// 		for (int j=0; j<m_nHeight; j++)
// 		{
// 			for (int k=0; k<m_nWidth; k++)
// 			{
// 				short x0 = m_pData[nZpos * m_nWidth * m_nHeight + nYpos * m_nWidth + nXpos];
// 				short x1 = pSrc->m_pData[nZpos * m_nWidth * m_nHeight + nYpos * m_nWidth + nXpos];
// 				short x2 = m_pData[nZpos * m_nWidth * m_nHeight + nYpos * m_nWidth + nXpos];
// 				short x3 = pSrc->m_pData[nZpos * m_nWidth * m_nHeight + nYpos * m_nWidth + nXpos];
// 				short x4 = m_pData[nZpos * m_nWidth * m_nHeight + nYpos * m_nWidth + nXpos];
// 				short x5 = pSrc->m_pData[nZpos * m_nWidth * m_nHeight + nYpos * m_nWidth + nXpos];
// 				short x6 = m_pData[nZpos * m_nWidth * m_nHeight + nYpos * m_nWidth + nXpos];
// 				short x7 = pSrc->m_pData[nZpos * m_nWidth * m_nHeight + nYpos * m_nWidth + nXpos];
// 				short x8 = m_pData[nZpos * m_nWidth * m_nHeight + nYpos * m_nWidth + nXpos];
// 				short x9 = pSrc->m_pData[nZpos * m_nWidth * m_nHeight + nYpos * m_nWidth + nXpos];
// 				short x10 = m_pData[nZpos * m_nWidth * m_nHeight + nYpos * m_nWidth + nXpos];
// 				short x11 = pSrc->m_pData[nZpos * m_nWidth * m_nHeight + nYpos * m_nWidth + nXpos];
// 			}
// 		}
// 	}
// 	DWORD end0 = GetTickCount();
// 	CString str;// = "111";
// 	str.Format("used time for get all points 12 times is: %d ms.\n", end0 - start0);
// 	file.Seek(0,CFile::end);
// 	file.WriteString(str);
// 	file.Close();


// 	BYTE* pUsedPoints = new BYTE[m_nWidth * m_nHeight * m_nLength];
// 	memset(pUsedPoints, 0, sizeof(BYTE)* m_nWidth * m_nHeight * m_nLength);
// 	queue <SeedPosition> vecSeeds;
// 	SeedPosition firstSeed(nXpos, nYpos, nZpos);
// 	SearchOnePoint(vecSeeds, firstSeed, pSrc->m_pData, pUsedPoints);
// 	m_pData[nZpos * m_nWidth * m_nHeight + nYpos * m_nWidth + nXpos] = 255;
// 	pUsedPoints[nZpos * m_nWidth * m_nHeight + nYpos * m_nWidth + nXpos] = 255;
// 	int nCount = 0;
// 	
// 	
// 	while (!vecSeeds.empty())
// 	{
// 		static DWORD start = 0;
// 		static CStdioFile file("timelog.txt", CFile::modeWrite);
// 		if (nCount == 0)
// 		{
// 			start = GetTickCount();
// 			
// 		}
// 		SeedPosition seed = vecSeeds.front();
// 		m_pData[seed.nZpos * m_nWidth * m_nHeight + seed.nYpos * m_nWidth + seed.nXpos] = 255;
// 		vecSeeds.pop();
// 		SearchOnePoint(vecSeeds, seed, pSrc->m_pData, pUsedPoints);
// 		nCount ++;
// 		if (nCount > 5000)
// 		{
// 			nCount = 0;
// 			DWORD end0 = GetTickCount();
// 			CString str;// = "111";
// 			str.Format("used time for every 5000 times loop is: %d ms.\n", end0 - start);
// 			file.Seek(0,CFile::end);
// 			file.WriteString(str);
// 			file.Close();
// 			file.Open("timelog.txt", CFile::modeWrite);
// 		}
// 	}
}

void CSeROIData::ROI(vector <CPoint>* pVecEdge, int nPlane, ROI_OPERATION op /*= ROI_ADD*/)
{
	if (pVecEdge == NULL)
		return;

	BYTE *pHeadPoint = m_pData;
	int nLength = 0, nWidth = 0, nHeight = 0;
	if (nPlane == 1)
	{
		nWidth = m_nWidth;
		nHeight = m_nHeight;
		nLength = m_nLength;
	}
	else if (nPlane == 2)
	{
		nWidth = m_nHeight;
		nHeight = m_nLength;
		nLength = m_nWidth;
	}
	else if (nPlane == 3)
	{
		nWidth = m_nWidth;
		nHeight = m_nLength;
		nLength = m_nHeight;
	}

	if (op == ROI_INTERSET)
	{
		BYTE* pTmp = new BYTE[m_nWidth * m_nLength * m_nHeight];
		memcpy(pTmp, pHeadPoint, sizeof(BYTE) * m_nWidth * m_nLength * m_nHeight);
		memset(pHeadPoint, 0, sizeof(BYTE) * m_nWidth * m_nLength * m_nHeight);
		for (int i=0; i<nLength; i++)
		{
			// create a slice of region
			if (!pVecEdge[i].empty())
			{
				GraphicsPath gcPath;
				const int nCount = (int)pVecEdge[i].size();
				PointF *ptFCurve = new PointF[nCount];
				for(int j=0; j<nCount; j++)
				{
					ptFCurve[j].X = pVecEdge[i][j].x;
					ptFCurve[j].Y = pVecEdge[i][j].y;
				}
				gcPath.AddClosedCurve(ptFCurve, (INT)pVecEdge[i].size());
				Region intersectRegion(&gcPath);  
				Matrix matrix; 
				int count = 0;  
				count = intersectRegion.GetRegionScansCount(&matrix); 
				Rect* rects = NULL;
				rects = (Rect*)malloc(count*sizeof(Rect));  
				intersectRegion.GetRegionScans(&matrix, rects, &count);  
				for(int j=0; j<count; j++)  
				{
					int nTmpHeight = rects[j].Height;
					for(int k=0; k<nTmpHeight; k++)
					{
						int nTmpWidth = rects[j].Width;
						for(int m=0; m<nTmpWidth; m++)
						{
							if(rects[j].X + m >= 0 && rects[j].X + m <= nWidth && rects[j].Y + k >=0 && rects[j].Y + k < nHeight)
							{
								SeedPosition pos = GetRealPosition(SeedPosition(rects[j].X + m, rects[j].Y + k, i), nPlane);
								pHeadPoint[pos.nZpos * m_nWidth * m_nHeight + pos.nYpos * m_nWidth + pos.nXpos] = 255 & pTmp[pos.nZpos * m_nWidth * m_nHeight + pos.nYpos * m_nWidth + pos.nXpos];
							}
						}
					}
				}
				free(rects);
			}
		}
		Safe_Delete(pTmp);
	}
	else
	{
		int nNewValue = (op == ROI_REMOVE) ? 0 : 255;

		for (int i=0; i<nLength; i++)
		{
			// create a slice of region
			if (!pVecEdge[i].empty())
			{
				GraphicsPath gcPath;
				const int nCount = (int)pVecEdge[i].size();
				PointF *ptFCurve = new PointF[nCount];
				for(int j=0; j<nCount; j++)
				{
					ptFCurve[j].X = pVecEdge[i][j].x;
					ptFCurve[j].Y = pVecEdge[i][j].y;
				}
				gcPath.AddClosedCurve(ptFCurve, (INT)pVecEdge[i].size());
				Region intersectRegion(&gcPath);  
				Matrix matrix; 
				int count = 0;  
				count = intersectRegion.GetRegionScansCount(&matrix); 
				Rect* rects = NULL;
				rects = (Rect*)malloc(count*sizeof(Rect));  
				intersectRegion.GetRegionScans(&matrix, rects, &count);  
				for(int j=0; j<count; j++)  
				{
					int nTmpHeight = rects[j].Height;
					for(int k=0; k<nTmpHeight; k++)
					{
						int nTmpWidth = rects[j].Width;
						for(int m=0; m<nTmpWidth; m++)
						{
							if(rects[j].X + m >= 0 && rects[j].X + m <= nWidth && rects[j].Y + k >=0 && rects[j].Y + k < nHeight)
							{
								// find point position in original data matrix
								SeedPosition pos = GetRealPosition(SeedPosition(rects[j].X + m, rects[j].Y + k, i), nPlane);
								pHeadPoint[pos.nZpos * m_nWidth * m_nHeight + pos.nYpos * m_nWidth + pos.nXpos] = nNewValue;
							}
						}
					}
				}

				free(rects);
			}
		}
	}
}

void CSeROIData::SearchOnePoint(queue <SeedPosition>& vecSeeds, SeedPosition seed, BYTE* pData, BYTE* pUsed)
{
	vector<SeedPosition> vecRoundSeeds;
	if (seed.nXpos == 0)
	{
		vecRoundSeeds.push_back(seed.Offset(1, 0, 0));
	}
	else if (seed.nXpos == m_nWidth)
	{
		vecRoundSeeds.push_back(seed.Offset(-1, 0, 0));
	}
	else
	{
		vecRoundSeeds.push_back(seed.Offset(1, 0, 0));
		vecRoundSeeds.push_back(seed.Offset(-1, 0, 0));
	}
	if (seed.nYpos == 0)
	{
		vecRoundSeeds.push_back(seed.Offset(0, 1, 0));
	}
	else if (seed.nYpos == m_nHeight)
	{
		vecRoundSeeds.push_back(seed.Offset(0, -1, 0));
	}
	else
	{
		vecRoundSeeds.push_back(seed.Offset(0, 1, 0));
		vecRoundSeeds.push_back(seed.Offset(0, -1, 0));
	}
	if (seed.nZpos == 0)
	{
		vecRoundSeeds.push_back(seed.Offset(0, 0, 1));
	}
	else if (seed.nZpos == m_nLength)
	{
		vecRoundSeeds.push_back(seed.Offset(0, 0, -1));
	}
	else
	{
		vecRoundSeeds.push_back(seed.Offset(0, 0, 1));
		vecRoundSeeds.push_back(seed.Offset(0, 0, -1));
	}


	for (int i=0; i<vecRoundSeeds.size(); i++)
	{
		if (pData[vecRoundSeeds[i].nZpos * m_nWidth * m_nHeight + vecRoundSeeds[i].nYpos * m_nWidth + vecRoundSeeds[i].nXpos] == 255 &&
			pUsed[vecRoundSeeds[i].nZpos * m_nWidth * m_nHeight + vecRoundSeeds[i].nYpos * m_nWidth + vecRoundSeeds[i].nXpos] == 0)
		{
			vecSeeds.push(vecRoundSeeds[i]);
		}
		pUsed[vecRoundSeeds[i].nZpos * m_nWidth * m_nHeight + vecRoundSeeds[i].nYpos * m_nWidth + vecRoundSeeds[i].nXpos] = 255;

	}
}

SeedPosition CSeROIData::GetRealPosition(SeedPosition pos, int nPlane)
{
	switch (nPlane)
	{
	case 1:
		return SeedPosition(pos.nXpos, pos.nYpos, pos.nZpos);
	case 2:	
		return SeedPosition(pos.nZpos, pos.nXpos, pos.nYpos);	
	case 3:
		return SeedPosition(pos.nXpos, pos.nZpos, pos.nYpos);
	default:
		return SeedPosition(0, 0, 0);
	}
}












