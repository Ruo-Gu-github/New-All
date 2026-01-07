// SeBoneDensity.cpp : 定义 DLL 的初始化例程。
//

#include "stdafx.h"
#include "SeBoneDensity.h"
#include "SeBoneDensityDlg.h"
#include "SeBoneDensityCtrlDlg.h"
#include "BoneDensitySwapData.h"
#include "SeAPRView.h"
#include <queue>
using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 添加MessageBoxTimeout支持
extern "C"
{
	int WINAPI MessageBoxTimeoutA(IN HWND hWnd, IN LPCSTR lpText, IN LPCSTR lpCaption, IN UINT uType, IN WORD wLanguageId, IN DWORD dwMilliseconds);
	int WINAPI MessageBoxTimeoutW(IN HWND hWnd, IN LPCWSTR lpText, IN LPCWSTR lpCaption, IN UINT uType, IN WORD wLanguageId, IN DWORD dwMilliseconds);
};
#ifdef UNICODE
#define MessageBoxTimeout MessageBoxTimeoutW
#else
#define MessageBoxTimeout MessageBoxTimeoutA
#endif

CCriticalSection g_criSection;


SeBoneDensity*	g_pBoneDensityModule;

SeBoneDensity::SeBoneDensity(void)
{
	g_pBoneDensityModule = this;
	m_pBoneDensityDlg = NULL;
	m_pBoneDensityCtrlDlg = NULL;
}


SeBoneDensity::~SeBoneDensity(void)
{
	for (int i=0; i<m_OriDcmArray.GetZDcmPicCount(); i++)
		Safe_Delete(m_OriDcmArray.GetDcmArray()[i]);
	m_OriDcmArray.ReleaseArray();
	for (int i=0; i<m_SliceArray.GetZDcmPicCount(); i++)
		Safe_Delete(m_SliceArray.GetDcmArray()[i]);
	m_SliceArray.ReleaseArray();	
	for (int i=0; i<m_MaskArray.GetZDcmPicCount(); i++)
		Safe_Delete(m_MaskArray.GetDcmArray()[i]);
	m_MaskArray.ReleaseArray();
	for (int i=0; i<m_MaskOutArray.GetZDcmPicCount(); i++)
		Safe_Delete(m_MaskOutArray.GetDcmArray()[i]);
	m_MaskOutArray.ReleaseArray();
	for (int i=0; i<m_ROIArray.GetZDcmPicCount(); i++)
		Safe_Delete(m_ROIArray.GetDcmArray()[i]);
	m_ROIArray.ReleaseArray();
	for (int i=0; i<m_BinaryArray.GetZDcmPicCount(); i++)
		Safe_Delete(m_BinaryArray.GetDcmArray()[i]);
	m_BinaryArray.ReleaseArray();
}

BOOL SeBoneDensity::Initialize()
{
	return TRUE;
}

void SeBoneDensity::ExitInstance()
{
	Safe_Delete(m_pBoneDensityDlg);
	Safe_Delete(m_pBoneDensityCtrlDlg);
}

void SeBoneDensity::Release()
{
	delete this;
}

void SeBoneDensity::Reset()
{
	m_pBoneDensityCtrlDlg->Reset();
	m_pBoneDensityDlg->Reset();
	for (int i=0; i<m_OriDcmArray.GetZDcmPicCount(); i++)
		Safe_Delete(m_OriDcmArray.GetDcmArray()[i]);
	m_OriDcmArray.ReleaseArray();
	for (int i=0; i<m_SliceArray.GetZDcmPicCount(); i++)
		Safe_Delete(m_SliceArray.GetDcmArray()[i]);
	m_SliceArray.ReleaseArray();
	for (int i=0; i<m_ROIArray.GetZDcmPicCount(); i++)
		Safe_Delete(m_ROIArray.GetDcmArray()[i]);
	m_ROIArray.ReleaseArray();
	for (int i=0; i< m_BinaryArray.GetZDcmPicCount(); i++)
		Safe_Delete(m_BinaryArray.GetDcmArray()[i]);
	m_BinaryArray.ReleaseArray();

}

CString SeBoneDensity::GetCaption()
{
	return _T("骨密度");
}

CWnd* SeBoneDensity::CreateUI( CWnd* pParent )
{
	if (m_pBoneDensityDlg == NULL)
	{
		m_pBoneDensityDlg = new SeBoneDensityDlg;
		m_pBoneDensityDlg->Create(SeBoneDensityDlg::IDD, pParent);
		m_pBoneDensityDlg->SetParent(pParent);
	}
	return m_pBoneDensityDlg;
}

CWnd* SeBoneDensity::GetUI()
{
	return m_pBoneDensityDlg;
}

HICON SeBoneDensity::GetIcon()
{
	return AfxGetApp()->LoadIcon(IDI_ICON_BONE);
}

BOOL SeBoneDensity::CanProcess( ProcessModule emModule )
{
	return TRUE;
}

