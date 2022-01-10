#include "SPU.h"
#include <iostream>
#include <complex>

#define USE_FFT

#define PI  3.141592653589793238460f
#define PI2 6.28318530717958647692f

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
	const int original = input.size();
	auto target_size = NextPowerOf2(original);
	input.resize(target_size);
}

void LoadWindowFunctions()
{
	WindowFunctions::Add("Rectangle", [](int idx, int N) -> double {
		return 1.0f;
		});
	WindowFunctions::Add("Triangle", [](int idx, int N) -> double {
		const float FN = (float)N;

		return 1.0f - fabs((idx - (FN / 2.0f)) / (FN / 2.0f));
		});
	WindowFunctions::Add("Blackman", [](int idx, int N) -> double {
		const float alpha = 0.16f;
		const float a0 = (1.0f - alpha) / 2.0f;
		const float a1 = 0.5f;
		const float a2 = alpha / 2.0f;

		return a0 - a1 * cosf((PI2 * idx) / (float)N) + a2 * cosf((2.0f * PI2 * idx) / (float)N);
		});
	WindowFunctions::Add("FlatTop", [](int idx, int N) -> double {
		const float a0 = 0.21557895f;
		const float a1 = 0.41663158f;
		const float a2 = 0.277263158f;
		const float a3 = 0.083578947f;
		const float a4 = 0.006947368f;
		const float FN = (float)N;

		return a0 - a1 * cosf((PI2 * idx) / FN) + a2 * cosf((2.0f * PI2 * idx) / FN) - a3 * cosf((3.0f * PI2 * idx) / FN) + a4 * cosf((4.0f * PI2 * idx) / FN);
		});
	WindowFunctions::Add("Tukey", [](int idx, int N) -> double {
		const float alpha = 0.5f;
		const float aN = (float)N * alpha;

		if (idx < aN / 2.0f)
			return 0.5f * (1.0f - cosf((PI2 * idx) / aN));
		else if (idx >= aN / 2.0f && idx < N / 2 + aN / 2.0f)
			return 1.0f;
		else
			return 0.5f * (1.0f - cosf((PI2 * idx) / aN));
		});
	WindowFunctions::Add("Hann", [](int idx, int N) -> double {
		const float FN = (float)N;
		const float a0 = 0.5f;
		const float a1 = 1.0f - a0;

		return a0 - a1 * cosf((PI2 * idx) / FN);
		});
	WindowFunctions::Add("Hamming", [](int idx, int N) -> double {
		const float FN = (float)N;
		const float a0 = 25.0f / 46.0f;
		const float a1 = 1.0f - a0;

		return a0 - a1 * cosf((PI2 * idx) / FN);
		});
}


void SPU::Load()
{
	LoadWindowFunctions();
	ReloadHeader();	

	//Vector test_data{ 0.0f, 0.3f, 0.6f, 0.8f, 1.0f, 1.0f, 0.9f, 0.7f, 0.5f, 0.2f, 0.2f, 0.5f, 0.7f, 0.9f, 1.0f, 1.0f, 0.8f, 0.6f, 0.3f, 0.0f };
	//CVector out = DFT(test_data);
	//
	//for(auto v : out)
	//	std::cout << abs(v) << std::endl;
}

