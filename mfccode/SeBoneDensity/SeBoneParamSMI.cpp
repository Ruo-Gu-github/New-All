#include "StdAfx.h"
#include "SeBoneParamSMI.h"
#include "SeBoneParamCube.h"
#include "SeBoneParamBasic.h"
#include "BoneDensitySwapData.h"


SeBoneParamSMI::SeBoneParamSMI(void)
{
}

SeBoneParamSMI::SeBoneParamSMI(CDcmPicArray* p, int nMinValue, int nMin, int nMax, int nWidth, int nHeight, int nSize)
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

SeBoneParamSMI::~SeBoneParamSMI(void)
{
	if (m_ReSImg != NULL)
	{
		delete[] m_ReSImg;
		m_ReSImg = NULL;
	}
}


void SeBoneParamSMI::DoBinaryzation()
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

void SeBoneParamSMI::Resample(int factorX,int factorY,int factorZ)
{
	int w = m_nWidth;
	int h = m_nHeight;
	int d = m_nOriImagePiece;

	m_ReSImg = new int*[1+(d-1)/factorZ];

	int* histo = new int[256];

	for(int z=0;z<d;z+=factorZ)
	{
		int kfactor = (z+factorZ<d?factorZ:d-z);

		int** slices = new int*[kfactor];
		//byte** slices = new byte*[kfactor];
		for(int k=0;k<kfactor;k++)
			slices[k] = m_BinImg[z+k];
		//	slices[k] = (byte*)m_BinImg[z+k];

		int pointsInNewSlice = (1+(w-1)/factorX)*(1+(h-1)/factorY);
		int* newSlice = new int[pointsInNewSlice];
		//byte* newSlice = new byte[pointsInNewSlice];

		for(int y=0;y<h;y+=factorY){
			for (int x=0;x<w;x+=factorX){
				int ifactor = (x+factorX<w?factorX:w-x);
				int jfcator = (y+factorY<h?factorY:h-y);
				
				int highest = -1;
				int indexOfHighest = -1;
				memset(histo,0,sizeof(int)*256);
				int nValue;

				for(int i=0;i<ifactor;i++){
					for(int j=0;j<jfcator;j++){
						for(int k=0;k<kfactor;k++)
						{
							nValue = slices[k][x+i+w*(y+j)];
							histo[nValue]++;

							if (histo[nValue]>highest)
							{
								highest = histo[nValue];
								indexOfHighest = nValue;
							}
						}
					}
				}
						newSlice[(x/factorX)+(int)((w-1)/factorX+1)*(y/factorY)] = indexOfHighest;
						//newSlice[(x/factorX)+(w/factorX)*(y/factorY)] = (byte)indexOfHighest;

			}
		}
		m_ReSImg[z/factorZ] = newSlice;

		if(slices !=NULL)
		{
			delete[] slices;
			slices = NULL;
		}
	}

	if(histo != NULL)
	{
		delete[] histo;
		histo = NULL;
	}

	m_nReWidth = 1+(w-1)/factorX;
	m_nReHeight = 1+(h-1)/factorY;
	m_nReSize = 1+(d-1)/factorZ;

	m_dblRePixelXY = m_dMMperPixelXY*m_nWidth/(double)m_nReWidth;
	m_dblRePixelZ = m_dMMperPixelZ*m_nOriImagePiece/(double)m_nReSize;

	delete[] m_BinImg;
	m_BinImg = NULL;
}

double SeBoneParamSMI::CalculateVolum()
{
	double v = 0.0;
	for(int z=0;z<m_nReSize;z++)
		for(int x=0;x<m_nReWidth;x++)
			for(int y=0;y<m_nReHeight;y++)
			{
				if(m_ReSImg[z][x+m_nReWidth*y] == 255)
				{
					v++;
				}
			}
			v = v*m_dblRePixelXY*m_dblRePixelXY*m_dblRePixelZ;

			return v;
}

double SeBoneParamSMI::CalculatSMI(int nReVolum)
{
	double dblSMI = 0;
	double r ;
	//r = m_dblRePixelXY*0.01;
	DoBinaryzation();
	Resample(nReVolum,nReVolum,nReVolum);
	r = m_dblRePixelXY*0.01;

	CCube cube ;
	cube.setVolum(m_ReSImg, m_nReWidth, m_nReHeight, m_nReSize, m_dblRePixelXY, m_dblRePixelZ,128.0);
	cube.getTriangles();
	double volume = abs(cube.getVolume());
	//double v = CalculateVolum();
	double area = cube.getSufaceArea();
	cube.Dilate(r);
	double dltarea = cube.getSufaceArea();

	double sR = (dltarea-area)/r;
	dblSMI = 6*sR*volume/(area*area);
	return dblSMI;
}