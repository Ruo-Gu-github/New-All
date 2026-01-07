#include "StdAfx.h"
#include "SeBoneParaTBPf.h"
#include "SeBoneParamCube.h"
#include "SeBoneParamBasic.h"
#include "BoneDensitySwapData.h"

SeBoneParaTBPf::SeBoneParaTBPf(void)
{
}

SeBoneParaTBPf::SeBoneParaTBPf(CDcmPicArray* p, int nMinValue, int nMin, int nMax, int nWidth, int nHeight, int nSize)
{
	m_pProcessArray = p;
	m_nMinValue = nMinValue;
	m_nMin = nMin;
	m_nMax = nMax;
	m_nWidth = nWidth;
	m_nHeight = nHeight;
	m_nOriImagePiece = nSize;
	m_dMMperPixelXY = m_pProcessArray->GetMMPerXYPixel();
	m_dMMperPixelZ = m_pProcessArray->GetMMPerZPixel();
}

SeBoneParaTBPf::~SeBoneParaTBPf(void)
{
	if(m_BinImg != NULL)
	{
		delete[] m_BinImg;
		m_BinImg = NULL;
	}
}


void  SeBoneParaTBPf::DoBinaryzation()
{
	m_BinImg = new int*[m_nOriImagePiece];
	memset(m_BinImg,0,sizeof(int)*m_nOriImagePiece);
	short* slice;

	for(int z=0;z<m_nOriImagePiece;z++)
	{
		slice = (short*)m_pProcessArray->GetDcmArray()[z]->GetData();
		int* nNewSlice = new int[m_nWidth*m_nHeight];

		for(int y=0;y<m_nHeight;y++)
			for(int x=0;x<m_nWidth;x++)
			{
				short nValue = slice[x+m_nWidth*y];
				if(nValue>m_nMin&&nValue<=m_nMax)
				{
					nNewSlice[x+m_nWidth*y] = 255;
				}
				else
				{
					nNewSlice[x+m_nWidth*y] = 0;
				}
			}

			m_BinImg[z] = nNewSlice;
	}
}

double SeBoneParaTBPf::CaculateTBPf()
{
	double r = m_dMMperPixelXY;
	CCube cube;
	DoBinaryzation();
	cube.setVolum(m_BinImg,m_nWidth,m_nHeight,m_nOriImagePiece,m_dMMperPixelXY,m_dMMperPixelZ,128.0);
	cube.getTriangles();
	double S1 = cube.getSufaceArea();
	double V1 = abs(cube.getVolume());

	cube.Dilate(r);
	double S2 = cube.getSufaceArea();
	double V2 = abs(cube.getVolume());

	double TbPf = (S1-S2)/(V1-V2);

	return TbPf;
}
