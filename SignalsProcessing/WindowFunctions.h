#pragma once

#ifndef PI2
#define PI2 6.28318530717958647692
#endif // PI2

#include <map>
#include <string>
#include <vector>
#include <functional>

namespace SP_LAB
{
	typedef std::function<float(int, int)> WindowFunc;
	typedef std::map<std::string, WindowFunc> FunctionsMap;
	typedef std::vector<std::string> FunctionsNames;

	class WindowFunctions
	{
	public:
		static void Add(std::string name, WindowFunc func);
		static double Get(std::string name, int idx, int N);
		static FunctionsNames& GetNames();
	private:
		WindowFunctions() = default;
		static FunctionsMap values;
		static FunctionsNames names;
	};
}