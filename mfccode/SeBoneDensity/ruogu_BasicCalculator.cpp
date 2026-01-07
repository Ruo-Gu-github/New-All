#include "stdafx.h"
#include "ruogu_BasicCalculator.h"

#include <assert.h>

using std::pair;

BasicCalculator::BasicCalculator()
{
	
}

BasicCalculator::~BasicCalculator()
{
}

string BasicCalculator::Calculation(shared_ptr<ImageStack> images, map<string, double>& results)
{
	string error = "";

	const double pixel_size = images->pixel_size_;
	const double pixel_spacing = images->pixel_spacing_;
	const int thread_number = images->thread_number_;
	const size_t width = images->width_;
	const size_t height = images->height_;
	const size_t length = images->length_;

	static const double EPSILON = 0.00001;

	if (pixel_size < EPSILON) {
		error = "pixel size should bigger than 0.";
		return error;
	}
	else if (pixel_spacing < EPSILON) {
		error = "pixel spacing should bigger than 0.";
		return error;
	}

	double total_volume = 0.0f;
	double bone_volume = 0.0f;
	double bone_fraction = 0.0f;
	vector<TRIANGLE> triangles;
	vector<NEW_POINT3D> normal_vectors;
	double surface_area = 0.0f;
	double surface_density = 0.0f;
	double specific_surface_area = 0.0f;
	
	// caculate volumn
	long total_pixel = 0;
	long bone_pixel = 0;

#ifdef USE_MULTIPLY_THREAD
	MultiThreadPixelCount(images, total_pixel, bone_pixel, thread_number);
#else
	PixelCount(images, total_pixel, bone_pixel);
#endif
	total_volume = total_pixel * pixel_size * pixel_size * pixel_spacing;
	bone_volume = bone_pixel * pixel_size * pixel_size * pixel_spacing;

	// caculate surface area
	// 1. count triangles
#ifdef USE_MULTIPLY_THREAD
	MultiThreadTriangleCount(images, triangles, pixel_size, pixel_spacing, thread_number);
#else
	TriangleCount(images, triangles, pixel_size, pixel_spacing);
#endif
	
	// 2. caculate area
#ifdef USE_MULTIPLY_THREAD
	MultlThreadAreaCaculator(triangles, normal_vectors, surface_area, thread_number);
#else
	AreaCalculator(triangles, normal_vectors, surface_area);
#endif // USE_MULTIPLY_THREAD
	

	results.insert(pair<string, double>("TV", total_volume));
	results.insert(pair<string, double>("BV", bone_volume));
	results.insert(pair<string, double>("BV/TV", bone_volume / total_volume));
	results.insert(pair<string, double>("BS", surface_area));
	results.insert(pair<string, double>("BS/BV", surface_area / bone_volume));
	results.insert(pair<string, double>("BS/TV", surface_area / total_volume));
	results.insert(pair<string, double>("Center.X", static_cast<double>(width) * pixel_size / 2.0f ));
	results.insert(pair<string, double>("Center.Y", static_cast<double>(height) * pixel_size / 2.0f ));
	results.insert(pair<string, double>("Center.Z", static_cast<double>(length) * pixel_spacing / 2.0f ));
	return error;
}

