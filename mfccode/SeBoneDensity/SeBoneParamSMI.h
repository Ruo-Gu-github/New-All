#pragma once
struct CTriangle;

//typedef struct mVertex
//{   
//	double x;     
//	double y;
//	double z;
//	bool operator < ( const vertex &rhs) const 
//	{
//		if (x<rhs.x)
//		{
//			return true;
//		}
//		else if (x>rhs.x)
//		{
//			return false;
//		}
//		else if (y<rhs.y)
//		{
//			return true;
//		}
//		else if (y>rhs.y)
//		{
//			return false;
//		}
//		else if (z<rhs.z)
//		{
//			return true;
//		}
//		else if (z>rhs.z)
//		{
//			return false;
//		}
//		else
//		{
//			return false;
//		}
//	}
//}mVertex;

class SeBoneParamSMI
{
public:
	SeBoneParamSMI(void);
	SeBoneParamSMI(CDcmPicArray* p, int nMinValue, int nMin, int nMax, int nWidth, int nHeight, int nSize);
	virtual ~SeBoneParamSMI(void);
private:
	CDcmPicArray*        m_pProcessArray;
	int**               m_BinImg;
	int**               m_ReSImg;
 	int                 m_nMinValue;
	int                 m_nMin;
	int                 m_nMax;
	int                 m_nWidth;
	int                 m_nHeight;
	int                 m_nOriImagePiece;
	double        m_dMMperPixelXY;
	double        m_dMMperPixelZ;

	vector<CTriangle>   m_vTriangle;
private:
	int            m_nReWidth;
	int            m_nReHeight;
	int            m_nReSize;
	double         m_dblRePixelXY;
	double         m_dblRePixelZ;
private:
	void    DoBinaryzation();
	void    Resample(int factorX,int factorY,int factorZ);
	double    CalculateVolum();
	//void    subexpr(double w0,double w1, double w2,double *fg);
	//void    CrossProduct(CTriangle& tri);
public:
	double    CalculatSMI(int nReVolum);
};

