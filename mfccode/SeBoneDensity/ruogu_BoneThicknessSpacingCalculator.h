#pragma once
#include "ruogu_BoneThicknessCalculator.h"

class BoneThicknessSpacingCalculator :
	public BoneThicknessCalculator {
public:
	BoneThicknessSpacingCalculator();
	~BoneThicknessSpacingCalculator();

public:
	virtual string Calculation(shared_ptr<ImageStack> images, map<string, double>& results);
};