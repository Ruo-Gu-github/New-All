#pragma once
#include <string>
#include <vector>
#include <map>

using std::string;
using std::vector;
using std::map;

__interface IInterface
{
public:
	virtual std::string Caculate(const vector<short> data, map<string, double>& results) = 0;
};
