#include "stdafx.h"
#include <queue>
#include <cstring>
#define NOMINMAX
#include <algorithm>
#include <utility>
#include "FatSeprater.h"
#include <omp.h>


// 只用物理核心数
// 6为你的物理核心数

// 辅助函数实现（直接复制你原来的实现即可）
int FatSeprater::calc_area(BYTE* mask, int width, int height) {
	int cnt = 0;
	for (int i = 0; i < width * height; ++i)
		if (mask[i]) cnt++;
	return cnt;
}

void FatSeprater::ErodeMask(BYTE* mask, int width, int height, int iterations) {
	BYTE* temp = new BYTE[width * height];
	for (int it = 0; it < iterations; ++it) {
		memcpy(temp, mask, width * height);
		for (int y = 1; y < height - 1; ++y) {
			BYTE* row = mask + y * width;
			BYTE* prow = temp + y * width;
			for (int x = 1; x < width - 1; ++x) {
				bool keep = true;
				for (int dy = -1; dy <= 1 && keep; ++dy) {
					for (int dx = -1; dx <= 1; ++dx) {
						if (!prow[x + dx + dy * width]) {
							keep = false;
							break;
						}
					}
				}
				row[x] = keep ? 255 : 0;
			}
		}
	}
	delete[] temp;
}

void FatSeprater::DilateMask(BYTE* mask, int width, int height, int iterations) {
	BYTE* temp = new BYTE[width * height];
	for (int it = 0; it < iterations; ++it) {
		memcpy(temp, mask, width * height);
		for (int y = 1; y < height - 1; ++y) {
			BYTE* row = mask + y * width;
			BYTE* prow = temp + y * width;
			for (int x = 1; x < width - 1; ++x) {
				if (prow[x]) {
					row[x - 1 - width] = 255;
					row[x - width] = 255;
					row[x + 1 - width] = 255;
					row[x - 1] = 255;
					row[x] = 255;
					row[x + 1] = 255;
					row[x - 1 + width] = 255;
					row[x + width] = 255;
					row[x + 1 + width] = 255;
				}
			}
		}
	}
	delete[] temp;
}

void FatSeprater::FatMainRegionByRadius(const BYTE* fatMask, int width, int height, int depth, double ratio, BYTE* mainRegionMask) {
	memset(mainRegionMask, 0, width * height * depth);

	for (int z = 0; z < depth; ++z) {
		theAppIVConfig.m_pILog->ProgressStepIt();
		const BYTE* layer = fatMask + z * width * height;

		double sumX = 0, sumY = 0;
		int count = 0;
		for (int i = 0; i < width * height; ++i) {
			if (layer[i] == 255) {
				int x = i % width;
				int y = i / width;
				sumX += x;
				sumY += y;
				count++;
			}
		}
		if (count == 0) continue;
		double cx = width / 2;
		double cy = sumY / count;
		double radius = sqrt(count * ratio / 3.1415926);

		int yStart = max(0, int(cy - radius));
		int yEnd = min(height - 1, int(cy + radius));
		int xStart = max(0, int(cx - radius));
		int xEnd = min(width - 1, int(cx + radius));
		vector<BYTE> region(width * height, 0);
		queue<pair<int, int> > q;
		for (int x = xStart; x <= xEnd; ++x) {
			int y = yStart;
			while (y <= yEnd) {
				while (y <= yEnd && layer[y * width + x] != 255) ++y;
				if (y > yEnd) break;
				int y1 = y;

				while (y <= yEnd && layer[y * width + x] == 255) ++y;
				int y2 = y - 1;
				int idx1 = y1 * width + x;
				int idx2 = y2 * width + x;
				if (!region[idx1]) {
					region[idx1] = 1;
					q.push(std::make_pair(x, y1));
				}
				if (y2 != y1 && !region[idx2]) {
					region[idx2] = 1;
					q.push(std::make_pair(x, y2));
				}
			}
		}

		const int dx[4] = {1, -1, 0, 0};
		const int dy[4] = {0, 0, 1, -1};
		while (!q.empty()) {
			std::pair<int, int> pt = q.front(); q.pop();
			int x = pt.first, y = pt.second;
			for (int d = 0; d < 4; ++d) {
				int nx = x + dx[d], ny = y + dy[d];
				if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
					int nidx = ny * width + nx;
					if (layer[nidx] == 255 && !region[nidx]) {
						region[nidx] = 1;
						q.push(std::make_pair(nx, ny));
					}
				}
			}
		}
		BYTE* outLayer = mainRegionMask + z * width * height;
		for (int i = 0; i < width * height; ++i) {
			if (region[i]) outLayer[i] = 255;
		}
	}
}

void FatSeprater::Dilate3D(BYTE* pData, int nWidth, int nHeight, int nLength, int nKernel) {
	LONGLONG totalSize = (LONGLONG)nWidth * nHeight * nLength;
	BYTE* pTmpData = new BYTE[totalSize];
	for (int n = 0; n < nKernel; n++) {
		memcpy(pTmpData, pData, totalSize);
#pragma omp parallel for
		for (int z = 1; z < nLength - 1; ++z) {
			for (int y = 1; y < nHeight - 1; ++y) {
				for (int x = 1; x < nWidth - 1; ++x) {
					LONGLONG idx = (LONGLONG)z * nHeight * nWidth + y * nWidth + x;
					if (pTmpData[idx] == 255) {
						for (int dz = -1; dz <= 1; ++dz)
							for (int dy = -1; dy <= 1; ++dy)
								for (int dx = -1; dx <= 1; ++dx)
									pData[(z + dz) * nHeight * nWidth + (y + dy) * nWidth + (x + dx)] = 255;
					}
				}
			}
		}
	}
	delete[] pTmpData;
}

