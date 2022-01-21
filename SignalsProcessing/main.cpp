#include "Core/Application.h"
#include "SPU.h"

int main(int argc, char const* argv[]) 
{
	auto sandbox = std::make_unique<SP_LAB::SPU>();
	auto app = SP_LAB::Application::Get(sandbox.get());
	{
		app->Setup(u8"Обработка сигналов v0.3", 1280, 720);
		app->Run();
	}

	delete app;
}