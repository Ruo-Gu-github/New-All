#include "stdafx.h"
#include "SeBoneParamCaculate.h"
#include "SeBoneParamBasic.h"
#include "SeBoneParamAnistrophy.h"
#include "SeBoneParamConnectivity.h"
#include "SeBoneParamThickness.h"
#include "SeBoneParamSMI.h"
#include "SeBoneParaTBPf.h"

// for new parameter calculator
#include "ruogu_Calculator.h"
#include "ruogu_ImageStack.h"
#include "ruogu_BasicCalculator.h"
#include "ruogu_DensityCalculator.h"
#include "ruogu_SMICalculator.h"
#include "ruogu_BoneThicknessCalculator.h"
#include "ruogu_AnisotropyCalculator.h"
#include "ruogu_ConnectivityCalculator.h"
#include "ruogu_BoneThicknessSpacingCalculator.h"


SeBoneParamCaculate::SeBoneParamCaculate()
{
	m_bThickness = FALSE;
	m_bCortical = FALSE;

	m_nSmallValue = 0;
	m_nBigValue = 0;
	m_fSmallDensity = 0.0;
	m_fBigDensity = 0.0;

	m_dTotleVolume = 0.0;
	m_dVolume = 0.0;
	m_dSurfaceArea = 0.0;
	m_dSpecificSurfaceArea = 0.0;
	m_dVolumeFraction = 0.0;
	m_dSurfaceDensity = 0.0;
	m_dCentroidX = 0.0;
	m_dCentroidY = 0.0;
	m_dCentroidZ = 0.0;

	m_dStructureThickness = 0.0;
	m_dThicknessMax = 0.0;
	m_dThicknessStdDev = 0.0;
	m_dSeparation = 0.0;
	m_dSeparationStdDev = 0.0;
	m_dSeparationMax = 0.0;
	m_dNumberOfBone = 0.0;
	m_dEulerNumber = 0.0;
	m_dConnectivity = 0.0;
	m_dConnectivityDensity = 0.0;
	m_dDegreeOfAnisotropy = 0.0;
	m_dEigenValue1 = 0.0;
	m_dEigenValue2 = 0.0;
	m_dEigenValue3 = 0.0;
	m_dSMI = 0.0;
	m_dTBPf = 0.0;
	m_dDensity = 0.0;
	m_dContent = 0.0;
	m_dMeanValue = 0.0;
	m_dDensityTotal = 0.0;
	m_dMeanValueTotal = 0.0;

	m_dCorticalTotalArea = 0.0;
	m_dCorticalArea = 0.0;
	m_dCorticalAreaFraction = 0.0;
	m_dCorticalThickness = 0.0;
	m_dMedullaryArea = 0.0;
	m_dCorticalDensity = 0.0;
	m_dCorticalContent = 0.0;

	m_pDcmArray = NULL;
	m_nWidth = 0;
	m_nHeight = 0;
	m_nSize = 0;
	m_nMinValue = 0; // 图片最小值
	m_nMin = 0; // 二值化最小值
	m_nMax = 0; // 二值化最大值
	m_dMMperPixelXY = 0.0;
	m_dMMperPixelZ = 0.0;

	m_nReWidth = 0;
	m_nReHeight = 0;
	m_nReSize = 0;
	m_dblRePixelXY = 0.0;
	m_dblRePixelZ = 0.0;
}

SeBoneParamCaculate::~SeBoneParamCaculate()
{
	m_pDcmArray = NULL;
	m_nWidth = 0;
	m_nHeight = 0;
	m_nSize = 0;
	m_nMinValue = 0; // 图片最小值
	m_nMin = 0; // 二值化最小值
	m_nMax = 0; // 二值化最大值
	m_dMMperPixelXY = 0.0;
	m_dMMperPixelZ = 0.0;
}


