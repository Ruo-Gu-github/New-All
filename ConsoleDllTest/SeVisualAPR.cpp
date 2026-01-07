
#include "stdafx.h"
#include "SeVisualAPR.h"


CRITICAL_SECTION	g_cs;
CEvent				g_eventStartThread(FALSE,TRUE);
CEvent				g_eventDrawAPR(FALSE,TRUE);	

int SeVisualAPR::m_nWinCenter = 2048;
int SeVisualAPR::m_nWinWidth = 4096;

SeVisualAPR::SeVisualAPR()
{
	m_pDcmArray = NULL;
	m_sChipArray = NULL;
//	m_pAngleXOY = m_pAngleYOZ = m_pAngleXOZ = NULL;

	m_fAPRNormal_X = m_fAPRNormal_Y = m_fAPRNormal_Z = 0.0;
	m_dAngleXOY = m_dAngleYOZ = m_dAngleXOZ = 0.0;

	m_pCuttingPlanePoint = NULL;
//	m_pCuttingPlanePointOri = NULL;
	m_sCuttingPlaneData = NULL;

	m_nMIPPieceNum = 1;

	m_bMipDownSampling = false;
	m_bMIPRotateDown = true;
	m_bInterplote = false;

	m_hStartThread = g_eventStartThread;
	m_hDrawAPR = g_eventDrawAPR;
	m_bRotateorTrans = true;
	m_nSlabMode = "MIP";

	m_bXOYParallel = false;
	m_bYOZParallel = false;
	m_bXOZParallel = false;
}


SeVisualAPR::~SeVisualAPR()
{
	Safe_DeleteVec(m_pCuttingPlanePoint);
//	Safe_Delete(m_pCuttingPlanePointOri);
	Safe_DeleteVec(m_sCuttingPlaneData);
	Safe_DeleteVec(m_sChipArray);
// 	Safe_Delete(m_pAngleXOY);
// 	Safe_Delete(m_pAngleYOZ);
// 	Safe_Delete(m_pAngleXOZ);
}

void SeVisualAPR::SetMipThick(int nThick)
{
	if (nThick<1)
		m_nMIPPieceNum = 1;
	else
		m_nMIPPieceNum = nThick;
}

void SeVisualAPR::SetDIProtateDown(int DownSalple)
{
	m_bMIPRotateDown = DownSalple;
}

void SeVisualAPR::SetDIPInterplote(int Interpolation)
{
	m_bInterplote = Interpolation;
}


void SeVisualAPR::SetDcmArray(CDcmPicArray* pArray)
{
	m_pDcmArray = pArray;
	CString	csMinValue;
	CDcmElement* pElement = m_pDcmArray->GetDcmArray()[0]->GetDcmElemList().FindElem(0x0028, 0x1052);
	if (pElement != NULL)
	{
		pElement->ValueGet(csMinValue);
		m_sInitValue = _ttoi(csMinValue);
	}
	else
		m_sInitValue = -1024;
	

	SetCuttingPlane();

//	InitializeCriticalSection(&g_cs);

	g_eventStartThread.SetEvent();
	g_eventDrawAPR.SetEvent();
}

CDcmPic* SeVisualAPR::GetAPRImage(double dRotateMatrix[16], double dCenterX, double dCenterY, double dCenterZ, bool bMipDownSampling)
{
	if (m_sChipArray == NULL)
		return NULL;

	memcpy(m_dRotateMatrix, dRotateMatrix, sizeof(double)*16);
	m_dCenterX = dCenterX;
	m_dCenterY = dCenterY;
	m_dCenterZ = dCenterZ;
	m_bMipDownSampling = bMipDownSampling;

	ChangeCuttingPlane();

	short* pDataCpy = new short[m_nCuttingPlaneWidth*m_nCuttingPlaneHeight];
	memset(pDataCpy, 0, m_nCuttingPlaneWidth*m_nCuttingPlaneHeight*sizeof(short));

//	EnterCriticalSection(&g_cs);
	memcpy(pDataCpy, m_sCuttingPlaneData, m_nCuttingPlaneWidth*m_nCuttingPlaneHeight*sizeof(short));
//	LeaveCriticalSection(&g_cs);

	CDcmPic* pDcm = (m_pDcmArray->GetDcmArray()[0])->CloneDcmPic();
	pDcm->SetPixelData((BYTE*)pDataCpy, m_nCuttingPlaneWidth, m_nCuttingPlaneHeight); //?????????????????????copy????????????????
//	g_eventDrawdcm.SetEvent();

	return pDcm;
}

Vector3D* SeVisualAPR::GetCuttingPlanePoint()
{
	return m_pCuttingPlanePoint;
}

int SeVisualAPR::GetCuttingPlaneWidth()
{
	return m_nCuttingPlaneWidth;
} 

int SeVisualAPR::GetCuttingPlaneHeight()
{
	return m_nCuttingPlaneHeight;
}

Vector3D SeVisualAPR::GetAPRNormal()
{
	Vector3D vNormal;
	vNormal.x = m_fAPRNormal_X;
	vNormal.y = m_fAPRNormal_Y;
	vNormal.z = m_fAPRNormal_Z;
	return vNormal;
}

