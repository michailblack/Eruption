#include "EditorLayer.h"

#include "Eruption/Platform/Vulkan/Shader.h"
#include "Eruption/Platform/Vulkan/Texture.h"
#include "Eruption/Renderer/MeshImporter.h"
#include "Eruption/Renderer/Renderer.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Eruption
{
	void EditorLayer::OnAttach()
	{
		m_SceneRenderer = CreateScope<SceneRenderer>();

		m_Shader = Vulkan::Shader::Create(std::filesystem::path(ER_SHADER_SOURCE_DIR) / "Mesh.slang");

		m_Albedo   = Vulkan::Texture2D::CreateFlat({0.85f, 0.85f, 0.88f, 1.0f}, "DefaultAlbedo");
		m_Material = m_SceneRenderer->CreateMaterial(m_Shader, m_Albedo);

		// A model can be dropped in and loaded instead:
		//   if (auto model = MeshImporter::Load(ER_ASSET_DIR "/Models/Suzanne.gltf")) m_Mesh = model->Geometry;
		m_Mesh = Mesh::CreateCube();

		m_Camera.SetPerspective(50.0f, 16.0f / 9.0f, 0.1f, 100.0f);
	}

	void EditorLayer::OnUpdate(DeltaTime dt)
	{
		m_Time += dt;

		const vk::Extent2D extent = Renderer::GetViewportExtent();
		m_Camera.SetViewportSize(extent.width, extent.height);

		// Orbit the camera around the origin.
		constexpr float radius = 3.5f;
		const glm::vec3 eye{std::sin(m_Time) * radius, 1.5f, std::cos(m_Time) * radius};
		m_Camera.LookAt(eye, glm::vec3(0.0f));
	}

	void EditorLayer::OnRender()
	{
		m_SceneRenderer->BeginScene(m_Camera, m_Light);

		const glm::mat4 transform = glm::rotate(glm::mat4(1.0f), m_Time * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));
		m_SceneRenderer->SubmitMesh(m_Mesh, m_Material, transform);

		m_SceneRenderer->EndScene();
	}

	void EditorLayer::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& e) { return OnKeyPressed(e); });
	}

	bool EditorLayer::OnKeyPressed(KeyPressedEvent& e)
	{
		// Hot-reload the shader on R and rebuild the material's pipeline against the new modules.
		if (e.GetKeyCode() == KeyCode::R)
		{
			if (m_Shader->Reload())
				m_Material = m_SceneRenderer->CreateMaterial(m_Shader, m_Albedo);
			return true;
		}
		return false;
	}
}        // namespace Eruption
