#include "stdafx.h"
#include "ruogu_BoneThicknessSpacingCalculator.h"

BoneThicknessSpacingCalculator::BoneThicknessSpacingCalculator() {

}

BoneThicknessSpacingCalculator::~BoneThicknessSpacingCalculator() {

}

string BoneThicknessSpacingCalculator::Calculation(shared_ptr<ImageStack> images, map<string, double>& results)
{
	string error = "";

	clock_t start, end;
	start = clock();

	const size_t width = images->width_;
	const size_t height = images->height_;
	const size_t length = images->length_;
	const double pixel_size = images->pixel_size_;
	const double pixel_spacing = images->pixel_spacing_;
	const int thread_number = images->thread_number_;

	const bool inverse = true;

	vector<BYTE> trinary_data = images->trinary_data_;
	vector<float> distance_map;
	vector<float> ridge_map;
	vector<float> max_radii_map;
	vector<float> thickness_map;

	float max_radii = 0.0f;

	// resease vector for save memory.
	distance_map.resize(trinary_data.size(), 0.0f);
#ifdef USE_MULTIPLY_THREAD
	MultiGeometryToDistance(trinary_data, distance_map, width, height, length, inverse, thread_number);
#else
	GeometryToDistance(trinary_data, distance_map, width, height, length, inverse);
#endif

	ridge_map.resize(trinary_data.size(), 0.0f);
#ifdef USE_MULTIPLY_THREAD
	MultiDistanceRidge(distance_map, ridge_map, width, height, length, max_radii, thread_number);
#else
	DistanceRidge(distance_map, ridge_map, width, height, length, max_radii);
#endif
	distance_map.clear();

	max_radii_map.resize(trinary_data.size(), 0.0f);
#ifdef USE_MULTIPLY_THREAD
	MultiLocalThickness(ridge_map, max_radii_map, width, height, length, thread_number);
#else
	LocalThickness(ridge_map, max_radii_map, width, height, length);
#endif
	ridge_map.clear();

	thickness_map.resize(trinary_data.size(), 0.0f);
#ifdef USE_MULTIPLY_THREAD
	MultiThicknessClean(max_radii_map, thickness_map, width, height, length, thread_number);
#else
	ThicknessCleaning(max_radii_map, thickness_map, width, height, length);
#endif
	max_radii_map.clear();


	size_t total_count = 0;
	double total_radii = 0.0f;
	vector<size_t> histogram;
	histogram.resize(static_cast<int>(max_radii * max_radii * 4 + 0.5f) + 1, 0);
	for (size_t i=0; i<thickness_map.size(); i++) {
		histogram[(static_cast<int>(thickness_map[i] * thickness_map[i] + 0.5f))]++;
		if (thickness_map[i] > 0.0f) {
			total_count++;
			total_radii += thickness_map[i];
		}
	}

	float mean_diameter = total_radii / static_cast<double>(total_count);
	float max_diameter = max_radii * 2.0f * pixel_size;
	float std_diameter = 0.0f;
	for (size_t i=1; i<histogram.size(); i++) {
		if (histogram[i] != 0) {
			std_diameter += (sqrt(static_cast<float>(i)) - mean_diameter) * (sqrt(static_cast<float>(i)) - mean_diameter) * histogram[i];
		}
	}
	mean_diameter = mean_diameter * pixel_size;
	std_diameter = sqrt(std_diameter / static_cast<double>(total_count)) * pixel_size;

	thickness_map.clear();

	end = clock();
	clock_t res = end - start;

	results.insert(make_pair<string, double>("Tb.sp mean", mean_diameter));
	results.insert(make_pair<string, double>("Tb.sp max", max_diameter));
	results.insert(make_pair<string, double>("Tb.sp std", std_diameter));
	return error;
}