void SeVisualAPR::SetCuttingPlane()
{
	m_nOriImageWidth = m_pDcmArray->GetDcmArray()[0]->GetWidth();
	m_nOriImageHeight = m_pDcmArray->GetDcmArray()[0]->GetHeight();
	m_nOriImagePiece = m_pDcmArray->GetZDcmPicCount();
	m_nZPixelNum = m_pDcmArray->GetZAxialPixelNumber();
	if (m_nZPixelNum>1000)
	{
		m_nZPixelNum = 1000;
	}//?????
	m_DataType = m_pDcmArray->GetDcmArray()[0]->GetPixelDataType();
	m_dPixelperPiece = m_nZPixelNum*1.0/m_nOriImagePiece;

	Safe_Delete(m_sChipArray);
	m_sChipArray = new short*[m_nOriImagePiece];

	for (int i = 0; i < m_nOriImagePiece; i++)
	{
		m_sChipArray[i] = (short*)(m_pDcmArray->GetDcmArray()[i])->GetData();
	}


	m_nCuttingPlaneWidth =  (int)sqrt((double)m_nOriImageWidth*m_nOriImageWidth + m_nOriImageHeight*m_nOriImageHeight + m_nZPixelNum*m_nZPixelNum);
	m_nCuttingPlaneHeight = m_nCuttingPlaneWidth = WIDTHBYTES(m_nCuttingPlaneWidth) * 8;

	Safe_DeleteVec(m_pCuttingPlanePoint);

//	Safe_DeleteVec(m_pCuttingPlanePointOri);

	m_pCuttingPlanePoint = new Vector3D[m_nCuttingPlaneWidth*m_nCuttingPlaneHeight];
//	m_pCuttingPlanePointOri = new Vector3D[m_nCuttingPlaneWidth*m_nCuttingPlaneHeight];

	for (int i = 0; i < m_nCuttingPlaneWidth*m_nCuttingPlaneHeight; i++)
	{
		m_pCuttingPlanePoint[i].x = 0.0;
		m_pCuttingPlanePoint[i].y = 0.0;
		m_pCuttingPlanePoint[i].z = 0.0;

// 		m_pCuttingPlanePointOri[i].x = 0.0;
// 		m_pCuttingPlanePointOri[i].y = 0.0;
// 		m_pCuttingPlanePointOri[i].z = 0.0;
	}

	m_sCuttingPlaneData = new short[m_nCuttingPlaneWidth*m_nCuttingPlaneHeight];
	memset(m_sCuttingPlaneData, 0, m_nCuttingPlaneWidth*m_nCuttingPlaneHeight*sizeof(short));

// 	for (i = -m_nCuttingPlaneHeight/2; i < m_nCuttingPlaneHeight/2; i++)
// 	{
// 		for (int j = -m_nCuttingPlaneWidth/2; j < m_nCuttingPlaneWidth/2; j++)
// 		{
// 			int nNum = (i + m_nCuttingPlaneHeight/2)*m_nCuttingPlaneWidth + (j + m_nCuttingPlaneWidth/2);
// 			m_pCuttingPlanePointOri[nNum].x = j;
// 			m_pCuttingPlanePointOri[nNum].y = i;
// 			m_pCuttingPlanePointOri[nNum].z = 0.0;
// 		}
// 	}

}


void SeVisualAPR::ResetCuttingPlane()
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

	for (int i = -m_nCuttingPlaneHeight/2; i < m_nCuttingPlaneHeight/2; i++)
	{
		for (int j = -m_nCuttingPlaneWidth/2; j < m_nCuttingPlaneWidth/2; j++)
		{
			int nNum = (i + m_nCuttingPlaneHeight/2)*m_nCuttingPlaneWidth + (j + m_nCuttingPlaneWidth/2);
			m_pCuttingPlanePoint[nNum].x = j;
			m_pCuttingPlanePoint[nNum].y = i;
			m_pCuttingPlanePoint[nNum].z = 0.0;
		}
	}
}

void SeVisualAPR::ComputeAPRNormal()
{
	float fPoint1X = m_pCuttingPlanePoint[0].x;
	float fPoint1Y = m_pCuttingPlanePoint[0].y;
	float fPoint1Z = m_pCuttingPlanePoint[0].z;

	float fPoint2X = m_pCuttingPlanePoint[m_nCuttingPlaneWidth-1].x;
	float fPoint2Y = m_pCuttingPlanePoint[m_nCuttingPlaneWidth-1].y;
	float fPoint2Z = m_pCuttingPlanePoint[m_nCuttingPlaneWidth-1].z;

	float fPoint3X = m_pCuttingPlanePoint[m_nCuttingPlaneWidth*(m_nCuttingPlaneHeight-1)].x;
	float fPoint3Y = m_pCuttingPlanePoint[m_nCuttingPlaneWidth*(m_nCuttingPlaneHeight-1)].y;
	float fPoint3Z = m_pCuttingPlanePoint[m_nCuttingPlaneWidth*(m_nCuttingPlaneHeight-1)].z;

	float fVec1X = fPoint2X - fPoint1X;
	float fVec1Y = fPoint2Y - fPoint1Y;
	float fVec1Z = fPoint2Z - fPoint1Z;

	float fVec2X = fPoint3X - fPoint1X;
	float fVec2Y = fPoint3Y - fPoint1Y;
	float fVec2Z = fPoint3Z - fPoint1Z;

	m_fAPRNormal_X = fVec1Y*fVec2Z - fVec1Z*fVec2Y;
	m_fAPRNormal_Y = fVec2X*fVec1Z - fVec1X*fVec2Z;
	m_fAPRNormal_Z = fVec1X*fVec2Y - fVec1Y*fVec2X;

	float fModule = sqrt(m_fAPRNormal_X*m_fAPRNormal_X + m_fAPRNormal_Y*m_fAPRNormal_Y + m_fAPRNormal_Z*m_fAPRNormal_Z);
	m_fAPRNormal_X = m_fAPRNormal_X/fModule;
	m_fAPRNormal_Y = m_fAPRNormal_Y/fModule;
	m_fAPRNormal_Z = m_fAPRNormal_Z/fModule;
}

