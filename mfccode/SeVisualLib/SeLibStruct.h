#pragma once

enum{RD_NORMAL = 0, RD_TAILOR, RD_TEXTURE};

#define	 MT_APR		MT_Ext + 15
#define  MT_MPRNEW  MT_Ext + 16


struct Vector3D
{
	float x , y, z;
	Vector3D()
	{
		x = 0;
		y = 0;
		z = 0;
	}

	Vector3D(float x0,float y0, float z0)
	{
		x = x0;
		y = y0;
		z = z0;
	}

	Vector3D(double x0,double y0, double z0)
	{
		x = x0;
		y = y0;
		z = z0;
	}

	Vector3D& operator = ( Vector3D& p1 ) 
	{
		x = p1.x , y = p1.y , z = p1.z ;
		return *this;
	}

	BOOL operator == ( Vector3D& p ) 
	{
		return ( x == p.x && y == p.y && z == p.z );
	}

	void Set( double xx , double yy , double zz )
	{
		x = xx , y = yy , z = zz;
	}

	void CrossProduct( Vector3D p1,Vector3D p2)
	{
		x = (p1.y * p2.z) - (p2.y * p1.z);
		y = -1 * ((p1.x * p2.z) - (p2.x * p1.z));
		z = (p1.x * p2.y) - (p2.x * p1.y);	
	}

	void Normalization()
	{
		double t = sqrt( x*x + y*y + z*z);
		if (t != 0 )
		{
			x = x/t;
			y = y/t;
			z = z/t;
		}
		else
		{
			x = y = z = 0; 
		}

	}

	void SameDirection(Vector3D m_Direct)
	{
		if (m_Direct.z * z < 0)
		{
			x = -1 * x;
			y = -1 * y;
			z = -1 * z;
		}
		else if (m_Direct.z * z == 0)
		{
			if (m_Direct.y * y < 0)
			{
				x = -1 * x;
				y = -1 * y;
				z = -1 * z;
			}
			else if (m_Direct.y * y == 0)
			{
				if (m_Direct.x * x < 0)
				{
					x = -1 * x;
					y = -1 * y;
					z = -1 * z;
				}
			}
		}
	}
	double Vectorlength()
	{
		return sqrt( x*x + y*y + z*z );
	}

	double Point2PointDistance(Vector3D p1)
	{
		return sqrt((p1.x - x)*(p1.x - x) + (p1.y - y)*(p1.y - y) + (p1.z - z)*(p1.z -z));
	}

	int	Point2PlaneDistance(Vector3D disPoint , Vector3D NormalVector)
	{
		double d1 = Point2PointDistance(disPoint);
		if ( d1 == 0)
		{
			return 0;
		}

		Vector3D RadialVector(disPoint.x - x , disPoint.y - y , disPoint.z - z);
		double cosa = (RadialVector.x*NormalVector.x + RadialVector.y*NormalVector.y + RadialVector.z*NormalVector.z)/(NormalVector.Vectorlength()*RadialVector.Vectorlength());

		int d = d1*cosa;

		return d;
	}

};

struct InterpolationData
{
	int i;
	void* pthis;
};

struct MprRotateInfo 
{
	double				dWorldX_x;  //世界坐标下当前X轴
	double				dWorldX_y;
	double				dWorldX_z;
	double				dWorldY_x;  //世界坐标下当前Y轴
	double				dWorldY_y;
	double				dWorldY_z;
	double				dWorldZ_x;  //世界坐标下当前Z轴
	double				dWorldZ_y;
	double				dWorldZ_z;

	double				dModelView[16];

	double				dRotateWorldX[16];
	double				dRotateWorldY[16];
	double				dRotateWorldZ[16];

