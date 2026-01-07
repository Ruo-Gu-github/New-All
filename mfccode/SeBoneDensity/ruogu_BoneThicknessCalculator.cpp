#include "stdafx.h"
#include "ruogu_BoneThicknessCalculator.h"

BoneThicknessCalculator::BoneThicknessCalculator()
{
}

BoneThicknessCalculator::~BoneThicknessCalculator()
{
}

string BoneThicknessCalculator::Calculation(shared_ptr<ImageStack> images, map<string, double>& results)
{
	string error = "";

	const size_t width = images->width_;
	const size_t height = images->height_;
	const size_t length = images->length_;
	const double pixel_size = images->pixel_size_;
	const double pixel_spacing = images->pixel_spacing_;
	const int thread_number = images->thread_number_;

	const bool inverse = false;

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
 
	results.insert(make_pair<string, double>("Tb.th mean", mean_diameter));
	results.insert(make_pair<string, double>("Tb.th max", max_diameter));
	results.insert(make_pair<string, double>("Tb.th std", std_diameter));
	return error;
}

void BoneThicknessCalculator::GeometryToDistance(vector<BYTE>& trinary_data, vector<float>& distance_map, const size_t width, const size_t height, const size_t length, bool inverse)
{
	size_t max_in_whl = width;
	max_in_whl = max_in_whl > height ? max_in_whl : height;
	max_in_whl = max_in_whl > length ? max_in_whl : length;

	const int no_result = static_cast<int>(3 * (max_in_whl + 1) * (max_in_whl + 1));

	int test, min;
	size_t line_position;

	// data not binary but three different type. 
	// 0 is empty, 1 is roi without bone, 2 is bone.
	int check_value;
	if (inverse) check_value = 1;
	else check_value = 2;

	clock_t astart, end1, end2, end3;
	astart = clock();

	// find the min of distance square in same line
	// example
	// line : 0000000*********@***00000 get 3^2 = 9T
	// use no_result when all *
	// do it for all three dim dimensions.
	for (size_t k = 0; k < length; k++) {
		theAppIVConfig.m_pILog->ProgressStepIt();
		for (size_t j = 0; j < height; j++) {
			for (size_t i = 0; i < width; i++) {
				min = no_result;
				line_position = j * width + k * width * height;
				for (int x = static_cast<int>(i); x < width; x++) {
					if (trinary_data[line_position + x] != check_value) {
						test = static_cast<int>(i - x);
						test *= test;
						min = test;
						break;
					}
				}
				for (int x = static_cast<int>(i) - 1; x >= 0; x--) {
					if (trinary_data[line_position + x] != check_value) {
						test = static_cast<int>(i - x);
						test *= test;
						min = test < min ? test : min;
						break;
					}
				}
				distance_map[line_position + i] = static_cast<float>(min);
			}
		}
	}

	end1 = clock();

	bool nonempty;

// use raw point instead of vector, because vector is slightly slower than raw point and it will loop a lot of times.
// 	vector<int> one_column;
 	vector<int> one_column_processed;
// 	one_column.reverse(max_in_whl);
 	one_column_processed.resize(max_in_whl, 0);

	int* one_column_ptr = new int[max_in_whl];
	memset(one_column_ptr, 0, sizeof(int) * max_in_whl);

 	int delta;
	for (size_t k = 0; k < length; k++) {
		theAppIVConfig.m_pILog->ProgressStepIt();
		for (size_t i = 0; i < width; i++) {
			nonempty = false;			
			// one_column.clear();
			for (size_t j = 0; j < height; j++) {
				//one_column.push_back(static_cast<int>(distance_map[i + j * width + k * width * height]));
				one_column_ptr[j] = static_cast<int>(distance_map[i + j * width + k * width * height]);
				if (distance_map[i + j * width + k * width * height] > 0) nonempty = true;
			}
			if (nonempty) {
				for (size_t j = 0; j < height; j++) {
					min = no_result;
					delta = static_cast<int>(j);
 					for (size_t y = 0; y < height; y++) {

						test = static_cast<int>(one_column_ptr[y] + delta * delta--);
 						min = test < min ? test : min;
 					}
					one_column_processed[j] = min;
				}
				for (size_t j = 0; j < height; j++) {
					distance_map[i + j * width + k * width * height] = static_cast<float>(one_column_processed[j]);
				}
			}
		}
	}

	delete [] one_column_ptr;

	end2 = clock();

	// vector<int> one_pole;
	vector<int> one_pole_processed;
	// one_pole.reserve(max_in_whl);
	one_pole_processed.resize(max_in_whl, 0);

	int* one_pole_ptr = new int[max_in_whl];
	memset(one_pole_ptr, 0, sizeof(int) * max_in_whl);

	for (size_t j = 0; j < height; j++) {
		theAppIVConfig.m_pILog->ProgressStepIt();
		for (size_t i = 0; i < width; i++) {
			nonempty = false;
			// one_pole.clear();
			for (size_t k = 0; k < length; k++) {
				// one_pole.push_back(static_cast<int>(distance_map[i + j * width + k * width * height]));
				one_pole_ptr[k] = static_cast<int>(distance_map[i + j * width + k * width * height]);
				if (distance_map[i + j * width + k * width * height] > 0) nonempty = true;
			}
			if (nonempty) {
				for (size_t k = 0; k < length; k++) {
					min = no_result;
					delta = static_cast<int>(k);
					for (size_t z = 0; z < length; z++) {
						test = static_cast<int>(one_pole_ptr[z] + delta * delta--);
						min = test < min ? test : min;
					}
					one_pole_processed[k] = min;
				}
				for (size_t k = 0; k < length; k++) {
					distance_map[i + j * width + k * width * height] = static_cast<float>(one_pole_processed[k]);
				}
			}
		}
	}

	delete [] one_pole_ptr;

	end3 = clock();

	clock_t res1 = end1 - astart;
	clock_t res2 = end2 - end1;
	clock_t res3 = end3 - end2;
}

