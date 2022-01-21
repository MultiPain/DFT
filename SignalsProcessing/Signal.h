#pragma once

#include <valarray>
#include <vector>
#include <string>
#include <complex>

namespace SP_LAB
{
	typedef std::complex<float> CNumber;
	typedef std::vector<float> Vector;
	typedef std::vector<CNumber> CVector;

	class Signal
	{
	private:
		Vector values;
		CVector dft;
		CVector idft;

		Vector real_dft;
		Vector real_idft;

		std::string lua_function;

	public:
		bool Enabled = true;

		Signal();
		Vector& GetValues();
		CVector& GetDFTValues();
		CVector& GetIDFTValues();
		std::string& GetFunction();
		Vector& GetRealDFTValues();
		Vector& GetRealIDFTValues();
	};
}