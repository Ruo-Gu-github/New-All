#include "stdafx.h"
#include "ruogu_AnisotropyCalculator.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include <algorithm>
#include <utility>
#include <limits>
#include <numeric>

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>

// VS2010兼容：包装参数结构体，解决boost::bind参数过多问题
struct DirectionRangeParams {
	const vector<BYTE>* binary_data;
	size_t width;
	size_t height;
	size_t length;
	NEW_POINT3D center;
	NEW_POINT3D min_corner;
	NEW_POINT3D max_corner;
	double step_length;
};


AnisotropyCalculator::AnisotropyCalculator()
{
}

AnisotropyCalculator::~AnisotropyCalculator()
{
}

string AnisotropyCalculator::Calculation(shared_ptr<ImageStack> images, map<string, double>& results)
{
	string error;

	if (!images) {
		error = "image stack is null";
		return error;
	}

	EnsureBinaryData(images);

	const size_t width = images->width_;
	const size_t height = images->height_;
	const size_t length = images->length_;
	const int num_of_directions = 2000;
	const double step_length = 0.5; // voxel units
	const vector<BYTE>& binary_data = images->binary_data_;
	const vector<short>& data = images->data_;
	if (binary_data.empty() || data.empty()) {
		error = "image data is empty";
		return error;
	}

	const NEW_POINT3D center = GetGeometricCenter(data, width, height, length, images->empty_value_);
	const NEW_POINT3D min_corner(0.0, 0.0, 0.0);
	const NEW_POINT3D max_corner(static_cast<double>(width - 1), static_cast<double>(height - 1), static_cast<double>(length - 1));

	vector<NEW_POINT3D> point_cloud;
	point_cloud.reserve(num_of_directions);

#ifdef USE_MULTIPLY_THREAD
	const int thread_number = std::max(1, images->thread_number_);
	const int actual_thread = std::min(num_of_directions, thread_number);
	vector<vector<NEW_POINT3D> > local_points(actual_thread);
	boost::thread_group grp;
	
	// VS2010兼容：创建参数包
	DirectionRangeParams params;
	params.binary_data = &binary_data;
	params.width = width;
	params.height = height;
	params.length = length;
	params.center = center;
	params.min_corner = min_corner;
	params.max_corner = max_corner;
	params.step_length = step_length;

	for (int t = 0; t < actual_thread; ++t) {
		const int start = static_cast<int>((static_cast<long long>(t) * num_of_directions) / actual_thread);
		const int end = static_cast<int>((static_cast<long long>(t + 1) * num_of_directions) / actual_thread);
		grp.create_thread(
			boost::bind(
				&AnisotropyCalculator::ProcessDirectionRangeWrapper,
				this,
				start,
				end,
				boost::cref(params),
				boost::ref(local_points[t])
			));
	}
	grp.join_all();
	for (size_t idx = 0; idx < local_points.size(); ++idx) {
		point_cloud.insert(point_cloud.end(), local_points[idx].begin(), local_points[idx].end());
	}
#else
	ProcessDirectionRange(0, num_of_directions, binary_data, width, height, length, center, min_corner, max_corner, step_length, point_cloud);
#endif

	if (point_cloud.size() < 3) {
		error = "insufficient samples for anisotropy calculation";
		return error;
	}

	double ev1 = 0.0;
	double ev2 = 0.0;
	double ev3 = 0.0;
	const double da = CalculateDAFromPointCloud(point_cloud, ev1, ev2, ev3);
	if (da <= 0.0) {
		error = "failed to compute anisotropy";
		return error;
	}

	results.insert(std::make_pair(string("DA"), da));
	results.insert(std::make_pair(string("DA.Eigenvalue1"), ev1));
	results.insert(std::make_pair(string("DA.Eigenvalue2"), ev2));
	results.insert(std::make_pair(string("DA.Eigenvalue3"), ev3));

	return error;
}


