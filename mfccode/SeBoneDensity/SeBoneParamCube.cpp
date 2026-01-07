// Cube.cpp: implementation of the CCube class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SeBoneParamCube.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCube::CCube()
{
	m_nEdge = 0x0;
}

CCube::~CCube()
{
	
}
//
//void CCube::SetEdge(VERTEX3D*	pVertex)
//{
//
//	int			nEdge = 0;
//	for (int i = 0; i < 8; i ++)
//	{
//		nEdge += (unsigned int)(NVP(pVertex[i].pointVal, 2, i));
//
//		m_pVertex[i] = pVertex[i];
//	}
//	m_nVertexIndex = nEdge;
//}

void CCube::SetEdge(VERTEX3D*	pVertex)
{

	int			nEdge = 0;
	for (int i = 0; i < 8; i ++)
	{
		nEdge += pVertex[i].pointVal*pow((double)2,(double)i);

		m_pVertex[i] = pVertex[i];
	}
	m_nVertexIndex = nEdge;
}

//void CCube::SetEdge(VERTEX3D*	pVertex)
//{
//
//	int			nEdge = 0;
//	for (int i = 0; i < 8; i ++)
//	{
//		nEdge += NVP(pVertex[i].pointVal,2,i);
//
//		m_pVertex[i] = pVertex[i];
//	}
//	m_nVertexIndex = nEdge;
//}

VERTEX3D* CCube::GetCubeVertex()
{
	return m_pVertex;
}

int CCube::GetVertexIndex()
{
	return (int)m_nVertexIndex;
}



/***********创建体元********/
void CCube:: setVolum(int** v,int w,int h,int d,double PxlXY,double PxlZ,float thre)
{
	m_Volum.m_volume = v;
	m_Volum.m_nWidth = w;
	m_Volum.m_nHeight = h;
	m_Volum.m_nSize = d;
	m_Volum.m_dblPixelXY = PxlXY;
	m_Volum.m_dblPixelZ = PxlZ;
	m_Volum.threshold = thre+0.5f;
}

void CCube::init(int x, int y, int z)
{
	m_Vertex[0] = POINT3D((double)x,(double)y,(double)z);
	m_Vertex[1] = POINT3D((double)(x+1),(double)y,(double)z);
	m_Vertex[2] = POINT3D((double)(x+1),(double)(y+1),(double)z);
	m_Vertex[3] = POINT3D((double)x,(double)(y+1),(double)z);
	m_Vertex[4] = POINT3D((double)x,(double)y,(double)(z+1));
	m_Vertex[5] = POINT3D((double)(x+1),(double)y,(double)(z+1));
	m_Vertex[6] = POINT3D((double)(x+1),(double)(y+1),(double)(z+1));
	m_Vertex[7] = POINT3D((double)x,(double)(y+1),(double)(z+1));
}

BOOL CCube::computeEdge(POINT3D v1,int i1,POINT3D v2,int i2,POINT3D& result)
{
	if(i2<i1)
		return computeEdge(v2,i2,v1,i1,result);
	float t = (m_Volum.threshold-i1)/(float)(i2-i1);
	if(t>=0&&t<=1)
	{
		result.fx = (v2.fx-v1.fx)*t+v1.fx;
		result.fy = (v2.fy-v1.fy)*t+v1.fy;
		result.fz = (v2.fz-v1.fz)*t+v1.fz;
		return TRUE;
	}
	result.fx = -1;
	result.fy = -1;
	result.fz = -1;

	return FALSE;
}

int CCube::intensity(POINT3D v)
{
	int w = m_Volum.m_nWidth;
	int h = m_Volum.m_nHeight;
	int d = m_Volum.m_nSize;

	int x = (int)v.fx;
	int y = (int)v.fy;
	int z = (int)v.fz;

	if(v.fx<0 || v.fy<0 || v.fz<0
		|| v.fx >= w || v.fy>=h || v.fz>=d)
	{
		return 0;
	}
	return m_Volum.m_volume[z][x+w*y];
}

