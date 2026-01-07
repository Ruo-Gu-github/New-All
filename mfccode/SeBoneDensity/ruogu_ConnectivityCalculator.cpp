#include "stdafx.h"
#include "ruogu_ConnectivityCalculator.h"

ConnectivityCalculator::ConnectivityCalculator()
{
}

ConnectivityCalculator::~ConnectivityCalculator()
{
}

string ConnectivityCalculator::Calculation(shared_ptr<ImageStack> images, map<string, double>& results)
{
	string error = "";

	vector<BYTE>& data = images->binary_data_;
	const size_t width = images->width_;
	const size_t height = images->height_;
	const size_t length = images->length_;
	const double pixel_size = images->pixel_size_;
	const double pixel_spacing = images->pixel_spacing_;
	const int thread_number = images->thread_number_;

	double euler, edge_correction;
#ifdef USE_MULTIPLY_THREAD
	MultiEuler(data, width, height, length, euler, thread_number);
#else
	Euler(data, static_cast<int>(width), static_cast<int>(height), static_cast<int>(length), euler);
#endif
	EdgeCorrection(data, width, height, length, edge_correction);

	const double corrected_euler = euler - edge_correction;
	const double connectivity = 1 - corrected_euler;
	const double connectivity_density = connectivity / (static_cast<double>(width * height * length) * pixel_size * pixel_size * pixel_spacing);

	results.insert(make_pair<string, double>("Euler number", euler));
	results.insert(make_pair<string, double>("Conn", connectivity));
	results.insert(make_pair<string, double>("Conn.Dn", connectivity_density));
	return error;
}

void ConnectivityCalculator::Euler(const vector<BYTE>& data, const int width, const int height, const int length, double& euler)
{
	euler = 0.0f;

	for (int k=-1; k<static_cast<int>(length); k++) {
		theAppIVConfig.m_pILog->ProgressStepIt();
		for(int j=-1; j<static_cast<int>(height); j++) {
			for(int i=-1; i<static_cast<int>(width); i++) {
				const bool o1 = ValidPosition(data.data(), width, height, length, i, j, k); 
				const bool o2 = ValidPosition(data.data(), width, height, length, i, j + 1, k);
				const bool o3 = ValidPosition(data.data(), width, height, length, i + 1, j, k);
				const bool o4 = ValidPosition(data.data(), width, height, length, i + 1, j + 1, k);
				const bool o5 = ValidPosition(data.data(), width, height, length, i, j, k + 1);
				const bool o6 = ValidPosition(data.data(), width, height, length, i, j + 1, k + 1);
				const bool o7 = ValidPosition(data.data(), width, height, length, i + 1, j, k + 1);
				const bool o8 = ValidPosition(data.data(), width, height, length, i + 1, j + 1, k + 1);
// 				 const size_t index = dilated_width * dilated_height * k + dilated_width * j + i;
// 				 bool o1 = (dilated_data[index] != 0);
// 				 bool o2 = (dilated_data[index + dilated_width] != 0);
// 				 bool o3 = (dilated_data[index + 1] != 0);
// 				 bool o4 = (dilated_data[index + dilated_width + 1] != 0);
// 				 bool o5 = (dilated_data[index + dilated_width * dilated_height] != 0);
// 				 bool o6 = (dilated_data[index + dilated_width * dilated_height + dilated_width] != 0);
// 				 bool o7 = (dilated_data[index + dilated_width * dilated_height + 1] != 0);
// 				 bool o8 = (dilated_data[index + dilated_width * dilated_height + dilated_width + 1] != 0);
				 if (o1 || o2 || o3 || o4 || o5 || o6 || o7 || o8) {
					 euler += GetDeltaEuler(o1, o2, o3, o4, o5, o6, o7, o8);
				 }
			}
		}
	}
	
	euler = euler / 8.0f;

}

