#include "stdafx.h"
#include "SeVisualMPR.h"

CEvent				g_MPReventStartThread(FALSE,TRUE);
CEvent				g_eventDrawMPR(FALSE,TRUE);	

int SeVisualMPR::m_nWinCenter = 2048;
int SeVisualMPR::m_nWinWidth = 4096;

SeVisualMPR::SeVisualMPR()
{
	m_nPlaneNum = 1;
	m_pDcmArray = NULL;
	m_pCuttingPlanePoint = NULL;
//	m_pCuttingPlanePointOri = NULL;
	m_sCuttingPlaneData = NULL;
	m_sChipArray = NULL;
	m_bDeleteChipArray = true;

	m_hDrawMPR = g_eventDrawMPR;
	m_MPRStartThread = g_MPReventStartThread;
}


SeVisualMPR::~SeVisualMPR()
{
	Safe_DeleteVec(m_pCuttingPlanePoint);
//	Safe_DeleteVec(m_pCuttingPlanePointOri);
	Safe_DeleteVec(m_sCuttingPlaneData);
	if (m_bDeleteChipArray)
	{
		Safe_DeleteVec(m_sChipArray);
	}
}

void SeVisualMPR::SetDcmArray( CDcmPicArray* pArray)
{
	if (m_bDeleteChipArray)
	{
		Safe_DeleteVec(m_sChipArray);
	}
	m_pDcmArray = pArray;
	SetCuttingPlane();
	g_MPReventStartThread.SetEvent();
	g_eventDrawMPR.SetEvent();
}

void SeVisualMPR::SetDcmArray(CDcmPicArray* pArray, short** pChipArray, int nOriWidth, int nOriHeight, int nOriPiece, int nZPixelNum, TPixelDataType DataType )
{
	if (m_bDeleteChipArray)
	{
		Safe_DeleteVec(m_sChipArray);
	}
	m_pDcmArray = pArray;
	m_bDeleteChipArray = false;
	m_sChipArray = pChipArray;
	m_nOriImageWidth = nOriWidth;
	m_nOriImageHeight = nOriHeight;
	m_nOriImagePiece = nOriPiece;
	m_nZPixelNum = nZPixelNum;
	m_DataType = DataType;
	m_dPixelperPiece = m_nZPixelNum*1.0/m_nOriImagePiece;
	SetCuttingPlane(false);
}


void SeVisualMPR::SetCuttingPlane( bool bDcmArray /*= true*/)
{
	if (bDcmArray)
	{
		m_nOriImageWidth = m_pDcmArray->GetDcmArray()[0]->GetWidth();
		m_nOriImageHeight = m_pDcmArray->GetDcmArray()[0]->GetHeight();
		m_nOriImagePiece = m_pDcmArray->GetZDcmPicCount();
		m_nZPixelNum = m_pDcmArray->GetZAxialPixelNumber();
		m_DataType = m_pDcmArray->GetDcmArray()[0]->GetPixelDataType();
		m_dPixelperPiece = m_nZPixelNum*1.0/m_nOriImagePiece;

		m_sChipArray = new short*[m_nOriImagePiece];

		for (int i = 0; i < m_nOriImagePiece; i++)
		{
			m_sChipArray[i] = (short*)(m_pDcmArray->GetDcmArray()[i])->GetData();
		}
	}


	switch (m_nPlaneNum)
	{
	case 1:
		{
			m_nWidth = m_nOriImageWidth;
			m_nHeight = m_nOriImageHeight;
			m_nPiece = m_nZPixelNum;
		}
		break;
	case 2:
		{
			m_nWidth = m_nOriImageHeight;
			m_nHeight = m_nZPixelNum;
			m_nPiece = m_nOriImageWidth;
		}
		break;
	case 3:
		{
			m_nWidth = m_nOriImageWidth;
			m_nHeight = m_nZPixelNum;
			m_nPiece = m_nOriImageHeight;
		}
		break;
	}


	m_nCuttingPlaneWidth = m_nWidth;
	m_nCuttingPlaneHeight = m_nHeight;

	if (m_pCuttingPlanePoint != NULL)
	{
		delete []m_pCuttingPlanePoint;
		m_pCuttingPlanePoint = NULL;
	}

	m_pCuttingPlanePoint = new Vector3D[m_nCuttingPlaneWidth*m_nCuttingPlaneHeight];
	m_nCuttingPlanePos = m_nPiece/2;

	int i =0;
	for (i = 0; i < m_nCuttingPlaneWidth*m_nCuttingPlaneHeight; i++)
	{
		m_pCuttingPlanePoint[i].x = 0.0;
		m_pCuttingPlanePoint[i].y = 0.0;
		m_pCuttingPlanePoint[i].z = 0.0;

	}

	if (m_sCuttingPlaneData != NULL)
	{
		delete[] m_sCuttingPlaneData;
		m_sCuttingPlaneData = NULL;
	}

	m_sCuttingPlaneData = new short[m_nCuttingPlaneWidth*m_nCuttingPlaneHeight];
	memset(m_sCuttingPlaneData, 0, m_nCuttingPlaneWidth*m_nCuttingPlaneHeight*sizeof(short));

	ResetCuttingPlane();
}

