#pragma once

#ifdef _WIN64

	#include <Windows.h>

#elif defined(_WIN32)

	#error "No win32 support"

#else

	#error "Wrong platform"

#endif

#include <string>
#include <map>
#include <GLFW/glfw3.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <codecvt>
#include <algorithm>
#include <memory>
#include <vector>
#include <cmath>
#include <Lua/lua.hpp>

#include "imgui/imgui.h"
#include "fonts/Fonts.h"
#include "imgui/imgui_internal.h"
#include "implot/implot.h"
#include "imgui/notify.h"
#include "stb_image.h"

#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"