void  CCube::computeEdges()
{
	int i0 = intensity(m_Vertex[0]);
	int i1 = intensity(m_Vertex[1]);
	int i2 = intensity(m_Vertex[2]);
	int i3 = intensity(m_Vertex[3]);
	int i4 = intensity(m_Vertex[4]);
	int i5 = intensity(m_Vertex[5]);
	int i6 = intensity(m_Vertex[6]);
	int i7 = intensity(m_Vertex[7]);

	computeEdge(m_Vertex[0],i0,m_Vertex[1],i1,m_pEdge[0]);
	computeEdge(m_Vertex[1],i1,m_Vertex[2],i2,m_pEdge[1]);
	computeEdge(m_Vertex[2],i2,m_Vertex[3],i3,m_pEdge[2]);
	computeEdge(m_Vertex[3],i3,m_Vertex[0],i0,m_pEdge[3]);

	computeEdge(m_Vertex[4],i4,m_Vertex[5],i5,m_pEdge[4]);
	computeEdge(m_Vertex[5],i5,m_Vertex[6],i6,m_pEdge[5]);
	computeEdge(m_Vertex[6],i6,m_Vertex[7],i7,m_pEdge[6]);
	computeEdge(m_Vertex[7],i7,m_Vertex[4],i4,m_pEdge[7]);

	computeEdge(m_Vertex[0],i0,m_Vertex[4],i4,m_pEdge[8]);
	computeEdge(m_Vertex[1],i1,m_Vertex[5],i5,m_pEdge[9]);
	computeEdge(m_Vertex[3],i3,m_Vertex[7],i7,m_pEdge[10]);
	computeEdge(m_Vertex[2],i2,m_Vertex[6],i6,m_pEdge[11]);
}

/*******获取表面三角形*******/
int CCube::caseNumber()
{
	int caseNumber = 0;
	for(int index = -1;++index<8;
		caseNumber += (intensity(m_Vertex[index])-m_Volum.threshold>0)
		? 1<<index : 0);
	return caseNumber;
}

void CCube::getTriangle()
{
	int cn = caseNumber();

	int offset = cn*15;
	for (int index=0;index<5;index++)
	{
		if(faces[offset] != -1){
			CTriangle   tri;
			tri.m_point[0] = m_pEdge[faces[offset+0]];
			tri.m_point[1] = m_pEdge[faces[offset+1]];
			tri.m_point[2] = m_pEdge[faces[offset+2]];

			Triangles.push_back(tri);
		}
		offset += 3;
	}
}

void  CCube::getTriangles()
{
	for(int z=-1;z<m_Volum.m_nSize+1;z+=1){
		for(int x=-1;x<m_Volum.m_nWidth+1;x+=1){
			for(int y=-1;y<m_Volum.m_nHeight+1;y+=1)
			{
				init(x,y,z);
				computeEdges();
				getTriangle();
			}
		}
	}

	for(int i=0;i<Triangles.size();i++)
	{
		for(int j=0;j<3;j++)
		{
			Triangles[i].m_point[j].fx = Triangles[i].m_point[j].fx*m_Volum.m_dblPixelXY;
			Triangles[i].m_point[j].fy = Triangles[i].m_point[j].fy*m_Volum.m_dblPixelXY;
			Triangles[i].m_point[j].fz = Triangles[i].m_point[j].fz*m_Volum.m_dblPixelZ;
		}
	}

}

