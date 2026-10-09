#pragma once
#include "Eruption/Platform/Vulkan/Shader.h"

namespace Eruption::Vulkan
{
	struct VertexInputDescription
	{
		std::vector<vk::VertexInputBindingDescription>   Bindings;
		std::vector<vk::VertexInputAttributeDescription> Attributes;
	};

	struct GraphicsPipelineSpecification
	{
		Ref<Shader>            Program;
		VertexInputDescription VertexInput;

		std::vector<vk::DescriptorSetLayout> SetLayouts;
		std::vector<vk::PushConstantRange>   PushConstantRanges;

		// Dynamic rendering: the formats this pipeline renders into.
		std::vector<vk::Format> ColorAttachmentFormats;
		vk::Format              DepthAttachmentFormat = vk::Format::eUndefined;

		vk::PrimitiveTopology Topology    = vk::PrimitiveTopology::eTriangleList;
		vk::PolygonMode       PolygonMode = vk::PolygonMode::eFill;
		vk::CullModeFlags     CullMode    = vk::CullModeFlagBits::eNone;
		vk::FrontFace         FrontFace   = vk::FrontFace::eCounterClockwise;

		bool          DepthTest      = true;
		bool          DepthWrite     = true;
		vk::CompareOp DepthCompareOp = vk::CompareOp::eLessOrEqual;

		bool Blend = false;
	};

	// A graphics pipeline configured for dynamic rendering (no VkRenderPass). Viewport and scissor are
	// dynamic and set per frame by the renderer.
	class GraphicsPipeline
	{
	public:
		explicit GraphicsPipeline(const GraphicsPipelineSpecification& spec);

		[[nodiscard]] vk::Pipeline       GetHandle() const { return *m_Pipeline; }
		[[nodiscard]] vk::PipelineLayout GetLayout() const { return *m_Layout; }

		void Bind(const vk::raii::CommandBuffer& cmd) const
		{
			cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_Pipeline);
		}

		[[nodiscard]] static Ref<GraphicsPipeline> Create(const GraphicsPipelineSpecification& spec);

	private:
		vk::raii::PipelineLayout m_Layout   = nullptr;
		vk::raii::Pipeline       m_Pipeline = nullptr;
	};
}        // namespace Eruption::Vulkan
