#pragma once
#include "Eruption/Core/Events/ApplicationEvent.h"
#include "Eruption/Core/Events/KeyEvent.h"
#include "Eruption/Core/Events/MouseEvent.h"
#include "Eruption/Core/Layer.h"

#include "Eruption/Renderer/Camera.h"
#include "Eruption/Renderer/Material.h"
#include "Eruption/Renderer/Mesh.h"
#include "Eruption/Renderer/SceneRenderer.h"

namespace Eruption
{
	class EditorLayer : public Layer
	{
	public:
		EditorLayer() : Layer("EditorLayer") {}

		void OnAttach() override;
		void OnUpdate(DeltaTime dt) override;
		void OnRender() override;
		void OnEvent(Event& event) override;

	private:
		bool OnKeyPressed(KeyPressedEvent& e);

	private:
		Scope<SceneRenderer> m_SceneRenderer;

		Ref<Vulkan::Shader>    m_Shader;
		Ref<Vulkan::Texture2D> m_Albedo;
		Ref<Material>          m_Material;
		Ref<Mesh>              m_Mesh;

		Camera           m_Camera;
		DirectionalLight m_Light;

		float m_Time = 0.0f;
	};
}        // namespace Eruption