	double				dModelViewCurrentInvX[16];
	double				dModelViewCurrentInvY[16];
	double				dModelViewCurrentInvZ[16];
	MprRotateInfo()
	{
		dWorldX_x = 1.0;  //世界坐标下当前X轴
		dWorldX_y = 0.0;
		dWorldX_z = 0.0;
		dWorldY_x = 0.0;  //世界坐标下当前Y轴
		dWorldY_y = 1.0;
		dWorldY_z = 0.0;
		dWorldZ_x = 0.0;  //世界坐标下当前Z轴
		dWorldZ_y = 0.0;
		dWorldZ_z = 1.0;


		for (int i = 0; i < 16; i++)
		{
			dModelView[i] = 0.0;

			dRotateWorldX[i] = 0.0;
			dRotateWorldY[i] = 0.0;
			dRotateWorldZ[i] = 0.0;
			dModelViewCurrentInvX[i] = 0.0;
			dModelViewCurrentInvY[i] = 0.0;
			dModelViewCurrentInvZ[i] = 0.0;

		}
		dModelView[0] = dModelView[5] = dModelView[10] = dModelView[15] = 1.0;

		dRotateWorldX[0] = dRotateWorldX[5] = dRotateWorldX[10] = dRotateWorldX[15] = 1.0;
		dRotateWorldY[0] = dRotateWorldY[5] = dRotateWorldY[10] = dRotateWorldY[15] = 1.0;
		dRotateWorldZ[0] = dRotateWorldZ[5] = dRotateWorldZ[10] = dRotateWorldZ[15] = 1.0;
		dModelViewCurrentInvX[0] = dModelViewCurrentInvX[5] = dModelViewCurrentInvX[10] = dModelViewCurrentInvX[15] = 1.0;
		dModelViewCurrentInvY[0] = dModelViewCurrentInvY[5] = dModelViewCurrentInvY[10] = dModelViewCurrentInvY[15] = 1.0;
		dModelViewCurrentInvZ[0] = dModelViewCurrentInvZ[5] = dModelViewCurrentInvZ[10] = dModelViewCurrentInvZ[15] = 1.0;
	}

	MprRotateInfo& operator = ( MprRotateInfo& p1 ) 
	{
		dWorldX_x = p1.dWorldX_x;  //世界坐标下当前X轴
		dWorldX_y = p1.dWorldX_y;
		dWorldX_z = p1.dWorldX_z;
		dWorldY_x = p1.dWorldY_x;  //世界坐标下当前Y轴
		dWorldY_y = p1.dWorldY_y;
		dWorldY_z = p1.dWorldY_z;
		dWorldZ_x = p1.dWorldZ_x;  //世界坐标下当前Z轴
		dWorldZ_y = p1.dWorldZ_y;
		dWorldZ_z = p1.dWorldZ_z;


		for (int i = 0; i < 16; i++)
		{
			dModelView[i] = p1.dModelView[i];

			dRotateWorldX[i] = p1.dRotateWorldX[i];
			dRotateWorldY[i] = p1.dRotateWorldY[i];
			dRotateWorldZ[i] = p1.dRotateWorldZ[i];
			dModelViewCurrentInvX[i] = p1.dModelViewCurrentInvX[i];
			dModelViewCurrentInvY[i] = p1.dModelViewCurrentInvY[i];
			dModelViewCurrentInvZ[i] = p1.dModelViewCurrentInvZ[i];

		}	
		return *this;
	}

