#pragma once
/*
This class is for caculate `bone volume(BV)` `total volume(TV) `fraction(BV/TV)`
`bone surface(BS)` `surface density(BS/TV)` `specific surface area(BS/BV)`.
data is original data for better accurance of result.
*/
#include "ruogu_Calculator.h"

#include <string>
#include <vector>
#include <memory>
#include <map>

#include "ruogu_ImageStack.h"

using std::string;
using std::vector;
using std::map;
using std::shared_ptr;

class BasicCalculator :
	public Calculator
{
public:
	BasicCalculator();
	virtual ~BasicCalculator();

public:
	virtual string Calculation(shared_ptr<ImageStack> images, map<string, double>& results);

private:
	void PixelCount(shared_ptr<ImageStack> images, long& total_pixel, long& bone_pixel);

	void TriangleCount(shared_ptr<ImageStack> images, vector<TRIANGLE>& triangles, const double pixel_size, const double pixel_spacing);

	void AreaCalculator(const vector<TRIANGLE>& triangles, vector<NEW_POINT3D>& normal_vector, double& area);

	const NEW_POINT3D CrossProduct(const TRIANGLE& triangle);

	void MultiThreadPixelCount(shared_ptr<ImageStack> images, long& total_pixel, long& bone_pixel, int thread_number);

	void MultlThreadAreaCaculator(const vector<TRIANGLE>& triangles, vector<NEW_POINT3D>& normal_vector, double& area, int thread_number);

	void MultiThreadTriangleCount(shared_ptr<ImageStack> images, vector<TRIANGLE>& triangles, const double pixel_size, const double pixel_spacing, int thread_number);

	void __PixelCount(const short* data, BYTE* binary_data, BYTE* trinary_data, const BASIC_INFO_TWO info, long& total_pixel, long& bone_pixel);

	void __AreaCaculator(const TRIANGLE* triangles, NEW_POINT3D* normal_vector, size_t size, double& area);

	// boost thread function can only take 9 parameters, so union some parameters into struct BASIC_INFO.
	void __TriangleCount(const short* data, const BYTE* binary_data, vector<TRIANGLE>& triangles, const BASIC_INFO info);

	NEW_POINT3D VertexInterapter(const int low_value, const NEW_POINT3D p1, const NEW_POINT3D p2, const double val1, const double val2,
		const double pixel_size, const double pixel_spacing);

	void Polygonise(const GRIDCELL& grid, vector<TRIANGLE>& triangles, const int low_value, const double pixel_size, const double pixel_spacing);

	// for both single thread and multi thread so use point rather than vector.
	inline double GetValueFromPosition(const short* data, const int x, const int y, const int z, const size_t width, const size_t height, const size_t length) {
		if (x < 0 || x >= width || y < 0 || y >= height || z < 0 || z >= length)
			return 0.0f;
		else {
			return static_cast<double>(data[width * height * z + width * y + x]);
		}
	}

	inline BYTE GetValidFromPosition(const BYTE* data, const int x, const int y, const int z, const size_t width, const size_t height, const size_t length) {
		if (x < 0 || x >= width || y < 0 || y >= height || z < 0 || z >= length)
			return 0;
		else {
			return data[width * height * z + width * y + x];
		}
	}
};

