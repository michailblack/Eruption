#include "Pipeline.h"

#include "Eruption/Platform/Vulkan/VulkanContext.h"

namespace Eruption::Vulkan
{
	GraphicsPipeline::GraphicsPipeline(const GraphicsPipelineSpecification& spec)
	{
		ER_CORE_ASSERT(spec.Program && spec.Program->IsValid(), "GraphicsPipeline requires a valid shader");

		const vk::raii::Device& device = VulkanContext::GetVkDevice();

		m_Layout = vk::raii::PipelineLayout(
		    device,
		    vk::PipelineLayoutCreateInfo{
		        .setLayoutCount         = static_cast<uint32_t>(spec.SetLayouts.size()),
		        .pSetLayouts            = spec.SetLayouts.data(),
		        .pushConstantRangeCount = static_cast<uint32_t>(spec.PushConstantRanges.size()),
		        .pPushConstantRanges    = spec.PushConstantRanges.data()
		    }
		);

		const std::vector<vk::PipelineShaderStageCreateInfo> stages = spec.Program->GetStageCreateInfos();

		const vk::PipelineVertexInputStateCreateInfo vertexInput{
		    .vertexBindingDescriptionCount   = static_cast<uint32_t>(spec.VertexInput.Bindings.size()),
		    .pVertexBindingDescriptions      = spec.VertexInput.Bindings.data(),
		    .vertexAttributeDescriptionCount = static_cast<uint32_t>(spec.VertexInput.Attributes.size()),
		    .pVertexAttributeDescriptions    = spec.VertexInput.Attributes.data()
		};

		const vk::PipelineInputAssemblyStateCreateInfo inputAssembly{.topology = spec.Topology};

		const vk::PipelineViewportStateCreateInfo viewportState{.viewportCount = 1, .scissorCount = 1};

		const vk::PipelineRasterizationStateCreateInfo rasterization{
		    .polygonMode = spec.PolygonMode, .cullMode = spec.CullMode, .frontFace = spec.FrontFace, .lineWidth = 1.0f
		};

		const vk::PipelineMultisampleStateCreateInfo multisample{.rasterizationSamples = vk::SampleCountFlagBits::e1};

		const vk::PipelineDepthStencilStateCreateInfo depthStencil{
		    .depthTestEnable  = spec.DepthTest ? vk::True : vk::False,
		    .depthWriteEnable = spec.DepthWrite ? vk::True : vk::False,
		    .depthCompareOp   = spec.DepthCompareOp
		};

		vk::PipelineColorBlendAttachmentState blendAttachment{
		    .blendEnable    = spec.Blend ? vk::True : vk::False,
		    .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
		                      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
		};
		if (spec.Blend)
		{
			blendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
			blendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
			blendAttachment.colorBlendOp        = vk::BlendOp::eAdd;
			blendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
			blendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
			blendAttachment.alphaBlendOp        = vk::BlendOp::eAdd;
		}

		const std::vector<vk::PipelineColorBlendAttachmentState> blendAttachments(
		    spec.ColorAttachmentFormats.size(), blendAttachment
		);
		const vk::PipelineColorBlendStateCreateInfo colorBlend{
		    .attachmentCount = static_cast<uint32_t>(blendAttachments.size()), .pAttachments = blendAttachments.data()
		};

		constexpr std::array dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
		const vk::PipelineDynamicStateCreateInfo dynamicState{
		    .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data()
		};

		const vk::PipelineRenderingCreateInfo renderingInfo{
		    .colorAttachmentCount    = static_cast<uint32_t>(spec.ColorAttachmentFormats.size()),
		    .pColorAttachmentFormats = spec.ColorAttachmentFormats.data(),
		    .depthAttachmentFormat   = spec.DepthAttachmentFormat
		};

		const vk::GraphicsPipelineCreateInfo pipelineInfo{
		    .pNext               = &renderingInfo,
		    .stageCount          = static_cast<uint32_t>(stages.size()),
		    .pStages             = stages.data(),
		    .pVertexInputState   = &vertexInput,
		    .pInputAssemblyState = &inputAssembly,
		    .pViewportState      = &viewportState,
		    .pRasterizationState = &rasterization,
		    .pMultisampleState   = &multisample,
		    .pDepthStencilState  = &depthStencil,
		    .pColorBlendState    = &colorBlend,
		    .pDynamicState       = &dynamicState,
		    .layout              = *m_Layout
		};

		m_Pipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);
	}

	Ref<GraphicsPipeline> GraphicsPipeline::Create(const GraphicsPipelineSpecification& spec)
	{
		return CreateRef<GraphicsPipeline>(spec);
	}
}        // namespace Eruption::Vulkan
