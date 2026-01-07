#pragma once
struct LibSwapData
{
public:
	LibSwapData();
	~LibSwapData();
public:
	MPRPObserverHost m_MPROberverHost;
	void							WriteLog(char * szLog,char* Loc);
	CSize m_szDelta;

	int						m_nWinCenter;
	int						m_nWinWidth;

	double					m_dbSharpen;

	img_process				m_ip;

};

extern LibSwapData SEV_EXT_CLASS theLibSwapData;

