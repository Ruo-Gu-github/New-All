#include "StdAfx.h"
#include "SeBoneParamAnistrophy.h"
#include "BoneDensitySwapData.h"

SeBoneAnistrophy::SeBoneAnistrophy(void)
{

}

SeBoneAnistrophy::SeBoneAnistrophy(CDcmPicArray* p, int nMinValue, int nMin, int nMax, int nWidth, int nHeight, int nSize, double dMMperPixelXY, double dMMperPixelZ)
{
	m_pProcessArray = p;
	m_nMinValue = nMinValue;
	m_nMin = nMin;
	m_nMax = nMax;
	m_nWidth = nWidth;
	m_nHeight = nHeight;
	m_nOriImagePiece = nSize;
	m_dMMperPixelXY = dMMperPixelXY;
	m_dMMperPixelZ = dMMperPixelZ;
}

SeBoneAnistrophy::~SeBoneAnistrophy(void)
{
}

double* SeBoneAnistrophy::gridCalculator(double radius)
{
	double dStackWidth = m_dMMperPixelXY * m_nWidth;
	double dStackHeight = m_dMMperPixelXY * m_nHeight;
	double dStackDepth = m_dMMperPixelZ * m_nOriImagePiece;
	// strategy: n random coordinates within bounding box (easy, no bias.)
	double random1 =rand()/(double)RAND_MAX;
	double random2 =rand()/(double)RAND_MAX;
	double random3 =rand()/(double)RAND_MAX;

	double* gridCentroids = new double[3];
	gridCentroids[0] = random1* (dStackWidth - 2 * radius - 2 * m_dMMperPixelXY) + radius;
	gridCentroids[1] = random2* (dStackHeight - 2 * radius - 2 * m_dMMperPixelXY) + radius;
	gridCentroids[2] = random3* (dStackDepth - 2 * radius - 2 * m_dMMperPixelZ) + radius;

	return gridCentroids;
}


double* SeBoneAnistrophy::regularVectors( int nVectors )
{
	double* vectors = new double[3*nVectors];
	double inc = PI * (3 - sqrt(double(5)));
	double off = 2 / (double) nVectors;

	for (int k = 0; k< nVectors; k++) {
		double y = k * off - 1 + (off / 2);
		double r = sqrt(1.0 - y * y);
		double phi = k * inc;
		double x = cos(phi) * r;
		double z = sin(phi) * r;
		vectors[k*3] = x;
		vectors[k*3+1] = y;
		vectors[k*3+2] = z;
	}
	return vectors;
}

double* SeBoneAnistrophy::irregularVectors( int nVectors )
{
	double* vectors = new double[3*nVectors];

	for (int k = 0; k < nVectors; k++) 
	{
		double random = rand()/(double)RAND_MAX;
		double z = 2 * random - 1;
		double rho = sqrt(1 - z * z);

		double randomphi = rand()/(double)RAND_MAX;
		double phi = PI * (2 * randomphi - 1);
		
		vectors[k*3] = rho * cos(phi);
		vectors[k*3+1] = rho * sin(phi);
		vectors[k*3+2] = z;
	}
	return vectors;
}

double* SeBoneAnistrophy::countIntercepts( double* centroid, double* vectorList,int nVectors,double radius,double vectorSampling )
{
	double vW = m_dMMperPixelXY;
	double vH = m_dMMperPixelXY;
	double vD = m_dMMperPixelZ;

	double cX = centroid[0];
	double cY = centroid[1];
	double cZ = centroid[2];

	int width = m_nWidth;
	int height = m_nHeight;
	int depth = m_nOriImagePiece;

	int w = doubleToInt(radius / vW);
	int h = doubleToInt(radius / vH);
	int d = doubleToInt(radius / vD);
	int sizework = (2 * w + 1) * (2 * h + 1)* (2 * d + 1);
	short* workArray = new short[sizework];
	memset(workArray,0,sizework*sizeof(short));

	int startCol = doubleToInt(cX / vW) - w;
	int endCol = doubleToInt(cX / vW) + w;
	int startRow = doubleToInt(cY / vH) - h;
	int endRow = doubleToInt(cY / vH) + h;
	int startSlice = doubleToInt(cZ / vD) - d;
	int endSlice = doubleToInt(cZ / vD) + d;

	int i = 0;
	for (int s = startSlice; s <= endSlice; s++)
	{
		for (int r = startRow; r <= endRow; r++) 
		{
			int index = width * r;
			for (int c = startCol; c <= endCol; c++) 
			{
				workArray[i] = ((short*)m_pProcessArray->GetDcmArray()[s]->GetData())[index + c];
				i++;
			}
		}
	}

	int a = (2 * w + 1);
	int b = a * (2 * h + 1);

	int wVz = b * d;
	int wVy = a * h;
	int wVx = w;
	int centroidIndex = wVz + wVy + wVx;

	// store an intercept count for each vector
	double* interceptCounts = new double[nVectors];
	memset(interceptCounts,0,nVectors*sizeof(double));

	double radVw = -radius / vW;
	double radVh = -radius / vH;
	double radVd = -radius / vD;

	for (int v = 0; v < nVectors; v++) 
	{
		double nIntercepts = 0;
		double vX = vectorList[3*v];
		double vY = vectorList[3*v+1];
		double vZ = vectorList[3*v+2];
		
		// start at negative end of vector
		int xS = doubleToInt(radVw * vX);
		int yS = doubleToInt(radVh * vY);
		int zS = doubleToInt(radVd * vZ);
		
		int startIndex = centroidIndex + b * zS + a * yS + xS;
		bool lastPos, thisPos;
		if (workArray[startIndex] <= m_nMin || workArray[startIndex] >= m_nMax) {
			lastPos = true;
		} else {
			lastPos = false;
		}

		double vXvW = vX / vW;
		double vYvH = vY / vH;
		double vZvD = vZ / vD;

		for (double pos = -radius; pos <= radius; pos += vectorSampling) 
		{
			int x = doubleToInt(pos * vXvW);
			int y = doubleToInt(pos * vYvH);
			int z = doubleToInt(pos * vZvD);
			int testIndex = centroidIndex + b * z + a * y + x;
			if (workArray[testIndex] < m_nMin || workArray[testIndex] > m_nMax){
				thisPos = true;
			}
			else{
				thisPos = false;
			}

			if (thisPos != lastPos) {
				nIntercepts++;
			}
			lastPos = thisPos;
		}
		interceptCounts[v] = nIntercepts;
	}
	Safe_DeleteVec(workArray);
	return interceptCounts;
}


