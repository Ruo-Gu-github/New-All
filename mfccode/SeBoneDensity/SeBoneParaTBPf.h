#pragma once
class SeBoneParaTBPf
{
public:
	SeBoneParaTBPf(void);
	SeBoneParaTBPf(CDcmPicArray* p, int nMinValue, int nMin, int nMax, int nWidth, int nHeight, int nSize);
	virtual ~SeBoneParaTBPf(void);
private:
	CDcmPicArray*        m_pProcessArray;
	int**               m_BinImg;
	int                 m_nMinValue;
	int                 m_nMin;
	int                 m_nMax;
	int                 m_nWidth;
	int                 m_nHeight;
	int                 m_nOriImagePiece;
	double              m_dMMperPixelXY;
	double              m_dMMperPixelZ;
public:
	void                DoBinaryzation();
	double                CaculateTBPf();
};

