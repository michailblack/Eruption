#include "Eruption/EntryPoint.h"

namespace Eruption
{
	Application* CreateApplication(int argc, char** argv)
	{
		return new Application(ApplicationSpecification{});
	}
}        // namespace Eruption
