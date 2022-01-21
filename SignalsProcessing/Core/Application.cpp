#define STB_IMAGE_IMPLEMENTATION
#define GL_CLAMP_TO_EDGE 0x812F

#include "Application.h"

namespace SP_LAB
{
	Application* Application::instance = nullptr;

	static void glfw_error_callback(int error, const char* description)
	{
		fprintf(stderr, "GLFW Error %d: %s\n", error, description);
	}

	Application::Application(Sandbox* sandbox)
	{
		app = sandbox;
	}

	Application::~Application()
	{
		if (instance == nullptr)
			return;

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImPlot::DestroyContext();
		ImGui::DestroyContext();
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	Application* Application::Get(Sandbox* sandbox)
	{
		if (instance == nullptr) 
		{
			instance = new Application(sandbox);
		}
		return instance;
	}

	void Application::AddFont(ImFontConfig* cfg, ImFontConfig* icons_config, const char* name, void* data, size_t len, float size, const ImWchar* ranges, const ImWchar* fa_ranges)
	{
		auto io = ImGui::GetIO();
		ImStrncpy(cfg->Name, name, 40);
		fonts[cfg->Name] = io.Fonts->AddFontFromMemoryTTF(data, len, size, cfg, ranges);
	}

	ImFont* Application::GetFont(std::string name)
	{
		return fonts[name];
	}

	void Application::InitImGui(const char* glsl_version)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init(glsl_version);

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		//io.IniFilename = "Settings.ini";
		io.IniFilename = NULL;

		io.Fonts->Clear();

		ImFontConfig font_cfg;
		font_cfg.FontDataOwnedByAtlas = false;

		ImFontConfig icons_config;
		icons_config.MergeMode = true;
		icons_config.PixelSnapH = true;
		icons_config.GlyphMinAdvanceX = 14.0f;
		icons_config.GlyphOffset = ImVec2(0, 0);
		icons_config.OversampleH = 1;
		icons_config.OversampleV = 1;
		icons_config.FontDataOwnedByAtlas = false;

		static const ImWchar fa_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
		static const ImWchar fa_brands_ranges[] = { ICON_MIN_FAB, ICON_MAX_FAB, 0 };

		const float font_size = 17.0f;

		//AddFont(&font_cfg, &icons_config, "Roboto Bold", Roboto_Bold_ttf, Roboto_Bold_ttf_len, font_size, ranges.Data, fa_ranges);
		//AddFont(&font_cfg, &icons_config, "Roboto Italic", Roboto_Italic_ttf, Roboto_Italic_ttf_len, font_size, ranges.Data, fa_ranges);
		AddFont(&font_cfg, &icons_config, "Roboto Regular", Roboto_Regular_ttf, Roboto_Regular_ttf_len, font_size, io.Fonts->GetGlyphRangesCyrillic(), fa_ranges);
		//AddFont(&font_cfg, &icons_config, "Roboto Mono Bold", RobotoMono_Bold_ttf, RobotoMono_Bold_ttf_len, font_size, ranges.Data, fa_ranges);
		//AddFont(&font_cfg, &icons_config, "Roboto Mono Italic", RobotoMono_Italic_ttf, RobotoMono_Italic_ttf_len, font_size, ranges.Data, fa_ranges);
		//AddFont(&font_cfg, &icons_config, "Roboto Mono Regular", RobotoMono_Regular_ttf, RobotoMono_Regular_ttf_len, font_size, ranges.Data, fa_ranges);

		io.Fonts->AddFontFromMemoryTTF(fa_solid_900_ttf, fa_solid_900_ttf_len, font_size - 1.0f, &icons_config, fa_ranges);
		//io.Fonts->AddFontFromMemoryTTF(fa_brands_400_ttf, fa_brands_400_ttf_len, font_size - 1.0f, &icons_config, fa_brands_ranges);

		io.Fonts->Build();

		ImPlot::CreateContext();
	}