void SPU::ReloadHeader()
{
	if (luaL_dofile(L, "Header.lua") == LUA_OK)
	{
		update_graph = true;
		update_dft = true;
		header_loaded = true;
		ImGui::InsertNotification({ ImGuiToastType_Success, 5, "Header.lua successfully loaded" });
	}
	else
	{
		const char* error = lua_tostring(L, -1);
		ImGui::InsertNotification({ ImGuiToastType_Error, 5, error });
	}
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

void SPU::UpdateDFT()
{

#ifdef USE_FFT
	const float K = 0.5f; // поливина отчетов
	const float RK = 1 / K;
	const int size = duration * fs;
	const unsigned int new_size = NextPowerOf2(size);
	const unsigned int half_size = new_size / RK;
	const float D = fs * RK / new_size; // scale frequency to match FFT

	if (calc_summ)
	{
		dft_summ.resize(half_size);
	}


	//0-4094-1 fs*(0,1,2,3,4,5,...) / new_size

	frequencies.resize(half_size);
	
	for (size_t i = 0; i < half_size; i++)
	{
		frequencies[i] = i * D;
	}

#else
	cosnt float RT = 1.0f / duration;
	const int size = duration * ts;
	frequencies.resize(size);

	for (size_t i = 0; i < frequencies.size(); i++)
	{
		frequencies[i] = i * RT;
	}

#endif

	if (calc_summ)
	{
#ifdef USE_FFT
		CVector fft = FFT(values_summ, new_size);
		ConvertComplexVector(fft, dft_summ);
#else
		CVector dft = DFT(values_summ, new_size);
		ConvertComplexVector(dft, dft_summ);
#endif
	}
	else
	{
		for (auto signal : signals)
		{
			if (signal->Enabled)
			{
				Vector& values = signal->GetValues();
				Vector& out = signal->GetDFTValues();
#ifdef USE_FFT
				ResizeToNextPowOf2(values);
				CVector fft = FFT(values, new_size);
				ConvertComplexVector(fft, out);
#else
				CVector dft = DFT(values, new_size);
				ConvertComplexVector(dft, out);
#endif
			}
		}
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

float SPU::CalculateFunctionValue(std::string name, float time_in_space)
{
	lua_getglobal(L, name.c_str());
	if (lua_isfunction(L, -1))
	{
		int size = duration * ts;
		lua_pushnumber(L, time_in_space);
		if (lua_pcall(L, 1, 1, 0) == LUA_OK)
		{
			if (lua_isnumber(L, -1))
			{
				float value = (float)lua_tonumber(L, -1);
				lua_pop(L, 1);
				return value;
			}
			else
			{
				//ImGui::InsertNotification({ ImGuiToastType_Error, 3, "Can calculate function value!" });
				lua_pop(L, 1);
			}
		}
	}
	else
	{
		ImGui::InsertNotification({ ImGuiToastType_Error, 3, "\"%s\" is not a valid function!", name});
		lua_pop(L, 1);
	}
	return 0.0f;
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
	if (ImGui::Button("+##AddSignal", ImVec2(-1.0, 0.0)))
	{
		Signal* signal = new Signal();
		//signal.Randomize();
		signals.push_back(signal);
		update_graph = true;
	}

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
	update_graph |= ImGui::SliderFloat(u8"Частота дискретизации", &fs, 0.1f, 10000.0f);
}

void SPU::DrawPlots()
{
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
					ImPlot::PlotStems("##DFT_on_summ", frequencies.data(), dft_summ.data(), frequencies.size() / K);
				}
				else
				{
					ImPlot::PlotLine("##DFT_on_summ", frequencies.data(), dft_summ.data(), frequencies.size() / K);
				}
			}
			else
			{
				int i = 0;
				for (auto signal : signals)
				{
					if (signal->Enabled)
					{
						Vector& values = signal->GetDFTValues();
						ImGui::PushID(i++);
						if (plot_dft_stems)
						{
							ImPlot::SetNextMarkerStyle(i % ImPlotMarker_COUNT);

#ifdef USE_FFT
							ImPlot::PlotStems(signal->GetFunction().c_str(), frequencies.data(), values.data(), frequencies.size() / K);
#else
							ImPlot::PlotStems(signal->GetFunction().c_str(), frequencies.data(), values.data(), frequencies.size() - 1);
#endif
						}
						else
						{
#ifdef USE_FFT
							ImPlot::PlotLine(signal->GetFunction().c_str(), frequencies.data(), values.data(), frequencies.size() / K);
#else
							ImPlot::PlotLine(signal->GetFunction().c_str(), frequencies.data(), values.data(), frequencies.size() - 1);
#endif
						}
						ImGui::PopID();
					}
				}
			}

			ImPlot::EndPlot();
		}

		ImGui::End();
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

void SPU::Draw()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu(u8"ДЕМО"))
		{
			ImGui::MenuItem("ImGui", NULL, &show_imgui_demo);
			ImGui::MenuItem("ImPlot", NULL, &show_implot_demo);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(u8"Тема"))
		{
			if (ImGui::MenuItem("\"Visual Studio\""))
			{
				SetStyleVS();
			}
			if (ImGui::MenuItem("Blue"))
			{
				SetStyleBlue();
			}
			if (ImGui::MenuItem("Dark"))
			{
				ImGui::StyleColorsDark();
				ImPlot::StyleColorsDark();
			}
			if (ImGui::MenuItem("Light"))
			{
				ImGui::StyleColorsLight();
				ImPlot::StyleColorsLight();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(u8"Настройки"))
		{
			if (ImGui::MenuItem(ICON_FA_REDO " LUA"))
			{
				ReloadHeader();
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
		//ImGui::DockBuilderDockWindow("DFT", dock_id_bottom);
		ImGui::DockBuilderDockWindow("Debug data", dock_id_right);
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

	if (ImGui::Begin("Debug data"))
	{
#ifdef USE_FFT
		ImGui::Text("Mode: FFT");
#else
		ImGui::Text("Mode: DFT");
#endif
		ImGui::Separator();

		const float offset = 150.0f;
		ImGui::Checkbox("header_loaded", &header_loaded); ImGui::SameLine(offset);
		ImGui::Checkbox("window_drag", &window_drag); 
		ImGui::Checkbox("show_imgui_demo", &show_imgui_demo); ImGui::SameLine(offset);
		ImGui::Checkbox("show_implot_demo", &show_implot_demo); 
		ImGui::Checkbox("apply_window", &apply_window);  ImGui::SameLine(offset);
		ImGui::Checkbox("calc_summ", &calc_summ);
		ImGui::Checkbox("update_graph", &update_graph); ImGui::SameLine(offset);
		ImGui::Checkbox("update_dft", &update_dft);

		ImGui::BulletText("Function: %s", selected_window_function.c_str());
		ImGui::BulletText("Signals size:     %d", signals.size());
		for (int i = 0; i < signals.size(); i++)
		{
			ImGui::Indent(10.0f);
			ImGui::BulletText("[%d] DFT size: %d", i, signals[i]->GetDFTValues().size());
			ImGui::Unindent(10.0f);
		}

		ImGui::BulletText("frequencies size: %d", frequencies.size());
		ImGui::BulletText("time_domain size: %d", time_domain.size());
		ImGui::BulletText("values_summ size: %d", values_summ.size());
		ImGui::BulletText("dft_summ size:    %d", dft_summ.size());

		ImGui::BulletText("Duration: %.3f", duration);
		ImGui::BulletText("TS:       %d", ts);
		ImGui::BulletText("FS:       %.3f", fs);

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

	if (show_imgui_demo)
		ImGui::ShowDemoWindow(&show_imgui_demo);

	if (show_implot_demo)
		ImPlot::ShowDemoWindow(&show_implot_demo);

	ImGui::RenderNotifications();
}

CVector SPU::DFT(Vector input, int N)
{
	if (N == 0) N = input.size();
	const float d = PI2 / N;

	CVector output(N);

	for (unsigned int m = 0; m < N; m++)
	{
		CNumber summ(0.0, 0.0);

		for (unsigned int n = 0; n < N; n++)
		{
			float theta = m * n * d;
			summ += exp(CNumber(0.0, theta)) * input[n];
		}

		output[m] = summ;
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

	float d = 2 * PI2 / N;
	for (int i = 0; i < N; i++)
	{
		float alpha = d * i;
		CNumber w(cosf(alpha), sinf(alpha));
		output[i] = even_v[i % HN] + w * odd_v[i % HN];
	}

	return output;
}
