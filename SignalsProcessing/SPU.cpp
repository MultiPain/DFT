#include "SPU.h"
#include "Core/Application.h"
#include <iostream>
#include <complex>

//#define SHOW_DEMO
//#define DEBUG

#define PI  3.141592653589793238460f
#define PI2 6.28318530717958647692f

const char* HELP_INFO = u8"sin(x) \n\
cos(x) \n\
tan(x) \n\
acos(x) \n\
asin(x) \n\
atan(x) \n\
atan2(y, x) \n\
saw(x) \n\
square(x) \n\
triangle(x) \n\
sign(x) - знак числа \n\
\n\
abs(x) \n\
sqrt(x) \n\
exp(x) \n\
log(x) \n\
pi = 3.14159265359 \n\
pi2 = pi * 2 \n\
pi_2 = pi / 2 \n\
pi_r = 1 / pi \n\
\n\
ceil(x) - округляет до следующего целого \n\
floor(x) - возвращает целую часть числа \n\
fmod(x, y) - остаток от деления \n\
max(...) - максимальное значение из списка \n\
min(...) - минимальное значение из списка \n\
random(min, max) - случайное число в диапазоне \n\
random(max) - случайное число от 0 до max";

#ifdef DEBUG

#define PrintVec(vec) \
	std::cout << #vec << ":" << std::endl; \
	for (auto v : vec) \
		std::cout << v << ", "; \
	std::cout << std::endl

#define PrintCVec(vec) \
	std::cout << #vec << ":" << std::endl; \
	for (auto v : vec) \
		std::cout << abs(v) << ", "; \
	std::cout << std::endl

#endif

namespace SP_LAB
{
	static lua_State* L;

	unsigned NextPowerOf2(unsigned n)
	{
		n = n - 1;
		while (n & n - 1)
		{
			n &= n - 1;
		}
		return n << 1;
	}

	inline void ResizeToNextPowOf2(Vector& input)
	{
		const int original = (int)input.size();
		auto target_size = NextPowerOf2(original);
		input.resize(target_size);
	}

	void LoadWindowFunctions()
	{
		WindowFunctions::Add("Rectangle", [](int idx, int N) -> float {
			return 1.0f;
			});
		WindowFunctions::Add("Triangle", [](int idx, int N) -> float {
			const float FN = (float)N;

			return 1.0f - fabs((idx - (FN / 2.0f)) / (FN / 2.0f));
			});
		WindowFunctions::Add("Blackman", [](int idx, int N) -> float {
			const float alpha = 0.16f;
			const float a0 = (1.0f - alpha) / 2.0f;
			const float a1 = 0.5f;
			const float a2 = alpha / 2.0f;

			return a0 - a1 * cosf((PI2 * idx) / (float)N) + a2 * cosf((2.0f * PI2 * idx) / (float)N);
			});
		WindowFunctions::Add("FlatTop", [](int idx, int N) -> float {
			const float a0 = 0.21557895f;
			const float a1 = 0.41663158f;
			const float a2 = 0.277263158f;
			const float a3 = 0.083578947f;
			const float a4 = 0.006947368f;
			const float FN = (float)N;

			return a0 - a1 * cosf((PI2 * idx) / FN) + a2 * cosf((2.0f * PI2 * idx) / FN) - a3 * cosf((3.0f * PI2 * idx) / FN) + a4 * cosf((4.0f * PI2 * idx) / FN);
			});
		WindowFunctions::Add("Tukey", [](int idx, int N) -> float {
			const float alpha = 0.5f;
			const float aN = (float)N * alpha;

			if (idx < aN / 2.0f)
				return 0.5f * (1.0f - cosf((PI2 * idx) / aN));
			else if (idx >= aN / 2.0f && idx < N / 2 + aN / 2.0f)
				return 1.0f;
			else
				return 0.5f * (1.0f - cosf((PI2 * idx) / aN));
			});
		WindowFunctions::Add("Hann", [](int idx, int N) -> float {
			const float FN = (float)N;
			const float a0 = 0.5f;
			const float a1 = 1.0f - a0;

			return a0 - a1 * cosf((PI2 * idx) / FN);
			});
		WindowFunctions::Add("Hamming", [](int idx, int N) -> float {
			const float FN = (float)N;
			const float a0 = 25.0f / 46.0f;
			const float a1 = 1.0f - a0;

			return a0 - a1 * cosf((PI2 * idx) / FN);
			});
	}

