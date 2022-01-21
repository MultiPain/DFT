#include "Core/Application.h"
#include "SPU.h"

#define eps 1e-6 
#define PI 3.14159265354

typedef std::complex<double> complex_t;
typedef std::vector<complex_t> cvector;
typedef std::vector<double> vector;

complex_t W(int k, int n, int N) 
{
	return complex_t(cos(2 * PI * k * n / N), -sin(2 * PI * k * n / N));
}

//Format zero 
complex_t format(complex_t& c) 
{
	if (fabs(c.real()) < eps)
		c._Val[0] = 0;
	if (fabs(c.imag()) < eps) 
		c._Val[1] = 0;
	return c;
}

double format(double& c) 
{
	if (fabs(c) < eps) 
		c = 0;
	return c;
}

void DFT(vector& x_n, cvector& X_k) {
	X_k.clear();
	int N = x_n.size();
	for (int k = 0; k < N; ++k) {
		complex_t t(0, 0);
		for (int n = 0; n < N; ++n) {
			t += x_n[n] * W(k, n, N);
		}
		X_k.push_back(format(t));
	}

	int cnt = 0;
	for (int i = 0; i < N; ++i) {
		if (cnt == (int)sqrt(N)) {
			std::cout << std::endl;
			cnt = 0;
		}
		++cnt;
		std::cout << format(X_k[i]) << " ";
	}
	std::cout << std::endl;

}

//IDFT calculation, only for real number sequences  
void IDFT(cvector& X_k, vector& x_n) {
	x_n.clear();
	int N = X_k.size();
	for (int n = 0; n < N; ++n) {
		complex_t t(0, 0);
		for (int k = 0; k < N; ++k) {
			t += X_k[k] * W(k, -n, N);
		}
		x_n.push_back(t.real() / N);//The result of the operation is only the real part 
		//cout<<(t/(double)N)<<endl; 
	}
	int cnt = 0;
	for (int i = 0; i < N; ++i) {
		if (cnt == (int)sqrt(N)) {
			std::cout << std::endl;
			cnt = 0;
		}
		++cnt;
		std::cout << format(x_n[i]) << " ";
	}
	std::cout << std::endl;

}

int main(int argc, char const* argv[]) 
{
	//int N = 16;
	//vector x_n(N);
	//cvector X_k(N);
	//for (int i = 0; i < N; ++i) 
	//{
	//	x_n[i] = i;
	//}
	//DFT(x_n, X_k);
	//IDFT(X_k, x_n);
	//
	//int a = 0;

	auto sandbox = std::make_unique<SP_LAB::SPU>();
	auto app = SP_LAB::Application::Get(sandbox.get());
	{
		app->Setup(u8"Обработка сигналов v0.3", 1280, 720);
		app->Run();
	}

	delete app;
}