void BoneThicknessCalculator::MultiGeometryToDistance(vector<BYTE>& trinary_data, vector<float>& distance_map, const size_t width, const size_t height, const size_t length, bool inverse, const int thread_number)
{
	clock_t astart, end1, end2, end3;
	astart = clock();
	size_t max_in_whl = width;
	max_in_whl = max_in_whl > height ? max_in_whl : height;
	max_in_whl = max_in_whl > length ? max_in_whl : length;

	const int no_result = static_cast<int>(3 * (max_in_whl + 1) * (max_in_whl + 1));

	// data not binary but three different type. 
	// 0 is empty, 1 is roi without bone, 2 is bone.
	int check_value;
	if (inverse) check_value = 1;
	else check_value = 2;

	boost::thread_group grp_1;
	const size_t size = length / thread_number;

	for (size_t i=0; i<thread_number; i++) {
		const size_t start = i * size;
		const size_t end = i  == (thread_number - 1) ? length : (i + 1) * size;
		grp_1.create_thread(boost::bind(boost::mem_fn(&BoneThicknessCalculator::__X_Distance_map), this, trinary_data.data(), distance_map.data(), width, height, length, start, end, check_value));
	}
	grp_1.join_all();
	end1 = clock();

	boost::thread_group grp_2;

	for (size_t i=0; i<thread_number; i++) {
		const size_t start = i * size;
		const size_t end = i  == (thread_number - 1) ? length : (i + 1) * size;
		grp_2.create_thread(boost::bind(boost::mem_fn(&BoneThicknessCalculator::__Y_Distance_map), this, distance_map.data(), width, height, length, start, end, no_result, max_in_whl));
	}
	grp_2.join_all();
	end2 = clock();

	boost::thread_group grp_3;
	const size_t new_size = height / thread_number;

	for (size_t i=0; i<thread_number; i++) {
		const size_t start = i * new_size;
		const size_t end = i  == (thread_number - 1) ? height : (i + 1) * new_size;
		grp_3.create_thread(boost::bind(boost::mem_fn(&BoneThicknessCalculator::__Z_Distance_map), this, distance_map.data(), width, height, length, start, end, no_result, max_in_whl));
	}
	grp_3.join_all();
	end3 = clock();

	clock_t res1 = end1 - astart;
	clock_t res2 = end2 - end1;
	clock_t res3 = end3 - end2;
}


void BoneThicknessCalculator::__X_Distance_map(const BYTE* trinary_data, float* distance_map, const size_t width, const size_t height, const size_t length, const size_t z_start, const size_t z_end, const int check_value)
{
	// can't have more than 8 parameters for thread function, so calculate one in thread.
	size_t max_in_whl = width;
	max_in_whl = max_in_whl > height ? max_in_whl : height;
	max_in_whl = max_in_whl > length ? max_in_whl : length;

	const int no_result = static_cast<int>(3 * (max_in_whl + 1) * (max_in_whl + 1));

	int test, min;
	size_t line_position; 
	for (size_t k = z_start; k < z_end; k++) {
		theAppIVConfig.m_pILog->ProgressStepIt();
		for (size_t j = 0; j < height; j++) {
			for (size_t i = 0; i < width; i++) {
				min = no_result;
				line_position = j * width + k * width * height;
				for (int x = static_cast<int>(i); x < width; x++) {
					if (trinary_data[line_position + x] != check_value) {
						test = static_cast<int>(i - x);
						test *= test;
						min = test;
						break;
					}
				}
				for (int x = static_cast<int>(i) - 1; x >= 0; x--) {
					if (trinary_data[line_position + x] != check_value) {
						test = static_cast<int>(i - x);
						test *= test;
						min = test < min ? test : min;
						break;
					}
				}
				distance_map[line_position + i] = static_cast<float>(min);
			}
		}
	}
}

void BoneThicknessCalculator::__Y_Distance_map(float* distance_map, const size_t width, const size_t height, const size_t length, const size_t z_start, const size_t z_end, const int no_result, const size_t max_in_whl)
{
	bool nonempty;
 	int test, min, delta;

	vector<int> one_column_processed;
	one_column_processed.resize(max_in_whl, 0);

	int* one_column_ptr = new int[max_in_whl];
	memset(one_column_ptr, 0, sizeof(int) * max_in_whl);

	for (size_t k = z_start; k < z_end; k++) {
		theAppIVConfig.m_pILog->ProgressStepIt();
		for (size_t i = 0; i < width; i++) {
			nonempty = false;			
			for (size_t j = 0; j < height; j++) {
				one_column_ptr[j] = static_cast<int>(distance_map[i + j * width + k * width * height]);
				if (distance_map[i + j * width + k * width * height] > 0) nonempty = true;
			}
			if (nonempty) {
				for (size_t j = 0; j < height; j++) {
					min = no_result;
					delta = static_cast<int>(j);
					for (size_t y = 0; y < height; y++) {
						test = static_cast<int>(one_column_ptr[y] + delta * delta--);
						min = test < min ? test : min;
					}
					one_column_processed[j] = min;
				}
				for (size_t j = 0; j < height; j++) {
					distance_map[i + j * width + k * width * height] = static_cast<float>(one_column_processed[j]);
				}
			}
		}
	}
	delete [] one_column_ptr;
}

void BoneThicknessCalculator::__Z_Distance_map(float* distance_map, const size_t width, const size_t height, const size_t length, const size_t y_start, const size_t y_end, const int no_result, const size_t max_in_whl)
{
	bool nonempty;
	int test, min, delta;

	vector<int> one_pole_processed;
	one_pole_processed.resize(max_in_whl, 0);

	int* one_pole_ptr = new int[max_in_whl];
	memset(one_pole_ptr, 0, sizeof(int) * max_in_whl);

	for (size_t j = y_start; j < y_end; j++) {
		theAppIVConfig.m_pILog->ProgressStepIt();
		for (size_t i = 0; i < width; i++) {
			nonempty = false;
			for (size_t k = 0; k < length; k++) {
				one_pole_ptr[k] = static_cast<int>(distance_map[i + j * width + k * width * height]);
				if (distance_map[i + j * width + k * width * height] > 0) nonempty = true;
			}
			if (nonempty) {
				for (size_t k = 0; k < length; k++) {
					min = no_result;
					delta = static_cast<int>(k);
					for (size_t z = 0; z < length; z++) {
						test = static_cast<int>(one_pole_ptr[z] + delta * delta--);
						min = test < min ? test : min;
					}
					one_pole_processed[k] = min;
				}
				for (size_t k = 0; k < length; k++) {
					distance_map[i + j * width + k * width * height] = static_cast<float>(one_pole_processed[k]);
				}
			}
		}
	}
	delete [] one_pole_ptr;
}