	SPU::~SPU()
	{
		lua_close(L);
	}

	void SPU::Load()
	{
		L = luaL_newstate();
		luaL_openlibs(L);

		LoadWindowFunctions();
		ReloadHeader();

#ifdef DEBUG
		Vector test;
		for (int i = 0; i < 16; i++)
			test.push_back(i);
		CVector fft_test = FFT(test);
		Vector rifft = IFFT(fft_test);

		PrintVec(test);
		PrintCVec(fft_test);
		PrintVec(rifft);
#endif
	}

	void SPU::ReloadHeader()
	{

		if (luaL_dofile(L, "Header.lua") == LUA_OK)
		{
			update_graph = true;
			update_dft = true;
			header_loaded = true;
			ImGui::InsertNotification({ ImGuiToastType_Success, 5, u8"Header.lua загружен" });
		}
		else
		{
			const char* error = lua_tostring(L, -1);
			ImGui::InsertNotification({ ImGuiToastType_Error, 5, error });
		}
	}


	////////////////////////////////////////////////////////////////////
	///LUA stuff
	////////////////////////////////////////////////////////////////////

	inline std::string SPU::GetFunctionName(int idx)
	{
		return "__f" + std::to_string(idx);
	}

	std::string SPU::GenerateCode()
	{
		std::string lua_code;

		for (int i = 0; i < signals.size(); i++)
		{
			Signal* signal = signals[i];

			if (signal->Enabled)
			{
				std::string fname = GetFunctionName(i);
				lua_code += "function " + fname + "(t) return " + signal->GetFunction() + " end\n" + fname + "(0)\n";
			}
		}

		return lua_code;
	}

	inline bool SPU::CheckFunctions()
	{
		if (!header_loaded)
		{
			ImGui::InsertNotification({ ImGuiToastType_Error, 3, "LUA header is not loaded!" });
			return false;
		}

		std::string lua_code = GenerateCode();
		return luaL_dostring(L, lua_code.c_str()) == LUA_OK;
	}

	// Считает значение пользовательской функции
	float SPU::CalculateFunctionValue(std::string name, float time_in_space)
	{
		lua_getglobal(L, name.c_str());
		if (lua_isfunction(L, -1))
		{
			lua_pushnumber(L, time_in_space);
			if (lua_pcall(L, 1, 1, 0) == LUA_OK && lua_isnumber(L, -1))
			{
				float value = (float)lua_tonumber(L, -1);
				lua_pop(L, 1);
				return value;
			}
		}
		else
		{
			ImGui::InsertNotification({ ImGuiToastType_Error, 3, "\"%s\" is not a valid function!", name });
			lua_pop(L, 1);
		}
		return 0.0f;
	}

	////////////////////////////////////////////////////////////////////
	/// Updating data
	////////////////////////////////////////////////////////////////////

	void SPU::UpdateData()
	{
		if (CheckFunctions())
		{
			ts = 1.0f / fs;
			const int size = duration * fs;
			time_domain.resize(size);

			auto& values_summ = summ.GetValues();
			auto& dft_summ = summ.GetDFTValues();

			if (calc_summ)
			{
				values_summ.resize(size);
			}

			for (int i = 0; i < size; i++)
			{
				float tick = i * ts;
				time_domain[i] = tick;

				float summ = 0.0f;
				for (int j = 0; j < signals.size(); j++)
				{
					Signal* signal = signals[j];
					Vector& values = signal->GetValues();
					if (values.size() != size)
						values.resize(size);

					if (signal->Enabled)
					{
						float value = CalculateFunctionValue(GetFunctionName(j), tick);

						if (apply_window)
						{
							value *= WindowFunctions::Get(selected_window_function, i, size);
						}

						values[i] = value;
						summ += value;
					}
				}
				if (calc_summ)
				{
					values_summ[i] = summ;
				}
			}
			update_dft = true;
		}
	}