void SeVisualAPR::Compute2DAPRLine()
{
	float fPoint1X = m_pCuttingPlanePoint[0].x - m_dCenterX;
	float fPoint1Y = m_pCuttingPlanePoint[0].y - m_dCenterY;
	float fPoint1Z = m_pCuttingPlanePoint[0].z - m_dCenterZ;

	float fPoint2X = m_pCuttingPlanePoint[m_nCuttingPlaneWidth-1].x - m_dCenterX;
	float fPoint2Y = m_pCuttingPlanePoint[m_nCuttingPlaneWidth-1].y - m_dCenterY;
	float fPoint2Z = m_pCuttingPlanePoint[m_nCuttingPlaneWidth-1].z - m_dCenterZ;

	float fPoint3X = m_pCuttingPlanePoint[m_nCuttingPlaneWidth*(m_nCuttingPlaneHeight-1)].x - m_dCenterX;
	float fPoint3Y = m_pCuttingPlanePoint[m_nCuttingPlaneWidth*(m_nCuttingPlaneHeight-1)].y - m_dCenterY;
	float fPoint3Z = m_pCuttingPlanePoint[m_nCuttingPlaneWidth*(m_nCuttingPlaneHeight-1)].z - m_dCenterZ;

	double dAngleXOY = 0.0;
	double dAngleYOZ = 0.0;
	double dAngleXOZ = 0.0;

	m_bXOYParallel = false;
	m_bYOZParallel = false;
	m_bXOZParallel = false;

	if (fabs(fPoint1X) == 0.0 && fabs(fPoint2X) == 0.0 && fabs(fPoint3X) == 0.0) //????????????????????
	{
		dAngleXOY = 90.0;
		dAngleXOZ = 90.0;
		m_bYOZParallel = true;
	}
	else if (fabs(fPoint1Y) == 0.0 && fabs(fPoint2Y) == 0.0 && fabs(fPoint3Y) == 0.0)
	{
		dAngleXOY = 0.0;
		dAngleYOZ = 0.0;
		m_bXOZParallel = true;
	}
	else if (fabs(fPoint1Z) == 0.0 && fabs(fPoint2Z) == 0.0 && fabs(fPoint3Z) == 0.0)
	{
		dAngleYOZ = 0.0;
		dAngleXOZ = 0.0;
		m_bXOYParallel = true;
	}
	else
	{
		if ((fabs(fPoint1Z)/(m_nCuttingPlaneWidth/1.414) <= 0.0849) && 
			(fabs(fPoint2Z)/(m_nCuttingPlaneWidth/1.414) <= 0.0849) && 
			(fabs(fPoint3Z)/(m_nCuttingPlaneWidth/1.414) <= 0.0849))  //??????????????????§Ò??£??§µ?????????
		{
			m_bXOYParallel = true;
		}
		else
		{
			double dXYDeltY = fPoint1Y*fPoint2Z - fPoint2Y*fPoint1Z + fPoint2Y*fPoint3Z - fPoint3Y*fPoint2Z;
			double dXYDeltX = fPoint1X*fPoint2Z - fPoint2X*fPoint1Z + fPoint2X*fPoint3Z - fPoint3X*fPoint2Z;

			double dAngleXOYRadian = atan2(dXYDeltY, dXYDeltX);
			dAngleXOY = dAngleXOYRadian*180.0/PI;
		}

		if ((fabs(fPoint1X)/(m_nCuttingPlaneWidth/1.414) <= 0.0849) && 
			(fabs(fPoint2X)/(m_nCuttingPlaneWidth/1.414) <= 0.0849) && 
			(fabs(fPoint3X)/(m_nCuttingPlaneWidth/1.414) <= 0.0849))
		{
			m_bYOZParallel = true;
		}
		else
		{
			double dYZDeltZ = fPoint2X*fPoint1Z - fPoint1X*fPoint2Z + fPoint2X*fPoint3Z - fPoint3X*fPoint2Z;
			double dYZDeltY = fPoint2X*fPoint1Y - fPoint1X*fPoint2Y + fPoint2X*fPoint3Y - fPoint3X*fPoint2Y;

			double dAngleYOZRadian = atan2(dYZDeltZ, dYZDeltY);
			dAngleYOZ = dAngleYOZRadian*180.0/PI;
		}
		if ((fabs(fPoint1Y)/(m_nCuttingPlaneWidth/1.414) <= 0.0849) && 
			(fabs(fPoint2Y)/(m_nCuttingPlaneWidth/1.414) <= 0.0849) && 
			(fabs(fPoint3Y)/(m_nCuttingPlaneWidth/1.414) <= 0.0849))
		{
			m_bXOZParallel = true;
		}
		else
		{
			double dXZDeltZ = fPoint2Y*fPoint1Z - fPoint1Y*fPoint2Z + fPoint3Y*fPoint2Z - fPoint2Y*fPoint3Z;
			double dXZDeltX = fPoint1X*fPoint2Y - fPoint2X*fPoint1Y + fPoint2X*fPoint3Y - fPoint3X*fPoint2Y;

			double dAngleXOZRadian = atan2(dXZDeltZ, dXZDeltX);
			dAngleXOZ = dAngleXOZRadian*180.0/PI;
		}

	}
	m_dAngleXOY = -dAngleXOY;
	m_dAngleYOZ = -dAngleYOZ;
	m_dAngleXOZ = -dAngleXOZ;
}


