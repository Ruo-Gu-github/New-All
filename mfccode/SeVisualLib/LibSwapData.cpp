#include "stdafx.h"
#include "LibSwapData.h"

LibSwapData theLibSwapData;

LibSwapData::LibSwapData()
{
	m_dbSharpen = 0.0;
}


LibSwapData::~LibSwapData()
{
}

void LibSwapData::WriteLog( char * szLog,char* Loc )
{
	SYSTEMTIME st;  
	GetLocalTime(&st);  
	FILE *fp;  
	fp=fopen("D:\\log.txt","at");  
	fprintf(fp,"MyLogInfo: %d:%d:%d:%d ",st.wHour,st.wMinute,st.wSecond,st.wMilliseconds);  
	fprintf(fp,szLog);  
	fprintf(fp,Loc);
	fclose(fp);  
	OutputDebugStringA(szLog);  
}