	void Application::Setup(std::string title, int width, int height)
	{
		glfwSetErrorCallback(glfw_error_callback);
		if (!glfwInit())
			abort();
		const char* glsl_version = "#version 130";
		glfwWindowHint(GLFW_SAMPLES, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

		glfwWindowHint(GLFW_SAMPLES, 4); // MSA 4x
		window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);

		if (window == NULL)
		{
			fprintf(stderr, "Failed to initialize GLFW window!\n");
			abort();
		}

		glfwMakeContextCurrent(window);
		glfwSwapInterval(1);
		glfwSetWindowSizeLimits(window, 800, 450, GLFW_DONT_CARE, GLFW_DONT_CARE);


		InitImGui(glsl_version);
	}

	void Application::Run()
	{
		app->Load();

		while (!glfwWindowShouldClose(window) && running)
		{
			glfwPollEvents();

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			app->Draw();

			// Rendering
			ImGui::Render();
			int display_w, display_h;
			glfwGetFramebufferSize(window, &display_w, &display_h);
			glViewport(0, 0, display_w, display_h);
			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT);
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			glfwSwapBuffers(window);
		}
	}

	void Application::Close()
	{
		running = false;
	}

	bool Application::LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height)
	{
		// Load from file
		int image_width = 0;
		int image_height = 0;
		unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
		if (image_data == NULL)
			return false;

		// Create a OpenGL texture identifier
		GLuint image_texture;
		glGenTextures(1, &image_texture);
		glBindTexture(GL_TEXTURE_2D, image_texture);

		// Setup filtering parameters for display
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

		// Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
		stbi_image_free(image_data);

		*out_texture = image_texture;
		*out_width = image_width;
		*out_height = image_height;

		return true;
	}

	void Application::SetStyleVS()
	{
		auto ColorFromBytes = [](uint8_t r, uint8_t g, uint8_t b)
		{
			return ImVec4((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, 1.0f);
		};

		auto& style = ImGui::GetStyle();
		ImVec4* colors = style.Colors;

		const ImVec4 bgColor = ColorFromBytes(37, 37, 38);
		const ImVec4 lightBgColor = ColorFromBytes(82, 82, 85);
		const ImVec4 veryLightBgColor = ColorFromBytes(90, 90, 95);

		const ImVec4 panelColor = ColorFromBytes(51, 51, 55);
		const ImVec4 panelHoverColor = ColorFromBytes(29, 151, 236);
		const ImVec4 panelActiveColor = ColorFromBytes(0, 119, 200);

		const ImVec4 textColor = ColorFromBytes(255, 255, 255);
		const ImVec4 textDisabledColor = ColorFromBytes(151, 151, 151);
		const ImVec4 borderColor = ColorFromBytes(78, 78, 78);

		colors[ImGuiCol_Text] = textColor;
		colors[ImGuiCol_TextDisabled] = textDisabledColor;
		colors[ImGuiCol_TextSelectedBg] = panelActiveColor;
		colors[ImGuiCol_WindowBg] = bgColor;
		colors[ImGuiCol_ChildBg] = bgColor;
		colors[ImGuiCol_PopupBg] = bgColor;
		colors[ImGuiCol_Border] = borderColor;
		colors[ImGuiCol_BorderShadow] = borderColor;
		colors[ImGuiCol_FrameBg] = panelColor;
		colors[ImGuiCol_FrameBgHovered] = panelHoverColor;
		colors[ImGuiCol_FrameBgActive] = panelActiveColor;
		colors[ImGuiCol_TitleBg] = bgColor;
		colors[ImGuiCol_TitleBgActive] = bgColor;
		colors[ImGuiCol_TitleBgCollapsed] = bgColor;
		colors[ImGuiCol_MenuBarBg] = panelColor;
		colors[ImGuiCol_ScrollbarBg] = panelColor;
		colors[ImGuiCol_ScrollbarGrab] = lightBgColor;
		colors[ImGuiCol_ScrollbarGrabHovered] = veryLightBgColor;
		colors[ImGuiCol_ScrollbarGrabActive] = veryLightBgColor;
		colors[ImGuiCol_CheckMark] = panelActiveColor;
		colors[ImGuiCol_SliderGrab] = panelHoverColor;
		colors[ImGuiCol_SliderGrabActive] = panelActiveColor;
		colors[ImGuiCol_Button] = panelColor;
		colors[ImGuiCol_ButtonHovered] = panelHoverColor;
		colors[ImGuiCol_ButtonActive] = panelHoverColor;
		colors[ImGuiCol_Header] = panelColor;
		colors[ImGuiCol_HeaderHovered] = panelHoverColor;
		colors[ImGuiCol_HeaderActive] = panelActiveColor;
		colors[ImGuiCol_Separator] = borderColor;
		colors[ImGuiCol_SeparatorHovered] = borderColor;
		colors[ImGuiCol_SeparatorActive] = borderColor;
		colors[ImGuiCol_ResizeGrip] = bgColor;
		colors[ImGuiCol_ResizeGripHovered] = panelColor;
		colors[ImGuiCol_ResizeGripActive] = lightBgColor;
		colors[ImGuiCol_PlotLines] = panelActiveColor;
		colors[ImGuiCol_PlotLinesHovered] = panelHoverColor;
		colors[ImGuiCol_PlotHistogram] = panelActiveColor;
		colors[ImGuiCol_PlotHistogramHovered] = panelHoverColor;
		//colors[ImGuiCol_ModalWindowDarkening] = bgColor;
		colors[ImGuiCol_DragDropTarget] = bgColor;
		colors[ImGuiCol_NavHighlight] = bgColor;
#ifdef IMGUI_HAS_DOCK
		colors[ImGuiCol_DockingPreview] = panelActiveColor;
#endif
		colors[ImGuiCol_Tab] = bgColor;
		colors[ImGuiCol_TabActive] = panelActiveColor;
		colors[ImGuiCol_TabUnfocused] = bgColor;
		colors[ImGuiCol_TabUnfocusedActive] = panelActiveColor;
		colors[ImGuiCol_TabHovered] = panelHoverColor;

		style.FrameBorderSize = 0.0f;
		style.WindowRounding = 0.0f;
		style.ChildRounding = 0.0f;
		style.FrameRounding = 0.0f;
		style.GrabRounding = 0.0f;
		style.PopupRounding = 0.0f;
		style.ScrollbarRounding = 0.0f;
		style.TabRounding = 0.0f;

		ImPlot::StyleColorsAuto();
	}

	void Application::SetStyleBlue()
	{
		static const ImVec4 bg_dark = ImVec4(0.15f, 0.16f, 0.21f, 1.00f);
		static const ImVec4 bg_mid = ImVec4(0.20f, 0.21f, 0.27f, 1.00f);
		static const ImVec4 accent_dark = ImVec4(0.292f, 0.360f, 0.594f, 1.000f);
		static const ImVec4 accent_light = ImVec4(0.409f, 0.510f, 0.835f, 1.000f);
		static const ImVec4 active = ImVec4(0.107f, 0.118f, 0.157f, 1.000f);
		static const ImVec4 attention = ImVec4(0.821f, 1.000f, 0.000f, 1.000f);

		auto& style = ImGui::GetStyle();
		style.WindowPadding = { 6.0f, 6.0f };
		style.FramePadding = { 6.0f, 3.0f };
		style.CellPadding = { 6.0f, 3.0f };
		style.ItemSpacing = { 6.0f, 6.0f };
		style.ItemInnerSpacing = { 6.0f, 6.0f };
		style.ScrollbarSize = 16.0f;
		style.GrabMinSize = 8.0f;
		style.WindowBorderSize = style.ChildBorderSize = style.PopupBorderSize = style.TabBorderSize = 0;
		style.FrameBorderSize = 1.0f;
		style.WindowRounding = style.ChildRounding = style.PopupRounding = style.ScrollbarRounding = style.GrabRounding = style.TabRounding = 4;

		ImVec4* colors = style.Colors;
		colors[ImGuiCol_Text] = ImVec4(0.89f, 0.89f, 0.92f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.38f, 0.45f, 0.64f, 1.00f);
		colors[ImGuiCol_WindowBg] = bg_mid;
		colors[ImGuiCol_ChildBg] = ImVec4(0.20f, 0.21f, 0.27f, 0.00f);
		colors[ImGuiCol_PopupBg] = bg_mid;
		colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.06f);
		colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.02f);
		colors[ImGuiCol_FrameBgHovered] = accent_light;
		colors[ImGuiCol_FrameBgActive] = active;
		colors[ImGuiCol_TitleBg] = accent_dark;
		colors[ImGuiCol_TitleBgActive] = accent_dark;
		colors[ImGuiCol_TitleBgCollapsed] = accent_dark;
		colors[ImGuiCol_MenuBarBg] = accent_dark;
		colors[ImGuiCol_ScrollbarBg] = bg_mid;
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.89f, 0.89f, 0.93f, 0.27f);
		colors[ImGuiCol_ScrollbarGrabHovered] = accent_light;
		colors[ImGuiCol_ScrollbarGrabActive] = active;
		colors[ImGuiCol_CheckMark] = accent_dark;
		colors[ImGuiCol_SliderGrab] = accent_dark;
		colors[ImGuiCol_SliderGrabActive] = accent_light;
		colors[ImGuiCol_Button] = accent_dark;
		colors[ImGuiCol_ButtonHovered] = accent_light;
		colors[ImGuiCol_ButtonActive] = active;
		colors[ImGuiCol_Header] = accent_dark;
		colors[ImGuiCol_HeaderHovered] = accent_light;
		colors[ImGuiCol_HeaderActive] = active;
		colors[ImGuiCol_Separator] = accent_dark;
		colors[ImGuiCol_SeparatorHovered] = accent_light;
		colors[ImGuiCol_SeparatorActive] = active;
		colors[ImGuiCol_ResizeGrip] = accent_dark;
		colors[ImGuiCol_ResizeGripHovered] = accent_light;
		colors[ImGuiCol_ResizeGripActive] = active;
		colors[ImGuiCol_Tab] = ImVec4(1.00f, 1.00f, 1.00f, 0.02f);
		colors[ImGuiCol_TabHovered] = accent_light;
		colors[ImGuiCol_TabActive] = accent_dark;
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive] = active;
		colors[ImGuiCol_PlotLines] = accent_light;
		colors[ImGuiCol_PlotLinesHovered] = active;
		colors[ImGuiCol_PlotHistogram] = accent_light;
		colors[ImGuiCol_PlotHistogramHovered] = active;
		colors[ImGuiCol_TableHeaderBg] = accent_dark;
		colors[ImGuiCol_TableBorderStrong] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
		colors[ImGuiCol_TableBorderLight] = ImVec4(1.00f, 1.00f, 1.00f, 0.02f);
		colors[ImGuiCol_TableRowBg] = ImVec4(0, 0, 0, 0);
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.02f);
		colors[ImGuiCol_TextSelectedBg] = accent_light;
		colors[ImGuiCol_DragDropTarget] = attention;
		colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.98f, 0.95f, 0.73f);