void BoneThicknessCalculator::DistanceRidge(const vector<float>& distance_map, vector<float>& ridge_map, const size_t width, const size_t height, const size_t length, float& max_radii)
{
	float max_distance = 0.0f;
	for (size_t i=0; i<distance_map.size(); i++) {
		if (max_distance < distance_map[i]) max_distance = distance_map[i];
	}
	
	max_distance = sqrt(max_distance);
	max_radii = max_distance;

	assert(distance_map.size() == width * height * length);

	const int distance_squared = static_cast<int>(max_distance * max_distance + 0.5f) + 1;

	vector<int> occurs;
	occurs.resize(distance_squared, 0);

	for (size_t i=0; i<distance_map.size(); i++) {
		occurs[static_cast<int>(distance_map[i])] = 1;
	}

	int radii_number = 0;
	for (int i=0; i<occurs.size(); i++) {
		if (occurs[i] == 1) radii_number++;
	}
	
	vector<int> distance_squared_index;
	distance_squared_index.resize(occurs.size(), 0);
	vector<int> distance_squared_values;
	distance_squared_values.resize(radii_number, 0);

	int index = 0;

	for (int i = 0; i < occurs.size(); i++) {
		if (occurs[i] == 1) {
			distance_squared_index[i] = index;
			distance_squared_values[index++] = i;
		}
	}

	// 有一个圆，半径为 r， 求 r' 使得当圆的原点移动到（0，0，1）， （0，1，1）， （1，1，1） 时， r' 形成的新圆能够包含 r 形成的圆所. 
	vector <vector<int>> bigger_square_radii_template;
	CreateTemplate(distance_squared_values, bigger_square_radii_template);

	int point_square_index, point_square;
	bool not_ridge_point = false;

	for (size_t z = 0; z < length; z++) {
		theAppIVConfig.m_pILog->ProgressStepIt();
		for (size_t y = 0; y < height; y++) {
			for (size_t x = 0; x < width; x++) {
				size_t index = z * width * height + y * width + x;
 				if (distance_map[index] > 0) {
					not_ridge_point = false;
					point_square = static_cast<int>(distance_map[index]);
					point_square_index = distance_squared_index[point_square];
					int i = static_cast<int>(x);
					int j = static_cast<int>(y);
					int k = static_cast<int>(z);

					if(Is_Bigger_Square(distance_map.data(), i - 1, j, k, width, height, length, point_square_index, bigger_square_radii_template[0].data())) continue;
					if(Is_Bigger_Square(distance_map.data(), i, j - 1, k, width, height, length, point_square_index, bigger_square_radii_template[0].data())) continue;
					if(Is_Bigger_Square(distance_map.data(), i, j, k - 1, width, height, length, point_square_index, bigger_square_radii_template[0].data())) continue;
					if(Is_Bigger_Square(distance_map.data(), i + 1, j, k, width, height, length, point_square_index, bigger_square_radii_template[0].data())) continue;
					if(Is_Bigger_Square(distance_map.data(), i, j + 1, k, width, height, length, point_square_index, bigger_square_radii_template[0].data())) continue;
					if(Is_Bigger_Square(distance_map.data(), i, j, k + 1, width, height, length, point_square_index, bigger_square_radii_template[0].data())) continue;
					if(Is_Bigger_Square(distance_map.data(), i - 1, j - 1, k, width, height, length, point_square_index, bigger_square_radii_template[1].data())) continue;
					if(Is_Bigger_Square(distance_map.data(), i - 1, j + 1, k, width, height, length, point_square_index, bigger_square_radii_template[1].data())) continue;
					if(Is_Bigger_Square(distance_map.data(), i - 1, j, k - 1, width, height, length, point_square_index, bigger_square_radii_template[1].data())) continue;
					if(Is_Bigger_Square(distance_map.data(), i - 1, j, k + 1, width, height, length, point_square_index, bigger_square_radii_template[1].data())) continue;
					if(Is_Bigger_Square(distance_map.data(), i + 1, j - 1, k, width, height, length, point_square_index, bigger_square_radii_template[1].data())) continue;
					if(Is_Bigger_Square(distance_map.data(), i + 1, j + 1, k, width, height, length, point_square_index, bigger_square_radii_template[1].data())) continue;
					if(Is_Bigger_Square(distance_map.data(), i + 1, j, k - 1, width, height, length, point_square_index, bigger_square_radii_template[1].data())) continue;
					if(Is_Bigger_Square(distance_map.data(), i + 1, j, k + 1, width, height, length, point_square_index, bigger_square_radii_template[1].data())) continue;
					if(Is_Bigger_Square(distance_map.data(), i, j - 1, k - 1, width, height, length, point_square_index, bigger_square_radii_template[1].data())) continue;
					if(Is_Bigger_Square(distance_map.data(), i, j - 1, k + 1, width, height, length, point_square_index, bigger_square_radii_template[1].data())) continue;
					if(Is_Bigger_Square(distance_map.data(), i, j + 1, k - 1, width, height, length, point_square_index, bigger_square_radii_template[1].data())) continue;
					if(Is_Bigger_Square(distance_map.data(), i, j + 1, k + 1, width, height, length, point_square_index, bigger_square_radii_template[1].data())) continue;
					if(Is_Bigger_Square(distance_map.data(), i - 1, j - 1, k - 1, width, height, length, point_square_index, bigger_square_radii_template[2].data())) continue;
					if(Is_Bigger_Square(distance_map.data(), i - 1, j - 1, k + 1, width, height, length, point_square_index, bigger_square_radii_template[2].data())) continue;
					if(Is_Bigger_Square(distance_map.data(), i - 1, j + 1, k - 1, width, height, length, point_square_index, bigger_square_radii_template[2].data())) continue;
					if(Is_Bigger_Square(distance_map.data(), i - 1, j + 1, k + 1, width, height, length, point_square_index, bigger_square_radii_template[2].data())) continue;
					if(Is_Bigger_Square(distance_map.data(), i + 1, j - 1, k - 1, width, height, length, point_square_index, bigger_square_radii_template[2].data())) continue;
					if(Is_Bigger_Square(distance_map.data(), i + 1, j - 1, k + 1, width, height, length, point_square_index, bigger_square_radii_template[2].data())) continue;
					if(Is_Bigger_Square(distance_map.data(), i + 1, j + 1, k - 1, width, height, length, point_square_index, bigger_square_radii_template[2].data())) continue;
					if(Is_Bigger_Square(distance_map.data(), i + 1, j + 1, k + 1, width, height, length, point_square_index, bigger_square_radii_template[2].data())) continue;

					ridge_map[index] = distance_map[index];

// 					vector<int> x_pos;
// 					x_pos.push_back(0);
// 					if (i == 0) x_pos.push_back(1);
// 					else if (i == width - 1) x_pos.push_back(-1);
// 					else {
// 						x_pos.push_back(1);
// 						x_pos.push_back(-1);
// 					};
// 
// 					vector<int> y_pos;
// 					y_pos.push_back(0);
// 					if (j == 0) y_pos.push_back(1);
// 					else if (j == height - 1) y_pos.push_back(-1);
// 					else {
// 						y_pos.push_back(1);
// 						y_pos.push_back(-1);
// 					}
// 
// 					vector<int> z_pos;
// 					z_pos.push_back(0);
// 					if (k == 0) z_pos.push_back(1);
// 					else if (k == length - 1) z_pos.push_back(-1);
// 					else {
// 						z_pos.push_back(1);
// 						z_pos.push_back(-1);
// 					}
// 
// 					float p = distance_map[0];
// 					bool skip = true;
// 					for(int x=0; x<x_pos.size(); x++) {
// 						for (int y=0; y<y_pos.size(); y++) {
// 							for (int z=0; z<z_pos.size(); z++) {
// 								if (skip) {
// 									skip = false;
// 									continue;
// 								}
// 								round_point_square = distance_map[index + z_pos[z] * width * height + y_pos[y] * width + x_pos[x]];
// 								if (round_point_square > bigger_square_radii_template[abs(x_pos[x]) + abs(y_pos[y]) + abs(z_pos[z]) - 1][point_square_index]) {
// 									goto CONTINUE;
// 								}
// 							}
// 						}
// 					}
// 					ridge_map[index] = distance_map[index];
// CONTINUE:
// 					continue;
 				}
			}
		}
	}
}

