#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>

class VascularAnalysis {
public:
    struct AnalysisResult {
        std::vector<float> thicknessMap;
        double meanDiameter;
        double maxDiameter;
        double stdDiameter;
        std::vector<uint64_t> histogram;
    };

    // Main Compute Function
    static AnalysisResult ComputeDiameter(const unsigned char* vesselMask, int width, int height, int length, double pixelSize = 1.0);

    // New Feature: Keep only the largest connected component (3D 6-connectivity)
    static void FilterKeepLargest(unsigned char* mask, int width, int height, int length);

private:
    struct NEW_POSITION {
        int x;
        int y;
        int z;
        float value;
        NEW_POSITION(int px, int py, int pz, float v) : x(px), y(py), z(pz), value(v) {}
    };

   static void GeometryToDistance(const std::vector<unsigned char>& trinary_data, std::vector<float>& distance_map, size_t width, size_t height, size_t length);
   static void DistanceRidge(const std::vector<float>& distance_map, std::vector<float>& ridge_map, size_t width, size_t height, size_t length, float& max_radii);
    static void LocalThickness(std::vector<float>& ridge_map, std::vector<float>& max_radii_map, size_t width, size_t height, size_t length);
    static void ThicknessCleaning(std::vector<float>& max_radii_map, std::vector<float>& thickness_map, size_t width, size_t height, size_t length);

    // Helper utilities (scaffolding to keep compilation stable)
    static void CreateTemplate(std::vector<int>& distance_square_values, std::vector<std::vector<int>>& radii_square_template);
    static void ScanCube(const int x, const int y, const int z, std::vector<int>& distance_square_values, std::vector<int>& cube);
    static bool Is_Bigger_Square(const float* data, int i, int j, int k, size_t width, size_t height, size_t length, int point_square_index, const int* radii_template);
    static float Look(const float* data, int i, int j, int k, size_t width, size_t height, size_t length);
    static float SetFlag(const float* data, size_t i, size_t j, size_t k, size_t width, size_t height, size_t length);
    static float AverageInteriorNeighbors(const float* data, const float* ori, size_t i, size_t j, size_t k, size_t width, size_t height, size_t length);
};
