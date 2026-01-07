#pragma once
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <assert.h>
#include "boost/thread/thread.hpp"

#define THREAD_TIME_TEST

#ifdef THREAD_TIME_TEST
#include <time.h>
#endif // THREAD_TIME_TEST

#include "ruogu_ImageStack.h"

#define USE_MULTIPLY_THREAD

using std::string;
using std::vector;
using std::shared_ptr;
using std::map;
using std::pair;

class Calculator
{
public:
	Calculator();
protected:
	virtual ~Calculator();

public:
	virtual string Calculation(shared_ptr<ImageStack> images, map<string, double>& results) = 0;
};