void FatSeprater::Corrosion3D(BYTE* pData, int nWidth, int nHeight, int nLength, int nKernel) {
	LONGLONG totalSize = (LONGLONG)nWidth * nHeight * nLength;
	BYTE* pTmpData = new BYTE[totalSize];
	for (int n = 0; n < nKernel; n++) {
		memcpy(pTmpData, pData, totalSize);
#pragma omp parallel for
		for (int z = 1; z < nLength - 1; ++z) {
			for (int y = 1; y < nHeight - 1; ++y) {
				for (int x = 1; x < nWidth - 1; ++x) {
					bool erode = false;
					for (int dz = -1; dz <= 1 && !erode; ++dz)
						for (int dy = -1; dy <= 1 && !erode; ++dy)
							for (int dx = -1; dx <= 1; ++dx)
								if (pTmpData[(z + dz) * nHeight * nWidth + (y + dy) * nWidth + (x + dx)] == 0) {
									erode = true;
									goto Apply;
								}
Apply:
								pData[z * nHeight * nWidth + y * nWidth + x] = erode ? 0 : 255;
				}
			}
		}
	}
	delete[] pTmpData;
}

void FatSeprater::Fast3DFloodFill(BYTE* mask, int width, int height, int length, int seedX, int seedY, int seedZ) {
	if (!mask) return;
	LONGLONG totalSize = (LONGLONG)width * height * length;

	// 预分配visited和队列
	BYTE* visited = new BYTE[totalSize];
	memset(visited, 0, totalSize);

	int queueSize = 4096;
	LONGLONG* queue = new LONGLONG[queueSize];
	int qHead = 0, qTail = 0;

	// 以种子点为中心的3x3x3邻域全部作为初始种子
	for (int dz = -1; dz <= 1; ++dz) {
		int z = seedZ + dz;
		if (z < 0 || z >= length) continue;
		for (int dy = -1; dy <= 1; ++dy) {
			int y = seedY + dy;
			if (y < 0 || y >= height) continue;
			for (int dx = -1; dx <= 1; ++dx) {
				int x = seedX + dx;
				if (x < 0 || x >= width) continue;
				LONGLONG idx = (LONGLONG)z * width * height + y * width + x;
				if (mask[idx] == 255 && !visited[idx]) {
					visited[idx] = 1;
					queue[qTail++] = idx;
				}
			}
		}
	}

	// 6邻域偏移
	const int dx[6] = {-1, 1, 0, 0, 0, 0};
	const int dy[6] = {0, 0, -1, 1, 0, 0};
	const int dz[6] = {0, 0, 0, 0, -1, 1};

	while (qHead < qTail) {
		LONGLONG idx = queue[qHead++];
		int z = idx / (width * height);
		int y = (idx % (width * height)) / width;
		int x = idx % width;
		mask[idx] = 255;
		for (int d = 0; d < 6; ++d) {
			int nx = x + dx[d];
			int ny = y + dy[d];
			int nz = z + dz[d];
			if (nx < 0 || nx >= width || ny < 0 || ny >= height || nz < 0 || nz >= length)
				continue;
			LONGLONG nidx = (LONGLONG)nz * width * height + ny * width + nx;
			if (!visited[nidx] && mask[nidx] == 255) {
				visited[nidx] = 1;
				if (qTail == queueSize) {
					int newSize = queueSize * 2;
					LONGLONG* newQueue = new LONGLONG[newSize];
					memcpy(newQueue, queue, sizeof(LONGLONG) * queueSize);
					delete[] queue;
					queue = newQueue;
					queueSize = newSize;
				}
				queue[qTail++] = nidx;
			}
		}
	}

	// 清理未访问点
	for (LONGLONG i = 0; i < totalSize; ++i) {
		if (!visited[i]) mask[i] = 0;
	}
	delete[] visited;
	delete[] queue;
}

void FatSeprater::MaskCombineWithDilatedPrev(BYTE* curMask, BYTE* lastMask, BYTE*OriMask, int width, int height, int dilateIter, int mode)
{
	BYTE* dilated = new BYTE[width * height];
	memcpy(dilated, lastMask, width * height);
	DilateMask(dilated, width, height, 1);

	BYTE* left = new BYTE[width * height];
	memcpy(left, curMask, width * height);
	for (int i = 0; i < width * height; ++i)
		left[i] = (curMask[i] && !dilated[i]) ? 255 : 0;

	DilateMask(left, width, height, 1);
	BYTE* polyMask = new BYTE[width * height];
	memset(polyMask, 0, width * height);


	int xs = width, xe = -1;
	int ys = height, ye = -1;
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			if (lastMask[y * width + x] == 255) {
				if (x < xs) xs = x;
				if (x > xe) xe = x;
				if (y < ys) ys = y;
				if (y > ye) ye = y;
			}
		}
	}
	std::vector<bool> visited(width * height, false);
	int dx[4] = {1, -1, 0, 0};
	int dy[4] = {0, 0, 1, -1};

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			int idx = y * width + x;
			if (left[idx] == 255 && !visited[idx]) {
				std::queue<int> q;
				std::vector<int> region;
				int sumX = 0, sumY = 0, count = 0;
				int minX = width, maxX = -1, minY = height, maxY = -1;

				q.push(idx);
				visited[idx] = true;

				while (!q.empty()) {
					int cur = q.front(); q.pop();
					int cx = cur % width, cy = cur / width;
					region.push_back(cur);
					sumX += cx; sumY += cy;
					count++;

					if (cx < minX) minX = cx;
					if (cx > maxX) maxX = cx;
					if (cy < minY) minY = cy;
					if (cy > maxY) maxY = cy;

					for (int d = 0; d < 4; ++d) {
						int nx = cx + dx[d];
						int ny = cy + dy[d];
						if (nx < 0 || nx >= width || ny < 0 || ny >= height)
							continue;
						int nidx = ny * width + nx;
						if (left[nidx] == 255 && !visited[nidx]) {
							visited[nidx] = true;
							q.push(nidx);
						}
					}
				}
				int centerX = sumX / count;
				int centerY = sumY / count;

				bool centerInRect = (centerX >= xs && centerX <= xe && centerY >= ys && centerY <= ye);
				int bboxArea = (maxX - minX + 1) * (maxY - minY + 1);
				double ratio = (double)count / bboxArea;

				bool isCompact = (ratio > 0.4);  

				if (!(centerInRect && isCompact)) {

					for (int i=0; i<region.size(); i++  ) {
						curMask[region[i]] = 0;
						OriMask[region[i]] = 255;
					}
				}
			}
		}
	}

	delete[] dilated;
	delete[] left;
	delete[] polyMask;
}



