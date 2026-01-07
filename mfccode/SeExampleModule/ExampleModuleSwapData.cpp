#include "StdAfx.h"
#include "SeExampleView.h"
#include "SeFattyOriView.h"
#include "SeProjectionView.h"
#include "SeFattyZSelectView.h"
#include "ExampleModuleSwapData.h"

ExampleModuleSwapData		theExampleModuleSwapData;

ExampleModuleSwapData::ExampleModuleSwapData(void)
{
	m_pExampleView = NULL;
	m_pExampleDoc = NULL;
	m_pProjectionData = NULL;
	m_pSrcData = NULL;
}



ExampleModuleSwapData::~ExampleModuleSwapData(void)
{
	m_pExampleView = NULL;
	m_pExampleDoc = NULL;
	Safe_Delete(m_pProjectionData);
	Safe_Delete(m_pSrcData);
	m_pProjectionData = NULL;
}

