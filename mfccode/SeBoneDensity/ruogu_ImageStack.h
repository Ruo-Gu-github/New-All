#pragma once

#include <vector>



using std::vector;

struct NEW_POINT3D
{
	double x;
	double y;
	double z;

	NEW_POINT3D(double _x, double _y, double _z):
	x(_x), y(_y), z(_z) {

	}

	NEW_POINT3D() {}

	NEW_POINT3D& operator+=(const NEW_POINT3D& point) {
		this->x += point.x;
		this->y += point.y;
		this->z += point.z;
		return *this;
	}

	bool operator<(const NEW_POINT3D& point) const{
		if (this->z != point.z) {
			return this->z < point.z;
		}
		else if (this->y != point.y) {
			return this->y < point.y;
		}
		else {
			return this->x < point.x;
		}
	}
};

struct TRIANGLE
{
	NEW_POINT3D p[3];
	TRIANGLE(NEW_POINT3D _p[3])
	{
		memcpy(&p[0], &_p[0], sizeof(NEW_POINT3D) * 3);
	}

	TRIANGLE() {
		this->p[0] = NEW_POINT3D(0.0f, 0.0f, 0.0f);
		this->p[1] = NEW_POINT3D(0.0f, 0.0f, 0.0f);
		this->p[2] = NEW_POINT3D(0.0f, 0.0f, 0.0f);
	}
};

struct GRIDCELL {
	NEW_POINT3D p[8];
	double val[8];
	BYTE valid[8];

	GRIDCELL(NEW_POINT3D _p[8], double _val[8], BYTE _valid[8])
	{
		memcpy(&p[0], &_p[0], sizeof(NEW_POINT3D) * 8);
		memcpy(&val[0], &_val[0], sizeof(double) * 8);
		memcpy(&valid[0], &_valid[0], sizeof(BYTE) * 8);
	}
};

struct NEW_POSITION {
	int x;
	int y;
	int z;
	double value;

	NEW_POSITION(int _x, int _y, int _z, double _value):
	x(_x), y(_y), z(_z), value(_value) {

	}
};

struct BASIC_INFO {
	size_t width;
	size_t height;
	size_t length;
	int z_start;
	int z_end;
	double pixel_size;
	double pixel_spacing;
	int low_value;

	BASIC_INFO(size_t _width, size_t _height, size_t _length, int _z_start, int _z_end, double _pixel_size, double _pixel_spacing, int _low_value):
	width(_width), height(_height), length(_length), z_start(_z_start), z_end(_z_end), pixel_size(_pixel_size), pixel_spacing(_pixel_spacing), low_value(_low_value) {

	}
};

struct BASIC_INFO_TWO {
	int low_value;
	int high_value;
	int empty_value;
	size_t start;
	size_t end;
	size_t width_height;
	BASIC_INFO_TWO(int _low_value, int _high_value, int _empty_value, size_t _start, size_t _end, size_t _width_height):
	low_value(_low_value), high_value(_high_value), empty_value(_empty_value), start(_start), end(_end), width_height(_width_height) {

	}
};

struct BASIC_INFO_THREE {
	size_t width;
	size_t height;
	size_t length;
	size_t start;
	size_t end;
	BASIC_INFO_THREE(size_t _width, size_t _height, size_t _length, size_t _start, size_t _end):
	width(_width), height(_height), length(_length), start(_start), end(_end) {

	}
};

class ImageStack
{
public:
	ImageStack(vector<short> data, size_t width, size_t height, size_t length, int low_value, int high_value, int empty_value);
	~ImageStack();

public:
	const vector<short> data_;
	const size_t width_;
	const size_t height_;
	const size_t length_;
	const int low_value_;
	const int high_value_;
	const int empty_value_;

	double pixel_size_;
	double pixel_spacing_;

	double low_density_;
	double high_density_;
	double low_ct_value_for_density_;
	double high_ct_value_for_density_;

	vector<TRIANGLE> triangels_;
	vector<TRIANGLE> dilated_triangles_;
	vector<NEW_POINT3D> normal_vevtors_;

	vector<short> dilate_data_;
	// 0 is empty and roi without bone, 1 is bone.
	vector<BYTE> binary_data_;
	vector<BYTE> dilated_binary_data_;
	// 0 is empty, 1 is roi without bone, 2 is bone.
	vector<BYTE> trinary_data_;

	long bone_pixel_num_;
	long tissue_and_bone_pixel_num_;
	long long bone_total_value_;

	double mean_bone_ct_value_;

	double surface_area_;
	double dilated_surface_area_;

	int thread_number_;
};

