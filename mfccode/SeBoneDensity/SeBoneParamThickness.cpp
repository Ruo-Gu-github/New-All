#include "StdAfx.h"
#include "SeBoneParamThickness.h"
#include "BoneDensitySwapData.h"

SeBoneThickness::SeBoneThickness(void)
{
// 	m_pImage = NULL;
// 	m_edgePoints = NULL;
// 	InitializeCriticalSection(&pLock);
// 	m_numRadii = 0;
// 	m_average = 0;
// 	m_sNew = NULL;

}

SeBoneThickness::SeBoneThickness(CDcmPicArray* p, int nMinValue, int nMin, int nMax, int nWidth, int nHeight, int nSize)
{
// 	m_edgePoints = NULL;
// 	InitializeCriticalSection(&pLock);
// 	m_numRadii = 0;
// 	m_average = 0;
// 	m_sNew = NULL;


	m_pProcessArray = p;
	m_nMinValue = nMinValue;
	m_nMin = nMin;
	m_nMax = nMax;
	m_nWidth = nWidth;
	m_nHeight = nHeight;
	m_nOriImagePiece = nSize;
}


SeBoneThickness::~SeBoneThickness(void)
{

//	DeleteCriticalSection(&pLock);
}

// float** SeBoneThickness::EDT(BOOL bSpace /*=False*/)     
// {
// 	int n = m_nWidth > m_nHeight ? m_nWidth : m_nHeight;
// 	n = n > m_nOriImagePiece ? n : m_nOriImagePiece;
// 
// 	int noResult = 3 * (n + 1) * (n + 1);
// 	bool* background = new bool[n];
// 	memset(background,0,sizeof(bool)*n);
// 	int test = 0, min = 0;
// 
// 	float** s = new float*[m_nOriImagePiece];
// 	for (int i=0;i<m_nOriImagePiece;i++)
// 	{
// 		s[i] = new float[m_nHeight*m_nWidth];
// 		memset(s[i],0,sizeof(float)*m_nHeight*m_nWidth);
// 	}
// 
// 	for (int k=0; k<m_nOriImagePiece; k++) 
// 	{
// 		short* dk = (short*)m_pProcessArray->GetDcmArray()[k]->GetData();
// 		for (int j = 0; j < m_nHeight; j++) {
// 			int wj = m_nWidth * j;
// 			for (int i = 0; i < m_nWidth; i++) {
// 				background[i] = (dk[i + wj] < m_nMin || dk[i + wj] > m_nMax);
// 				if (bSpace)
// 					background[i] = !background[i];
// 			}
// 			for (int i = 0; i < m_nWidth; i++) {
// 				min = noResult;
// 				for (int x = i; x < m_nWidth; x++) {
// 					if (background[x]) {
// 						test = i - x;
// 						test *= test;
// 						min = test;
// 						break;
// 					}
// 				}
// 				for (int x = i-1; x >= 0; x--) {
// 					if (background[x]) {
// 						test = i - x;
// 						test *= test;
// 						if (test < min)
// 							min = test;
// 						break;
// 					}
// 				}
// 				s[k][i + wj] = min;
// 			}
// 		}
// 	}	
// 
// 	int* tempInt = new int[n];
// 	memset(tempInt,0,sizeof(int)*n);
// 	int* tempS = new int[n];
// 	memset(tempS,0,sizeof(int)*n);
// 	bool nonempty = true;
// 	test = 0, min = 0;
// 	int delta = 0;
// 	for (int k = 0; k < m_nOriImagePiece; k ++)
// 	{
// 		for (int i = 0; i < m_nWidth; i++) {
// 			nonempty = false;
// 			for (int j = 0; j < m_nHeight; j++) {
// 				tempS[j] = (int) s[k][i + m_nWidth * j];
// 				if (tempS[j] > 0)
// 					nonempty = true;
// 			}
// 			if (nonempty) {
// 				for (int j = 0; j < m_nHeight; j++) {
// 					min = noResult;
// 					delta = j;
// 					for (int y = 0; y < m_nHeight; y++) {
// 						test = tempS[y] + delta * delta--;
// 						if (test < min)
// 							min = test;
// 					}
// 					tempInt[j] = min;
// 				}
// 				for (int j = 0; j < m_nHeight; j++) {
// 					s[k][i + m_nWidth * j] = tempInt[j];
// 				}
// 			}
// 		}
// 	}
// 
// 	memset(tempInt,0,sizeof(int)*n);
// 	memset(tempS,0,sizeof(int)*n);
// 	int zStart = 0, zStop = 0, zBegin = 0, zEnd = 0;
// 	for (int j = 0; j < m_nHeight; j ++) {
// 		int wj = m_nWidth * j;
// 		for (int i = 0; i < m_nWidth; i++) {
// 			nonempty = false;
// 			for (int k = 0; k < m_nOriImagePiece; k++) {
// 				tempS[k] = (int) s[k][i + wj];
// 				if (tempS[k] > 0)
// 					nonempty = true;
// 			}
// 			if (nonempty) {
// 				zStart = 0;
// 				while ((zStart < (m_nOriImagePiece - 1)) && (tempS[zStart] == 0))
// 					zStart++;
// 				if (zStart > 0)
// 					zStart--;
// 				zStop = m_nOriImagePiece - 1;
// 				while ((zStop > 0) && (tempS[zStop] == 0))
// 					zStop--;
// 				if (zStop < (m_nOriImagePiece - 1))
// 					zStop++;
// 
// 				for (int k = 0; k < m_nOriImagePiece; k++) {
// 					short* daTa = (short*)m_pProcessArray->GetDcmArray()[k]->GetData();
// 					// Limit to the non-background to save time,
// 					if (daTa[i + wj] > m_nMin && daTa[i + wj] < m_nMax) {
// 						min = noResult;
// 						zBegin = zStart;
// 						zEnd = zStop;
// 						if (zBegin > k)
// 							zBegin = k;
// 						if (zEnd < k)
// 							zEnd = k;
// 						delta = k - zBegin;
// 						for (int z = zBegin; z <= zEnd; z++) {
// 							test = tempS[z] + delta * delta--;
// 							if (test < min)
// 								min = test;
// 							// min = (test < min) ? test : min;
// 						}
// 						tempInt[k] = min;
// 					}
// 				}
// 				for (int k = 0; k < m_nOriImagePiece; k++) {
// 					s[k][i + wj] = tempInt[k];
// 				}
// 			}
// 		}
// 	}
// 	Safe_DeleteVec(background);
// 	Safe_DeleteVec(tempInt);
// 	Safe_DeleteVec(tempS);
// 
// 	return s;
// }
// 
// 
// 
// 	////////////////////////////////////////////edge
// 
// void SeBoneThickness::DistanceMaptoDistanceRidge(float** s)
// {
// 	float m_nValueMax = 0,dist = 0;
// 	int wh = m_nHeight * m_nWidth;
// 	for (int k = 0; k < m_nOriImagePiece; k++) {
// 		short* daTa = (short*)m_pProcessArray->GetDcmArray()[k]->GetData();
// 		for (int ind = 0; ind < wh; ind++) {
// 			if (daTa[ind] <= m_nMin || daTa[ind] > m_nMax) {
// 				s[k][ind] = 0;
// 			} else {
// 				dist = (float) sqrt(s[k][ind]);
// 				s[k][ind] = dist;
// 				m_nValueMax = (dist > m_nValueMax) ? dist : m_nValueMax;
// 			}
// 		}
// 	}//max distance = m_nValueMax
// 
// 	int k1 = 0,j1 =0 ,i1 = 0;
// 	int skOSqlnd = 0,skOSq = 0;
// 	int rSqMax = (int) (m_nValueMax * m_nValueMax + 0.5f) + 1;
// 	bool* occurs = new bool[rSqMax];
// 	memset(occurs , false , rSqMax*sizeof(bool));
// 	for (int k = 0; k < m_nOriImagePiece; k++) 
// 	{
// 		float* sk = s[k];
// 		for (int j = 0; j < m_nHeight; j++) {
// 			int wj = m_nWidth * j;
// 			for (int i = 0; i < m_nWidth; i++) {
// 				int ind = i+wj;
// 				occurs[(int)(sk[ind]*sk[ind]+0.5f)] = true;
// 				}
// 			}
// 	}//distance range. occurs value is true in distance range
// 		
// 	m_numRadii = 0;
// 	for (int i=0; i<rSqMax; i++)
// 	{
// 		if (occurs[i])
// 			m_numRadii++;
// 	}
// 
// 	//make an index of the distance-squared values
// 	int* distSqIndex = new int[rSqMax];
// 	memset(distSqIndex,0,sizeof(int)*rSqMax);
// 	int* distSqValues = new int[m_numRadii];
// 	memset(distSqValues,0,sizeof(int)*m_numRadii);
// 	int indDs = 0;
// 	for (int i=0;i<rSqMax;i++)
// 	{
// 		if (occurs[i])
// 		{
// 			distSqIndex[i] = indDs;
// 			distSqValues[indDs++] = i;
// 		}
// 	}
// 
// 	int** rSqTemplate = createTemplate(distSqValues);
// 	int numCompZ, numCompY, numCompX, numComp;
// 
// 	m_sNew = new float*[m_nOriImagePiece];
// 	for (int i=0;i<m_nOriImagePiece;i++)
// 	{
// 		m_sNew[i] = new float[m_nHeight*m_nWidth];
// 		memset(m_sNew[i],0,sizeof(float)*m_nHeight*m_nWidth);
// 	}
// 
// 	for(int i = 0; i < m_nOriImagePiece; i++)
// 	{
// 		float* sk = s[i];
// 		float* m_edge = m_sNew[i];
// 		for (int m = 0; m < m_nHeight; m++)
// 		{
// 			int IndexY = m*m_nWidth;
// 			for (int n = 0; n < m_nWidth; n++)
// 			{	
// 				int ind = IndexY+n;
// 				if (sk[ind]>0)
// 				{
// 					bool notRidgePoint = false;
// 					skOSq = (int) (sk[ind] * sk[ind] + 0.5f);
// 					skOSqlnd = distSqIndex[skOSq];
// 					for (int dz=-1;dz<=1;dz++)
// 					{
// 						k1=i+dz;
// 						if ((k1>=0)&&(k1<m_nOriImagePiece))
// 						{
// 							float* sk1 = s[k1];
// 							if (dz==0){numCompZ = 0;}
// 							else {numCompZ=1;}
// 							for (int dy=-1;dy<=1;dy++)
// 							{
// 								j1=m+dy;
// 								int wj1 = m_nWidth * j1;
// 								if ((j1>=0)&&(j1<m_nHeight))
// 								{
// 									if (dy==0){numCompY = 0;}
// 									else {numCompY=1;}
// 									for (int dx=-1;dx<=1;dx++)
// 									{
// 										i1=n+dx;
// 										if ((i1>=0)&&(i1<m_nWidth))
// 										{
// 											if (dx==0){numCompX = 0;}
// 											else {numCompX=1;}
// 											numComp=numCompX+numCompY+numCompZ;
// 											if (numComp>0)
// 											{
// 												float sk1i1wj1 = sk1[i1+ wj1];
// 												int sk1Sq = (int) (sk1i1wj1 * sk1i1wj1 + 0.5f);
// 												if (sk1Sq >= rSqTemplate[numComp - 1][skOSqlnd])
// 													notRidgePoint = true;
// 											}
// 										}
// 										if (notRidgePoint)
// 											break;
// 									}	
// 								}
// 								if (notRidgePoint)
// 									break;
// 							}	
// 						}
// 						if (notRidgePoint)
// 							break;
// 					}
// 					if (!notRidgePoint)	
// 						m_edge[ind] = sk[ind];
// 				}
// 	        }
// 		}
// 	}
// 
// 	for(int i = 0; i < m_nOriImagePiece; i++)
// 	{	
// 		memcpy(s[i],m_sNew[i],sizeof(float)*m_nHeight*m_nWidth);
// 		memset(m_sNew[i],0,sizeof(float)*m_nHeight*m_nWidth);
// 	}
// 
// 	for (int i=0;i<3;i++)
// 	{
// 		Safe_DeleteVec(rSqTemplate[i]);
// 	}
// 	Safe_Delete(rSqTemplate);
// 
// 	Safe_DeleteVec(occurs);
// 	Safe_DeleteVec(distSqIndex);
// 	Safe_DeleteVec(distSqValues);
// }
// 
// 
// 
// 	////////////////////////////////////////////thickness
// void SeBoneThickness::DistanceRidgetoLocalThickness(float**s)
// {
// 	int* nRidge = new int[m_nOriImagePiece];
// 	memset(nRidge,0,sizeof(int)*m_nOriImagePiece);
// 	int ind = 0, nr = 0, iR = 0;
// 	for (int k = 0; k < m_nOriImagePiece; k++) {
// 		float* sk = s[k];
// 		nr = 0;
// 		for (int j = 0; j < m_nHeight; j++) {
// 			int wj = m_nWidth * j;
// 			for (int i = 0; i < m_nWidth; i++) {
// 				ind = i + wj;
// 				if (sk[ind] > 0)
// 					nr++;
// 			}
// 		}
// 		nRidge[k] = nr;
// 	}
// 	int** iRidge = new int*[m_nOriImagePiece];
// 	int** jRidge = new int*[m_nOriImagePiece];
// 	float** rRidge = new float*[m_nOriImagePiece];
// 	// Pull out the distance ridge points
// 	int* iRidgeK = NULL;
// 	int* jRidgeK = NULL;
// 	float* rRidgeK = NULL;
// 	float sMax = 0;
// 	for (int k = 0; k < m_nOriImagePiece; k++) {
// 		nr = nRidge[k];
// 		iRidge[k] = new int[nr];
// 		memset(iRidge[k],0,nr*sizeof(int));
// 		jRidge[k] = new int[nr];
// 		memset(jRidge[k],0,nr*sizeof(int));
// 		rRidge[k] = new float[nr];
// 		memset(rRidge[k],0,nr*sizeof(float));
// 		float* sk = s[k];
// 		iRidgeK = iRidge[k];
// 		jRidgeK = jRidge[k];
// 		rRidgeK = rRidge[k];
// 		iR = 0;
// 		for (int j = 0; j < m_nHeight; j++) {
// 			int wj = m_nWidth * j;
// 			for (int i = 0; i < m_nWidth; i++) {
// 				ind = i + wj;
// 				if (sk[ind] > 0) {
// 					iRidgeK[iR] = i;
// 					jRidgeK[iR] = j;
// 					rRidgeK[iR++] = sk[ind];
// 					if (sk[ind] > sMax)
// 						sMax = sk[ind];
// 					sk[ind] = 0;
// 				}
// 			}
// 		}
// 	}
// 	int rInt;
// 	int iStart, iStop, jStart, jStop, kStart, kStop;
// 	float r1SquaredK, r1SquaredJK, r1Squared, s1;
// 	int rSquared;
// 	for (int k = 0; k < m_nOriImagePiece; k++) {
// 		int nR = nRidge[k];
// 		int* iRidgeK = iRidge[k];
// 		int* jRidgeK = jRidge[k];
// 		float* rRidgeK = rRidge[k];
// 		for (int iR = 0; iR < nR; iR++) {
// 			int i = iRidgeK[iR];
// 			int j = jRidgeK[iR];
// 			float r = rRidgeK[iR];
// 			rSquared = (int) (r * r + 0.5f);
// 			rInt = (int) r;
// 			if (rInt < r)
// 				rInt++;
// 			iStart = i - rInt;
// 			if (iStart < 0)
// 				iStart = 0;
// 			iStop = i + rInt;
// 			if (iStop >= m_nWidth)
// 				iStop = m_nWidth - 1;
// 			jStart = j - rInt;
// 			if (jStart < 0)
// 				jStart = 0;
// 			jStop = j + rInt;
// 			if (jStop >= m_nHeight)
// 				jStop = m_nHeight - 1;
// 			kStart = k - rInt;
// 			if (kStart < 0)
// 				kStart = 0;
// 			kStop = k + rInt;
// 			if (kStop >= m_nOriImagePiece)
// 				kStop = m_nOriImagePiece - 1;
// 			for (int k1 = kStart; k1 <= kStop; k1++) {
// 				r1SquaredK = (k1 - k) * (k1 - k);
// 				float* sk1 = s[k1];
// 				for (int j1 = jStart; j1 <= jStop; j1++) {
// 					int widthJ1 = m_nWidth * j1;
// 					r1SquaredJK = r1SquaredK + (j1 - j) * (j1 - j);
// 					if (r1SquaredJK <= rSquared) {
// 						for (int i1 = iStart; i1 <= iStop; i1++) {
// 							r1Squared = r1SquaredJK + (i1 - i)
// 								* (i1 - i);
// 							if (r1Squared <= rSquared) {
// 								int ind1 = i1 + widthJ1;
// 								s1 = sk1[ind1];
// 								if (rSquared > s1) {
// 									s1 = sk1[ind1];
// 									if (rSquared > s1) {
// 										sk1[ind1] = rSquared;
// 									}
// 								}
// 							}// if within sphere of DR point
// 						}// i1
// 					}// if k and j components within sphere of DR point
// 				}// j1
// 			}// k1
// 		}// iR
// 	}// k
// 
// 	for (int k = 0; k < m_nOriImagePiece; k++) {
// 		float* sk = s[k];
// 		for (int j = 0; j < m_nHeight; j++) {
// 			int wj = m_nWidth * j;
// 			for (int i = 0; i < m_nWidth; i++) {
// 				ind = i + wj;
// 				sk[ind] = (float) (2 * sqrt(sk[ind]));
// 			}
// 		}
// 	}	
// 	Safe_DeleteVec(nRidge);
// 
// 	for (int i=0;i<m_nOriImagePiece;i++)
// 	{
// 		Safe_DeleteVec(iRidge[i]);
// 		Safe_DeleteVec(jRidge[i]);
// 		Safe_DeleteVec(rRidge[i]);
// 	}
// 	Safe_Delete(iRidge);
// 	Safe_Delete(jRidge);
// 	Safe_Delete(rRidge);
// 
// }
// 
// float* SeBoneThickness::CleanedUpLocalThickness(float** s)
// {
// 	for (int k = 0; k < m_nOriImagePiece; k++) {
// 		for (int j = 0; j < m_nHeight; j++) {
// 			int wj = m_nWidth * j;
// 			for (int i = 0; i < m_nWidth; i++) {
// 				m_sNew[k][i + wj] = setFlag(s, i, j, k, m_nWidth, m_nHeight, m_nOriImagePiece);
// 			}// i
// 		}// j
// 	}// k
// 
// 	for (int k = 0; k < m_nOriImagePiece; k++) {
// 		for (int j = 0; j < m_nHeight; j++) {
// 			int wj = m_nWidth * j;
// 			for (int i = 0; i < m_nWidth; i++) {
// 				int ind = i + wj;
// 				if (m_sNew[k][ind] == -1) {
// 					m_sNew[k][ind] = -averageInteriorNeighbors(s, i, j, k, m_nWidth, m_nHeight, m_nOriImagePiece);
// 				}
// 			}// i
// 		}// j
// 	}// k
// 
// 	for (int i=0;i<m_nOriImagePiece;i++)
// 		Safe_DeleteVec(s[i]);
// 	Safe_Delete(s);
// 
// 	for (int k = 0; k < m_nOriImagePiece; k++) {
// 		for (int j = 0; j < m_nHeight; j++) {
// 			int wj = m_nWidth * j;
// 			for (int i = 0; i < m_nWidth; i++) {
// 				int ind = i + wj;
// 				m_sNew[k][ind] = (float)abs(m_sNew[k][ind]);
// 			}// i
// 		}// j
// 	}// k
// 
// 	////// Reduce error in thickness quantitation by trimming the one pixel overhang in the thickness map
// 	int wh = m_nHeight*m_nWidth;
// 	for (int k = 0; k < m_nOriImagePiece; k++) {
// 		short* daTa = (short*)m_pProcessArray->GetDcmArray()[k]->GetData();
// 		for (int ind = 0; ind < wh; ind++) {
// 			if (daTa[ind] <= m_nMin || daTa[ind] >= m_nMax) {
// 				m_sNew[k][ind] = 0;
// 			}
// 		}
// 	}
// 
// 	double pixCount = 0;
// 	double sumThick = 0;
// 	double maxThick = 0;
// 
// 	float* result = new float[3];
// 	memset(result,0,sizeof(float)*3);
// 	for (int k = 0; k < m_nOriImagePiece; k++) {
// 		float* slicePixels = m_sNew[k];
// 		for (int p = 0; p < wh; p++) {
// 			double pixVal = slicePixels[p];
// 			if (pixVal > 0) {
// 				sumThick += pixVal;
// 				maxThick = max(maxThick, pixVal);
// 				pixCount++;
// 			}
// 		}
// 	}
// 	float meanThick = sumThick / pixCount;
// 	double sumSquares = 0;
// 	for (int s = 0; s < m_nOriImagePiece; s++) {
// 		float* slicePixels = m_sNew[s];
// 		for (int p = 0; p < wh; p++) {
// 			double pixVal = slicePixels[p];
// 			if (pixVal > 0) {
// 				double residual = meanThick - pixVal;
// 				sumSquares += residual * residual;
// 			}
// 		}
// 	}
// 	double stDev = sqrt(sumSquares / pixCount);	
// 
// 	result[0] = meanThick;
// 	result[1] = stDev;
// 	result[2] = maxThick;
// 
// 	for (int i=0;i<m_nOriImagePiece;i++)
// 		Safe_DeleteVec(m_sNew[i]);
// 	Safe_Delete(m_sNew);
// 
// 	return result;
// }
// 
// float SeBoneThickness::averageInteriorNeighbors(float** s, int i, int j, int k, int w,int h, int d) {
// 		int n = 0;
// 		float sum = 0;
// 		// change 1
// 		float value = lookNew(i, j, k - 1, w, h, d);
// 		if (value > 0) {
// 			n++;
// 			sum += value;
// 		}
// 		value = lookNew(i, j, k + 1, w, h, d);
// 		if (value > 0) {
// 			n++;
// 			sum += value;
// 		}
// 		value = lookNew(i, j - 1, k, w, h, d);
// 		if (value > 0) {
// 			n++;
// 			sum += value;
// 		}
// 		value = lookNew(i, j + 1, k, w, h, d);
// 		if (value > 0) {
// 
// 			n++;
// 			sum += value;
// 		}
// 		value = lookNew(i - 1, j, k, w, h, d);
// 		if (value > 0) {
// 			n++;
// 			sum += value;
// 		}
// 		value = lookNew(i + 1, j, k, w, h, d);
// 		if (value > 0) {
// 			n++;
// 			sum += value;
// 		}
// 		// change 1 before plus
// 		value = lookNew(i, j + 1, k - 1, w, h, d);
// 		if (value > 0) {
// 			n++;
// 			sum += value;
// 		}
// 		value = lookNew(i, j + 1, k + 1, w, h, d);
// 		if (value > 0) {
// 			n++;
// 			sum += value;
// 		}
// 		value = lookNew(i + 1, j - 1, k, w, h, d);
// 		if (value > 0) {
// 			n++;
// 			sum += value;
// 		}
// 		value = lookNew(i + 1, j + 1, k, w, h, d);
// 		if (value > 0) {
// 			n++;
// 			sum += value;
// 		}
// 		value = lookNew(i - 1, j, k + 1, w, h, d);
// 		if (value > 0) {
// 			n++;
// 			sum += value;
// 		}
// 		value = lookNew(i + 1, j, k + 1, w, h, d);
// 		if (value > 0) {
// 			n++;
// 			sum += value;
// 		}
// 		// change 1 before minus
// 		value = lookNew(i, j - 1, k - 1, w, h, d);
// 		if (value > 0) {
// 			n++;
// 			sum += value;
// 		}
// 		value = lookNew(i, j - 1, k + 1, w, h, d);
// 		if (value > 0) {
// 			n++;
// 			sum += value;
// 		}
// 		value = lookNew(i - 1, j - 1, k, w, h, d);
// 		if (value > 0) {
// 			n++;
// 			sum += value;
// 		}
// 		value = lookNew(i - 1, j + 1, k, w, h, d);
// 		if (value > 0) {
// 			n++;
// 			sum += value;
// 		}
// 		value = lookNew(i - 1, j, k - 1, w, h, d);
// 		if (value > 0) {
// 			n++;
// 			sum += value;
// 		}
// 		value = lookNew(i + 1, j, k - 1, w, h, d);
// 		if (value > 0) {
// 			n++;
// 			sum += value;
// 		}
// 		// change 3, k+1
// 		value = lookNew(i + 1, j + 1, k + 1, w, h, d);
// 		if (value > 0) {
// 			n++;
// 			sum += value;
// 		}
// 		value = lookNew(i + 1, j - 1, k + 1, w, h, d);
// 		if (value > 0) {
// 			n++;
// 			sum += value;
// 		}
// 		value = lookNew(i - 1, j + 1, k + 1, w, h, d);
// 		if (value > 0) {
// 			n++;
// 			sum += value;
// 		}
// 		value = lookNew(i - 1, j - 1, k + 1, w, h, d);
// 		if (value > 0) {
// 			n++;
// 			sum += value;
// 		}
// 		// change 3, k-1
// 		value = lookNew(i + 1, j + 1, k - 1, w, h, d);
// 		if (value > 0) {
// 			n++;
// 			sum += value;
// 		}
// 		value = lookNew(i + 1, j - 1, k - 1, w, h, d);
// 		if (value > 0) {
// 			n++;
// 			sum += value;
// 		}
// 		value = lookNew(i - 1, j + 1, k - 1, w, h, d);
// 		if (value > 0) {
// 			n++;
// 			sum += value;
// 		}
// 		value = lookNew(i - 1, j - 1, k - 1, w, h, d);
// 		if (value > 0) {
// 			n++;
// 			sum += value;
// 		}
// 		if (n > 0)
// 			return sum / n;
// 		return s[k][i + w * j];
// }
// 
// // A positive result means this is an interior, non-background, point.
// float SeBoneThickness::lookNew(int i, int j, int k, int w, int h, int d) {
// 	if ((i < 0) || (i >= w))
// 		return -1;
// 	if ((j < 0) || (j >= h))
// 		return -1;
// 	if ((k < 0) || (k >= d))
// 		return -1;
// 	return m_sNew[k][i + w * j];
// }
// 
// float SeBoneThickness::setFlag(float** s, int i, int j, int k, int w, int h, int d) {
// 	if (s[k][i + w * j] == 0)
// 		return 0;
// 	// change 1
// 	if (look(s, i, j, k - 1, w, h, d) == 0)
// 		return -1;
// 	if (look(s, i, j, k + 1, w, h, d) == 0)
// 		return -1;
// 	if (look(s, i, j - 1, k, w, h, d) == 0)
// 		return -1;
// 	if (look(s, i, j + 1, k, w, h, d) == 0)
// 		return -1;
// 	if (look(s, i - 1, j, k, w, h, d) == 0)
// 		return -1;
// 	if (look(s, i + 1, j, k, w, h, d) == 0)
// 		return -1;
// 	// change 1 before plus
// 	if (look(s, i, j + 1, k - 1, w, h, d) == 0)
// 		return -1;
// 	if (look(s, i, j + 1, k + 1, w, h, d) == 0)
// 		return -1;
// 	if (look(s, i + 1, j - 1, k, w, h, d) == 0)
// 		return -1;
// 	if (look(s, i + 1, j + 1, k, w, h, d) == 0)
// 		return -1;
// 	if (look(s, i - 1, j, k + 1, w, h, d) == 0)
// 		return -1;
// 	if (look(s, i + 1, j, k + 1, w, h, d) == 0)
// 		return -1;
// 	// change 1 before minus
// 	if (look(s, i, j - 1, k - 1, w, h, d) == 0)
// 		return -1;
// 	if (look(s, i, j - 1, k + 1, w, h, d) == 0)
// 		return -1;
// 	if (look(s, i - 1, j - 1, k, w, h, d) == 0)
// 		return -1;
// 	if (look(s, i - 1, j + 1, k, w, h, d) == 0)
// 		return -1;
// 	if (look(s, i - 1, j, k - 1, w, h, d) == 0)
// 		return -1;
// 	if (look(s, i + 1, j, k - 1, w, h, d) == 0)
// 		return -1;
// 	// change 3, k+1
// 	if (look(s, i + 1, j + 1, k + 1, w, h, d) == 0)
// 		return -1;
// 	if (look(s, i + 1, j - 1, k + 1, w, h, d) == 0)
// 		return -1;
// 	if (look(s, i - 1, j + 1, k + 1, w, h, d) == 0)
// 		return -1;
// 	if (look(s, i - 1, j - 1, k + 1, w, h, d) == 0)
// 		return -1;
// 	// change 3, k-1
// 	if (look(s, i + 1, j + 1, k - 1, w, h, d) == 0)
// 		return -1;
// 	if (look(s, i + 1, j - 1, k - 1, w, h, d) == 0)
// 		return -1;
// 	if (look(s, i - 1, j + 1, k - 1, w, h, d) == 0)
// 		return -1;
// 	if (look(s, i - 1, j - 1, k - 1, w, h, d) == 0)
// 		return -1;
// 	return s[k][i + w * j];
// }
// 
// float SeBoneThickness::look(float** s, int i, int j, int k, int w, int h, int d) {
// 	if ((i < 0) || (i >= w))
// 		return -1;
// 	if ((j < 0) || (j >= h))
// 		return -1;
// 	if ((k < 0) || (k >= d))
// 		return -1;
// 	return s[k][i + w * j];
// }
// 
// 
// int** SeBoneThickness::createTemplate(int* distSqValues)
// {
// 	int** t= new int*[3];
// 	t[0]=scanCube(1,0,0,distSqValues);
// 	t[1]=scanCube(1,1,0,distSqValues);
// 	t[2]=scanCube(1,1,1,distSqValues);
// 	return t;
// }
// 
// int* SeBoneThickness::scanCube(int dx, int dy, int dz,int* distSqValues)
// {
// 	int* r1Sq=new int[m_numRadii];
// 	memset(r1Sq,0,sizeof(int)*m_numRadii);
// 	if ((dx==0)&&(dy==0)&&(dz==0))
// 	{return r1Sq;}
// 	else{
// 		int dxAbs = -abs(dx);
// 		int dyAbs = -abs(dy);
// 		int dzAbs = -abs(dz);
// 		for (int rSqInd = 0;rSqInd<m_numRadii;rSqInd++)
// 		{
// 			int rSq = distSqValues[rSqInd];
// 			int max = 0;
// 			int r = 1 + (int)sqrt((double)rSq);
// 			int scank,dk,scankj,dkji,iPlus ;
// 			for (int k=0;k<=r;k++)
// 			{
// 				scank = k*k;
// 				dk = (k-dzAbs)*(k-dzAbs);
// 				for (int j=0;j<=r;j++)
// 				{
// 					scankj = scank + j*j;
// 					if (scankj <= rSq)
// 					{
// 						iPlus = (int)sqrt((double)(rSq-scankj)) - dxAbs;
// 						dkji = dk+(j-dyAbs)*(j-dyAbs)+iPlus*iPlus;
// 						if (dkji>max)
// 							max = dkji;
// 					}
// 				}
// 			}
// 			r1Sq[rSqInd] = max;
// 		}
// 		return r1Sq;
// 	}
// }
// 
// 
// UINT SeBoneThickness::__BonejThicknessThread(LPVOID pParam)
// {
// 	ParamThickness* Parameter = (ParamThickness*)pParam;
// 	SeBoneThickness* pThis = Parameter->pSeBone;
// 	int nThread = Parameter->nThread;
// 	int*  nRidge = Parameter->nRidge;
// 	int ** iRidge = Parameter->iRidge;
// 	int ** jRidge = Parameter->jRidge;
// 	float** rRidge = Parameter->rRidge;
// 	float** s = Parameter->Edtdate;
// 	int m_nWidth = pThis->m_nWidth;
// 	int m_nHeight = pThis->m_nHeight;
// 	int m_nOriImagePiece = pThis->m_nOriImagePiece;
// 	
// 	for (int k = nThread; k < m_nOriImagePiece; k+=8)
// 	{
// 		int nR = nRidge[k];
// 		int* iRidgeK = iRidge[k];
// 		int* jRidgeK = jRidge[k];
// 		float* rRidgeK = rRidge[k];
// 
// 		int rInt;
// 		int iStart, iStop, jStart, jStop, kStart, kStop;
// 		float r1SquaredK, r1SquaredJK, r1Squared, s1;
// 		int rSquared;
// 		for (int iR = 0; iR < nR; iR++) {
// 			int i = iRidgeK[iR];
// 			int j = jRidgeK[iR];
// 			float r = rRidgeK[iR];
// 			rSquared = (int) (r * r + 0.5f);
// 			rInt = (int) r;
// 			if (rInt < r)
// 				rInt++;
// 			iStart = i - rInt;
// 			if (iStart < 0)
// 				iStart = 0;
// 			iStop = i + rInt;
// 			if (iStop >= m_nWidth)
// 				iStop = m_nWidth - 1;
// 			jStart = j - rInt;
// 			if (jStart < 0)
// 				jStart = 0;
// 			jStop = j + rInt;
// 			if (jStop >= m_nHeight)
// 				jStop = m_nHeight - 1;
// 			kStart = k - rInt;
// 			if (kStart < 0)
// 				kStart = 0;
// 			kStop = k + rInt;
// 			if (kStop >= m_nOriImagePiece)
// 				kStop = m_nOriImagePiece - 1;
// 			for (int k1 = kStart; k1 <= kStop; k1++) {
// 				r1SquaredK = (k1 - k) * (k1 - k);
// 				float* sk1 = s[k1];
// 				for (int j1 = jStart; j1 <= jStop; j1++) {
// 					int widthJ1 = m_nWidth * j1;
// 					r1SquaredJK = r1SquaredK + (j1 - j) * (j1 - j);
// 					if (r1SquaredJK <= rSquared) {
// 						for (int i1 = iStart; i1 <= iStop; i1++) {
// 							r1Squared = r1SquaredJK + (i1 - i)
// 								* (i1 - i);
// 							if (r1Squared <= rSquared) {
// 								int ind1 = i1 + widthJ1;
// 								EnterCriticalSection(Parameter->pCriticalLock);
// 								s1 = sk1[ind1];
// 								if (rSquared > s1) {
// 									s1 = sk1[ind1];
// 									if (rSquared > s1) {
// 										sk1[ind1] = rSquared;
// 									}
// 								}
// 								LeaveCriticalSection(Parameter->pCriticalLock);	
// 							}// if within sphere of DR point
// 						}// i1
// 					}// if k and j components within sphere of DR point
// 				}// j1
// 			}// k1
// 		}// iR
// 		Safe_DeleteVec(iRidge[k]);
// 		Safe_DeleteVec(jRidge[k]);
// 		Safe_DeleteVec(rRidge[k]);
// 	}// k
// 	return 0;
// }
// 
// 
// double SeBoneThickness::BoneJThickness(float** s)//, vector<Point>* vpRidgePoints)
// {
// 	int* nRidge = new int[m_nOriImagePiece];
// 	memset(nRidge,0,sizeof(int)*m_nOriImagePiece);
// 	int ind = 0, nr = 0, iR = 0;
// 	for (int k = 0; k < m_nOriImagePiece; k++) {
// 		float* sk = s[k];
// 		nr = 0;
// 		for (int j = 0; j < m_nHeight; j++) {
// 			int wj = m_nWidth * j;
// 			for (int i = 0; i < m_nWidth; i++) {
// 				ind = i + wj;
// 				if (sk[ind] > 0)
// 					nr++;
// 			}
// 		}
// 		nRidge[k] = nr;
// 	}
// 
// 	int** iRidge = new int*[m_nOriImagePiece];
// 	int** jRidge = new int*[m_nOriImagePiece];
// 	float** rRidge = new float*[m_nOriImagePiece];
// 
// 	for (int k = 0; k < m_nOriImagePiece; k++)
// 	{
// 		int nR = nRidge[k];
// 		iRidge[k] = new int[nR];
// 		jRidge[k] = new int[nR];
// 		rRidge[k] = new float[nR];
// 		int* iRidgeK = iRidge[k];
// 		int* jRidgeK = jRidge[k];
// 		float* rRidgeK = rRidge[k];
// 		float* sk = s[k];
// 		float sMax = 0;
// 		int iR = 0;
// 		for (int j = 0; j < m_nHeight; j++) {
// 			int wj = m_nWidth * j;
// 			for (int i = 0; i < m_nWidth; i++) {
// 				int ind = i + wj;
// 				if (sk[ind] > 0) {
// 					iRidgeK[iR] = i;
// 					jRidgeK[iR] = j;
// 					rRidgeK[iR++] = sk[ind];
// 					if (sk[ind] > sMax)
// 						sMax = sk[ind];
// 					sk[ind] = 0;
// 				}
// 			}
// 		}
// 	}
// 
// 	vector<CWinThread*> Cwin_thread;
// 	ParamThickness* NewParameter = new ParamThickness[THREAD_NUMBER];
// 	for (int i=0; i<THREAD_NUMBER; i++)
// 	{
// 		NewParameter[i].pSeBone = this;
// 		NewParameter[i].nThread = i;
// 		NewParameter[i].nRidge = nRidge;
// 		NewParameter[i].iRidge = iRidge;
// 		NewParameter[i].jRidge = jRidge;
// 		NewParameter[i].rRidge = rRidge;
// 		NewParameter[i].Edtdate = s;
// 		NewParameter[i].pCriticalLock = &pLock;
// 		Cwin_thread.push_back(AfxBeginThread(__BonejThicknessThread,&NewParameter[i]));
// 	}
// 	bool bDone = false;
// 	int  nThreadCount = 0;
// 	while(!bDone)
// 	{
// 		if(nThreadCount >= THREAD_NUMBER)
// 		{
// 			bDone = true;
// 			break;
// 		}
// 		if(WaitForSingleObject(Cwin_thread[nThreadCount]->m_hThread,0) == WAIT_OBJECT_0)
// 		{
// 			nThreadCount++;
// 			continue;
// 		}
// 		else
// 		{
// 			WaitForSingleObject(Cwin_thread[nThreadCount]->m_hThread,INFINITE);
// 			nThreadCount++;
// 		}
// 	}
// 	
// 	for (int k = 0; k < m_nOriImagePiece; k++) {
// 		float* sk = s[k];
// 		for (int j = 0; j < m_nHeight; j++) {
// 			int wj = m_nWidth * j;
// 			for (int i = 0; i < m_nWidth; i++) {
// 				ind = i + wj;
// 				sk[ind] = (float) (2 * sqrt(sk[ind]));
// 			}
// 		}
// 	}	
// 	Safe_DeleteVec(nRidge);
// 	return 0;
// }

