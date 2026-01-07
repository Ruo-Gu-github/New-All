#include "StdAfx.h"
#include "BoneDensitySwapData.h"

BoneDensitySwapData		theBoneDensitySwapData;

BoneDensitySwapData::BoneDensitySwapData(void)
{
	m_pXOYView = NULL;
	m_pXOYDoc = NULL;
	m_pXOZView = NULL;
	m_pXOZDoc = NULL;
	m_pYOZView = NULL;
	m_pYOZDoc = NULL;
	m_pROIView = NULL;
	m_pROIDoc = NULL;
	m_pBinaryView = NULL;
	m_pBinaryDoc = NULL;

	
	m_nSamllValue = 0;	
	m_nBigValue = 0;	
	m_fSmallDensity = 0.0;
	m_fBigDensity = 0.0;

	m_bPosChanged = FALSE;

	m_csSegPartName = "trabecular";
	m_bSeg2Cal = FALSE;
	m_vecFuncList.clear();

	m_tmpSize = 0;
// 	FuncSingle temp;
// 	temp.strFuncName = "close";
// 	temp.strKernelType = "square";
// 	temp.nKernelSize = 9;
// 	temp.bSel = FALSE;
// 	m_vecFuncList.push_back(temp);
// 
// 	temp.strFuncName = "open";
// 	temp.strKernelType = "square";
// 	temp.nKernelSize = 9;
// 	temp.bSel = FALSE;
// 	m_vecFuncList.push_back(temp);
// 
// 	temp.strFuncName = "反白";
// 	temp.strKernelType = "无";
// 	temp.nKernelSize = 0;
// 	temp.bSel = TRUE;
// 	m_vecFuncList.push_back(temp);
// 
// 	temp.strFuncName = "得到孔洞";
// 	temp.strKernelType = "无";
// 	temp.nKernelSize = 0;
// 	temp.bSel = TRUE;
// 	m_vecFuncList.push_back(temp);
// 
// 	temp.strFuncName = "开操作";
// 	temp.strKernelType = "圆盘";
// 	temp.nKernelSize = 5;
// 	temp.bSel = FALSE;
// 	m_vecFuncList.push_back(temp);
// 
// 	temp.strFuncName = "闭操作";
// 	temp.strKernelType = "圆盘";
// 	temp.nKernelSize = 21;
// 	temp.bSel = TRUE;
// 	m_vecFuncList.push_back(temp);

	//
	m_mapFunc.insert(pair<CString, int>("reverse",0));
	m_mapFunc.insert(pair<CString, int>("gethole",1));
	m_mapFunc.insert(pair<CString, int>("fillhole",2));
	m_mapFunc.insert(pair<CString, int>("corrosion",3));
	m_mapFunc.insert(pair<CString, int>("dilate",4));
	m_mapFunc.insert(pair<CString, int>("open",5));
	m_mapFunc.insert(pair<CString, int>("close",6));
	m_mapFunc.insert(pair<CString, int>("circle",0));
	m_mapFunc.insert(pair<CString, int>("square",1));
	m_mapFunc.insert(pair<CString, int>("none",2));
	m_mapFunc.insert(pair<CString, int>("red",0));
	m_mapFunc.insert(pair<CString, int>("green",1));
	m_mapFunc.insert(pair<CString, int>("both",2));
	m_mapFunc.insert(pair<CString, int>("trabecular",0));
	m_mapFunc.insert(pair<CString, int>("corticalbone",1));

}



BoneDensitySwapData::~BoneDensitySwapData(void)
{
	m_pXOYView = NULL;
	m_pXOYDoc = NULL;
	m_pXOZView = NULL;
	m_pXOZDoc = NULL;
	m_pYOZView = NULL;
	m_pYOZDoc = NULL;
	m_pROIView = NULL;
	m_pROIDoc = NULL;
	m_pBinaryView = NULL;
	m_pBinaryDoc = NULL;
}