void BasicCalculator::PixelCount(shared_ptr<ImageStack> images, long & total_pixel, long & bone_pixel)
{
	const vector<short>& data = images->data_;
	vector<BYTE>& binary_data = images->binary_data_;
	vector<BYTE>& trinary_data = images->trinary_data_;

	const int low_value = images->low_value_;
	const int high_value = images->high_value_;
	const int empty_value = images->empty_value_;
	const size_t width_height = images->width_ * images->height_;

	binary_data.empty();
	trinary_data.empty();
	total_pixel = 0;
	bone_pixel = 0;

	binary_data.reserve(data.size());
	trinary_data.reserve(data.size());

	for(size_t i = 0; i < data.size(); i++ ) {
		if (i % width_height == 0)
			theAppIVConfig.m_pILog->ProgressStepIt();
		short pixel = data[i];
		if (pixel == empty_value) {
			binary_data.push_back(0);
			trinary_data.push_back(0);
		}
		else if (pixel >= low_value && pixel <= high_value) {
			binary_data.push_back(1);
			trinary_data.push_back(2);
			total_pixel++;
			bone_pixel++;
		}
		else {
			binary_data.push_back(0);
			trinary_data.push_back(1);
			total_pixel++;
		}
	}

	// don't konw why use iter will be much more slower. it should not be like that. 
// 	// caculate volumn
// 	if (images->bone_pixel_num_ && images->tissue_and_bone_pixel_num_
// 		&& images->binary_data_.size() == data.size() && images->trinary_data_.size() == data.size()) {
// 		total_pixel = images->tissue_and_bone_pixel_num_;
// 		bone_pixel = images->bone_pixel_num_;
// 	}
// 	else {
// 		binary_data.reserve(data.size());
// 		trinary_data.reserve(data.size());
// 		auto iter = data.begin();
// 		while (iter != data.end()) {
// 			short pixel = *iter;
// 			if (pixel == empty_value) {
// 				binary_data.push_back(0);
// 				trinary_data.push_back(0);
// 			}
// 			else if (pixel >= low_value && pixel <= high_value) {
// 				binary_data.push_back(1);
// 				trinary_data.push_back(2);
// 				total_pixel++;
// 				bone_pixel++;
// 			}
// 			else {
// 				binary_data.push_back(0);
// 				trinary_data.push_back(1);
// 				total_pixel++;
// 			}
// 			iter++;
// 		}
// 	}
}

void BasicCalculator::TriangleCount(shared_ptr<ImageStack> images, vector<TRIANGLE>& triangles, const double pixel_size, const double pixel_spacing)
{
	const vector<short>& data = images->data_;
	vector<BYTE>& binary_data = images->binary_data_;

	assert(binary_data.size() == data.size());

	const int width = static_cast<int>(images->width_);
	const int height = static_cast<int>(images->height_);
	const int length = static_cast<int>(images->length_);

	//const int low_value = images->low_value_;
	const int low_value = 1;

	// if (triangles.size() != 0) return;
	triangles.clear();
	for (int k = -1; k < length; k++) {
		theAppIVConfig.m_pILog->ProgressStepIt();
		for (int j = -1; j < height; j++) {
			for (int i = -1; i < width; i++) {
				NEW_POINT3D points[8] = {
					NEW_POINT3D(static_cast<double>(i), static_cast<double>(j), static_cast<double>(k + 1)),
					NEW_POINT3D(static_cast<double>(i + 1), static_cast<double>(j), static_cast<double>(k + 1)),
					NEW_POINT3D(static_cast<double>(i + 1), static_cast<double>(j), static_cast<double>(k)),
					NEW_POINT3D(static_cast<double>(i), static_cast<double>(j), static_cast<double>(k)),
					NEW_POINT3D(static_cast<double>(i), static_cast<double>(j + 1), static_cast<double>(k + 1)),
					NEW_POINT3D(static_cast<double>(i + 1), static_cast<double>(j + 1), static_cast<double>(k + 1)),
					NEW_POINT3D(static_cast<double>(i + 1), static_cast<double>(j + 1), static_cast<double>(k)),
					NEW_POINT3D(static_cast<double>(i), static_cast<double>(j + 1), static_cast<double>(k))
				};
				double values[8] = {
					GetValueFromPosition(data.data(), i, j, k + 1, width, height, length),
					GetValueFromPosition(data.data(), i + 1, j, k + 1, width, height, length),
					GetValueFromPosition(data.data(), i + 1, j, k, width, height, length),
					GetValueFromPosition(data.data(), i, j, k, width, height, length),
					GetValueFromPosition(data.data(), i, j + 1 , k + 1, width, height, length),
					GetValueFromPosition(data.data(), i + 1, j + 1 , k + 1, width, height, length),
					GetValueFromPosition(data.data(), i + 1, j + 1, k, width, height, length),
					GetValueFromPosition(data.data(), i, j + 1, k, width, height, length)
				};

				BYTE bytes[8] = {
					GetValidFromPosition(binary_data.data(), i, j, k + 1, width, height, length),
					GetValidFromPosition(binary_data.data(), i + 1, j, k + 1, width, height, length),
					GetValidFromPosition(binary_data.data(), i + 1, j, k, width, height, length),
					GetValidFromPosition(binary_data.data(), i, j, k, width, height, length),
					GetValidFromPosition(binary_data.data(), i, j + 1 , k + 1, width, height, length),
					GetValidFromPosition(binary_data.data(), i + 1, j + 1 , k + 1, width, height, length),
					GetValidFromPosition(binary_data.data(), i + 1, j + 1, k, width, height, length),
					GetValidFromPosition(binary_data.data(), i, j + 1, k, width, height, length)
				};
				const GRIDCELL grid(points, values, bytes);
				Polygonise(grid, triangles, low_value, pixel_size, pixel_spacing);
			}
		}
	}
}





