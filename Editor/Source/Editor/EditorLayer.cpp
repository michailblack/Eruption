#include "EditorLayer.h"

#include "Eruption/Core/Input.h"

namespace Eruption
{
	EditorLayer::EditorLayer() : Layer("EditorLayer")
	{}

	void EditorLayer::OnAttach()
	{
		ER_CORE_INFO("EditorLayer::OnAttach");
	}

	void EditorLayer::OnDetach()
	{
		ER_CORE_INFO("EditorLayer::OnDetach");
	}

	void EditorLayer::OnUpdate(DeltaTime dt)
	{}

	void EditorLayer::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);

		dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& e) { return OnKeyPressed(e); });

		dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& e) {
			return OnMouseButtonPressed(e);
		});

		dispatcher.Dispatch<MouseScrolledEvent>([this](MouseScrolledEvent& e) { return OnMouseScrolled(e); });

		dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e) { return OnWindowResize(e); });
	}

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

	void EditorLayer::OnImGuiRender()
	{
		// Dockspace
		// Menu bar
		// Viewport
		// Scene hierarchy
		// Properties panel
		// Content browser
		// Console
	}
}        // namespace Eruption
