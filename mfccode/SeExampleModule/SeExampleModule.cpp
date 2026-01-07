// SeExampleModule.cpp : 定义 DLL 的初始化例程。
//


#include "stdafx.h"
#include "SeExampleModule.h"
#include "SeExampleModuleDlg.h"
#include "SeExampleModuleCtrlDlg.h"
#include "ExampleModuleSwapData.h"
#include "SeExampleView.h"

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


SeExampleModule*	g_pExampleModule;
SeExampleModule::SeExampleModule(void)
{
	g_pExampleModule = this;
	m_pExampleModuleDlg = NULL;
	m_pExampleModuleCtrlDlg = NULL;
}

SeExampleModule::~SeExampleModule(void)
{
	for (int i=0; i<m_ExampleArray.GetZDcmPicCount(); i++)
		Safe_Delete(m_ExampleArray.GetDcmArray()[i]);
	m_ExampleArray.ReleaseArray();
}

BOOL SeExampleModule::Initialize()
{
	return TRUE;
}

void SeExampleModule::ExitInstance()
{
	Safe_Delete(m_pExampleModuleDlg);
	Safe_Delete(m_pExampleModuleCtrlDlg);
}

void SeExampleModule::Release()
{
	delete this;
}

void SeExampleModule::Reset()
{
	m_pExampleModuleCtrlDlg->Reset();
	m_pExampleModuleDlg->Reset();
	for (int i=0; i<m_ExampleArray.GetZDcmPicCount(); i++)
		Safe_Delete(m_ExampleArray.GetDcmArray()[i]);
	m_ExampleArray.ReleaseArray();

	for (int i = 0; i < m_SliceArray.GetZDcmPicCount(); i++)
		Safe_Delete(m_SliceArray.GetDcmArray()[i]);
	m_SliceArray.ReleaseArray();
}

CString SeExampleModule::GetCaption()
{
	return "脂肪智能识别";
}

CWnd* SeExampleModule::CreateUI(CWnd* pParent)
{
	if (m_pExampleModuleDlg == NULL)
	{
		m_pExampleModuleDlg = new SeExampleModuleDlg;
		m_pExampleModuleDlg->Create(SeExampleModuleDlg::IDD, pParent);
		m_pExampleModuleDlg->SetParent(pParent);
	}
	return m_pExampleModuleDlg;
}

CWnd* SeExampleModule::GetUI()
{
	return m_pExampleModuleDlg;
}

HICON SeExampleModule::GetIcon()
{
	return AfxGetApp()->LoadIcon(IDI_ICON_EXAMPLE);
}

BOOL SeExampleModule::CanProcess(ProcessModule emModule)
{
	return TRUE;
}

BOOL SeExampleModule::InitProcess(ProcessModule emModule)
{
	BOOL bCanProcess = CanProcess(emModule);
	if (!bCanProcess)
	{
		return FALSE;
	}
	return TRUE;
}

void SeExampleModule::AssignProcessData(const vector<CDcmPic*>& vecDcmArr)
{

}

void SeExampleModule::AssignProcessData(CStringArray& csaDcmFiles)
{
	if (csaDcmFiles.GetSize() == 0)
	{
		AfxMessageBox("未选择图像");
		return;
	}

	if (theExampleModuleSwapData.m_pProjectionData != NULL) {
		theExampleModuleSwapData.m_pProjectionData = NULL;
	}
	if (theExampleModuleSwapData.m_pSrcData != NULL) {
		Safe_Delete(theExampleModuleSwapData.m_pSrcData);
		theExampleModuleSwapData.m_pSrcData = NULL;
	}
	theAppIVConfig.m_pILog->ProgressInit(csaDcmFiles.GetSize());
	theExampleModuleSwapData.m_nLength = csaDcmFiles.GetSize();
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

	for (int i=0; i<csaDcmFiles.GetSize(); i++)
	{
		m_ExampleArray.AddDcmImage(pInfo->imgList[i]);
	}
	Safe_Delete(pInfo);

	for (int i=0; i<65536; i++)
	{
		theExampleModuleSwapData.m_lMaxNumber = theExampleModuleSwapData.m_lMaxNumber > theExampleModuleSwapData.m_Histogram[i] ? theExampleModuleSwapData.m_lMaxNumber : theExampleModuleSwapData.m_Histogram[i];
	}


	// 获得 图像路径
	CString csFolderPath = m_ExampleArray.GetDcmArray()[0]->GetFilePathName();
	for(int i = csFolderPath.GetLength() - 1; i >= 0; i--)
	{
		if (csFolderPath[i] == '\\')
		{
			csFolderPath = csFolderPath.Left(i);
			break;
		}
	}
	theExampleModuleSwapData.m_csFolderPath = csFolderPath;

	// 获得 图像基本参数
	CDcmPic* pDcmTmp = m_ExampleArray.GetDcmArray()[0]->CloneDcmPic();
	theExampleModuleSwapData.m_dbZSliceSpace = pDcmTmp->GetSliceSpace();
	theExampleModuleSwapData.m_dbXYSliceSpace = pDcmTmp->GetMMPerpixel();
	theExampleModuleSwapData.m_csSeriesName = pDcmTmp->GetSeriesInstanceID();
	theExampleModuleSwapData.m_nWidth = pDcmTmp->GetWidth();
	theExampleModuleSwapData.m_nHeight = pDcmTmp->GetHeight();
	Safe_Delete(pDcmTmp);
	theAppIVConfig.m_pILog->ProgressClose();

	theExampleModuleSwapData.m_nStep = 0;
	m_pExampleModuleDlg->SendMessage(WM_SIZE);
	m_pExampleModuleCtrlDlg->SendMessage(WM_SIZE);

	CDcmPic* pDcm = m_ExampleArray.GetDcmArray()[0]->CloneDcmPic();
	pDcm->SetPixelData((BYTE*)theExampleModuleSwapData.m_pProjectionData, pDcm->GetWidth(), csaDcmFiles.GetSize());
	m_SliceArray.AddDcmImage(pDcm);

	m_pExampleModuleDlg->m_pOriDcmArray = &m_ExampleArray;
	m_pExampleModuleDlg->m_pProjectionArray = &m_SliceArray;
	m_pExampleModuleDlg->ShowFirstView();

	TRACE("AssignProcessData: m_pExampleModuleDlg=%p, m_pOriDcmArray=%p, &m_ExampleArray=%p\n",
		m_pExampleModuleDlg, m_pExampleModuleDlg->m_pOriDcmArray, &m_ExampleArray);
}

