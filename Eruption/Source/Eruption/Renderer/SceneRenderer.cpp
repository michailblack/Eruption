#include "SceneRenderer.h"

#include "Eruption/Platform/Vulkan/VulkanContext.h"
#include "Eruption/Renderer/Renderer.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Eruption
{
	SceneRenderer::SceneRenderer()
	{
		using namespace Vulkan;

		// set 0: per-frame scene data, visible to both stages.
		m_SceneLayout = DescriptorSetLayout::Builder()
		                    .AddBinding(
		                        0,
		                        vk::DescriptorType::eUniformBuffer,
		                        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment
		                    )
		                    .Build();

		const uint32_t framesInFlight = Renderer::GetFramesInFlight();
		m_SceneUniformBuffers.reserve(framesInFlight);
		m_SceneDescriptorSets.reserve(framesInFlight);

		for (uint32_t i = 0; i < framesInFlight; ++i)
		{
			Ref<Buffer> ubo = Buffer::CreateUniform(sizeof(GPUSceneData), std::format("SceneUBO[{}]", i));

			const vk::DescriptorSet set = m_Allocator.Allocate(m_SceneLayout->GetHandle());

			const vk::DescriptorBufferInfo bufferInfo{
			    .buffer = ubo->GetHandle(), .offset = 0, .range = sizeof(GPUSceneData)
			};
			const vk::WriteDescriptorSet write{
			    .dstSet          = set,
			    .dstBinding      = 0,
			    .descriptorCount = 1,
			    .descriptorType  = vk::DescriptorType::eUniformBuffer,
			    .pBufferInfo     = &bufferInfo
			};
			VulkanContext::GetVkDevice().updateDescriptorSets(write, {});

			m_SceneUniformBuffers.push_back(std::move(ubo));
			m_SceneDescriptorSets.push_back(set);
		}
	}

	Ref<Material> SceneRenderer::CreateMaterial(const Ref<Vulkan::Shader>& shader, const Ref<Vulkan::Texture2D>& albedo)
	{
		return CreateRef<Material>(
		    shader, albedo, m_SceneLayout, m_Allocator, Renderer::GetSwapChainFormat(), Renderer::GetDepthFormat()
		);
	}

	void SceneRenderer::BeginScene(const Camera& camera, const DirectionalLight& light)
	{
		const GPUSceneData data{
		    .ViewProjection = camera.GetViewProjection(),
		    .CameraPosition = glm::vec4(camera.GetPosition(), 1.0f),
		    .LightDirection = glm::vec4(glm::normalize(light.Direction), 0.0f),
		    .LightColor     = glm::vec4(light.Color, light.Intensity)
		};

		// Upload happens on the render thread for the frame's slot, before any draw in this frame.
		Renderer::Submit([this, data](const vk::raii::CommandBuffer&) {
			const uint32_t frame = Renderer::GetCurrentFrameIndex();
			m_SceneUniformBuffers[frame]->SetData(&data, sizeof(data));
		});
	}

	void SceneRenderer::SubmitMesh(const Ref<Mesh>& mesh, const Ref<Material>& material, const glm::mat4& transform)
	{
		Renderer::Submit([this, mesh, material, transform](const vk::raii::CommandBuffer& cmd) {
			const uint32_t                       frame    = Renderer::GetCurrentFrameIndex();
			const Ref<Vulkan::GraphicsPipeline>& pipeline = material->GetPipeline();

			pipeline->Bind(cmd);

			const std::array descriptorSets = {m_SceneDescriptorSets[frame], material->GetDescriptorSet()};
			cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline->GetLayout(), 0, descriptorSets, {});

			cmd.pushConstants<glm::mat4>(pipeline->GetLayout(), vk::ShaderStageFlagBits::eVertex, 0, transform);

			mesh->Bind(cmd);
			mesh->Draw(cmd);
		});
	}

	void SceneRenderer::EndScene()
	{
		// Draw commands are recorded during BeginScene/SubmitMesh; the renderer submits them on EndFrame.
	}
}        // namespace Eruption