void BasicCalculator::AreaCalculator(const vector<TRIANGLE>& triangles, vector<NEW_POINT3D>& normal_vector, double & area)
{
	area = 0.0f;

	const size_t size = triangles.size();
	const size_t step_size = size / 10;

	for(size_t i=0; i<size; i++) {
		if (i % step_size == 0) theAppIVConfig.m_pILog->ProgressStepIt();
		NEW_POINT3D cross_vector = CrossProduct(triangles[i]);
		area += sqrt((cross_vector.x * cross_vector.x) + (cross_vector.y * cross_vector.y) + (cross_vector.z * cross_vector.z)) * 0.5f;
		normal_vector.push_back(cross_vector);
	}

// 	auto iter = triangles.begin();
// 	while (iter != triangles.end())	{
// 		NEW_POINT3D cross_vector = CrossProduct(*iter);
// 		area += sqrt((cross_vector.x * cross_vector.x) + (cross_vector.y * cross_vector.y) + (cross_vector.z * cross_vector.z)) * 0.5f;
// 		normal_vector.push_back(cross_vector);
// 
// 		iter++;
// 	}
}

const NEW_POINT3D BasicCalculator::CrossProduct(const TRIANGLE& triangle)
{
	NEW_POINT3D vector1(
		triangle.p[0].x - triangle.p[1].x,
			triangle.p[0].y - triangle.p[1].y,
			triangle.p[0].z - triangle.p[1].z
	);
	NEW_POINT3D vector2(
		triangle.p[2].x - triangle.p[1].x,
			triangle.p[2].y - triangle.p[1].y,
			triangle.p[2].z - triangle.p[1].z
	);

	return NEW_POINT3D(
		vector1.y * vector2.z - vector2.y * vector1.z,
			vector1.z * vector2.x - vector2.z * vector1.x,
			vector1.x * vector2.y - vector2.x * vector1.y
	);

}

