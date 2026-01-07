#pragma once
struct CTriangle;

typedef struct vertex
{   
	double x;     
	double y;
	double z;
	bool operator < ( const vertex &rhs) const 
	{
		if (x<rhs.x)
		{
			return true;
		}
		else if (x>rhs.x)
		{
			return false;
		}
		else if (y<rhs.y)
		{
			return true;
		}
		else if (y>rhs.y)
		{
			return false;
		}
		else if (z<rhs.z)
		{
			return true;
		}
		else if (z>rhs.z)
		{
			return false;
		}
		else
		{
			return false;
		}
	}
}Vertex;

struct SingleParam
{
	CString strDescription;
	CString strDescription2;
	CString strAbbreviation;
	double  dValue;
	CString strUint;
	SingleParam(CString cs1, CString cs2, CString cs3, double d, CString cs4)

	{
		strDescription = cs1;
		strDescription2 = cs2;
		strAbbreviation = cs3;
		dValue = d;
		strUint = cs4;
	}
};

class SeBoneParamCaculate
{
public:
	SeBoneParamCaculate();
	~SeBoneParamCaculate();

	void CaculateChoicedParam(CDcmPicArray* pDcmArray, int nMin, int nMax, int nMinValue);

	void SetCaculateParam(BOOL BasicParameter = FALSE,
		BOOL BoneThick = FALSE,
		BOOL SpaceThick = FALSE,
		BOOL BoneNumber = FALSE,
		BOOL Connectivity = FALSE,
		BOOL SMI = FALSE,
		BOOL DA = FALSE,
		BOOL TBPf = FALSE,
		BOOL Density = FALSE,
		BOOL CorticalArea = FALSE,
		BOOL CorticalThick = FALSE,
		BOOL CorticalFraction = FALSE,
		BOOL MedullaryArea = FALSE,
		BOOL CorticalDensity = FALSE,
		int  SmallValue = 0,
		int  BigValue = 0,
		float SmallDensity = 0.0,
		float BigDensity = 0.0);

public:
	vector<SingleParam> m_vecResult;

private:
	void ResetAllResult();

	void CalculateArea();
	void CalculateVolume();

	void CalculateBoneThick();
	void CalculateSpaceThick();
	void CalculateBoneNumber();
	void CalculateConnectivity();
	void CalculateSMI();
	void CalculateTBPf();
	void CalculateDA();
	void CalculateDensity();

	void CalculateCorticalArea();
	void CalculateCorticalThickness();
	void CalculateCorticalFraction();
	void CalculateMedullaryArea();
	void CalculateCorticalDensity();

	void ConvertResult();

private:
	BOOL m_bBasicParameter;
	BOOL m_bBoneThick;
	BOOL m_bSpaceThick;
	BOOL m_bBoneNumber;
	BOOL m_bConnectivity;
	BOOL m_bSMI;
	BOOL m_bDA;
	BOOL m_bTBPf;
	BOOL m_bDensity;
	BOOL m_bCorticalArea;
	BOOL m_bCorticalThick;
	BOOL m_bCorticalFraction;
	BOOL m_bMedullaryArea;
	BOOL m_bCorticalDensity;

	int  m_nSmallValue;
	int  m_nBigValue;
	float m_fSmallDensity;
	float m_fBigDensity;

	// Basic
	double m_dTotleVolume;
	double m_dVolume;
	double m_dSurfaceArea;
	double m_dSpecificSurfaceArea;
	double m_dVolumeFraction;
	double m_dSurfaceDensity;
	double m_dCentroidX;
	double m_dCentroidY;
	double m_dCentroidZ;

	double m_dStructureThickness;
	double m_dThicknessMax;
	double m_dThicknessStdDev;
	double m_dSeparation;
	double m_dSeparationStdDev;
	double m_dSeparationMax;
	double m_dNumberOfBone;
	double m_dEulerNumber;
	double m_dConnectivity;
	double m_dConnectivityDensity;
	double m_dDegreeOfAnisotropy;
	double m_dEigenValue1;
	double m_dEigenValue2;
	double m_dEigenValue3;
	double m_dSMI;
	double m_dTBPf;
	double m_dDensity;
	double m_dContent;
	double m_dMeanValue;
	double m_dDensityTotal;
	double m_dMeanValueTotal;


	double m_dCorticalTotalArea;
	double m_dCorticalArea;
	double m_dCorticalThickness;
	double m_dCorticalAreaFraction;
	double m_dMedullaryArea;
	double m_dCorticalContent;
	double m_dCorticalDensity;
	// temporary
	double m_dEmptyVolume;

	BOOL  m_bCortical;
	BOOL  m_bThickness;
public:
	void SetEmptyVolume(double volume){ m_dEmptyVolume = volume;}
private:
	// ParamForImageInfo
	CDcmPicArray* m_pDcmArray;
	int           m_nWidth;
	int           m_nHeight;
	int           m_nSize;
	int           m_nMinValue; // 图片最小值
	int           m_nMin; // 二值化最小值
	int           m_nMax; // 二值化最大值
	double        m_dMMperPixelXY;
	double        m_dMMperPixelZ;

	

	// help to caculate
	vector<CTriangle>   m_vTriangle;
	double		ResampleSMI(int ResampleVoxel,BOOL BForSMI);
	short**		Resample(int factorX, int factorY, int factorZ, int& VoxelNum);
	double		ResampleAreaCalulate(short** Imageplus, int Width,int Height,int ImagePiece);
	void        TriangleClassify(map<vertex,vector<int>>& vertexHash, map<vertex, vector<int>>::iterator& it);
	void        AverageNormal(map <vertex,vertex>& normal, map<vertex,vector<int>>& vertexHash, map<vertex, vector<int>>::iterator& it);

	double		MoveTriangleAndCalArea(map <vertex,vertex>& normal);
	vertex		crossProduct(CTriangle &vertext,int corner);

	//Resample
	int            m_nReWidth;
	int            m_nReHeight;
	int            m_nReSize;
	double         m_dblRePixelXY;
	double         m_dblRePixelZ;

};