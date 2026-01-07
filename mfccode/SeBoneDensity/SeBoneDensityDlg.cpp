// SeBoneDensityDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "SeBoneDensity.h"
#include "SeBoneDensityDlg.h"
#include "BoneDensitySwapData.h"
#include "SeROIView.h"
#include "afxdialogex.h"
#include "SeAPRView.h"
#include "SeBinaryView.h"


// SeBoneDensityDlg 对话框

IMPLEMENT_DYNAMIC(SeBoneDensityDlg, CDialog)

SeBoneDensityDlg::SeBoneDensityDlg(CWnd* pParent /*=NULL*/)
	: CDialog(SeBoneDensityDlg::IDD, pParent)
{
	m_pOriPicArray = NULL;
}

SeBoneDensityDlg::~SeBoneDensityDlg()
{
// 	for (int i = 0; i < m_ZROIPicArray.GetZDcmPicCount(); i++)
// 		Safe_Delete(m_ZROIPicArray.GetDcmArray()[i]);
// 	m_ZROIPicArray.ReleaseArray();
// 
// 	for (int i = 0; i < m_RotateArray.GetZDcmPicCount(); i++)
// 		Safe_Delete(m_RotateArray.GetDcmArray()[i]);
// 	m_RotateArray.ReleaseArray();
// 
// 	for (int i = 0; i < m_BoneArray.GetZDcmPicCount(); i++)
// 		Safe_Delete(m_BoneArray.GetDcmArray()[i]);
// 	m_BoneArray.ReleaseArray();
// 
// 	for (int i = 0; i < m_ModelArray.GetZDcmPicCount(); i++)
// 		Safe_Delete(m_ModelArray.GetDcmArray()[i]);
// 	m_ModelArray.ReleaseArray();
	
}

void SeBoneDensityDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(SeBoneDensityDlg, CDialog)
	ON_WM_ERASEBKGND()
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()

BOOL SeBoneDensityDlg::OnCmdMsg( UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	return TRUE;
}

// SeBoneDensityDlg 消息处理程序


BOOL SeBoneDensityDlg::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CRect rect;
	GetClientRect(&rect);
	CZKMemDC memDC(pDC);
	memDC.FillSolidRect(rect,RGB(94,94,94));//34,33,73
	return TRUE;
	//return CDialog::OnEraseBkgnd(pDC);
}


int SeBoneDensityDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;


 
	// TODO:  在此添加您专用的创建代码
	//theBoneDensitySwapData.m_pParentDlg = this;

//  创建 View 窗口

// 创建 APR 窗口
	m_wndAPRXOYCtrl.Create(this, 0, 
		RUNTIME_CLASS(SeVisualLibDoc),
		RUNTIME_CLASS(CImageViewerFrame),
		RUNTIME_CLASS(SeAPRView),
		WS_CHILD | WS_VISIBLE, 0);
	theBoneDensitySwapData.m_pXOYView = (SeAPRView*)m_wndAPRXOYCtrl.GetView();
	theBoneDensitySwapData.m_pXOYDoc = (SeVisualLibDoc*)theBoneDensitySwapData.m_pXOYView->GetDocument();
	theBoneDensitySwapData.m_pXOYView->SetPlaneNum(1);

	m_wndAPRXOZCtrl.Create(this, 0, 
		RUNTIME_CLASS(SeVisualLibDoc),
		RUNTIME_CLASS(CImageViewerFrame),
		RUNTIME_CLASS(SeAPRView),
		WS_CHILD | WS_VISIBLE, 0);
	theBoneDensitySwapData.m_pXOZView = (SeAPRView*)m_wndAPRXOZCtrl.GetView();
	theBoneDensitySwapData.m_pXOZDoc = (SeVisualLibDoc*)theBoneDensitySwapData.m_pXOZView->GetDocument();
	theBoneDensitySwapData.m_pXOZView->SetPlaneNum(2);

	m_wndAPRYOZCtrl.Create(this, 0, 
		RUNTIME_CLASS(SeVisualLibDoc),
		RUNTIME_CLASS(CImageViewerFrame),
		RUNTIME_CLASS(SeAPRView),
		WS_CHILD | WS_VISIBLE, 0);
	theBoneDensitySwapData.m_pYOZView = (SeAPRView*)m_wndAPRYOZCtrl.GetView();
	theBoneDensitySwapData.m_pYOZDoc = (SeVisualLibDoc*)theBoneDensitySwapData.m_pYOZView->GetDocument();
	theBoneDensitySwapData.m_pYOZView->SetPlaneNum(3);

	m_wndROICtrl.Create(this, 0, 
		RUNTIME_CLASS(CImageViewerDoc),
		RUNTIME_CLASS(CImageViewerFrame),
		RUNTIME_CLASS(SeROIView),
		WS_CHILD | WS_VISIBLE, 0);
	theBoneDensitySwapData.m_pROIView = (SeROIView*)m_wndROICtrl.GetView();
	theBoneDensitySwapData.m_pROIDoc = (CImageViewerDoc*)theBoneDensitySwapData.m_pROIView->GetDocument();
	theBoneDensitySwapData.m_pROIView->SendMessage(WM_COMMAND, IDM_IMAGEVIEWER_LAYOUT_1X1);



	m_wndBinaryCtrl.Create(this, 0, 
		RUNTIME_CLASS(CImageViewerDoc),
		RUNTIME_CLASS(CImageViewerFrame),
		RUNTIME_CLASS(SeBinaryView),
		WS_CHILD | WS_VISIBLE, 0);
	theBoneDensitySwapData.m_pBinaryView = (SeBinaryView*)m_wndBinaryCtrl.GetView();
	theBoneDensitySwapData.m_pBinaryDoc = (CImageViewerDoc*)theBoneDensitySwapData.m_pBinaryView->GetDocument();
	theBoneDensitySwapData.m_pBinaryView->SendMessage(WM_COMMAND, IDM_IMAGEVIEWER_LAYOUT_1X1);
