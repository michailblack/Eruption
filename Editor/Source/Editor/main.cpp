#include "Eruption/EntryPoint.h"

#include "Editor/EditorLayer.h"

namespace Eruption
{
	class EditorApplication final : public Application
	{
	public:
		explicit EditorApplication(const ApplicationSpecification& spec) : Application(spec)
		{
			PushLayer(new EditorLayer());
		}
	};

	Application* CreateApplication(int argc, char** argv)
	{
		return new EditorApplication(ApplicationSpecification{});
	}
}        // namespace Eruption
