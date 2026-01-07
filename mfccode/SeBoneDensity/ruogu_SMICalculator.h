#pragma once
#include "ruogu_Calculator.h"

#include <string>
#include <vector>
#include <memory>
#include <map>


using std::string;
using std::vector;
using std::shared_ptr;
using std::map;

class SMICalculator :
	public Calculator
{
public:
	SMICalculator();

	virtual ~SMICalculator();

public:
	virtual string Calculation(shared_ptr<ImageStack> data, map<string, double>& results);

private:
	void BuildTriangles(shared_ptr<ImageStack> images, vector<TRIANGLE>& triangles) const;

	NEW_POINT3D VertexInterapter(int low_value, const NEW_POINT3D p1, const NEW_POINT3D p2, double val1, double val2,
		double pixel_size, double pixel_spacing) const;

	void Polygonise(const GRIDCELL& grid, vector<TRIANGLE>& triangles, int low_value, double pixel_size, double pixel_spacing) const;
};