int SeBoneAnistrophy::doubleToInt(double f){  
	int i = 0;  
	if(f>0) //正数  
		i = (f*10 + 5)/10;  
	else if(f<0) //负数  
		i = (f*10 - 5)/10;  
	else i = 0;  
	return i;  
}  

double* SeBoneAnistrophy::calculateCoordinates( double* meanInterceptLengths,double* vectorList,int nVectors )
{
	vector<double> coordList;
	for (int v = 0; v < nVectors; v++) {
		double milV = meanInterceptLengths[v];
		if (milV == 0)
			continue;
		double x1 = milV * vectorList[3*v];
		double y1 = milV * vectorList[3*v + 1];
		double z1 = milV * vectorList[3*v + 2];
		coordList.push_back(x1);
		coordList.push_back(y1);
		coordList.push_back(z1);
	}
	m_nTotleNum = (int)coordList.size();
	double* coordinates = new double[m_nTotleNum];
	for (int i = 0; i < m_nTotleNum; i++)
		coordinates[i] = coordList[i];

	coordList.clear();
	return coordinates;
}

double* SeBoneAnistrophy::harriganMann( double* coOrdinates)
{
	double da = 0;
	int Num = m_nTotleNum/3;
	MatrixXd D = MatrixXd::Zero(Num,9);
	for (int i = 0; i < Num; i++) {
		double x = coOrdinates[i*3];
		double y = coOrdinates[i*3+1];
		double z = coOrdinates[i*3+2];
		D(i,0) = x * x;
		D(i,1) = y * y;
		D(i,2) = z * z;
		D(i,3) = 2 * x * y;
		D(i,4) = 2 * x * z;
		D(i,5) = 2 * y * z;
		D(i,6) = 2 * x;
		D(i,7) = 2 * y;
		D(i,8) = 2 * z;
	}
	MatrixXd ones = MatrixXd::Ones(Num, 1);
	MatrixXd V = ((D.transpose()*(D)).inverse())*(D.transpose()*(ones));
	double* coEf = getColumnPackedCopy( V );

	MatrixXd tensor(3,3);
	tensor  << coEf[0],coEf[3],coEf[4],
		coEf[3],coEf[1],coEf[5],
		coEf[4],coEf[5],coEf[2];

	EigenSolver<MatrixXd> eigensolver(tensor);
	MatrixXcd EigenVal = eigensolver.eigenvalues();
	complex<double> a = EigenVal(0,0);
	complex<double> b = EigenVal(1,0);
	complex<double> c = EigenVal(2,0);

	double* diag  = new double[4];
	diag[0] =abs(a.real());
	diag[1] =abs(b.real());
	diag[2] =abs(c.real());	

	double maxvalue;
	if(diag[0]>diag[1]&&diag[0]>diag[2])
	{ maxvalue = diag[0];}
	else if(diag[1]>diag[0]&&diag[1]>diag[2])
	{ maxvalue = diag[1];}
	else
	{ maxvalue = diag[2];}

	double minvalue;
	if(diag[0]<diag[1]&&diag[0]<diag[2])
	{ minvalue = diag[0];}
	else if(diag[1]<diag[0]&&diag[1]<diag[2])
	{ minvalue = diag[1];}
	else
	{ minvalue = diag[2];}

	da = 1 - minvalue / maxvalue;
	if (da > 1)
		da = 1;
	else if (da < 0)
		da = 0;

	diag[3] = da;

	Safe_DeleteVec(coEf);
	Safe_DeleteVec(coOrdinates);
	return diag;
}


double* SeBoneAnistrophy::getColumnPackedCopy( MatrixXd eVec )
{
	int rowv = eVec.rows();
	int colv = eVec.cols();
	double* eigenVectors = new double[eVec.size()];
	
	for (int nv=0;nv<rowv;nv++)
	{
		for (int mv=0;mv<colv;mv++)
		{
			eigenVectors[mv*rowv+nv] = eVec(nv,mv);
		}
	}

	return eigenVectors;
}

double SeBoneAnistrophy::getVariance( vector<double> anisotropyHistory, int n )
{
	double sum = 0;

	double sumSquares = 0;

	int count = 0;

	int lgth = (int)anisotropyHistory.size()-1;

	while ((lgth-count)>=0) {
		double value = anisotropyHistory[lgth-count];
		sum += value;
		count++;
		if (count >= n)
			break;
	}

	double mean = sum / n;

	count = 0;
	while ((lgth-count)>=0) {
		double value = anisotropyHistory[lgth-count];
		double a = value - mean;
		sumSquares += a * a;
		count++;
		if (count >= n)
			break;
	}

	double stDev = sqrt(sumSquares / n);
	double coeffVariation = stDev / mean;
	return coeffVariation;
}