	void	Reset()
	{
		dWorldX_x = 1.0;  //世界坐标下当前X轴
		dWorldX_y = 0.0;
		dWorldX_z = 0.0;
		dWorldY_x = 0.0;  //世界坐标下当前Y轴
		dWorldY_y = 1.0;
		dWorldY_z = 0.0;
		dWorldZ_x = 0.0;  //世界坐标下当前Z轴
		dWorldZ_y = 0.0;
		dWorldZ_z = 1.0;


		for (int i = 0; i < 16; i++)
		{
			dModelView[i] = 0.0;

			dRotateWorldX[i] = 0.0;
			dRotateWorldY[i] = 0.0;
			dRotateWorldZ[i] = 0.0;
			dModelViewCurrentInvX[i] = 0.0;
			dModelViewCurrentInvY[i] = 0.0;
			dModelViewCurrentInvZ[i] = 0.0;

		}
		dModelView[0] = dModelView[5] = dModelView[10] = dModelView[15] = 1.0;

		dRotateWorldX[0] = dRotateWorldX[5] = dRotateWorldX[10] = dRotateWorldX[15] = 1.0;
		dRotateWorldY[0] = dRotateWorldY[5] = dRotateWorldY[10] = dRotateWorldY[15] = 1.0;
		dRotateWorldZ[0] = dRotateWorldZ[5] = dRotateWorldZ[10] = dRotateWorldZ[15] = 1.0;
		dModelViewCurrentInvX[0] = dModelViewCurrentInvX[5] = dModelViewCurrentInvX[10] = dModelViewCurrentInvX[15] = 1.0;
		dModelViewCurrentInvY[0] = dModelViewCurrentInvY[5] = dModelViewCurrentInvY[10] = dModelViewCurrentInvY[15] = 1.0;
		dModelViewCurrentInvZ[0] = dModelViewCurrentInvZ[5] = dModelViewCurrentInvZ[10] = dModelViewCurrentInvZ[15] = 1.0;
	}

