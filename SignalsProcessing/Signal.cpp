#include "Signal.h"

Signal::Signal()
{
	lua_function = "sin(0 + pi2 * t * 100)";

	values.reserve(1000);
	dft.reserve(1000);
	idft.reserve(1000);
}

Vector& Signal::GetValues()
{
	return values;
}

Vector& Signal::GetDFTValues()
{
	return dft;
}

Vector& Signal::GetIDFTValues()
{
	return idft;
}

std::string& Signal::GetFunction()
{
	return lua_function;
}