#ifdef IMGUI_HAS_DOCK
		colors[ImGuiCol_DockingPreview] = ImVec4(0.85f, 0.85f, 0.85f, 0.28f);
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
#endif

		ImPlot::StyleColorsAuto();

		ImPlotStyle& plot_style = ImPlot::GetStyle();

		ImVec4* plot_colors = plot_style.Colors;
		plot_colors[ImPlotCol_PlotBg] = ImVec4(0, 0, 0, 0);
		plot_colors[ImPlotCol_PlotBorder] = ImVec4(0, 0, 0, 0);
		plot_colors[ImPlotCol_Selection] = attention;
		plot_colors[ImPlotCol_Crosshairs] = colors[ImGuiCol_Text];


		plot_style.AntiAliasedLines = true;
		plot_style.DigitalBitHeight = 20;
		plot_style.PlotPadding = plot_style.LegendPadding = { 12.0f, 12.0f };
		plot_style.LabelPadding = plot_style.LegendInnerPadding = { 6.0f, 6.0f };
		plot_style.LegendSpacing = { 10.0f, 2.0f };
		plot_style.AnnotationPadding = { 4.0f, 2.0f };

		const ImU32 dracula[] = { 4288967266, 4285315327, 4286315088, 4283782655, 4294546365, 4287429361, 4291197439, 4294830475, 4294113528, 4284106564 };
		plot_style.Colormap = ImPlot::AddColormap("Dracula", dracula, 10);
	}


	void Application::SetStyleLight()
	{
		ImGui::StyleColorsLight();
		ImPlot::StyleColorsLight();

		ImGui::GetStyle().FrameBorderSize = 1.0f;
	}

	void Application::SetStyleDark()
	{
		ImGui::StyleColorsDark();
		ImPlot::StyleColorsDark();

		ImGui::GetStyle().FrameBorderSize = 0.0f;
	}
}