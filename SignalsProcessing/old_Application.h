#pragma once
//#define NO_TITLE_WINDOW
#define PI2 6.28318530717958647692

#ifdef NO_TITLE_WINDOW
#define GLFW_EXPOSE_NATIVE_WIN32
#endif // NO_TITLE_WINDOW


#include <functional>
#include <map>
#include <GLFW/glfw3.h>
#ifdef NO_TITLE_WINDOW
#include <GLFW/glfw3native.h>
#endif // NO_TITLE_WINDOW
#include "fonts/Fonts.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "implot/implot.h"
#include "imgui/notify.h"
#include <Lua/lua.hpp>
#include "Signal.h"

/// Macro to disable console on Windows
#if defined(_WIN32) && defined(APP_NO_CONSOLE)
#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif

class Application
{
private:
	ImVec4 clear_color;
	GLFWwindow* glfw_window;
	std::map<std::string, ImFont*> fonts;
	bool force_close = false;

	void AddFont(ImFontConfig* cfg, ImFontConfig* icons_config, const char* name, void* data, size_t len, float size, const ImWchar* ranges, const ImWchar* fa_ranges);
protected:
	lua_State* L;
public:
	Application(std::string title, int width, int height);
	~Application();

	void SetStyleVS();
	void SetStyleBlue();

	virtual void Load() {};
	virtual void Draw() {};
	ImFont* GetFont(std::string name);
	GLFWwindow* GetWindow();
	void Run();
	void Close();
};