void BasicCalculator::MultiThreadPixelCount(shared_ptr<ImageStack> images, long& total_pixel, long& bone_pixel, int thread_number)
{

	boost::thread_group grp;

	const int low_value = images->low_value_;
	const int high_value = images->high_value_;
	const int empty_value = images->empty_value_;
	const size_t width_height = images->width_ * images->height_;

	const vector<short>& data = images->data_;
	vector<BYTE>& binary_data = images->binary_data_;
	vector<BYTE>& trinary_data = images->trinary_data_;
	binary_data.clear();
	trinary_data.clear();

	binary_data.resize(data.size(), 0);
	trinary_data.resize(data.size(), 0);

	size_t size = data.size() / thread_number;
	size_t last_size = size + data.size() % thread_number;

	vector<long> total_pixels;
	vector<long> bone_pixels;
	total_pixels.resize(thread_number, 0);
	bone_pixels.resize(thread_number, 0);

	BASIC_INFO_TWO info(low_value, high_value, empty_value, 0, 0, width_height);

	for (size_t i=0; i<thread_number; i++) {
		info.start = i * size;
		if (i != thread_number - 1)
			info.end = size * (i + 1);
		else
			info.end = data.size();
		
		grp.create_thread(boost::bind(boost::mem_fn(&BasicCalculator::__PixelCount), this, data.data(), binary_data.data(), trinary_data.data(), info, boost::ref(total_pixels[i]),  boost::ref(bone_pixels[i])));
	}

	grp.join_all();
	for(size_t i=0; i<thread_number; i++) {
		total_pixel += total_pixels[i];
		bone_pixel += bone_pixels[i];
	}

}

void BasicCalculator::MultlThreadAreaCaculator(const vector<TRIANGLE>& triangles, vector<NEW_POINT3D>& normal_vector, double& area, int thread_number)
{
	boost::thread_group grp;
	area = 0.0f;
	normal_vector.resize(triangles.size(), NEW_POINT3D(0.0f,0.0f,0.0f));
	size_t size = triangles.size() / thread_number;
	size_t last_size = size + triangles.size() % thread_number;
	vector<double> single_area;
	single_area.resize(thread_number, 0.0f);
	for (size_t i=0; i<thread_number; i++) {
		size_t single_size;
		if (i != thread_number - 1)
			single_size = size;
		else
			single_size = last_size;
		grp.create_thread(boost::bind(boost::mem_fn(&BasicCalculator::__AreaCaculator), this, &triangles[size * i], &normal_vector[size * i], single_size, boost::ref(single_area[i])));
	}

	grp.join_all();

	for (size_t i=0; i<thread_number; i++) area += single_area[i];
// 	auto iter = single_area.begin();
// 	while (iter != single_area.end()) {
// 		area += *iter;
// 		iter++;
// 	}
}


void BasicCalculator::MultiThreadTriangleCount(shared_ptr<ImageStack> images, vector<TRIANGLE>& triangles, const double pixel_size, const double pixel_spacing, int thread_number)
{
	boost::thread_group grp;

	const vector<short>& data = images->data_;
	const vector<BYTE>& binary_data = images->binary_data_;

	assert(binary_data.size() == data.size());

	const size_t width = images->width_;
	const size_t height = images->height_;
	const size_t length = images->length_;

	const int low_value = images->low_value_;

	triangles.clear();
	
	vector<vector<TRIANGLE>> triangles_for_threads;
	triangles_for_threads.resize(thread_number, vector<TRIANGLE>());

	const size_t slice = length + 1;

	const size_t size = slice / thread_number;
	const size_t last_size = size + slice % thread_number;

	
	BASIC_INFO info(width, height, length, 0, 0, pixel_size, pixel_spacing, low_value);


	for (size_t i=0; i<thread_number; i++) {
		info.z_start = static_cast<int>(size * i) - 1;
		if (i != thread_number - 1) {
			info.z_end = static_cast<int>(size * i + size) - 1;
		}
		else {
			info.z_end = static_cast<int>(length);
		}
		grp.create_thread(boost::bind(boost::mem_fn(&BasicCalculator::__TriangleCount), this, data.data(), binary_data.data(), boost::ref(triangles_for_threads[i]), info));
	}

	grp.join_all();

	for (int i=0; i<thread_number; i++) {
		triangles.insert(triangles.end(), triangles_for_threads[i].begin(), triangles_for_threads[i].end());
	}

	// 	auto iter = triangles_for_threads.begin();
	// 	while (iter != triangles_for_threads.end()) {
	// 		triangles.insert(triangles.end(), iter->begin(), iter->end());
	// 		iter++;
	// 	}
}