void SeVisualMPR::ResetCuttingPlane()
{
	// 	for (int i = 0; i < m_nCuttingPlaneHeight; i++)
	// 	{
	// 		for (int j = 0; j < m_nCuttingPlaneWidth; j++)
	// 		{
	// 			int nNum = i*m_nCuttingPlaneWidth + j;
	// 			m_pCuttingPlanePoint[nNum].x = m_pCuttingPlanePointOri[nNum].x;
	// 			m_pCuttingPlanePoint[nNum].y = m_pCuttingPlanePointOri[nNum].y;
	// 			m_pCuttingPlanePoint[nNum].z = m_pCuttingPlanePointOri[nNum].z;
	// 		}
	// 	}

	switch (m_nPlaneNum)
	{
	case 1:
		{
			for (int i = 0; i < m_nCuttingPlaneHeight; i++)
			{
				for (int j = 0; j < m_nCuttingPlaneWidth; j++)
				{
					int nNum = i*m_nCuttingPlaneWidth + j;
					m_pCuttingPlanePoint[nNum].x = j;
					m_pCuttingPlanePoint[nNum].y = i;
					m_pCuttingPlanePoint[nNum].z = m_nPiece/2;
				}
			}
		}
		break;

	case 2:
		{
			for (int i = 0; i < m_nCuttingPlaneHeight; i++)
			{
				for (int j = 0; j < m_nCuttingPlaneWidth; j++)
				{
					int nNum = i*m_nCuttingPlaneWidth + j;
					m_pCuttingPlanePoint[nNum].x = m_nPiece/2;			
					m_pCuttingPlanePoint[nNum].y = j;
					m_pCuttingPlanePoint[nNum].z = i;
				}
			}
		}
		break;

	case 3:
		{
			for (int i = 0; i < m_nCuttingPlaneHeight; i++)
			{
				for (int j = 0; j < m_nCuttingPlaneWidth; j++)
				{
					int nNum = i*m_nCuttingPlaneWidth + j;
					m_pCuttingPlanePoint[nNum].x = j;
					m_pCuttingPlanePoint[nNum].y = m_nPiece/2;
					m_pCuttingPlanePoint[nNum].z = i;
				}
			}
		}
		break;
	}
}