BOOL SeBoneDensity::InitProcess( ProcessModule emModule )
{
	BOOL bCanProcess = CanProcess(emModule);
	if (!bCanProcess)
	{
		return FALSE;
	}
	return TRUE;
}

void SeBoneDensity::AssignProcessData( const vector<CDcmPic*>& vecDcmArr )
{

}

void SeBoneDensity::AssignProcessData( CStringArray& csaDcmFiles )
{
	if (csaDcmFiles.GetSize() == 0)
	{
		AfxMessageBox("未选择图像");
		return;
	}

	// csaDcmFiles.RemoveAt(0, 1);
	for (int i=0; i<65536; i++)
	{
		theBoneDensitySwapData.m_Histogram[i] = 0;
	}
	theBoneDensitySwapData.m_nMaxValue = 0;
	theBoneDensitySwapData.m_nMinValue = 0;
	theBoneDensitySwapData.m_lMaxNumber = 0;


	theAppIVConfig.m_pILog->ProgressInit(csaDcmFiles.GetSize());

	// 多线程加快读取速度。
	vector<CDcmPic*> vecImageList;
	queue <ImageLoadInfo> queueInfoList;
	static int nAliveThread = THREAD_NUMBER;
	nAliveThread = THREAD_NUMBER;
	for (int i=0; i<csaDcmFiles.GetSize(); i++)
	{
		ImageLoadInfo Imageinfo(csaDcmFiles[i], i);
		vecImageList.push_back(NULL);
		queueInfoList.push(Imageinfo);
	}

	m_strInfoFile = csaDcmFiles[0];

	ThreadInfo* pInfo = new ThreadInfo(queueInfoList, vecImageList, nAliveThread);

	for (int i=0; i<THREAD_NUMBER; i++)
	{
		AfxBeginThread(__LoadImage, pInfo);
	}

	while(pInfo->nAliveThread > 0)
	{
		int x = 0;
		Sleep(1000);
	}

	// the instanceNumber of dicom image is not always match from the file name, 
	// so arrangement images from the instanceNumber instead of filename. 
	// different dicom image use different start instanceNumber, can not just believe start zero and did not miss mid layer or repeat.
	// so rearrangement it yourself.
	// the old SeProcessPro create dicom image with same instanceNumber, because of that, if the instanceNumber is all the same,
	// did not rearrangement it.
	vector <pair <int, CDcmPic*>> vecNewSet;
	for (int i=0; i<csaDcmFiles.GetSize(); i++)
	{
		int nNum = pInfo->imgList[i]->GetImageNumber();
		vecNewSet.push_back(make_pair(nNum, pInfo->imgList[i]));
	}
	BOOL bAlltheSame = TRUE;
// 	for (auto begin = vecNewSet.begin(); begin != vecNewSet.end(); ++begin)
// 	{
// 		if((*begin).first != (*vecNewSet.begin()).first)
// 		{
// 			bAlltheSame = FALSE;
// 			break;
// 		}
// 	}
	if (!bAlltheSame)
		sort(vecNewSet.begin(), vecNewSet.end());
	for (int i=0; i<csaDcmFiles.GetSize(); i++)
	{
		m_OriDcmArray.AddDcmImage(vecNewSet[i].second);

	}
// 	for (int i=0; i<csaDcmFiles.GetSize(); i++)
// 	{
// 		m_OriDcmArray.AddDcmImage(pInfo->imgList[i]);
// 	}
	Safe_Delete(pInfo);

	for (int i=0; i<65536; i++)
	{
		theBoneDensitySwapData.m_lMaxNumber = theBoneDensitySwapData.m_lMaxNumber > theBoneDensitySwapData.m_Histogram[i] ? theBoneDensitySwapData.m_lMaxNumber : theBoneDensitySwapData.m_Histogram[i];
	}


	// 获得 图像路径
	CString csFolderPath = m_OriDcmArray.GetDcmArray()[0]->GetFilePathName();
	for(int i = csFolderPath.GetLength() - 1; i >= 0; i--)
	{
		if (csFolderPath[i] == '\\')
		{
			csFolderPath = csFolderPath.Left(i);
			break;
		}
	}
	theBoneDensitySwapData.m_csFolderPath = csFolderPath;

	// 获得 图像基本参数
	if (theBoneDensitySwapData.m_nMinValue < -1000) {
		theBoneDensitySwapData.m_nMinValue = 0;
	}
	CDcmPic* pDcmTmp = m_OriDcmArray.GetDcmArray()[0]->CloneDcmPic();
 	theBoneDensitySwapData.m_dbZSliceSpace = pDcmTmp->GetSliceSpace();
 	theBoneDensitySwapData.m_dbXYSliceSpace = pDcmTmp->GetMMPerpixel();
	theBoneDensitySwapData.m_csSeriesName = pDcmTmp->GetSeriesInstanceID();
	theBoneDensitySwapData.m_nMinValuePos = theBoneDensitySwapData.m_nMinValue;
	theBoneDensitySwapData.m_nMaxValuePos = theBoneDensitySwapData.m_nMaxValue;
	Safe_Delete(pDcmTmp);
	theAppIVConfig.m_pILog->ProgressClose();

	m_pBoneDensityDlg->m_pOriPicArray = &m_OriDcmArray;
	theBoneDensitySwapData.m_nStep = SP_SELECTZ;
	m_pBoneDensityCtrlDlg->SendMessage(WM_SIZE);
	m_pBoneDensityDlg->SendMessage(WM_SIZE);
	m_pBoneDensityDlg->ShowFirstView();
}