int ConnectivityCalculator::GetDeltaEuler(bool o1, bool o2, bool o3, bool o4, bool o5, bool o6, bool o7, bool o8)
{
	const static int EULER_LUT[256] = {1, 0, 0, -1, -2, -1, -1, 0, 0, -1, -1, -2, -3, -2, -2, -1, -2, -1, -3, -2, -1, -2, 0, -1, -1,
										0, -2, -1, 0, -1, 1, 0, -2, -3, -1, -2, -1, 0, -2, -1, -1, -2, 0, -1, 0, 1, -1, 0, -1, 0,
										0, 1, 4, 3, 3, 2, -2, -1, -1, 0, 3, 2, 2, 1, -6, -3, -3, 0, -3, -2, -2, -1, -3, 0, 0,
										3, 0, 1, 1, 2, -3, -2, 0, 1, 0 ,-1, 1, 0, -2, -1, 1, 2, 1, 0, 2, 1, -3, 0, -2, 1,
										0, 1, -1, 0, -2, 1, -1, 2, 1, 2, 0, 1, 0, 1, 1, 2, 3, 2, 2, 1, -1, 0, 0, 1, 2, 1, 1, 0};

	int index = 1;
	if (o8) {
		if (o1) { index |= 128; }
		if (o2) { index |= 64; }
		if (o3) { index |= 32; }
		if (o4) { index |= 16; }
		if (o5) { index |= 8; }
		if (o6) { index |= 4; }
		if (o7) { index |= 2; }
	} else if (o7) {
		if (o2) { index |= 128; }
		if (o4) { index |= 64; }
		if (o1) { index |= 32; }
		if (o3) { index |= 16; }
		if (o6) { index |= 8; }
		if (o5) { index |= 2; }
	} else if (o6) {
		if (o3) { index |= 128; }
		if (o1) { index |= 64; }
		if (o4) { index |= 32; }
		if (o2) { index |= 16; }
		if (o5) { index |= 4; }
	} else if (o5) {
		if (o4) { index |= 128; }
		if (o3) { index |= 64; }
		if (o2) { index |= 32; }
		if (o1) { index |= 16; }
	} else if (o4) {
		if (o1) { index |= 8; }
		if (o3) { index |= 4; }
		if (o2) { index |= 2; }
	} else if (o3) {
		if (o2) { index |= 8; }
		if (o1) { index |= 4; }
	} else if (o2) {
		if (o1) { index |= 2; }
	} else return 1;
	
	return EULER_LUT[static_cast<int>((index - 1) / 2)];
}

void ConnectivityCalculator::EdgeCorrection(const vector<BYTE>& data, const size_t width, const size_t height, const size_t length, double& edge_correction)
{
	const long chi_zero = StackCorners(data, width, height, length);
	theAppIVConfig.m_pILog->ProgressStepIt();
	const long e = StackEdges(data, width, height, length);
	theAppIVConfig.m_pILog->ProgressStepIt();
	const long d = VoxelEdgeIntersections(data, width, height, length) + chi_zero;
	theAppIVConfig.m_pILog->ProgressStepIt();
	const long c = StackFaces(data, width, height, length) + 2 * e - 3 * chi_zero;
	theAppIVConfig.m_pILog->ProgressStepIt();
	const long b = VoxelEdgeFaceIntersections(data, static_cast<int>(width), static_cast<int>(height), static_cast<int>(length));
	theAppIVConfig.m_pILog->ProgressStepIt();
	const long a = VoxelFaceIntersections(data, static_cast<int>(width), static_cast<int>(height), static_cast<int>(length));
	theAppIVConfig.m_pILog->ProgressStepIt();

	const long chi_one = d - e;
	const long chi_two = a - b + c;

	edge_correction = chi_two / 2.0f + chi_one / 4.0f + chi_zero / 8.0f;
}

long long ConnectivityCalculator::StackCorners(const vector<BYTE>& data, const size_t width, const size_t height, const size_t length)
{
	long long foreground_voxels = 0;
	foreground_voxels += data[0];
	foreground_voxels += data[width - 1];
	foreground_voxels += data[width * (height - 1)];
	foreground_voxels += data[width * height - 1];
	foreground_voxels += data[width * height * (length - 1)];
	foreground_voxels += data[width * height * (length - 1) + width - 1];
	foreground_voxels += data[width * height * (length - 1) + width * (height - 1)];
	foreground_voxels += data[width * height * length - 1];

	return foreground_voxels;
}