	void	ResetAPR(int nMainPlane)
	{
		switch(nMainPlane)
		{
		case 1:
			{
				dWorldX_x = 1.0;  //世界坐标下当前X轴
				dWorldX_y = 0.0;
				dWorldX_z = 0.0;
				dWorldY_x = 0.0;  //世界坐标下当前Y轴
				dWorldY_y = 1.0;
				dWorldY_z = 0.0;
				dWorldZ_x = 0.0;  //世界坐标下当前Z轴
				dWorldZ_y = 0.0;
				dWorldZ_z = 1.0;


				for (int i = 0; i < 16; i++)
				{
					dModelView[i] = 0.0;

					dRotateWorldX[i] = 0.0;
					dRotateWorldY[i] = 0.0;
					dRotateWorldZ[i] = 0.0;
					dModelViewCurrentInvX[i] = 0.0;
					dModelViewCurrentInvY[i] = 0.0;
					dModelViewCurrentInvZ[i] = 0.0;

				}
				dModelView[0] = dModelView[5] = dModelView[10] = dModelView[15] = 1.0;

				dRotateWorldX[0] = dRotateWorldX[5] = dRotateWorldX[10] = dRotateWorldX[15] = 1.0;
				dRotateWorldY[0] = dRotateWorldY[5] = dRotateWorldY[10] = dRotateWorldY[15] = 1.0;
				dRotateWorldZ[0] = dRotateWorldZ[5] = dRotateWorldZ[10] = dRotateWorldZ[15] = 1.0;
				dModelViewCurrentInvX[0] = dModelViewCurrentInvX[5] = dModelViewCurrentInvX[10] = dModelViewCurrentInvX[15] = 1.0;
				dModelViewCurrentInvY[0] = dModelViewCurrentInvY[5] = dModelViewCurrentInvY[10] = dModelViewCurrentInvY[15] = 1.0;
				dModelViewCurrentInvZ[0] = dModelViewCurrentInvZ[5] = dModelViewCurrentInvZ[10] = dModelViewCurrentInvZ[15] = 1.0;
			}
			break;
		case 2:
			{
				dWorldX_x = 1.4805936071169177e-016;  
				dWorldX_y = -1.2167964413833943e-008;
				dWorldX_z = 1.0;
				dWorldY_x = 1.0;  
				dWorldY_y = 1.2167964413833943e-008;
				dWorldY_z = 0.0;
				dWorldZ_x = -1.2167964413833943e-008;  
				dWorldZ_y = 1.0;
				dWorldZ_z = 1.2167964413833943e-008;

				dModelView[3] = dModelView[7] = dModelView[9] = dModelView[11] = dModelView[12] = dModelView[13] = dModelView[14] = 0.0;
				dModelView[1] = dModelView[6] = dModelView[8] = dModelView[15] = 1.0;
				dModelView[0] = 1.4805936071169177e-016;
				dModelView[2] = dModelView[4] = -1.2167964413833943e-008;
				dModelView[5] = dModelView[10] = 1.2167964413833943e-008;

				dRotateWorldX[3] = dRotateWorldX[6] = dRotateWorldX[7] = dRotateWorldX[11] = dRotateWorldX[12] = dRotateWorldX[13] = 
					dRotateWorldX[14] = 0.0;
				dRotateWorldX[2] = dRotateWorldX[4] = dRotateWorldX[9] = dRotateWorldX[15] = 1.0;
				dRotateWorldX[0] = 1.4805936071169177e-016;
				dRotateWorldX[5] = dRotateWorldX[10] = 1.2167964413833943e-008;
				dRotateWorldX[1] = dRotateWorldX[8] = -1.2167964413833943e-008;

				dRotateWorldY[3] = dRotateWorldY[6] = dRotateWorldY[7] = dRotateWorldY[11] = dRotateWorldY[12] = dRotateWorldY[13] = 
					dRotateWorldY[14] = 0.0;
				dRotateWorldY[2] = dRotateWorldY[4] = dRotateWorldY[9] = dRotateWorldY[15] = 1.0;
				dRotateWorldY[0] = 1.4805936071169177e-016;
				dRotateWorldY[5] = dRotateWorldY[10] = 1.2167964413833943e-008;
				dRotateWorldY[1] = dRotateWorldY[8] = -1.2167964413833943e-008;

				dRotateWorldZ[3] = dRotateWorldZ[6] = dRotateWorldZ[7] = dRotateWorldZ[11] = dRotateWorldZ[12] = dRotateWorldZ[13] = 
					dRotateWorldZ[14] = 0.0;
				dRotateWorldZ[2] = dRotateWorldZ[4] = dRotateWorldZ[9] = dRotateWorldZ[15] = 1.0;
				dRotateWorldZ[0] = 1.4805936071169177e-016;
				dRotateWorldZ[5] = dRotateWorldZ[10] = 1.2167964413833943e-008;
				dRotateWorldZ[1] = dRotateWorldZ[8] = -1.2167964413833943e-008;

				dModelViewCurrentInvX[2] = dModelViewCurrentInvX[3] = dModelViewCurrentInvX[6] = dModelViewCurrentInvX[7] = dModelViewCurrentInvX[8] = 
					dModelViewCurrentInvX[9] = dModelViewCurrentInvX[11] = dModelViewCurrentInvX[12] = dModelViewCurrentInvX[13] = dModelViewCurrentInvX[14] = 0.0;
				dModelViewCurrentInvX[4] = dModelViewCurrentInvX[10] = dModelViewCurrentInvX[15] = 1.0;
				dModelViewCurrentInvX[1] = -1.0;
				dModelViewCurrentInvX[0] = dModelViewCurrentInvX[5] = 1.2167964413833943e-008;

				dModelViewCurrentInvY[2] = dModelViewCurrentInvY[3] = dModelViewCurrentInvY[6] = dModelViewCurrentInvY[7] = dModelViewCurrentInvY[8] = 
					dModelViewCurrentInvY[9] = dModelViewCurrentInvY[11] = dModelViewCurrentInvY[12] = dModelViewCurrentInvY[13] = dModelViewCurrentInvY[14] = 0.0;
				dModelViewCurrentInvY[4] = dModelViewCurrentInvY[10] = dModelViewCurrentInvY[15] = 1.0;
				dModelViewCurrentInvY[1] = -1.0;
				dModelViewCurrentInvY[0] = dModelViewCurrentInvY[5] = 1.2167964413833943e-008;

				dModelViewCurrentInvZ[2] = dModelViewCurrentInvZ[3] = dModelViewCurrentInvZ[6] = dModelViewCurrentInvZ[7] = dModelViewCurrentInvZ[8] = 
					dModelViewCurrentInvZ[9] = dModelViewCurrentInvZ[11] = dModelViewCurrentInvZ[12] = dModelViewCurrentInvZ[13] = dModelViewCurrentInvZ[14] = 0.0;
				dModelViewCurrentInvZ[4] = dModelViewCurrentInvZ[10] = dModelViewCurrentInvZ[15] = 1.0;
				dModelViewCurrentInvZ[1] = -1.0;
				dModelViewCurrentInvZ[0] = dModelViewCurrentInvZ[5] = 1.2167964413833943e-008;
			}
			break;
		case 3:
			{
				dWorldX_x = 1.0;  
				dWorldX_y = 0.0;
				dWorldX_z = 0.0;
				dWorldY_x = 0.0;  
				dWorldY_y = 1.2167964413833943e-008;
				dWorldY_z = -1.0;
				dWorldZ_x = 0.0;  
				dWorldZ_y = 1.0;
				dWorldZ_z = 1.2167964413833943e-008;

				dModelView[1] = dModelView[2] = dModelView[3] = dModelView[4] = dModelView[7] = dModelView[8] = 
					dModelView[11] = dModelView[12] = dModelView[13] = dModelView[14] = 0.0;
				dModelView[0] = dModelView[6] = dModelView[15] = 1.0;
				dModelView[9] = -1.0;
				dModelView[5] = dModelView[10] = 1.2167964413833943e-008;

				dRotateWorldX[1] = dRotateWorldX[2] = dRotateWorldX[3] = dRotateWorldX[4] = dRotateWorldX[7] = dRotateWorldX[8] = 
					dRotateWorldX[11] = dRotateWorldX[12] = dRotateWorldX[13] = dRotateWorldX[14] = 0.0;
				dRotateWorldX[0] = dRotateWorldX[9] = dRotateWorldX[15] = 1.0;
				dRotateWorldX[6] = -1.0;
				dRotateWorldX[5] = dRotateWorldX[10] = 1.2167964413833943e-008;

				dRotateWorldY[1] = dRotateWorldY[2] = dRotateWorldY[3] = dRotateWorldY[4] = dRotateWorldY[7] = dRotateWorldY[8] = 
					dRotateWorldY[11] = dRotateWorldY[12] = dRotateWorldY[13] = dRotateWorldY[14] = 0.0;
				dRotateWorldY[0] = dRotateWorldY[9] = dRotateWorldY[15] = 1.0;
				dRotateWorldY[6] = -1.0;
				dRotateWorldY[5] = dRotateWorldY[10] = 1.2167964413833943e-008;

				dRotateWorldZ[1] = dRotateWorldZ[2] = dRotateWorldZ[3] = dRotateWorldZ[4]= dRotateWorldZ[7] = dRotateWorldZ[8] = 
					dRotateWorldZ[11] = dRotateWorldZ[12] = dRotateWorldZ[13] = dRotateWorldZ[14] = 0.0;
				dRotateWorldZ[0] = dRotateWorldZ[9] = dRotateWorldZ[15] = 1.0;
				dRotateWorldZ[6] = -1.0;
				dRotateWorldZ[5] = dRotateWorldZ[10] = 1.2167964413833943e-008;
			}
			break;
		}
	}
};