void BasicCalculator::__PixelCount(const short* data, BYTE* binary_data, BYTE* trinary_data, const BASIC_INFO_TWO info, long& total_pixel, long& bone_pixel)
{
	const int low_value = info.low_value;
	const int high_value = info.high_value;
	const int empty_value = info.empty_value;
	const size_t start = info.start;
	const size_t end = info.end;
	const size_t width_height = info.width_height;

	total_pixel = 0;
	bone_pixel = 0;

	for(size_t i=start; i<end; i++) {
		if (i % width_height == 0)
			theAppIVConfig.m_pILog->ProgressStepIt();
		short pixel = data[i];
		if (pixel == empty_value) continue;
		else if (pixel >= low_value && pixel <= high_value) {
			binary_data[i] = 1;
			trinary_data[i] = 2;
			total_pixel++;
			bone_pixel++;
		}
		else {
			trinary_data[i] = 1;
			total_pixel++;
		}
	}
}

void BasicCalculator::__AreaCaculator(const TRIANGLE* triangles, NEW_POINT3D* normal_vector, size_t size, double& area)
{
	const size_t step_size = size / 10;
	for(size_t i=0; i<size; i++) {
		if (i % step_size == 0) theAppIVConfig.m_pILog->ProgressStepIt();
		const TRIANGLE triangle = triangles[i];
		const NEW_POINT3D vector1(
			triangle.p[0].x - triangle.p[1].x,
			triangle.p[0].y - triangle.p[1].y,
			triangle.p[0].z - triangle.p[1].z
			);
		const NEW_POINT3D vector2(
			triangle.p[2].x - triangle.p[1].x,
			triangle.p[2].y - triangle.p[1].y,
			triangle.p[2].z - triangle.p[1].z
			);

		const NEW_POINT3D cross_vector(
			vector1.y * vector2.z - vector2.y * vector1.z,
			vector1.z * vector2.x - vector2.z * vector1.x,
			vector1.x * vector2.y - vector2.x * vector1.y
			);
		area += sqrt((cross_vector.x * cross_vector.x) + (cross_vector.y * cross_vector.y) + (cross_vector.z * cross_vector.z)) * 0.5f;
		normal_vector[i] = cross_vector;
	}
}

