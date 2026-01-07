// SeGeneralModule.cpp : 定义 DLL 的初始化例程。
//

#include "stdafx.h"
#include "SeGeneralModule.h"
#include "SeGeneralModuleDlg.h"
#include "SeGeneralModuleCtrlDlg.h"
#include "GeneralSwapData.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

SeGeneralModule* g_pGeneralModule;


CCriticalSection g_criSection;


SeGeneralModule::SeGeneralModule(void)
{
	m_pGeneralModuleDlg = nullptr;
	m_pGeneralModuleCtrlDlg = nullptr;
	m_pMainFrame = nullptr;

	g_pGeneralModule = this;
}


SeGeneralModule::~SeGeneralModule(void)
{
	for (int i = 0; i < m_OriDcmArray.GetZDcmPicCount(); i++)
		Safe_Delete(m_OriDcmArray.GetDcmArray()[i]);
	m_OriDcmArray.ReleaseArray();
}

BOOL SeGeneralModule::Initialize()
{
	return TRUE;
}

void SeGeneralModule::ExitInstance()
{
	Safe_Delete(m_pGeneralModuleDlg);
	Safe_Delete(m_pGeneralModuleCtrlDlg);
}

void SeGeneralModule::Release()
{
	delete this;
}

void SeGeneralModule::Reset()
{
	m_pGeneralModuleDlg->Reset();
	m_pGeneralModuleCtrlDlg->Reset();
	for (int i = 0; i < m_OriDcmArray.GetZDcmPicCount(); i++)
		Safe_Delete(m_OriDcmArray.GetDcmArray()[i]);
	m_OriDcmArray.ReleaseArray();
}

CString SeGeneralModule::GetCaption()
{
	return "  图像浏览         ";
}

CWnd* SeGeneralModule::CreateUI(CWnd* pParent)
{
	if (m_pGeneralModuleDlg == nullptr)
	{
		m_pGeneralModuleDlg = new SeGeneralModuleDlg;
		m_pGeneralModuleDlg->Create(SeGeneralModuleDlg::IDD, pParent);
		m_pGeneralModuleDlg->SetParent(pParent);
	}
	return m_pGeneralModuleDlg;
}

CWnd* SeGeneralModule::GetUI()
{
	return m_pGeneralModuleDlg;
}

CWnd* SeGeneralModule::CreateCtrlUI(CWnd* pParent)
{
	if (m_pGeneralModuleCtrlDlg == nullptr)
	{
		m_pGeneralModuleCtrlDlg = new SeGeneralModuleCtrlDlg;
		m_pGeneralModuleCtrlDlg->Create(SeGeneralModuleCtrlDlg::IDD, pParent);
		m_pGeneralModuleCtrlDlg->SetParent(pParent);
	}
	return m_pGeneralModuleCtrlDlg;
}

CWnd* SeGeneralModule::GetCtrlUI(CWnd* pParent)
{
	return m_pGeneralModuleCtrlDlg;
}

HICON SeGeneralModule::GetIcon()
{
	return AfxGetApp()->LoadIcon(IDI_ICON_GENERAL);
}

BOOL SeGeneralModule::CanProcess(ProcessModule emModule)
{
	return TRUE;
}

BOOL SeGeneralModule::InitProcess(ProcessModule emModule)
{
	BOOL bCanProcess = CanProcess(emModule);
	if (!bCanProcess)
	{
		return FALSE;
	}
	return TRUE;
}

void SeGeneralModule::AssignProcessData(const vector<CDcmPic*>& vecDcmArr)
{

}