struct V3dRotateInfo
{
	double				dModelView[16];
	double				dtranslateMatrix[16];
	double				dRotateMatrixX[16];
	double				dRotateMatrixY[16];
	double				dModelViewCurrentInv[16];
	double				dModelViewCurrentInvX[16];
	double				dModelViewCurrentInvY[16];
	double				dCurrentHoriX;
	double				dCurrentHoriY;
	double				dCurrentHoriZ;
	double				dCurrentVertiX;
	double				dCurrentVertiY;
	double				dCurrentVertiZ;
	V3dRotateInfo()
	{
		for (int i = 0; i < 16; i++)
		{
			dModelView[i] = 0.0;
			dtranslateMatrix[i] = 0.0;
			dRotateMatrixX[i] = 0.0;
			dRotateMatrixY[i] = 0.0;
			dModelViewCurrentInv[i] = 0.0;
			dModelViewCurrentInvX[i] = 0.0;
			dModelViewCurrentInvY[i] = 0.0;
		}
		dModelView[0] = dModelView[5] = dModelView[10] = dModelView[15] = 1.0;
		dtranslateMatrix[0] = dtranslateMatrix[5] = dtranslateMatrix[10] = dtranslateMatrix[15] = 1.0;
		dRotateMatrixX[0] = dRotateMatrixX[5] = dRotateMatrixX[10] = dRotateMatrixX[15] = 1.0;
		dRotateMatrixY[0] = dRotateMatrixY[5] = dRotateMatrixY[10] = dRotateMatrixY[15] = 1.0;
		dModelViewCurrentInv[0] = dModelViewCurrentInv[5] = dModelViewCurrentInv[10] = dModelViewCurrentInv[15] = 1.0;
		dModelViewCurrentInvX[0] = dModelViewCurrentInvX[5] = dModelViewCurrentInvX[10] = dModelViewCurrentInvX[15] = 1.0;
		dModelViewCurrentInvY[0] = dModelViewCurrentInvY[5] = dModelViewCurrentInvY[10] = dModelViewCurrentInvY[15] = 1.0;
		

		dCurrentHoriX = 1.0;
		dCurrentHoriY = 0.0;
		dCurrentHoriZ = 0.0;
		dCurrentVertiX = 0.0;
		dCurrentVertiY = 1.0;
		dCurrentVertiZ = 0.0;
	}

