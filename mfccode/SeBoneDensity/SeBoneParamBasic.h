#if !defined(AFX_DIRECT3D01_H__0DF4C2EB_7902_49F3_B806_CB1E29F0FF4B__INCLUDED_)
#define AFX_DIRECT3D01_H__0DF4C2EB_7902_49F3_B806_CB1E29F0FF4B__INCLUDED_

#pragma once
#define EPS		  10e-6
#include "SeBoneParamCube.h"
struct CTriangle;

class CBasicCube;



class SeBoneParameter
{
public:
	SeBoneParameter(void);
	SeBoneParameter(CDcmPicArray* p, int nMinValue, int nMin, int nMax, int nWidth, int nHeight, int nSize);
	~SeBoneParameter(void);

public:
	static	unsigned int s_nEdgeVertex[12][2];
	static	unsigned int s_EdgeMetrix[256];
	static  unsigned int s_TriangleMetrix[256][16];
	static  unsigned int s_TriangleCountMetrix[256];
public:
	static POINT3D				GetVertex3D(POINT3D p1, POINT3D p2, double pV1, double pV2);
	static int					Polygonise(VERTEX3D* pVertex, CTriangle* pTri, unsigned int nTri, int nCubeIndex);
	static void					GenerateTriangleCount();
	void						CreateCubes(int nSlice, int nWidth, int nHeight, short* pBits1, short* pBits2);
	int							GetCubeCount();
	CBasicCube*					GetAllCubes();

public:
	int							m_nCubeCount;
	CBasicCube*					m_pCube;

private:
	CDcmPicArray*       m_pProcessArray;
	int                 m_nMinValue;
	int                 m_nMin;
	int                 m_nMax;
	int                 m_nWidth;
	int                 m_nHeight;
	int                 m_nOriImagePiece;
};

#endif