void SeGeneralModule::AssignProcessData(CStringArray& csaDcmFiles)
{
	if (csaDcmFiles.GetSize() == 0)
	{
		Reset();
		return;
	}

 	// csaDcmFiles.RemoveAt(0, 1);
	for (int i=0; i<65536; i++)
	{
		theGeneralSwapData.m_Histogram[i] = 0;
	}
	theGeneralSwapData.m_nMaxValue = 0;
	theGeneralSwapData.m_nMinValue = 0;
	theGeneralSwapData.m_lMaxNumber = 0;
 
 	theAppIVConfig.m_pILog->ProgressInit(csaDcmFiles.GetSize());

	// 多线程加快读取速度。
	vector<CDcmPic*> vecImageList;
	queue <ImageLoadInfo> queueInfoList;
	static int nAliveThread = THREAD_NUMBER;
	nAliveThread = THREAD_NUMBER;
	for (int i=0; i<csaDcmFiles.GetSize(); i++)
	{
		// 只读取第一组数据
		// if(csaDcmFiles[i] == "1") break;
		ImageLoadInfo Imageinfo(csaDcmFiles[i], i);
		vecImageList.push_back(NULL);
		queueInfoList.push(Imageinfo);
	}

	ThreadInfo* info = new ThreadInfo(queueInfoList, vecImageList, nAliveThread);

	for (int i=0; i<THREAD_NUMBER; i++)
	{
		AfxBeginThread(__LoadImage, info);
	}
	
	while(info->nAliveThread > 0)
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
		int nNum = info->imgList[i]->GetImageNumber();
		vecNewSet.push_back(make_pair(nNum, info->imgList[i]));
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
	Safe_Delete(info);

// 	for (int i=0; i<csaDcmFiles.GetSize(); i++)
// 	{
// 		CDcmPic* pDcm = new CDcmPic();
// 		pDcm->LoadFromDcmFile(csaDcmFiles[i]);
// 		pDcm->ReloadBuffer();
// 		int nSize = pDcm->GetWidth() * pDcm->GetHeight();
// 		pDcm->AdjustWin(pDcm->GetWinCenter(), pDcm->GetWinWidth());
// 		short* pData = (short*)pDcm->GetData();
// 		short* pHeadPoint = pData;
// 		for (int i=0; i<nSize; i++)
// 		{
// 			// dcm 值的范围 -32768 - 32767
// 			theGeneralSwapData.m_nMinValue = theGeneralSwapData.m_nMinValue < *pHeadPoint ? theGeneralSwapData.m_nMinValue : *pHeadPoint;
// 			theGeneralSwapData.m_nMaxValue = theGeneralSwapData.m_nMaxValue > *pHeadPoint ? theGeneralSwapData.m_nMaxValue : *pHeadPoint;
// 			theGeneralSwapData.m_Histogram[*pHeadPoint++ + 32768] += 1;
// 		}
// 		m_OriDcmArray.AddDcmImage(pDcm);
// 		theAppIVConfig.m_pILog->ProgressStepIt();
// 	}

	for (int i=0; i<65536; i++)
	{
		theGeneralSwapData.m_lMaxNumber = theGeneralSwapData.m_lMaxNumber > theGeneralSwapData.m_Histogram[i] ? theGeneralSwapData.m_lMaxNumber : theGeneralSwapData.m_Histogram[i];
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


	theGeneralSwapData.m_csFolderPath = csFolderPath;


	// 获得 图像基本参数
	CDcmPic* pDcmTmp = m_OriDcmArray.GetDcmArray()[0]->CloneDcmPic();
	theGeneralSwapData.m_dbZSliceSpace = pDcmTmp->GetSliceSpace();
	theGeneralSwapData.m_dbXYSliceSpace = pDcmTmp->GetMMPerpixel();
	theGeneralSwapData.m_nWidth = pDcmTmp->GetWidth();
	theGeneralSwapData.m_nHeight = pDcmTmp->GetHeight();
	theGeneralSwapData.m_nDepth = m_OriDcmArray.GetDcmArray().size();
	Safe_Delete(pDcmTmp);
	theAppIVConfig.m_pILog->ProgressClose();

	m_pGeneralModuleDlg->m_pOriPicArray = &m_OriDcmArray;
	m_pGeneralModuleDlg->InitMPRPos();
	m_pGeneralModuleDlg->Init3D();
	m_pGeneralModuleDlg->SendMessage(WM_SIZE);

}

UINT SeGeneralModule::__LoadImage(LPVOID pParam)
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
				return 0;
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
				//if (temp < 0 || temp > 16382) {
				//	temp = 16382;
				//}
				theGeneralSwapData.m_nMinValue = theGeneralSwapData.m_nMinValue < temp ? theGeneralSwapData.m_nMinValue : temp;
				theGeneralSwapData.m_nMaxValue = theGeneralSwapData.m_nMaxValue > temp ? theGeneralSwapData.m_nMaxValue : temp;
				theGeneralSwapData.m_Histogram[temp + 32768] += 1;
				pHeadPoint++;
			}
			pInfo->imgList[info.Index] = pDcm;
			theAppIVConfig.m_pILog->ProgressStepIt();
		}
	}
	return 0;
}