	V3dRotateInfo& operator = ( V3dRotateInfo& p1 ) 
	{
		dCurrentHoriX = p1.dCurrentHoriX;
		dCurrentHoriY = p1.dCurrentHoriY;
		dCurrentHoriZ = p1.dCurrentHoriZ;
		dCurrentVertiX = p1.dCurrentVertiX;
		dCurrentVertiY = p1.dCurrentVertiY;
		dCurrentVertiZ = p1.dCurrentVertiZ;
		for (int i = 0; i < 16; i++)
		{
			dModelView[i] = p1.dModelView[i];
			dRotateMatrixX[i] = p1.dRotateMatrixX[i];
			dRotateMatrixY[i] = p1.dRotateMatrixY[i];
			dModelViewCurrentInvX[i] = p1.dModelViewCurrentInvX[i];
			dModelViewCurrentInvY[i] = p1.dModelViewCurrentInvY[i];
		}
		return *this;
	}

	void	Reset()
	{
		for (int i = 0; i < 16; i++)
		{
			dModelView[i] = 0.0;
			dRotateMatrixX[i] = 0.0;
			dRotateMatrixY[i] = 0.0;
			dModelViewCurrentInvX[i] = 0.0;
			dModelViewCurrentInvY[i] = 0.0;
		}
		dModelView[0] = dModelView[5] = dModelView[10] = dModelView[15] = 1.0;
		dRotateMatrixX[0] = dRotateMatrixX[5] = dRotateMatrixX[10] = dRotateMatrixX[15] = 1.0;
		dRotateMatrixY[0] = dRotateMatrixY[5] = dRotateMatrixY[10] = dRotateMatrixY[15] = 1.0;
		dModelViewCurrentInvX[0] = dModelViewCurrentInvX[5] = dModelViewCurrentInvX[10] = dModelViewCurrentInvX[15] = 1.0;
		dModelViewCurrentInvY[0] = dModelViewCurrentInvY[5] = dModelViewCurrentInvY[10] = dModelViewCurrentInvY[15] = 1.0;

		dCurrentHoriX = 1.0;
		dCurrentHoriY = 0.0;
		dCurrentHoriZ = 0.0;
		dCurrentVertiX = 0.0;
		dCurrentVertiY = 1.0;
		dCurrentVertiZ = 0.0;
	}
};

struct ChipPoint
{
	float x;
	float y;
	float z;
};

struct flocal
{
	float localx;
	float localy;
};
