#pragma once

#include "Eigen/Dense" 
using namespace Eigen; 
using namespace std;

class SeBoneAnistrophy
{
public:
	SeBoneAnistrophy(void);
	SeBoneAnistrophy(CDcmPicArray* p, int nMinValue, int nMin, int nMax, int nWidth, int nHeight, int nSize, double dMMperPixelXY, double dMMperPixelZ);

	~SeBoneAnistrophy(void);
public:
	double*				regularVectors(int nVectors);
	double*				gridCalculator(double radius);
	double*				countIntercepts(double* centroid, double* vectorList,int nVectors,double radius,double vectorSampling);
	double*				calculateCoordinates(double* meanInterceptLengths,double* vectorList,int nVectors);
	double*				harriganMann(double* coOrdinates);
	double				getVariance(vector<double> anisotropyHistory, int n);

private:
	double*				irregularVectors(int nVectors);
	double*				getColumnPackedCopy(MatrixXd V);
	int					doubleToInt(double f);

private:
	CDcmPicArray*       m_pProcessArray;
	int                 m_nMinValue;
	int                 m_nMin;
	int                 m_nMax;
	int                 m_nWidth;
	int                 m_nHeight;
	int                 m_nOriImagePiece;
	double				m_dMMperPixelXY;
	double				m_dMMperPixelZ;
	int                 m_nTotleNum;
};

