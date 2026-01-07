#pragma once

// ruogu redefine 0
typedef struct SPOINT3D 
{
	SPOINT3D(){}
	SPOINT3D(double x, double y, double z)
	{
		fx = x;
		fy = y;
		fz = z;
	}

	double	fx;
	double	fy;
	double	fz;

	bool operator < ( const SPOINT3D &rhs) const 
	{
		if (fx<rhs.fx)
		{
			return true;
		}
		else if (fx>rhs.fx)
		{
			return false;
		}
		else if (fy<rhs.fy)
		{
			return true;
		}
		else if (fy>rhs.fy)
		{
			return false;
		}
		else if (fz<rhs.fz)
		{
			return true;
		}
		else if (fz>rhs.fz)
		{
			return false;
		}
		else
		{
			return false;
		}
	}
}POINT3D;

struct VERTEX3D
{
	POINT3D		point;
	int			pointVal;
};

#define NV(VR) ((VR < 128.0 ? 1 : 0))
#define NVP(VR, a, n) (VR*pow(double(a), double(n)))

class CBasicCube  
{
public:
	CBasicCube();
	virtual ~CBasicCube();
	void			SetEdge(VERTEX3D*	pVertex);
	VERTEX3D*		GetCubeVertex();
	int				GetVertexIndex();
private:
	unsigned int	m_nVertexIndex;
	unsigned int	m_nEdge;
	VERTEX3D		m_pVertex[8];
};