// void SeBoneThickness::BackgroungToZero()
// {
// 	Safe_Delete(m_pStackData);
// 	m_pStackData = new float[m_nWidth * m_nHeight * m_nOriImagePiece];
// 	memset(m_pStackData, 0, sizeof(float) * m_nWidth * m_nHeight * m_nOriImagePiece);
// 	float* pOriHead = m_pStackData;
// 	for (int i=0; i<m_nOriImagePiece; i++)
// 	{
// 		short* pData = (short*)m_pProcessArray->GetDcmArray()[i]->GetData();
// 		short* pHead = pData;
// 		for (int j=0; j<m_nWidth * m_nHeight; j++)
// 		{
// 			*pOriHead++ = (*pHead > m_nMin && *pHead < m_nMax) ? (float)*pHead : 0.0;
// 			pHead++;
// 		}
// 	}
// }

FLOAT* SeBoneThickness::GetLocalThickness(BOOL bInverse)
{
	clock_t start, end1, end2, end3, end4, end5;
	start = clock();
	FLOAT** s = GeometryToDistanceMap(bInverse);
	end1 = clock();
	DistanceMaptoDistanceRidge(s);
	end2 = clock();
	DistanceRidgetoLocalThickness(s);
	end3 = clock();
	FLOAT* sNew = LocalThicknesstoCleanedUpLocalThickness(s);
	end4 = clock();
	FLOAT* result = MeanStdDev(sNew);
	end5 = clock();

	clock_t res1 = end1 - start;
	clock_t res2 = end2 - end1;
	clock_t res3 = end3 - end2;
	clock_t res4 = end4 - end3;
	clock_t res5 = end5 - end4;

	for (int i=0; i<m_nOriImagePiece; i++)
	{
		delete [] s[i];
	}
	delete [] s;
	delete [] sNew;

//	FLOAT* result = new FLOAT[3];
// 	result[0] = 0.0;
// 	result[1] = 0.0;
// 	result[2] = 0.0;
	return result;
	

}

