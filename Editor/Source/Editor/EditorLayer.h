#pragma once
#include "Eruption/Core/Events/ApplicationEvent.h"
#include "Eruption/Core/Events/KeyEvent.h"
#include "Eruption/Core/Events/MouseEvent.h"

#include "Eruption/Core/Layer.h"

namespace Eruption
{
	class EditorLayer : public Layer
	{
	public:
		~EditorLayer() override = default;

		void OnUpdate(DeltaTime dt) override;
		void OnRender() override {}

	private:
		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
		bool OnMouseScrolled(MouseScrolledEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

	private:
	};
}        // namespace Eruption