void AnisotropyCalculator::EnsureBinaryData(shared_ptr<ImageStack> images) const
{
	if (!images) return;
	vector<BYTE>& binary_data = images->binary_data_;
	vector<BYTE>& trinary_data = images->trinary_data_;
	const vector<short>& data = images->data_;
	if (binary_data.size() == data.size() && trinary_data.size() == data.size()) return;

	binary_data.clear();
	trinary_data.clear();
	binary_data.reserve(data.size());
	trinary_data.reserve(data.size());

	const int low_value = images->low_value_;
	const int high_value = images->high_value_;
	const int empty_value = images->empty_value_;

	for (size_t i = 0; i < data.size(); ++i) {
		short pixel = data[i];
		if (pixel == empty_value) {
			binary_data.push_back(0);
			trinary_data.push_back(0);
		}
		else if (pixel >= low_value && pixel <= high_value) {
			binary_data.push_back(1);
			trinary_data.push_back(2);
		}
		else {
			binary_data.push_back(0);
			trinary_data.push_back(1);
		}
	}
}

NEW_POINT3D AnisotropyCalculator::GetGeometricCenter(const vector<short>& data, size_t width, size_t height, size_t length, int empty_value) const
{
	if (data.empty()) {
		return NEW_POINT3D(0.0, 0.0, 0.0);
	}

	const size_t slice = width * height;
	long long sum_x = 0;
	long long sum_y = 0;
	long long sum_z = 0;
	long long count = 0;
	for (size_t z = 0; z < length; ++z) {
		for (size_t y = 0; y < height; ++y) {
			for (size_t x = 0; x < width; ++x) {
				size_t idx = z * slice + y * width + x;
				if (data[idx] != empty_value) {
					sum_x += static_cast<long long>(x);
					sum_y += static_cast<long long>(y);
					sum_z += static_cast<long long>(z);
					++count;
				}
			}
		}
	}
	if (count == 0) {
		return NEW_POINT3D(static_cast<double>(width) * 0.5, static_cast<double>(height) * 0.5, static_cast<double>(length) * 0.5);
	}
	return NEW_POINT3D(
		static_cast<double>(sum_x) / static_cast<double>(count),
		static_cast<double>(sum_y) / static_cast<double>(count),
		static_cast<double>(sum_z) / static_cast<double>(count));
}

RotateMatrix AnisotropyCalculator::GetRandomRotateMatrix(std::mt19937& generator) const
{
	// algorithm from http://planning.cs.uiuc.edu/node198.html
	std::uniform_real_distribution<double> distribution(0.0, 1.0);
	double u1 = distribution(generator);
	double u2 = distribution(generator);
	double u3 = distribution(generator);

	double a = sqrt(1.0f - u1) * sin(2.0f * M_PI * u2);
	double b = sqrt(1.0f - u1) * cos(2.0f * M_PI * u2);
	double c = sqrt(u1) * sin(2.0f * M_PI * u3);
	double d = sqrt(u1) * cos(2.0f * M_PI * u3);
		
	// algorithm from https://krasjet.github.io/quaternion/quaternion.pdf
	return RotateMatrix(
		1 - 2 * c * c - 2 * d * d,
		2 * b * c - 2 * a * d,
		2 * a * c + 2 * b * d,
		2 * b * c + 2 * a * d,
		1 - 2 * b * b - 2 * d * d,
		2 * c * d - 2 * a * b,
		2 * b * d - 2 * a * c,
		2 * a * b + 2 * c * d,
		1 - 2 * b * b - 2 * c * c
	);
}

NEW_POINT3D AnisotropyCalculator::GetRandomDirection(const RotateMatrix rotate_matrix) const
{
	const static NEW_POINT3D original_vector( 0.0f, 0.0f, 1.0f );
	NEW_POINT3D direction(
		rotate_matrix.m00 * original_vector.x + rotate_matrix.m01 * original_vector.y + rotate_matrix.m02 * original_vector.z,
		rotate_matrix.m10 * original_vector.x + rotate_matrix.m11 * original_vector.y + rotate_matrix.m12 * original_vector.z,
		rotate_matrix.m20 * original_vector.x + rotate_matrix.m21 * original_vector.y + rotate_matrix.m22 * original_vector.z
	);
	const double norm = sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
	if (norm <= std::numeric_limits<double>::epsilon()) {
		return NEW_POINT3D(0.0, 0.0, 1.0);
	}
	return NEW_POINT3D(direction.x / norm, direction.y / norm, direction.z / norm);
}

