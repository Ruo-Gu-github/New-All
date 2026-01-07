#include "stdafx.h"
#include "ruogu_ImageStack.h"
#include "windows.h"

ImageStack::ImageStack(vector<short> data, size_t width, size_t height, size_t length, int low_value, int high_value, int empty_value) :
	data_(data), width_(width), height_(height), length_(length), low_value_(low_value), high_value_(high_value), empty_value_(empty_value)
{
	pixel_size_ = 1.0f;
	pixel_spacing_ = 1.0f;

	low_density_ = 0.0f;
	high_density_ = 0.0f;
	low_ct_value_for_density_ = 0.0f;
	high_ct_value_for_density_ = 0.0f;

	bone_pixel_num_ = 0;
	tissue_and_bone_pixel_num_ = 0;
	bone_total_value_ = 0;

	mean_bone_ct_value_ = 0.0f;

	surface_area_ = 0.0f;
	dilated_surface_area_ = 0.0f;
	
	SYSTEM_INFO sysInfo;
	GetSystemInfo( &sysInfo );
	thread_number_ = sysInfo.dwNumberOfProcessors > 1 ? sysInfo.dwNumberOfProcessors : 1;
}

ImageStack::~ImageStack()
{
}