// 主分割函数
std::pair<BYTE*, BYTE*> FatSeprater::SeprateFat(
	CDcmPicArray* dcmArray,
	int width,
	int height,
	int depth,
	int nowPos,
	int minFat,
	int maxFat
	) {
		omp_set_num_threads(6); 
		int totalSize = width * height * depth;

		BYTE* fatMask = new BYTE[totalSize];
		memset(fatMask, 0, totalSize);

		BYTE* mainRegionMaskZ = new BYTE[totalSize];
		memset(mainRegionMaskZ, 0, totalSize);

		std::vector<int> areas(depth, 0);
		int max_area = 0;
		int start = nowPos, end = nowPos;

		theAppIVConfig.m_pILog->ProgressInit(depth);

		// 1. 先处理nowPos层
		{
			CDcmPic* pDcm = dcmArray->GetDcmArray()[nowPos];
			short* pData = (short*)pDcm->GetData();
			BYTE* layerFatMask = new BYTE[width * height];
			memset(layerFatMask, 0, width * height);

			for (int j = 0; j < width * height; ++j)
				if (pData[j] >= minFat && pData[j] <= maxFat)
					layerFatMask[j] = 255;
			ErodeMask(layerFatMask, width, height, 1);
			DilateMask(layerFatMask, width, height, 2);
			for (int j = 0; j < width * height; ++j)
				if (!(pData[j] >= minFat && pData[j] <= maxFat))
					layerFatMask[j] = 0;
			memcpy(fatMask + nowPos * width * height, layerFatMask, width * height);

			FatMainRegionByRadius(layerFatMask, width, height, 1, 0.6, mainRegionMaskZ + nowPos * width * height);
			areas[nowPos] = calc_area(mainRegionMaskZ + nowPos * width * height, width, height);
			max_area = areas[nowPos];

			delete[] layerFatMask;
			theAppIVConfig.m_pILog->ProgressStepIt();
		}
		double max_ratio = 0.0;
		// 2. 向上递推
		int last_area = areas[nowPos];
		for (int z = nowPos - 1; z >= 0; --z) {
			CDcmPic* pDcm = dcmArray->GetDcmArray()[z];
			short* pData = (short*)pDcm->GetData();
			BYTE* layerFatMask = new BYTE[width * height];
			memset(layerFatMask, 0, width * height);

			for (int j = 0; j < width * height; ++j)
				if (pData[j] >= minFat && pData[j] <= maxFat)
					layerFatMask[j] = 255;
			ErodeMask(layerFatMask, width, height, 1);
			DilateMask(layerFatMask, width, height, 2);
			for (int j = 0; j < width * height; ++j)
				if (!(pData[j] >= minFat && pData[j] <= maxFat))
					layerFatMask[j] = 0;
			memcpy(fatMask + z * width * height, layerFatMask, width * height);

			FatMainRegionByRadius(layerFatMask, width, height, 1, 0.6, mainRegionMaskZ + z * width * height);
			areas[z] = calc_area(mainRegionMaskZ + z * width * height, width, height);

			int cur_area = areas[z];
			if (cur_area > max_area) max_area = cur_area;

			// 判断是否到start
			if (cur_area < max_area * 0.05) {
				delete[] layerFatMask;
				break;
			}
			double ratio = (last_area == 0) ? 0 : double(cur_area) / last_area;
			if  (ratio > max_ratio) max_ratio = ratio;
			if (ratio > 3.0) {
				start = z + 1;
				delete[] layerFatMask;
				break;
			}
			if (ratio > 1.1) {
				BYTE* lastMask = mainRegionMaskZ + (z + 1) * width * height;
				// 这里的origFatMask可以传fatMask + z * width * height
				MaskCombineWithDilatedPrev(mainRegionMaskZ + z * width * height, lastMask, fatMask + z * width * height, width, height, 2, 1);
				cur_area = calc_area(mainRegionMaskZ + z * width * height, width, height);
			}
			last_area = cur_area;
			delete[] layerFatMask;
			theAppIVConfig.m_pILog->ProgressStepIt();
		}

		// 3. 向下递推
		last_area = areas[nowPos];
		for (int z = nowPos + 1; z < depth; ++z) {
			CDcmPic* pDcm = dcmArray->GetDcmArray()[z];
			short* pData = (short*)pDcm->GetData();
			BYTE* layerFatMask = new BYTE[width * height];
			memset(layerFatMask, 0, width * height);

			for (int j = 0; j < width * height; ++j)
				if (pData[j] >= minFat && pData[j] <= maxFat)
					layerFatMask[j] = 255;
			ErodeMask(layerFatMask, width, height, 1);
			DilateMask(layerFatMask, width, height, 2);
			for (int j = 0; j < width * height; ++j)
				if (!(pData[j] >= minFat && pData[j] <= maxFat))
					layerFatMask[j] = 0;
			memcpy(fatMask + z * width * height, layerFatMask, width * height);

			FatMainRegionByRadius(layerFatMask, width, height, 1, 0.6, mainRegionMaskZ + z * width * height);
			areas[z] = calc_area(mainRegionMaskZ + z * width * height, width, height);

			int cur_area = areas[z];
			if (cur_area > max_area) max_area = cur_area;

			// 判断是否到end
			if (cur_area < max_area * 0.05) {
				end = z - 1;
				delete[] layerFatMask;
				break;
			}
			double ratio = (last_area == 0) ? 0 : double(cur_area) / last_area;
			if (ratio > 3.0) {
				delete[] layerFatMask;
				break;
			}

			if (ratio > 1.1) {
				BYTE* lastMask = mainRegionMaskZ + (z - 1) * width * height;
				// 这里的origFatMask可以传fatMask + z * width * height
				MaskCombineWithDilatedPrev(mainRegionMaskZ + z * width * height, lastMask, fatMask + z * width * height, width, height, 2, 1);
				cur_area = calc_area(mainRegionMaskZ + z * width * height, width, height);
			}

			last_area = cur_area;
			delete[] layerFatMask;
			theAppIVConfig.m_pILog->ProgressStepIt();
		}

		// 4. 后续所有mask操作只在[start, end]层
		// 后续所有mask操作只在[start, end]层
		// 1. 腹壁mask三维膨胀，得到腹腔mask
		BYTE* abdCavityMask = new BYTE[totalSize];
		memcpy(abdCavityMask, mainRegionMaskZ, totalSize);
		Dilate3D(abdCavityMask, width, height, depth, 5);


		// 2. 脂肪mask与腹腔mask做差，得到腹腔内脂肪候选
		BYTE* visceralFatCandidate = new BYTE[totalSize];
		for (int i = 0; i < totalSize; ++i)
			visceralFatCandidate[i] = (fatMask[i] && !abdCavityMask[i]) ? 255 : 0;


		// 3. 腹腔内脂肪候选腐蚀
		BYTE* visceralFatErode = new BYTE[totalSize];
		memcpy(visceralFatErode, visceralFatCandidate, totalSize);
		Corrosion3D(visceralFatErode, width, height, depth, 1);


		// 4. 腹部脂肪连通域
		//int seedY = 0, seedX = width / 2, seedZ = nowPos;
		//for (int dx = 0; dx < height / 2; ++dx) {
		//	if (visceralFatErode[seedZ * width * height + (seedY + dx) * width + seedX]) { seedY = seedY + dx; break; }
		//}
		//BYTE* visceralFatMask = new BYTE[totalSize];
		//memset(visceralFatMask, 0, totalSize);
		//if (seedX >= 0) {

		//	memcpy(visceralFatMask, visceralFatErode, totalSize);
		//	Fast3DFloodFill(visceralFatMask, width, height, depth, seedX, seedY, seedZ);
		//}

		//seedY = height / 2, seedX = 0, seedZ = nowPos;
		//for (int dx = 0; dx < width / 2; ++dx) {
		//	if (visceralFatErode[seedZ * width * height + seedY  * width + seedX + dx]) { seedX = seedX + dx; break; }
		//}
		//BYTE* visceralFatMask2 = new BYTE[totalSize];
		//memset(visceralFatMask2, 0, totalSize);
		//if (seedX >= 0) {
		//	memcpy(visceralFatMask2, visceralFatErode, totalSize);
		//	Fast3DFloodFill(visceralFatMask2, width, height, depth, seedX, seedY, seedZ);
		//}

		//for (int i=0; i<totalSize; i++) {
		//	if (visceralFatMask2[i] == 255)
		//		visceralFatMask[i] = 255;
		//}

		int seedY = 0, seedX = width / 2, seedZ = nowPos;
		// 从上往下找
		for (int dy = 0; dy < height / 2; ++dy) {
			if (visceralFatErode[seedZ * width * height + (seedY + dy) * width + seedX]) {
				seedY = seedY + dy;
				break;
			}
		}
		BYTE* visceralFatMask = new BYTE[totalSize];
		memset(visceralFatMask, 0, totalSize);
		if (seedX >= 0 && seedY >= 0) {
			memcpy(visceralFatMask, visceralFatErode, totalSize);
			Fast3DFloodFill(visceralFatMask, width, height, depth, seedX, seedY, seedZ);
		}

		// 从左往右找
		seedY = height / 2, seedX = 0, seedZ = nowPos;
		for (int dx = 0; dx < width / 2; ++dx) {
			if (visceralFatErode[seedZ * width * height + seedY * width + seedX + dx]) {
				seedX = seedX + dx;
				break;
			}
		}
		BYTE* visceralFatMask2 = new BYTE[totalSize];
		memset(visceralFatMask2, 0, totalSize);
		if (seedX >= 0 && seedY >= 0) {
			memcpy(visceralFatMask2, visceralFatErode, totalSize);
			Fast3DFloodFill(visceralFatMask2, width, height, depth, seedX, seedY, seedZ);
		}
		// 合并
		for (int i = 0; i < totalSize; i++) {
			if (visceralFatMask2[i] == 255)
				visceralFatMask[i] = 255;
		}
		delete[] visceralFatMask2;

		// 从右往左找
		seedY = height / 2, seedX = width - 1, seedZ = nowPos;
		for (int dx = 0; dx < width / 2; ++dx) {
			if (visceralFatErode[seedZ * width * height + seedY * width + seedX - dx]) {
				seedX = seedX - dx;
				break;
			}
		}
		BYTE* visceralFatMask3 = new BYTE[totalSize];
		memset(visceralFatMask3, 0, totalSize);
		if (seedX >= 0 && seedY >= 0) {
			memcpy(visceralFatMask3, visceralFatErode, totalSize);
			Fast3DFloodFill(visceralFatMask3, width, height, depth, seedX, seedY, seedZ);
		}
		// 合并
		for (int i = 0; i < totalSize; i++) {
			if (visceralFatMask3[i] == 255)
				visceralFatMask[i] = 255;
		}
		delete[] visceralFatMask3;

		// 5. 腹部脂肪膨胀，得到皮下脂肪候选
		BYTE* visceralFatDilate = new BYTE[totalSize];
		memcpy(visceralFatDilate, visceralFatMask, totalSize);
		Dilate3D(visceralFatDilate, width, height, depth, 6);


		BYTE* subcutaneousFatCandidate = new BYTE[totalSize];
		for (int i = 0; i < totalSize; ++i)
			subcutaneousFatCandidate[i] = (fatMask[i] && !visceralFatDilate[i]) ? 255 : 0;
		Corrosion3D(subcutaneousFatCandidate, width, height, depth, 1);

		// 6. 皮下脂肪连通域
		int subSeedZ = seedZ, subSeedY = height / 2, subSeedX = width / 2;
		for (int dx = 0; dx < width / 2; ++dx) {
			int x1 = width / 2 + dx, x2 = width / 2 - 1 - dx;
			if (subcutaneousFatCandidate[subSeedZ * width * height + subSeedY * width + x1]) { subSeedX = x1; break; }
			if (subcutaneousFatCandidate[subSeedZ * width * height + subSeedY * width + x2]) { subSeedX = x2; break; }
		}
		BYTE* subcutaneousFatMask = new BYTE[totalSize];
		memset(subcutaneousFatMask, 0, totalSize);
		if (subSeedX >= 0) {
			memcpy(subcutaneousFatMask, subcutaneousFatCandidate, totalSize);
			Fast3DFloodFill(subcutaneousFatMask, width, height, depth, subSeedX, subSeedY, subSeedZ);
		}

		// 皮下mask膨胀
		Dilate3D(subcutaneousFatMask, width, height, depth, 6);

		// 7. 最终腹部/皮下脂肪
		BYTE* finalVisceralFat = new BYTE[totalSize];
		BYTE* finalSubcutaneousFat = new BYTE[totalSize];
		for (int i = 0; i < totalSize; ++i) {
			finalVisceralFat[i] = (fatMask[i] && visceralFatDilate[i]) ? 255 : 0;
			finalSubcutaneousFat[i] = (fatMask[i] && subcutaneousFatMask[i]) ? 255 : 0;
		}

		// 清理
		delete[] mainRegionMaskZ;
		delete[] abdCavityMask;
		delete[] visceralFatCandidate;
		delete[] visceralFatErode;
		delete[] visceralFatMask;
		delete[] visceralFatDilate;
		delete[] subcutaneousFatCandidate;
		delete[] subcutaneousFatMask;
		delete[] fatMask;

		theAppIVConfig.m_pILog->ProgressClose();

		// 返回两个mask
		return std::make_pair(finalVisceralFat, finalSubcutaneousFat);
}