void BoneThicknessCalculator::MultiDistanceRidge(const vector<float>& distance_map, vector<float>& ridge_map, const size_t width, const size_t height, const size_t length, float& max_radii, const int thread_number)
{
	float max_distance = 0.0f;
	for (size_t i=0; i<distance_map.size(); i++) {
		if (max_distance < distance_map[i]) max_distance = distance_map[i];
	}

	max_distance = sqrt(max_distance);
	max_radii = max_distance;

	assert(distance_map.size() == width * height * length);

	const int distance_squared = static_cast<int>(max_distance * max_distance + 0.5f) + 1;

	vector<int> occurs;
	occurs.resize(distance_squared, 0);

	for (size_t i=0; i<distance_map.size(); i++) {
		occurs[static_cast<int>(distance_map[i])] = 1;
	}

	int radii_number = 0;
	for (int i=0; i<occurs.size(); i++) {
		if (occurs[i] == 1) radii_number++;
	}

	vector<int> distance_squared_index;
	distance_squared_index.resize(occurs.size(), 0);
	vector<int> distance_squared_values;
	distance_squared_values.resize(radii_number, 0);

	int index = 0;

	for (int i = 0; i < occurs.size(); i++) {
		if (occurs[i] == 1) {
			distance_squared_index[i] = index;
			distance_squared_values[index++] = i;
		}
	}

	// 有一个圆，半径为 r， 求 r' 使得当圆的原点移动到（0，0，1）， （0，1，1）， （1，1，1） 时， r' 形成的新圆能够包含 r 形成的圆所. 
	vector <vector<int>> bigger_square_radii_template;
	CreateTemplate(distance_squared_values, bigger_square_radii_template);

	boost::thread_group grp;

	const size_t size = length / thread_number;

	BASIC_INFO_THREE info(width, height, length, 0, 0);

	for (size_t i=0; i<thread_number; i++) {
		info.start = i * size;
		info.end = i  == (thread_number - 1) ? length : (i + 1) * size;
		grp.create_thread(boost::bind(boost::mem_fn(&BoneThicknessCalculator::__DistanceRidge), this, distance_map.data(), ridge_map.data(), info, distance_squared_index, bigger_square_radii_template));
	}

	grp.join_all();
}

