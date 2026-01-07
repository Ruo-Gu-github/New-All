#pragma once
#include <vector>

struct Point2D {
	int x, y;
	Point2D(int _x, int _y) : x(_x), y(_y) {}
};

class FatSeprater {
public:
	// 返回值：pair<腹部脂肪mask, 皮下脂肪mask>
	static std::pair<BYTE*, BYTE*> SeprateFat(
		CDcmPicArray* dcmArray,
		int width,
		int height,
		int depth,
		int nowPos,
		int minFat,
		int maxFat
		);

	static vector<BYTE*> SeprateLung(
		CDcmPicArray* dcmArray,
		int width,
		int height,
		int depth,
		int minFat,
		int maxFat,
		int minLung,
		int maxLung,
		int minBone,
		int maxBone
		);

	// 下面是所有辅助函数声明
	static int calc_area(BYTE* mask, int width, int height);
	static void ErodeMask(BYTE* mask, int width, int height, int iterations = 1);
	static void DilateMask(BYTE* mask, int width, int height, int iterations = 1);
	static void FatMainRegionByRadius(const BYTE* fatMask, int width, int height, int depth, double ratio, BYTE* mainRegionMask);
	static void Dilate3D(BYTE* pData, int nWidth, int nHeight, int nLength, int nKernel);
	static void Corrosion3D(BYTE* pData, int nWidth, int nHeight, int nLength, int nKernel);
	static void Fast3DFloodFill(BYTE* mask, int width, int height, int length, int seedX, int seedY, int seedZ);
	static void MaskCombineWithDilatedPrev(BYTE* curMask, BYTE* lastMask, BYTE*OriMask, int width, int height, int dilateIter, int mode);

	static int cross(const Point2D& o, const Point2D& a, const Point2D& b);
	static std::vector<Point2D> convexHull(std::vector<Point2D>& pts);
	static bool pointInPolygon(int x, int y, const std::vector<Point2D>& poly);
	static void DrawEllipse(BYTE* mask, int width, int height, double cx, double cy, double a, double b, double theta);
	static void BoneMaskConvexHullInternalMask(const BYTE* boneMask, int width, int height, int depth, int start, int end, BYTE* internalMask);
	static void FindConnectedCenters(const BYTE* mask, int width, int height, std::vector<Point2D>& centers);
};