vector<BYTE*> FatSeprater::SeprateLung(CDcmPicArray* dcmArray, int width, int height, int depth, int minFat, int maxFat, int minLung, int maxLung, int minBone,int maxBone)
{
	LONGLONG totalSize = width * height * depth;
	theAppIVConfig.m_pILog->ProgressInit(depth); 
	BYTE* fatMask = new BYTE[totalSize];
	memset(fatMask, 0, totalSize);

	BYTE* lungMask = new BYTE[totalSize];
	memset(lungMask, 0, totalSize);

	BYTE* boneMask = new BYTE[totalSize];
	memset(boneMask, 0, totalSize);

	std::vector<int> lungAreas(depth, 0);

	// 1. 生成肺部mask并计算面积
	for (int z = 0; z < depth; ++z) {
		CDcmPic* pDcm = dcmArray->GetDcmArray()[z];
		short* pData = (short*)pDcm->GetData();

		// 先生成mask
		for (int i = 0; i < width * height; ++i) {
			short v = pData[i];
			lungMask[z * width * height + i] = (v >= minLung && v <= maxLung) ? 255 : 0;
		}

		// 先腐蚀再膨胀
		ErodeMask(lungMask + z * width * height, width, height, 1);
		DilateMask(lungMask + z * width * height, width, height, 1);

		// 再统计面积
		int area = 0;
		for (int i = 0; i < width * height; ++i) {
			if (lungMask[z * width * height + i] == 255)
				area++;
		}
		lungAreas[z] = area;

		theAppIVConfig.m_pILog->ProgressStepIt();
	}

	// 2. 找最大面积层
	int startZ, endZ;
	int maxArea = 0, maxZ = 0;
	for (int z = 0; z < depth; ++z) {
		if (lungAreas[z] > maxArea) { maxArea = lungAreas[z]; maxZ = z; }
	}

	// === 新增：在最大面积层中心点左右各找一个属于肺的点，做3D flood fill ===
	int centerX = width / 2;
	int centerY = height / 2;
	int seedLeft = -1, seedRight = -1;

	// 向左找
	for (int x = centerX; x >= 0; --x) {
		int idx = maxZ * width * height + centerY * width + x;
		if (lungMask[idx] == 255) {
			seedLeft = x;
			break;
		}
	}
	// 向右找
	for (int x = centerX + 1; x < width; ++x) {
		int idx = maxZ * width * height + centerY * width + x;
		if (lungMask[idx] == 255) {
			seedRight = x;
			break;
		}
	}

	// flood fill函数（可复用你的Fast3DFloodFill）
	if (seedLeft != -1) {
		FatSeprater::Fast3DFloodFill(lungMask, width, height, depth, seedLeft, centerY, maxZ);
	}
	if (seedRight != -1) {
		FatSeprater::Fast3DFloodFill(lungMask, width, height, depth, seedRight, centerY, maxZ);
	}

	// 3. 往上找
	startZ = maxZ;
	for (int z = maxZ; z >= 0; --z) {
		if (lungAreas[z] < maxArea * 0.05) {
			startZ = z + 1;
			break;
		}
	}
	// 4. 往下找
	endZ = maxZ;
	for (int z = maxZ; z < depth; ++z) {
		if (lungAreas[z] < maxArea * 0.05) {
			endZ = z - 1;
			break;
		}
	}


	// 5. 只在[startZ, endZ]生成脂肪和骨头mask
	BYTE* layerFat = new BYTE[width * height];
	for (int z = startZ; z <= endZ; ++z) {
		CDcmPic* pDcm = dcmArray->GetDcmArray()[z];
		short* pData = (short*)pDcm->GetData();

		for (int i = 0; i < width * height; ++i) {
			short v = pData[i];
			layerFat[i] = (v >= minFat && v <= maxFat) ? 255 : 0;
			boneMask[z * width * height + i] = (v >= minBone && v <= maxBone) ? 255 : 0;
		}
		ErodeMask(layerFat, width, height, 1);
		DilateMask(layerFat, width, height, 2);
		for (int i = 0; i < width * height; ++i) {
			if (!(pData[i] >= minFat && pData[i] <= maxFat))
				layerFat[i] = 0;
			fatMask[z * width * height + i] = layerFat[i];
		}
		theAppIVConfig.m_pILog->ProgressStepIt();
	}
	delete[] layerFat;

	// 6. 骨头FloodFill（上下左右各找一个种子点）
	centerY = height / 2, centerX = width / 2;
	int boneSeedUp = -1, boneSeedDown = -1, boneSeedLeft = -1, boneSeedRight = -1;
	for (int dy = 0; dy < height / 2; ++dy) {
		if (boneMask[maxZ * width * height + (centerY - dy) * width + centerX]) {
			boneSeedUp = centerY - dy;
			break;
		}
	}
	for (int dy = 0; dy < height / 2; ++dy) {
		if (boneMask[maxZ * width * height + (centerY + dy) * width + centerX]) {
			boneSeedDown = centerY + dy;
			break;
		}
	}
	for (int dx = 0; dx < width / 2; ++dx) {
		if (boneMask[maxZ * width * height + centerY * width + centerX - dx]) {
			boneSeedLeft = centerX - dx;
			break;
		}
	}
	for (int dx = 0; dx < width / 2; ++dx) {
		if (boneMask[maxZ * width * height + centerY * width + centerX + dx]) {
			boneSeedRight = centerX + dx;
			break;
		}
	}
	if (boneSeedUp >= 0)
		FatSeprater::Fast3DFloodFill(boneMask, width, height, depth, centerX, boneSeedUp, maxZ);
	if (boneSeedDown >= 0)
		FatSeprater::Fast3DFloodFill(boneMask, width, height, depth, centerX, boneSeedDown, maxZ);
	if (boneSeedLeft >= 0)
		FatSeprater::Fast3DFloodFill(boneMask, width, height, depth, boneSeedLeft, centerY, maxZ);
	if (boneSeedRight >= 0)
		FatSeprater::Fast3DFloodFill(boneMask, width, height, depth, boneSeedRight, centerY, maxZ);

	// 7. 生成骨头凸包mask
	BYTE* internalMask = new BYTE[width * height * depth];
	memset(internalMask, 0, width * height * depth);
	BoneMaskConvexHullInternalMask(boneMask, width, height, depth, startZ, endZ, internalMask);

	// 8. 求交集得到内脏脂肪
	BYTE* visceralFatMask = new BYTE[totalSize];
	memset(visceralFatMask, 0, totalSize);
	BYTE* subcutaneousFatMask = new BYTE[totalSize];
	memset(subcutaneousFatMask, 0, totalSize);

	for (int z = startZ; z <= endZ; ++z) {
		for (int i = 0; i < width * height; ++i) {
			int idx = z * width * height + i;
			if (fatMask[idx] && internalMask[idx])
				visceralFatMask[idx] = 255;
			else if (fatMask[idx])
				subcutaneousFatMask[idx] = 255;
		}
	}

	delete[] fatMask;
	//delete[] internalMask;

	theAppIVConfig.m_pILog->ProgressClose();
	vector<BYTE*> masks;
	masks.push_back(visceralFatMask);
	masks.push_back(subcutaneousFatMask);
	masks.push_back(lungMask);
	masks.push_back(boneMask);
	return masks;
}