/*********计算体积与表面积*******/
double CCube::getVolume()
{
	int ntmax = (int)Triangles.size();

	double mult[10] = {(double)(1.0/6),(double)(1.0/24),(double)(1.0/24),(double)(1.0/24),(double)(1.0/60),(double)(1.0/60),
		(double)(1.0/60),(double)(1.0/120),(double)(1.0/120),(double)(1.0/120)};

	double intg[10] = {0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};

	double* fg = new double[6];

	for(int t=0; t<ntmax; t++)
	{
		double x0 = Triangles[t].m_point[0].fx;
		double y0 = Triangles[t].m_point[0].fy;
		double z0 = Triangles[t].m_point[0].fz;

		double x1 = Triangles[t].m_point[1].fx;
		double y1 = Triangles[t].m_point[1].fy;
		double z1 = Triangles[t].m_point[1].fz;

		double x2 = Triangles[t].m_point[2].fx;
		double y2 = Triangles[t].m_point[2].fy;
		double z2 = Triangles[t].m_point[2].fz;

		// get edges and cross product of edges
		double a1 = x1 - x0, b1 = y1 - y0, c1 = z1 - z0;
		double a2 = x2 - x0, b2 = y2 - y0, c2 = z2 - z0;
		double d0 = b1 * c2 - b2 * c1;
		double d1 = a2 * c1 - a1 * c2;
		double d2 = a1 * b2 - a2 * b1;

		// compute integral terms
		SubExper(x0, x1, x2, fg);	
		double f1x = fg[0], f2x = fg[1], f3x = fg[2];
		double g0x = fg[3], g1x = fg[4], g2x = fg[5];
		SubExper(y0, y1, y2, fg);
		double f1y = fg[0], f2y = fg[1], f3y = fg[2];
		double g0y = fg[3], g1y = fg[4], g2y = fg[5];
		SubExper(z0, z1, z2, fg);
		double f1z = fg[0], f2z = fg[1], f3z = fg[2];
		double g0z = fg[3], g1z = fg[4], g2z = fg[5];

		// update integrals
		intg[0] += d0 * f1x;
		intg[1] += d0 * f2x;
		intg[2] += d1 * f2y;
		intg[3] += d2 * f2z;
		intg[4] += d0 * f3x;
		intg[5] += d1 * f3y;
		intg[6] += d2 * f3z;
		intg[7] += d0*(y0 * g0x + y1 * g1x + y2 * g2x);
		intg[8] += d1*(z0 * g0y + z1 * g1y + z2 * g2y);
		intg[9] += d2*(x0 * g0z + x1 * g1z + x2 * g2z);
	}

	double mass = intg[0] / mult[0];
	
	for (int i = 0;i<10;i++)
	{
		intg[i] *= mult[i];
	}

	if(fg != NULL)
	{
		delete[] fg;
		fg =NULL;
	}
	return mass;
}

double CCube::getSufaceArea()
{
	double   sumArea = 0;
	int  nTris = (int)Triangles.size();

	for (int n=0;n<nTris;n++)
	{
		POINT3D cp = CrossProduct(Triangles[n].m_point[0],Triangles[n].m_point[1],Triangles[n].m_point[2]);
		double  deltaArea = sqrt(cp.fx*cp.fx+cp.fy*cp.fy+cp.fz*cp.fz)/(double)2;
	
		sumArea += deltaArea;
	}
	return sumArea;
}

POINT3D CCube::CrossProduct(POINT3D& point0,POINT3D& point1,POINT3D& point2)
{
	double x1 = point1.fx-point0.fx;
	double y1 = point1.fy-point0.fy;
	double z1 = point1.fz-point0.fz;

	double x2 = point2.fx-point0.fx;
	double y2 = point2.fy-point0.fy;
	double z2 = point2.fz-point0.fz;

	POINT3D crossVector(0,0,0);

	crossVector.fx = (float)(y1 * z2 - z1 * y2);
	crossVector.fy = (float)(z1 * x2 - x1 * z2);
	crossVector.fz = (float)(x1 * y2 - y1 * x2);

	return crossVector;
}

void CCube::SubExper(double w0,double w1, double w2,double *fg)
{
	double temp0 = w0 + w1;
	fg[0] = temp0 + w2;
	double temp1 = w0 * w0;
	double temp2 = temp1 + w1 * temp0;
	fg[1] = temp2 + w2 * fg[0];
	fg[2] = w0 * temp1 + w1 * temp2 + w2 * fg[1];
	fg[3] = fg[1] + w0 * (fg[0] + w0);
	fg[4] = fg[1] + w1 * (fg[0] + w1);
	fg[5] = fg[1] + w2 * (fg[0] + w2);
}