void SeBoneDensity::AssignProcessData( CDcmPicArray* pDcmPicArray )
{

}

void SeBoneDensity::AssignInterface( ISeProcessMainFrame* pMainFrame )
{
	m_pMainFrame = pMainFrame;
}

CWnd* SeBoneDensity::CreateCtrlUI( CWnd* pParent )
{
	if (m_pBoneDensityCtrlDlg == NULL)
	{
		m_pBoneDensityCtrlDlg = new SeBoneDensityCtrlDlg;
		m_pBoneDensityCtrlDlg->Create(SeBoneDensityCtrlDlg::IDD, pParent);
		m_pBoneDensityCtrlDlg->SetParent(pParent);
	}
	return m_pBoneDensityCtrlDlg;
}

CWnd* SeBoneDensity::GetCtrlUI( CWnd* pParent )
{
	return m_pBoneDensityCtrlDlg;
}

UINT SeBoneDensity::__LoadImage(LPVOID pParam)
{
	ThreadInfo* pInfo = (ThreadInfo*) pParam;
	bool bKillThread = false;
	while(TRUE)
	{
		g_criSection.Lock();
		if (pInfo->info.empty())
		{
			// 等待其他线程读取完成
			g_criSection.Unlock();
			if (bKillThread == false)
			{
				pInfo->nAliveThread -= 1;
				bKillThread = true;	
				continue;
			}	
			if (pInfo->nAliveThread == 0)
			{
				return 0;
			}
		}
		else
		{
			ImageLoadInfo info = pInfo->info.front();
			pInfo->info.pop();
			g_criSection.Unlock();

			CDcmPic* pDcm = new CDcmPic;
			pDcm->LoadFromDcmFile(info.csImageName);
			pDcm->ReloadBuffer();
			int nSize = pDcm->GetWidth() * pDcm->GetHeight();
			short* pData = (short*)pDcm->GetData();

			short* pHeadPoint = pData;
			for (int i=0; i<nSize; i++)
			{
				// dcm 值的范围 -32768 - 32767
				short temp = *pHeadPoint;
				if (temp < 0 || temp > 16382) {
					temp = 16382;
				}
				theBoneDensitySwapData.m_nMinValue = theBoneDensitySwapData.m_nMinValue < temp ? theBoneDensitySwapData.m_nMinValue : temp;
				theBoneDensitySwapData.m_nMaxValue = theBoneDensitySwapData.m_nMaxValue > temp ? theBoneDensitySwapData.m_nMaxValue : temp;
				theBoneDensitySwapData.m_Histogram[temp + 32768] += 1;
				pHeadPoint++;
			}

			pInfo->imgList[info.Index] = pDcm;
			theAppIVConfig.m_pILog->ProgressStepIt();
		}
	}
	return 0;
}

