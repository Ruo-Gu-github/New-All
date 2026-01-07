#pragma once

/*
This class is for caculate the `average` value and `density` for region of interest.
data is original value for the dicom is short type.
*/

#include "ruogu_Calculator.h"

#include <string>
#include <vector>
#include <memory>
#include <map>

using std::string;
using std::vector;
using std::map;
using std::shared_ptr;


class DensityCalculator :
	public Calculator
{
public:
	DensityCalculator();

	virtual ~DensityCalculator();

public:
	string Calculation(shared_ptr<ImageStack> images, map<string, double>& results);

private:
	void PixelValueCount(const vector<short>& data, long long& value, long& count, const int low_value, const int high_value);

	void MultiPixelValueCount(const vector<short>& data, long long& value, long& count, const int low_value, const int high_value, int thread_number);

	void __PixelValueCount(const short* data, size_t start, size_t end, long long& value, long& count, const int low_value, const int high_value);
};