FLOAT** SeBoneThickness::GeometryToDistanceMap(BOOL bInverse)
{
	const int w = m_nWidth;
	const int h = m_nHeight;
	const int d = m_nOriImagePiece;
	const int nThread = THREAD_NUMBER;
	FLOAT** s = new FLOAT*[d];
	for (int i=0; i<d; i++)
	{
		s[i] = new FLOAT[w * h];
		memset(s[i], 0, sizeof(FLOAT) * w * h);
	}

	vector <ThreadInfo_ForBoneThickness*> vecThread;

	for (int i=0; i<THREAD_NUMBER; i++)
	{
		ThreadInfo_ForBoneThickness* pParam = new ThreadInfo_ForBoneThickness(
			THREAD_NUMBER, i, w, h, d, m_nMin, m_nMax, FALSE, FALSE, m_pProcessArray, s);

		AfxBeginThread(__Step1Thread, pParam);
		vecThread.push_back(pParam);
	}

	while(TRUE)
	{
		BOOL bFinished = TRUE;
		for (int i=0; i<THREAD_NUMBER; i++)
		{
			if (vecThread[i]->bFinished == FALSE)
			{
				bFinished = FALSE;
				break;
			}
		}
		if (bFinished == FALSE)
		{
			Sleep(1000);
			continue;
		}
		break;
	}
	for (int i=0; i<THREAD_NUMBER; i++)
	{
		delete vecThread[i];
	}

	float max;
	max = 0.0f;
	for (int i=0; i<d; i++) {
		for (int j=0; j<w*h; j++) {
			if (s[i][j] > max) max = s[i][j];
		}
	}


	vector <ThreadInfo_ForBoneThickness2*> vecThread2;

	for (int i=0; i<THREAD_NUMBER; i++)
	{
		ThreadInfo_ForBoneThickness2* pParam = new ThreadInfo_ForBoneThickness2(
			THREAD_NUMBER, i, w, h, d, s, FALSE);

		AfxBeginThread(__Step2Thread, pParam);
		vecThread2.push_back(pParam);
	}

	while(TRUE)
	{
		BOOL bFinished = TRUE;
		for (int i=0; i<THREAD_NUMBER; i++)
		{
			if (vecThread2[i]->bFinished == FALSE)
			{
				bFinished = FALSE;
				break;
			}
		}
		if (bFinished == FALSE)
		{
			Sleep(1000);
			continue;
		}
		break;
	}
	for (int i=0; i<THREAD_NUMBER; i++)
	{
		delete vecThread2[i];
	}

	max = 0.0f;
	for (int i=0; i<d; i++) {
		for (int j=0; j<w*h; j++) {
			if (s[i][j] > max) max = s[i][j];
		}
	}

	vector <ThreadInfo_ForBoneThickness*> vecThread3;

	for (int i=0; i<THREAD_NUMBER; i++)
	{
		ThreadInfo_ForBoneThickness* pParam = new ThreadInfo_ForBoneThickness(
			THREAD_NUMBER, i, w, h, d, m_nMin, m_nMax, FALSE, FALSE, m_pProcessArray, s);

		AfxBeginThread(__Step3Thread, pParam);
		vecThread3.push_back(pParam);
	}

	while(TRUE)
	{
		BOOL bFinished = TRUE;
		for (int i=0; i<THREAD_NUMBER; i++)
		{
			if (vecThread3[i]->bFinished == FALSE)
			{
				bFinished = FALSE;
				break;
			}
		}
		if (bFinished == FALSE)
		{
			Sleep(1000);
			continue;
		}
		break;
	}
	for (int i=0; i<THREAD_NUMBER; i++)
	{
		delete vecThread3[i];
	}

	max = 0.0f;
	for (int i=0; i<d; i++) {
		for (int j=0; j<w*h; j++) {
			if (s[i][j] > max) max = s[i][j];
		}
	}

	float distMax = 0;
	float dist;
	float* pS;
	for (int i=0; i<d; i++)
	{
		pS = s[i];
		short* pData = (short*)m_pProcessArray->GetDcmArray()[i]->GetData();
		short* pNewData = NULL;
		short* pDataHead = NULL;
		if (!bInverse) {
			pNewData = new short[w * h];
			memcpy(pNewData, pData, sizeof(short) * w * h);
			pDataHead = pNewData;
			short* pDst = pNewData;
			short* pSrc = pData;
			for (int k=0; k<w * h; k++)
			{
				if (*pSrc++ <= 0)
					*pDst++ = m_nMax - 1;
				else
					pDst++;
			}
		}
		else {
			pDataHead = pData;
		}

		
		for (int j=0; j<w * h; j++)
		{
			if (!(*pDataHead < m_nMin || *pDataHead > m_nMax))
			{
				int x = 0;
			}
			if ((*pDataHead < m_nMin || *pDataHead > m_nMax) ^ bInverse)
			{
				*pS++ = 0.0;
			}
			else
			{
				dist = (FLOAT)sqrt(*pS);
				*pS++ = dist;
				distMax = (dist > distMax) ? dist : distMax;
			}
			pDataHead++;
		}
		
		if (pNewData != NULL) {
			delete [] pNewData;
		}
	}
	return s;
}