void SeBoneParamCaculate::CaculateChoicedParam(CDcmPicArray* pDcmArray, int nMin, int nMax, int nMinValue)
{
	assert(pDcmArray->GetDcmArray().size() > 0);

	SYSTEM_INFO sysInfo;
	GetSystemInfo( &sysInfo );
	const int thread_number = sysInfo.dwNumberOfProcessors > 1 ? sysInfo.dwNumberOfProcessors : 1;

	int images_number = pDcmArray->GetZDcmPicCount();
	int images_width = pDcmArray->GetDcmArray()[0]->GetWidth();

	int nSteps = images_number;
	if (m_bBasicParameter)
		nSteps += (images_number * 2 + 10 * thread_number + thread_number);
	if (m_bBoneThick)
		nSteps += (images_number * 5 + images_width + 10 * thread_number + thread_number);
	if (m_bSpaceThick)
		nSteps += (images_number * 5 + images_width + 10 * thread_number + thread_number);
	if (m_dNumberOfBone)
		nSteps += 1;
	if (m_bConnectivity)
		nSteps += images_number + 6;
	if (m_bSMI)
		nSteps += 0;
	if (m_bTBPf)
		nSteps += 0;
	if (m_bDA)
		nSteps += 0;
	if (m_bDensity)
		nSteps += 10 * thread_number + thread_number;
	if (m_bCorticalArea)
		nSteps += 1;
	if (m_bCorticalFraction)
		nSteps += 1;
	if (m_bCorticalThick)
		nSteps += (images_number * 5 + images_width + 10 * thread_number + thread_number);;
	if (m_bMedullaryArea)
		nSteps += (images_number * 2 + 10 * thread_number + thread_number);
	if (m_bCorticalDensity)
		nSteps += 10 * thread_number + thread_number;

	if (m_bBoneThick || m_bSpaceThick || m_bSMI || m_bDA || m_bDensity || m_bConnectivity)
	{
		m_bThickness = TRUE;
	} else {
		m_bThickness = FALSE;
	}

	if (m_bCorticalArea || m_bCorticalThick || m_bCorticalFraction || m_bMedullaryArea || m_bCorticalDensity) {
		m_bCortical = TRUE;
	} else {
		m_bCortical = FALSE;
	}

	nSteps += (nSteps / 500);
	theAppIVConfig.m_pILog->ProgressInit(nSteps);
	theAppIVConfig.m_pILog->LogMessage(_T("正在加载图像数据......"));

	vector<short> data;
	size_t width = pDcmArray->GetDcmArray()[0]->GetWidth();
	size_t height = pDcmArray->GetDcmArray()[0]->GetHeight();
	size_t length = pDcmArray->GetZDcmPicCount();

	data.reserve(width * height * length);
	for (int i=0; i<pDcmArray->GetDcmArray().size(); i++) {
		theAppIVConfig.m_pILog->ProgressStepIt();
		short* slice = (short*)pDcmArray->GetDcmArray()[i]->GetData();
		data.insert(data.end(), slice, slice + height * width);
	}
	shared_ptr<ImageStack> image_stack(new ImageStack(data, width, height, length, nMin, nMax, nMinValue));

	image_stack->pixel_size_ = pDcmArray->GetMMPerXYPixel();
	image_stack->pixel_spacing_ = pDcmArray->GetMMPerZPixel();
	image_stack->low_density_ = m_fSmallDensity;
	image_stack->high_density_ = m_fBigDensity;
	image_stack->low_ct_value_for_density_ = m_nSmallValue;
	image_stack->high_ct_value_for_density_ = m_nBigValue;

	map<string, double> result;

	if (m_bBasicParameter) {
		theAppIVConfig.m_pILog->LogMessage(_T("正在计算基础参数......"));
		BasicCalculator basic;
		basic.Calculation(image_stack, result);
	}

	if (m_bBoneThick) {
		theAppIVConfig.m_pILog->LogMessage(_T("正在计算骨小梁厚度......"));
		BoneThicknessCalculator thickness;
		thickness.Calculation(image_stack, result);
	}

	if (m_bSpaceThick) {
		theAppIVConfig.m_pILog->LogMessage(_T("正在计算骨小梁间隙......"));
		BoneThicknessSpacingCalculator spacing;
		spacing.Calculation(image_stack, result);
	}

	if (m_bBoneNumber) {
		theAppIVConfig.m_pILog->LogMessage(_T("正在计算骨小梁数量......"));
		double tbn = 1.0f / (result["Tb.th mean"] + result["Tb.sp mean"]);
		result.insert(pair<string, double>("Tb.num", tbn));
		theAppIVConfig.m_pILog->ProgressStepIt();
	}

	if (m_bConnectivity) {
		theAppIVConfig.m_pILog->LogMessage(_T("正在计算连通性......"));
		ConnectivityCalculator connectivity;
		connectivity.Calculation(image_stack, result);		
	}

	if (m_bDA) {
		theAppIVConfig.m_pILog->LogMessage(_T("正在计算各向异性......"));
		AnisotropyCalculator anisotropy;
		anisotropy.Calculation(image_stack, result);
	}

	if (m_bDensity) {
		theAppIVConfig.m_pILog->LogMessage(_T("正在计算密度......"));
		DensityCalculator density;
		density.Calculation(image_stack, result);
		double dBMC = result["BMD"] * result["BV"];
		result.insert(pair<string, double>("BMC", dBMC));
	}

	if (m_bCorticalArea) {
		theAppIVConfig.m_pILog->LogMessage(_T("正在皮质骨面积......"));
		double dCorticalArea = result["TV"] / (image_stack->pixel_spacing_ * images_number);
		result.insert(pair<string, double>("Tt.Ar", result["BS"]));
		result.insert(pair<string, double>("Ct.Ar", dCorticalArea));
	}

	if (m_bCorticalThick) {
		theAppIVConfig.m_pILog->LogMessage(_T("正在计算皮质骨厚度......"));
		BoneThicknessCalculator thickness;
		thickness.Calculation(image_stack, result);
		double thick = result["Tb.th mean"];
		result.insert(pair<string, double>("Ct.Th", result["Tb.th mean"]));
	}

	if (m_bCorticalFraction) {
		theAppIVConfig.m_pILog->LogMessage(_T("正在计算皮质骨面积比......"));
		double dCorticalFraction = (result["Ct.Ar"] / result["Tt.Ar"]) * 100.0;
		result.insert(pair<string, double>("Ct.Ar/Tt.Ar", dCorticalFraction));
	}

	if (m_bMedullaryArea) {
		theAppIVConfig.m_pILog->LogMessage(_T("正在计算骨髓腔面积......"));
		double dMedullaryArea = m_dEmptyVolume * image_stack->pixel_size_ * image_stack->pixel_size_ / images_number;
		result.insert(pair<string, double>("Ma.Ar", dMedullaryArea));
	}

	if (m_bCorticalDensity) {
		theAppIVConfig.m_pILog->LogMessage(_T("正在计算皮质骨密度......"));
		DensityCalculator density;
		density.Calculation(image_stack, result);
		result.insert(pair<string, double>("CBMD", result["BMD"]));
		double dCBMC = result["BMD"] * result["BV"];
		result.insert(pair<string, double>("CBMC", dCBMC));
	}

	theAppIVConfig.m_pILog->ProgressClose();

	ResetAllResult();

 	m_dTotleVolume = result["TV"];
	m_dVolume =  result["BV"];
	m_dSurfaceArea = result["BS"];
	m_dSpecificSurfaceArea = result["BS/BV"];
	m_dVolumeFraction = result["BV/TV"];
	m_dSurfaceDensity = result["BS/TV"];
	m_dCentroidX = result["Center.X"];
 	m_dCentroidY = result["Center.Y"];
	m_dCentroidZ = result["Center.Z"];
	m_dStructureThickness = result["Tb.th mean"];
 	m_dThicknessStdDev =  result["Tb.th std"];
 	m_dThicknessMax = result["Tb.th max"];
 	m_dSeparation = result["Tb.sp mean"];
 	m_dSeparationStdDev =  result["Tb.sp std"];
 	m_dSeparationMax = result["Tb.sp max"];
	m_dNumberOfBone = result["Tb.num"];
 	m_dEulerNumber = result["Euler number"];
 	m_dConnectivity = result["Conn"];
 	m_dConnectivityDensity = result["Conn.Dn"];
 	m_dMeanValue = result["Average"];
 	m_dDensity = result["BMD"];
	m_dContent = result["BMC"];
	m_dDensityTotal = result["DensityTotal"];
	m_dMeanValueTotal = result["AverageTotal"];
	m_dCorticalTotalArea = result["Tt.Ar"];
	m_dCorticalArea = result["Ct.Ar"];
	m_dCorticalAreaFraction = result["Ct.Ar/Tt.Ar"];
	m_dCorticalThickness = result["Ct.Th"];
	m_dMedullaryArea = result["Ma.Ar"];
	m_dCorticalDensity = result["CBMD"];
	m_dCorticalContent = result["CBMC"];

	if (m_bDA) {
		map<string, double>::const_iterator it = result.find("DA");
		if (it != result.end()) {
			m_dDegreeOfAnisotropy = it->second;
		}
		it = result.find("DA.Eigenvalue1");
		if (it != result.end()) {
			m_dEigenValue1 = it->second;
		}
		it = result.find("DA.Eigenvalue2");
		if (it != result.end()) {
			m_dEigenValue2 = it->second;
		}
		it = result.find("DA.Eigenvalue3");
		if (it != result.end()) {
			m_dEigenValue3 = it->second;
		}
	}
	ConvertResult();
}