long long ConnectivityCalculator::StackEdges(const vector<BYTE>& data, const size_t width, const size_t height, const size_t length)
{
	long long foreground_voxels = 0;
	for (size_t x=0; x<width; x++) {
		foreground_voxels += data[x];
		foreground_voxels += data[width * (height - 1) + x];
		foreground_voxels += data[width * height * (length - 1) + x];
		foreground_voxels += data[width * height * (length - 1) + width * (height - 1) + x];
	}

	for (size_t y=0; y<height; y++) {
		foreground_voxels += data[y * width];
		foreground_voxels += data[width - 1 + y * width];
		foreground_voxels += data[width * height * (length - 1) + y * width];
		foreground_voxels += data[width * height * (length - 1) + width - 1 + y * width];
	}

	for (size_t z=0; z<length; z++) {
		foreground_voxels += data[z * width * height];
		foreground_voxels += data[width - 1 + z * width * height];
		foreground_voxels += data[width * (height - 1) + z * width * height];
		foreground_voxels += data[width * (height - 1) + width - 1 + z * width * height];
	}
	return foreground_voxels;
}


long long ConnectivityCalculator::StackFaces(const vector<BYTE>& data, const size_t width, const size_t height, const size_t length)
{
	long long foreground_voxels = 0;
	for (size_t x=0; x<width; x++) {
		for(size_t y=0; y<height; y++) {
			if (x == 0 || y == 0 || x == width - 1 || y == height - 1) continue;
			foreground_voxels += data[width * y + x];
			foreground_voxels += data[width * height * (length - 1) + width * y + x];
		}
	}

	for (size_t z=0; z<length; z++) {
		for (size_t x=0; x<width; x++) {
			if (x == 0 || z == 0 || x == width - 1 || z == length - 1) continue;
			foreground_voxels += data[width * height * z + x];
			foreground_voxels += data[width * height * z + width * (height - 1) + x];
		}
	}

	for (size_t z=0; z<length; z++) {
		for(size_t y=0; y<height; y++) {
			if (y == 0 || z == 0 || y == height - 1 || z == length - 1) continue;
			foreground_voxels += data[width * height * z + width * y];
			foreground_voxels += data[width * height * z + width * y + width - 1];
		}
	}

	return foreground_voxels;
}

long long ConnectivityCalculator::VoxelEdgeIntersections(const vector<BYTE>& data, const size_t width, const size_t height, const size_t length)
{
	long long voxel_vertices = 0;

	for (size_t x=1; x<width; x++) {
		voxel_vertices += data[x] | data[x - 1];
		voxel_vertices += data[width * (height - 1) + x] | data[width * (height - 1) + x - 1];
		voxel_vertices += data[width * height * (length - 1) + x] | data[width * height * (length - 1) + x - 1];
		voxel_vertices += data[width * height * (length - 1) + width * (height - 1) + x] | data[width * height * (length - 1) + width * (height - 1) + x - 1];
	}

	for (size_t y=1; y<height; y++) {
		voxel_vertices += data[y * width] | data[y * width - width];
		voxel_vertices += data[width - 1 + y * width] | data[width - 1 + y * width - width];
		voxel_vertices += data[width * height * (length - 1) + y * width] | data[width * height * (length - 1) + y * width - width];
		voxel_vertices += data[width * height * (length - 1) + width - 1 + y * width] | data[width * height * (length - 1) + width - 1 + y * width - width];
	}

	for (size_t z=1; z<length; z++) {
		voxel_vertices += data[z * width * height] | data[z * width * height - width * height];
		voxel_vertices += data[width - 1 + z * width * height] | data[width - 1 + z * width * height - width * height];
		voxel_vertices += data[width * (height - 1) + z * width * height] | data[width * (height - 1) + z * width * height - width * height];
		voxel_vertices += data[width * (height - 1) + width - 1 + z * width * height] | data[width * (height - 1) + width - 1 + z * width * height - width * height];
	}
	return voxel_vertices;
}