void SeVisualAPR::ChangeCuttingPlane()
{
	double dModelViewCpy[16];
	memset(dModelViewCpy, 0, sizeof(double)*16);

//	EnterCriticalSection(&g_cs);
	memcpy(dModelViewCpy, m_dRotateMatrix, sizeof(double)*16);   //???????????????????????????????????????????§Õ???
//	LeaveCriticalSection(&g_cs);
	////////////////////////////////////////////////???3?????????//////////////////////////////////////////

	ResetCuttingPlane();

	////////////////////////////////////////////////????????////////////////////////////////////////////////	

	int nOriNum = -m_nCuttingPlaneWidth/2;
	int nDstNum = m_nCuttingPlaneWidth/2;
	int nOffset = m_nCuttingPlaneWidth/2;

	int i= 0;
	for (i = nOriNum; i < nDstNum; i++)
	{
		int nTempI = (i + nOffset)*m_nCuttingPlaneWidth;
		for (int j = nOriNum; j < nDstNum; j++)
		{
			int nNum = nTempI + (j + nOffset);
			float fX = m_pCuttingPlanePoint[nNum].x * dModelViewCpy[0] + m_pCuttingPlanePoint[nNum].y * dModelViewCpy[4]
			+ m_pCuttingPlanePoint[nNum].z * dModelViewCpy[8];
			float fY = m_pCuttingPlanePoint[nNum].x * dModelViewCpy[1] + m_pCuttingPlanePoint[nNum].y * dModelViewCpy[5]
			+ m_pCuttingPlanePoint[nNum].z * dModelViewCpy[9];
			float fZ = m_pCuttingPlanePoint[nNum].x * dModelViewCpy[2] + m_pCuttingPlanePoint[nNum].y * dModelViewCpy[6]
			+ m_pCuttingPlanePoint[nNum].z * dModelViewCpy[10];

			m_pCuttingPlanePoint[nNum].x = fX + m_dCenterX;
			m_pCuttingPlanePoint[nNum].y = fY + m_dCenterY;
			m_pCuttingPlanePoint[nNum].z = fZ + m_dCenterZ;
		}
	}

	if (m_bRotateorTrans)
	{
		ComputeAPRNormal();
		Compute2DAPRLine();
	}
	
	if (m_nMIPPieceNum == 1)
	{
		
		for (i = 0; i < m_nCuttingPlaneHeight; i++)
		{
			int nTempI = i*m_nCuttingPlaneWidth;
			for (int j = 0; j < m_nCuttingPlaneWidth; j++)
			{
				int nNum = nTempI + j;
				double dX = m_pCuttingPlanePoint[nNum].x;
				double dY = m_pCuttingPlanePoint[nNum].y;
				double dZ = m_pCuttingPlanePoint[nNum].z/m_dPixelperPiece;

				if (dX < -m_nOriImageWidth/2 || dX >= m_nOriImageWidth/2-1 || dY < -m_nOriImageHeight/2 || dY >= m_nOriImageHeight/2-1 || dZ < -m_nOriImagePiece/2 || dZ >= m_nOriImagePiece/2-1)  //???????????¦¶????????
					m_sCuttingPlaneData[i*m_nCuttingPlaneWidth + j] = m_sInitValue;
				else  //????????
				{
					int nX1 = floor(dX);
					int nX2 = ceil(dX);
					int nY1 = floor(dY);
					int nY2 = ceil(dY); 
					int nZ1 = floor(dZ);
					int nZ2 = ceil(dZ);

					int nOffsetX1 = nX1+m_nOriImageWidth/2;
					int nOffsetX2 = nX2+m_nOriImageWidth/2;
					int nOffsetY1 = nY1+m_nOriImageHeight/2;
					int nOffsetY2 = nY2+m_nOriImageHeight/2;
					int nOffsetZ1 = nZ1+m_nOriImagePiece/2;
					int nOffsetZ2 = nZ2+m_nOriImagePiece/2;

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

					m_sCuttingPlaneData[i*m_nCuttingPlaneWidth +j] = (short)dColor;
				}
			}
		}
		
	}
	else
	{
		if (m_bMipDownSampling&&m_bMIPRotateDown)
		{
			m_nSampleNum = m_nMIPPieceNum/20;
			if (m_nSampleNum < 1)
			{
				m_nSampleNum = 1;
			}
		}
		else
		{
			m_nSampleNum = 1;
		}
		WaitForSingleObject(m_hStartThread,INFINITE);
		g_eventDrawAPR.ResetEvent();

		CUDA_Interpolation();
	}
	WaitForSingleObject(m_hDrawAPR,INFINITE);
	g_eventStartThread.ResetEvent();
	g_eventStartThread.SetEvent();
}

typedef short*(*MIPFUNC14) (short** sChipArray,int width, int height,int MIPPieceNum,int SampleNum, double dPixelperPiece,Vector3D* pCuttingPlanePoint,
	float fAPRNormal_X,float fAPRNormal_Y,float fAPRNormal_Z, int nOriImageWidth,int nOriImageHeight,int nOriImagePiece,short sInitValue);