void SeVisualMPR::ChangeCuttingPlane()
{
	for (int i = 0; i < m_nCuttingPlaneHeight; i++)
	{
		int nTempI = i*m_nCuttingPlaneWidth;
		for (int j = 0; j < m_nCuttingPlaneWidth; j++)
		{
			int nNum = nTempI + j;
			double dX = m_pCuttingPlanePoint[nNum].x;
			double dY = m_pCuttingPlanePoint[nNum].y;
			double dZ = m_pCuttingPlanePoint[nNum].z/m_dPixelperPiece;

			if (dX < 0 || dX > m_nOriImageWidth - 1 || dY < 0 || dY > m_nOriImageHeight - 1 || dZ < 0 || dZ > m_nOriImagePiece - 1)  //超出体数据范围不做计算
			{
				if (PT_INT_16 == m_DataType)
				{
					m_sCuttingPlaneData[nNum] = 0;
					//					m_sCuttingPlaneData[i*m_nCuttingPlaneLength + j] = 4095;
				}
				else if(PT_UINT_16 == m_DataType || PT_BYTE == m_DataType)
				{
					m_sCuttingPlaneData[nNum] = 0;
				}

			}
			else  //三线性插值
			{
				int nX1 = floor(dX);
				int nX2 = ceil(dX);
				int nY1 = floor(dY);
				int nY2 = ceil(dY); 
				int nZ1 = floor(dZ);
				int nZ2 = ceil(dZ);

				int nOffsetX1 = nX1;
				int nOffsetX2 = nX2;
				int nOffsetY1 = nY1;
				int nOffsetY2 = nY2;
				int nOffsetZ1 = nZ1;
				int nOffsetZ2 = nZ2;

				double dDeltX1 = dX - nX1;
				double dDeltX2 = 1.0 - dDeltX1;
				double dDeltY1 = dY - nY1;
				double dDeltY2 = 1.0 - dDeltY1;
				double dDeltZ1 = dZ - nZ1;
				double dDeltZ2 = 1.0 - dDeltZ1;

				double dColorX1 = m_sChipArray[nOffsetZ1][nOffsetY1*m_nOriImageWidth + nOffsetX1]*dDeltX2 + m_sChipArray[nOffsetZ1][nOffsetY1*m_nOriImageWidth + nOffsetX2]*dDeltX1;
				double dColorX2 = m_sChipArray[nOffsetZ1][nOffsetY2*m_nOriImageWidth + nOffsetX1]*dDeltX2 + m_sChipArray[nOffsetZ1][nOffsetY2*m_nOriImageWidth + nOffsetX2]*dDeltX1;
				double dColorX3 = m_sChipArray[nOffsetZ2][nOffsetY1*m_nOriImageWidth + nOffsetX1]*dDeltX2 + m_sChipArray[nOffsetZ2][nOffsetY1*m_nOriImageWidth + nOffsetX2]*dDeltX1;
				double dColorX4 = m_sChipArray[nOffsetZ2][nOffsetY2*m_nOriImageWidth + nOffsetX1]*dDeltX2 + m_sChipArray[nOffsetZ2][nOffsetY2*m_nOriImageWidth + nOffsetX2]*dDeltX1;

				double dColorY1 = dColorX1*dDeltY2 + dColorX2*dDeltY1; 
				double dColorY2 = dColorX3*dDeltY2 + dColorX4*dDeltY1; 

				double dColor = dColorY1*dDeltZ2 + dColorY2*dDeltZ1;

				m_sCuttingPlaneData[nNum] = (short)dColor;
			}

		}
	}
}

UINT SeVisualMPR::__LinearInterpolation( void* lpVoid )
{
	InterpolationData* pData = (InterpolationData*)lpVoid;
	SeVisualMPR* pThis = (SeVisualMPR*)pData->pthis;
	pThis->LinearInterpolation(pData->i);
	pThis->m_MPRThreadEnd[pData->i] = true;

	bool bTrue = true;
	for (int j = 0; j < 20; j++)
	{
		if (!pThis->m_MPRThreadEnd[j])
		{
			bTrue = false;
			break;
		}
	}
	if (bTrue)
	{
		g_eventDrawMPR.SetEvent();
	}
	delete lpVoid;
	lpVoid = NULL;
	return 0;
}