void BoneThicknessCalculator::__DistanceRidge(const float* distance_map, float* ridge_map, const BASIC_INFO_THREE info, const vector<int>& distance_squared_index, const vector<vector<int>>& bigger_square_radii_template)
{
	int point_square_index, point_square;
	bool not_ridge_point = false;

	const size_t width = info.width;
	const size_t height = info.height;
	const size_t length = info.length;
	const size_t z_start = info.start;
	const size_t z_end = info.end;


	vector<int> x_pos, y_pos, z_pos;
	x_pos.reserve(3);
	y_pos.reserve(3);
	z_pos.reserve(3);

	for (size_t z = z_start; z < z_end; z++) {
		theAppIVConfig.m_pILog->ProgressStepIt();
		for (size_t y = 0; y < height; y++) {
			for (size_t x = 0; x < width; x++) {
				size_t index = z * width * height + y * width + x;
				if (distance_map[index] > 0) {
					not_ridge_point = false;
					point_square = static_cast<int>(distance_map[index]);
					point_square_index = distance_squared_index[point_square];
					int i = static_cast<int>(x);
					int j = static_cast<int>(y);
					int k = static_cast<int>(z);

					if(Is_Bigger_Square(distance_map, i - 1, j, k, width, height, length, point_square_index, bigger_square_radii_template[0].data())) continue;
					if(Is_Bigger_Square(distance_map, i, j - 1, k, width, height, length, point_square_index, bigger_square_radii_template[0].data())) continue;
					if(Is_Bigger_Square(distance_map, i, j, k - 1, width, height, length, point_square_index, bigger_square_radii_template[0].data())) continue;
					if(Is_Bigger_Square(distance_map, i + 1, j, k, width, height, length, point_square_index, bigger_square_radii_template[0].data())) continue;
					if(Is_Bigger_Square(distance_map, i, j + 1, k, width, height, length, point_square_index, bigger_square_radii_template[0].data())) continue;
					if(Is_Bigger_Square(distance_map, i, j, k + 1, width, height, length, point_square_index, bigger_square_radii_template[0].data())) continue;
					if(Is_Bigger_Square(distance_map, i - 1, j - 1, k, width, height, length, point_square_index, bigger_square_radii_template[1].data())) continue;
					if(Is_Bigger_Square(distance_map, i - 1, j + 1, k, width, height, length, point_square_index, bigger_square_radii_template[1].data())) continue;
					if(Is_Bigger_Square(distance_map, i - 1, j, k - 1, width, height, length, point_square_index, bigger_square_radii_template[1].data())) continue;
					if(Is_Bigger_Square(distance_map, i - 1, j, k + 1, width, height, length, point_square_index, bigger_square_radii_template[1].data())) continue;
					if(Is_Bigger_Square(distance_map, i + 1, j - 1, k, width, height, length, point_square_index, bigger_square_radii_template[1].data())) continue;
					if(Is_Bigger_Square(distance_map, i + 1, j + 1, k, width, height, length, point_square_index, bigger_square_radii_template[1].data())) continue;
					if(Is_Bigger_Square(distance_map, i + 1, j, k - 1, width, height, length, point_square_index, bigger_square_radii_template[1].data())) continue;
					if(Is_Bigger_Square(distance_map, i + 1, j, k + 1, width, height, length, point_square_index, bigger_square_radii_template[1].data())) continue;
					if(Is_Bigger_Square(distance_map, i, j - 1, k - 1, width, height, length, point_square_index, bigger_square_radii_template[1].data())) continue;
					if(Is_Bigger_Square(distance_map, i, j - 1, k + 1, width, height, length, point_square_index, bigger_square_radii_template[1].data())) continue;
					if(Is_Bigger_Square(distance_map, i, j + 1, k - 1, width, height, length, point_square_index, bigger_square_radii_template[1].data())) continue;
					if(Is_Bigger_Square(distance_map, i, j + 1, k + 1, width, height, length, point_square_index, bigger_square_radii_template[1].data())) continue;
					if(Is_Bigger_Square(distance_map, i - 1, j - 1, k - 1, width, height, length, point_square_index, bigger_square_radii_template[2].data())) continue;
					if(Is_Bigger_Square(distance_map, i - 1, j - 1, k + 1, width, height, length, point_square_index, bigger_square_radii_template[2].data())) continue;
					if(Is_Bigger_Square(distance_map, i - 1, j + 1, k - 1, width, height, length, point_square_index, bigger_square_radii_template[2].data())) continue;
					if(Is_Bigger_Square(distance_map, i - 1, j + 1, k + 1, width, height, length, point_square_index, bigger_square_radii_template[2].data())) continue;
					if(Is_Bigger_Square(distance_map, i + 1, j - 1, k - 1, width, height, length, point_square_index, bigger_square_radii_template[2].data())) continue;
					if(Is_Bigger_Square(distance_map, i + 1, j - 1, k + 1, width, height, length, point_square_index, bigger_square_radii_template[2].data())) continue;
					if(Is_Bigger_Square(distance_map, i + 1, j + 1, k - 1, width, height, length, point_square_index, bigger_square_radii_template[2].data())) continue;
					if(Is_Bigger_Square(distance_map, i + 1, j + 1, k + 1, width, height, length, point_square_index, bigger_square_radii_template[2].data())) continue;

					ridge_map[index] = distance_map[index];
				}
			}
		}
	}
}