bool AnisotropyCalculator::InsertBox(const NEW_POINT3D& origin, const NEW_POINT3D& direction, const NEW_POINT3D& min_corner, const NEW_POINT3D& max_corner, double& entry_step, double& exit_step) const
{
	const double EPSILON = 1e-9;
	double t_min = -std::numeric_limits<double>::infinity();
	double t_max = std::numeric_limits<double>::infinity();

	const double dir_components[3] = { direction.x, direction.y, direction.z };
	const double origin_components[3] = { origin.x, origin.y, origin.z };
	const double min_components[3] = { min_corner.x, min_corner.y, min_corner.z };
	const double max_components[3] = { max_corner.x, max_corner.y, max_corner.z };

	for (int axis = 0; axis < 3; ++axis) {
		const double dir = dir_components[axis];
		const double orig = origin_components[axis];
		const double min_c = min_components[axis];
		const double max_c = max_components[axis];

		if (fabs(dir) < EPSILON) {
			if (orig < min_c || orig > max_c) {
				return false;
			}
			continue;
		}

		double t0 = (min_c - orig) / dir;
		double t1 = (max_c - orig) / dir;
		if (t0 > t1) std::swap(t0, t1);
		if (t0 > t_min) t_min = t0;
		if (t1 < t_max) t_max = t1;
		if (t_max < t_min) {
			return false;
		}
	}

	entry_step = t_min;
	exit_step = t_max;
	return t_max > t_min;
}

int AnisotropyCalculator::IntersectCounter(const NEW_POINT3D& entry_point, const NEW_POINT3D& direction, int step_num, double step_length, const vector<BYTE>& data, size_t width, size_t height, size_t length) const
{
	if (step_num <= 0) return 1;
	int intersect_num = 0;
	bool previous = IsBone(data, width, height, length, entry_point.x, entry_point.y, entry_point.z);
	for (int step_index = 1; step_index <= step_num; ++step_index) {
		const double distance = step_length * static_cast<double>(step_index);
		const double x = entry_point.x + direction.x * distance;
		const double y = entry_point.y + direction.y * distance;
		const double z = entry_point.z + direction.z * distance;
		const bool current = IsBone(data, width, height, length, x, y, z);
		if (current != previous) {
			++intersect_num;
			previous = current;
		}
	}
	return intersect_num > 0 ? intersect_num : 1;
}

bool AnisotropyCalculator::IsBone(const vector<BYTE>& data, size_t width, size_t height, size_t length, double x, double y, double z) const
{
	if (data.empty()) return false;
	if (x < 0.0 || y < 0.0 || z < 0.0) return false;
	if (x >= static_cast<double>(width) || y >= static_cast<double>(height) || z >= static_cast<double>(length)) return false;
	const size_t xi = static_cast<size_t>(std::max(0.0, std::min(static_cast<double>(width - 1), floor(x + 0.5))));
	const size_t yi = static_cast<size_t>(std::max(0.0, std::min(static_cast<double>(height - 1), floor(y + 0.5))));
	const size_t zi = static_cast<size_t>(std::max(0.0, std::min(static_cast<double>(length - 1), floor(z + 0.5))));
	const size_t index = zi * width * height + yi * width + xi;
	return data[index] != 0;
}

// VS2010兼容：包装函数，解决boost::bind参数过多问题
void AnisotropyCalculator::ProcessDirectionRangeWrapper(int start, int end, const DirectionRangeParams& params, vector<NEW_POINT3D>& points) const
{
	ProcessDirectionRange(start, end, *params.binary_data, params.width, params.height, params.length,
		params.center, params.min_corner, params.max_corner, params.step_length, points);
}