void BasicCalculator::__TriangleCount(const short* data, const BYTE* binary_data, vector<TRIANGLE>& triangles, const BASIC_INFO info)
{
	triangles.clear();

	const int width = static_cast<int>(info.width);
	const int height = static_cast<int>(info.height);
	const int length = static_cast<int>(info.length);
	const int z_start = info.z_start;
	const int z_end = info.z_end;
	const double pixel_size = info.pixel_size;
	const double pixel_spacing = info.pixel_spacing;
	// const int low_value = info.low_value;
	const int low_value = 1;

	for (int k = z_start; k < z_end; k++) {
		theAppIVConfig.m_pILog->ProgressStepIt();
		for (int j = -1; j < height; j++) {
			for (int i = -1; i < width; i++) {
				NEW_POINT3D points[8] = {
					NEW_POINT3D(static_cast<double>(i), static_cast<double>(j), static_cast<double>(k + 1)),
					NEW_POINT3D(static_cast<double>(i + 1), static_cast<double>(j), static_cast<double>(k + 1)),
					NEW_POINT3D(static_cast<double>(i + 1), static_cast<double>(j), static_cast<double>(k)),
					NEW_POINT3D(static_cast<double>(i), static_cast<double>(j), static_cast<double>(k)),
					NEW_POINT3D(static_cast<double>(i), static_cast<double>(j + 1), static_cast<double>(k + 1)),
					NEW_POINT3D(static_cast<double>(i + 1), static_cast<double>(j + 1), static_cast<double>(k + 1)),
					NEW_POINT3D(static_cast<double>(i + 1), static_cast<double>(j + 1), static_cast<double>(k)),
					NEW_POINT3D(static_cast<double>(i), static_cast<double>(j + 1), static_cast<double>(k))
				};
				double values[8] = {
					GetValueFromPosition(data, i, j, k + 1, width, height, length),
					GetValueFromPosition(data, i + 1, j, k + 1, width, height, length),
					GetValueFromPosition(data, i + 1, j, k, width, height, length),
					GetValueFromPosition(data, i, j, k, width, height, length),
					GetValueFromPosition(data, i, j + 1 , k + 1, width, height, length),
					GetValueFromPosition(data, i + 1, j + 1 , k + 1, width, height, length),
					GetValueFromPosition(data, i + 1, j + 1, k, width, height, length),
					GetValueFromPosition(data, i, j + 1, k, width, height, length)
				};

				BYTE bytes[8] = {
					GetValidFromPosition(binary_data, i, j, k + 1, width, height, length),
					GetValidFromPosition(binary_data, i + 1, j, k + 1, width, height, length),
					GetValidFromPosition(binary_data, i + 1, j, k, width, height, length),
					GetValidFromPosition(binary_data, i, j, k, width, height, length),
					GetValidFromPosition(binary_data, i, j + 1 , k + 1, width, height, length),
					GetValidFromPosition(binary_data, i + 1, j + 1 , k + 1, width, height, length),
					GetValidFromPosition(binary_data, i + 1, j + 1, k, width, height, length),
					GetValidFromPosition(binary_data, i, j + 1, k, width, height, length)
				};
				const GRIDCELL grid(points, values, bytes);
				Polygonise(grid, triangles, low_value, pixel_size, pixel_spacing);
			}
		}
	}
}