void BoneThicknessCalculator::LocalThickness(vector<float>& ridge_map, vector<float>& max_radii_map, const size_t width, const size_t height, const size_t length)
{
	vector<NEW_POSITION> valid_points;
	// assume there are 5% of ridge point, saving time for create new vector and copy data.
	valid_points.reserve(width * height * length / 20);
	size_t index = 0;
	for (size_t k = 0; k < length; k++) {
		for (size_t j = 0; j < height; j++) {
			for (size_t i = 0; i < width; i++) {
				index = k * width * height + j * width + i;
				if (ridge_map[index] > 0) {
					valid_points.push_back(NEW_POSITION(static_cast<int>(i), static_cast<int>(j), static_cast<int>(k), sqrt(ridge_map[index])));
				}
			}
		}
	}
	const size_t slice_size = valid_points.size() / 10;
	for (size_t index=0; index<valid_points.size(); index++) {
		if (slice_size == 0 || index % slice_size == 0) theAppIVConfig.m_pILog->ProgressStepIt();
		int squared_radii = static_cast<int>(valid_points[index].value * valid_points[index].value + 0.5f);

		int new_squared_radii;
		float original = 0.0f;
		int x = valid_points[index].x;
		int y = valid_points[index].y;
		int z = valid_points[index].z;

		int radii = static_cast<int>(valid_points[index].value);

		if (radii < valid_points[index].value) radii++;


		size_t position;

		int index_width = static_cast<int>(width) - 1;
		int index_height = static_cast<int>(height) - 1;
		int index_length = static_cast<int>(length) - 1;


		int i_start, i_end, j_start, j_end, k_start, k_end;
		
		i_start = x - radii > 0 ? x - radii : 0;
		i_end = x + radii <= index_width ? x + radii : index_width;
		
		j_start = y - radii > 0 ? y - radii : 0;
		j_end = y + radii <= index_height ? y + radii : index_height;
		
		k_start = z - radii > 0 ? z - radii : 0;
		k_end = z + radii <= index_length ? z + radii : index_length;


		for (int k = k_start; k <= k_end; k++) {
			for (int j = j_start; j <= j_end; j++) {
				for (int i = i_start; i <= i_end; i++) {
					new_squared_radii = (i - x) * (i - x) + (j - y) * (j - y) + (k - z) * (k - z);
					position = static_cast<size_t>(k) * width * height + static_cast<size_t>(j) * width + static_cast<size_t>(i);
					original = max_radii_map[position];
					if (new_squared_radii <= squared_radii && squared_radii > original) {
						max_radii_map[position] = static_cast<float>(squared_radii);
					}
				}
			}
		}
	}

	for (size_t index = 0; index < max_radii_map.size(); index++) {
		max_radii_map[index] = sqrt(max_radii_map[index]) * 2.0f;
	}

}

void BoneThicknessCalculator::MultiLocalThickness(vector<float>& ridge_map, vector<float>& max_radii_map, const size_t width, const size_t height, const size_t length, const int thread_number)
{
	boost::thread_group grp;

	const size_t size = length / thread_number;

	for (size_t i=0; i<thread_number; i++) {
		const size_t start = i * size;
		const size_t end = (i  == (thread_number - 1)) ? length : (i + 1) * size;
		grp.create_thread(boost::bind(boost::mem_fn(&BoneThicknessCalculator::__LocalThickness), this, ridge_map.data(), max_radii_map.data(), width, height, length, start, end));
	}

	grp.join_all();

	for (size_t index = 0; index < max_radii_map.size(); index++) {
		max_radii_map[index] = sqrt(max_radii_map[index]) * 2.0f;
	}
}

void BoneThicknessCalculator::__LocalThickness(const float* ridge_map, float* max_radii_map, const size_t width, const size_t height, const size_t length, const size_t z_start, const size_t z_end)
{
	vector<NEW_POSITION> valid_points;
	valid_points.reserve(width * height * (z_end - z_start) / 20);
	size_t index = 0;
	for (size_t k = z_start; k < z_end; k++) {
		for (size_t j = 0; j < height; j++) {
			for (size_t i = 0; i < width; i++) {
				index = k * width * height + j * width + i;
				if (ridge_map[index] > 0) {
					valid_points.push_back(NEW_POSITION(static_cast<int>(i), static_cast<int>(j), static_cast<int>(k), sqrt(ridge_map[index])));
				}
			}
		}
	}

	const size_t slice_size = valid_points.size() / 10;
	for (size_t index=0; index<valid_points.size(); index++) {
		if (slice_size == 0 || index % slice_size == 0) theAppIVConfig.m_pILog->ProgressStepIt();
		int squared_radii = static_cast<int>(valid_points[index].value * valid_points[index].value + 0.5f);

		int new_squared_radii;
		float original = 0.0f;
		int x = valid_points[index].x;
		int y = valid_points[index].y;
		int z = valid_points[index].z;

		int radii = static_cast<int>(valid_points[index].value);

		if (radii < valid_points[index].value) radii++;

		size_t position;

		int index_width = static_cast<int>(width) - 1;
		int index_height = static_cast<int>(height) - 1;
		int index_length = static_cast<int>(length) - 1;

		int i_start, i_end, j_start, j_end, k_start, k_end;

		i_start = x - radii > 0 ? x - radii : 0;
		i_end = x + radii <= index_width ? x + radii : index_width;

		j_start = y - radii > 0 ? y - radii : 0;
		j_end = y + radii <= index_height ? y + radii : index_height;

		k_start = z - radii > 0 ? z - radii : 0;
		k_end = z + radii <= index_length ? z + radii : index_length;

		for (int k = k_start; k <= k_end; k++) {
			for (int j = j_start; j <= j_end; j++) {
				for (int i = i_start; i <= i_end; i++) {
					new_squared_radii = (i - x) * (i - x) + (j - y) * (j - y) + (k - z) * (k - z);
					position = static_cast<size_t>(k) * width * height + static_cast<size_t>(j) * width + static_cast<size_t>(i);
					original = max_radii_map[position];
					if (new_squared_radii <= squared_radii && squared_radii > original) {
						max_radii_map[position] = static_cast<float>(squared_radii);
					}
				}
			}
		}
	}
}

void BoneThicknessCalculator::ThicknessCleaning(vector<float>& radii_map, vector<float>& thickness_map, const size_t width, const size_t height, const size_t length)
{
	for (size_t k = 0; k < length; k++) {
		theAppIVConfig.m_pILog->ProgressStepIt();
		for (size_t j = 0; j < height; j++) {
			for (size_t i = 0; i < width; i++) {
				size_t pos = k * height * width + j * width + i;
				thickness_map[pos] =  SetFlag(radii_map.data(), i, j, k, width, height, length);
			}
		}
	}

	for (size_t k = 0; k < length; k++) {
		theAppIVConfig.m_pILog->ProgressStepIt();
		for (size_t j = 0; j < height; j++) {
			for (size_t i = 0; i < width; i++) {
				const size_t pos = k * height * width + j * width + i;
				const float data = thickness_map[pos];
				if (data == -1) 
					thickness_map[pos] = -AverageInteriorNeighbors(thickness_map.data(), radii_map.data(), i, j, k, width, height, length);
			}
		}
	}

	for(size_t i=0; i<thickness_map.size(); i++) {
		thickness_map[i] = abs(thickness_map[i]);
	}
}