void CCube::Dilate(double r)
{
	/////给三角形分类
	map<POINT3D,vector<int>>  vertexHash;
	map<POINT3D, vector<int>>::iterator it;
	int nPoints = (int)Triangles.size();

	for(int p=0;p<nPoints;p++)
	{
		for(int j=0;j<3;j++)
		{
			POINT3D testPoint;

			testPoint.fx = Triangles[p].m_point[j].fx;
			testPoint.fy = Triangles[p].m_point[j].fy;
			testPoint.fz = Triangles[p].m_point[j].fz;

			it = vertexHash.find(testPoint);

			if (it == vertexHash.end())
			{
				vector<int>  points;
				points.push_back(p);
				vertexHash.insert(pair<POINT3D,vector<int>>(testPoint,points));
			}
			else
			{
				it->second.push_back(p);
				vertexHash.insert(pair<POINT3D,vector<int>>(testPoint,it->second));
			}
		}
	}

	//////////////寻找平均法向量
	map<POINT3D,POINT3D>  normalsHash;
	POINT3D vert;
	
	it = vertexHash.begin();
	while(it != vertexHash.end())
	{
		vert = it->first;
		POINT3D sumNormals(0.0,0.0,0.0);
		int vT = (int)it->second.size();
		int corner;

		for(int i=0; i<vT; i++)
		{
			int pointIndex = it->second[i];
			
			if ((Triangles[pointIndex].m_point[0].fx == it->first.fx)&&(Triangles[pointIndex].m_point[0].fy == it->first.fy)&&(Triangles[pointIndex].m_point[0].fz == it->first.fz))
			{
				corner = 0;
			}
			else if ((Triangles[pointIndex].m_point[1].fx == it->first.fx)&&(Triangles[pointIndex].m_point[1].fy == it->first.fy)&&(Triangles[pointIndex].m_point[1].fz == it->first.fz))
			{
				corner = 1;
			}
			else if ((Triangles[pointIndex].m_point[2].fx == it->first.fx)&&(Triangles[pointIndex].m_point[2].fy == it->first.fy)&&(Triangles[pointIndex].m_point[2].fz == it->first.fz))
			{
				corner = 2;
			}

			POINT3D point0,point1,point2;
			switch(corner){
			case 0:
				point0 = Triangles[pointIndex].m_point[0];
				point1 = Triangles[pointIndex].m_point[2];
				point2 = Triangles[pointIndex].m_point[1];
				break;
			case 1:
				point0 = Triangles[pointIndex].m_point[2];
				point1 = Triangles[pointIndex].m_point[1];
				point2 = Triangles[pointIndex].m_point[0];
				break;
			case 2:
				point0 = Triangles[pointIndex].m_point[1];
				point1 = Triangles[pointIndex].m_point[0];
				point2 = Triangles[pointIndex].m_point[2];
				break;

			}
			POINT3D surfaceNormal = CrossProduct(point0,point1,point2);
			sumNormals.fx += surfaceNormal.fx;
			sumNormals.fy += surfaceNormal.fy;
			sumNormals.fz += surfaceNormal.fz;
		}

		POINT3D normal ;
		normal.fx = sumNormals.fx/vT;
		normal.fy = sumNormals.fy/vT;
		normal.fz = sumNormals.fz/vT;

		double length = sqrt(normal.fx*normal.fx+normal.fy*normal.fy+normal.fz*normal.fz);
		normal.fx /= length;
		normal.fy /= length;
		normal.fz /= length;

		normalsHash.insert(pair<POINT3D,POINT3D>(vert,normal));
		it++;
	}

	/////////膨胀
	map<POINT3D, POINT3D>::iterator it_s;

	for(int t=0; t<nPoints;t++){

		for(int j=0;j<3;j++)
		{
			POINT3D point;
			point.fx = Triangles[t].m_point[j].fx;
			point.fy = Triangles[t].m_point[j].fy;
			point.fz = Triangles[t].m_point[j].fz;

			it_s = normalsHash.find(point);
			POINT3D normal = it_s->second;

			point.fx += normal.fx*r;
			point.fy += normal.fy*r;
			point.fz += normal.fz*r;
			Triangles[t].m_point[j].fx = point.fx;
			Triangles[t].m_point[j].fy = point.fy;
			Triangles[t].m_point[j].fz = point.fz;
		}
	}
}

 int CCube::faces[3840] =
{
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	1, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	0, 8, 3, 1, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	9, 2, 11, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	2, 8, 3, 2, 11, 8, 11, 9, 8, -1, -1, -1, -1, -1, -1,
	3, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	0, 10, 2, 8, 10, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	1, 9, 0, 2, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	1, 10, 2, 1, 9, 10, 9, 8, 10, -1, -1, -1, -1, -1, -1,
	3, 11, 1, 10, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	0, 11, 1, 0, 8, 11, 8, 10, 11, -1, -1, -1, -1, -1, -1,
	3, 9, 0, 3, 10, 9, 10, 11, 9, -1, -1, -1, -1, -1, -1,
	9, 8, 11, 11, 8, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1,
	1, 2, 11, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	3, 4, 7, 3, 0, 4, 1, 2, 11, -1, -1, -1, -1, -1, -1,
	9, 2, 11, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1,
	2, 11, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1,
	8, 4, 7, 3, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	10, 4, 7, 10, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1,
	9, 0, 1, 8, 4, 7, 2, 3, 10, -1, -1, -1, -1, -1, -1,
	4, 7, 10, 9, 4, 10, 9, 10, 2, 9, 2, 1, -1, -1, -1,
	3, 11, 1, 3, 10, 11, 7, 8, 4, -1, -1, -1, -1, -1, -1,
	1, 10, 11, 1, 4, 10, 1, 0, 4, 7, 10, 4, -1, -1, -1,
	4, 7, 8, 9, 0, 10, 9, 10, 11, 10, 0, 3, -1, -1, -1,
	4, 7, 10, 4, 10, 9, 9, 10, 11, -1, -1, -1, -1, -1, -1,
	9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1,
	1, 2, 11, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	3, 0, 8, 1, 2, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1,
	5, 2, 11, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1,
	2, 11, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1,
	9, 5, 4, 2, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	0, 10, 2, 0, 8, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1,
	0, 5, 4, 0, 1, 5, 2, 3, 10, -1, -1, -1, -1, -1, -1,
	2, 1, 5, 2, 5, 8, 2, 8, 10, 4, 8, 5, -1, -1, -1,
	11, 3, 10, 11, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1,
	4, 9, 5, 0, 8, 1, 8, 11, 1, 8, 10, 11, -1, -1, -1,
	5, 4, 0, 5, 0, 10, 5, 10, 11, 10, 0, 3, -1, -1, -1,
	5, 4, 8, 5, 8, 11, 11, 8, 10, -1, -1, -1, -1, -1, -1,
	9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1,
	0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1,
	1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	9, 7, 8, 9, 5, 7, 11, 1, 2, -1, -1, -1, -1, -1, -1,
	11, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1,
	8, 0, 2, 8, 2, 5, 8, 5, 7, 11, 5, 2, -1, -1, -1,
	2, 11, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1,
	7, 9, 5, 7, 8, 9, 3, 10, 2, -1, -1, -1, -1, -1, -1,
	9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 10, -1, -1, -1,
	2, 3, 10, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1,
	10, 2, 1, 10, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1,
	9, 5, 8, 8, 5, 7, 11, 1, 3, 11, 3, 10, -1, -1, -1,
	5, 7, 0, 5, 0, 9, 7, 10, 0, 1, 0, 11, 10, 11, 0,
	10, 11, 0, 10, 0, 3, 11, 5, 0, 8, 0, 7, 5, 7, 0,
	10, 11, 5, 7, 10, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	11, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	0, 8, 3, 5, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	9, 0, 1, 5, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	1, 8, 3, 1, 9, 8, 5, 11, 6, -1, -1, -1, -1, -1, -1,
	1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1,
	9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1,
	5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1,
	2, 3, 10, 11, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	10, 0, 8, 10, 2, 0, 11, 6, 5, -1, -1, -1, -1, -1, -1,
	0, 1, 9, 2, 3, 10, 5, 11, 6, -1, -1, -1, -1, -1, -1,
	5, 11, 6, 1, 9, 2, 9, 10, 2, 9, 8, 10, -1, -1, -1,
	6, 3, 10, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1,
	0, 8, 10, 0, 10, 5, 0, 5, 1, 5, 10, 6, -1, -1, -1,
	3, 10, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1,
	6, 5, 9, 6, 9, 10, 10, 9, 8, -1, -1, -1, -1, -1, -1,
	5, 11, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	4, 3, 0, 4, 7, 3, 6, 5, 11, -1, -1, -1, -1, -1, -1,
	1, 9, 0, 5, 11, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1,
	11, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1,
	6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1,
	1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1,
	8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1,
	7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9,
	3, 10, 2, 7, 8, 4, 11, 6, 5, -1, -1, -1, -1, -1, -1,
	5, 11, 6, 4, 7, 2, 4, 2, 0, 2, 7, 10, -1, -1, -1,
	0, 1, 9, 4, 7, 8, 2, 3, 10, 5, 11, 6, -1, -1, -1,
	9, 2, 1, 9, 10, 2, 9, 4, 10, 7, 10, 4, 5, 11, 6,
	8, 4, 7, 3, 10, 5, 3, 5, 1, 5, 10, 6, -1, -1, -1,
	5, 1, 10, 5, 10, 6, 1, 0, 10, 7, 10, 4, 0, 4, 10,
	0, 5, 9, 0, 6, 5, 0, 3, 6, 10, 6, 3, 8, 4, 7,
	6, 5, 9, 6, 9, 10, 4, 7, 9, 7, 10, 9, -1, -1, -1,
	11, 4, 9, 6, 4, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	4, 11, 6, 4, 9, 11, 0, 8, 3, -1, -1, -1, -1, -1, -1,
	11, 0, 1, 11, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1,
	8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 11, -1, -1, -1,
	1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1,
	3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1,
	0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1,
	11, 4, 9, 11, 6, 4, 10, 2, 3, -1, -1, -1, -1, -1, -1,
	0, 8, 2, 2, 8, 10, 4, 9, 11, 4, 11, 6, -1, -1, -1,
	3, 10, 2, 0, 1, 6, 0, 6, 4, 6, 1, 11, -1, -1, -1,
	6, 4, 1, 6, 1, 11, 4, 8, 1, 2, 1, 10, 8, 10, 1,
	9, 6, 4, 9, 3, 6, 9, 1, 3, 10, 6, 3, -1, -1, -1,
	8, 10, 1, 8, 1, 0, 10, 6, 1, 9, 1, 4, 6, 4, 1,
	3, 10, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1,
	6, 4, 8, 10, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	7, 11, 6, 7, 8, 11, 8, 9, 11, -1, -1, -1, -1, -1, -1,
	0, 7, 3, 0, 11, 7, 0, 9, 11, 6, 7, 11, -1, -1, -1,
	11, 6, 7, 1, 11, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1,
	11, 6, 7, 11, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1,
	1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1,
	2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9,
	7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1,
	7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	2, 3, 10, 11, 6, 8, 11, 8, 9, 8, 6, 7, -1, -1, -1,
	2, 0, 7, 2, 7, 10, 0, 9, 7, 6, 7, 11, 9, 11, 7,
	1, 8, 0, 1, 7, 8, 1, 11, 7, 6, 7, 11, 2, 3, 10,
	10, 2, 1, 10, 1, 7, 11, 6, 1, 6, 7, 1, -1, -1, -1,
	8, 9, 6, 8, 6, 7, 9, 1, 6, 10, 6, 3, 1, 3, 6,
	0, 9, 1, 10, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	7, 8, 0, 7, 0, 6, 3, 10, 0, 10, 6, 0, -1, -1, -1,
	7, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	7, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	3, 0, 8, 10, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	0, 1, 9, 10, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	8, 1, 9, 8, 3, 1, 10, 7, 6, -1, -1, -1, -1, -1, -1,
	11, 1, 2, 6, 10, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	1, 2, 11, 3, 0, 8, 6, 10, 7, -1, -1, -1, -1, -1, -1,
	2, 9, 0, 2, 11, 9, 6, 10, 7, -1, -1, -1, -1, -1, -1,
	6, 10, 7, 2, 11, 3, 11, 8, 3, 11, 9, 8, -1, -1, -1,
	7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1,
	2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1,
	1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1,
	11, 7, 6, 11, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1,
	11, 7, 6, 1, 7, 11, 1, 8, 7, 1, 0, 8, -1, -1, -1,
	0, 3, 7, 0, 7, 11, 0, 11, 9, 6, 11, 7, -1, -1, -1,
	7, 6, 11, 7, 11, 8, 8, 11, 9, -1, -1, -1, -1, -1, -1,
	6, 8, 4, 10, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	3, 6, 10, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1,
	8, 6, 10, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1,
	9, 4, 6, 9, 6, 3, 9, 3, 1, 10, 3, 6, -1, -1, -1,
	6, 8, 4, 6, 10, 8, 2, 11, 1, -1, -1, -1, -1, -1, -1,
	1, 2, 11, 3, 0, 10, 0, 6, 10, 0, 4, 6, -1, -1, -1,
	4, 10, 8, 4, 6, 10, 0, 2, 9, 2, 11, 9, -1, -1, -1,
	11, 9, 3, 11, 3, 2, 9, 4, 3, 10, 3, 6, 4, 6, 3,
	8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1,
	0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1,
	1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1,
	8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 11, 1, -1, -1, -1,
	11, 1, 0, 11, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1,
	4, 6, 3, 4, 3, 8, 6, 11, 3, 0, 3, 9, 11, 9, 3,
	11, 9, 4, 6, 11, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	4, 9, 5, 7, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	0, 8, 3, 4, 9, 5, 10, 7, 6, -1, -1, -1, -1, -1, -1,
	5, 0, 1, 5, 4, 0, 7, 6, 10, -1, -1, -1, -1, -1, -1,
	10, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1,
	9, 5, 4, 11, 1, 2, 7, 6, 10, -1, -1, -1, -1, -1, -1,
	6, 10, 7, 1, 2, 11, 0, 8, 3, 4, 9, 5, -1, -1, -1,
	7, 6, 10, 5, 4, 11, 4, 2, 11, 4, 0, 2, -1, -1, -1,
	3, 4, 8, 3, 5, 4, 3, 2, 5, 11, 5, 2, 10, 7, 6,
	7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1,
	9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1,
	3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1,
	6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8,
	9, 5, 4, 11, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1,
	1, 6, 11, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4,
	4, 0, 11, 4, 11, 5, 0, 3, 11, 6, 11, 7, 3, 7, 11,
	7, 6, 11, 7, 11, 8, 5, 4, 11, 4, 8, 11, -1, -1, -1,
	6, 9, 5, 6, 10, 9, 10, 8, 9, -1, -1, -1, -1, -1, -1,
	3, 6, 10, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1,
	0, 10, 8, 0, 5, 10, 0, 1, 5, 5, 6, 10, -1, -1, -1,
	6, 10, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1,
	1, 2, 11, 9, 5, 10, 9, 10, 8, 10, 5, 6, -1, -1, -1,
	0, 10, 3, 0, 6, 10, 0, 9, 6, 5, 6, 9, 1, 2, 11,
	10, 8, 5, 10, 5, 6, 8, 0, 5, 11, 5, 2, 0, 2, 5,
	6, 10, 3, 6, 3, 5, 2, 11, 3, 11, 5, 3, -1, -1, -1,
	5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1,
	9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1,
	1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8,
	1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	1, 3, 6, 1, 6, 11, 3, 8, 6, 5, 6, 9, 8, 9, 6,
	11, 1, 0, 11, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1,
	0, 3, 8, 5, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	11, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	10, 5, 11, 7, 5, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	10, 5, 11, 10, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1,
	5, 10, 7, 5, 11, 10, 1, 9, 0, -1, -1, -1, -1, -1, -1,
	11, 7, 5, 11, 10, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1,
	10, 1, 2, 10, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1,
	0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 10, -1, -1, -1,
	9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 10, 7, -1, -1, -1,
	7, 5, 2, 7, 2, 10, 5, 9, 2, 3, 2, 8, 9, 8, 2,
	2, 5, 11, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1,
	8, 2, 0, 8, 5, 2, 8, 7, 5, 11, 2, 5, -1, -1, -1,
	9, 0, 1, 5, 11, 3, 5, 3, 7, 3, 11, 2, -1, -1, -1,
	9, 8, 2, 9, 2, 1, 8, 7, 2, 11, 2, 5, 7, 5, 2,
	1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1,
	9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1,
	9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	5, 8, 4, 5, 11, 8, 11, 10, 8, -1, -1, -1, -1, -1, -1,
	5, 0, 4, 5, 10, 0, 5, 11, 10, 10, 3, 0, -1, -1, -1,
	0, 1, 9, 8, 4, 11, 8, 11, 10, 11, 4, 5, -1, -1, -1,
	11, 10, 4, 11, 4, 5, 10, 3, 4, 9, 4, 1, 3, 1, 4,
	2, 5, 1, 2, 8, 5, 2, 10, 8, 4, 5, 8, -1, -1, -1,
	0, 4, 10, 0, 10, 3, 4, 5, 10, 2, 10, 1, 5, 1, 10,
	0, 2, 5, 0, 5, 9, 2, 10, 5, 4, 5, 8, 10, 8, 5,
	9, 4, 5, 2, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	2, 5, 11, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1,
	5, 11, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1,
	3, 11, 2, 3, 5, 11, 3, 8, 5, 4, 5, 8, 0, 1, 9,
	5, 11, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1,
	8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1,
	0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1,
	9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	4, 10, 7, 4, 9, 10, 9, 11, 10, -1, -1, -1, -1, -1, -1,
	0, 8, 3, 4, 9, 7, 9, 10, 7, 9, 11, 10, -1, -1, -1,
	1, 11, 10, 1, 10, 4, 1, 4, 0, 7, 4, 10, -1, -1, -1,
	3, 1, 4, 3, 4, 8, 1, 11, 4, 7, 4, 10, 11, 10, 4,
	4, 10, 7, 9, 10, 4, 9, 2, 10, 9, 1, 2, -1, -1, -1,
	9, 7, 4, 9, 10, 7, 9, 1, 10, 2, 10, 1, 0, 8, 3,
	10, 7, 4, 10, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1,
	10, 7, 4, 10, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1,
	2, 9, 11, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1,
	9, 11, 7, 9, 7, 4, 11, 2, 7, 8, 7, 0, 2, 0, 7,
	3, 7, 11, 3, 11, 2, 7, 4, 11, 1, 11, 0, 4, 0, 11,
	1, 11, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1,
	4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1,
	4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	9, 11, 8, 11, 10, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	3, 0, 9, 3, 9, 10, 10, 9, 11, -1, -1, -1, -1, -1, -1,
	0, 1, 11, 0, 11, 8, 8, 11, 10, -1, -1, -1, -1, -1, -1,
	3, 1, 11, 10, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	1, 2, 10, 1, 10, 9, 9, 10, 8, -1, -1, -1, -1, -1, -1,
	3, 0, 9, 3, 9, 10, 1, 2, 9, 2, 10, 9, -1, -1, -1,
	0, 2, 10, 8, 0, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	3, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	2, 3, 8, 2, 8, 11, 11, 8, 9, -1, -1, -1, -1, -1, -1,
	9, 11, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	2, 3, 8, 2, 8, 11, 0, 1, 8, 1, 11, 8, -1, -1, -1,
	1, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};


// CBasicCube::CBasicCube()
// {
// 	m_nEdge = 0x0;
// }
// 
// CBasicCube::~CBasicCube()
// {
// 
// }
// 
// void CBasicCube::SetEdge(VERTEX3D*	pVertex)
// {
// 
// 	int			nEdge = 0;
// 	for (int i = 0; i < 8; i ++)
// 	{
// 		nEdge += (unsigned int)(NVP(pVertex[i].pointVal, 2, i));
// 
// 		m_pVertex[i] = pVertex[i];
// 	}
// 	m_nVertexIndex = nEdge;
// }
// 
// 
// VERTEX3D* CBasicCube::GetCubeVertex()
// {
// 	return m_pVertex;
// }
// 
// int CBasicCube::GetVertexIndex()
// {
// 	return (int)m_nVertexIndex;
// }