void SeVisualAPR::CUDA_Interpolation()
{    
// 	HINSTANCE hcudaDll=LoadLibrary(__T("SeGeneralMIP.dll"));//????????smooth.dll
// 	if (hcudaDll)
// 	{
// 		MIPFUNC14 dllFun=(MIPFUNC14)GetProcAddress(hcudaDll,"MIPCuttingPlane");//??¨²??????
// 		if (dllFun)
// 		{
// 			short* result = dllFun(m_sChipArray,m_nCuttingPlaneWidth,m_nCuttingPlaneHeight,m_nMIPPieceNum,m_nSampleNum, m_dPixelperPiece,m_pCuttingPlanePoint,
// 				m_fAPRNormal_X,m_fAPRNormal_Y,m_fAPRNormal_Z, m_nOriImageWidth,m_nOriImageHeight,m_nOriImagePiece,m_sInitValue);
// 			memcpy(m_sCuttingPlaneData,result,m_nCuttingPlaneHeight * m_nCuttingPlaneWidth * sizeof(short));
// 			delete []result;
// 			result = NULL;
// 		}
// 		else
// 		{
// 			AfxMessageBox("Load Function Failed");
// 		}
// 		FreeLibrary(hcudaDll);//?????§Ø??CUDAdlltest.dll
// 		g_eventDrawAPR.SetEvent();
// 	}
// 	else
// 	{
		memset(m_bThreadEnd,0,sizeof(bool)*20); 
		for (int i = 0; i < 20; i++)
		{
			InterpolationData* interpolationData = new InterpolationData;
			interpolationData->i = i;
			interpolationData->pthis = this;
			AfxBeginThread(__LinearInterpolation,interpolationData);
		}
//	}
}


void SeVisualAPR::MIPLinearInterpolation(int i,int Interpolation)
{
	int num1 = m_nCuttingPlaneHeight/20*i;
	int num2 = m_nCuttingPlaneHeight/20*(i+1);
	int nTempK = (m_nMIPPieceNum - 1)/2;
	int num = m_nMIPPieceNum/m_nSampleNum;
	double dNormal_X = (nTempK+1)*m_fAPRNormal_X;
	double dNormal_Y = (nTempK+1)*m_fAPRNormal_Y;
	double dNormal_Z = (nTempK+1)*m_fAPRNormal_Z;
	double dNormal_X1 = m_nSampleNum*m_fAPRNormal_X;
	double dNormal_Y1 = m_nSampleNum*m_fAPRNormal_Y;
	double dNormal_Z1 = m_nSampleNum*m_fAPRNormal_Z;
	for(int l = num1;l < num2 ; l++)
	{
		int nTempI = l*m_nCuttingPlaneWidth;
		for (int j = 0; j < m_nCuttingPlaneWidth; j++)
		{
			int nNum = nTempI + j;
			double dX = m_pCuttingPlanePoint[nNum].x - dNormal_X + m_nOriImageWidth/2;
			double dY = m_pCuttingPlanePoint[nNum].y - dNormal_Y + m_nOriImageHeight/2;
			double dZ = m_pCuttingPlanePoint[nNum].z/m_dPixelperPiece - dNormal_Z + m_nOriImagePiece/2;
			short dTempData = -1024;
			int nOutsideNumber = 0;
			for (int k = 0 ; k < num ; k++ )
			{
				dX += dNormal_X1;// + m_fAPRNormal_X + m_fAPRNormal_X;
				dY += dNormal_Y1;// + m_fAPRNormal_Y + m_fAPRNormal_Y;
				dZ += dNormal_Z1;// + m_fAPRNormal_Z + m_fAPRNormal_Z;

				if (dX < 0 || dX >= m_nOriImageWidth-1 || dY < 0 || dY >= m_nOriImageHeight-1 || dZ < 0 || dZ >= m_nOriImagePiece-1)  //???????????¦¶????????
				{
					nOutsideNumber++;
					continue;
				}

				int nX1 = floor(dX);     ////////////////////??????????
				int nX2 = ceil(dX);
				int nY1 = floor(dY);
				int nY2 = ceil(dY);
				int nZ1 = floor(dZ);
				int nZ2 = ceil(dZ);

				double dDeltX1 = dX - nX1;
				double dDeltX2 = 1.0 - dDeltX1;
				double dDeltY1 = dY - nY1;
				double dDeltY2 = 1.0 - dDeltY1;
				double dDeltZ1 = dZ - nZ1;
				double dDeltZ2 = 1.0 - dDeltZ1;

				double dColorX1 = m_sChipArray[nZ1][nY1*m_nOriImageWidth + nX1]*dDeltX2 + m_sChipArray[nZ1][nY1*m_nOriImageWidth + nX2]*dDeltX1;
				double dColorX2 = m_sChipArray[nZ1][nY2*m_nOriImageWidth + nX1]*dDeltX2 + m_sChipArray[nZ1][nY2*m_nOriImageWidth + nX2]*dDeltX1;
				double dColorX3 = m_sChipArray[nZ2][nY1*m_nOriImageWidth + nX1]*dDeltX2 + m_sChipArray[nZ2][nY1*m_nOriImageWidth + nX2]*dDeltX1;
				double dColorX4 = m_sChipArray[nZ2][nY2*m_nOriImageWidth + nX1]*dDeltX2 + m_sChipArray[nZ2][nY2*m_nOriImageWidth + nX2]*dDeltX1;

				double dColorY1 = dColorX1*dDeltY2 + dColorX2*dDeltY1; 
				double dColorY2 = dColorX3*dDeltY2 + dColorX4*dDeltY1; 

				double dColor = dColorY1*dDeltZ2 + dColorY2*dDeltZ1;

				if (dTempData < dColor)
				{
					dTempData = dColor;
				}		
			}
			if (nOutsideNumber == num)
			    m_sCuttingPlaneData[nNum] = m_sInitValue;
			else
				m_sCuttingPlaneData[nNum] = dTempData;
		}
	}
}