void SeBoneThickness::DistanceMaptoDistanceRidge(FLOAT** s)
{
	const int w = m_nWidth;
	const int h = m_nHeight;
	const int d = m_nOriImagePiece;

	FLOAT** sNew = new FLOAT*[d];
	for (int i=0; i<d; i++)
	{
		sNew[i] = new FLOAT[w * h];
		memset(sNew[i], 0, sizeof(FLOAT) * w * h);
	}

	int k1, j1, i1, dz, dy, dx;
	BOOL notRidgePoint;
	FLOAT* sk1;
	FLOAT* sk;
	FLOAT* skNew;
	int sk0Sq, sk0SqInd, sk1Sq;
	// Find the largest distance in the data
	float distMax = 0;
	for (int k = 0; k < d; k++) 
	{
		sk = s[k];
		for (int j = 0; j < h; j++) 
		{
			for (int i = 0; i < w; i++) 
			{
				if (*sk > distMax) distMax = *sk;
				sk++;
			}
		}
	}

	const int rSqMax = (int) (distMax * distMax + 0.5f) + 1;
	BOOL* occurs = new BOOL[rSqMax];
	memset(occurs, 0, sizeof(BOOL) * rSqMax);
// 	for (int i = 0; i < rSqMax; i++)
// 		occurs[i] = FALSE;
	for (int k = 0; k < d; k++) 
	{
		sk = s[k];
		for (int j = 0; j < h; j++) 
		{
			for (int i = 0; i < w; i++) 
			{
				occurs[(int) (*sk * *sk + 0.5f)] = TRUE;
				sk++;
			}
		}
	}

	int numRadii = 0;
	for (int i = 0; i < rSqMax; i++) 
	{
		if (occurs[i])
			numRadii++;
	}


	// Make an index of the distance-squared values
	int* distSqIndex = new int[rSqMax];
	int* distSqValues = new int[numRadii];
	int indDS = 0;
	for (int i = 0; i < rSqMax; i++) 
	{
		if (occurs[i]) 
		{
			distSqIndex[i] = indDS;
			distSqValues[indDS++] = i;
		}
	}

	int** rSqTemplate = CreateTemplate(distSqValues, numRadii);
	int numCompZ, numCompY, numCompX, numComp;
	for (int k = 0; k < d; k++) {
		// IJ.showProgress(k/(1.*d));
		theAppIVConfig.m_pILog->ProgressStepIt();
		sk = s[k];
		skNew = sNew[k];
		for (int j = 0; j < h; j++) 
		{
			int wj = m_nWidth * j;
			for (int i = 0; i < w; i++) 
			{
				int ind = i + wj;
				if (sk[ind] > 0) 
				{
					notRidgePoint = FALSE;
					sk0Sq = (int) (sk[ind] * sk[ind] + 0.5f);
					sk0SqInd = distSqIndex[sk0Sq];
					for (dz = -1; dz <= 1; dz++) 
					{
						k1 = k + dz;
						if ((k1 >= 0) && (k1 < d)) 
						{
							sk1 = s[k1];
							if (dz == 0) numCompZ = 0;
							else numCompZ = 1;
								
							for (dy = -1; dy <= 1; dy++) 
							{
								j1 = j + dy;
								int wj1 = m_nWidth * j1;
								if ((j1 >= 0) && (j1 < h)) 
								{
									if (dy == 0) numCompY = 0;
									else numCompY = 1;
										
									for (dx = -1; dx <= 1; dx++) 
									{
										i1 = i + dx;
										if ((i1 >= 0) && (i1 < w)) 
										{
											if (dx == 0) numCompX = 0;
											else numCompX = 1;

											numComp = numCompX + numCompY + numCompZ;
											if (numComp > 0) 
											{
												FLOAT sk1i1wj1 = sk1[i1 + wj1];
												sk1Sq = (int) (sk1i1wj1 * sk1i1wj1 + 0.5f);
												if (sk1Sq >= rSqTemplate[numComp - 1][sk0SqInd])
													notRidgePoint = TRUE;
											}
										} // if in grid for i1
										if (notRidgePoint)
											break;
									} // dx
								} // if in grid for j1
								if (notRidgePoint)
									break;
							} // dy
						} // if in grid for k1
						if (notRidgePoint)
							break;
					} // dz
					if (!notRidgePoint)
						skNew[ind] = sk[ind];
				} // if not in background
			} // i
		} // j
	} // k

	for (int i=0; i<3; i++)
	{
		delete [] rSqTemplate[i];
	}
	delete [] rSqTemplate;

	delete [] occurs;
	delete [] distSqIndex;
	delete [] distSqValues;
	for (int i=0; i<m_nOriImagePiece; i++)
	{
		delete [] sNew[i];
	}
	delete [] sNew;
}

