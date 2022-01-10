#pragma once

#include "Application.h"
#include "WindowFunctions.h"
#include "Signal.h"

class SPU : public Application
{
private:
	using Application::Application;

	bool header_loaded		= false;
	bool window_drag		= false;
	bool show_imgui_demo	= false;
	bool show_implot_demo	= false;
	bool apply_window		= false;
	bool calc_summ			= false;
	bool update_graph		= true;
	bool update_dft			= true;
	bool plot_dft_stems		= true;
	bool draw_half_fft		= true;

	std::string selected_window_function;

	std::vector<Signal*> signals;
	Vector frequencies;
	Vector time_domain;
	Vector values_summ;
	Vector dft_summ;

	float duration = 5.0f; // Период времени (в секундах)
	float fs = 1000.0f; 
	float ts = 0.0f;

	void ReloadHeader();
	void UpdateData();
	void UpdateDFT();
	inline std::string GetFunctionName(int idx);
	std::string GenerateCode();
	inline bool CheckFunctions();
	float CalculateFunctionValue(std::string name, float time_in_space);
	void Remove(int idx);

	void DrawInputs();
	void DrawPlots();
	void ConvertComplexVector(const CVector input, Vector& output);
public:
	void Load();
	void Draw();

	CVector DFT(Vector input, int N = 0);
	CVector FFT(Vector input, int N = 0);
};