void SeVisualMPR::LinearInterpolation(int i)
{
	for (int l = i; l < m_nCuttingPlaneHeight; l+=20)
	{
		int nTempI = l*m_nCuttingPlaneWidth;
		for (int j = 0; j < m_nCuttingPlaneWidth; j++)
		{
			int nNum = nTempI + j;
			double dX = m_pCuttingPlanePoint[nNum].x;
			double dY = m_pCuttingPlanePoint[nNum].y;
			double dZ = m_pCuttingPlanePoint[nNum].z/m_dPixelperPiece;

			if (dX < 0 || dX > m_nOriImageWidth - 1 || dY < 0 || dY > m_nOriImageHeight - 1 || dZ < 0 || dZ > m_nOriImagePiece - 1)  //超出体数据范围不做计算
			{
				if (PT_INT_16 == m_DataType)
				{
					m_sCuttingPlaneData[nNum] = 0;
				}
				else if(PT_UINT_16 == m_DataType || PT_BYTE == m_DataType)
				{
					m_sCuttingPlaneData[nNum] = 0;
				}

			}
			else  //三线性插值
			{
				int nX1 = floor(dX);
				int nX2 = ceil(dX);
				int nY1 = floor(dY);
				int nY2 = ceil(dY); 
				int nZ1 = floor(dZ);
				int nZ2 = ceil(dZ);

				int nOffsetX1 = nX1;
				int nOffsetX2 = nX2;
				int nOffsetY1 = nY1;
				int nOffsetY2 = nY2;
				int nOffsetZ1 = nZ1;
				int nOffsetZ2 = nZ2;

				double dDeltX1 = dX - nX1;
				double dDeltX2 = 1.0 - dDeltX1;
				double dDeltY1 = dY - nY1;
				double dDeltY2 = 1.0 - dDeltY1;
				double dDeltZ1 = dZ - nZ1;
				double dDeltZ2 = 1.0 - dDeltZ1;

				double dColorX1 = m_sChipArray[nOffsetZ1][nOffsetY1*m_nOriImageWidth + nOffsetX1]*dDeltX2 + m_sChipArray[nOffsetZ1][nOffsetY1*m_nOriImageWidth + nOffsetX2]*dDeltX1;
				double dColorX2 = m_sChipArray[nOffsetZ1][nOffsetY2*m_nOriImageWidth + nOffsetX1]*dDeltX2 + m_sChipArray[nOffsetZ1][nOffsetY2*m_nOriImageWidth + nOffsetX2]*dDeltX1;
				double dColorX3 = m_sChipArray[nOffsetZ2][nOffsetY1*m_nOriImageWidth + nOffsetX1]*dDeltX2 + m_sChipArray[nOffsetZ2][nOffsetY1*m_nOriImageWidth + nOffsetX2]*dDeltX1;
				double dColorX4 = m_sChipArray[nOffsetZ2][nOffsetY2*m_nOriImageWidth + nOffsetX1]*dDeltX2 + m_sChipArray[nOffsetZ2][nOffsetY2*m_nOriImageWidth + nOffsetX2]*dDeltX1;

				double dColorY1 = dColorX1*dDeltY2 + dColorX2*dDeltY1; 
				double dColorY2 = dColorX3*dDeltY2 + dColorX4*dDeltY1; 

				double dColor = dColorY1*dDeltZ2 + dColorY2*dDeltZ1;

				m_sCuttingPlaneData[nNum] = (short)dColor;
			}
		}
	}
}

void SeVisualMPR::MoveCuttingPlane( int nDis )
{
	m_nCuttingPlanePos += nDis;

	if (m_nCuttingPlanePos < 0)
		m_nCuttingPlanePos = 0;

	// 修改这里：针对不同平面使用正确的最大值
	int nMaxPos = 0;
	if(m_nPlaneNum == 1)
	{
		// XOY平面：使用Z轴的实际像素数（考虑层间距）
		nMaxPos = m_nZPixelNum - 1;
	}
	else
	{
		// 其他平面：使用对应方向的像素数
		nMaxPos = m_nPiece - 1;
	}

	if (m_nCuttingPlanePos > nMaxPos)
		m_nCuttingPlanePos = nMaxPos;

	// 后面的代码保持不变...
	switch (m_nPlaneNum)
	{
	case 1:
		{
			for (int i = 0; i < m_nCuttingPlaneHeight; i++)
			{
				int Index =  i*m_nCuttingPlaneWidth;
				for (int j = 0; j < m_nCuttingPlaneWidth; j++)
				{
					int nNum = Index + j;
					m_pCuttingPlanePoint[nNum].z = m_nCuttingPlanePos;
				}
			}
		}
		break;
	case 2:
		{
			for (int i = 0; i < m_nCuttingPlaneHeight; i++)
			{
				int Index =  i*m_nCuttingPlaneWidth;
				for (int j = 0; j < m_nCuttingPlaneWidth; j++)
				{
					int nNum = Index + j;
					m_pCuttingPlanePoint[nNum].x = m_nCuttingPlanePos;
				}
			}
		}
		break;
	case 3:
		{
			for (int i = 0; i < m_nCuttingPlaneHeight; i++)
			{
				int Index =  i*m_nCuttingPlaneWidth;
				for (int j = 0; j < m_nCuttingPlaneWidth; j++)
				{
					int nNum = Index + j;
					m_pCuttingPlanePoint[nNum].y = m_nCuttingPlanePos;
				}
			}
		}
		break;
	}
}


