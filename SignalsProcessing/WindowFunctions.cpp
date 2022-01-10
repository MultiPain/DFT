#include "WindowFunctions.h"

// initialize static map !!!
FunctionsMap WindowFunctions::values = []
{
	FunctionsMap ret;
	return ret;
}();

WindowFunctions::FunctionsNames WindowFunctions::names = []
{
	FunctionsNames ret;
	ret.reserve(7);
	return ret;
}();

double WindowFunctions::Get(std::string name, int idx, int N)
{
	auto callback = values[name];
	if (callback)
	{
		return callback(idx, N);
	}
	return 0.0f;
}

void WindowFunctions::Add(std::string name, WindowFunc func)
{
	values.insert(std::make_pair(name, func));
	names.push_back(name);
}

WindowFunctions::FunctionsNames& WindowFunctions::GetNames()
{
	return names;
}