void SeBoneThickness::DistanceRidgetoLocalThickness(FLOAT** s)
{
	const int w = m_nWidth;
	const int h = m_nHeight;
	const int d = m_nOriImagePiece;
	FLOAT* sk;
	// Count the distance ridge points on each slice
	int* nRidge = new int[d];
	int ind, nr, iR;
	for (int k = 0; k < d; k++) 
	{
		sk = s[k];
		nr = 0;
		for (int j = 0; j < h; j++) 
		{
			const int wj = w * j;
			for (int i = 0; i < w; i++) 
			{
				ind = i + wj;
				if (sk[ind] > 0)
					nr++;
			}
		}
		nRidge[k] = nr;
	}

	int** iRidge = new int*[d];
	int** jRidge = new int*[d];
	float** rRidge = new float*[d];
	// Pull out the distance ridge points
	int* iRidgeK; 
	int* jRidgeK;
	float* rRidgeK;
	float sMax = 0;
	for (int k = 0; k < d; k++) 
	{
		nr = nRidge[k];
		iRidge[k] = new int[nr];
		jRidge[k] = new int[nr];
		rRidge[k] = new float[nr];
		sk = s[k];
		iRidgeK = iRidge[k];
		jRidgeK = jRidge[k];
		rRidgeK = rRidge[k];
		iR = 0;
		for (int j = 0; j < h; j++) 
		{
			const int wj = w * j;
			for (int i = 0; i < w; i++) 
			{
				ind = i + wj;
				if (sk[ind] > 0.0) 
				{
					iRidgeK[iR] = i;
					jRidgeK[iR] = j;
					rRidgeK[iR++] = sk[ind];
					if (sk[ind] > sMax)
						sMax = sk[ind];
					sk[ind] = 0.0;
				}
			}
		}
	}

	vector <ThreadInfo_ForBoneThickness3*> vecThread4;
	const int nThreadNum = THREAD_NUMBER;
	for (int i=0; i<THREAD_NUMBER; i++)
	{
		ThreadInfo_ForBoneThickness3* pParam = new ThreadInfo_ForBoneThickness3(nThreadNum, i, w, h, d, s, nRidge, iRidge, jRidge, rRidge, FALSE);
		AfxBeginThread(__LTThread, pParam);
		vecThread4.push_back(pParam);
	}

	while(TRUE)
	{
		BOOL bFinished = TRUE;
		for (int i=0; i<THREAD_NUMBER; i++)
		{
			if (vecThread4[i]->bFinished == FALSE)
			{
				bFinished = FALSE;
				break;
			}
		}
		if (bFinished == FALSE)
		{
			Sleep(1000);
			continue;
		}
		break;
	}
	for (int i=0; i<THREAD_NUMBER; i++)
	{
		delete vecThread4[i];
	}
	 
	// Fix the square values and apply factor of 2
	for (int k = 0; k < d; k++) 
	{
		sk = s[k];
		for (int j = 0; j < h; j++) 
		{
			const int wj = w * j;
			for (int i = 0; i < w; i++) 
			{
				ind = i + wj;
				sk[ind] = (float) (2 * sqrt(sk[ind]));
			}
		}
	}

}