void SeBoneParamCaculate::CalculateArea()
{
	short* slicefront = new short[m_nHeight * m_nWidth];
	for (int j=0;j<m_nWidth * m_nHeight;j++)
	{
		slicefront[j] = m_nMinValue;
	}
	short* slice1 = NULL;
	short* slice2 = NULL;

 	SeBoneParameter* Param = new SeBoneParameter(m_pDcmArray, m_nMinValue, m_nMin, m_nMax, m_nWidth, m_nHeight, m_nSize);
	Param->GenerateTriangleCount();
	for(int i = -1; i < m_nSize; i++)     //calculate the area of bone surface
	{
		theAppIVConfig.m_pILog->ProgressStepIt();
		if (i != -1 && i != m_nSize - 1)
		{
			slice1 = (short*)m_pDcmArray->GetDcmArray()[i]->GetData();
			slice2 = (short*)m_pDcmArray->GetDcmArray()[i+1]->GetData();
		}
		else if (i == -1)
		{
			slice1 = slicefront;
			slice2 = (short*)m_pDcmArray->GetDcmArray()[i+1]->GetData();
		}
		else if (i == m_nSize - 1)
		{
			slice1 = (short*)m_pDcmArray->GetDcmArray()[i]->GetData();
			slice2 = slicefront;
		}
		Param->CreateCubes(i, m_nWidth, m_nHeight, slice1, slice2);
		CBasicCube*		pCube = Param->GetAllCubes();
		int	 nCubeCount = Param->GetCubeCount();
		int nCount = 0;
		for (int j = 0; j < nCubeCount; j++)
		{
			int		nCubeIndex = pCube[j].GetVertexIndex();
			int		nTri = Param->s_TriangleCountMetrix[nCubeIndex];
			if (nTri == 0)
			{continue;}
			CTriangle *pTri = new CTriangle[nTri];
			Param->Polygonise(pCube[j].GetCubeVertex(), pTri, nTri, nCubeIndex);//获取每个立方体的三角面片
			for (int k = 0; k < nTri; k++)
			{
				m_vTriangle.push_back(pTri[k]);
				nCount++;
			}
			Safe_DeleteVec(pTri);			
		}
		//Safe_DeleteVec(pCube);
		nCount += 1;
	}
	Safe_Delete(Param);
	Safe_DeleteVec(slicefront);

	double totalarea = 0.0;
	for (int i = 0; i < (int)m_vTriangle.size(); i++)      //calculate total triangle area
	{
		double a1=(m_vTriangle[i].m_point[0].fx-m_vTriangle[i].m_point[1].fx);
		double a2=(m_vTriangle[i].m_point[0].fy-m_vTriangle[i].m_point[1].fy);
		double a3=(m_vTriangle[i].m_point[0].fz-m_vTriangle[i].m_point[1].fz);
		double line1=sqrt(a1*a1+a2*a2+a3*a3);
		double b1=(m_vTriangle[i].m_point[1].fx-m_vTriangle[i].m_point[2].fx);
		double b2=(m_vTriangle[i].m_point[1].fy-m_vTriangle[i].m_point[2].fy);
		double b3=(m_vTriangle[i].m_point[1].fz-m_vTriangle[i].m_point[2].fz);
		double line2=sqrt(b1*b1+b2*b2+b3*b3);
		double c1=(m_vTriangle[i].m_point[0].fx-m_vTriangle[i].m_point[2].fx);
		double c2=(m_vTriangle[i].m_point[0].fy-m_vTriangle[i].m_point[2].fy);
		double c3=(m_vTriangle[i].m_point[0].fz-m_vTriangle[i].m_point[2].fz);
		double line3=sqrt(c1*c1+c2*c2+c3*c3);
		double p=(line1+line2+line3)/2;
		totalarea += sqrt(p*(p-line1)*(p-line2)*(p-line3));
	}
	m_dSurfaceArea = totalarea * m_dMMperPixelXY * m_dMMperPixelXY;
	Safe_Delete(Param);
}

void SeBoneParamCaculate::CalculateVolume()
{
	LONG nCountTotle = 0;
	LONG nCountBone = 0;
	for (int i=0; i<m_nSize; i++)
	{
		short* pData = (short*)m_pDcmArray->GetDcmArray()[i]->GetData();
		theAppIVConfig.m_pILog->ProgressStepIt();
		short* pDataHead = pData;
		for (int j=0; j<m_nWidth*m_nHeight; j++)
		{
			short sValue = *pDataHead++;
			if ( sValue> m_nMin && sValue < m_nMax)
				nCountBone++;
			if (sValue > m_nMinValue)
				nCountTotle++;
		}
	}
	m_dTotleVolume = double(nCountTotle) * m_dMMperPixelXY * m_dMMperPixelXY * m_dMMperPixelZ;
	m_dVolume =  double(nCountBone) * m_dMMperPixelXY * m_dMMperPixelXY * m_dMMperPixelZ; 
}



void SeBoneParamCaculate::CalculateBoneThick()
{
	SeBoneThickness* pThickness = new SeBoneThickness(m_pDcmArray, m_nMinValue, m_nMin, m_nMax, m_nWidth, m_nHeight, m_nSize);
	FLOAT* pResult = pThickness->GetLocalThickness(TRUE);
	m_dStructureThickness = pResult[0] * m_dMMperPixelXY;
	m_dThicknessStdDev =  pResult[1] * m_dMMperPixelXY;
	m_dThicknessMax = pResult[2] * m_dMMperPixelXY;
	delete [] pResult;
}

void SeBoneParamCaculate::CalculateSpaceThick()
{
	SeBoneThickness* pThickness = new SeBoneThickness(m_pDcmArray, m_nMinValue, m_nMin, m_nMax, m_nWidth, m_nHeight, m_nSize);
	FLOAT* pResult = pThickness->GetLocalThickness(FALSE);
 	m_dSeparation = pResult[0] * m_dMMperPixelXY;
 	m_dSeparationStdDev =  pResult[1] * m_dMMperPixelXY;
 	m_dSeparationMax = pResult[2] * m_dMMperPixelXY;
	delete [] pResult;
}