const void SeBoneDensity::CutImage(int nXStart, int nXEnd, int nYStart, int nYEnd, int nZStart, int nZEnd, SHAPE_TYPE shape)
{
	theAppIVConfig.m_pILog->ProgressInit(nZEnd - nZStart);
	queue <ImageCropInfo> queueInfoList;
	vector <CDcmPic*> vecImgs;
#ifdef CUDA_AVAILABLE
	SeVisualAPR_with_CUDA* pApr = theBoneDensitySwapData.m_pXOYView->GetAPR();
#else
	SeVisualAPR* pApr = theBoneDensitySwapData.m_pXOYView->GetAPR();
#endif // CUDA_AVAILABLE
	MprRotateInfo rotateInfo = theBoneDensitySwapData.m_pXOYView->GetRotateInfor();
	int nMinValue = theBoneDensitySwapData.m_nMinValue;
	static int nAliveThread = THREAD_NUMBER;
	nAliveThread = THREAD_NUMBER;
	for (int i=nZStart; i<nZEnd; i++)
	{
		// APR 层数从 -图像宽度/2 - +图像宽度/2；
		queueInfoList.push(ImageCropInfo(i - nZStart, i - theBoneDensitySwapData.m_nRotateDcmSideLength/2));
		//queueInfoList.push(ImageCropInfo(i - nZStart, i));
		vecImgs.push_back(NULL);
	}

	ThreadInfoForCrop* pInfo = new ThreadInfoForCrop(queueInfoList, vecImgs, pApr, rotateInfo, nAliveThread, nXStart, nXEnd, nYStart, nYEnd, nMinValue, shape);

	for (int i=0; i<THREAD_NUMBER; i++)
	{
		AfxBeginThread(__CropImage, pInfo);
	}

	while(pInfo->nAliveThread > 0)
	{
		int x = 0;
		Sleep(1000);
	}

	if (theBoneDensitySwapData.m_pROIDoc != NULL)
		theBoneDensitySwapData.m_pROIDoc->ReleaseSeries();
	for (int i=0; i < m_SliceArray.GetZDcmPicCount(); i++)
		Safe_Delete(m_SliceArray.GetDcmArray()[i]);
	m_SliceArray.ReleaseArray();

	for (int i=0; i<pInfo->imgList.size(); i++)
	{
		pInfo->imgList[i]->SetImageNumber(i);
		m_SliceArray.AddDcmImage(pInfo->imgList[i]);
	}

	for (int i=0; i < m_MaskArray.GetZDcmPicCount(); i++)
		Safe_Delete(m_MaskArray.GetDcmArray()[i]);
	m_MaskArray.ReleaseArray();

	for (int i=0; i<pInfo->imgList.size(); i++)
	{
		m_MaskArray.AddDcmImage(m_SliceArray.GetDcmArray()[0]->CloneDcmPic());
	}

	for (int i=0; i < m_MaskOutArray.GetZDcmPicCount(); i++)
		Safe_Delete(m_MaskOutArray.GetDcmArray()[i]);
	m_MaskOutArray.ReleaseArray();

	for (int i=0; i<pInfo->imgList.size(); i++)
	{
		m_MaskOutArray.AddDcmImage(m_SliceArray.GetDcmArray()[0]->CloneDcmPic());
	}

// 	for (int i=0; i < m_ROIArray.GetZDcmPicCount(); i++)
// 		Safe_Delete(m_ROIArray.GetDcmArray()[i]);
// 	m_ROIArray.ReleaseArray();
// 
// 	for (int i=0; i<pInfo->imgList.size(); i++)
// 	{
// 		m_ROIArray.AddDcmImage(m_SliceArray.GetDcmArray()[0]->CloneDcmPic());
// 	}
	Safe_Delete(pInfo);
	theAppIVConfig.m_pILog->ProgressClose();

}

const void SeBoneDensity::CutImage(vector<CPoint>* pVecEdge, vector<CPoint>* pVecEdgeInside)
{
	if (pVecEdge == NULL && pVecEdgeInside == NULL)
		return;

	queue <ImageROIInfo> queueInfoList;
	vector <CDcmPic*> vecImgs;
	int nMinValue = theBoneDensitySwapData.m_nMinValue;
	static int nAliveThread = THREAD_NUMBER;
	nAliveThread = THREAD_NUMBER;
	assert(m_SliceArray.GetDcmArray().size() > 0);
	CDcmPic* pDcm = m_SliceArray.GetDcmArray()[0];
	int nWidth = pDcm->GetWidth();
	int nHeight = pDcm->GetHeight();
	int nPos = 0;
	for (int i=0; i<m_SliceArray.GetDcmArray().size(); i++)
	{
		if (pVecEdge[i].empty())
			continue;
		queueInfoList.push(ImageROIInfo(pVecEdge[i], pVecEdgeInside[i], nPos++)); 
		CDcmPic *pDcm = m_SliceArray.GetDcmArray()[i]->CloneDcmPic();
		vecImgs.push_back(pDcm);
	}

	theAppIVConfig.m_pILog->ProgressInit(INT(m_SliceArray.GetDcmArray().size() * 1.5));

	ThreadInfoForROI* pInfo = new ThreadInfoForROI(queueInfoList, vecImgs, nAliveThread, nWidth, nHeight, nMinValue);


	for (int i=0; i<THREAD_NUMBER; i++)
	{
		AfxBeginThread(__ROIImage, pInfo);
	}

	while(pInfo->nAliveThread > 0)
	{
		int x = 0;
		Sleep(1000);
	}

	for (int i=0; i<65536; i++)
	{
		theBoneDensitySwapData.m_lMaxNumber = theBoneDensitySwapData.m_lMaxNumber > theBoneDensitySwapData.m_Histogram[i] ? theBoneDensitySwapData.m_lMaxNumber : theBoneDensitySwapData.m_Histogram[i];
	}

	if (theBoneDensitySwapData.m_pBinaryDoc != NULL)
		theBoneDensitySwapData.m_pBinaryDoc->ReleaseSeries();
	for (int i=0; i < m_ROIArray.GetZDcmPicCount(); i++)
		Safe_Delete(m_ROIArray.GetDcmArray()[i]);
	m_ROIArray.ReleaseArray();

	for (int i=0; i<pInfo->imgList.size(); i++)
	{
		pInfo->imgList[i]->SetImageNumber(i);
		m_ROIArray.AddDcmImage(pInfo->imgList[i]);
	}
	Safe_Delete(pInfo);
	theAppIVConfig.m_pILog->ProgressClose();
}



