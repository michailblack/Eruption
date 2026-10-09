#pragma once
#include "Eruption/Platform/Vulkan/Descriptors.h"
#include "Eruption/Renderer/Camera.h"
#include "Eruption/Renderer/Material.h"
#include "Eruption/Renderer/Mesh.h"

#include <glm/glm.hpp>

namespace Eruption
{
	struct DirectionalLight
	{
		glm::vec3 Direction = {-0.5f, -1.0f, -0.3f};
		glm::vec3 Color     = {1.0f, 1.0f, 1.0f};
		float     Intensity = 1.0f;
	};

	// A minimal forward renderer. Owns the per-frame scene uniform (set 0) and shared descriptor
	// allocator, creates materials bound to the scene layout, and records mesh draws through the
	// renderer's command queue.
	class SceneRenderer
	{
	public:
		SceneRenderer();

		[[nodiscard]] Ref<Material> CreateMaterial(
		    const Ref<Vulkan::Shader>& shader, const Ref<Vulkan::Texture2D>& albedo
		);

		void BeginScene(const Camera& camera, const DirectionalLight& light);
		void SubmitMesh(const Ref<Mesh>& mesh, const Ref<Material>& material, const glm::mat4& transform);
		void EndScene();

		[[nodiscard]] const Ref<Vulkan::DescriptorSetLayout>& GetSceneLayout() const { return m_SceneLayout; }

	private:
		// std140-friendly layout matching SceneData in Mesh.slang.
		struct GPUSceneData
		{
			glm::mat4 ViewProjection;
			glm::vec4 CameraPosition;
			glm::vec4 LightDirection;
			glm::vec4 LightColor;
		};

		Ref<Vulkan::DescriptorSetLayout> m_SceneLayout;
		Vulkan::DescriptorAllocator      m_Allocator;

		std::vector<Ref<Vulkan::Buffer>> m_SceneUniformBuffers;        // per frame in flight
		std::vector<vk::DescriptorSet>   m_SceneDescriptorSets;        // per frame in flight
	};
}        // namespace Eruption