int FatSeprater::cross(const Point2D& o, const Point2D& a, const Point2D& b)
{
	return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
}

std::vector<Point2D> FatSeprater::convexHull(std::vector<Point2D>& pts)
{
	if (pts.size() < 3) return pts;
	std::sort(pts.begin(), pts.end(), [](const Point2D& a, const Point2D& b) {
		return a.x < b.x || (a.x == b.x && a.y < b.y);
	});
	std::vector<Point2D> hull;
	// 下凸包
	for (int i=0; i<pts.size(); i++) {
		auto p = pts[i];
		while (hull.size() >= 2 && cross(hull[hull.size()-2], hull.back(), p) <= 0)
			hull.pop_back();
		hull.push_back(p);
	}
	// 上凸包
	size_t t = hull.size() + 1;
	for (int i = (int)pts.size() - 2; i >= 0; --i) {
		while (hull.size() >= t && cross(hull[hull.size()-2], hull.back(), pts[i]) <= 0)
			hull.pop_back();
		hull.push_back(pts[i]);
	}
	hull.pop_back();
	return hull;
}

bool FatSeprater::pointInPolygon(int x, int y, const std::vector<Point2D>& poly)
{
	int cnt = 0, n = poly.size();
	for (int i = 0; i < n; ++i) {
		const Point2D& a = poly[i];
		const Point2D& b = poly[(i+1)%n];
		if (a.y == b.y) continue;
		if (y < min(a.y, b.y) || y >= max(a.y, b.y)) continue;
		double tx = (double)(y - a.y) * (b.x - a.x) / (b.y - a.y) + a.x;
		if (tx > x) cnt++;
	}
	return cnt % 2 == 1;
}

