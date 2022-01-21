#pragma once

#include "../pch.h"
#include "Sandbox.h"

namespace SP_LAB
{
	class Application
	{

	public:
		Application(Application& other) = delete;
		~Application();

		void operator=(const Application&) = delete;

		static Application* Get(Sandbox* sandbox = nullptr);
		GLFWwindow* GetWindow() { return window; }
		ImFont* GetFont(std::string name);
		void Setup(std::string title, int width, int height);
		void Run();
		void Close();
		bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height);

		void SetStyleVS();
		void SetStyleBlue();
		void SetStyleLight();
		void SetStyleDark();

	protected:
		Application(Sandbox* sandbox);
	private:
		void AddFont(ImFontConfig* cfg, ImFontConfig* icons_config, const char* name, void* data, size_t len, float size, const ImWchar* ranges, const ImWchar* fa_ranges);
		void InitImGui(const char* glsl_version);

		bool running = true;

		Sandbox* app;
		GLFWwindow* window;
		std::map<std::string, ImFont*> fonts;
		static Application* instance;
	};
}