long long ConnectivityCalculator::VoxelEdgeFaceIntersections(const vector<BYTE>& data, const int width, const int height, const int length)
{
	long long voxel_verties = 0;

// 	vector<size_t> z;
// 	z.push_back(1);
// 	z.push_back(length);
// 	for(size_t k=0; k<z.size(); k++) {
// 		for(size_t y=1; y<=height; y++) {
// 			for(size_t x=1; x<=width; x++) {
// 				const size_t start = dilated_width * dilated_height * z[k] + dilated_width * y + x;
// 				const int pixelA = dilated_data[start];
// 				const int pixelB = dilated_data[start + 1];
// 				const int pixelC = dilated_data[start + dilated_width];
// 				if (pixelB | pixelA) voxel_verties++;
// 				if (pixelC | pixelA) voxel_verties++;
// 			}
// 		}
// 	}
// 
// 	vector<size_t> y;
// 	y.push_back(1);
// 	y.push_back(height);
// 	for(size_t z=1; z<=length; z++) {
// 		for(size_t j=0; j<y.size(); j++) {
// 			for(size_t x=1; x<=width; x++) {
// 				const size_t start = dilated_width * dilated_height * z + dilated_width * y[j] + x;
// 				const int pixelA = dilated_data[start];
// 				const int pixelB = dilated_data[start + 1];
// 				const int pixelC = dilated_data[start + dilated_width * dilated_height];
// 				if (pixelB | pixelA) voxel_verties++;
// 				if (pixelC | pixelA && z != length) voxel_verties++;
// 			}
// 		}
// 	}
// 
// 	vector<size_t> x;
// 	x.push_back(1);
// 	x.push_back(width);
// 	for(size_t z=1; z<=length; z++) {
// 		for(size_t y=1; y<=height; y++) {
// 			for(size_t i=0; i<x.size(); i++) {
// 				const size_t start = dilated_width * dilated_height * z + dilated_width * y + x[i];
// 				const int pixelA = dilated_data[start];
// 				const int pixelB = dilated_data[start + dilated_width];
// 				const int pixelC = dilated_data[start + dilated_width * dilated_height];
// 				if (pixelB | pixelA && y != height) voxel_verties++;
// 				if (pixelC | pixelA && z != length) voxel_verties++;
// 			}
// 		}
// 	}

	for(int k=0; k<length; k+=length-1) {
		for (int j=0; j<height; j++) {
			for(int i=0; i<width; i++) {
				const int pixelA = GetValueFromPosition(data.data(), width, height, length, i, j, k);
				const int pixelB = GetValueFromPosition(data.data(), width, height, length, i + 1, j, k);
				const int pixelC = GetValueFromPosition(data.data(), width, height, length, i, j + 1, k);
				if (pixelB | pixelA) voxel_verties++;
				if (pixelC | pixelA) voxel_verties++;
			}
		}
	}

	for(int k=0; k<length; k++) {
		for (int j=0; j<height; j+=height-1) {
			for(int i=0; i<width; i++) {
				const int pixelA = GetValueFromPosition(data.data(), width, height, length, i, j, k);
				const int pixelB = GetValueFromPosition(data.data(), width, height, length, i + 1, j, k);
				const int pixelC = GetValueFromPosition(data.data(), width, height, length, i, j, k + 1);
				if (pixelB | pixelA) voxel_verties++;
				if (pixelC | pixelA && k != length - 1) voxel_verties++;

			}
		}
	}

	for(int k=0; k<length; k++) {
		for (int j=0; j<height; j++) {
			for(int i=0; i<width; i+=width-1) {
				const int pixelA = GetValueFromPosition(data.data(), width, height, length, i, j, k);
				const int pixelB = GetValueFromPosition(data.data(), width, height, length, i, j + 1, k);
				const int pixelC = GetValueFromPosition(data.data(), width, height, length, i, j, k + 1);
				if (pixelB | pixelA && j != height - 1) voxel_verties++;
				if (pixelC | pixelA && k != length - 1) voxel_verties++;
			}
		}
	}

 	return voxel_verties;
}