const void SeBoneDensity::GetBoneSegOuter()
{
	vector <CDcmPic*> vecImgs;
	for (int i=0; i<m_SliceArray.GetDcmArray().size(); i++)
	{
		CDcmPic *pDcm = m_SliceArray.GetDcmArray()[i]->CloneDcmPic();
		vecImgs.push_back(pDcm);
	}

	if (theBoneDensitySwapData.m_pBinaryDoc != NULL)
		theBoneDensitySwapData.m_pBinaryDoc->ReleaseSeries();

	for (int i=0; i < m_ROIArray.GetZDcmPicCount(); i++)
		Safe_Delete(m_ROIArray.GetDcmArray()[i]);
	m_ROIArray.ReleaseArray();

	for (int i=0; i<vecImgs.size(); i++)
	{
		m_ROIArray.AddDcmImage(vecImgs[i]);
	}

	int nMinValue = theBoneDensitySwapData.m_nMinValue;

	for (int k = 0; k < m_MaskOutArray.GetZDcmPicCount(); k++)
	{
		CImageBase* pDcmMaskOut = m_MaskOutArray.GetDcmArray()[k];
		if(pDcmMaskOut == NULL)
			return;
		int nWidth = pDcmMaskOut->GetWidth();
		int nHeight = pDcmMaskOut->GetHeight();
		short* pDataMaskOut = (short*)pDcmMaskOut->GetData();

		CImageBase* pDcmOri = m_SliceArray.GetDcmArray()[k];
		short* pDataOri = (short*)pDcmOri->GetData();

		CImageBase* pDcmOut = m_ROIArray.GetDcmArray()[k];
		short* pDataOut = (short*)pDcmOut->GetData();

		CImageBase* pDcmMaskIn = m_MaskArray.GetDcmArray()[k];
		short* pDataMaskIn = (short*)pDcmMaskIn->GetData();

		for (int i=0;i<(nWidth*nHeight);i++)
		{
			if (pDataMaskOut[i]==0 || pDataMaskIn[i] == 4096)
			{
				pDataOut[i] = nMinValue;
			}
			else{
				if (pDataOri[i] == nMinValue)
				{
					pDataOut[i] = nMinValue+1;
				}
				else{
					pDataOut[i] = pDataOri[i];
				}
			}
		}
	}
}

const void SeBoneDensity::GetBoneSegInner()
{
	vector <CDcmPic*> vecImgs;
	for (int i=0; i<m_SliceArray.GetDcmArray().size(); i++)
	{
		CDcmPic *pDcm = m_SliceArray.GetDcmArray()[i]->CloneDcmPic();
		vecImgs.push_back(pDcm);
	}

	if (theBoneDensitySwapData.m_pBinaryDoc != NULL)
		theBoneDensitySwapData.m_pBinaryDoc->ReleaseSeries();

	for (int i=0; i < m_ROIArray.GetZDcmPicCount(); i++)
		Safe_Delete(m_ROIArray.GetDcmArray()[i]);
	m_ROIArray.ReleaseArray();

	for (int i=0; i<vecImgs.size(); i++)
	{
		m_ROIArray.AddDcmImage(vecImgs[i]);
	}

	int nMinValue = theBoneDensitySwapData.m_nMinValue;

	for (int k = 0; k < m_MaskArray.GetZDcmPicCount(); k++)
	{
		CImageBase* pDcmMask = m_MaskArray.GetDcmArray()[k];
		if(pDcmMask == NULL)
			return;
		int nWidth = pDcmMask->GetWidth();

		int nHeight = pDcmMask->GetHeight();
		short* pDataMask = (short*)pDcmMask->GetData();

		CImageBase* pDcmOri = m_SliceArray.GetDcmArray()[k];
		short* pDataOri = (short*)pDcmOri->GetData();

		CImageBase* pDcmOut = m_ROIArray.GetDcmArray()[k];
		short* pDataOut = (short*)pDcmOut->GetData();

		for (int i=0;i<(nWidth*nHeight);i++)
		{
			if (pDataMask[i]==0)
			{
				pDataOut[i] = nMinValue;
			}
			else{
				if (pDataOri[i] == nMinValue)
				{
					pDataOut[i] = nMinValue+1;
				}
				else{
					pDataOut[i] = pDataOri[i];
				}
			}
		}
	}
}

const BOOL SeBoneDensity::PtInEclipse(int nPosX, int nPosY, int nXS, int nXE, int nYS, int nYE)
{
	float fA = (float)abs((nXE - nXS))/2.0;
	float fB = (float)abs((nYE - nYS))/2.0;
	float fC = sqrt((float)abs(pow(fA, 2) - pow(fB, 2)));
	float fPtCx = (float)abs((nXE + nXS))/2.0;
	float fPtCy = (float)abs((nYE + nYS))/2.0;

	if (fA >= fB)
	{
		float fRst = sqrt(pow((float)nPosX - fPtCx - fC, 2) + pow(nPosY - fPtCy, 2)) + sqrt(pow((float)nPosX - fPtCx + fC, 2) + pow(nPosY - fPtCy, 2));
		if (fRst <= fA * 2 )
			return TRUE;
		return FALSE;
	}
	else
	{
		float fRst = sqrt(pow((float)nPosX - fPtCx, 2) + pow(nPosY - fPtCy - fC, 2)) + sqrt(pow((float)nPosX - fPtCx, 2) + pow(nPosY - fPtCy + fC, 2));
		if (fRst <= fB * 2 )
			return TRUE;
		return FALSE;
	}
}