void SeExampleModule::AssignProcessData(CDcmPicArray* pDcmPicArray)
{
	
}

void SeExampleModule::AssignInterface(ISeProcessMainFrame* pMainFrame)
{
	m_pMainFrame = pMainFrame;
}

CWnd* SeExampleModule::CreateCtrlUI(CWnd* pParent)
{
	if (m_pExampleModuleCtrlDlg == NULL)
	{
		m_pExampleModuleCtrlDlg = new SeExampleModuleCtrlDlg;
		m_pExampleModuleCtrlDlg->Create(SeExampleModuleCtrlDlg::IDD, pParent);
		m_pExampleModuleCtrlDlg->SetParent(pParent);
	}
	return m_pExampleModuleCtrlDlg;
}

CWnd* SeExampleModule::GetCtrlUI(CWnd* pParent)
{
	return m_pExampleModuleCtrlDlg;
}

UINT SeExampleModule::__LoadImage(LPVOID pParam)
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
			int nWidth = pDcm->GetWidth();
			int nHeight = pDcm->GetHeight();
			int nLength = theExampleModuleSwapData.m_nLength;
			int nSize = pDcm->GetWidth() * pDcm->GetHeight();
			short* pData = (short*)pDcm->GetData();

			if (theExampleModuleSwapData.m_pProjectionData == NULL) {
				theExampleModuleSwapData.m_pProjectionData = new short[nWidth * nLength];
			}
			if (theExampleModuleSwapData.m_pSrcData == NULL) {
				theExampleModuleSwapData.m_pSrcData = new short[nWidth * nHeight * nLength];
				memset(theExampleModuleSwapData.m_pSrcData, 0, sizeof(short) * nWidth * nHeight * nLength);
			}
			short* pHeadPoint = pData;
			for (int i = 0; i < nWidth; ++i) {
				double fTotal = 0.0;
				int offset = i;  // 计算初始偏移
				for (int j = 0; j < nHeight; ++j, offset += nWidth) {
					fTotal += static_cast<double>(pData[offset]);
				}

				fTotal = fTotal * 2.0 / static_cast<double>(nHeight);
				if (fTotal > 16383.0) fTotal = 16383.0;

				theExampleModuleSwapData.m_pProjectionData[nWidth * (nLength - info.Index - 1) + i] = static_cast<short>(fTotal);
			}
			for (int i=0; i<nSize; i++)
			{
				// dcm 值的范围 -32768 - 32767
				theExampleModuleSwapData.m_nMinValue = theExampleModuleSwapData.m_nMinValue < *pHeadPoint ? theExampleModuleSwapData.m_nMinValue : *pHeadPoint;
				theExampleModuleSwapData.m_nMaxValue = theExampleModuleSwapData.m_nMaxValue > *pHeadPoint ? theExampleModuleSwapData.m_nMaxValue : *pHeadPoint;
				theExampleModuleSwapData.m_pSrcData[info.Index * nSize + i] = *pHeadPoint;
				theExampleModuleSwapData.m_Histogram[*pHeadPoint++ + 32768] += 1;
				
			}

			pInfo->imgList[info.Index] = pDcm;
			theAppIVConfig.m_pILog->ProgressStepIt();
		}
	}
	return 0;
}