void SeBoneParamCaculate::CalculateBoneNumber()
{
	if (m_dStructureThickness != 0.0 && m_dSeparation != 0.0)
	{
		m_dNumberOfBone = 1.0/(m_dStructureThickness + m_dSeparation);
	}
}


void SeBoneParamCaculate::CalculateConnectivity()
{
	SeBoneConnectivity* Connect = new SeBoneConnectivity(m_pDcmArray, m_nMinValue, m_nMin, m_nMax, m_nWidth, m_nHeight, m_nSize);

	m_dEulerNumber = Connect->getSumEuler();

	double deltaChi = Connect->getDeltaChi(m_dEulerNumber);
	m_dConnectivity = 1 - deltaChi;

	m_dConnectivityDensity = m_dConnectivity/(m_nSize*m_nHeight*m_nWidth*m_dMMperPixelXY*m_dMMperPixelXY*m_dMMperPixelZ);
}

void SeBoneParamCaculate::CalculateSMI()
{
	SeBoneParamSMI* SMI = new SeBoneParamSMI(m_pDcmArray,m_nMinValue,m_nMin,m_nMax,m_nWidth,m_nHeight,m_nSize);
	m_dSMI = SMI->CalculatSMI(6);
}

void SeBoneParamCaculate::CalculateTBPf()
{
	SeBoneParaTBPf* TBPf = new SeBoneParaTBPf(m_pDcmArray,m_nMinValue,m_nMin,m_nMax,m_nWidth,m_nHeight,m_nSize);
	m_dTBPf = TBPf->CaculateTBPf();
}

void SeBoneParamCaculate::CalculateDA()
{
	int s = 0;
	SeBoneAnistrophy* Anisotrophy = new SeBoneAnistrophy(m_pDcmArray, m_nMinValue, m_nMin, m_nMax, m_nWidth, m_nHeight, m_nSize, m_dMMperPixelXY, m_dMMperPixelZ);
	double vW = m_dMMperPixelXY;
	double vH = m_dMMperPixelXY;
	double vD = m_dMMperPixelZ;
	int w = m_nWidth;
	int h = m_nHeight;
	int d = m_nSize;
	double vectorSampling = max(vW, max(vH, vD)) * 2.3;
	double radius = min(h * vH, min(d * vD, w * vW)) / 4.0;
	int nVectors = 500;
	int minSize =100;
	int maxSize = 2000;
	double variance =  100000;
	double tolerance = 0.005;
	vector <double> anisotropyHistory;
	//	vector<double> erroHistory;

	double* vectorList = Anisotrophy->regularVectors(nVectors);

	double* sumInterceptCounts = new double[nVectors];
	memset(sumInterceptCounts,0,nVectors*sizeof(double));
	// don't konw how many steps going to use, so just think it will take 1500 steps;
	int nCount = 0;
	while ((s<minSize)||(s >= minSize && s < maxSize && variance > tolerance)) 
	{
		if (nCount < 1500)
			theAppIVConfig.m_pILog->ProgressStepIt();
		s++;
		// return a single centroid within the bounds
		double* centroid = Anisotrophy->gridCalculator(radius);

		// count intercepts at centroid
		double* interceptCounts = Anisotrophy->countIntercepts(centroid, vectorList,
			nVectors, radius, vectorSampling);

		for (int i = 0; i < nVectors; i++) {
			sumInterceptCounts[i] += interceptCounts[i];
		}

		// work out the current mean intercept length
		double* meanInterceptLengths = new double[nVectors];
		memset(meanInterceptLengths,0,nVectors*sizeof(double));

		double probeLength = radius * s;
		for (int v = 0; v < nVectors; v++) {
			if (sumInterceptCounts[v] == 0)
				meanInterceptLengths[v] = probeLength;
			else
				meanInterceptLengths[v] = probeLength / sumInterceptCounts[v];
		}
		// work out coordinates of vector cloud
		double* coOrdinates = Anisotrophy->calculateCoordinates(meanInterceptLengths, vectorList, nVectors);

		double* d_Anisotropyarray = Anisotrophy->harriganMann(coOrdinates);

		m_dDegreeOfAnisotropy = d_Anisotropyarray[3];
		m_dEigenValue1 = d_Anisotropyarray[0];
		m_dEigenValue2 = d_Anisotropyarray[1];
		m_dEigenValue3 = d_Anisotropyarray[2];

		anisotropyHistory.push_back(d_Anisotropyarray[3]);
		variance = Anisotrophy->getVariance(anisotropyHistory, minSize);
		//if (variance + anisotropy > 1 || anisotropy - variance < 0) {
		//	variance = max(min(1 - anisotropy, anisotropy),tolerance);
		//}
		//erroHistory.push_back(variance);
		Safe_DeleteVec(d_Anisotropyarray);
		Safe_DeleteVec(centroid);
		Safe_DeleteVec(interceptCounts);
	}
	while (nCount < 1500)
	{
		theAppIVConfig.m_pILog->ProgressStepIt();
		nCount++;
	}
	Safe_Delete(Anisotrophy);
	Safe_DeleteVec(vectorList);
	Safe_DeleteVec(sumInterceptCounts);
	anisotropyHistory.clear();
	//erroHistory.clear();

}

double SeBoneParamCaculate::ResampleSMI(int ResampleVoxel,BOOL BForSMI)
{
	map<vertex,vector<int>> vertexHash;
	map<vertex, vector<int>>::iterator it;
	map <vertex,vertex>		 normal;
	map<vertex, vertex>::iterator it_this;
	int VoxelNum = 0;

	short** newImage = Resample(ResampleVoxel,ResampleVoxel,ResampleVoxel, VoxelNum);
	int wet = 1+(m_nWidth-1)/ResampleVoxel;
	int het = 1+(m_nHeight-1)/ResampleVoxel;
	int slicethick =1+(m_nSize-1)/ResampleVoxel;
	double pixelwidth = m_dMMperPixelXY; //size of pixel
	double pixelZwidth = m_dMMperPixelZ;

	double ResampleVoxelX = m_nWidth/(double)wet;
	double ResampleVoxelY = m_nHeight/(double)het;
	double ResampleVoxelZ = m_nSize/(double)slicethick;
	double d_Volume = VoxelNum*(pixelwidth*ResampleVoxelX)*(pixelwidth*ResampleVoxelY)*(pixelZwidth*ResampleVoxelZ);

	double d_Area = ResampleAreaCalulate(newImage,wet,het,slicethick);
	TriangleClassify(vertexHash, it);          //Multithread processing
	
	AverageNormal(normal, vertexHash, it);
	double d_DilationArea = MoveTriangleAndCalArea(normal);
	d_Area = d_Area * (pixelwidth*ResampleVoxelX)*(pixelwidth*ResampleVoxelY);
	d_DilationArea = d_DilationArea * (pixelwidth*ResampleVoxelX)*(pixelwidth*ResampleVoxelY);

	double d_Result = 0;
	double d_step1 = (ResampleVoxelX*pixelwidth)/100;
	d_Result = 6* d_Volume * (d_DilationArea - d_Area)/(d_step1*d_Area*d_Area);
	return d_Result;	
}