void BoneThicknessCalculator::MultiThicknessClean(vector<float>& radii_map, vector<float>& thickness_map, const size_t width, const size_t height, const size_t length, const int thread_number)
{
	boost::thread_group grp;

	const size_t size = length / thread_number;

	for (size_t i=0; i<thread_number; i++) {
		const size_t start = i * size;
		const size_t end = i  == (thread_number - 1) ? length : (i + 1) * size;
		grp.create_thread(boost::bind(boost::mem_fn(&BoneThicknessCalculator::__ThicknessCleanSetFlag), this, radii_map.data(), thickness_map.data(), width, height, length, start, end));
 	}

	grp.join_all();

	boost::thread_group grp_2;

	for (size_t i=0; i<thread_number; i++) {
		const size_t start = i * size;
		const size_t end = i  == (thread_number - 1) ? length : (i + 1) * size;
		grp_2.create_thread(boost::bind(boost::mem_fn(&BoneThicknessCalculator::__ThicknessCleanAverageInterNeighbor), this, radii_map.data(), thickness_map.data(), width, height, length, start, end));
	}

	grp_2.join_all();

	for(size_t i=0; i<thickness_map.size(); i++) {
		thickness_map[i] = abs(thickness_map[i]);
	}
}

void BoneThicknessCalculator::__ThicknessCleanSetFlag(const float* radii_map, float* thickness_map, const size_t width, const size_t height, const size_t length, const size_t z_start, const size_t z_end)
{
	for (size_t k = z_start; k < z_end; k++) {
		theAppIVConfig.m_pILog->ProgressStepIt();
		for (size_t j = 0; j < height; j++) {
			for (size_t i = 0; i < width; i++) {
				size_t pos = k * height * width + j * width + i;
				thickness_map[pos] =  SetFlag(radii_map, i, j, k, width, height, length);
			}
		}
	}
}

void BoneThicknessCalculator::__ThicknessCleanAverageInterNeighbor(const float* radii_map, float* thickness_map, const size_t width, const size_t height, const size_t length, const size_t z_start, const size_t z_end)
{
	for (size_t k = z_start; k < z_end; k++) {
		theAppIVConfig.m_pILog->ProgressStepIt();
		for (size_t j = 0; j < height; j++) {
			for (size_t i = 0; i < width; i++) {
				size_t pos = k * height * width + j * width + i;
				if (thickness_map[pos] == -1) 
					thickness_map[pos] = -AverageInteriorNeighbors(thickness_map, radii_map, i, j, k, width, height, length);
			}
		}
	}
}


void BoneThicknessCalculator::CreateTemplate(vector<int>& distance_square_values, vector<vector<int>>& radii_square_template)
{
	vector<int> temp_cube;
	temp_cube.reserve(radii_square_template.size());
	ScanCube(1, 0, 0, distance_square_values, temp_cube);
	radii_square_template.push_back(temp_cube);
	ScanCube(1, 1, 0, distance_square_values, temp_cube);
	radii_square_template.push_back(temp_cube);
	ScanCube(1, 1, 1, distance_square_values, temp_cube);
	radii_square_template.push_back(temp_cube);
}

void BoneThicknessCalculator::ScanCube(const int x, const int y, const int z, vector<int>& distance_square_values, vector<int>& cube)
{
	cube.clear();
	size_t radii_number = distance_square_values.size();
	if (x == 0 && y == 0 && z == 0) {
		for (size_t i = 0; i < radii_number; i++) {
			cube.push_back(INT_MAX);
		}
	}
	else {
		const int x_abs = x > 0 ? -x : x;
		const int y_abs = y > 0 ? -y : y;
		const int z_abs = z > 0 ? -z : z;
// 		for (int radii_square_index = 0; radii_square_index < radii_number; radii_square_index++) {
// 			const int radii_square = distance_square_values[radii_square_index];
// 			int max = 0;
// 			const int radii = static_cast<int>(sqrt(static_cast<double>(radii_square))) + 1;
// 
// 		}

		for (int index=0; index < distance_square_values.size(); index++) {
			const int radii = static_cast<int>(sqrt(static_cast<double>(distance_square_values[index]))) + 1;
			int ijk, i;
			int max = 0;
			for (int k = 0; k < radii; k++) {
				for (int j = 0; j < radii; j++) {
					if (((k * k) + (j * j)) <= distance_square_values[index]) {
						i = static_cast<int>(sqrt(static_cast<double>(distance_square_values[index] - ((k * k) + (j * j)))));
						ijk = ((k - z_abs) * (k - z_abs)) + ((j - y_abs) * (j - y_abs)) + ((i - x_abs) * (i - x_abs));
						max = max > ijk ? max : ijk;
					}
				}
			}
			cube.push_back(max);
		}

//		auto iter = distance_square_values.begin();
// 		while (iter != distance_square_values.end()) {
// 			const int radii = static_cast<int>(sqrt(static_cast<double>(*iter))) + 1;
// 			int ijk, i;
// 			int max = 0;
// 			for (int k = 0; k < radii; k++) {
// 				for (int j = 0; j < radii; j++) {
// 					if (((k * k) + (j * j)) <= *iter) {
// 						i = static_cast<int>(sqrt(static_cast<double>(*iter - ((k * k) + (j * j)))));
// 						ijk = ((k - z_abs) * (k - z_abs)) + ((j - y_abs) * (j - y_abs)) + ((i - x_abs) * (i - x_abs));
// 						max = max > ijk ? max : ijk;
// 					}
// 				}
// 			}
// 			cube.push_back(max);
// 			iter++;
// 		}
	}
}