//void SeVisualMPR::MoveCuttingPlane( int nDis )
//{
//	m_nCuttingPlanePos += nDis;
//
//	if (m_nCuttingPlanePos < 0)
//		m_nCuttingPlanePos = 0;
//
//	if(m_nPlaneNum == 1)
//	{
//		if (m_nCuttingPlanePos >= (m_nOriImagePiece - 1)*m_dPixelperPiece)
//			m_nCuttingPlanePos = (m_nOriImagePiece - 1)*m_dPixelperPiece;
//	}
//	else
//	{
//		if (m_nCuttingPlanePos >= m_nPiece)
//			m_nCuttingPlanePos = m_nPiece - 1;
//	}
//	switch (m_nPlaneNum)
//	{
//	case 1:
//		{
//
//			for (int i = 0; i < m_nCuttingPlaneHeight; i++)
//			{
//				int Index =  i*m_nCuttingPlaneWidth;
//				for (int j = 0; j < m_nCuttingPlaneWidth; j++)
//				{
//					int nNum = Index + j;
//					m_pCuttingPlanePoint[nNum].z = m_nCuttingPlanePos;
//				}
//			}
//		}
//		break;
//	case 2:
//		{
//			for (int i = 0; i < m_nCuttingPlaneHeight; i++)
//			{
//				int Index =  i*m_nCuttingPlaneWidth;
//				for (int j = 0; j < m_nCuttingPlaneWidth; j++)
//				{
//					int nNum = Index + j;
//					m_pCuttingPlanePoint[nNum].x = m_nCuttingPlanePos;
//				}
//			}
//		}
//		break;
//	case 3:
//		{
//			for (int i = 0; i < m_nCuttingPlaneHeight; i++)
//			{
//	    		int Index =  i*m_nCuttingPlaneWidth;
//				for (int j = 0; j < m_nCuttingPlaneWidth; j++)
//				{
//					int nNum = Index + j;
//					m_pCuttingPlanePoint[nNum].y = m_nCuttingPlanePos;
//				}
//			}
//		}
//		break;
//	}
//}

int SeVisualMPR::GetMPRPosition()
{
	return	m_nCuttingPlanePos;
}

CDcmPic* SeVisualMPR::GetMPRImage( int nPlanePos )
{
	if (m_sChipArray == NULL)
		return NULL;

	int nDis = nPlanePos - m_nCuttingPlanePos;
	MoveCuttingPlane(nDis);
	return GetMPRImage();
}

CDcmPic* SeVisualMPR::GetMPRImage()
{
	if (m_sChipArray == NULL)
		return NULL;


	//ChangeCuttingPlane();
	WaitForSingleObject(m_MPRStartThread,INFINITE);             ///////////////MPR multi-Lineprocess
	g_eventDrawMPR.ResetEvent();
	memset(m_MPRThreadEnd,0,sizeof(bool)*20); 
	for (int i = 0; i < 20; i++)
	{
		InterpolationData* interpolationData = new InterpolationData;
		interpolationData->i = i;
		interpolationData->pthis = this;
		AfxBeginThread(__LinearInterpolation,interpolationData);
	}
	WaitForSingleObject(m_hDrawMPR,INFINITE);
	g_MPReventStartThread.ResetEvent();
	g_MPReventStartThread.SetEvent();
	

	short* pDataCpy = new short[m_nCuttingPlaneWidth*m_nCuttingPlaneHeight];
	memset(pDataCpy, 0, m_nCuttingPlaneWidth*m_nCuttingPlaneHeight*sizeof(short));

	memcpy(pDataCpy, m_sCuttingPlaneData, m_nCuttingPlaneWidth*m_nCuttingPlaneHeight*sizeof(short));

	CDcmPic* pDcm = (m_pDcmArray->GetDcmArray()[0])->CloneDcmPic();
	pDcm->SetPixelData((BYTE*)pDataCpy, m_nCuttingPlaneWidth, m_nCuttingPlaneHeight);
	pDcm->AdjustWin(m_nWinCenter, m_nWinWidth);
	return pDcm;
}