void SeBoneParamCaculate::CalculateDensity()
{
	LONGLONG lCount = 0;
	LONGLONG lValueSum = 0;
	LONGLONG lCountTotal = 0;
	LONGLONG lValueSumTotal = 0;
	for (int i=0; i<m_nSize; i++)
	{
		theAppIVConfig.m_pILog->ProgressStepIt();
		short* pData = (short*)m_pDcmArray->GetDcmArray()[i]->GetData();
		short* pHead = pData;
		for(int j=0; j<m_nWidth*m_nHeight; j++)
		{
			if (*pHead != 0)
			{
				lValueSumTotal += *pHead;
				lCountTotal++;
			}
			if (*pHead > m_nMin && *pHead < m_nMax)
			{
				lValueSum += *pHead;
				lCount++;
			}
			
			*pHead++;
		}
	}
	double dMeanValue = (double)lValueSum/(double)lCount;
	m_dMeanValue = dMeanValue;
	if (m_nSmallValue < m_nBigValue && m_fSmallDensity < m_fBigDensity)
	{
		m_dDensity = (dMeanValue - m_nSmallValue) * (m_fBigDensity - m_fSmallDensity) / (m_nBigValue - m_nSmallValue) + m_fSmallDensity;
	}
	double dMeanValueTotal= (double)lValueSumTotal/(double)lCountTotal;
	m_dMeanValueTotal = dMeanValueTotal;
	if (m_nSmallValue < m_nBigValue && m_fSmallDensity < m_fBigDensity)
	{
		m_dDensityTotal = (dMeanValueTotal - m_nSmallValue) * (m_fBigDensity - m_fSmallDensity) / (m_nBigValue - m_nSmallValue) + m_fSmallDensity;
	}
}

void SeBoneParamCaculate::CalculateCorticalArea()
{

}

void SeBoneParamCaculate::CalculateCorticalThickness()
{

}

void SeBoneParamCaculate::CalculateCorticalFraction()
{

}

void SeBoneParamCaculate::CalculateMedullaryArea()
{

}

void SeBoneParamCaculate::CalculateCorticalDensity()
{

}

short** SeBoneParamCaculate::Resample(int factorX, int factorY, int factorZ, int& VoxelNum)
{
	VoxelNum = 0;
	int* histo = new int[510];
	memset(histo,0,sizeof(int)*510);

	int d = m_nSize;
	int w = m_nWidth;
	int h = m_nHeight;

	short** newImage = new short*[(d-1)/factorZ+1];

	for(int z=0;z<d;z+=factorZ) {
		int kfactor=(z+factorZ<d?factorZ:d-z);
		short** slices = new short*[kfactor];

		for(int k=0;k<kfactor;k++)
			slices[k]=(short*)m_pDcmArray->GetDcmArray()[z+k]->GetData();

		int pointsInNewSlice = (1+(w-1)/factorX)*(1+(h-1)/factorY);
		short* newSlice = new short[pointsInNewSlice];
		memset(newSlice,0,sizeof(short)*pointsInNewSlice);
		for(int y=0;y<h;y+=factorY)
		{
			for(int x=0;x<w;x+=factorX)
			{
				int ifactor=(x+factorX<w?factorX:w-x);
				int jfactor=(y+factorY<h?factorY:h-y);

				int indexOfHighest = -1;
				int highest = -1;
				memset(histo,0,sizeof(int)*510);

				for(int i=0;i<ifactor;i++)
				{
					for(int j=0;j<jfactor;j++)
					{
						for(int k=0;k<kfactor;k++)
						{
							int value = slices[k][x+i+w*(y+j)]+1000;
							histo[value]++;
							if (histo[value] > highest)
							{
								highest = histo[value];
								indexOfHighest = value;
							}
						}
					}
				}
				if(indexOfHighest-1000 == -500)
					VoxelNum++;
				newSlice[(x/factorX)+((w-1)/factorX+1)*(y/factorY)] = indexOfHighest-1000;
			}
		}
		newImage[z/factorZ] = newSlice;
	} 	
	Safe_DeleteVec(histo);
	return newImage;
}

double SeBoneParamCaculate::ResampleAreaCalulate(short** Imageplus, int Width, int Height, int ImagePiece)
{
	short* slicefront = new short[Height*Width];
	for (int j=0;j<Width*Height;j++)
	{
		slicefront[j] = m_nSmallValue;
	}
	short* slice1 = NULL;
	short* slice2 = NULL;

	SeBoneParameter* Param = new SeBoneParameter;
	Param->GenerateTriangleCount();
	for(int i = -1; i < ImagePiece; i++)     //calculate the area of bone surface
	{
		int Nobone = m_nMin; 
		if (i != -1&&i !=ImagePiece-1)
		{
			slice1 =(short*)Imageplus[i];
			slice2=(short*)Imageplus[i+1];
		}
		else if (i == -1)
		{
			slice1=slicefront;
			slice2=(short*)Imageplus[i+1];
		}
		else if (i == ImagePiece-1)
		{
			slice1=Imageplus[i];
			slice2=slicefront;
		}
		Param->CreateCubes(i,Width, Height, slice1, slice2);
		CBasicCube*		pCube = Param->GetAllCubes();
		int	 nCubeCount = Param->GetCubeCount();
		for (int j = 0; j < nCubeCount; j++)
		{
			int		nCubeIndex = pCube[j].GetVertexIndex();
			int		nTri = Param->s_TriangleCountMetrix[nCubeIndex];
			if (nTri == 0)
			{continue;}
			CTriangle *pTri = new CTriangle[nTri];
			Param->Polygonise(pCube[j].GetCubeVertex(), pTri, nTri, nCubeIndex);//获取每个立方体的三角面片
			for (int k = 0; k < nTri; k++)
			{
				m_vTriangle.push_back(pTri[k]);
			}
			Safe_DeleteVec(pTri);			
		}
		Safe_DeleteVec(pCube);
	}
	Safe_Delete(Param);
	Safe_DeleteVec(slicefront);

	for (int i=0;i<ImagePiece;i++)
	{
		Safe_DeleteVec(Imageplus[i]);
	}
	
	double totalarea = 0.0;
	for (int i = 0; i < (int)m_vTriangle.size(); i++)      //calculate total triangle area
	{
		double a1=(m_vTriangle[i].m_point[0].fx-m_vTriangle[i].m_point[1].fx);
		double a2=(m_vTriangle[i].m_point[0].fy-m_vTriangle[i].m_point[1].fy);
		double a3=(m_vTriangle[i].m_point[0].fz-m_vTriangle[i].m_point[1].fz);
		double line1=sqrt(a1*a1+a2*a2+a3*a3);
		double b1=(m_vTriangle[i].m_point[1].fx-m_vTriangle[i].m_point[2].fx);
		double b2=(m_vTriangle[i].m_point[1].fy-m_vTriangle[i].m_point[2].fy);
		double b3=(m_vTriangle[i].m_point[1].fz-m_vTriangle[i].m_point[2].fz);
		double line2=sqrt(b1*b1+b2*b2+b3*b3);
		double c1=(m_vTriangle[i].m_point[0].fx-m_vTriangle[i].m_point[2].fx);
		double c2=(m_vTriangle[i].m_point[0].fy-m_vTriangle[i].m_point[2].fy);
		double c3=(m_vTriangle[i].m_point[0].fz-m_vTriangle[i].m_point[2].fz);
		double line3=sqrt(c1*c1+c2*c2+c3*c3);
		double p=(line1+line2+line3)/2;
		totalarea += sqrt(p*(p-line1)*(p-line2)*(p-line3));
	}
	return totalarea;
}


