// Cube.h: interface for the CCube class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CUBE_H__804013F6_6081_4316_9D20_00E042282694__INCLUDED_)
#define AFX_CUBE_H__804013F6_6081_4316_9D20_00E042282694__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "BoneDensitySwapData.h"
#include "BasicCube.h"

// typedef struct SPOINT3D 
// {
// 	SPOINT3D(){}
// 	SPOINT3D(double x, double y, double z)
// 	{
// 		fx = x;
// 		fy = y;
// 		fz = z;
// 	}
// 
// 	double	fx;
// 	double	fy;
// 	double	fz;
// 
// 	bool operator < ( const SPOINT3D &rhs) const 
// 	{
// 		if (fx<rhs.fx)
// 		{
// 			return true;
// 		}
// 		else if (fx>rhs.fx)
// 		{
// 			return false;
// 		}
// 		else if (fy<rhs.fy)
// 		{
// 			return true;
// 		}
// 		else if (fy>rhs.fy)
// 		{
// 			return false;
// 		}
// 		else if (fz<rhs.fz)
// 		{
// 			return true;
// 		}
// 		else if (fz>rhs.fz)
// 		{
// 			return false;
// 		}
// 		else
// 		{
// 			return false;
// 		}
// 	}
// }POINT3D;
// 
// struct VERTEX3D
// {
// 	POINT3D		point;
// 	int			pointVal;
// };

struct CTriangle
{
	POINT3D		m_point[3];
};


struct Volume
{
	int**   m_volume;
	int     m_nWidth;
	int     m_nHeight;
	int     m_nSize;
	double  m_dblPixelXY;
	double  m_dblPixelZ;
	float   threshold;
};


//// #define NV(VR) ((VR <= theBoneDensitySwapData.m_nMinValue+50 ? 1 : 0))
// #define NV(VR) ((VR < 128.0 ? 1 : 0))
// #define NVP(VR, a, n) (NV(VR)*pow(double(a), double(n)))

class CCube  
{
public:
	CCube();
	virtual ~CCube();
	void			SetEdge(VERTEX3D*	pVertex);
	VERTEX3D*		GetCubeVertex();
	int				GetVertexIndex();
private:
	unsigned int	   m_nVertexIndex;
	unsigned int	   m_nEdge;
	VERTEX3D		   m_pVertex[8];
	Volume          m_Volum;

private:
	POINT3D         m_pEdge[12];
	POINT3D         m_Vertex[8];
	static int             faces[3840];
	vector<CTriangle>   Triangles;

public:
	void            setVolum(int** v,int w,int h,int d,double PxlXY,double PxlZ,float thre);
	void            init(int x, int y, int z);
	BOOL            computeEdge(POINT3D v1,int i1,POINT3D v2,int i2,POINT3D& result);
    void            computeEdges();
	int             intensity(POINT3D v);
	//获取表面三角形
	void            getTriangle();
	int             caseNumber(); 
	void            getTriangles();
	//计算体积、表面积
	double          getVolume();
	double          getSufaceArea();
	void            SubExper(double w0,double w1, double w2,double *fg);
	POINT3D         CrossProduct(POINT3D& point0,POINT3D& point1,POINT3D& point2);
	//膨胀
	void            Dilate(double r);
 };

// class CBasicCube  
// {
// public:
// 	CBasicCube();
// 	virtual ~CBasicCube();
// 	void			SetEdge(VERTEX3D*	pVertex);
// 	VERTEX3D*		GetCubeVertex();
// 	int				GetVertexIndex();
// private:
// 	unsigned int	m_nVertexIndex;
// 	unsigned int	m_nEdge;
// 	VERTEX3D		m_pVertex[8];
// };

#endif // !defined(AFX_CUBE_H__804013F6_6081_4316_9D20_00E042282694__INCLUDED_)
