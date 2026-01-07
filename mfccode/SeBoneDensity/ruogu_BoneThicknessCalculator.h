#pragma once
#include "ruogu_Calculator.h"

#include <string>
#include <vector>
#include <memory>
#include <map>

using std::string;
using std::vector;
using std::shared_ptr;
using std::map;


class BoneThicknessCalculator :
	public Calculator
{
public:
	BoneThicknessCalculator();

	~BoneThicknessCalculator();

public:
	virtual string Calculation(shared_ptr<ImageStack> images, map<string, double>& results);

protected:
	void GeometryToDistance(vector<BYTE>& trinary_data, vector<float>& distance_map, const size_t width, const size_t height, const size_t length, bool inverse);

	void DistanceRidge(const vector<float>& distance_map, vector<float>& ridge_map, const size_t width, const size_t height, const size_t length, float& max_radii);

	void LocalThickness(vector<float>& ridge_map, vector<float>& max_radii_map, const size_t width, const size_t height, const size_t length);

	void ThicknessCleaning(vector<float>& radii_map, vector<float>& thickness_map, const size_t width, const size_t height, const size_t length);

	void MultiGeometryToDistance(vector<BYTE>& trinary_data, vector<float>& distance_map, const size_t width, const size_t height, const size_t length, bool inverse, const int thread_number);

	void __X_Distance_map(const BYTE* trinary_data, float* distance_map, const size_t width, const size_t height, const size_t length, const size_t z_start, const size_t z_end, const int check_value);

	void __Y_Distance_map(float* distance_map, const size_t width, const size_t height, const size_t length, const size_t z_start, const size_t z_end, const int no_result, const size_t max_in_whl);

	void __Z_Distance_map(float* distance_map, const size_t width, const size_t height, const size_t length, const size_t y_start, const size_t y_end, const int no_result, const size_t max_in_whl);

	void MultiDistanceRidge(const vector<float>& distance_map, vector<float>& ridge_map, const size_t width, const size_t height, const size_t length, float& max_radii, const int thread_number);

	void __DistanceRidge(const float* distance_map, float* ridge_map, const BASIC_INFO_THREE info, const vector<int>& distance_squared_index, const vector<vector<int>>& bigger_square_radii_template);

	void MultiLocalThickness(vector<float>& ridge_map, vector<float>& max_radii_map, const size_t width, const size_t height, const size_t length, const int thread_number);

	void __LocalThickness(const float* ridge_map, float* max_radii_map, const size_t width, const size_t height, const size_t length, const size_t z_start, const size_t z_end);

	void MultiThicknessClean(vector<float>& radii_map, vector<float>& thickness_map, const size_t width, const size_t height, const size_t length, const int thread_number);

	void __ThicknessCleanSetFlag(const float* radii_map, float* thickness_map, const size_t width, const size_t height, const size_t length, const size_t z_start, const size_t z_end);
	
	void __ThicknessCleanAverageInterNeighbor(const float* radii_map, float* thickness_map, const size_t width, const size_t height, const size_t length, const size_t z_start, const size_t z_end);
	
	void CreateTemplate(vector<int>& distance_square_values, vector<vector<int>>& radii_square_template);

	void ScanCube(const int x, const int y, const int z, vector<int>& distance_square_values, vector<int>& cube);

	float SetFlag(const float* data, const size_t i, const size_t j, const size_t k, const size_t width, const size_t height, const size_t length);

	inline float Look(const float* data, const size_t i, const size_t j, const size_t k, const size_t width, const size_t height, const size_t length) {
		if ((i < 0) || (i >= width) || (j < 0) || (j >= height) || (k < 0) || (k >= length )) return -1;
		return data[k * width * height + j * width + i];
	}

	float AverageInteriorNeighbors(const float* data, const float* ori, const size_t i, const size_t j, const size_t k, const size_t width, const size_t height, const size_t length);

	inline bool Is_Bigger_Square(const float* data, const int x, const  int y, const  int z, const size_t width, const size_t height, const size_t length, const int point_square_index, const int* bigger_square_vector) {
		if ((x < 0) || (x >= width)  || (y < 0) || (y >= height) || (z < 0) || (z >= length)) return false;
		else if(static_cast<int>(data[width * height * z + width * y + x]) > bigger_square_vector[point_square_index]) return true;
		else return false;
	}
};