void SeVisualAPR::MinIPLinearInterpolation(int i,int Interpolation)
{
	int num1 = m_nCuttingPlaneHeight/20*i;
	int num2 = m_nCuttingPlaneHeight/20*(i+1);
	int nTempK = (m_nMIPPieceNum - 1)/2;
	int num = m_nMIPPieceNum/m_nSampleNum;
	double dNormal_X = (nTempK+1)*m_fAPRNormal_X;
	double dNormal_Y = (nTempK+1)*m_fAPRNormal_Y;
	double dNormal_Z = (nTempK+1)*m_fAPRNormal_Z;
	double dNormal_X1 = m_nSampleNum*m_fAPRNormal_X;
	double dNormal_Y1 = m_nSampleNum*m_fAPRNormal_Y;
	double dNormal_Z1 = m_nSampleNum*m_fAPRNormal_Z;
	for(int l = num1;l < num2 ; l++)
	{
		int nTempI = l*m_nCuttingPlaneWidth;
		for (int j = 0; j < m_nCuttingPlaneWidth; j++)
		{
			int nNum = nTempI + j;
			double dX = m_pCuttingPlanePoint[nNum].x - dNormal_X + m_nOriImageWidth/2;
			double dY = m_pCuttingPlanePoint[nNum].y - dNormal_Y + m_nOriImageHeight/2;
			double dZ = m_pCuttingPlanePoint[nNum].z/m_dPixelperPiece - dNormal_Z + m_nOriImagePiece/2;
			short dTempData = 4096;
			int nOutsideNumber = 0;
			for (int k = 0 ; k < num ; k++ )
			{
				dX += dNormal_X1;// + m_fAPRNormal_X + m_fAPRNormal_X;
				dY += dNormal_Y1;// + m_fAPRNormal_Y + m_fAPRNormal_Y;
				dZ += dNormal_Z1;// + m_fAPRNormal_Z + m_fAPRNormal_Z;

				if (dX < 0 || dX >= m_nOriImageWidth-1 || dY < 0 || dY >= m_nOriImageHeight-1 || dZ < 0 || dZ >= m_nOriImagePiece-1)  //???????????¦¶????????
				{
					nOutsideNumber++;
					continue;
				}

				int nX1 = floor(dX);
				int nX2 = ceil(dX);
				int nY1 = floor(dY);
				int nY2 = ceil(dY); 
				int nZ1 = floor(dZ);
				int nZ2 = ceil(dZ);

				double dDeltX1 = dX - nX1;
				double dDeltX2 = 1.0 - dDeltX1;
				double dDeltY1 = dY - nY1;
				double dDeltY2 = 1.0 - dDeltY1;
				double dDeltZ1 = dZ - nZ1;
				double dDeltZ2 = 1.0 - dDeltZ1;

				double dColorX1 = m_sChipArray[nZ1][nY1*m_nOriImageWidth + nX1]*dDeltX2 + m_sChipArray[nZ1][nY1*m_nOriImageWidth + nX2]*dDeltX1;
				double dColorX2 = m_sChipArray[nZ1][nY2*m_nOriImageWidth + nX1]*dDeltX2 + m_sChipArray[nZ1][nY2*m_nOriImageWidth + nX2]*dDeltX1;
				double dColorX3 = m_sChipArray[nZ2][nY1*m_nOriImageWidth + nX1]*dDeltX2 + m_sChipArray[nZ2][nY1*m_nOriImageWidth + nX2]*dDeltX1;
				double dColorX4 = m_sChipArray[nZ2][nY2*m_nOriImageWidth + nX1]*dDeltX2 + m_sChipArray[nZ2][nY2*m_nOriImageWidth + nX2]*dDeltX1;

				double dColorY1 = dColorX1*dDeltY2 + dColorX2*dDeltY1; 
				double dColorY2 = dColorX3*dDeltY2 + dColorX4*dDeltY1; 

				double dColor = dColorY1*dDeltZ2 + dColorY2*dDeltZ1;

				if (dTempData > dColor)
				{
					dTempData = dColor;
				}	
			}
			if (nOutsideNumber == num)
				m_sCuttingPlaneData[nNum] = m_sInitValue;
			else
				m_sCuttingPlaneData[nNum] = dTempData;
		}
	}
}