UINT SeBoneDensity::__CropImage(LPVOID pParam)
{
	ThreadInfoForCrop* pInfo = (ThreadInfoForCrop*)pParam;
	MprRotateInfo rotateInfo = pInfo->rotateInfo;
#ifdef CUDA_AVAILABLE
	SeVisualAPR_with_CUDA* pAPR        = pInfo->pAPR;
#else
	SeVisualAPR*		pAPR        = pInfo->pAPR;
#endif // CUDA_AVAILABLE
	int nXStart = pInfo->nXStart;
	int nXEnd   = pInfo->nXEnd;
	int nYStart = pInfo->nYStart;
	int nYEnd   = pInfo->nYEnd;
	int nMinValue = pInfo->nMinValue;
	SHAPE_TYPE shape = pInfo->shape;


	bool bKillThread = false;
	while(TRUE)
	{
		g_criSection.Lock();
		if (pInfo->info.empty())
		{
			// 等待其他线程读取完成
			g_criSection.Unlock();
			if (bKillThread == false)
			{
				pInfo->nAliveThread -= 1;
				bKillThread = true;
				continue;
			}	
			if (pInfo->nAliveThread == 0)
				return 0;
		}
		else
		{
			ImageCropInfo infoCrop = pInfo->info.front();
			pInfo->info.pop();
			CDcmPic* pDcm = NULL;
			int nPos = infoCrop.nPos;
			double dX, dY, dZ;
			dX = nPos*rotateInfo.dWorldX_z;
			dY = nPos*rotateInfo.dWorldY_z;
			dZ = nPos*rotateInfo.dWorldZ_z;
			//pDcm = pAPR->GetAPRImage(rotateInfo.dModelView, dX, dY, dZ, TRUE);
			pDcm = pAPR->GetAPRImage(rotateInfo.dModelView, dX, dY, dZ, TRUE);
			g_criSection.Unlock();
			if (pDcm != NULL)
			{
				int nWidth = pDcm->GetWidth();
				int nHeight = pDcm->GetHeight();
				short* pOriData = new short[nWidth * nHeight];
				memcpy(pOriData, (short*)pDcm->GetData(),nWidth*nHeight*sizeof(short));	
				short* pRstData = new short[(nXEnd - nXStart) * (nYEnd - nYStart)];
				memset(pRstData, 0, sizeof(short) * (nXEnd - nXStart) * (nYEnd - nYStart));

				short* pTmpOri = pOriData;// 使用临时指针，只移动临时指针，避免原指针指向的内存地址发生变化，delete 指针出错
				short* pTmpRst = pRstData;
				pTmpOri += nWidth * (nYStart);
				for(int i = nYStart; i < nYEnd; i++)
				{
					pTmpOri += (nXStart);
					for (int j = nXStart; j < nXEnd; j++)
					{
						if (shape == ROUND)
						{
							if (PtInEclipse(j, i, nXStart, nXEnd, nYStart, nYEnd))
								*pTmpRst++ = *pTmpOri;
							else
								*pTmpRst++ = nMinValue;
							pTmpOri++;
						}
						else if(shape == SQUARE)
						{
							if (j <= nXStart || j >= nXEnd || i <= nYStart || i >= nYEnd)
								*pTmpRst++ = nMinValue;
							else
								*pTmpRst++ = *pTmpOri;
							pTmpOri++;
						}
						else
						{
							pTmpOri++;
						}
					}
					pTmpOri += (nWidth - (nXEnd));		
				}

				// 只能通过新建dicom才能修改pixelSpacing 和 pixelSize
				//CDcmPic* pSliceDcm = new CDcmPic;
				//pSliceDcm->LoadFromDcmFile(g_pBoneDensityModule->m_strInfoFile);
				//pSliceDcm->ReloadBuffer();
				//pSliceDcm->SetMMPerpixel(pDcm->GetMMPerpixel());
				//pSliceDcm->SetPixelSpacing(pDcm->GetMMPerpixel());
				CDcmPic* pSliceDcm = pDcm->CloneDcmPic();
				pSliceDcm->SetPixelData((BYTE*)pRstData, (nXEnd - nXStart), (nYEnd - nYStart));
				//CDcmElement* pEle = pSliceDcm->GetDcmElemList().FindElem(0x0018, 0x0050);
				//pEle->RemoveAllValue();
				//pEle->ValueAddFloat(pDcm->GetMMPerpixel());
				pInfo->imgList[infoCrop.Index] = pSliceDcm;
				delete [] pOriData;
			}
			Safe_Delete(pDcm);
			theAppIVConfig.m_pILog->ProgressStepIt();
		}
	}
	return 0;


}