FLOAT* SeBoneThickness::LocalThicknesstoCleanedUpLocalThickness(FLOAT** s)
{
	const int w = m_nWidth;
	const int h = m_nHeight;
	const int d = m_nOriImagePiece;
	FLOAT* sNew = new FLOAT[w * h * d];
	memset(sNew, 0, sizeof(FLOAT) * w * h * d);
	/*
	* First set the output array to flags: 0 for a background point -1 for
	* a non-background point that borders a background point s (input data)
	* for an interior non-background point
	*/
	FLOAT* sNewHead = sNew;
	for (int k = 0; k < d; k++) 
	{
		for (int j = 0; j < h; j++) 
		{
			for (int i = 0; i < w; i++) 
			{
				try
				{
					*sNewHead++ = SetFlag(s, i, j, k, w, h, d);
				}
				catch (CMemoryException* e)
				{
					char* ch = new char[1000];
					e->GetErrorMessage(ch, 1000);
				}

			} // i
		} // j
	} // k

	/*
	* Process the surface points. Initially set results to negative values
	* to be able to avoid including them in averages of for subsequent
	* points. During the calculation, positive values in sNew are interior
	* non-background local thicknesses. Negative values are surface points.
	* In this case the value might be -1 (not processed yet) or -result,
	* where result is the average of the neighboring interior points.
	* Negative values are excluded from the averaging.
	*/
	sNewHead = sNew;
	for (int k = 0; k < d; k++) 
	{
		for (int j = 0; j < h; j++) 
		{
			for (int i = 0; i < w; i++) 
			{
				if (*sNewHead == -1) 
				{
					*sNewHead = -AverageInteriorNeighbors(s, sNew, i, j, k, w, h, d);
				}
				sNewHead++;
			} // i
		} // j
	} // k

	// Fix the negative values and double the results
	sNewHead = sNew;
	for (int k = 0; k < d; k++) 
	{
		for (int j = 0; j < h; j++) 
		{
			for (int i = 0; i < w; i++) 
			{
				*sNewHead = abs(*sNewHead);
				sNewHead++;
			} // i
		} // j
	} // k
	
	return sNew;
}

UINT SeBoneThickness::__Step1Thread(LPVOID pParam)
{
	ThreadInfo_ForBoneThickness* pInfo = (ThreadInfo_ForBoneThickness*)pParam;
	const int nThreadNum = pInfo->nThreadNum;
	const int nCurrentThread = pInfo->nCurrentThread;
	const int nWidth = pInfo->nWidth;
	const int nHeight = pInfo->nHeight;
	const int nSize = pInfo->nSize;
	const int nMin = pInfo->nMin;
	const int nMax = pInfo->nMax;
	BOOL bInverse = pInfo->bInverse;
	CDcmPicArray* pDcmArray = pInfo->pDcmArray;
	FLOAT** s = pInfo->s;

	int n = nWidth > nHeight ? nWidth : nHeight;
	n = n > nSize ? n : nSize;

	const int noResult = 3 * (n + 1) * (n + 1);

	BOOL* background = new BOOL[n];
	memset(background, 0, sizeof(BOOL) * n);

	int test, min;
	for (int i=nCurrentThread; i<nSize; i+=nThreadNum)
	{	
		theAppIVConfig.m_pILog->ProgressStepIt();
		short* pData = (short*) pDcmArray->GetDcmArray()[i]->GetData();	
		short* pDataHead = NULL;
		short* pNewData = NULL;
		if (!bInverse) {
			pNewData = new short[nWidth * nHeight];
			memcpy(pNewData, pData, sizeof(short) * nWidth * nHeight);
			pDataHead = pNewData;
			short* pDst = pNewData;
			short* pSrc = pData;
			for (int k=0; k<nWidth * nHeight; k++)
			{
				if (*pSrc++ <= 0)
					*pDst++ = nMax - 1;
				else
					pDst++;
			}
		}
		else {
			 pDataHead = pData;
		}

		float* pS = s[i];
		for (int j=0; j<nHeight; j++)
		{
			BOOL* pBkHead = background;
			for (int k=0; k<nWidth; k++)
			{	
				*pBkHead++ = ((*pDataHead < nMin || *pDataHead > nMax) ^ bInverse);
				if (((*pDataHead < nMin || *pDataHead > nMax) ^ bInverse) == TRUE)
				{
					int x = 0;
				}
				pDataHead++;
			}
			for (int k = 0; k < nWidth; k++) 
			{
				min = noResult;
				for (int x = k; x < nWidth; x++) 
				{
					if (background[x]) 
					{
						test = k - x;
						test *= test;
						min = test;
						break;
					}
				}
				for (int x = k - 1; x >= 0; x--) 
				{
					if (background[x]) 
					{
						test = k - x;
						test *= test;
						if (test < min)
							min = test;
						break;
					}
				}
				*pS++ = (FLOAT)min;
				if (min != 0)
				{
					int x = 0;
				}
			}

		}
		if (pNewData != NULL) {
			delete [] pNewData;
		}
	}

	delete [] (background);
	pInfo->bFinished = TRUE;

	return 0;
}

