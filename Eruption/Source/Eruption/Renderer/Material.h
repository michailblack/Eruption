#pragma once
#include "Eruption/Platform/Vulkan/Descriptors.h"
#include "Eruption/Platform/Vulkan/Pipeline.h"
#include "Eruption/Platform/Vulkan/Shader.h"
#include "Eruption/Platform/Vulkan/Texture.h"

namespace Eruption
{
	// Binds a shader + its graphics pipeline to a set of per-material resources (set 1). For the built-in
	// mesh shader that is a single albedo texture (sampled image + sampler). The scene set (set 0) and
	// the per-object model push constant are supplied by the SceneRenderer at draw time.
	class Material
	{
	public:
		Material(
		    const Ref<Vulkan::Shader>&              shader,
		    const Ref<Vulkan::Texture2D>&           albedo,
		    const Ref<Vulkan::DescriptorSetLayout>& sceneLayout,
		    Vulkan::DescriptorAllocator&            allocator,
		    vk::Format                              colorFormat,
		    vk::Format                              depthFormat
		);

		[[nodiscard]] const Ref<Vulkan::GraphicsPipeline>& GetPipeline() const { return m_Pipeline; }
		[[nodiscard]] vk::DescriptorSet                    GetDescriptorSet() const { return m_DescriptorSet; }

	private:
		Ref<Vulkan::Shader>              m_Shader;
		Ref<Vulkan::Texture2D>           m_Albedo;
		Ref<Vulkan::DescriptorSetLayout> m_MaterialLayout;
		Ref<Vulkan::GraphicsPipeline>    m_Pipeline;
		vk::DescriptorSet                m_DescriptorSet;
	};
}        // namespace Eruption
