#pragma once

#include "ruogu_Calculator.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

using std::map;
using std::shared_ptr;
using std::string;
using std::vector;

class TBPfCalculator : public Calculator
{
public:
	TBPfCalculator();
	virtual ~TBPfCalculator();

public:
	virtual string Calculation(shared_ptr<ImageStack> images, map<string, double>& results);

private:
	void BuildTriangles(shared_ptr<ImageStack> images, vector<TRIANGLE>& triangles) const;

	NEW_POINT3D VertexInterapter(int low_value, const NEW_POINT3D p1, const NEW_POINT3D p2, double val1, double val2,
		double pixel_size, double pixel_spacing) const;

	void Polygonise(const GRIDCELL& grid, vector<TRIANGLE>& triangles, int low_value, double pixel_size, double pixel_spacing) const;
};