UINT SeBoneThickness::__Step2Thread(LPVOID pParam)
{
	ThreadInfo_ForBoneThickness2* pInfo = (ThreadInfo_ForBoneThickness2*)pParam;
	const int nThreadNum = pInfo->nThreadNum;
	const int nCurrentThread = pInfo->nCurrentThread;
	const int nWidth = pInfo->nWidth;
	const int nHeight = pInfo->nHeight;
	const int nSize = pInfo->nSize;
	FLOAT** s = pInfo->s;

	int n = nWidth > nHeight ? nWidth : nHeight;
	n = n > nSize ? n : nSize;

	const int noResult = 3 * (n + 1) * (n + 1);

	int* tempInt = new int[n];
	int* tempS = new int[n];
	BOOL nonEmpty;
	int test, min, delta;
	for (int i=nCurrentThread; i<nSize; i+=nThreadNum)
	{
		theAppIVConfig.m_pILog->ProgressStepIt();
		FLOAT* pS = s[i];
		for (int j=0; j<nWidth; j++)
		{
			nonEmpty = FALSE;
			for (int k=0; k<nHeight; k++)
			{
				tempS[k] = (int) pS[nWidth * k + j];
				if (tempS[k] > 0) nonEmpty = TRUE;
			}
			if (nonEmpty)
			{
				pS = s[i];
				for (int k=0; k<nHeight; k++)
				{
					min = noResult;
					delta = k;
					for (int y=0; y<nHeight; y++)
					{
						test = tempS[y] + delta * delta--;
						if (test < min) min = test;
					}
					tempInt[k] = min;
				}
				for (int k=0; k<nHeight; k++)
					pS[nWidth * k + j] = (FLOAT)tempInt[k];
 			}
		}
	}
	delete [] tempInt;
	delete [] tempS;
	pInfo->bFinished = TRUE;
	return 0;
}

UINT SeBoneThickness::__Step3Thread(LPVOID pParam)
{
	ThreadInfo_ForBoneThickness* pInfo = (ThreadInfo_ForBoneThickness*)pParam;
	const int nThreadNum = pInfo->nThreadNum;
	const int nCurrentThread = pInfo->nCurrentThread;
	const int nWidth = pInfo->nWidth;
	const int nHeight = pInfo->nHeight;
	const int nSize = pInfo->nSize;
	const int nMin = pInfo->nMin;
	const int nMax = pInfo->nMax;
	BOOL bInverse = pInfo->bInverse;
	CDcmPicArray* pDcmArray = pInfo->pDcmArray;
	FLOAT** s = pInfo->s;
	int zStart, zStop, zBegin, zEnd;

	BOOL* p = new BOOL[nWidth * nHeight * nSize];
	memset(p, 0, sizeof(BOOL) * nWidth * nHeight * nSize);
	BOOL* pHead = p;
	for (int i=0; i<nSize; i++)
	{
		short* pData = (short*)pDcmArray->GetDcmArray()[i]->GetData();

		short* pDataHead = NULL;
		short* pNewData = NULL;
		if (!bInverse) {
			pNewData = new short[nWidth * nHeight];
			memcpy(pNewData, pData, sizeof(short) * nWidth * nHeight);
			pDataHead = pNewData;
			short* pDst = pNewData;
			short* pSrc = pData;
			for (int k=0; k<nWidth * nHeight; k++)
			{
				if (*pSrc++ <= 0)
					*pDst++ = nMax - 1;
				else
					pDst++;
			}
		}
		else {
			pDataHead = pData;
		}

		for (int j=0; j<nWidth * nHeight; j++)
		{
			short data = *pDataHead;
			if (data > nMin && data < nMax)
				*pHead++ = TRUE;
			else
				*pHead++ = FALSE;
			pDataHead++;
		}
		
		if (pNewData != NULL) {
			delete [] pNewData;
		}
	}


	int n = nWidth > nHeight ? nWidth : nHeight;
	n = n > nSize ? n : nSize;

	const int noResult = 3 * (n + 1) * (n + 1);

	int* tempInt = new int[n];
	memset(tempInt, 0, sizeof(int) * n);
	int* tempS = new int[n];
	memset(tempS, 0, sizeof(int) * n);
	BOOL nonempty;
	int test, min, delta;
	for (int i=nCurrentThread; i<nHeight; i+= nThreadNum)
	{
		theAppIVConfig.m_pILog->ProgressStepIt();
		for (int j=0; j<nWidth; j++)
		{
			nonempty = FALSE;
			for (int k=0; k<nSize; k++)
			{
				tempS[k] = (int)s[k][nWidth * i + j];
				if (tempS[k] > 0)
					nonempty = TRUE;
			}
			if (nonempty)
			{
				zStart = 0;
				while ((zStart < (nSize - 1)) && (tempS[zStart] == 0)) zStart++;
				if (zStart > 0) zStart--;
				zStop = nSize - 1;
				while ((zStop > 0) && (tempS[zStop] == 0)) zStop--;
				if (zStop < (nSize - 1)) zStop++;

				for (int k=0; k<nSize; k++)
				{
					if (p[k*nWidth*nHeight + i*nWidth + j] ^ bInverse) 
					{
						min = noResult;
						zBegin = zStart;
						zEnd = zStop;
						if (zBegin > k) zBegin = k;
						if (zEnd < k) zEnd = k;
						delta = k - zBegin;
						for (int z = zBegin; z <= zEnd; z++) 
						{
							test = tempS[z] + delta * delta--;
							if (test < min)
								min = test;
							// min = (test < min) ? test : min;
						}
						tempInt[k] = min;
					}
				}
				for (int k=0; k<nSize; k++)
					s[k][i * nWidth + j] = tempInt[k];
			}
		}
	}
	delete [] p;
	delete [] tempInt;
	delete [] tempS;
	pInfo->bFinished = TRUE;
	return 0;
}

int** SeBoneThickness::CreateTemplate(int* distSqValues, int size)
{
	int** t = new int*[3];
	t[0] = ScanCube(1, 0, 0, distSqValues, size);
	t[1] = ScanCube(1, 1, 0, distSqValues, size);
	t[2] = ScanCube(1, 1, 1, distSqValues, size);
	return t;
}

int* SeBoneThickness::ScanCube(int dx, int dy, int dz, int* distSqValues, int size)
{
	const int numRadii = size;
	int* r1Sq = new int[numRadii];
	if ((dx == 0) && (dy == 0) && (dz == 0)) {
		for (int rSq = 0; rSq < numRadii; rSq++) {
			*r1Sq++ = INT_MAX;
		}
	} else {
		const int dxAbs = -1 * abs(dx);
		const int dyAbs = -1 * abs(dy);
		const int dzAbs = -1 * abs(dz);
		for (int rSqInd = 0; rSqInd < numRadii; rSqInd++) {
			const int rSq = distSqValues[rSqInd];
			int max = 0;
			const int r = 1 + (int)sqrt((float)rSq);
			int scank, scankj;
			int dk, dkji;
			// int iBall;
			int iPlus;
			for (int k = 0; k <= r; k++) {
				scank = k * k;
				dk = (k - dzAbs) * (k - dzAbs);
				for (int j = 0; j <= r; j++) {
					scankj = scank + j * j;
					if (scankj <= rSq) {
						iPlus = ((int) sqrt((float)(rSq - scankj))) - dxAbs;
						dkji = dk + (j - dyAbs) * (j - dyAbs) + iPlus * iPlus;
						if (dkji > max)
							max = dkji;
					}
				}
			}
			r1Sq[rSqInd] = max;
		}
	}
	return r1Sq;
}

UINT SeBoneThickness::__LTThread(LPVOID pParam)
{
	ThreadInfo_ForBoneThickness3* pInfo = (ThreadInfo_ForBoneThickness3*)pParam;
	const int nThreadNum = pInfo->nThreadNum;
	const int nCurrentThread = pInfo->nCurrentThread;
	const int nWidth = pInfo->nWidth;
	const int nHeight = pInfo->nHeight;
	const int nSize = pInfo->nSize;
	FLOAT** s = pInfo->s;
	int* nRidge = pInfo->nRidge;
	int** iRidge = pInfo->iRidge; 
	int** jRidge = pInfo->jRidge;
	FLOAT** rRidge = pInfo->rRidge;
	FLOAT* sk1;
	int rInt;
	int iStart, iStop, jStart, jStop, kStart, kStop;
	float r1SquaredK, r1SquaredJK, r1Squared, s1;
	int rSquared;
	for (int k = nCurrentThread; k < nSize; k += nThreadNum) 
	{
		theAppIVConfig.m_pILog->ProgressStepIt();
		const int nR = nRidge[k];
		int* iRidgeK = iRidge[k];
		int* jRidgeK = jRidge[k];
		float* rRidgeK = rRidge[k];
		for (int iR = 0; iR < nR; iR++) {
			const int i = iRidgeK[iR];
			const int j = jRidgeK[iR];
			const float r = rRidgeK[iR];
			rSquared = (int) (r * r + 0.5f);
			rInt = (int) r;
			if (rInt < r)
				rInt++;

			iStart = ((i - rInt) < 0) ? 0 : (i - rInt);
//			iStart = i - rInt;
// 			if (iStart < 0)
// 				iStart = 0;
			iStop =  ((i + rInt) >= nWidth) ? (nWidth - 1) : (i + rInt);
//			iStop = i + rInt;
// 			if (iStop >= nWidth)
// 				iStop = nWidth - 1;
			jStart = ((j - rInt) < 0) ? 0 : (j - rInt);
// 			jStart = j - rInt;
// 			if (jStart < 0)
// 				jStart = 0;
			jStop =  ((j + rInt) >= nHeight) ? (nHeight - 1) : (j + rInt);
// 			jStop = j + rInt;
// 			if (jStop >= nHeight)
// 				jStop = nHeight - 1;
			kStart = ((k - rInt) < 0) ? 0 : (k - rInt);
// 			kStart = k - rInt;
// 			if (kStart < 0)
// 				kStart = 0;
			kStop =  ((k + rInt) >= nSize) ? (nSize - 1) : (k + rInt);
// 			kStop = k + rInt;
// 			if (kStop >= nSize)
// 				kStop = nSize - 1;
			for (int k1 = kStart; k1 <= kStop; k1++) 
			{
				r1SquaredK = (k1 - k) * (k1 - k);
				sk1 = s[k1];
				for (int j1 = jStart; j1 <= jStop; j1++) 
				{
					const int widthJ1 = nWidth * j1;
					r1SquaredJK = r1SquaredK + (j1 - j) * (j1 - j);
					if (r1SquaredJK <= rSquared) 
					{
						for (int i1 = iStart; i1 <= iStop; i1++) 
						{
							r1Squared = r1SquaredJK + (i1 - i) * (i1 - i);
							if (r1Squared <= rSquared) 
							{
								const int ind1 = i1 + widthJ1;
								s1 = sk1[ind1];
								if (rSquared > s1) 
								{
									s1 = sk1[ind1];
									if (rSquared > s1) sk1[ind1] = rSquared;
								}
							} // if within sphere of DR point
						} // i1
					} // if k and j components within sphere of DR point
				} // j1
			} // k1
		} // iR
	} // k
	pInfo->bFinished = TRUE;
	return 0;
}

