#include "EditorLayer.h"

#include "Eruption/Core/Input.h"

namespace Eruption
{
	void EditorLayer::OnUpdate(DeltaTime dt)
	{}

	bool EditorLayer::OnKeyPressed(KeyPressedEvent& e)
	{
		return false;
	}

	bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		return false;
	}

	bool EditorLayer::OnMouseScrolled(MouseScrolledEvent& e)
	{
		return false;
	}

	bool EditorLayer::OnWindowResize(WindowResizeEvent& e)
	{
		return false;
	}
}        // namespace Eruption