// 
// 	m_RotateBoxDlg.Create(SeRotateBoxDlg::IDD, this);
 	return 0;
}

// void SeBoneDensityDlg::ShowImage()
// {
// 	InitAPRPos();
// 
// }
// 
// 
void SeBoneDensityDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	CRect rect;
	CRect rtHide(0,0,0,0);
	GetClientRect(&rect);	
	m_wndAPRXOYCtrl.MoveWindow(rtHide);
	m_wndAPRXOZCtrl.MoveWindow(rtHide);
	m_wndAPRYOZCtrl.MoveWindow(rtHide);
	m_wndROICtrl.MoveWindow(rtHide);
	m_wndBinaryCtrl.MoveWindow(rtHide);
	switch(theBoneDensitySwapData.m_nStep)
	{
	case SP_SELECTZ:
		{
			// 闪过 旋转 Dlg 否则 APR 里没数据。
// 			m_RotateBoxDlg.MoveWindow(0,0,100,100);
// 			m_RotateBoxDlg.ShowWindow(SW_SHOW);
// 			m_RotateBoxDlg.UpdateWindow();
// 			m_RotateBoxDlg.ShowWindow(SW_HIDE);

			//三个窗口的长宽比例需要相同，否则会出错。
			CRect rtShow(&rect);
			CRect rtPart(rtShow);
			rtPart.right = rtShow.left + rtShow.Width()/2;
			rtPart.bottom = rtShow.top + rtShow.Height()/2;
			m_wndAPRYOZCtrl.MoveWindow(rtPart);
			rtPart.OffsetRect(0, rtPart.Height());
			m_wndAPRXOYCtrl.MoveWindow(rtPart);
			rtPart.OffsetRect(rtPart.Width(),0);
			m_wndAPRXOZCtrl.MoveWindow(rtPart);
			break;

		}	
	case SP_ROTATEDATA:
		{


			CRect rtShow(&rect);
			CRect rtPart(rtShow);
			rtPart.right = rtShow.left + rtShow.Width()/2;
			rtPart.bottom = rtShow.top + rtShow.Height()/2;
			m_wndAPRYOZCtrl.MoveWindow(rtPart);
			rtPart.OffsetRect(0, rtPart.Height());
			m_wndAPRXOYCtrl.MoveWindow(rtPart);
			rtPart.OffsetRect(rtPart.Width(),0);
			m_wndAPRXOZCtrl.MoveWindow(rtPart);
			break;
		}
	case SP_CALCU:
		{
			CRect rtView(&rect);
			m_wndROICtrl.MoveWindow(rtView);
		}
		break;
	case SP_BINARY:
		{
			CRect rtView(&rect);
			m_wndBinaryCtrl.MoveWindow(rtView);

			// 以后可能会重新添加3D界面
// 			CRect rtView(&rect);
// 			rtView.right = rtView.left + rect.Width()/2;
// 			m_wndBinaryCtrl.MoveWindow(rtView);
// 			rtView.OffsetRect(rtView.Width(), 0);
// 			m_wnd3DCtrl.MoveWindow(rtView);
		}
		break;
	default:
		break;
	}
}


