#pragma once
#include "ruogu_Calculator.h"

#include "Eigen/Dense"
using namespace Eigen;

#include <string>
#include <vector>
#include <map>
#include <random>

using std::string;
using std::vector;
using std::map;

// VS2010兼容：前向声明参数结构体
struct DirectionRangeParams;

struct RotateMatrix
{
	double m00;
	double m01;
	double m02;
	double m10;
	double m11;
	double m12;
	double m20;
	double m21;
	double m22;
	RotateMatrix(double _m00, double _m01, double _m02, double _m10, double _m11, double _m12, double _m20, double _m21, double _m22) {
		m00 = _m00;
		m01 = _m01;
		m02 = _m02;
		m10 = _m10;
		m11 = _m11;
		m12 = _m12;
		m20 = _m20;
		m21 = _m21;
		m22 = _m22;
	}
};

class AnisotropyCalculator :
	public Calculator
{
public:
	AnisotropyCalculator();	
	virtual ~AnisotropyCalculator();

public:
	virtual string Calculation(shared_ptr<ImageStack> images, map<string, double>& results);

private:
	void EnsureBinaryData(shared_ptr<ImageStack> images) const;
	NEW_POINT3D GetGeometricCenter(const vector<short>& data, size_t width, size_t height, size_t length, int empty_value) const;
	RotateMatrix GetRandomRotateMatrix(std::mt19937& generator) const;
	NEW_POINT3D GetRandomDirection(const RotateMatrix rotate_matrix) const;
	bool InsertBox(const NEW_POINT3D& origin, const NEW_POINT3D& direction, const NEW_POINT3D& min_corner, const NEW_POINT3D& max_corner, double& entry_step, double& exit_step) const;
	int IntersectCounter(const NEW_POINT3D& entry_point, const NEW_POINT3D& direction, int step_num, double step_length, const vector<BYTE>& data, size_t width, size_t height, size_t length) const;
	bool IsBone(const vector<BYTE>& data, size_t width, size_t height, size_t length, double x, double y, double z) const;
	void ProcessDirectionRange(int start, int end, const vector<BYTE>& binary_data, size_t width, size_t height, size_t length, const NEW_POINT3D& center, const NEW_POINT3D& min_corner, const NEW_POINT3D& max_corner, double step_length, vector<NEW_POINT3D>& points) const;
	void ProcessDirectionRangeWrapper(int start, int end, const DirectionRangeParams& params, vector<NEW_POINT3D>& points) const;  // VS2010兼容包装函数
	double CalculateDAFromPointCloud(const vector<NEW_POINT3D>& point_cloud, double& eigen_value1, double& eigen_value2, double& eigen_value3) const;
};

