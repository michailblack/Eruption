#pragma once
#include "Eruption/Core/Application.h"
#include "Eruption/Core/Assert.h"

namespace Eruption
{
	int Main(int argc, char** argv)
	{
		Application* app = CreateApplication(argc, argv);
		ER_CORE_ASSERT(app, "Client Application is null!");
		if (app)
			app->Run();
		delete app;
		return 0;
	}
}        // namespace Eruption

#if ER_PLATFORM_WINDOWS

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
	return Hazel::Main(__argc, __argv);
}

#else

int main(int argc, char** argv)
{
	return Eruption::Main(argc, argv);
}

#endif
