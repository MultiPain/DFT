#pragma once

#include <valarray>
#include <vector>
#include <string>
#include <complex>

typedef std::complex<float> CNumber;
typedef std::vector<float> Vector;
typedef std::vector<CNumber> CVector;

class Signal
{
private:
	Vector values;
	Vector dft;
	Vector idft;

	std::string lua_function;

public:
	bool Enabled = true;

	Signal();
	Vector& GetValues();
	Vector& GetDFTValues();
	Vector& GetIDFTValues();
	std::string& GetFunction();
};

