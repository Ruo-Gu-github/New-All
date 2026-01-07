#pragma once
#include <vector>
#include <map>
#include <string>
#include <cmath>
#include <algorithm>
#include <climits>
#include <omp.h>

class VascularAnalysis {
public:
    struct AnalysisResult {
        std::vector<float> thicknessMap;
        double meanDiameter;
        double maxDiameter;
        double stdDiameter;
        std::vector<size_t> histogram;
    };

    static AnalysisResult ComputeDiameter(
        const unsigned char* vesselMask, 
        int width, 
        int height, 
        int length,
        double pixelSize
    );

private:
   struct BASIC_INFO_THREE {
        size_t width;
        size_t height;
        size_t length;
        size_t start;
        size_t end;
        BASIC_INFO_THREE(size_t w, size_t h, size_t l, size_t s, size_t e) 
            : width(w), height(h), length(l), start(s), end(e) {}
    };

    struct NEW_POSITION {
        int x, y, z;
        float value;
        NEW_POSITION(int _x, int _y, int _z, float _value) 
            : x(_x), y(_y), z(_z), value(_value) {}
    };

    static void GeometryToDistance(const std::vector<unsigned char>& trinary_data, std::vector<float>& distance_map, size_t width, size_t height, size_t length);
    static void DistanceRidge(const std::vector<float>& distance_map, std::vector<float>& ridge_map, size_t width, size_t height, size_t length, float& max_radii);
    static void LocalThickness(std::vector<float>& ridge_map, std::vector<float>& max_radii_map, size_t width, size_t height, size_t length);
    static void ThicknessCleaning(std::vector<float>& radii_map, std::vector<float>& thickness_map, size_t width, size_t height, size_t length);
    
    static void CreateTemplate(std::vector<int>& distance_square_values, std::vector<std::vector<int>>& radii_square_template);
    static void ScanCube(const int x, const int y, const int z, std::vector<int>& distance_square_values, std::vector<int>& cube);
    static float SetFlag(const float* data, size_t i, size_t j, size_t k, size_t width, size_t height, size_t length);
    static float AverageInteriorNeighbors(const float* data, const float* ori, size_t i, size_t j, size_t k, size_t width, size_t height, size_t length);
    
    static inline float Look(const float* data, size_t i, size_t j, size_t k, size_t width, size_t height, size_t length) {
		if ((i < 0) || (i >= width) || (j < 0) || (j >= height) || (k < 0) || (k >= length )) return -1;
		return data[k * width * height + j * width + i];
	}

    static inline bool Is_Bigger_Square(const float* data, int x, int y, int z, size_t width, size_t height, size_t length, int point_square_index, const int* bigger_square_vector) {
		if ((x < 0) || (x >= width)  || (y < 0) || (y >= height) || (z < 0) || (z >= length)) return false;
		else if(static_cast<int>(data[width * height * z + width * y + x]) > bigger_square_vector[point_square_index]) return true;
		else return false;
	}
};