UINT SeBoneDensity::__ROIImage(LPVOID pParam)
{
	ThreadInfoForROI* pInfo = (ThreadInfoForROI*)pParam;
	int nWidth = pInfo->nImageWidth;
	int nHeight = pInfo->nImageHeight;
	int nMinValue = pInfo->nMinValue;

	bool bKillThread = false;
	while(TRUE)
	{
		g_criSection.Lock();
		if (pInfo->info.empty())
		{
			// 等待其他线程读取完成
			g_criSection.Unlock();
			if (bKillThread == false)
			{
				pInfo->nAliveThread -= 1;
				bKillThread = true;
				continue;
			}	
			if (pInfo->nAliveThread == 0)
				return 0;
		}
		else
		{
			ImageROIInfo info = pInfo->info.front();
			pInfo->info.pop();
			int nSize = (int)info.vecPts.size();
			int nSizeInside = (int)info.vecPtsInside.size();
			g_criSection.Unlock();

			CRgn rgnROI;
			CRgn rgnROIInside;
			CPoint* ptEdge = new CPoint[nSize];	
			CPoint* ptEdgeInside = new CPoint[nSizeInside];
			for (int j=0; j<info.vecPts.size(); j++)
			{
				ptEdge[j] = info.vecPts[j];
			}
			for(int k=0; k<info.vecPtsInside.size(); k++)
			{
				ptEdgeInside[k] = info.vecPtsInside[k];
			}
			rgnROI.CreatePolygonRgn(ptEdge, nSize, ALTERNATE);
			rgnROIInside.CreatePolygonRgn(ptEdgeInside, nSizeInside, ALTERNATE);
			CDcmPic* pDcm = pInfo->imgList[info.nPos];

			CRgn rgnResult;
			//虚拟一个任意区域接受结果；
			rgnResult.CreateRectRgn(0,0,10,10);
			
			// temporary
			CRgn rgnTemp;
			rgnTemp.CreateRectRgn(0,0,10,10);
			LONGLONG tmpSize = 0;


			if(nSizeInside == 0)
			{
				rgnResult.CopyRgn(&rgnROI);
			}
			else
			{
				rgnResult.CombineRgn(&rgnROI, &rgnROIInside, RGN_DIFF);
				rgnTemp.CopyRgn(&rgnROIInside);
			}
			short* pData = (short*)pDcm->GetData();
			short* pDataTmp = pData;
			for (int m = 0; m < nHeight; m++)
			{
				for (int n = 0; n < nWidth; n++)
				{
					CPoint pt(n,m);
					if(rgnTemp.PtInRegion(pt)) tmpSize++;
					if (!rgnResult.PtInRegion(pt)/* == !theBoneDensitySwapData.m_bClipOutside*/)
					{
						*pDataTmp++ = nMinValue;
					}
					else
					{
						if (*pDataTmp == nMinValue)
							*pDataTmp+=2;
						pDataTmp++;
					}

				}
			}

			Safe_Delete(ptEdge);
			pDcm->Refresh();

			// 计算直方图
			int nSizeTotle = pDcm->GetWidth() * pDcm->GetHeight();
			short* pDataRst = (short*)pDcm->GetData();

			short* pHeadPoint = pDataRst;
			for (int i=0; i<nSizeTotle; i++)
			{
				// dcm 值的范围 -32768 - 32767
				theBoneDensitySwapData.m_nMinValue = theBoneDensitySwapData.m_nMinValue < *pHeadPoint ? theBoneDensitySwapData.m_nMinValue : *pHeadPoint;
				theBoneDensitySwapData.m_nMaxValue = theBoneDensitySwapData.m_nMaxValue > *pHeadPoint ? theBoneDensitySwapData.m_nMaxValue : *pHeadPoint;
				theBoneDensitySwapData.m_Histogram[*pHeadPoint++ + 32768] += 1;
			}
			theBoneDensitySwapData.m_tmpSize += tmpSize;
		}
		theAppIVConfig.m_pILog->ProgressStepIt();
	}
}