void SeVisualAPR::AverageLinearInterpolation(int i,int Interpolation)
{
	int num1 = m_nCuttingPlaneHeight/20*i;
	int num2 = m_nCuttingPlaneHeight/20*(i+1);
	int nTempK = (m_nMIPPieceNum - 1)/2;
	int num = m_nMIPPieceNum/m_nSampleNum;
	double dNormal_X = (nTempK+1)*m_fAPRNormal_X;
	double dNormal_Y = (nTempK+1)*m_fAPRNormal_Y;
	double dNormal_Z = (nTempK+1)*m_fAPRNormal_Z;
	double dNormal_X1 = m_nSampleNum*m_fAPRNormal_X;
	double dNormal_Y1 = m_nSampleNum*m_fAPRNormal_Y;
	double dNormal_Z1 = m_nSampleNum*m_fAPRNormal_Z;
	for(int l = num1;l < num2 ; l++)
	{
		int nTempI = l*m_nCuttingPlaneWidth;
		for (int j = 0; j < m_nCuttingPlaneWidth; j++)
		{
			int nNum = nTempI + j;
			double dX = m_pCuttingPlanePoint[nNum].x - dNormal_X + m_nOriImageWidth/2;
			double dY = m_pCuttingPlanePoint[nNum].y - dNormal_Y + m_nOriImageHeight/2;
			double dZ = m_pCuttingPlanePoint[nNum].z/m_dPixelperPiece - dNormal_Z + m_nOriImagePiece/2;
			int nTempData = 0;
			int nOutsideNumber = 0;
			for (int k = 0 ; k < num ; k++ )
			{
				dX += dNormal_X1;// + m_fAPRNormal_X + m_fAPRNormal_X;
				dY += dNormal_Y1;// + m_fAPRNormal_Y + m_fAPRNormal_Y;
				dZ += dNormal_Z1;// + m_fAPRNormal_Z + m_fAPRNormal_Z;

				if (dX < 0 || dX >= m_nOriImageWidth-1 || dY < 0 || dY >= m_nOriImageHeight-1 || dZ < 0 || dZ >= m_nOriImagePiece-1)  //???????????¦¶????????
				{
					nOutsideNumber++;
					continue;
				}

				int nX1 = floor(dX);
				int nX2 = ceil(dX);
				int nY1 = floor(dY);
				int nY2 = ceil(dY); 
				int nZ1 = floor(dZ);
				int nZ2 = ceil(dZ);

				double dDeltX1 = dX - nX1;
				double dDeltX2 = 1.0 - dDeltX1;
				double dDeltY1 = dY - nY1;
				double dDeltY2 = 1.0 - dDeltY1;
				double dDeltZ1 = dZ - nZ1;
				double dDeltZ2 = 1.0 - dDeltZ1;

				double dColorX1 = m_sChipArray[nZ1][nY1*m_nOriImageWidth + nX1]*dDeltX2 + m_sChipArray[nZ1][nY1*m_nOriImageWidth + nX2]*dDeltX1;
				double dColorX2 = m_sChipArray[nZ1][nY2*m_nOriImageWidth + nX1]*dDeltX2 + m_sChipArray[nZ1][nY2*m_nOriImageWidth + nX2]*dDeltX1;
				double dColorX3 = m_sChipArray[nZ2][nY1*m_nOriImageWidth + nX1]*dDeltX2 + m_sChipArray[nZ2][nY1*m_nOriImageWidth + nX2]*dDeltX1;
				double dColorX4 = m_sChipArray[nZ2][nY2*m_nOriImageWidth + nX1]*dDeltX2 + m_sChipArray[nZ2][nY2*m_nOriImageWidth + nX2]*dDeltX1;

				double dColorY1 = dColorX1*dDeltY2 + dColorX2*dDeltY1; 
				double dColorY2 = dColorX3*dDeltY2 + dColorX4*dDeltY1; 

				double dColor = dColorY1*dDeltZ2 + dColorY2*dDeltZ1;

				nTempData += dColor;	
			}
			if (nOutsideNumber == num)
				m_sCuttingPlaneData[nNum] = m_sInitValue;
			else
				m_sCuttingPlaneData[nNum] = nTempData/num;
		}
	}
}

UINT SeVisualAPR::__LinearInterpolation( void* lpVoid )
{
	InterpolationData* pData = (InterpolationData*)lpVoid;
	SeVisualAPR* pThis = (SeVisualAPR*)pData->pthis;

	if (pThis->m_nSlabMode == "MIP")         
		pThis->MIPLinearInterpolation(pData->i,pThis->m_bInterplote);
	else if (pThis->m_nSlabMode == "MinIP")
		pThis->MinIPLinearInterpolation(pData->i,pThis->m_bInterplote);
	else if (pThis->m_nSlabMode == "AVERAGE")
		pThis->AverageLinearInterpolation(pData->i,pThis->m_bInterplote);
	
	pThis->m_bThreadEnd[pData->i] = true;

	bool bTrue = true;
	for (int j = 0; j < 20; j++)
	{
		if (!pThis->m_bThreadEnd[j])
		{
			bTrue = false;
			break;
		}
	}
	if (bTrue)
	{
		g_eventDrawAPR.SetEvent();
	}
	delete lpVoid;
	lpVoid = NULL;
	return 0;
}

void SeVisualAPR::SetSlabMode(CString nSlabMode)
{
	m_nSlabMode = nSlabMode;
}

double* SeVisualAPR::GetAPRAngle(int nPlaneNum)
{
	switch(nPlaneNum)
	{
	case 1:
		return &m_dAngleXOY;
	case 2:
		return &m_dAngleYOZ;
	case 3:
		return &m_dAngleXOZ;
	default:
		return NULL;
	}
}

bool SeVisualAPR::GetParallel(int nPlaneNum)
{
	switch(nPlaneNum)
	{
	case 1:
		return m_bXOYParallel;
	case 2:
		return m_bYOZParallel;
	case 3:
		return m_bXOZParallel;
	default:
		return false;
	}
}