void SeBoneParamCaculate::TriangleClassify(map<vertex,vector<int>>& vertexHash, map<vertex, vector<int>>::iterator& it)
{
	for (int i=0;i<m_vTriangle.size();i++)
	{	
		for (int j=0;j<3;j++)
		{
			vertex cPoint;

			cPoint.x = m_vTriangle[i].m_point[j].fx;
			cPoint.y = m_vTriangle[i].m_point[j].fy;
			cPoint.z = m_vTriangle[i].m_point[j].fz;

			it = vertexHash.find(cPoint);
			if (it == vertexHash.end()) {
				vector<int> Array;
				Array.push_back(i);
				vertexHash.insert(pair<vertex,vector<int>>(cPoint,Array));
			} else {
				it->second.push_back(i);
				vertexHash.insert(pair<vertex,vector<int>>(cPoint,it->second));
			}
		}
	}
}


void SeBoneParamCaculate::AverageNormal(map <vertex,vertex>& normal, map<vertex,vector<int>>& vertexHash, map<vertex, vector<int>>::iterator& it)
{
	vertex sumNormals,moveVertex;
	double dPixel = m_dMMperPixelXY;
	double lStep = 0.01;     //dilation 1/100pixel
	int	   corner = 0;
	for(it = vertexHash.begin(); it!=vertexHash.end(); ++it) 
	{
		sumNormals.x = 0;
		sumNormals.y = 0;
		sumNormals.z = 0;

		moveVertex.x = 0;
		moveVertex.y = 0;
		moveVertex.z = 0;

		int vT = (int)it->second.size();
		for (int i=0;i<vT;i++)
		{
			int range = it->second[i];

			if ((m_vTriangle[range].m_point[0].fx == it->first.x)&&(m_vTriangle[range].m_point[0].fy == it->first.y)&&(m_vTriangle[range].m_point[0].fz == it->first.z))
			{
				corner = 0;
			}
			else if ((m_vTriangle[range].m_point[1].fx == it->first.x)&&(m_vTriangle[range].m_point[1].fy == it->first.y)&&(m_vTriangle[range].m_point[1].fz == it->first.z))
			{
				corner = 1;
			}
			else if ((m_vTriangle[range].m_point[2].fx == it->first.x)&&(m_vTriangle[range].m_point[2].fy == it->first.y)&&(m_vTriangle[range].m_point[2].fz == it->first.z))
			{
				corner = 2;
			}
			vertex surfaceNormal = crossProduct(m_vTriangle[range],corner);
			sumNormals.x += surfaceNormal.x;
			sumNormals.y += surfaceNormal.y;
			sumNormals.z += surfaceNormal.z;
		}
		sumNormals.x /= vT;
		sumNormals.y /= vT;
		sumNormals.z /= vT;
		double length = sqrt(sumNormals.x*sumNormals.x+sumNormals.y*sumNormals.y+sumNormals.z*sumNormals.z);
		sumNormals.x /= length;
		sumNormals.y /= length;
		sumNormals.z /= length;

		moveVertex.x =it->first.x + lStep*sumNormals.x;
		moveVertex.y =it->first.y + lStep*sumNormals.y;
		moveVertex.z =it->first.z + lStep*sumNormals.z;

		normal.insert(pair<vertex,vertex>(it->first,moveVertex));
	}
	vertexHash.clear();
}


double SeBoneParamCaculate::MoveTriangleAndCalArea(map <vertex,vertex>& normal)
{
	map<vertex, vertex>::iterator it_this;
	for (int i=0;i<m_vTriangle.size();i++)
	{	
		for (int j=0;j<3;j++)
		{
			vertex cPoint;
			cPoint.x = m_vTriangle[i].m_point[j].fx;
			cPoint.y = m_vTriangle[i].m_point[j].fy;
			cPoint.z = m_vTriangle[i].m_point[j].fz;

			it_this = normal.find(cPoint);

			m_vTriangle[i].m_point[j].fx = it_this->second.x;
			m_vTriangle[i].m_point[j].fy = it_this->second.y;
			m_vTriangle[i].m_point[j].fz = it_this->second.z;
		}
	}
	normal.clear();
	double totalarea = 0.0;
	for (int i = 0; i < (int)m_vTriangle.size(); i++)      //calculate total triangle area
	{
		double a1=(m_vTriangle[i].m_point[0].fx-m_vTriangle[i].m_point[1].fx);
		double a2=(m_vTriangle[i].m_point[0].fy-m_vTriangle[i].m_point[1].fy);
		double a3=(m_vTriangle[i].m_point[0].fz-m_vTriangle[i].m_point[1].fz);
		double line1=sqrt(a1*a1+a2*a2+a3*a3);
		double b1=(m_vTriangle[i].m_point[1].fx-m_vTriangle[i].m_point[2].fx);
		double b2=(m_vTriangle[i].m_point[1].fy-m_vTriangle[i].m_point[2].fy);
		double b3=(m_vTriangle[i].m_point[1].fz-m_vTriangle[i].m_point[2].fz);
		double line2=sqrt(b1*b1+b2*b2+b3*b3);
		double c1=(m_vTriangle[i].m_point[0].fx-m_vTriangle[i].m_point[2].fx);
		double c2=(m_vTriangle[i].m_point[0].fy-m_vTriangle[i].m_point[2].fy);
		double c3=(m_vTriangle[i].m_point[0].fz-m_vTriangle[i].m_point[2].fz);
		double line3=sqrt(c1*c1+c2*c2+c3*c3);
		double p=(line1+line2+line3)/2;
		totalarea += sqrt(p*(p-line1)*(p-line2)*(p-line3));
	}
	return totalarea;
}