const void SeBoneDensity::ExportDcmArray(CString strFolderName, CDcmPicArray* pDcmArray)
{
	CString csFullPath;
	csFullPath.Format("%s\\%s\\", theBoneDensitySwapData.m_csFolderPath, strFolderName);
	CreateFolder(csFullPath);

// 	// 单线程
// 	for (int nIndex=0; nIndex<pDcmArray->GetZDcmPicCount(); nIndex++)
// 	{
// 		CString csName;
// 		CString csIndex;
// 		csIndex.Format("%d", nIndex);
// 		if (nIndex<10)
// 		{
// 			csName = "Hiscan_0000" + csIndex;
// 		}
// 		else if (nIndex>=10 && nIndex<100)
// 		{
// 			csName = "Hiscan_000" + csIndex;
// 		}
// 		else if (nIndex>=100)
// 		{
// 			csName = "Hiscan_00" + csIndex;
// 		}
// 		else
// 			csName = "Hiscan_0" + csIndex;
// 		CDcmPic* pDcm = pDcmArray->GetDcmArray()[nIndex]->CloneDcmPic();
// 		short* pTmp = new short[pDcm->GetWidth() * pDcm->GetHeight()];
// 		memset(pTmp, 0, sizeof(short) * pDcm->GetWidth() * pDcm->GetHeight());
// 		memcpy(pTmp, pDcmArray->GetDcmArray()[nIndex]->GetData(), sizeof(short) * pDcm->GetWidth() * pDcm->GetHeight());
// 		pDcm->SetPixelData((BYTE*)pTmp, 
// 			pDcmArray->GetDcmArray()[nIndex]->GetWidth(), 
// 			pDcmArray->GetDcmArray()[nIndex]->GetHeight());
// /*		pDcm->ReloadBuffer();*/
// 		CDcmElement* pEle = pDcm->GetDcmElemList().FindElem(0x0020, 0x0013);
// 		pEle->ValueAdd(csName);
// 		CString csFilePath = csFullPath + csName + _T(".dcm");
// 		pDcm->ExportDcm(csFilePath);
// 		Safe_Delete(pDcm);
// 	}
// 	MessageBoxTimeout(NULL, "         导出文件完成！       ", "提示", MB_ICONINFORMATION, 0, 300);

	//新开一个线程
	ThreadInfoForSaveDcm* pInfo = new ThreadInfoForSaveDcm(csFullPath, pDcmArray);
	AfxBeginThread(__SaveDcmFile, pInfo);
}

UINT SeBoneDensity::__SaveDcmFile(LPVOID pParam)
{
	ThreadInfoForSaveDcm* pInfo = (ThreadInfoForSaveDcm*) pParam;
	CDcmPicArray* pDcmArray = pInfo->pDcmArray;
	CString csFullPath = pInfo->csFullPath;
	for (int nIndex=0; nIndex<pDcmArray->GetZDcmPicCount(); nIndex++)
	{
		CString csName;
		CString csIndex;
		csIndex.Format("%d", nIndex);
		if (nIndex<10)
		{
			csName = "Hiscan_0000" + csIndex;
		}
		else if (nIndex>=10 && nIndex<100)
		{
			csName = "Hiscan_000" + csIndex;
		}
		else if (nIndex>=100)
		{
			csName = "Hiscan_00" + csIndex;
		}
		else
			csName = "Hiscan_0" + csIndex;
		CDcmPic* pDcm = pDcmArray->GetDcmArray()[nIndex]->CloneDcmPic();
		short* pTmp = new short[pDcm->GetWidth() * pDcm->GetHeight()];
		memset(pTmp, 0, sizeof(short) * pDcm->GetWidth() * pDcm->GetHeight());
		memcpy(pTmp, pDcmArray->GetDcmArray()[nIndex]->GetData(), sizeof(short) * pDcm->GetWidth() * pDcm->GetHeight());
		pDcm->SetPixelData((BYTE*)pTmp, 
			pDcmArray->GetDcmArray()[nIndex]->GetWidth(), 
			pDcmArray->GetDcmArray()[nIndex]->GetHeight());
		CDcmElement* pEle = pDcm->GetDcmElemList().FindElem(0x0020, 0x0013);
		pEle->RemoveAllValue();
		pEle->ValueAdd(csIndex);
		
		CDcmElement* pElePosition = pDcm->GetDcmElemList().FindElem(0x0020, 0x0032);
		CString csPostion; 
		float sliceSize = pDcm->GetSliceSpace();
		pElePosition->ValueGetString(csPostion);
		int lastPositionIndex = csPostion.ReverseFind('\\');
		float startPostion = CSeToolKit::CStr2Float(csPostion.Right(csPostion.GetLength() - lastPositionIndex - 1));
		CString csNewPosition = csPostion.Left(lastPositionIndex + 1) + CSeToolKit::float2CStr(startPostion + sliceSize * static_cast<float>(nIndex));
 		CString csFilePath = csFullPath + csName + _T(".dcm");
		pElePosition->RemoveAllValue();
		pElePosition->ValueAdd(csNewPosition);
		//pEle = pDcm->GetDcmElemList().FindElem(0x0018, 0x0050);
		//pEle->RemoveAllValue();
		//pEle->ValueAddFloat(pDcm->GetMMPerpixel());
// 		pDcm->SetImageNumber(nIndex);
// 		pDcm->ReloadImage();
		pDcm->ExportDcm(csFilePath);
		Safe_Delete(pDcm);
	}
	MessageBoxTimeout(NULL, "         导出文件完成！       ", "提示", MB_ICONINFORMATION, 0, 300);
	return 0;
}

const void SeBoneDensity::CreateFolder(CString csPath)
{
	if (!::CreateDirectoryA(csPath, NULL))
	{
		CFileFind   filefind;
		CString     csfilePath = csPath + "\\*.dcm";
		BOOL	bFind = filefind.FindFile(csfilePath); 
		while(bFind)
		{
			bFind = filefind.FindNextFile();
			if(filefind.IsDots())
				continue;
			else if(filefind.IsDirectory())
				continue;		
			else
			{
				CString   csfilename = filefind.GetFilePath();
				::DeleteFile(csfilename);
			}
		}
	}
}