void SeGeneralModule::AssignProcessData(CDcmPicArray* pDcmPicArray)
{
	// 获得 图像直方图
	theAppIVConfig.m_pILog->ProgressInit(pDcmPicArray->GetZDcmPicCount());
	for(int i = 0; i < pDcmPicArray->GetZDcmPicCount(); i++)
	{
		CDcmPic* pDcm = pDcmPicArray->GetDcmArray()[i]->CloneDcmPic();
		pDcm->ReloadBuffer();

		int nSize = pDcm->GetWidth() * pDcm->GetHeight();
		short* pData = (short*)pDcm->GetData();
		short* pHeadPoint = pData;
		for (int i=0; i<nSize; i++)
		{
			// dcm 值的范围 -32768 - 32767
			theGeneralSwapData.m_nMinValue = theGeneralSwapData.m_nMinValue < *pHeadPoint ? theGeneralSwapData.m_nMinValue : *pHeadPoint;
			theGeneralSwapData.m_nMaxValue = theGeneralSwapData.m_nMaxValue > *pHeadPoint ? theGeneralSwapData.m_nMaxValue : *pHeadPoint;
			theGeneralSwapData.m_Histogram[*pHeadPoint++ + 32768] += 1;
		}
		m_OriDcmArray.AddDcmImage(pDcm);
		theAppIVConfig.m_pILog->ProgressStepIt();
	}
	for (int i=0; i<65536; i++)
	{
		theGeneralSwapData.m_lMaxNumber = theGeneralSwapData.m_lMaxNumber > theGeneralSwapData.m_Histogram[i] ? theGeneralSwapData.m_lMaxNumber : theGeneralSwapData.m_Histogram[i];
	}

	// 获得 图像路径
	CString csFolderPath = pDcmPicArray->GetDcmArray()[0]->GetFilePathName();
	for(int i = csFolderPath.GetLength() - 1; i >= 0; i--)
	{
		if (csFolderPath[i] == '\\')
		{
			csFolderPath = csFolderPath.Left(i);
			break;
		}
	}
	theGeneralSwapData.m_csFolderPath = csFolderPath;

	// 获得 图像基本参数
	CDcmPic* pDcmTmp = pDcmPicArray->GetDcmArray()[0]->CloneDcmPic();
	theGeneralSwapData.m_dbZSliceSpace = pDcmTmp->GetSliceSpace();
	theGeneralSwapData.m_dbXYSliceSpace = pDcmTmp->GetMMPerpixel();
	Safe_Delete(pDcmTmp);
	theAppIVConfig.m_pILog->ProgressClose();

	m_pGeneralModuleDlg->m_pOriPicArray = &m_OriDcmArray;
	m_pGeneralModuleDlg->ShowWnds();
}

void SeGeneralModule::AssignInterface(ISeProcessMainFrame* pMainFrame)
{
	m_pMainFrame = pMainFrame;
}



CString SeGeneralModule::GetValueFromTag(CDcmPic* pDcm, int TagG, int TagE)
{
	CDcmElement* pEle;
	pEle = pDcm->GetDcmElemList().FindElem(TagG, TagE);
	CString csName;
	if (pEle != NULL)
		pEle->ValueGetString(csName);
	else
		csName = "--";
	Safe_Delete(pEle);
	return csName;
}