void SeVisualAPR::GetSeriesData( double SliceSpace, CDcmPicArray* pDcmArray)
{

	vector<short*>	vBoneData;
	bool bFinish = false;
	int	nCount = 0;

	int nOriNum = -m_nCuttingPlaneWidth/2;
	int nDstNum = m_nCuttingPlaneWidth/2;
	int nOffset = m_nCuttingPlaneWidth/2;

	while(!bFinish)
	{
		bFinish = true;
		short* pCuttingPlane = new short[m_nCuttingPlaneWidth * m_nCuttingPlaneHeight];
		memset(pCuttingPlane , 0 ,sizeof(short) * m_nCuttingPlaneWidth * m_nCuttingPlaneHeight);

		for (int i = 0; i < m_nCuttingPlaneHeight; i++)
		{
			int nTempI = i*m_nCuttingPlaneWidth;
			for (int j = 0; j < m_nCuttingPlaneWidth; j++)
			{

				int nNum = nTempI + j;

				double dX = m_pCuttingPlanePoint[nNum].x +  SliceSpace * nCount* m_fAPRNormal_X;
				double dY = m_pCuttingPlanePoint[nNum].y + SliceSpace * nCount* m_fAPRNormal_Y  ;
				double dZ = (m_pCuttingPlanePoint[nNum].z + SliceSpace * nCount * m_fAPRNormal_Z )/m_dPixelperPiece;

				if (dX > -m_nOriImageWidth/2 && dX < m_nOriImageWidth/2-1 && dY > -m_nOriImageHeight/2 && dY < m_nOriImageHeight/2-1 && dZ > -m_nOriImagePiece/2 && dZ < m_nOriImagePiece/2-1) 
				{
					bFinish =false;

					int nX1 = floor(dX);
					int nX2 = ceil(dX);
					int nY1 = floor(dY);
					int nY2 = ceil(dY); 
					int nZ1 = floor(dZ);
					int nZ2 = ceil(dZ);

					int nOffsetX1 = nX1+m_nOriImageWidth/2;
					int nOffsetX2 = nX2+m_nOriImageWidth/2;
					int nOffsetY1 = nY1+m_nOriImageHeight/2;
					int nOffsetY2 = nY2+m_nOriImageHeight/2;
					int nOffsetZ1 = nZ1+m_nOriImagePiece/2;
					int nOffsetZ2 = nZ2+m_nOriImagePiece/2;

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


					pCuttingPlane[i*m_nCuttingPlaneWidth +j] = (short)dColor;
				}

				else
					pCuttingPlane[i*m_nCuttingPlaneWidth + j] = m_sInitValue;

			}
		}


		if (bFinish)
		{
			Safe_DeleteVec(pCuttingPlane);
		} 
		else
		{
			nCount++ ;
			vBoneData.push_back(pCuttingPlane);
		}
	}


	reverse(vBoneData.begin(),vBoneData.end());
	nCount = -1;
	bFinish = false;

	while(!bFinish)
	{
		bFinish = true;
		short* pCuttingPlane = new short[m_nCuttingPlaneWidth * m_nCuttingPlaneHeight];
		memset(pCuttingPlane , 0 ,sizeof(short) * m_nCuttingPlaneWidth * m_nCuttingPlaneHeight);

		for (int i = 0; i < m_nCuttingPlaneHeight; i++)
		{
			int nTempI = i*m_nCuttingPlaneWidth;
			for (int j = 0; j < m_nCuttingPlaneWidth; j++)
			{
				int nNum = nTempI + j ;

				double dX = m_pCuttingPlanePoint[nNum].x +  SliceSpace *nCount* m_fAPRNormal_X;
				double dY = m_pCuttingPlanePoint[nNum].y + SliceSpace *nCount* m_fAPRNormal_Y;
				double dZ = (m_pCuttingPlanePoint[nNum].z + SliceSpace *nCount * m_fAPRNormal_Z)/m_dPixelperPiece;

				if(dX < -m_nOriImageWidth/2 || dX >= m_nOriImageWidth/2-1 || dY < -m_nOriImageHeight/2 || dY >= m_nOriImageHeight/2-1 || dZ < -m_nOriImagePiece/2 || dZ >= m_nOriImagePiece/2-1)
					pCuttingPlane[i*m_nCuttingPlaneWidth + j] = m_sInitValue;

				else
				{
					bFinish =false;

					int nX1 = floor(dX);
					int nX2 = ceil(dX);
					int nY1 = floor(dY);
					int nY2 = ceil(dY); 
					int nZ1 = floor(dZ);
					int nZ2 = ceil(dZ);

					int nOffsetX1 = nX1+m_nOriImageWidth/2;
					int nOffsetX2 = nX2+m_nOriImageWidth/2;
					int nOffsetY1 = nY1+m_nOriImageHeight/2;
					int nOffsetY2 = nY2+m_nOriImageHeight/2;
					int nOffsetZ1 = nZ1+m_nOriImagePiece/2;
					int nOffsetZ2 = nZ2+m_nOriImagePiece/2;

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

					pCuttingPlane[i*m_nCuttingPlaneWidth +j] = (short)dColor;
				}



			}
		}

		if(bFinish)
		{
			Safe_DeleteVec(pCuttingPlane);
		}
		else
		{
			nCount-- ;
			vBoneData.push_back(pCuttingPlane);
		}

	}

	pDcmArray->ReleaseArray();

	for(int i = 0; i < vBoneData.size(); i++)
	{
		CDcmPic* pDcm = m_pDcmArray->GetDcmArray()[0]->CloneDcmPic();
		pDcm->SetPixelData((BYTE*)vBoneData[i], m_nCuttingPlaneWidth, m_nCuttingPlaneHeight);
		CDcmElement* pEle = pDcm->GetDcmElemList().FindElem(0x0020, 0x0013);
		pEle->RemoveAllValue();
		CString csIndex;
		csIndex.Format("%d", i);
		pEle->ValueAdd(csIndex);
		pDcmArray->AddDcmImage(pDcm);
	}

}

void SeVisualAPR::Reset()
{
	m_pDcmArray = NULL;
	SafeDeleteVec(m_sChipArray);
}

void SeVisualAPR::RotateorTrans( bool bRotateorTrans )
{
	m_bRotateorTrans = bRotateorTrans;
}

void SeVisualAPR::SetPlaneNum(int nPlaneNum)
{
	m_nPlaneNum = nPlaneNum;
}

int SeVisualAPR::GetPlaneNum()
{
	return m_nPlaneNum;
}


void SeVisualAPR::SetWinLevel(int nWinCenter, int nWinWidth)
{
	m_nWinCenter = nWinCenter;
	m_nWinWidth = nWinWidth;
}