void AnisotropyCalculator::ProcessDirectionRange(int start, int end, const vector<BYTE>& binary_data, size_t width, size_t height, size_t length, const NEW_POINT3D& center, const NEW_POINT3D& min_corner, const NEW_POINT3D& max_corner, double step_length, vector<NEW_POINT3D>& points) const
{
	const double EPSILON = 1e-8;
	points.reserve(points.size() + static_cast<size_t>(end - start));
	for (int direction_index = start; direction_index < end; ++direction_index) {
		std::mt19937 generator(static_cast<unsigned int>(direction_index * 2654435761U + 1U));
		RotateMatrix rotate_matrix = GetRandomRotateMatrix(generator);
		NEW_POINT3D direction = GetRandomDirection(rotate_matrix);
		if (fabs(direction.x) < EPSILON && fabs(direction.y) < EPSILON && fabs(direction.z) < EPSILON) {
			continue;
		}

		double entry_step = 0.0;
		double exit_step = 0.0;
		if (!InsertBox(center, direction, min_corner, max_corner, entry_step, exit_step)) {
			continue;
		}

		double line_length = exit_step - entry_step;
		if (line_length <= EPSILON) {
			continue;
		}

		const NEW_POINT3D entry_point(
			center.x + direction.x * entry_step,
			center.y + direction.y * entry_step,
			center.z + direction.z * entry_step);

		int step_num = static_cast<int>(line_length / step_length);
		if (step_num <= 1) step_num = 1;

		int intersects = IntersectCounter(entry_point, direction, step_num, step_length, binary_data, width, height, length);
		if (intersects <= 0) {
			continue;
		}

		const double mil = line_length / static_cast<double>(intersects);
		if (mil <= EPSILON) {
			continue;
		}

		points.push_back(NEW_POINT3D(direction.x * mil, direction.y * mil, direction.z * mil));
	}
}

double AnisotropyCalculator::CalculateDAFromPointCloud(const vector<NEW_POINT3D>& point_cloud, double& eigen_value1, double& eigen_value2, double& eigen_value3) const
{
	if (point_cloud.size() < 3) {
		eigen_value1 = eigen_value2 = eigen_value3 = 0.0;
		return 0.0;
	}

	Vector3d mean = Vector3d::Zero();
	for (size_t i = 0; i < point_cloud.size(); ++i) {
		const NEW_POINT3D& point = point_cloud[i];
		mean.x() += point.x;
		mean.y() += point.y;
		mean.z() += point.z;
	}
	mean /= static_cast<double>(point_cloud.size());

	Matrix3d covariance = Matrix3d::Zero();
	for (size_t i = 0; i < point_cloud.size(); ++i) {
		const NEW_POINT3D& point = point_cloud[i];
		Vector3d diff(point.x - mean.x(), point.y - mean.y(), point.z - mean.z());
		covariance += diff * diff.transpose();
	}
	covariance /= static_cast<double>(point_cloud.size());

	SelfAdjointEigenSolver<Matrix3d> solver(covariance);
	if (solver.info() != Success) {
		eigen_value1 = eigen_value2 = eigen_value3 = 0.0;
		return 0.0;
	}

	Vector3d eigenvalues = solver.eigenvalues();
	const double EPSILON = 1e-10;
	const double lambda1 = std::max(eigenvalues(0), EPSILON);
	const double lambda2 = std::max(eigenvalues(1), EPSILON);
	const double lambda3 = std::max(eigenvalues(2), EPSILON);

	const double radius_min = sqrt(lambda1);
	const double radius_mid = sqrt(lambda2);
	const double radius_max = sqrt(lambda3);

	eigen_value1 = radius_min;
	eigen_value2 = radius_mid;
	eigen_value3 = radius_max;

	return radius_max > 0.0 ? radius_max / radius_min : 0.0;
}





