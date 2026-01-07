#include "StdAfx.h"
#include "GeneralSwapData.h"
#include "SeGeneralModuleDlg.h"
#include "SeGeneralModuleCtrlDlg.h"

GeneralSwapData	theGeneralSwapData;
GeneralSwapData::GeneralSwapData(void)
{
	m_pXOYView = NULL;
	m_pYOZView = NULL;
	m_pXOZView = NULL;
	m_p3DView = NULL;
}


GeneralSwapData::~GeneralSwapData(void)
{
	m_pXOYView = NULL;
	m_pYOZView = NULL;
	m_pXOZView = NULL;
	m_p3DView = NULL;
}