	void SPU::Hint(const char* text)
	{
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();

			ImGui::TextUnformatted(text);

			ImGui::EndTooltip();
		}
	}

	int SPU::PrepareData(CVector& input)
	{
		if (use_fft)
		{
			const float RK = 2.0f;
			const int size = duration * fs;
			const unsigned int new_size = NextPowerOf2(size);
			const unsigned int half_size = new_size / 2;
			const float D = fs * RK / new_size; // scale frequency to match FFT

			if (calc_summ)
			{
				input.resize(half_size);
			}

			frequencies.resize(half_size);

			for (size_t i = 0; i < half_size; i++)
			{
				frequencies[i] = i * D;
			}


			return new_size;
		}
		else
		{
			const float RT = 1.0f / duration;
			const int size = duration * fs;
			frequencies.resize(size);

			for (size_t i = 0; i < frequencies.size(); i++)
			{
				frequencies[i] = i * RT;
			}

			return size;
		}
	}

	void SPU::UpdateDFT()
	{
		auto& values_summ = summ.GetValues();
		auto& dft_summ = summ.GetDFTValues();
		auto& real_dft_summ = summ.GetRealDFTValues();

		int new_size = PrepareData(dft_summ);

		if (calc_summ)
		{
			if (use_fft)
			{
				dft_summ = FFT(values_summ, new_size);
				ConvertComplexVector(dft_summ, real_dft_summ);
			}
			else
			{
				dft_summ = DFT(values_summ, new_size);
				ConvertComplexVector(dft_summ, real_dft_summ);
			}
		}
		else
		{
			for (auto signal : signals)
			{
				if (signal->Enabled)
				{
					Vector& values = signal->GetValues();
					CVector& out = signal->GetDFTValues();
					Vector& real_out = signal->GetRealDFTValues();

					if (use_fft)
					{
						ResizeToNextPowOf2(values);
						out = FFT(values, new_size);
						ConvertComplexVector(out, real_out);
					}
					else
					{
						out = DFT(values, new_size);
						ConvertComplexVector(out, real_out);
					}
				}
			}
		}
	
		UpdateIDFT();
	}

	void SPU::UpdateIDFT()
	{
		auto& dft_summ = summ.GetDFTValues();
		auto& real_dft_summ = summ.GetRealIDFTValues();

		int new_size = PrepareData(dft_summ);

		if (calc_summ)
		{
			if (use_fft)
			{
				real_dft_summ = IFFT(dft_summ, new_size);
			}
			else
			{
				real_dft_summ = IDFT(dft_summ, new_size);
			}
		}
		else
		{
			for (auto signal : signals)
			{
				if (signal->Enabled)
				{
					CVector& values = signal->GetDFTValues();
					Vector& out = signal->GetRealIDFTValues();
					if (use_fft)
					{
						//ResizeToNextPowOf2(values);
						out = IFFT(values, new_size);
						//CVector ifft = IFFT(values, new_size);
						//ConvertComplexVector(ifft, out);
					}
					else
					{
						out = IDFT(values, new_size);
						//CVector idft = IDFT(values, new_size);
						//ConvertComplexVector(idft, out);
					}
				}
			}
		}
	}

	void SPU::ConvertComplexVector(const CVector input, Vector& output)
	{
		output.resize(input.size());

		for (size_t i = 0; i < input.size(); i++)
		{
			output[i] = abs(input[i]); // abs - magnitude of complex number e.g. sqrt(re^2 + im^2)
		}
	}

	////////////////////////////////////////////////////////////////////

	void SPU::Remove(int idx)
	{
		Signal* signal = signals.at(idx);

		auto it = signals.begin();
		std::advance(it, idx);
		signals.erase(it);

		delete signal;
	}

	////////////////////////////////////////////////////////////////////
	/// Drawing stuff
	////////////////////////////////////////////////////////////////////

	void SPU::DrawInputs()
	{
		const int SIZE = 64;
		static int current_window = 0;
		static int previous_hovered_window = -1;
		static int current_hovered_window = current_window;
		static bool is_window_data_updated = false;
		static float xs[SIZE + 1], ys[SIZE + 1];
		auto names = WindowFunctions::GetNames();

		// Window functions
		if (ImGui::BeginCombo("##Functions", names[current_window].c_str()))
		{
			for (size_t i = 0; i < names.size(); i++)
			{
				const char* name = names[i].c_str();
				if (ImGui::Selectable(name, current_window == i))
				{
					selected_window_function = names[i];
					current_window = i;
					if (apply_window)
					{
						update_graph = true;
					}
				}

				if (ImGui::IsItemHovered())
				{
					if (previous_hovered_window != current_hovered_window)
					{
						previous_hovered_window = current_hovered_window;
						for (size_t i = 0; i <= SIZE; i++)
						{
							xs[i] = i;
							ys[i] = WindowFunctions::Get(name, i, SIZE);
						}
					}

					ImGui::SetNextWindowSize(ImVec2(256, 256));
					ImGui::BeginTooltip();

					if (ImPlot::BeginPlot("Function", ImVec2(-1.0f, -1.0f), ImPlotFlags_NoLegend | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoTitle))
					{
						const float bounds_v[]{ 0.0f, SIZE };
						const float bounds_h[]{ 1.0f };
						ImPlot::PlotHLines("##BoundsH", bounds_h, 1);
						ImPlot::PlotVLines("##BoundsV", bounds_v, 2);
						ImPlot::SetupAxes("", "", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
						ImPlot::SetNextFillStyle(ImVec4(1.0f, 1.0f, 1.0f, 0.4f), 1.0f);
						ImPlot::PlotShaded(name, xs, ys, SIZE + 1);
						ImPlot::EndPlot();
					}

					ImGui::EndTooltip();
					current_hovered_window = i;
				}

			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		update_graph |= ImGui::Checkbox(u8"Применть", &apply_window);

		// LUA functions
		if (ImGui::Button("+##AddSignal", ImVec2(-ImGui::GetFontSize() * 3.0f, 0.0)))
		{
			Signal* signal = new Signal();
			//signal.Randomize();
			signals.push_back(signal);
			update_graph = true;
		}

		ImGui::SameLine();
		ImGui::Text("(?)");
		Hint(HELP_INFO);

		int i = 0;
		size_t size = signals.size();
		while (i < size)
		{
			Signal* signal = signals[i];

			ImGui::PushID(i);
			update_graph |= ImGui::Checkbox("##Enabled", &signal->Enabled);

			ImGui::BeginDisabled(!signal->Enabled);
			ImGui::SameLine();
			update_graph |= ImGui::InputText("", &signal->GetFunction());
			ImGui::EndDisabled();

			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_TRASH))
			{
				update_graph = true;
				Remove(i);
				size -= 1;
				ImGui::PopID();
				continue;
			}
			ImGui::PopID();

			i++;
		}

		// other Options
		update_graph |= ImGui::Checkbox(u8"Суммировать все сигналы", &calc_summ);
		update_graph |= ImGui::SliderFloat(u8"Время", &duration, 0.1f, 10.0f);
		update_graph |= ImGui::SliderFloat(u8"Частота дискретизации", &fs, 10.0f, 10000.0f, NULL, ImGuiSliderFlags_AlwaysClamp);
	}

	void SPU::DrawPlots()
	{
		auto& values_summ = summ.GetValues();
		auto& dft_summ = summ.GetDFTValues();
		auto& real_dft_summ = summ.GetRealDFTValues();

		if (ImGui::Begin(u8"Сигналы"))
		{
			ImVec2 main_row_size(-1.0f, -1.0f);
			ImPlotFlags summ_plot_flags = ImPlotFlags_None;

			if (calc_summ)
			{

				if (ImPlot::BeginPlot(u8"Сумма сигналов", ImVec2(-1.0f, -1.0), summ_plot_flags))
				{
					ImPlot::SetupAxes(u8"Время, t", "f1(x) + f2(x) + ... + fn(x)");
					ImPlot::PlotLine(u8"Сумма", time_domain.data(), values_summ.data(), time_domain.size());
					ImPlot::EndPlot();
				}
			}
			else
			{
				if (ImPlot::BeginPlot(u8"Сигналы", main_row_size))
				{
					ImPlot::SetupAxes(u8"Время, t", "f(x)");

					int i = 0;
					for (auto signal : signals)
					{
						if (signal->Enabled)
						{
							auto values = signal->GetValues();
							ImGui::PushID(i++);
							ImPlot::PlotLine(signal->GetFunction().c_str(), time_domain.data(), values.data(), time_domain.size());
							ImGui::PopID();
						}
					}

					ImPlot::EndPlot();
				}
			}

			ImGui::End();
		}

		if (ImGui::Begin(u8"ДПФ"))
		{
			ImGui::Checkbox(u8"Точками", &plot_dft_stems);
			ImGui::SameLine();
			ImGui::Checkbox(u8"Половина", &draw_half_fft);

			int K = 1;
			if (draw_half_fft)
				K = 2;

			if (ImPlot::BeginPlot(u8"ДПФ##Summ", ImVec2(-1.0f, -1.0f)))
			{
				ImPlot::SetupAxes(u8"Частота", u8"Длина");
				
				if (calc_summ)
				{
					if (plot_dft_stems)
					{
						ImPlot::SetNextMarkerStyle(ImPlotMarker_Diamond);
						ImPlot::PlotStems("##DFT_on_summ", frequencies.data(), real_dft_summ.data(), frequencies.size() / K);
					}
					else
					{
						ImPlot::PlotLine("##DFT_on_summ", frequencies.data(), real_dft_summ.data(), frequencies.size() / K);
					}
				}
				else
				{
					int i = 0;
					for (auto signal : signals)
					{
						if (signal->Enabled)
						{
							Vector& values = signal->GetRealDFTValues();

							ImGui::PushID(i++);
							if (plot_dft_stems)
							{
								ImPlot::SetNextMarkerStyle(i % ImPlotMarker_COUNT);

								if (use_fft)
									ImPlot::PlotStems(signal->GetFunction().c_str(), frequencies.data(), values.data(), frequencies.size() / K);
								else
									ImPlot::PlotStems(signal->GetFunction().c_str(), frequencies.data(), values.data(), frequencies.size() - 1);
							}
							else
							{
								if (use_fft)
									ImPlot::PlotLine(signal->GetFunction().c_str(), frequencies.data(), values.data(), frequencies.size() / K);
								else
									ImPlot::PlotLine(signal->GetFunction().c_str(), frequencies.data(), values.data(), frequencies.size() - 1);
							}
							ImGui::PopID();
						}
					}
				}

				ImPlot::EndPlot();
			}

			ImGui::End();
		}

		if (ImGui::Begin(u8"Обратное ДПФ"))
		{
			auto& idft_summ = summ.GetRealIDFTValues();

			ImGui::Checkbox(u8"Половина", &draw_half_fft);

			int K = 1;
			if (draw_half_fft)
				K = 2;

			if (ImPlot::BeginPlot(u8"ОПФ##Summ", ImVec2(-1.0f, -1.0f)))
			{
				ImPlot::SetupAxes(u8"Частота", u8"Длина");

				if (calc_summ)
				{
					ImPlot::SetNextMarkerStyle(ImPlotMarker_Diamond);

					if (use_fft)
						ImPlot::PlotLine("##DFT_on_summ", frequencies.data(), idft_summ.data(), frequencies.size() / K);
					else
						ImPlot::PlotLine("##DFT_on_summ", frequencies.data(), idft_summ.data(), frequencies.size() - 1);
				}
				else
				{
					int i = 0;
					for (auto signal : signals)
					{
						if (signal->Enabled)
						{
							Vector& values = signal->GetRealIDFTValues();

							ImGui::PushID(i++);
							
							ImPlot::SetNextMarkerStyle(i % ImPlotMarker_COUNT);

							if (use_fft)
								ImPlot::PlotLine(signal->GetFunction().c_str(), frequencies.data(), values.data(), frequencies.size() / K);
							else
								ImPlot::PlotLine(signal->GetFunction().c_str(), frequencies.data(), values.data(), frequencies.size() - 1);
							
							ImGui::PopID();
						}
					}
				}

				ImPlot::EndPlot();
			}

			ImGui::End();
		}
	}

	void SPU::Draw()
	{
		if (ImGui::BeginMainMenuBar())
		{
#ifdef SHOW_DEMO
			if (ImGui::BeginMenu(u8"ДЕМО"))
			{
				ImGui::MenuItem("ImGui", NULL, &show_imgui_demo);
				ImGui::MenuItem("ImPlot", NULL, &show_implot_demo);
				ImGui::EndMenu();
			}
#endif
			if (ImGui::BeginMenuEx(u8"Файл", ICON_FA_MINUS))
			{
				if (ImGui::MenuItemEx(u8"Выход", ICON_FA_SIGN_OUT_ALT))
				{
					Application::Get()->Close();
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenuEx(u8"Тема", ICON_FA_CARET_DOWN))
			{
				if (ImGui::MenuItem("\"Visual Studio\""))
				{
					Application::Get()->SetStyleVS();
				}
				if (ImGui::MenuItem("Blue"))
				{
					Application::Get()->SetStyleBlue();
				}
				if (ImGui::MenuItem("Dark"))
				{
					Application::Get()->SetStyleDark();
				}
				if (ImGui::MenuItem("Light"))
				{
					Application::Get()->SetStyleLight();
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenuEx(u8"Настройки", ICON_FA_CARET_DOWN))
			{
				if (ImGui::MenuItemEx("Reload \"Header.lua\"", ICON_FA_REDO))
				{
					ReloadHeader();
				}


				if (ImGui::MenuItemEx(u8"FFT", "\xef\xa2\x99", NULL, &use_fft))
				{
					update_graph = true;
					update_dft = true;
				}

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		static ImGuiWindowFlags dockspace_flags =
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_MenuBar |
			ImGuiWindowFlags_NoNavFocus;

		ImGuiID dockspace_id = ImGui::GetID("root");

		if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr)
		{
			ImGui::DockBuilderRemoveNode(dockspace_id);
			ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
			ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetIO().DisplaySize);

			ImGuiID dock_main_id = dockspace_id;
			ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.85f, NULL, &dock_main_id);
			ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.2f, NULL, &dock_main_id);
			///ImGuiDockNode* right_node = ImGui::DockBuilderGetNode(dock_id_right);
			///right_node->LocalFlags = ImGuiDockNodeFlags_NoResize;

			ImGui::DockBuilderDockWindow(u8"Сигналы", dock_id_bottom);
			ImGui::DockBuilderDockWindow(u8"ДПФ", dock_id_bottom);
			ImGui::DockBuilderDockWindow(u8"Обратное ДПФ", dock_id_bottom);
			ImGui::DockBuilderDockWindow("Main", dock_main_id);

			ImGui::DockBuilderFinish(dockspace_id);
		}
		ImGuiIO& io = ImGui::GetIO();
		static ImVec2 offset = ImVec2(0.0f, 0.0f);
		ImVec2 new_size = ImVec2(io.DisplaySize.x - offset.x * 2.0f, io.DisplaySize.y - offset.y);
		ImGui::SetNextWindowPos(offset);
		ImGui::SetNextWindowSize(new_size);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		if (ImGui::Begin("DockSpace Demo", NULL, dockspace_flags))
		{
			ImGui::PopStyleVar(3);
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_NoCloseButton);

			ImGui::End();
		}

		ImGui::BeginDisabled(!header_loaded);

		if (ImGui::Begin("Main", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus))
		{
			DrawInputs();

			ImGui::End();
		}

		if (update_graph)
		{
			UpdateData();
			update_graph = false;
		}

		if (update_dft)
		{
			UpdateDFT();
			update_dft = false;
		}

		DrawPlots();

		ImGui::EndDisabled();

#ifdef SHOW_DEMO
		if (show_imgui_demo)
			ImGui::ShowDemoWindow(&show_imgui_demo);

		if (show_implot_demo)
			ImPlot::ShowDemoWindow(&show_implot_demo);
#endif

		ImGui::RenderNotifications();
	}

	CVector SPU::DFT(Vector input, int N)
	{
		if (N == 0) N = input.size();

		CVector output(N);

		for (unsigned int m = 0; m < N; m++)
		{
			CNumber summ(0.0, 0.0);

			for (unsigned int n = 0; n < N; n++)
			{
				float theta = PI2 * m * n / N;
				summ += CNumber(cosf(theta), -sin(theta)) * input[n];
			}

			output[m] = summ;
		}

		return output;
	}

	Vector SPU::IDFT(CVector input, int N)
	{
		if (N == 0) N = input.size();

		const float RN = 1.0f / (float)N;

		Vector output(N);

		for (unsigned int m = 0; m < N; m++)
		{
			CNumber summ(0.0, 0.0);
			for (int n = 0; n < N; n++)
			{
				float theta = PI2 * m * -n / N;
				summ += CNumber(cosf(theta), -sin(theta)) * input[n];
			}

			output[m] = summ.real() * RN;
		}

		return output;
	}

	CVector SPU::FFT(Vector input, int N)
	{
		if (N == 0)
		{
			N = input.size();
		}
		else
		{
			input.resize(N);
		}

		if (N == 1)
		{
			return CVector(1, input[0]);
		}

		const int HN = N / 2;
		Vector even(HN);
		Vector odd(HN);

		for (int i = 0; i < HN; i++)
		{
			even[i] = input[i * 2];
			odd[i] = input[i * 2 + 1];
		}

		CVector even_v = FFT(even);
		CVector odd_v = FFT(odd);
		CVector output(N);

		float d = 2 * PI2;
		for (int i = 0; i < N; i++)
		{
			float alpha = PI2 * i / N;
			CNumber w(cosf(alpha), sinf(alpha));
			output[i] = even_v[i % HN] + w * odd_v[i % HN];
		}

		return output;
	}

	CVector SPU::FFT(CVector input, int N)
	{
		if (N == 0)
		{
			N = input.size();
		}
		else
		{
			input.resize(N);
		}

		if (N == 1)
		{
			return CVector(1, input[0]);
		}

		const int HN = N / 2;
		Vector even(HN);
		Vector odd(HN);

		for (int i = 0; i < HN; i++)
		{
			even[i] = input[i * 2].real();
			odd[i] = input[i * 2 + 1].real();
		}

		CVector even_v = FFT(even);
		CVector odd_v = FFT(odd);
		CVector output(N);

		float d = 2 * PI2;
		for (int i = 0; i < N; i++)
		{
			float alpha = PI2 * i / N;
			CNumber w(cosf(alpha), sinf(alpha));
			output[i] = even_v[i % HN] + w * odd_v[i % HN];
		}

		return output;
	}

	Vector SPU::IFFT(CVector input, int N)
	{
		if (N == 0)
		{
			N = input.size();
		}
		else
		{
			input.resize(N);
		}

		if (N == 1)
		{
			return Vector(1, input[0].real());
		}

		const int HN = N / 2;
		const float RN = 1.0f / (float)N;

		CVector even(HN);
		CVector odd(HN);

		for (int i = 0; i < HN; i++)
		{
			even[i] = input[i * 2];
			odd[i] = input[i * 2 + 1];
		}

		Vector even_v = IFFT(even);
		Vector odd_v = IFFT(odd);
		Vector output(N);

		for (int i = 0; i < N; i++)
		{
			float alpha = PI2 * i / N;
			CNumber w(cosf(alpha), sinf(alpha));
			auto complex = even_v[i % HN] + w * odd_v[i % HN];
			output[i] = complex.real() * RN;
		}

		return output;
	}
}