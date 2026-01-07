	#include "StdAfx.h"
	#include "SeBoneParamConnectivity.h"
	#include "SeBoneDensityDlg.h"
	#include "BoneDensitySwapData.h"

	SeBoneConnectivity::SeBoneConnectivity(void)
	{
		m_nMinValue = 0;
		m_nWidth = 0;
		m_nHeight = 0;
		m_nOriImagePiece = 0;
		m_pProcessArray = NULL;
		m_eulerLUT = NULL;
		m_sumEulerInt = NULL;
		//InitializeCriticalSection(&pLock);
	}

	SeBoneConnectivity::SeBoneConnectivity(CDcmPicArray* p, int nMinValue, int nMin, int nMax, int nWidth, int nHeight, int nSize)
	{
		m_eulerLUT = NULL;
		m_sumEulerInt = NULL;
		m_pProcessArray = p;
		m_nMinValue = nMinValue;
		m_nMin = nMin;
		m_nMax = nMax;
		m_nWidth = nWidth;
		m_nHeight = nHeight;
		m_nOriImagePiece = nSize;
	}

	SeBoneConnectivity::~SeBoneConnectivity(void)
	{
		//DeleteCriticalSection(&pLock);
	}


	double SeBoneConnectivity::getSumEuler()
	{
		m_eulerLUT = new int[256];
		memset(m_eulerLUT, 0, 256*sizeof(int));
		fillEulerLUT2(m_eulerLUT);

		m_sumEulerInt = new int[m_nOriImagePiece+1];
		memset(m_sumEulerInt, 0, (m_nOriImagePiece+1)*sizeof(int));
		//long deltaEuler = 0;

		vector<CWinThread*> Cwin_thread;
		ParamConnect* NewParameter = new ParamConnect[8];

		for (int i=0; i<8; i++)
		{
			NewParameter[i].pSeBone = this;
			NewParameter[i].nThread = i;
			//NewParameter[i].pCriticalLock = &pLock;
			Cwin_thread.push_back(AfxBeginThread(_threadGetEuler , &NewParameter[i]));
		}
		bool bDone = false;
		int  nThreadCount = 0;
		while(!bDone)
		{
			if(nThreadCount >= 8)
			{
				bDone = true;
				break;
			}
			if(WaitForSingleObject(Cwin_thread[nThreadCount]->m_hThread,0) == WAIT_OBJECT_0)
			{
				nThreadCount++;
				continue;
			}
			else
			{
				WaitForSingleObject(Cwin_thread[nThreadCount]->m_hThread,INFINITE);
				nThreadCount++;
			}
		}
		double sumEuler = 0;
		for (int i = 0; i < m_nOriImagePiece+1; i++) {
			sumEuler += m_sumEulerInt[i];
		}
		sumEuler /= 8;
		Safe_DeleteVec(m_eulerLUT);
		Cwin_thread.clear();
		Safe_DeleteVec(NewParameter);
		Safe_DeleteVec(m_sumEulerInt);
		return sumEuler;
	}


	UINT SeBoneConnectivity::_threadGetEuler(LPVOID pVoid)
	{
		ParamConnect* Parameter = (ParamConnect*)pVoid;
		SeBoneConnectivity* pThis = Parameter->pSeBone;
		int nThread = Parameter->nThread;
		
		for (int z = nThread; z <= pThis->m_nOriImagePiece; z+=8) 
		{
			theAppIVConfig.m_pILog->ProgressStepIt();
			for (int y = 0; y <= pThis->m_nHeight; y++) {
				for (int x = 0; x <= pThis->m_nWidth; x++) {
					int* octant = pThis->getOctant(x, y, z);
					if (octant[0] > 0) { 
						long deltaEuler = pThis->getDeltaEuler(octant, pThis->m_eulerLUT);
						pThis->m_sumEulerInt[z] += deltaEuler;
					}
					Safe_DeleteVec(octant);
				}
			}
		}		
		return 0;
	}


	void SeBoneConnectivity::fillEulerLUT2( int* LUT )
	{
		LUT[1] = 1;
		LUT[3] = 0;
		LUT[5] = 0;
		LUT[7] = -1;
		LUT[9] = -2;
		LUT[11] = -1;
		LUT[13] = -1;
		LUT[15] = 0;
		LUT[17] = 0;
		LUT[19] = -1;
		LUT[21] = -1;
		LUT[23] = -2;
		LUT[25] = -3;
		LUT[27] = -2;
		LUT[29] = -2;
		LUT[31] = -1;
		LUT[33] = -2;
		LUT[35] = -1;
		LUT[37] = -3;
		LUT[39] = -2;
		LUT[41] = -1;
		LUT[43] = -2;
		LUT[45] = 0;
		LUT[47] = -1;
		LUT[49] = -1;

		LUT[51] = 0;
		LUT[53] = -2;
		LUT[55] = -1;
		LUT[57] = 0;
		LUT[59] = -1;
		LUT[61] = 1;
		LUT[63] = 0;
		LUT[65] = -2;
		LUT[67] = -3;
		LUT[69] = -1;
		LUT[71] = -2;
		LUT[73] = -1;
		LUT[75] = 0;
		LUT[77] = -2;
		LUT[79] = -1;
		LUT[81] = -1;
		LUT[83] = -2;
		LUT[85] = 0;
		LUT[87] = -1;
		LUT[89] = 0;
		LUT[91] = 1;
		LUT[93] = -1;
		LUT[95] = 0;
		LUT[97] = -1;
		LUT[99] = 0;

		LUT[101] = 0;
		LUT[103] = 1;
		LUT[105] = 4;
		LUT[107] = 3;
		LUT[109] = 3;
		LUT[111] = 2;
		LUT[113] = -2;
		LUT[115] = -1;
		LUT[117] = -1;
		LUT[119] = 0;
		LUT[121] = 3;
		LUT[123] = 2;
		LUT[125] = 2;
		LUT[127] = 1;
		LUT[129] = -6;
		LUT[131] = -3;
		LUT[133] = -3;
		LUT[135] = 0;
		LUT[137] = -3;
		LUT[139] = -2;
		LUT[141] = -2;
		LUT[143] = -1;
		LUT[145] = -3;
		LUT[147] = 0;
		LUT[149] = 0;

		LUT[151] = 3;
		LUT[153] = 0;
		LUT[155] = 1;
		LUT[157] = 1;
		LUT[159] = 2;
		LUT[161] = -3;
		LUT[163] = -2;
		LUT[165] = 0;
		LUT[167] = 1;
		LUT[169] = 0;
		LUT[171] = -1;
		LUT[173] = 1;
		LUT[175] = 0;
		LUT[177] = -2;
		LUT[179] = -1;
		LUT[181] = 1;
		LUT[183] = 2;
		LUT[185] = 1;
		LUT[187] = 0;
		LUT[189] = 2;
		LUT[191] = 1;
		LUT[193] = -3;
		LUT[195] = 0;
		LUT[197] = -2;
		LUT[199] = 1;

		LUT[201] = 0;
		LUT[203] = 1;
		LUT[205] = -1;
		LUT[207] = 0;
		LUT[209] = -2;
		LUT[211] = 1;
		LUT[213] = -1;
		LUT[215] = 2;
		LUT[217] = 1;
		LUT[219] = 2;
		LUT[221] = 0;
		LUT[223] = 1;
		LUT[225] = 0;
		LUT[227] = 1;
		LUT[229] = 1;
		LUT[231] = 2;
		LUT[233] = 3;
		LUT[235] = 2;
		LUT[237] = 2;
		LUT[239] = 1;
		LUT[241] = -1;
		LUT[243] = 0;
		LUT[245] = 0;
		LUT[247] = 1;
		LUT[249] = 2;
		LUT[251] = 1;
		LUT[253] = 1;
		LUT[255] = 0;
	}

	int* SeBoneConnectivity::getOctant( int x, int y, int z )
	{
		int* octant = new int[9]; // index 0 is counter to determine octant
		// emptiness, index 8 is at (x,y,z)
		octant[0] = 0;
		octant[1] = getPixel(x - 1, y - 1, z - 1);
		octant[2] = getPixel(x - 1, y, z - 1);
		octant[3] = getPixel(x, y - 1, z - 1);
		octant[4] = getPixel(x, y, z - 1);
		octant[5] = getPixel(x - 1, y - 1, z);
		octant[6] = getPixel(x - 1, y, z);
		octant[7] = getPixel(x, y - 1, z);
		octant[8] = getPixel(x, y, z);

		for (int n = 1; n < 9; n++)
			octant[0] -= octant[n]; // foreground is -1, so octant[0] contains
		// nVoxels in octant
		return octant;
	}

	int SeBoneConnectivity::getDeltaEuler( int* octant, int* LUT )
	{
		int deltaEuler = 0;
		if (octant[0] == 0) { // check to make sure there is a foreground voxel
			// in this octant
			return deltaEuler;
		}
		int n = 1;
		// have to rotate octant voxels around vertex so that
		// octant[8] is foreground as eulerLUT assumes that voxel in position
		// 8 is always foreground. Only have to check each voxel once.
		if (octant[8] == -1) {
			n = 1;
			if (octant[1] == -1)
				n |= 128;
			if (octant[2] == -1)
				n |= 64;
			if (octant[3] == -1)
				n |= 32;
			if (octant[4] == -1)
				n |= 16;
			if (octant[5] == -1)
				n |= 8;
			if (octant[6] == -1)
				n |= 4;
			if (octant[7] == -1)
				n |= 2;
		} else if (octant[7] == -1) {
			n = 1;
			if (octant[2] == -1)
				n |= 128;
			if (octant[4] == -1)
				n |= 64;
			if (octant[1] == -1)
				n |= 32;
			if (octant[3] == -1)
				n |= 16;
			if (octant[6] == -1)
				n |= 8;
			if (octant[5] == -1)
				n |= 2;
		} else if (octant[6] == -1) {
			n = 1;
			if (octant[3] == -1)
				n |= 128;
			if (octant[1] == -1)
				n |= 64;
			if (octant[4] == -1)
				n |= 32;
			if (octant[2] == -1)
				n |= 16;
			if (octant[5] == -1)
				n |= 4;
		} else if (octant[5] == -1) {
			n = 1;
			if (octant[4] == -1)
				n |= 128;
			if (octant[3] == -1)
				n |= 64;
			if (octant[2] == -1)
				n |= 32;
			if (octant[1] == -1)
				n |= 16;
		} else if (octant[4] == -1) {
			n = 1;
			if (octant[1] == -1)
				n |= 8;
			if (octant[3] == -1)
				n |= 4;
			if (octant[2] == -1)
				n |= 2;
		} else if (octant[3] == -1) {
			n = 1;
			if (octant[2] == -1)
				n |= 8;
			if (octant[1] == -1)
				n |= 4;
		} else if (octant[2] == -1) {
			n = 1;
			if (octant[1] == -1)
				n |= 2;
		} else {
			// if we have got here, all the other voxels are background
			n = 1;
		}
		deltaEuler += LUT[n];
		return deltaEuler;
	}

	int SeBoneConnectivity::getPixel( int x, int y, int z )
	{
		if (x >= 0 && x < m_nWidth && y >= 0 && y < m_nHeight && z >= 0
			&& z < m_nOriImagePiece)
		{
			int value = ((short*)m_pProcessArray->GetDcmArray()[z]->GetData())[m_nWidth*y + x];
			if (value > m_nMin && value < m_nMax)
			{
				return -1;
			}else{
				return 0;
			}

		}
		else{
			return 0;
		}

	}


	double SeBoneConnectivity::getDeltaChi( double sumEuler )
	{
		double deltaChi = sumEuler - correctForEdges();
		return deltaChi;
	}

	double SeBoneConnectivity::correctForEdges()
	{
		long f = getStackVertices();
		long e = getStackEdges() + 3 * f;
		long c = getStackFaces() + 2 * e - 3 * f; // there are already 6 *
		// f in 2 * e, so remove
		// 3 * f
		long d = getEdgeVertices() + f;
		long a = getFaceVertices();
		long b = getFaceEdges();

		double chiZero = (double) f;
		double chiOne = (double) d - (double) e;
		double chiTwo = (double) a - (double) b + (double) c;

		double edgeCorrection = chiTwo / 2 + chiOne / 4 + chiZero / 8;

		return edgeCorrection;
	}

	long SeBoneConnectivity::getEdgeVertices()
	{
		long nEdgeVertices = 0;

		// vertex voxels contribute 1 edge vertex each
		// this could be taken out into a variable to avoid recalculating it
		// nEdgeVertices += getStackVertices(stack);

		// left->right edges
		for (int z = 0; z < m_nOriImagePiece; z += m_nOriImagePiece - 1) {
			for (int y = 0; y < m_nHeight; y += m_nHeight - 1) {
				for (int x = 1; x < m_nWidth; x++) {
					if (getPixel( x, y, z) == -1)
						nEdgeVertices++;
					else if (getPixel( x - 1, y, z) == -1)
						nEdgeVertices++;
				}
			}
			if (m_nOriImagePiece == 1)
				break;
		}

		// back->front edges
		for (int z = 0; z < m_nOriImagePiece; z += m_nOriImagePiece - 1) {
			for (int x = 0; x < m_nWidth; x += m_nWidth - 1) {
				for (int y = 1; y < m_nHeight; y++) {
					if (getPixel( x, y, z) == -1)
						nEdgeVertices++;
					else if (getPixel( x, y - 1, z) == -1)
						nEdgeVertices++;
				}
			}
			if (m_nOriImagePiece == 1)
				break;
		}

		// top->bottom edges
		for (int x = 0; x < m_nWidth; x += m_nWidth - 1) {
			for (int y = 0; y < m_nHeight; y += m_nHeight - 1) {
				for (int z = 1; z < m_nOriImagePiece; z++) {
					if (getPixel( x, y, z) == -1)
						nEdgeVertices++;
					else if (getPixel( x, y, z - 1) == -1)
						nEdgeVertices++;
					if (m_nOriImagePiece == 1)
						break;
				}
			}
		}
		return nEdgeVertices;
	}

	long SeBoneConnectivity::getFaceEdges()
	{
		long nFaceEdges = 0;

		// top and bottom faces (all 4 edges)
		// check 2 edges per voxel
		for (int z = 0; z < m_nOriImagePiece; z += m_nOriImagePiece - 1) {
			for (int y = 0; y <= m_nHeight; y++) {
				for (int x = 0; x <= m_nWidth; x++) {
					// if the voxel or any of its neighbours are foreground, the
					// vertex is counted
					if (getPixel(x, y, z) == -1) {
						nFaceEdges += 2;
					} else {
						if (getPixel(x, y - 1, z) == -1) {
							nFaceEdges++;
						}
						if (getPixel(x - 1, y, z) == -1) {
							nFaceEdges++;
						}
					}
				}
			}
			if (m_nOriImagePiece == 1)
				break;
		}

		// back and front faces, horizontal edges
		for (int y = 0; y < m_nHeight; y += m_nHeight - 1) {
			for (int z = 1; z < m_nOriImagePiece; z++) {
				for (int x = 0; x < m_nWidth; x++) {
					if (getPixel(x, y, z) == -1)
						nFaceEdges++;
					else if (getPixel(x, y, z - 1) == -1)
						nFaceEdges++;
				}
				if (m_nOriImagePiece == 1)
					break;
			}
		}

		// back and front faces, vertical edges
		for (int y = 0; y < m_nHeight; y += m_nHeight - 1) {
			for (int z = 0; z < m_nOriImagePiece; z++) {
				for (int x = 0; x <= m_nWidth; x++) {
					if (getPixel(x, y, z) == -1)
						nFaceEdges++;
					else if (getPixel(x - 1, y, z) == -1)
						nFaceEdges++;
				}
				if (m_nOriImagePiece == 1)
					break;
			}
		}

		// left and right stack faces, horizontal edges
		for (int x = 0; x < m_nWidth; x += m_nWidth - 1) {
			for (int z = 1; z < m_nOriImagePiece; z++) {
				for (int y = 0; y < m_nHeight; y++) {
					if (getPixel(x, y, z) == -1)
						nFaceEdges++;
					else if (getPixel(x, y, z - 1) == -1)
						nFaceEdges++;
				}
				if (m_nOriImagePiece == 1)
					break;
			}
		}

		// left and right stack faces, vertical voxel edges
		for (int x = 0; x < m_nWidth; x += m_nWidth - 1) {
			for (int z = 0; z < m_nOriImagePiece; z++) {
				for (int y = 1; y < m_nHeight; y++) {
					if (getPixel(x, y, z) == -1)
						nFaceEdges++;
					else if (getPixel(x, y - 1, z) == -1)
						nFaceEdges++;
				}
				if (m_nOriImagePiece == 1)
					break;
			}
		}
		return nFaceEdges;
	}/* end getFaceEdges */

	long SeBoneConnectivity::getFaceVertices()
	{
		long nFaceVertices = 0;

		// top and bottom faces (all 4 edges)
		for (int z = 0; z < m_nOriImagePiece; z += m_nOriImagePiece - 1) {
			for (int y = 0; y <= m_nHeight; y++) {
				for (int x = 0; x <= m_nWidth; x++) {
					// if the voxel or any of its neighbours are foreground, the
					// vertex is counted
					if (getPixel(x, y, z) == -1)
						nFaceVertices++;
					else if (getPixel(x, y - 1, z) == -1)
						nFaceVertices++;
					else if (getPixel(x - 1, y - 1, z) == -1)
						nFaceVertices++;
					else if (getPixel(x - 1, y, z) == -1)
						nFaceVertices++;
				}
			}
			if (m_nOriImagePiece == 1)
				break;
		}

		// left and right faces (2 vertical edges)
		for (int x = 0; x < m_nWidth; x += m_nWidth - 1) {
			for (int y = 0; y <= m_nHeight; y++) {
				for (int z = 1; z < m_nOriImagePiece; z++) {
					// if the voxel or any of its neighbours are foreground, the
					// vertex is counted
					if (getPixel(x, y, z) == -1)
						nFaceVertices++;
					else if (getPixel(x, y - 1, z) == -1)
						nFaceVertices++;
					else if (getPixel(x, y - 1, z - 1) == -1)
						nFaceVertices++;
					else if (getPixel(x, y, z - 1) == -1)
						nFaceVertices++;
					if (m_nOriImagePiece == 1)
						break;
				}
			}
		}

		// back and front faces (0 vertical edges)
		for (int y = 0; y < m_nHeight; y += m_nHeight - 1) {
			for (int x = 1; x < m_nWidth; x++) {
				for (int z = 1; z < m_nOriImagePiece; z++) {
					// if the voxel or any of its neighbours are foreground, the
					// vertex is counted
					if (getPixel(x, y, z) == -1)
						nFaceVertices++;
					else if (getPixel(x, y, z - 1) == -1)
						nFaceVertices++;
					else if (getPixel(x - 1, y, z - 1) == -1)
						nFaceVertices++;
					else if (getPixel(x - 1, y, z) == -1)
						nFaceVertices++;
					if (m_nOriImagePiece == 1)
						break;
				}
			}
		}
		return nFaceVertices;
	}

	long SeBoneConnectivity::getStackFaces()
	{
		long nStackFaces = 0;

		// vertex voxels contribute 3 faces
		// this could be taken out into a variable to avoid recalculating it
		// nStackFaces += getStackVertices(stack) * 3;

		// edge voxels contribute 2 faces
		// this could be taken out into a variable to avoid recalculating it
		// nStackFaces += getStackEdges(stack) * 2;

		// top and bottom faces
		for (int z = 0; z < m_nOriImagePiece; z += m_nOriImagePiece - 1) {
			for (int y = 1; y < m_nHeight - 1; y++) {
				for (int x = 1; x < m_nWidth - 1; x++) {
					if (getPixel(x, y, z) == -1)
						nStackFaces++;
				}
			}
			if (m_nOriImagePiece == 1)
				break;
		}

		// back and front faces
		for (int y = 0; y < m_nHeight; y += m_nHeight - 1) {
			for (int z = 1; z < m_nOriImagePiece - 1; z++) {
				for (int x = 1; x < m_nWidth - 1; x++) {
					if (getPixel(x, y, z) == -1)
						nStackFaces++;
				}
				if (m_nOriImagePiece == 1)
					break;
			}
		}

		// left and right faces
		for (int x = 0; x < m_nWidth; x += m_nWidth - 1) {
			for (int y = 1; y < m_nHeight - 1; y++) {
				for (int z = 1; z < m_nOriImagePiece - 1; z++) {
					if (getPixel(x, y, z) == -1)
						nStackFaces++;
					if (m_nOriImagePiece == 1)
						break;
				}
			}
		}
		return nStackFaces;
	}

	long SeBoneConnectivity::getStackEdges()
	{
		long nStackEdges = 0;

		// vertex voxels contribute 3 edges
		// this could be taken out into a variable to avoid recalculating it
		// nStackEdges += getStackVertices(stack) * 3; = f * 3;

		// left to right stack edges
		for (int z = 0; z < m_nOriImagePiece; z += m_nOriImagePiece - 1) {
			for (int y = 0; y < m_nHeight; y += m_nHeight - 1) {
				for (int x = 1; x < m_nWidth - 1; x++) {
					if (getPixel(x, y, z) == -1)
						nStackEdges++;
				}
			}
			if (m_nOriImagePiece == 1)
				break;
		}

		// back to front stack edges
		for (int z = 0; z < m_nOriImagePiece; z += m_nOriImagePiece - 1) {
			for (int x = 0; x < m_nWidth; x += m_nWidth - 1) {
				for (int y = 1; y < m_nHeight - 1; y++) {
					if (getPixel(x, y, z) == -1)
						nStackEdges++;
				}
			}
			if (m_nOriImagePiece == 1)
				break;
		}

		// top to bottom stack edges
		for (int y = 0; y < m_nHeight; y += m_nHeight - 1) {
			for (int x = 0; x < m_nWidth; x += m_nWidth - 1) {
				for (int z = 1; z < m_nOriImagePiece - 1; z++) {
					if (getPixel(x, y, z) == -1)
						nStackEdges++;
					if (m_nOriImagePiece == 1)
						break;
				}
			}
		}
		return nStackEdges;
	}

	long SeBoneConnectivity::getStackVertices()
	{
		long nStackVertices = 0;
		for (int z = 0; z < m_nOriImagePiece; z += m_nOriImagePiece - 1) {
			for (int y = 0; y < m_nHeight; y += m_nHeight - 1) {
				for (int x = 0; x < m_nWidth; x += m_nWidth - 1) {
					if (getPixel(x, y, z) == -1)
						nStackVertices++;
				}
			}
			if (m_nOriImagePiece == 1)
				break;
		}
		return nStackVertices;
	}