float BoneThicknessCalculator::SetFlag(const float* data, const size_t i, const size_t j, const size_t k, const size_t width, const size_t height, const size_t length)
{
	
	if (data[k * width * height + j * width + i] == 0) return 0;
	// change 1
	if (Look(data, i, j, k - 1, width, height, length) == 0) return -1;
	if (Look(data, i, j, k + 1, width, height, length) == 0) return -1;
	if (Look(data, i, j - 1, k, width, height, length) == 0) return -1;
	if (Look(data, i, j + 1, k, width, height, length) == 0) return -1;
	if (Look(data, i - 1, j, k, width, height, length) == 0) return -1;
	if (Look(data, i + 1, j, k, width, height, length) == 0) return -1;
	// change 1 before plus
	if (Look(data, i, j + 1, k - 1, width, height, length) == 0) return -1;
	if (Look(data, i, j + 1, k + 1, width, height, length) == 0) return -1;
	if (Look(data, i + 1, j - 1, k, width, height, length) == 0) return -1;
	if (Look(data, i + 1, j + 1, k, width, height, length) == 0) return -1;
	if (Look(data, i - 1, j, k + 1, width, height, length) == 0) return -1;
	if (Look(data, i + 1, j, k + 1, width, height, length) == 0) return -1;
	// change 1 before minus
	if (Look(data, i, j - 1, k - 1, width, height, length) == 0) return -1;
	if (Look(data, i, j - 1, k + 1, width, height, length) == 0) return -1;
	if (Look(data, i - 1, j - 1, k, width, height, length) == 0) return -1;
	if (Look(data, i - 1, j + 1, k, width, height, length) == 0) return -1;
	if (Look(data, i - 1, j, k - 1, width, height, length) == 0) return -1;
	if (Look(data, i + 1, j, k - 1, width, height, length) == 0) return -1;
	// change 3, k+1
	if (Look(data, i + 1, j + 1, k + 1, width, height, length) == 0) return -1;

	if (Look(data, i + 1, j - 1, k + 1, width, height, length) == 0) return -1;
	if (Look(data, i - 1, j + 1, k + 1, width, height, length) == 0) return -1;
	if (Look(data, i - 1, j - 1, k + 1, width, height, length) == 0) return -1;
	// change 3, k-1
	if (Look(data, i + 1, j + 1, k - 1, width, height, length) == 0) return -1;
	if (Look(data, i + 1, j - 1, k - 1, width, height, length) == 0) return -1;
	if (Look(data, i - 1, j + 1, k - 1, width, height, length) == 0) return -1;
	if (Look(data, i - 1, j - 1, k - 1, width, height, length) == 0) return -1;
	return data[k * width * height + j * width + i];
}

float BoneThicknessCalculator::AverageInteriorNeighbors(const float* data, const float* ori, const size_t i, const size_t j, const size_t k, const size_t width, const size_t height, const size_t length)
{
	int n = 0;
	float sum = 0;

	float value = Look(data, i, j, k - 1, width, height, length);
	if (value > 0) {
		n++;
		sum += value;
	}
	value = Look(data, i, j, k + 1, width, height, length);
	if (value > 0) {
		n++;
		sum += value;
	}
	value = Look(data, i, j - 1, k, width, height, length);
	if (value > 0) {
		n++;
		sum += value;
	}
	value = Look(data, i, j + 1, k, width, height, length);
	if (value > 0) {
		n++;
		sum += value;
	}
	value = Look(data, i - 1, j, k, width, height, length);
	if (value > 0) {
		n++;
		sum += value;
	}
	value = Look(data, i + 1, j, k, width, height, length);
	if (value > 0) {
		n++;
		sum += value;
	}
	value = Look(data, i, j + 1, k - 1, width, height, length);
	if (value > 0) {
		n++;
		sum += value;
	}
	value = Look(data, i, j + 1, k + 1, width, height, length);
	if (value > 0) {
		n++;
		sum += value;
	}
	value = Look(data, i + 1, j - 1, k, width, height, length);
	if (value > 0) {
		n++;
		sum += value;
	}
	value = Look(data, i + 1, j + 1, k, width, height, length);
	if (value > 0) {
		n++;
		sum += value;
	}
	value = Look(data, i - 1, j, k + 1, width, height, length);
	if (value > 0) {
		n++;
		sum += value;
	}
	value = Look(data, i + 1, j, k + 1, width, height, length);
	if (value > 0) {
		n++;
		sum += value;
	}
	value = Look(data, i, j - 1, k - 1, width, height, length);
	if (value > 0) {
		n++;
		sum += value;
	}
	value = Look(data, i, j - 1, k + 1, width, height, length);
	if (value > 0) {
		n++;
		sum += value;
	}
	value = Look(data, i - 1, j - 1, k, width, height, length);
	if (value > 0) {
		n++;
		sum += value;
	}
	value = Look(data, i - 1, j + 1, k, width, height, length);
	if (value > 0) {
		n++;
		sum += value;
	}
	value = Look(data, i - 1, j, k - 1, width, height, length);
	if (value > 0) {
		n++;
		sum += value;
	}
	value = Look(data, i + 1, j, k - 1, width, height, length);
	if (value > 0) {
		n++;
		sum += value;
	}
	value = Look(data, i + 1, j + 1, k + 1, width, height, length);
	if (value > 0) {
		n++;
		sum += value;
	}
	value = Look(data, i + 1, j - 1, k + 1, width, height, length);
	if (value > 0) {
		n++;
		sum += value;
	}
	value = Look(data, i - 1, j + 1, k + 1, width, height, length);
	if (value > 0) {
		n++;
		sum += value;
	}
	value = Look(data, i - 1, j - 1, k + 1, width, height, length);
	if (value > 0) {
		n++;
		sum += value;
	}
	value = Look(data, i + 1, j + 1, k - 1, width, height, length);
	if (value > 0) {
		n++;
		sum += value;
	}
	value = Look(data, i + 1, j - 1, k - 1, width, height, length);
	if (value > 0) {
		n++;
		sum += value;
	}
	value = Look(data, i - 1, j + 1, k - 1, width, height, length);
	if (value > 0) {
		n++;
		sum += value;
	}
	value = Look(data, i - 1, j - 1, k - 1, width, height, length);
	if (value > 0) {
		n++;
		sum += value;
	}

	if (n > 0) return sum / n;
	return ori[k * width * height + j * width + i];
}





