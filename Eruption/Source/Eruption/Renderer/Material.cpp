#include "Material.h"

#include "Eruption/Platform/Vulkan/VulkanContext.h"
#include "Eruption/Renderer/Mesh.h"

#include <glm/glm.hpp>

namespace Eruption
{
	Material::Material(
	    const Ref<Vulkan::Shader>&              shader,
	    const Ref<Vulkan::Texture2D>&           albedo,
	    const Ref<Vulkan::DescriptorSetLayout>& sceneLayout,
	    Vulkan::DescriptorAllocator&            allocator,
	    vk::Format                              colorFormat,
	    vk::Format                              depthFormat
	) :
	    m_Shader(shader), m_Albedo(albedo)
	{
		using namespace Vulkan;

		// set 1: albedo texture (sampled image) + its sampler.
		m_MaterialLayout = DescriptorSetLayout::Builder()
		                       .AddBinding(0, vk::DescriptorType::eSampledImage, vk::ShaderStageFlagBits::eFragment)
		                       .AddBinding(1, vk::DescriptorType::eSampler, vk::ShaderStageFlagBits::eFragment)
		                       .Build();

		m_DescriptorSet = allocator.Allocate(m_MaterialLayout->GetHandle());

		const vk::DescriptorImageInfo imageInfo{
		    .imageView = m_Albedo->GetView(), .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
		};
		const vk::DescriptorImageInfo samplerInfo{.sampler = m_Albedo->GetSampler()};

		const std::array writes = {
		    vk::WriteDescriptorSet{
		                           .dstSet          = m_DescriptorSet,
		                           .dstBinding      = 0,
		                           .descriptorCount = 1,
		                           .descriptorType  = vk::DescriptorType::eSampledImage,
		                           .pImageInfo      = &imageInfo  },
		    vk::WriteDescriptorSet{
		                           .dstSet          = m_DescriptorSet,
		                           .dstBinding      = 1,
		                           .descriptorCount = 1,
		                           .descriptorType  = vk::DescriptorType::eSampler,
		                           .pImageInfo      = &samplerInfo}
		};
		VulkanContext::GetVkDevice().updateDescriptorSets(writes, {});

		const GraphicsPipelineSpecification pipelineSpec{
		    .Program                = m_Shader,
		    .VertexInput            = Vertex::GetVertexInputDescription(),
		    .SetLayouts             = {sceneLayout->GetHandle(), m_MaterialLayout->GetHandle()},
		    .PushConstantRanges     = {vk::PushConstantRange{vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4)}},
		    .ColorAttachmentFormats = {colorFormat},
		    .DepthAttachmentFormat  = depthFormat
		};
		m_Pipeline = GraphicsPipeline::Create(pipelineSpec);
	}
}        // namespace Eruption