int SeVisualMPR::GetPlaneNum()
{
	return m_nPlaneNum;
}

void SeVisualMPR::SetPlaneNum( int nPlaneNum )
{
	m_nPlaneNum = nPlaneNum;
}

int SeVisualMPR::GetMPRWidth()
{
	return m_nWidth;
}

int SeVisualMPR::GetMPRHeight()
{
	return m_nHeight;
}

int SeVisualMPR::GetMPRPiece()
{
	return m_nPiece;
}

int SeVisualMPR::GetCuttingPlaneWidth()
{
	return m_nCuttingPlaneWidth;
}

int SeVisualMPR::GetCuttingPlaneHeight()
{
	return m_nCuttingPlaneHeight;
}

void SeVisualMPR::Reset()
{
	m_pDcmArray = NULL;
	SafeDeleteVec(m_sChipArray);
}

CDcmPicArray* SeVisualMPR::GetDcmArray()
{
	return m_pDcmArray;
}

short** SeVisualMPR::GetChipArray()
{
	return	m_sChipArray;
}

bool SeVisualMPR::GetChipArrayBool()
{
	if (m_sChipArray ==NULL)
	{
		return false;
	}
	else return true;
}

Vector3D* SeVisualMPR::GetCuttingPlanePoint()
{
	return m_pCuttingPlanePoint;
}

void SeVisualMPR::Smooth()
{
	int SmoothKernel[9]={1,1,1,1,1,1,1,1,1};
	for (int k = 0; k < m_nOriImagePiece; k++)
	{
		long DataLength = m_nHeight*m_nWidth;
		short * ptempData = new short[DataLength];
		memset(ptempData,0,DataLength*sizeof(short));
		long temp = 0;
		for (int i = 1; i < m_nHeight-1; i++)
		{
			for (int j = 1; j < m_nWidth-1 ; j++)
			{ 
				temp = *(m_sChipArray[i]+(i-1)*m_nWidth+(j-1))*SmoothKernel[0]+
					*(m_sChipArray[i]+(i-1)*m_nWidth+(j  ))*SmoothKernel[1]+
					*(m_sChipArray[i]+(i-1)*m_nWidth+(j+1))*SmoothKernel[2]+
					*(m_sChipArray[i]+(i  )*m_nWidth+(j-1))*SmoothKernel[3]+
					*(m_sChipArray[i]+(i  )*m_nWidth+(j  ))*SmoothKernel[4]+
					*(m_sChipArray[i]+(i  )*m_nWidth+(j+1))*SmoothKernel[5]+
					*(m_sChipArray[i]+(i+1)*m_nWidth+(j-1))*SmoothKernel[6]+
					*(m_sChipArray[i]+(i+1)*m_nWidth+(j  ))*SmoothKernel[7]+
					*(m_sChipArray[i]+(i+1)*m_nWidth+(j+1))*SmoothKernel[8];
				*(ptempData+(i)*m_nWidth+(j))=(short)((float)temp/9.0);
			}
		}
		memset(m_sChipArray[k],0,DataLength*sizeof(short));
		memcpy(m_sChipArray[k],ptempData,DataLength*sizeof(short));
		delete []ptempData;
		ptempData=NULL;
	}

}


void SeVisualMPR::SetWinLevel(int nWinCenter, int nWinwidth)
{
	m_nWinCenter = nWinCenter;
	m_nWinWidth = nWinwidth;
}