long long ConnectivityCalculator::VoxelFaceIntersections(const vector<BYTE>& data, const int width, const int height, const int length)
{
	long long voxel_verties = 0;

	for(int k=0; k<length; k+=length-1) {
		for (int j=-1; j<height; j++) {
			for(int i=-1; i<width; i++) {
				const int pixelA = GetValueFromPosition(data.data(), width, height, length, i, j, k);
				const int pixelB = GetValueFromPosition(data.data(), width, height, length, i + 1, j, k);
				const int pixelC = GetValueFromPosition(data.data(), width, height, length, i, j + 1, k);
				const int pixelD = GetValueFromPosition(data.data(), width, height, length, i + 1, j + 1, k);
				voxel_verties += pixelA | pixelB | pixelC | pixelD;
			}
		}
	}

	for(int k=0; k<length-1; k++) {
		for (int j=-1; j<height; j++) {
			for(int i=0; i<width; i+=width-1) {
				const int pixelA = GetValueFromPosition(data.data(), width, height, length, i, j, k);
				const int pixelB = GetValueFromPosition(data.data(), width, height, length, i, j + 1, k);
				const int pixelC = GetValueFromPosition(data.data(), width, height, length, i, j, k + 1);
				const int pixelD = GetValueFromPosition(data.data(), width, height, length, i, j + 1, k + 1);
				voxel_verties += pixelA | pixelB | pixelC | pixelD;
			}
		}
	}

	for(int k=0; k<length-1; k++) {
		for (int j=0; j<height; j+=height-1) {
			for(int i=0; i<width-1; i++) {
				const int pixelA = GetValueFromPosition(data.data(), width, height, length, i, j, k);
				const int pixelB = GetValueFromPosition(data.data(), width, height, length, i + 1, j, k);
				const int pixelC = GetValueFromPosition(data.data(), width, height, length, i, j, k + 1);
				const int pixelD = GetValueFromPosition(data.data(), width, height, length, i + 1, j, k + 1);
				voxel_verties += pixelA | pixelB | pixelC | pixelD;
			}
		}
	}

// 	for(size_t y=0; y<=height; y++) {
// 		for (size_t x=0; x<=width; x++) {
// 			{
// 				const size_t start = dilated_width * dilated_height + y * dilated_width + x;
// 				const int pixelA = dilated_data[start];
// 				const int pixelB = dilated_data[start + 1];
// 				const int pixelC = dilated_data[start + dilated_width];
// 				const int pixelD = dilated_data[start + dilated_width + 1];
// 				voxel_verties += pixelA | pixelB | pixelC | pixelD;
// 			}
// 			{
// 				const size_t start = dilated_width * dilated_height * (dilated_length - 2)  + y * dilated_width + x;
// 				const int pixelA = dilated_data[start];
// 				const int pixelB = dilated_data[start + 1];
// 				const int pixelC = dilated_data[start + dilated_width];
// 				const int pixelD = dilated_data[start + dilated_width + 1];
// 				voxel_verties += pixelA | pixelB | pixelC | pixelD;
// 			}
// 		}
// 	}
// 
// 	for(size_t z=1; z<length; z++) {
// 		for (size_t y=0; y<=height; y++) {
// 			{
// 				const size_t start = 1 + z * dilated_width * dilated_height + y * dilated_width;
// 				const int pixelA = dilated_data[start];
// 				const int pixelB = dilated_data[start + dilated_width * dilated_height];
// 				const int pixelC = dilated_data[start + dilated_width];
// 				const int pixelD = dilated_data[start + dilated_width * dilated_height + dilated_width];
// 				voxel_verties+= pixelA | pixelB | pixelC | pixelD;
// 			}
// 			{
// 				const size_t start = dilated_width - 2 + z * dilated_width * dilated_height + y * dilated_width;
// 				const int pixelA = dilated_data[start];
// 				const int pixelB = dilated_data[start + dilated_width * dilated_height];
// 				const int pixelC = dilated_data[start + dilated_width];
// 				const int pixelD = dilated_data[start + dilated_width * dilated_height + dilated_width];
// 				if (z == length || z == 0) continue;
// 				voxel_verties+= pixelA | pixelB | pixelC | pixelD;
// 			}
// 		}
// 	}
// 
// 	for(size_t z=1; z<length; z++) {
// 		for (size_t x=1; x<width; x++) {
// 			{
// 				const size_t start = dilated_width + z * dilated_width * dilated_height + x;
// 				const int pixelA = dilated_data[start];
// 				const int pixelB = dilated_data[start + 1];
// 				const int pixelC = dilated_data[start + dilated_width * dilated_height];
// 				const int pixelD = dilated_data[start + dilated_width * dilated_height + 1];
// 				if (z == length || z == 0 || x == width || x == 0) continue;
// 				voxel_verties+= pixelA | pixelB | pixelC | pixelD;
// 			}
// 			{
// 				const size_t start = dilated_width * (dilated_height - 2) + z * dilated_width * dilated_height + x;
// 				const int pixelA = dilated_data[start];
// 				const int pixelB = dilated_data[start + 1];
// 				const int pixelC = dilated_data[start + dilated_width * dilated_height];
// 				const int pixelD = dilated_data[start + dilated_width * dilated_height + 1];
// 				if (z == length || z == 0 || x == width || x == 0) continue;
// 				voxel_verties+= pixelA | pixelB | pixelC | pixelD;
// 			}
// 		}
// 	}

	return voxel_verties;
}