void FatSeprater::DrawEllipse(BYTE* mask, int width, int height, double cx, double cy, double a, double b, double theta)
{
	double cosT = cos(theta), sinT = sin(theta);
	for (int y = 0; y < height; ++y)
		for (int x = 0; x < width; ++x) {
			double dx = x - cx, dy = y - cy;
			double xRot =  cosT * dx + sinT * dy;
			double yRot = -sinT * dx + cosT * dy;
			if ((xRot * xRot) / (a * a) + (yRot * yRot) / (b * b) <= 1.0)
				mask[y * width + x] = 255;
		}
}

void FatSeprater::FindConnectedCenters(const BYTE* mask, int width, int height, std::vector<Point2D>& centers)
{
	int layerSize = width * height;
	std::vector<bool> visited(layerSize, false);
	for (int i = 0; i < layerSize; ++i) {
		if (mask[i] && !visited[i]) {
			// BFS
			std::queue<int> q;
			q.push(i);
			visited[i] = true;
			int sumX = 0, sumY = 0, count = 0;
			while (!q.empty()) {
				int idx = q.front(); q.pop();
				int x = idx % width, y = idx / width;
				sumX += x; sumY += y; count++;
				// 4邻域
				const int dx[4] = {1, -1, 0, 0};
				const int dy[4] = {0, 0, 1, -1};
				for (int d = 0; d < 4; ++d) {
					int nx = x + dx[d], ny = y + dy[d];
					if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
						int nidx = ny * width + nx;
						if (mask[nidx] && !visited[nidx]) {
							visited[nidx] = true;
							q.push(nidx);
						}
					}
				}
			}
			if (count > 0) {
				int cx = sumX / count;
				int cy = sumY / count;
				centers.push_back(Point2D(cx, cy));
			}
		}
	}
}