vertex SeBoneParamCaculate::crossProduct(CTriangle &vertext,int corner)
{
	CTriangle point;
	switch (corner) {
	case 0:
		point.m_point[0] = vertext.m_point[0];
		point.m_point[1] = vertext.m_point[2];
		point.m_point[2] = vertext.m_point[1];
		break;
	case 1:
		point.m_point[0] = vertext.m_point[2];
		point.m_point[1] = vertext.m_point[1];
		point.m_point[2] = vertext.m_point[0];
		break;
	case 2:
		point.m_point[0] = vertext.m_point[1];
		point.m_point[1] = vertext.m_point[0];
		point.m_point[2] = vertext.m_point[2];
		break;
	}
	double x1 = point.m_point[1].fx - point.m_point[0].fx;
	double y1 = point.m_point[1].fy - point.m_point[0].fy;
	double z1 = point.m_point[1].fz - point.m_point[0].fz;
	double x2 = point.m_point[2].fx - point.m_point[0].fx;
	double y2 = point.m_point[2].fy - point.m_point[0].fy;
	double z2 = point.m_point[2].fz - point.m_point[0].fz;

	vertex crossVector;
	crossVector.x = y1 * z2 - z1 * y2;
	crossVector.y = z1 * x2 - x1 * z2;
	crossVector.z = x1 * y2 - y1 * x2;
	return crossVector;
}

void SeBoneParamCaculate::ResetAllResult()
{
	m_nSmallValue = 0;
	m_nBigValue = 0;
	m_fSmallDensity = 0.0;
	m_fBigDensity = 0.0;

	m_dTotleVolume = 0.0;
	m_dVolume = 0.0;
	m_dSurfaceArea = 0.0;
	m_dSpecificSurfaceArea = 0.0;
	m_dVolumeFraction = 0.0;
	m_dSurfaceDensity = 0.0;
	m_dCentroidX = 0.0;
	m_dCentroidY = 0.0;
	m_dCentroidZ = 0.0;

	m_dStructureThickness = 0.0;
	m_dThicknessMax = 0.0;
	m_dThicknessStdDev = 0.0;
	m_dSeparation = 0.0;
	m_dSeparationStdDev = 0.0;
	m_dSeparationMax = 0.0;
	m_dNumberOfBone = 0.0;
	m_dEulerNumber = 0.0;
	m_dConnectivity = 0.0;
	m_dConnectivityDensity = 0.0;
	m_dDegreeOfAnisotropy = 0.0;
	m_dEigenValue1 = 0.0;
	m_dEigenValue2 = 0.0;
	m_dEigenValue3 = 0.0;
	m_dSMI = 0.0;
	m_dDensity = 0.0;
	m_dContent = 0.0;
	m_dMeanValue = 0.0;
	m_dDensityTotal = 0.0;
	m_dMeanValueTotal = 0.0;

	m_dCorticalTotalArea = 0.0;
	m_dCorticalThickness = 0.0;
	m_dCorticalAreaFraction = 0.0;
	m_dMedullaryArea = 0.0;
	m_dCorticalDensity = 0.0;
	m_dCorticalContent = 0.0;
}


void SeBoneParamCaculate::SetCaculateParam(BOOL BasicParameter /*= FALSE*/, BOOL BoneThick /*= FALSE*/, BOOL SpaceThick /*= FALSE*/, BOOL BoneNumber /*= FALSE*/, BOOL Connectivity /*= FALSE*/, BOOL SMI /*= FALSE*/, BOOL DA /*= FALSE*/, BOOL TBPf /*= FALSE*/, BOOL Density /*= FALSE*/, BOOL CorticalArea /*= FALSE*/, BOOL CorticalThick /*= FALSE*/, BOOL CorticalFraction /*= FALSE*/, BOOL MedullaryArea /*= FALSE*/, BOOL CorticalDensity /*= FALSE*/, int SmallValue /*= 0*/, int BigValue /*= 0*/, float SmallDensity /*= 0.0*/, float BigDensity /*= 0.0*/)
{
	ResetAllResult();
	m_bBasicParameter = BasicParameter;
	m_bBoneThick = BoneThick;
	m_bSpaceThick = SpaceThick;
	m_bBoneNumber = BoneNumber;
	m_bConnectivity = Connectivity;
	m_bSMI = SMI;
	m_bDA = DA;
	m_bTBPf = TBPf;
	m_bDensity = Density;
	m_bCorticalArea = CorticalArea;
	m_bCorticalThick = CorticalThick;
	m_bCorticalFraction = CorticalFraction;
	m_bMedullaryArea = MedullaryArea;
	m_bCorticalDensity = CorticalDensity;
	m_nSmallValue = SmallValue;
	m_nBigValue = BigValue;
	m_fSmallDensity = SmallDensity;
	m_fBigDensity = BigDensity;
}