void ConnectivityCalculator::MultiEuler(const vector<BYTE>& data, const size_t width, const size_t height, const size_t length, double& euler, const int thread_number)
{
	boost::thread_group grp;

	euler = 0;

	size_t size = (length + 1) / thread_number;
	vector<double> euler_for_threads;
	euler_for_threads.resize(thread_number, 0.0f);

	for(size_t i=0; i<thread_number; i++) {
		const int start = static_cast<int>(i*size) - 1;
		const int end = i == (thread_number - 1) ? static_cast<int>(length) : static_cast<int>((i + 1) * size) - 1;
		grp.create_thread(boost::bind(boost::mem_fn(&ConnectivityCalculator::__Euler), this, data.data(), static_cast<int>(width), static_cast<int>(height), static_cast<int>(length), start, end, boost::ref(euler_for_threads[i])));
	}

	grp.join_all();
	for(size_t i=0; i<thread_number; i++) {
		euler += euler_for_threads[i];
	}

}

void ConnectivityCalculator::__Euler(const BYTE* data, const int width, const int height, const int length, const int z_start, const int z_end, double& euler)
{
	euler = 0.0f;

	for (int k=z_start; k<z_end; k++) {
		theAppIVConfig.m_pILog->ProgressStepIt();
		for(int j=-1; j<height; j++) {
			for(int i=-1; i<width; i++) {
				const bool o1 = ValidPosition(data, width, height, length, i, j, k); 
				const bool o2 = ValidPosition(data, width, height, length, i, j + 1, k);
				const bool o3 = ValidPosition(data, width, height, length, i + 1, j, k);
				const bool o4 = ValidPosition(data, width, height, length, i + 1, j + 1, k);
				const bool o5 = ValidPosition(data, width, height, length, i, j, k + 1);
				const bool o6 = ValidPosition(data, width, height, length, i, j + 1, k + 1);
				const bool o7 = ValidPosition(data, width, height, length, i + 1, j, k + 1);
				const bool o8 = ValidPosition(data, width, height, length, i + 1, j + 1, k + 1);
				// 				 const size_t index = dilated_width * dilated_height * k + dilated_width * j + i;
				// 				 bool o1 = (dilated_data[index] != 0);
				// 				 bool o2 = (dilated_data[index + dilated_width] != 0);
				// 				 bool o3 = (dilated_data[index + 1] != 0);
				// 				 bool o4 = (dilated_data[index + dilated_width + 1] != 0);
				// 				 bool o5 = (dilated_data[index + dilated_width * dilated_height] != 0);
				// 				 bool o6 = (dilated_data[index + dilated_width * dilated_height + dilated_width] != 0);
				// 				 bool o7 = (dilated_data[index + dilated_width * dilated_height + 1] != 0);
				// 				 bool o8 = (dilated_data[index + dilated_width * dilated_height + dilated_width + 1] != 0);
				if (o1 || o2 || o3 || o4 || o5 || o6 || o7 || o8) {
					euler += GetDeltaEuler(o1, o2, o3, o4, o5, o6, o7, o8);
				}
			}
		}
	}

	euler = euler / 8.0f;
}