void FatSeprater::BoneMaskConvexHullInternalMask(const BYTE* boneMask, int width, int height, int depth, int start, int end, BYTE* internalMask)
{
	memset(internalMask, 0, width * height * depth);

	std::vector< std::vector<Point2D> > hulls(end - start);
	std::vector<double> areas(end - start, 0.0);

	// 1. 计算每层凸包和面积
#pragma omp parallel for
	for (int z = start; z < end; ++z) {
		theAppIVConfig.m_pILog->ProgressStepIt();
		std::vector<Point2D> pts;
		int minX = width, maxX = -1, minY = height, maxY = -1;
		for (int y = 0; y < height; ++y)
			for (int x = 0; x < width; ++x)
				if (boneMask[z * width * height + y * width + x]) {
					pts.push_back(Point2D(x, y));
					if (x < minX) minX = x;
					if (x > maxX) maxX = x;
					if (y < minY) minY = y;
					if (y > maxY) maxY = y;
				}
				if (pts.size() < 3) continue;
				std::vector<Point2D> hull = convexHull(pts);
				hulls[z - start] = hull;

				// 计算面积
				double area = 0.0;
				int n = hull.size();
				for (int i = 0; i < n; ++i) {
					const Point2D& a = hull[i];
					const Point2D& b = hull[(i + 1) % n];
					area += (double)a.x * b.y - (double)a.y * b.x;
				}
				areas[z - start] = fabs(area) / 2.0;

				// 保存ROI
				if (minX > maxX || minY > maxY) {
					minX = 0; maxX = width - 1; minY = 0; maxY = height - 1;
				}
				// 存到hulls后面或新vector都可以
				hull.push_back(Point2D(minX, minY)); // 仅示例，实际可单独存ROI
				hull.push_back(Point2D(maxX, maxY));
	}

	// 2. 找最大面积层
	//int maxIdx = 0;
	//double maxArea = 0.0;
	//for (int i = 0; i < areas.size(); ++i) {
	//	if (areas[i] > maxArea) {
	//		maxArea = areas[i];
	//		maxIdx = i;
	//	}
	//}

	// 3. 面积突变修正
	//for (int i = maxIdx - 1; i >= 0; --i) {
	//	if (areas[i] < 1e-6 || areas[i + 1] < 1e-6) continue;
	//	double ratio = areas[i] / areas[i + 1];
	//	if (ratio < 1.0) {
	//		hulls[i] = hulls[i + 1];
	//		areas[i] = areas[i + 1];
	//	}
	//}
	//for (int i = maxIdx + 1; i < hulls.size(); ++i) {
	//	if (areas[i] < 1e-6 || areas[i - 1] < 1e-6) continue;
	//	double ratio = areas[i] / areas[i - 1];
	//	if (ratio < 1.0) {
	//		hulls[i] = hulls[i - 1];
	//		areas[i] = areas[i - 1];
	//	}
	//}

	// 4. 填充mask（只在ROI内判断）
#pragma omp parallel for
	for (int z = start; z < end; ++z) {
		theAppIVConfig.m_pILog->ProgressStepIt();
		const std::vector<Point2D>& hull = hulls[z - start];
		if (hull.size() < 3) continue;

		// 取ROI
		int minX = width, maxX = -1, minY = height, maxY = -1;
		for (size_t i = 0; i < hull.size(); ++i) {
			if (hull[i].x < minX) minX = hull[i].x;
			if (hull[i].x > maxX) maxX = hull[i].x;
			if (hull[i].y < minY) minY = hull[i].y;
			if (hull[i].y > maxY) maxY = hull[i].y;
		}
		minX = max(0, minX); maxX = min(width - 1, maxX);
		minY = max(0, minY); maxY = min(height - 1, maxY);

		for (int y = minY; y <= maxY; ++y)
			for (int x = minX; x <= maxX; ++x)
				if (pointInPolygon(x, y, hull))
					internalMask[z * width * height + y * width + x] = 255;
		// 进度条可放主线程
	}

	//int midIdx = (end - start) / 2;

	//// 3. 从中间层向上修正
	//for (int i = midIdx - 1; i >= 0; --i) {
	//	theAppIVConfig.m_pILog->ProgressStepIt();
	//	// 生成当前层和前一层的凸包mask
	//	BYTE* maskCur = new BYTE[width * height];
	//	BYTE* maskPrev = new BYTE[width * height];
	//	memset(maskCur, 0, width * height);
	//	memset(maskPrev, 0, width * height);

	//	// 当前层
	//	const std::vector<Point2D>& hullCur = hulls[i];
	//	if (hullCur.size() >= 3) {
	//		int minX = width, maxX = -1, minY = height, maxY = -1;
	//		for (size_t j = 0; j < hullCur.size(); ++j) {
	//			if (hullCur[j].x < minX) minX = hullCur[j].x;
	//			if (hullCur[j].x > maxX) maxX = hullCur[j].x;
	//			if (hullCur[j].y < minY) minY = hullCur[j].y;
	//			if (hullCur[j].y > maxY) maxY = hullCur[j].y;
	//		}
	//		minX = std::max(0, minX); maxX = std::min(width - 1, maxX);
	//		minY = std::max(0, minY); maxY = std::min(height - 1, maxY);
	//		for (int y = minY; y <= maxY; ++y)
	//			for (int x = minX; x <= maxX; ++x)
	//				if (pointInPolygon(x, y, hullCur))
	//					maskCur[y * width + x] = 255;
	//	}
	//	// 前一层
	//	const std::vector<Point2D>& hullPrev = hulls[i + 1];
	//	if (hullPrev.size() >= 3) {
	//		int minX = width, maxX = -1, minY = height, maxY = -1;
	//		for (size_t j = 0; j < hullPrev.size(); ++j) {
	//			if (hullPrev[j].x < minX) minX = hullPrev[j].x;
	//			if (hullPrev[j].x > maxX) maxX = hullPrev[j].x;
	//			if (hullPrev[j].y < minY) minY = hullPrev[j].y;
	//			if (hullPrev[j].y > maxY) maxY = hullPrev[j].y;
	//		}
	//		minX = std::max(0, minX); maxX = std::min(width - 1, maxX);
	//		minY = std::max(0, minY); maxY = std::min(height - 1, maxY);
	//		for (int y = minY; y <= maxY; ++y)
	//			for (int x = minX; x <= maxX; ++x)
	//				if (pointInPolygon(x, y, hullPrev))
	//					maskPrev[y * width + x] = 255;
	//	}

	//	// 差集和并集
	//	int diff = 0, total = 0;
	//	for (int j = 0; j < width * height; ++j) {
	//		if (maskCur[j] || maskPrev[j]) total++;
	//		if (maskCur[j] != maskPrev[j]) diff++;
	//	}
	//	double diffRatio = (total > 0) ? (double)diff / total : 0.0;
	//	if (diffRatio > 0.05) {
	//		// 用并集
	//		for (int j = 0; j < width * height; ++j)
	//			maskCur[j] = (maskCur[j] || maskPrev[j]) ? 255 : 0;
	//		// 重新提取并集mask的点作为新的hull
	//		std::vector<Point2D> newPts;
	//		for (int j = 0; j < width * height; ++j)
	//			if (maskCur[j]) newPts.push_back(Point2D(j % width, j / width));
	//		hulls[i] = convexHull(newPts);
	//	}
	//	delete[] maskCur;
	//	delete[] maskPrev;
	//}

	//// 4. 从中间层向下修正
	//for (int i = midIdx + 1; i < (int)hulls.size(); ++i) {
	//	theAppIVConfig.m_pILog->ProgressStepIt();
	//	BYTE* maskCur = new BYTE[width * height];
	//	BYTE* maskPrev = new BYTE[width * height];
	//	memset(maskCur, 0, width * height);
	//	memset(maskPrev, 0, width * height);

	//	const std::vector<Point2D>& hullCur = hulls[i];
	//	if (hullCur.size() >= 3) {
	//		int minX = width, maxX = -1, minY = height, maxY = -1;
	//		for (size_t j = 0; j < hullCur.size(); ++j) {
	//			if (hullCur[j].x < minX) minX = hullCur[j].x;
	//			if (hullCur[j].x > maxX) maxX = hullCur[j].x;
	//			if (hullCur[j].y < minY) minY = hullCur[j].y;
	//			if (hullCur[j].y > maxY) maxY = hullCur[j].y;
	//		}
	//		minX = std::max(0, minX); maxX = std::min(width - 1, maxX);
	//		minY = std::max(0, minY); maxY = std::min(height - 1, maxY);
	//		for (int y = minY; y <= maxY; ++y)
	//			for (int x = minX; x <= maxX; ++x)
	//				if (pointInPolygon(x, y, hullCur))
	//					maskCur[y * width + x] = 255;
	//	}
	//	const std::vector<Point2D>& hullPrev = hulls[i - 1];
	//	if (hullPrev.size() >= 3) {
	//		int minX = width, maxX = -1, minY = height, maxY = -1;
	//		for (size_t j = 0; j < hullPrev.size(); ++j) {
	//			if (hullPrev[j].x < minX) minX = hullPrev[j].x;
	//			if (hullPrev[j].x > maxX) maxX = hullPrev[j].x;
	//			if (hullPrev[j].y < minY) minY = hullPrev[j].y;
	//			if (hullPrev[j].y > maxY) maxY = hullPrev[j].y;
	//		}
	//		minX = std::max(0, minX); maxX = std::min(width - 1, maxX);
	//		minY = std::max(0, minY); maxY = std::min(height - 1, maxY);
	//		for (int y = minY; y <= maxY; ++y)
	//			for (int x = minX; x <= maxX; ++x)
	//				if (pointInPolygon(x, y, hullPrev))
	//					maskPrev[y * width + x] = 255;
	//	}

	//	int diff = 0, total = 0;
	//	for (int j = 0; j < width * height; ++j) {
	//		if (maskCur[j] || maskPrev[j]) total++;
	//		if (maskCur[j] != maskPrev[j]) diff++;
	//	}
	//	double diffRatio = (total > 0) ? (double)diff / total : 0.0;
	//	if (diffRatio > 0.05) {
	//		for (int j = 0; j < width * height; ++j)
	//			maskCur[j] = (maskCur[j] || maskPrev[j]) ? 255 : 0;
	//		std::vector<Point2D> newPts;
	//		for (int j = 0; j < width * height; ++j)
	//			if (maskCur[j]) newPts.push_back(Point2D(j % width, j / width));
	//		hulls[i] = convexHull(newPts);
	//	}
	//	delete[] maskCur;
	//	delete[] maskPrev;
	//}
}
