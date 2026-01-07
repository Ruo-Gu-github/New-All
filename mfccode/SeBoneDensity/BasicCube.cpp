#include "StdAfx.h"
#include "BasicCube.h"


CBasicCube::CBasicCube()
{
	m_nEdge = 0x0;
}

CBasicCube::~CBasicCube()
{

}

void CBasicCube::SetEdge(VERTEX3D*	pVertex)
{

	int			nEdge = 0;
	for (int i = 0; i < 8; i ++)
	{
		nEdge += (unsigned int)(NVP(pVertex[i].pointVal, 2, i));

		m_pVertex[i] = pVertex[i];
	}
	m_nVertexIndex = nEdge;
}


VERTEX3D* CBasicCube::GetCubeVertex()
{
	return m_pVertex;
}

int CBasicCube::GetVertexIndex()
{
	return (int)m_nVertexIndex;
}
