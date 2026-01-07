#pragma once
#include <vector>
#include <utility>
#include <queue>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <omp.h>

struct Point2D {
    int x, y;
    Point2D(int _x, int _y) : x(_x), y(_y) {}
};

class FatAnalysis {
public:
    // Returns <VisceralFatMask, SubcutaneousFatMask>
    // Input: density volume (short*), dimensions, threshold parameters
    static std::pair<unsigned char*, unsigned char*> SeprateFat(
        const short* pData,
        int width,
        int height,
        int depth,
        int nowPos,
        int minFat,
        int maxFat
    );

    // Returns vector of masks: [VisceralFat, SubcutaneousFat, LungMask, BoneMask]
    static std::vector<unsigned char*> SeprateLung(
        const short* pData,
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

private:
    static int calc_area(unsigned char* mask, int width, int height);
    static void ErodeMask(unsigned char* mask, int width, int height, int iterations = 1);
    static void DilateMask(unsigned char* mask, int width, int height, int iterations = 1);
    static void FatMainRegionByRadius(const unsigned char* fatMask, int width, int height, int depth, double ratio, unsigned char* mainRegionMask);
    static void Dilate3D(unsigned char* pData, int nWidth, int nHeight, int nLength, int nKernel);
    static void Corrosion3D(unsigned char* pData, int nWidth, int nHeight, int nLength, int nKernel);
    static void Fast3DFloodFill(unsigned char* mask, int width, int height, int length, int seedX, int seedY, int seedZ);
    static void MaskCombineWithDilatedPrev(unsigned char* curMask, unsigned char* lastMask, unsigned char* OriMask, int width, int height, int dilateIter, int mode);

    static int cross(const Point2D& o, const Point2D& a, const Point2D& b);
    static std::vector<Point2D> convexHull(std::vector<Point2D>& pts);
    static bool pointInPolygon(int x, int y, const std::vector<Point2D>& poly);
    static void BoneMaskConvexHullInternalMask(const unsigned char* boneMask, int width, int height, int depth, int start, int end, unsigned char* internalMask);
    static void FindConnectedCenters(const unsigned char* mask, int width, int height, std::vector<Point2D>& centers);
};
