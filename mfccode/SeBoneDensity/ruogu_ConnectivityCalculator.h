#pragma once
#include "ruogu_Calculator.h"

#include <string>
#include <vector>
#include <map>

using std::string;
using std::vector;
using std::map;

class ConnectivityCalculator :
	public Calculator
{
public:
	ConnectivityCalculator();

	~ConnectivityCalculator();

public:
	virtual string Calculation(shared_ptr<ImageStack> images, map<string, double>& results);

private:
	void Euler(const vector<BYTE>& data, const int width, const int height, const int length, double& euler);
	int GetDeltaEuler(bool o1, bool o2, bool o3, bool o4, bool o5, bool o6, bool o7, bool  o8);

	void EdgeCorrection(const vector<BYTE>& data, const size_t width, const size_t height, const size_t length, double& edge_correction);
	long long StackCorners(const vector<BYTE>& data, const size_t width, const size_t height, const size_t length);
	long long StackEdges(const vector<BYTE>& data, const size_t width, const size_t height, const size_t length);
	long long StackFaces(const vector<BYTE>& data, const size_t width, const size_t height, const size_t length);
	long long VoxelEdgeIntersections(const vector<BYTE>& data, const size_t width, const size_t height, const size_t length);
	long long VoxelEdgeFaceIntersections(const vector<BYTE>& data, const int width, const int height, const int length);
	long long VoxelFaceIntersections(const vector<BYTE>& data, const int width, const int height, const int length);

	// edge correction only scan the surface data, it glowing really slow. so no need to use multiply thread.

 	void MultiEuler(const vector<BYTE>& data, const size_t width, const size_t height, const size_t length, double& euler, const int thread_number);
 	// void MultiEdgeCorrection(const vector<BYTE>& data, const size_t width, const size_t height, const size_t length, double& edge_correction, const int thread_number);

	void __Euler(const BYTE* data, const int width, const int height, const int length, const int z_start, const int z_end, double& euler);
	// void __EdgeCorrection(const BYTE* data, const size_t width, const size_t height, const size_t length, const int z_start, const int z_end, double& edge_correction);

	inline BYTE GetValueFromPosition(const BYTE* data, const int width, const int height, const int length, const int x, const int y, const int z) {
		if (x < 0 || x >= width || y < 0 || y >= height || z < 0 || z >= length) return 0;
		else return data[static_cast<size_t>(width) * static_cast<size_t>(height) * z + static_cast<size_t>(width) * y + x];
	}

	inline bool ValidPosition(const BYTE* data, const int width, const int height, const int length, const int x, const int y, const int z) {
		if (x < 0 || x >= width || y < 0 || y >= height || z < 0 || z >= length) return false;
		else return data[static_cast<size_t>(width) * static_cast<size_t>(height) * z + static_cast<size_t>(width) * y + x] == 1;
	}
};

