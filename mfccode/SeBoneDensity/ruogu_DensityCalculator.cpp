#include "stdafx.h"
#include "ruogu_DensityCalculator.h"

using std::pair;


DensityCalculator::DensityCalculator()
{
}


DensityCalculator::~DensityCalculator()
{
}


string DensityCalculator::Calculation(shared_ptr<ImageStack> images, map<string, double>& results)
{
	string error = "";
	static const double EPSILON = 0.001;
	const vector<short>& data = images->data_;
	const int thread_number = images->thread_number_;

	long long total_value = 0;
	long total_pixel = 0;
	double average;
	double density;

	const double low_density = images->low_density_;
	const int ct_value_for_low_density = images->low_ct_value_for_density_;
	const double high_density = images->high_density_;
	const int ct_value_for_high_density = images->high_ct_value_for_density_;
	const int binary_low_value = images->low_value_;
	const int binary_high_value = images->high_value_;

//  for test data.	
// 	const double low_density = 0.0f;
// 	const double high_density = 1.0f;
// 	const int ct_value_for_low_density = 0;
// 	const int ct_value_for_high_density = 1000;

	if (high_density - low_density < EPSILON) {
		error = "high density should bigger than low density.";
		return error;
	}

	if (ct_value_for_low_density >= ct_value_for_high_density) {
		error = "CT value for high density should bigger than low density.";
		return error;
	}

#ifdef USE_MULTIPLY_THREAD
	MultiPixelValueCount(data, total_value, total_pixel, binary_low_value, binary_high_value, thread_number);
#else
	PixelValueCount(data, total_value, total_pixel, binary_low_value, binary_high_value);
#endif

// 	if (images->bone_pixel_num_ && images->bone_total_value_) {
// 		total_pixel = images->bone_pixel_num_;
// 		total_value = images->bone_total_value_;
// 	}
// 	else {
// 		auto iter = data.begin();
// 		while (iter != data.end()) {
// 			if (*iter >= binary_high_value || *iter <= binary_low_value) continue;
// 
// 			total_value += *iter;
// 			total_pixel++;
// 
// 			iter++;
// 		}
// 		
// 		images->bone_pixel_num_ = total_pixel;
// 		images->bone_total_value_ = total_value;
// 
// 	}

	images->bone_pixel_num_ = total_pixel;
	images->bone_total_value_ = total_value;

	average = total_value / static_cast<double>(total_pixel);
	density = (high_density - low_density) / (ct_value_for_high_density - ct_value_for_low_density) * average +
		((ct_value_for_high_density * low_density) - (ct_value_for_low_density * high_density)) /
		(ct_value_for_high_density - ct_value_for_low_density);

	results.insert(pair<string, double>("Average", average));
	results.insert(pair<string, double>("BMD", density));

	// ¡Ÿ ±
	LONGLONG value = 0;
	LONGLONG count = 0;
	const size_t step_size = data.size() / 10;
	for(int i=0; i<data.size(); i++) {
		if (data[i] > 0 && data[i] <= binary_high_value) {
			value += data[i];
			count++;
		}
	}
	
	double meanvalue = (double)value/(double)count;
	double den = (high_density - low_density) / (ct_value_for_high_density - ct_value_for_low_density) * meanvalue +
		((ct_value_for_high_density * low_density) - (ct_value_for_low_density * high_density)) /
		(ct_value_for_high_density - ct_value_for_low_density);

	results.insert(pair<string, double>("AverageTotal", meanvalue));
	results.insert(pair<string, double>("DensityTotal", den));

	return error;
}

void DensityCalculator::PixelValueCount(const vector<short>& data, long long& value, long& count, const int low_value, const int high_value)
{
	value = 0;
	count = 0;
	const size_t step_size = data.size() / 10;
	for(int i=0; i<data.size(); i++) {
		if (i % step_size == 0) theAppIVConfig.m_pILog->ProgressStepIt(); 
		if (data[i] >= low_value && data[i] <= high_value) {
			value += data[i];
			count++;
		}
	}
}

void DensityCalculator::MultiPixelValueCount(const vector<short>& data, long long& value, long& count, const int low_value, const int high_value, int thread_number)
{
	boost::thread_group grp;

	value = 0;
	count = 0;

	size_t size = data.size() / thread_number;
	size_t last_size = size + data.size() % thread_number;

	vector<long long> values;
	values.resize(thread_number, 0);
	vector<long> pixels;
	pixels.resize(thread_number, 0);

	for(size_t i=0; i<thread_number; i++) {
		const size_t start = i * size;
		const size_t end = i == thread_number ? data.size() : size * (i + 1);
		grp.create_thread(boost::bind(boost::mem_fn(&DensityCalculator::__PixelValueCount), this, data.data(), start, end, boost::ref(values[i]), boost::ref(pixels[i]), low_value, high_value));
	}

	grp.join_all();
	for(size_t i=0; i<thread_number; i++) {
		value += values[i];
		count += pixels[i];
	}
}

void DensityCalculator::__PixelValueCount(const short* data, size_t start, size_t end, long long& value, long& count, const int low_value, const int high_value)
{
	value = 0;
	count = 0;
	const size_t step_size = (end - start) / 10;
	for (size_t i=start; i<end; i++) {
		if ((i - start) % step_size == 0) theAppIVConfig.m_pILog->ProgressStepIt();
		if (data[i] >= low_value && data[i] <= high_value) {
			value += data[i];
			count++;
		}
	}
}
