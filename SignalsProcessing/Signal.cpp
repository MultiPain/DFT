#include "Signal.h"

namespace SP_LAB
{
	Signal::Signal()
	{
		lua_function = "sin(0 + pi2 * t * 100)";

		values.reserve(1000);
		dft.reserve(1000);
		idft.reserve(1000);
		real_dft.reserve(1000);
		real_idft.reserve(1000);
	}

	Vector& Signal::GetValues()
	{
		return values;
	}

	CVector& Signal::GetDFTValues()
	{
		return dft;
	}

	CVector& Signal::GetIDFTValues()
	{
		return idft;
	}

	Vector& Signal::GetRealDFTValues()
	{
		return real_dft;
	}

	Vector& Signal::GetRealIDFTValues()
	{
		return real_idft;
	}

	std::string& Signal::GetFunction()
	{
		return lua_function;
	}
}