void BasicCalculator::Polygonise(const GRIDCELL& grid, vector<TRIANGLE>& triangles, const int low_value, const double pixel_size, const double pixel_spacing)
{
	static const  int EDGE_TABLE[256] = 
	{
		0x0  , 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
		0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
		0x190, 0x99 , 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
		0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
		0x230, 0x339, 0x33 , 0x13a, 0x636, 0x73f, 0x435, 0x53c,
		0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
		0x3a0, 0x2a9, 0x1a3, 0xaa , 0x7a6, 0x6af, 0x5a5, 0x4ac,
		0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
		0x460, 0x569, 0x663, 0x76a, 0x66 , 0x16f, 0x265, 0x36c,
		0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
		0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff , 0x3f5, 0x2fc,
		0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
		0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55 , 0x15c,
		0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
		0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc ,
		0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
		0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
		0xcc , 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
		0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
		0x15c, 0x55 , 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
		0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
		0x2fc, 0x3f5, 0xff , 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
		0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
		0x36c, 0x265, 0x16f, 0x66 , 0x76a, 0x663, 0x569, 0x460,
		0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
		0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa , 0x1a3, 0x2a9, 0x3a0,
		0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
		0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33 , 0x339, 0x230,
		0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
		0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99 , 0x190,
		0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
		0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0 
	};
	static const int TRIANGLE_TABLE[256][16] =
	{
		{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
		{3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
		{3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
		{3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
		{9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1},
		{9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
		{2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
		{8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
		{9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
		{4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
		{3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1},
		{1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
		{4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1},
		{4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
		{9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
		{5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1},
		{2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1},
		{9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
		{0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
		{2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1},
		{10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1},
		{4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1},
		{5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
		{5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1},
		{9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1},
		{0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1},
		{1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1},
		{10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1},
		{8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1},
		{2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1},
		{7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1},
		{9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1},
		{2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1},
		{11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1},
		{9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1},
		{5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1},
		{11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1},
		{11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
		{1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1},
		{9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1},
		{5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1},
		{2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
		{0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
		{5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1},
		{6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1},
		{3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1},
		{6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1},
		{5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1},
		{1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
		{10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1},
		{6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1},
		{8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1},
		{7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1},
		{3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
		{5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1},
		{0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1},
		{9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1},
		{8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1},
		{5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1},
		{0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1},
		{6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1},
		{10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1},
		{10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1},
		{8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1},
		{1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1},
		{3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1},
		{0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1},
		{10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1},
		{3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1},
		{6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1},
		{9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1},
		{8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1},
		{3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1},
		{6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1},
		{0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1},
		{10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1},
		{10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1},
		{2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1},
		{7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1},
		{7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1},
		{2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1},
		{1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1},
		{11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1},
		{8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1},
		{0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1},
		{7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
		{10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
		{2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
		{6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1},
		{7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1},
		{2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1},
		{1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1},
		{10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1},
		{10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1},
		{0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1},
		{7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1},
		{6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1},
		{8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1},
		{9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1},
		{6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1},
		{1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1},
		{4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1},
		{10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1},
		{8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1},
		{0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1},
		{1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1},
		{8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1},
		{10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1},
		{4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1},
		{10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
		{0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
		{5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
		{11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1},
		{9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
		{6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1},
		{7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1},
		{3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1},
		{7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1},
		{9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1},
		{3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1},
		{ 6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1 },
		{ 9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1 },
		{ 1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1 },
		{ 4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1 },
		{ 7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1 },
		{ 6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1 },
		{ 0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1 },
		{ 6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1 },
		{ 0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1 },
		{ 11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1 },
		{ 6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1 },
		{ 5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1 },
		{ 9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1 },
		{ 1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1 },
		{ 10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1 },
		{ 0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1 },
		{ 5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1 },
		{ 10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1 },
		{ 11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1 },
		{ 9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1 },
		{ 7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1 },
		{ 2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1 },
		{ 8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1 },
		{ 9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1 },
		{ 9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1 },
		{ 1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1 },
		{ 5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1 },
		{ 0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1 },
		{ 10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1 },
		{ 2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1 },
		{ 0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1 },
		{ 0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1 },
		{ 9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1 },
		{ 5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1 },
		{ 5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1 },
		{ 8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1 },
		{ 9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1 },
		{ 1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1 },
		{ 3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1 },
		{ 4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1 },
		{ 9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1 },
		{ 11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1 },
		{ 11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1 },
		{ 2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1 },
		{ 9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1 },
		{ 3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1 },
		{ 1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1 },
		{ 4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1 },
		{ 4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1 },
		{ 0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1 },
		{ 9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1 },
		{ 1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ 0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }
	};

	NEW_POINT3D vertex_list[12];
	int cube_index = 0;

	if (!grid.valid[0]) cube_index |= 1;
	if (!grid.valid[1]) cube_index |= 2;
	if (!grid.valid[2]) cube_index |= 4;
	if (!grid.valid[3]) cube_index |= 8;
	if (!grid.valid[4]) cube_index |= 16;
	if (!grid.valid[5]) cube_index |= 32;
	if (!grid.valid[6]) cube_index |= 64;
	if (!grid.valid[7]) cube_index |= 128;

	if (EDGE_TABLE[cube_index] & 1) {
		vertex_list[0] = VertexInterapter(low_value, grid.p[0], grid.p[1], grid.valid[0], grid.valid[1], pixel_size, pixel_spacing);
	}
	if (EDGE_TABLE[cube_index] & 2) {
		vertex_list[1] = VertexInterapter(low_value, grid.p[1], grid.p[2], grid.valid[1], grid.valid[2], pixel_size, pixel_spacing);
	}
	if (EDGE_TABLE[cube_index] & 4) {
		vertex_list[2] = VertexInterapter(low_value, grid.p[2], grid.p[3], grid.valid[2], grid.valid[3], pixel_size, pixel_spacing);
	}
	if (EDGE_TABLE[cube_index] & 8) {
		vertex_list[3] = VertexInterapter(low_value, grid.p[3], grid.p[0], grid.valid[3], grid.valid[0], pixel_size, pixel_spacing);
	}
	if (EDGE_TABLE[cube_index] & 16) {
		vertex_list[4] = VertexInterapter(low_value, grid.p[4], grid.p[5], grid.valid[4], grid.valid[5], pixel_size, pixel_spacing);
	}
	if (EDGE_TABLE[cube_index] & 32) {
		vertex_list[5] = VertexInterapter(low_value, grid.p[5], grid.p[6], grid.valid[5], grid.valid[6], pixel_size, pixel_spacing);
	}
	if (EDGE_TABLE[cube_index] & 64) {
		vertex_list[6] = VertexInterapter(low_value, grid.p[6], grid.p[7], grid.valid[6], grid.valid[7], pixel_size, pixel_spacing);
	}
	if (EDGE_TABLE[cube_index] & 128) {
		vertex_list[7] = VertexInterapter(low_value, grid.p[7], grid.p[4], grid.valid[7], grid.valid[4], pixel_size, pixel_spacing);
	}
	if (EDGE_TABLE[cube_index] & 256) {
		vertex_list[8] = VertexInterapter(low_value, grid.p[0], grid.p[4], grid.valid[0], grid.valid[4], pixel_size, pixel_spacing);
	}
	if (EDGE_TABLE[cube_index] & 512) {
		vertex_list[9] = VertexInterapter(low_value, grid.p[1], grid.p[5], grid.valid[1], grid.valid[5], pixel_size, pixel_spacing);
	}
	if (EDGE_TABLE[cube_index] & 1024) {
		vertex_list[10] = VertexInterapter(low_value, grid.p[2], grid.p[6], grid.valid[2], grid.valid[6], pixel_size, pixel_spacing);
	}
	if (EDGE_TABLE[cube_index] & 2048) {
		vertex_list[11] = VertexInterapter(low_value, grid.p[3], grid.p[7], grid.valid[3], grid.valid[7], pixel_size, pixel_spacing);
	}

	for (int i = 0; TRIANGLE_TABLE[cube_index][i] != -1; i += 3) {
		NEW_POINT3D points[3] = {
			vertex_list[TRIANGLE_TABLE[cube_index][i]],
			vertex_list[TRIANGLE_TABLE[cube_index][i + 1]],
			vertex_list[TRIANGLE_TABLE[cube_index][i + 2]]
		};
		NEW_POINT3D normal_vector = CrossProduct(TRIANGLE(points));
		if(normal_vector.x != 0.0f || normal_vector.y != 0.0f || normal_vector.z != 0.0f)
			triangles.push_back(TRIANGLE(points));
	}
}

NEW_POINT3D BasicCalculator::VertexInterapter(const int low_value, const NEW_POINT3D p1, const NEW_POINT3D p2, const double val1, const double val2,
	const double pixel_size, const double pixel_spacing)
{
	static const double EPSILON = 0.001;
	double scale;

	if (abs(val1 - low_value) < EPSILON) {
		return NEW_POINT3D(
			p1.x * pixel_size,
			p1.y * pixel_size,
			p1.z * pixel_spacing
			);
	}
	else if (abs(val1 - val2) < EPSILON) {
		return NEW_POINT3D(
			p1.x * pixel_size,
			p1.y * pixel_size,
			p1.z * pixel_spacing
			);
	}
	else if (abs(val2 - low_value) < EPSILON) {
		return NEW_POINT3D(
			p2.x * pixel_size,
			p2.y * pixel_size,
			p2.z * pixel_spacing
			);
	}
	else {
		scale = (low_value - val1) / (val2 - val1);
		return NEW_POINT3D(
			(p1.x + scale * (p2.x - p1.x)) * pixel_size,
			(p1.y + scale * (p2.y - p1.y)) * pixel_size,
			(p1.z + scale * (p2.z - p1.z)) * pixel_spacing
			);
	}
}