void SeBoneDensityDlg::Reset()
{
	if (theBoneDensitySwapData.m_pROIView != NULL)
		theBoneDensitySwapData.m_pROIView->Reset();
	if (theBoneDensitySwapData.m_pBinaryView != NULL)
		theBoneDensitySwapData.m_pBinaryView->Reset();
	if (theBoneDensitySwapData.m_pXOYView != NULL)
		theBoneDensitySwapData.m_pXOYView->Reset();
	if (theBoneDensitySwapData.m_pXOZView != NULL)
		theBoneDensitySwapData.m_pXOZView->Reset();
	if (theBoneDensitySwapData.m_pYOZView != NULL)
		theBoneDensitySwapData.m_pYOZView->Reset();
// 	if (theBoneDensitySwapData.m_pROIDoc != NULL)
// 		theBoneDensitySwapData.m_pROIDoc->ReleaseSeries();
// 	if (theBoneDensitySwapData.m_pBinaryDoc != NULL)
// 		theBoneDensitySwapData.m_pBinaryDoc->ReleaseSeries();
}

void SeBoneDensityDlg::ShowFirstView()
{
	theBoneDensitySwapData.m_pXOYView->SetDcmArray(m_pOriPicArray);
	theBoneDensitySwapData.m_pXOZView->SetDcmArray(m_pOriPicArray);
	theBoneDensitySwapData.m_pYOZView->SetDcmArray(m_pOriPicArray);
	theBoneDensitySwapData.m_pXOYView->SetPlaneNum(1);
	theBoneDensitySwapData.m_pXOZView->SetPlaneNum(2);
	theBoneDensitySwapData.m_pYOZView->SetPlaneNum(3);

	theBoneDensitySwapData.m_pXOYView->SetAPRTool();
	theBoneDensitySwapData.m_pYOZView->SetAPRTool();
	theBoneDensitySwapData.m_pXOZView->SetAPRTool();

	theBoneDensitySwapData.m_pXOYView->InitView();
	theBoneDensitySwapData.m_pYOZView->InitView();
	theBoneDensitySwapData.m_pXOZView->InitView();
}