FLOAT SeBoneThickness::SetFlag(float** s, const int i, const int j, const int k, const int w, const int h, const int d)
{
	if (s[k][i + w * j] == 0)
		return 0;
	// change 1
	if (Look(s, i, j, k - 1, w, h, d) == 0)
		return -1;
	if (Look(s, i, j, k + 1, w, h, d) == 0)
		return -1;
	if (Look(s, i, j - 1, k, w, h, d) == 0)
		return -1;
	if (Look(s, i, j + 1, k, w, h, d) == 0)
		return -1;
	if (Look(s, i - 1, j, k, w, h, d) == 0)
		return -1;
	if (Look(s, i + 1, j, k, w, h, d) == 0)
		return -1;
	// change 1 before plus
	if (Look(s, i, j + 1, k - 1, w, h, d) == 0)
		return -1;
	if (Look(s, i, j + 1, k + 1, w, h, d) == 0)
		return -1;
	if (Look(s, i + 1, j - 1, k, w, h, d) == 0)
		return -1;
	if (Look(s, i + 1, j + 1, k, w, h, d) == 0)
		return -1;
	if (Look(s, i - 1, j, k + 1, w, h, d) == 0)
		return -1;
	if (Look(s, i + 1, j, k + 1, w, h, d) == 0)
		return -1;
	// change 1 before minus
	if (Look(s, i, j - 1, k - 1, w, h, d) == 0)
		return -1;
	if (Look(s, i, j - 1, k + 1, w, h, d) == 0)
		return -1;
	if (Look(s, i - 1, j - 1, k, w, h, d) == 0)
		return -1;
	if (Look(s, i - 1, j + 1, k, w, h, d) == 0)
		return -1;
	if (Look(s, i - 1, j, k - 1, w, h, d) == 0)
		return -1;
	if (Look(s, i + 1, j, k - 1, w, h, d) == 0)
		return -1;
	// change 3, k+1
	if (Look(s, i + 1, j + 1, k + 1, w, h, d) == 0)
		return -1;
	if (Look(s, i + 1, j - 1, k + 1, w, h, d) == 0)
		return -1;
	if (Look(s, i - 1, j + 1, k + 1, w, h, d) == 0)
		return -1;
	if (Look(s, i - 1, j - 1, k + 1, w, h, d) == 0)
		return -1;
	// change 3, k-1
	if (Look(s, i + 1, j + 1, k - 1, w, h, d) == 0)
		return -1;
	if (Look(s, i + 1, j - 1, k - 1, w, h, d) == 0)
		return -1;
	if (Look(s, i - 1, j + 1, k - 1, w, h, d) == 0)
		return -1;
	if (Look(s, i - 1, j - 1, k - 1, w, h, d) == 0)
		return -1;
	return s[k][i + w * j];
}

FLOAT SeBoneThickness::Look(float** s, const int i, const int j, const int k, const int w, const int h, const int d)
{		
	if ((i < 0) || (i >= w) || (j < 0) || (j >= h) || (k < 0) || (k >= d))
		return -1;
	return s[k][i + w * j];
}

FLOAT SeBoneThickness::LookNew(FLOAT* sNew, const int i, const int j, const int k, const int w, const int h, const int d)
{
	if ((i < 0) || (i >= w) || (j < 0) || (j >= h) || (k < 0) || (k >= d))
		return -1;
	return sNew[k * w * h + i + w * j];
}

FLOAT SeBoneThickness::AverageInteriorNeighbors(FLOAT** s, FLOAT* sNew, const int i, const int j, const int k, const int w, const int h, const int d)
{
	int n = 0;
	float sum = 0;
	// change 1
	float value = LookNew(sNew, i, j, k - 1, w, h, d);
	if (value > 0) 
	{
		n++;
		sum += value;
	}
	value = LookNew(sNew, i, j, k + 1, w, h, d);
	if (value > 0) 
	{
		n++;
		sum += value;
	}
	value = LookNew(sNew, i, j - 1, k, w, h, d);
	if (value > 0) 
	{
		n++;
		sum += value;
	}
	value = LookNew(sNew, i, j + 1, k, w, h, d);
	if (value > 0) 
	{
		n++;
		sum += value;
	}
	value = LookNew(sNew, i - 1, j, k, w, h, d);
	if (value > 0) 
	{
		n++;
		sum += value;
	}
	value = LookNew(sNew, i + 1, j, k, w, h, d);
	if (value > 0) 
	{
		n++;
		sum += value;
	}
	// change 1 before plus
	value = LookNew(sNew, i, j + 1, k - 1, w, h, d);
	if (value > 0) 
	{
		n++;
		sum += value;
	}
	value = LookNew(sNew, i, j + 1, k + 1, w, h, d);
	if (value > 0) 
	{
		n++;
		sum += value;
	}
	value = LookNew(sNew, i + 1, j - 1, k, w, h, d);
	if (value > 0) 
	{
		n++;
		sum += value;
	}
	value = LookNew(sNew, i + 1, j + 1, k, w, h, d);
	if (value > 0) 
	{
		n++;
		sum += value;
	}
	value = LookNew(sNew, i - 1, j, k + 1, w, h, d);
	if (value > 0) 
	{
		n++;
		sum += value;
	}
	value = LookNew(sNew, i + 1, j, k + 1, w, h, d);
	if (value > 0) 
	{
		n++;
		sum += value;
	}
	// change 1 before minus
	value = LookNew(sNew, i, j - 1, k - 1, w, h, d);
	if (value > 0) 
	{
		n++;
		sum += value;
	}
	value = LookNew(sNew, i, j - 1, k + 1, w, h, d);
	if (value > 0) 
	{
		n++;
		sum += value;
	}
	value = LookNew(sNew, i - 1, j - 1, k, w, h, d);
	if (value > 0) 
	{
		n++;
		sum += value;
	}
	value = LookNew(sNew, i - 1, j + 1, k, w, h, d);
	if (value > 0) 
	{
		n++;
		sum += value;
	}
	value = LookNew(sNew, i - 1, j, k - 1, w, h, d);
	if (value > 0) 
	{
		n++;
		sum += value;
	}
	value = LookNew(sNew, i + 1, j, k - 1, w, h, d);
	if (value > 0) 
	{
		n++;
		sum += value;
	}
	// change 3, k+1
	value = LookNew(sNew, i + 1, j + 1, k + 1, w, h, d);
	if (value > 0) 
	{
		n++;
		sum += value;
	}
	value = LookNew(sNew, i + 1, j - 1, k + 1, w, h, d);
	if (value > 0) 
	{
		n++;
		sum += value;
	}
	value = LookNew(sNew, i - 1, j + 1, k + 1, w, h, d);
	if (value > 0) 
	{
		n++;
		sum += value;
	}
	value = LookNew(sNew, i - 1, j - 1, k + 1, w, h, d);
	if (value > 0) 
	{
		n++;
		sum += value;
	}
	// change 3, k-1
	value = LookNew(sNew, i + 1, j + 1, k - 1, w, h, d);
	if (value > 0) 
	{
		n++;
		sum += value;
	}
	value = LookNew(sNew, i + 1, j - 1, k - 1, w, h, d);
	if (value > 0) 
	{
		n++;
		sum += value;
	}
	value = LookNew(sNew, i - 1, j + 1, k - 1, w, h, d);
	if (value > 0) 
	{
		n++;
		sum += value;
	}
	value = LookNew(sNew, i - 1, j - 1, k - 1, w, h, d);
	if (value > 0) 
	{
		n++;
		sum += value;
	}
	if (n > 0)
		return sum / n;
	return s[k][i + w * j];
}

FLOAT* SeBoneThickness::MeanStdDev(FLOAT* sNew)
{
	const int w = m_nWidth;
	const int h = m_nHeight;
	const int d = m_nOriImagePiece;
	double pixCount = 0;
	double sumThick = 0;
	double maxThick = 0;

	float* result = new float[3];
	memset(result,0,sizeof(float)*3);
	FLOAT* pHead = sNew;
	for (int i = 0; i < w * h * d; i++) 
	{
		if (*pHead > 0) 
		{
			sumThick += *pHead;
			maxThick = maxThick > *pHead ? maxThick : *pHead; 
			pixCount++;
		}
		pHead++;
	}
	float meanThick = sumThick / pixCount;

	double sumSquares = 0;
	pHead = sNew;
	for (int i = 0; i < w * h * d; i++) 
	{
		if (*pHead > 0) 
		{
			double residual = meanThick - *pHead;
			sumSquares += residual * residual;
		}
		pHead++;
	}

	double stDev = sqrt(sumSquares / pixCount);	

	result[0] = meanThick;
	result[1] = stDev;
	result[2] = maxThick;

	return result;
}