void SeBoneParamCaculate::ConvertResult()
{
	m_vecResult.clear();
	m_vecResult.push_back(SingleParam(_T("总体积"),			_T("Total VOI volume"),				 _T("TV"),				m_dTotleVolume,			_T("mm^3")));
	m_vecResult.push_back(SingleParam(_T("骨体积"),			_T("Bone volume"),					 _T("BV"),				m_dVolume,				_T("mm^3")));
	m_vecResult.push_back(SingleParam(_T("骨面积"),			_T("Bone surface"),					 _T("BS"),				m_dSurfaceArea,			_T("mm^2")));
	m_vecResult.push_back(SingleParam(_T("骨面积/骨体积"),	_T("Bone surface/volume ratio"),	 _T("BS/BV"),			m_dSpecificSurfaceArea, _T("")));
	m_vecResult.push_back(SingleParam(_T("体积分数"),		_T("Percent Bone volume"),			 _T("BV/TV"),			m_dVolumeFraction,		_T("")));
	m_vecResult.push_back(SingleParam(_T("骨表面密度"),		_T("Bone surface density"),			 _T("BS/TV"),			m_dSurfaceDensity,		_T("")));
	m_vecResult.push_back(SingleParam(_T("中心点X坐标"),	_T("X"),							 _T("Crd.X"),			m_dCentroidX,			_T("mm")));
	m_vecResult.push_back(SingleParam(_T("中心点Y坐标"),	_T("Y"),							 _T("Crd.Y"),			m_dCentroidY,			_T("mm")));
	m_vecResult.push_back(SingleParam(_T("中心点Z坐标"),	_T("Z"),							 _T("Crd.Z"),			m_dCentroidZ,			_T("mm")));
	if (m_bThickness) {
		m_vecResult.push_back(SingleParam(_T("骨小梁厚度"),		_T("Structure thickness"),			 _T("Tb.Th"),			m_dStructureThickness,	_T("mm")));
		m_vecResult.push_back(SingleParam(_T("厚度最大值"),		_T("Thickness Max"),				 _T("Tb.Th Max"),		m_dThicknessMax,		_T("mm")));
		m_vecResult.push_back(SingleParam(_T("厚度方差"),		_T("Thickness Std Dev"),			 _T("Tb.Th Std Dev"),	m_dThicknessStdDev,		_T("")));
		m_vecResult.push_back(SingleParam(_T("骨小梁间隙"),		_T("Structure separation"),			 _T("Tb.Sp"),			m_dSeparation,			_T("mm")));
		m_vecResult.push_back(SingleParam(_T("间隙最大值"),		_T("Separation Max"),				 _T("Tb.Sp Max"),		m_dSeparationMax,		_T("mm")));
		m_vecResult.push_back(SingleParam(_T("间隙方差"),		_T("Separation Std Dev"),			 _T("Tb.Sp Std Dev"),	m_dSeparationStdDev,	_T("")));
		m_vecResult.push_back(SingleParam(_T("骨小梁数量"),		_T("Number of Bone"),				 _T("Tb.N"),			m_dNumberOfBone,		_T("1/mm")));

		m_vecResult.push_back(SingleParam(_T("欧拉数"),			_T("Euler number"),					 _T("Eu.N"),			m_dEulerNumber,			_T("")));
		m_vecResult.push_back(SingleParam(_T("连通性"),			_T("Connectivity"),					 _T("Conn"),			m_dConnectivity,		_T("")));
		m_vecResult.push_back(SingleParam(_T("连通性密度"),		_T("Connectivity density"),			 _T("Conn.Dn"),			m_dConnectivityDensity, _T("1/mm")));
		m_vecResult.push_back(SingleParam(_T("各项异性"),		_T("Degree of anisotropy"),			 _T("DA"),				m_dDegreeOfAnisotropy,	_T("")));
		m_vecResult.push_back(SingleParam(_T("特征值1"),		_T("EigenValue 1"),					 _T("EV 1"),			m_dEigenValue1,			_T("")));
		m_vecResult.push_back(SingleParam(_T("特征值2"),		_T("EigenValue 2"),					 _T("EV 2"),			m_dEigenValue2,			_T("")));
		m_vecResult.push_back(SingleParam(_T("特征值3"),		_T("EigenValue 3"),					 _T("EV 3"),			m_dEigenValue3,			_T("")));
		m_vecResult.push_back(SingleParam(_T("骨小梁结构因子"),	_T("Trabecular Bone Pattern factor"),_T("TBPf"),			m_dTBPf,				_T("")));
		m_vecResult.push_back(SingleParam(_T("结构模型指数"),	_T("Structure model index"),		 _T("SMI"),				m_dSMI,					_T("")));
		m_vecResult.push_back(SingleParam(_T("骨矿物密度"),			_T("Bone Mineral Density"),					 _T("BMD"),				m_dDensity,				_T("g/cm^3")));
		m_vecResult.push_back(SingleParam(_T("骨矿物含量"),			_T("Bone Mineral Content"),					 _T("BMC"),				m_dContent,				_T("mg")));
		m_vecResult.push_back(SingleParam(_T("平均CT值"),		_T("Average CT Value"),				 _T("--"),				m_dMeanValue,			_T("HU")));	
		m_vecResult.push_back(SingleParam(_T("综合骨密度"),			_T("Bone Density"),					 _T("BMD"),				m_dDensityTotal,				_T("g/cm^3")));
		m_vecResult.push_back(SingleParam(_T("综合平均CT值"),		_T("Average CT Value"),				 _T("--"),				m_dMeanValueTotal,			_T("HU")));	
	}
	if (m_bCortical) {
		m_vecResult.push_back((SingleParam(_T("皮质骨总面积"),   _T("Area of Total Cortical Bone"),   _T("Tt.Ar"),           m_dCorticalTotalArea,   _T("mm^2"))));
		m_vecResult.push_back((SingleParam(_T("皮质骨面积"),     _T("Area of Avg Cortical Bone"),    _T("Ct.Ar"),            m_dCorticalArea,        _T("mm^2"))));
		m_vecResult.push_back((SingleParam(_T("皮质骨厚度"),     _T("Thinckness of Cortical Bone"),  _T("Ct.Th"),            m_dCorticalThickness,   _T("mm"))));
		m_vecResult.push_back((SingleParam(_T("皮质骨面积和总面积比值"), _T("Fraction of Cortical Bone Area"), _T("Ct.Ar/Tt.Ar"), m_dCorticalAreaFraction, _T("%"))));
		m_vecResult.push_back((SingleParam(_T("骨髓腔面积"),     _T("Area of Medullary Cavity"),      _T("Ma.Ar"),           m_dMedullaryArea,       _T("mm^2"))));
		m_vecResult.push_back(SingleParam(_T("皮质骨矿物密度"),			_T("Cortical Mineral Bone Density"),					 _T("CBMD"),				m_dCorticalDensity,				_T("g/cm^3")));
		m_vecResult.push_back(SingleParam(_T("皮质骨矿物含量"),			_T("Cortical Mineral Bone Content"),					 _T("CBMC"),				m_dCorticalContent,				_T("mg")));
		m_vecResult.push_back(SingleParam(_T("平均CT值"),		_T("Average CT Value"),				 _T("--"),				m_dMeanValue,			_T("HU")));	
	}

}