// 
// void SeBoneDensityDlg::GetRoiRegion()
// {
// 	InitRoiParamter();
// 	Load2DData();
// 	Load3DData();
// }
// 
// 
// void SeBoneDensityDlg::GetSeries()
// {
// 
// 	m_nTimes = 35;
// 	for (int i = 0; i < m_ZROIPicArray.GetZDcmPicCount(); i++)
// 		m_ZROIPicArray.GetDcmArray()[i]->SetDataInMem(true);
// 
// 	m_SeViualAPR.SetDcmArray(&m_ZROIPicArray);
// 
// 	double dMartix[16];
// 	for (int i = 0; i < 16; i++)
// 		dMartix[i] = theBoneDensitySwapData.m_p3DView->m_SeVisualV3D.GetMprRotateInfor().dModelView[i];
// 
// 	CDcmPic* pTemp = m_SeViualAPR.GetAPRImage(dMartix, 0, 0, 0, false);
// 
//  	Safe_Delete(pTemp);
// 	m_SeViualAPR.GetSeriesData(theBoneDensitySwapData.m_dPixelperPiece, &m_RotateArray);
// 
// 	SaveDcmArray(&m_RotateArray, "\\Temp\\RotateData");
// 	for (int i = 0; i < m_ZROIPicArray.GetZDcmPicCount(); i++)
// 	{
// 		m_ZROIPicArray.GetDcmArray()[i]->SetDataInMem(false);
// 		m_ZROIPicArray.GetDcmArray()[i]->ReleaseBuffer();
// 	}
// 
// 
// }
// 
// void SeBoneDensityDlg::ReleaseRotateArray()
// {
// 	theBoneDensitySwapData.m_pBoneDensityDoc->ReleaseSeries();
// 
// 	for (int i = 0; i < m_RotateArray.GetZDcmPicCount(); i++)
// 		Safe_Delete(m_RotateArray.GetDcmArray()[i]);
// 	m_RotateArray.ReleaseArray();
// 
// 	
// }
// 
// void SeBoneDensityDlg::ReleaseBoneArray()
// {
// 	for (int i = 0; i < m_BoneArray.GetZDcmPicCount(); i++)
// 		Safe_Delete(m_BoneArray.GetDcmArray()[i]);
// 	m_BoneArray.ReleaseArray();
// }
// 
// void SeBoneDensityDlg::OpenModelFile()
// {
// 	if (!m_ModelArray.GetDcmArray().empty())
// 	{
// 		for (int i = 0; i < m_ModelArray.GetZDcmPicCount(); i++)
// 			Safe_Delete(m_ModelArray.GetDcmArray()[i]);
// 		m_ModelArray.ReleaseArray();
// 	}
// 	theBoneDensitySwapData.m_pModelView->m_vecPtRgn.clear();
// 
// 	CFileDialog dlgFile(true,NULL,NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT |OFN_EXPLORER,"Dicom(.dcm)|*.dcm");
// 
// 	if (dlgFile.DoModal()==IDOK)
// 	{
// 		vector<CDcmPic*>	vecDcmPic;
// 		CString     cspath = dlgFile.GetFolderPath();
// 		CFileFind   filefind;
// 		CString     csfilePath = cspath + "\\*.dcm";
// 
// 		BOOL bFind = filefind.FindFile(csfilePath);
// 		while (bFind)
// 		{
// 			bFind = filefind.FindNextFile();
// 			if(filefind.IsDots())
// 				continue;
// 			else if(filefind.IsDirectory())
// 				continue;		
// 			else
// 			{
// 				CString   csfilename = filefind.GetFilePath();
// 				CDcmPic* pDcm = new CDcmPic;
// 				pDcm->LoadFromDcmFile(csfilename);
// 				vecDcmPic.push_back(pDcm);
// 			}
// 		}
// 		for (int j = 0; j < vecDcmPic.size() - 1; j++)
// 		{
// 			for (int i = 0; i <  vecDcmPic.size() - 1 - j; i++)
// 			{
// 				CDcmPic* temp = NULL;
// 				if(vecDcmPic[i]->GetImageNumber() > vecDcmPic[i + 1]->GetImageNumber())
// 				{
// 					temp = vecDcmPic[i];
// 					vecDcmPic[i] = vecDcmPic[i + 1];
// 					vecDcmPic[i + 1] = temp;
// 				}
// 			}
// 		}
// 
// 		theBoneDensitySwapData.m_pModelDoc->ReleaseSeries();
// 		theBoneDensitySwapData.m_pModelView->SetLayoutFormat(1,1);
// 
// 		for (int i = 0 ; i < vecDcmPic.size() ; i++)
// 		{
// 			m_ModelArray.AddDcmImage(vecDcmPic[i]);
// 			theBoneDensitySwapData.m_pModelDoc->AddImage(vecDcmPic[i], -1, FALSE);
// 		}
// 	
// 		vecDcmPic.clear();
// 	}
// 
// }
// 
// void SeBoneDensityDlg::ReturnToLastStep()
// {
// 	switch (theBoneDensitySwapData.m_nStep)
// 	{
// 	case SP_SELECTZ:
// 		{
// 			ShowImage();
// 			break;
// 		}
// 	case SP_ROTATEDATA:
// 		{
// 			ShowImage();
// 			theBoneDensitySwapData.m_pXOYView->SetAPRRectTool();
// 			theBoneDensitySwapData.m_pYOZView->SetAPRRectTool();
// 			theBoneDensitySwapData.m_pXOZView->SetAPRRectTool();
// 			theBoneDensitySwapData.m_pXOYView->Invalidate(FALSE);
// 			theBoneDensitySwapData.m_pXOYView->UpdateWindow();
// 			theBoneDensitySwapData.m_pXOZView->Invalidate(FALSE);
// 			theBoneDensitySwapData.m_pXOZView->UpdateWindow();
// 			theBoneDensitySwapData.m_pYOZView->Invalidate(FALSE);
// 			theBoneDensitySwapData.m_pYOZView->UpdateWindow();
// 			break;
// 		}
// 	case SP_BINARY:
// 		{
// 			ReleaseBoneArray();
// 			ReLoadBeforeBinaryData();			
// 		}
// 		break;
// 	case SP_CALCU:
// 		{
// 			ReleaseRotateArray();
// 			ReleaseBoneArray();
// 			ReLoadBeforeRoiSelectData();
// 			theBoneDensitySwapData.m_pBoneDensityView->m_bInterported = false;
// 			theBoneDensitySwapData.m_pBoneDensityView->SetMouseTool(MT_Select);
// 		}
// 		break;
// 	}
// }
// 
// void SeBoneDensityDlg::ReLoadBeforeBinaryData()
// {
// 	CString cspath = theBoneDensitySwapData.m_csFolderPath + "\\ROIData";
// 	CString     csfilePath = cspath + "\\*.dcm";
// 	CFileFind   filefind;
// 	BOOL bFind = filefind.FindFile(csfilePath);
// 	vector<CDcmPic*> vecDcmPic;
// 	while (bFind)
// 	{
// 		bFind = filefind.FindNextFile();
// 		if(filefind.IsDots())
// 			continue;
// 		else if(filefind.IsDirectory())
// 			continue;		
// 		else
// 		{
// 			CString   csfilename = filefind.GetFilePath();
// 			CDcmPic* pDcm = new CDcmPic;
// 			pDcm->LoadFromDcmFile(csfilename);
// 			vecDcmPic.push_back(pDcm);
// 		}
// 	}
// 	for (int j = 0; j < vecDcmPic.size() - 1; j++)
// 	{
// 		for (int i = 0; i <  vecDcmPic.size() - 1 - j; i++)
// 		{
// 			CDcmPic* temp = NULL;
// 			if(vecDcmPic[i]->GetImageNumber() > vecDcmPic[i + 1]->GetImageNumber())
// 			{
// 				temp = vecDcmPic[i];
// 				vecDcmPic[i] = vecDcmPic[i + 1];
// 				vecDcmPic[i + 1] = temp;
// 			}
// 		}
// 	}
// 
// // 	theBoneDensitySwapData.m_pBoneAnalysisDoc->ReleaseSeries();
// // 	theBoneDensitySwapData.m_pBoneAnalysisView->SetLayoutFormat(1,1);
// 
// 	for (int i = 0 ; i < vecDcmPic.size() ; i++)
// 	{
// 		m_BoneArray.AddDcmImage(vecDcmPic[i]);
// 		vecDcmPic[i]->SetDataInMem(true);
// 		theBoneDensitySwapData.m_pBoneAnalysisDoc->AddImage(vecDcmPic[i], -1, FALSE);
// 	}
// 
// 	vecDcmPic.clear();
// }
// 
// 
// void SeBoneDensityDlg::ReLoadBeforeRoiSelectData()
// {
// 	CString		cspath = theBoneDensitySwapData.m_csFolderPath + "\\Temp\\RotateData";
// 	CString     csfilePath = cspath + "\\*.dcm";
// 	CFileFind   filefind;
// 	BOOL bFind = filefind.FindFile(csfilePath);
// 	vector<CDcmPic*> vecDcmPic;
// 	while (bFind)
// 	{
// 		bFind = filefind.FindNextFile();
// 		if(filefind.IsDots())
// 			continue;
// 		else if(filefind.IsDirectory())
// 			continue;		
// 		else
// 		{
// 			CString   csfilename = filefind.GetFilePath();
// 			CDcmPic* pDcm = new CDcmPic;
// 			pDcm->LoadFromDcmFile(csfilename);
// 			vecDcmPic.push_back(pDcm);
// 		}
// 	}
// 	for (int j = 0; j < vecDcmPic.size() - 1; j++)
// 	{
// 		for (int i = 0; i <  vecDcmPic.size() - 1 - j; i++)
// 		{
// 			CDcmPic* temp = NULL;
// 			if(vecDcmPic[i]->GetImageNumber() > vecDcmPic[i + 1]->GetImageNumber())
// 			{
// 				temp = vecDcmPic[i];
// 				vecDcmPic[i] = vecDcmPic[i + 1];
// 				vecDcmPic[i + 1] = temp;
// 			}
// 		}
// 	}
// 
// 	//theBoneDensitySwapData.m_pBoneDensityDoc->ReleaseSeries();
// 	theBoneDensitySwapData.m_pBoneDensityView->SetLayoutFormat(1,1);
// 
// 	for (int i = 0 ; i < vecDcmPic.size() ; i++)
// 	{
// 
// 		m_RotateArray.AddDcmImage(vecDcmPic[i]);
// 		vecDcmPic[i]->SetDataInMem(true);
// 		theBoneDensitySwapData.m_pBoneDensityDoc->AddImage(vecDcmPic[i], -1, FALSE);
// 	}
// 	vecDcmPic.clear();
// }
// 
// void SeBoneDensityDlg::Reset()
// {
// 
// 	theBoneDensitySwapData.m_pOriImageDoc->ReleaseSeries();
// 	theBoneDensitySwapData.m_pModelDoc->ReleaseSeries();
// 
// 	for (int i = 0; i < m_ZROIPicArray.GetZDcmPicCount(); i++)
// 		Safe_Delete(m_ZROIPicArray.GetDcmArray()[i]);
// 	m_ZROIPicArray.ReleaseArray();
// 	for (int i = 0; i < m_RotateArray.GetZDcmPicCount(); i++)
// 		Safe_Delete(m_RotateArray.GetDcmArray()[i]);
// 	m_RotateArray.ReleaseArray();
// 	for (int i = 0; i < m_BoneArray.GetZDcmPicCount(); i++)
// 		Safe_Delete(m_BoneArray.GetDcmArray()[i]);
// 	m_BoneArray.ReleaseArray();
// 	for (int i = 0; i < m_ModelArray.GetZDcmPicCount(); i++)
// 		Safe_Delete(m_ModelArray.GetDcmArray()[i]);
// 	m_ModelArray.ReleaseArray();
// 	m_pOriPicArray = NULL;
// }
// 
// void SeBoneDensityDlg::InitRoiParamter()
// {
// // 	CRect rtXOY = theBoneDensitySwapData.m_pXOY->m_objMgr.GetHead()->GetDimRect();
// // 	theBoneDensitySwapData.m_nXstart = rtXOY.left;
// // 	theBoneDensitySwapData.m_nWidth = rtXOY.Width() - rtXOY.Width()%4;
// // 	theBoneDensitySwapData.m_nYstart = rtXOY.top;
// // 	theBoneDensitySwapData.m_nHeight = rtXOY.Height();
// // 	CRect rtYOZ = theBoneDensitySwapData.m_pYOZ->m_objMgr.GetHead()->GetDimRect();
// // 	theBoneDensitySwapData.m_nZstart = rtYOZ.top/theBoneDensitySwapData.m_dPixelperPiece;
// // 	theBoneDensitySwapData.m_nZpiece = rtYOZ.Height()/theBoneDensitySwapData.m_dPixelperPiece;
// }
// 
// void SeBoneDensityDlg::Load2DData()
// {
// 	CString	 csZROIPath = theBoneDensitySwapData.m_csFolderPath + "\\Temp\\ZROI";
// 	theBoneDensitySwapData.CreateFolder(csZROIPath);
// 	m_ZROIPicArray.ReleaseArray();
// 	for (int i = 0; i < m_ZROIPicArray.GetZDcmPicCount(); i++)
// 		Safe_Delete(m_ZROIPicArray.GetDcmArray()[i]);
// 	m_ZROIPicArray.ReleaseArray();
// 
// 	theAppIVConfig.m_pILog->ProgressInit(theBoneDensitySwapData.m_nZpiece);
// 
// 	for(int i = 0; i < theBoneDensitySwapData.m_nZpiece; i++)
// 	{
// 		theAppIVConfig.m_pILog->ProgressStepIt();
// 		LoadSingleDcm(csZROIPath, i);
// 	}
// 	theAppIVConfig.m_pILog->ProgressClose();
// }
// 
// void SeBoneDensityDlg::LoadSingleDcm(CString path, int index)
// {
// 	int nWidth = theBoneDensitySwapData.m_nWidth;
// 	int nHeight = theBoneDensitySwapData.m_nHeight;
// 	int nOriWidth = m_pOriPicArray->GetDcmArray()[0]->GetWidth();
// 	short* pTempData = new short[nWidth*nHeight];
// 	CDcmPic* pDcmOri = m_pOriPicArray->GetDcmArray()[index + theBoneDensitySwapData.m_nZstart];
// 	pDcmOri->ReloadBuffer();
// 	for (int x = 0; x < nHeight; x++) 
// 	{
// 		for (int y = 0; y < nWidth; y++)
// 			pTempData[x*nWidth + y] = ((short*)pDcmOri->GetData())[(x + theBoneDensitySwapData.m_nYstart)*nOriWidth + (y + theBoneDensitySwapData.m_nXstart)];
// 	}
// 	pDcmOri->SetDataInMem(false);
// 	pDcmOri->ReleaseBuffer();
// 
// 	CDcmPic* pZROI = pDcmOri->CloneDcmPic();
// 	pZROI->SetPixelData((BYTE*)pTempData, nWidth, nHeight);
// 
// 	CString csIndex;
// 	csIndex.Format("%d", index);
// 	CString csFilenName = path + "\\" + csIndex +".dcm";
// 
// 	pZROI->ExportDcm(csFilenName);
// 	Safe_Delete(pZROI);
// 
// 	CDcmPic* pDcm = new CDcmPic;
// 	pDcm->LoadFromDcmFile(csFilenName);
// 	m_ZROIPicArray.AddDcmImage(pDcm);
// }
// 
// void SeBoneDensityDlg::Load3DData()
// {
// 	for (int i = 0; i < m_ZROIPicArray.GetZDcmPicCount(); i++)
// 		m_ZROIPicArray.GetDcmArray()[i]->ReloadBuffer();
// 	((SeBoneDensity3DView*)m_wnd3DCtrl.GetView())->Init(&m_ZROIPicArray);
// 	((SeBoneDensity3DView*)m_wnd3DCtrl.GetView())->Invalidate(FALSE);
// 	for (int i = 0; i < m_ZROIPicArray.GetZDcmPicCount(); i++)
// 	{
// 		m_ZROIPicArray.GetDcmArray()[i]->SetDataInMem(false);
// 		m_ZROIPicArray.GetDcmArray()[i]->ReleaseBuffer();
// 	}
// }
// 
// void SeBoneDensityDlg::SaveDcmArray(CDcmPicArray* pDcmArray, CString path)
// {
// 	CString csRotateDataFolder = theBoneDensitySwapData.m_csFolderPath + path;
// 	theBoneDensitySwapData.CreateFolder(csRotateDataFolder);
// 
// 	for (int i = 0; i < pDcmArray->GetZDcmPicCount(); i++)
// 	{
// 		CString csIndex;
// 		csIndex.Format("%d", i);
// 		CString csPath = csRotateDataFolder + "\\" + csIndex + ".dcm";
// 		m_RotateArray.GetDcmArray()[i]->ExportDcm(csPath);
// 	}
// }
// 
// void SeBoneDensityDlg::InitAPRPos()
// {
// 	for (int i = 0; i < m_pOriPicArray->GetZDcmPicCount(); i++)
// 		m_pOriPicArray->GetDcmArray()[i]->ReloadBuffer();
// 	theBoneDensitySwapData.m_pXOYView->SetDcmArray(m_pOriPicArray);
// 	theBoneDensitySwapData.m_pXOZView->SetDcmArray(m_pOriPicArray);
// 	theBoneDensitySwapData.m_pYOZView->SetDcmArray(m_pOriPicArray);
// 	theBoneDensitySwapData.m_pXOYView->SetPlaneNum(1);
// 	theBoneDensitySwapData.m_pXOZView->SetPlaneNum(2);
// 	theBoneDensitySwapData.m_pYOZView->SetPlaneNum(3);
// 
// 	theBoneDensitySwapData.m_pXOYView->SetAPRTool();
// 	theBoneDensitySwapData.m_pYOZView->SetAPRTool();
// 	theBoneDensitySwapData.m_pXOZView->SetAPRTool();
// 
// 	theBoneDensitySwapData.m_pXOYView->InitView();
// 	theBoneDensitySwapData.m_pYOZView->InitView();
// 	theBoneDensitySwapData.m_pXOZView->InitView();
// }
// 
// void SeBoneDensityDlg::GetROIArray()
// {
// 	theBoneDensitySwapData.m_pXOYView->ClipAllImage();
// 	ReleaseRotateArray();
// 	ReLoadBeforeRoiSelectData();
// 
// }







