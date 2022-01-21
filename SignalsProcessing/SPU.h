#pragma once

#include "Core/Sandbox.h"
#include "WindowFunctions.h"
#include "Signal.h"

namespace SP_LAB
{
	class SPU : public Sandbox
	{
	public:
		~SPU();

		void Load();
		void Draw();

		CVector DFT(Vector input, int N = 0);
		Vector IDFT(CVector input, int N = 0);

		CVector FFT(Vector input, int N = 0);
		CVector FFT(CVector input, int N = 0);
		Vector IFFT(CVector input, int N = 0);

	private:
		using Sandbox::Sandbox;

		bool header_loaded = false;
		bool window_drag = false;
		bool show_imgui_demo = false;
		bool show_implot_demo = false;
		bool apply_window = false;
		bool calc_summ = false;
		bool plot_dft_stems = true;
		bool draw_half_fft = true;
		bool update_graph = true;
		bool update_dft = true;
		bool update_idft = false;

		bool use_fft = false;

		std::string selected_window_function;

		std::vector<Signal*> signals;
		Vector frequencies;
		Vector time_domain;
		Signal summ;

		float duration = 1.0f; // Период времени (в секундах)
		float fs = 1024.0f;
		float ts = 0.0f;

		void Hint(const char* text);

		int PrepareData(CVector& input);
		void ReloadHeader();
		void UpdateData();
		void UpdateDFT();
		void UpdateIDFT();
		inline std::string GetFunctionName(int idx);
		std::string GenerateCode();
		inline bool CheckFunctions();
		float CalculateFunctionValue(std::string name, float time_in_space);
		void Remove(int idx);

		void DrawInputs();
		void DrawPlots();
		void ConvertComplexVector(const CVector input, Vector& output);
	};
}