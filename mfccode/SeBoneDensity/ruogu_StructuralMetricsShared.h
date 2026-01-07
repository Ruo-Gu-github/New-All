#pragma once

#include "ruogu_ImageStack.h"

#include "Eigen/Dense"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <numeric>
#include <utility>
#include <vector>

namespace structural_metrics {

using Eigen::Matrix3d;
using Eigen::SelfAdjointEigenSolver;
using Eigen::Vector3d;
using std::shared_ptr;
using std::size_t;
using std::vector;

struct SurfaceTriangle {
	Vector3d v0;
	Vector3d v1;
	Vector3d v2;
	Vector3d normal;
	double area;
	Vector3d centroid;
	
	// 构造函数初始化成员（VS2010兼容�?
	SurfaceTriangle() : area(0.0) {}
};

struct EllipsoidStats {
	double mean;
	double stddev;
	double median;
	double rod_fraction;
	double plate_fraction;
	double min_value;
	double max_value;
	double total_area;
	double rod_component;
	double plate_component;
	
	// 构造函数初始化成员（VS2010兼容�?
	EllipsoidStats() 
		: mean(0.0), stddev(0.0), median(0.0), rod_fraction(0.0), plate_fraction(0.0),
		  min_value(0.0), max_value(0.0), total_area(0.0), rod_component(0.0), plate_component(0.0) {}
};

inline Vector3d ToVector(const NEW_POINT3D& p)
{
	return Vector3d(p.x, p.y, p.z);
}

inline NEW_POINT3D ToPoint(const Vector3d& v)
{
	return NEW_POINT3D(v.x(), v.y(), v.z());
}

inline void EnsureBinaryData(const shared_ptr<ImageStack>& images)
{
	if (!images) {
		return;
	}
	vector<BYTE>& binary_data = images->binary_data_;
	vector<BYTE>& trinary_data = images->trinary_data_;
	const vector<short>& data = images->data_;
	if (binary_data.size() == data.size() && trinary_data.size() == data.size()) {
		return;
	}

	binary_data.clear();
	trinary_data.clear();
	binary_data.reserve(data.size());
	trinary_data.reserve(data.size());

	const int low_value = images->low_value_;
	const int high_value = images->high_value_;
	const int empty_value = images->empty_value_;

	for (size_t i = 0; i < data.size(); ++i) {
		const short pixel = data[i];
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

inline long EnsureBoneVoxelCount(const shared_ptr<ImageStack>& images)
{
	if (!images) {
		return 0;
	}
	if (images->bone_pixel_num_ > 0) {
		return images->bone_pixel_num_;
	}

	EnsureBinaryData(images);
	const vector<BYTE>& binary_data = images->binary_data_;
	long bone_count = 0;
	// VS2010兼容：使用传统for循环
	for (size_t i = 0; i < binary_data.size(); ++i) {
		if (binary_data[i] != 0) {
			++bone_count;
		}
	}
	images->bone_pixel_num_ = bone_count;
	return bone_count;
}

inline bool IsInsideVolume(const shared_ptr<ImageStack>& images, int x, int y, int z)
{
	return x >= 0 && y >= 0 && z >= 0 &&
		x < static_cast<int>(images->width_) &&
		y < static_cast<int>(images->height_) &&
		z < static_cast<int>(images->length_);
}

inline bool IsBoneAt(const shared_ptr<ImageStack>& images, const Vector3d& pos)
{
	if (!images) {
		return false;
	}
	EnsureBinaryData(images);
	const double pixel_size = images->pixel_size_;
	const double pixel_spacing = images->pixel_spacing_;
	if (pixel_size <= 0.0 || pixel_spacing <= 0.0) {
		return false;
	}

	const int x = static_cast<int>(std::floor(pos.x() / pixel_size));
	const int y = static_cast<int>(std::floor(pos.y() / pixel_size));
	const int z = static_cast<int>(std::floor(pos.z() / pixel_spacing));
	if (!IsInsideVolume(images, x, y, z)) {
		return false;
	}

	const size_t width = images->width_;
	const size_t height = images->height_;
	const size_t index = static_cast<size_t>(z) * width * height + static_cast<size_t>(y) * width + static_cast<size_t>(x);
	if (index >= images->binary_data_.size()) {
		return false;
	}
	return images->binary_data_[index] != 0;
}

inline SurfaceTriangle MakeSurfaceTriangle(const Vector3d& v0, const Vector3d& v1, const Vector3d& v2, const Vector3d& reference_normal)
{
	SurfaceTriangle tri;
	tri.v0 = v0;
	tri.v1 = v1;
	tri.v2 = v2;
	tri.centroid = (v0 + v1 + v2) / 3.0;

	Vector3d edge1 = v1 - v0;
	Vector3d edge2 = v2 - v0;
	Vector3d cross = edge1.cross(edge2);
	double area_twice = cross.norm();
	if (area_twice < 1e-9) {
		tri.area = 0.0;
		tri.normal = reference_normal;
		return tri;
	}

	Vector3d normal = cross / area_twice;
	if (reference_normal.dot(normal) < 0.0) {
		normal = -normal;
		edge1 = tri.v2 - tri.v0;
		edge2 = tri.v1 - tri.v0;
		cross = edge1.cross(edge2);
		area_twice = cross.norm();
		if (area_twice < 1e-9) {
			tri.area = 0.0;
			tri.normal = normal;
			return tri;
		}
	}

	tri.normal = normal;
	tri.area = 0.5 * area_twice;
	return tri;
}

inline SurfaceTriangle BuildSurfaceTriangle(const shared_ptr<ImageStack>& images, const TRIANGLE& tri)
{
	Vector3d v0 = ToVector(tri.p[0]);
	Vector3d v1 = ToVector(tri.p[1]);
	Vector3d v2 = ToVector(tri.p[2]);
	Vector3d centroid = (v0 + v1 + v2) / 3.0;
	Vector3d edge1 = v1 - v0;
	Vector3d edge2 = v2 - v0;
	Vector3d cross = edge1.cross(edge2);
	double area_twice = cross.norm();
	SurfaceTriangle surface;
	if (area_twice < 1e-9) {
		return surface;
	}

	Vector3d normal = cross / area_twice;
	const double probe_step = 0.25 * std::min(images->pixel_size_, images->pixel_spacing_);
	Vector3d probe_point = centroid + normal * probe_step;
	if (IsBoneAt(images, probe_point)) {
		normal = -normal;
	}

	Vector3d adjusted_v1 = v1;
	Vector3d adjusted_v2 = v2;
	Vector3d adjusted_edge1 = adjusted_v1 - v0;
	Vector3d adjusted_edge2 = adjusted_v2 - v0;
	Vector3d adjusted_cross = adjusted_edge1.cross(adjusted_edge2);
	if (adjusted_cross.dot(normal) < 0.0) {
		std::swap(adjusted_v1, adjusted_v2);
		adjusted_edge1 = adjusted_v1 - v0;
		adjusted_edge2 = adjusted_v2 - v0;
		adjusted_cross = adjusted_edge1.cross(adjusted_edge2);
	}

	surface.v0 = v0;
	surface.v1 = adjusted_v1;
	surface.v2 = adjusted_v2;
	surface.normal = normal;
	surface.centroid = (surface.v0 + surface.v1 + surface.v2) / 3.0;
	surface.area = 0.5 * adjusted_cross.norm();
	return surface;
}

inline void ConvertTrianglesToSurface(const shared_ptr<ImageStack>& images, const vector<TRIANGLE>& triangles, vector<SurfaceTriangle>& surface)
{
	surface.clear();
	surface.reserve(triangles.size());

	// VS2010兼容：使用传统for循环
	for (size_t i = 0; i < triangles.size(); ++i) {
		SurfaceTriangle surface_tri = BuildSurfaceTriangle(images, triangles[i]);
		if (surface_tri.area > 0.0) {
			surface.push_back(surface_tri);
		}
	}
}

inline double ComputeSurfaceArea(const vector<SurfaceTriangle>& triangles)
{
	double sum = 0.0;
	// VS2010兼容：使用传统for循环
	for (size_t i = 0; i < triangles.size(); ++i) {
		sum += triangles[i].area;
	}
	return sum;
}

inline double ComputeEnclosedVolume(const vector<SurfaceTriangle>& triangles)
{
	double volume = 0.0;
	// VS2010兼容：使用传统for循环
	for (size_t i = 0; i < triangles.size(); ++i) {
		const SurfaceTriangle& tri = triangles[i];
		const Vector3d& v0 = tri.v0;
		const Vector3d& v1 = tri.v1;
		const Vector3d& v2 = tri.v2;
		volume += v0.dot((v1 - v0).cross(v2 - v0));
	}
	return volume / 6.0;
}

inline double MeasureDirectionalExtent(const shared_ptr<ImageStack>& images, const Vector3d& origin, const Vector3d& direction, double step, double max_distance)
{
	double travelled = 0.0;
	Vector3d position = origin;
	while (travelled + step <= max_distance) {
		position += direction * step;
		if (!IsBoneAt(images, position)) {
			break;
		}
		travelled += step;
	}
	return travelled;
}

inline void CollectNeighborhood(const shared_ptr<ImageStack>& images, const Vector3d& centroid, double radius, vector<Vector3d>& neighbors)
{
	neighbors.clear();
	EnsureBinaryData(images);
	const vector<BYTE>& binary = images->binary_data_;
	if (binary.empty()) {
		return;
	}

	const size_t width = images->width_;
	const size_t height = images->height_;
	const size_t length = images->length_;
	const double pixel_size = images->pixel_size_;
	const double pixel_spacing = images->pixel_spacing_;
	if (pixel_size <= 0.0 || pixel_spacing <= 0.0) {
		return;
	}

	const int cx = static_cast<int>(std::floor(centroid.x() / pixel_size));
	const int cy = static_cast<int>(std::floor(centroid.y() / pixel_size));
	const int cz = static_cast<int>(std::floor(centroid.z() / pixel_spacing));

	const int radius_xy = static_cast<int>(std::ceil(radius / pixel_size));
	const int radius_z = static_cast<int>(std::ceil(radius / pixel_spacing));
	const double radius_sq = radius * radius;

	for (int dz = -radius_z; dz <= radius_z; ++dz) {
		const int z = cz + dz;
		if (z < 0 || z >= static_cast<int>(length)) {
			continue;
		}
		for (int dy = -radius_xy; dy <= radius_xy; ++dy) {
			const int y = cy + dy;
			if (y < 0 || y >= static_cast<int>(height)) {
				continue;
			}
			for (int dx = -radius_xy; dx <= radius_xy; ++dx) {
				const int x = cx + dx;
				if (x < 0 || x >= static_cast<int>(width)) {
					continue;
				}
				const size_t index = static_cast<size_t>(z) * width * height + static_cast<size_t>(y) * width + static_cast<size_t>(x);
				if (index >= binary.size() || binary[index] == 0) {
					continue;
				}
				const double px = (static_cast<double>(x) + 0.5) * pixel_size;
				const double py = (static_cast<double>(y) + 0.5) * pixel_size;
				const double pz = (static_cast<double>(z) + 0.5) * pixel_spacing;
				Vector3d point(px, py, pz);
				if ((point - centroid).squaredNorm() <= radius_sq) {
					neighbors.push_back(point);
				}
			}
		}
	}
}

inline double ComputeEllipsoidFactor(const shared_ptr<ImageStack>& images, const SurfaceTriangle& tri, double radius)
{
	vector<Vector3d> neighbors;
	CollectNeighborhood(images, tri.centroid, radius, neighbors);
	if (neighbors.size() < 10) {
		return 0.0;
	}

	Vector3d mean = Vector3d::Zero();
	// VS2010兼容：使用传统for循环
	for (size_t i = 0; i < neighbors.size(); ++i) {
		mean += neighbors[i];
	}
	mean /= static_cast<double>(neighbors.size());

	Matrix3d covariance = Matrix3d::Zero();
	// VS2010兼容：使用传统for循环
	for (size_t i = 0; i < neighbors.size(); ++i) {
		Vector3d diff = neighbors[i] - mean;
		covariance += diff * diff.transpose();
	}
	covariance /= static_cast<double>(neighbors.size());

	SelfAdjointEigenSolver<Matrix3d> solver(covariance);
	if (solver.info() != Eigen::Success) {
		return 0.0;
	}

	Vector3d eigenvalues = solver.eigenvalues();
	const double epsilon = 1e-9;
	double lambda_min = std::max(eigenvalues(0), epsilon);
	double lambda_mid = std::max(eigenvalues(1), epsilon);
	double lambda_max = std::max(eigenvalues(2), epsilon);

	double a = std::sqrt(5.0 * lambda_max);
	double b = std::sqrt(5.0 * lambda_mid);
	double c = std::sqrt(5.0 * lambda_min);
	double denominator = std::max(a - c, epsilon);
	double epsilon_value = (a - b) / denominator;
	double ef = 2.0 * epsilon_value - 1.0;
	// VS2010兼容：手动实现clamp
	ef = std::max(-1.0, std::min(ef, 1.0));

	const double step = 0.25 * std::min(images->pixel_size_, images->pixel_spacing_);
	const double max_distance = radius;
	double forward = MeasureDirectionalExtent(images, tri.centroid, tri.normal, step, max_distance);
	double backward = MeasureDirectionalExtent(images, tri.centroid, -tri.normal, step, max_distance);
	double extent_sum = forward + backward;
	double sign = 1.0;
	if (extent_sum > step) {
		double asymmetry = std::abs(forward - backward) / extent_sum;
		sign = (asymmetry < 0.25) ? -1.0 : 1.0;
	}
	// VS2010兼容：手动实现clamp
	return std::max(-1.0, std::min(ef * sign, 1.0));
}

inline EllipsoidStats ComputeEllipsoidFactors(const shared_ptr<ImageStack>& images, const vector<SurfaceTriangle>& triangles, double radius)
{
	EllipsoidStats stats;
	if (triangles.empty()) {
		stats.min_value = 0.0;
		stats.max_value = 0.0;
		return stats;
	}

	stats.min_value = std::numeric_limits<double>::infinity();
	stats.max_value = -std::numeric_limits<double>::infinity();

	double weighted_sum = 0.0;
	double weighted_sq_sum = 0.0;
	double rod_area = 0.0;
	double plate_area = 0.0;
	vector<std::pair<double, double> > weighted_values;
	weighted_values.reserve(triangles.size());

	// VS2010兼容：使用传统for循环
	for (size_t i = 0; i < triangles.size(); ++i) {
		const SurfaceTriangle& tri = triangles[i];
		if (tri.area <= 0.0) {
			continue;
		}
		double ef = ComputeEllipsoidFactor(images, tri, radius);
		stats.total_area += tri.area;
		weighted_sum += ef * tri.area;
		weighted_sq_sum += ef * ef * tri.area;
		if (ef >= 0.0) {
			rod_area += ef * tri.area;
		}
		else {
			plate_area += -ef * tri.area;
		}
		stats.min_value = std::min(stats.min_value, ef);
		stats.max_value = std::max(stats.max_value, ef);
		weighted_values.push_back(std::make_pair(ef, tri.area));
	}

	if (stats.total_area <= 0.0 || weighted_values.empty()) {
		stats.min_value = stats.min_value == std::numeric_limits<double>::infinity() ? 0.0 : stats.min_value;
		stats.max_value = stats.max_value == -std::numeric_limits<double>::infinity() ? 0.0 : stats.max_value;
		return stats;
	}

	stats.mean = weighted_sum / stats.total_area;
	const double mean_sq = weighted_sq_sum / stats.total_area;
	stats.stddev = std::sqrt(std::max(0.0, mean_sq - stats.mean * stats.mean));
	// VS2010兼容：手动实现clamp
	stats.rod_fraction = std::max(0.0, std::min(rod_area / stats.total_area, 1.0));
	stats.plate_fraction = std::max(0.0, std::min(plate_area / stats.total_area, 1.0));
	stats.rod_component = 3.0 * stats.rod_fraction;
	stats.plate_component = -3.0 * stats.plate_fraction;

	// VS2010兼容：使用函数对象代替lambda
	struct PairCompare {
		bool operator()(const std::pair<double, double>& lhs, const std::pair<double, double>& rhs) const {
			return lhs.first < rhs.first;
		}
	};
	std::sort(weighted_values.begin(), weighted_values.end(), PairCompare());
	double half_weight = stats.total_area * 0.5;
	double cumulative = 0.0;
	// VS2010兼容：使用传统for循环
	for (size_t i = 0; i < weighted_values.size(); ++i) {
		const std::pair<double, double>& pair = weighted_values[i];
		cumulative += pair.second;
		if (cumulative >= half_weight) {
			stats.median = pair.first;
			break;
		}
	}

	if (stats.min_value == std::numeric_limits<double>::infinity()) {
		stats.min_value = 0.0;
	}
	if (stats.max_value == -std::numeric_limits<double>::infinity()) {
		stats.max_value = 0.0;
	}
	return stats;
}

inline vector<SurfaceTriangle> OffsetSurface(const vector<SurfaceTriangle>& triangles, double delta)
{
	vector<SurfaceTriangle> result;
	result.reserve(triangles.size());
	// VS2010兼容：使用传统for循环
	for (size_t i = 0; i < triangles.size(); ++i) {
		const SurfaceTriangle& tri = triangles[i];
		Vector3d shift = tri.normal * delta;
		SurfaceTriangle offset = MakeSurfaceTriangle(tri.v0 + shift, tri.v1 + shift, tri.v2 + shift, tri.normal);
		result.push_back(offset);
	}
	return result;
}

} // namespace structural_metrics
