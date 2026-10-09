#include "Vulkan.h"

namespace Eruption::Vulkan::Utils
{
	void CheckResult(vk::Result result, const char* file, int line)
	{
		if (result != vk::Result::eSuccess) [[unlikely]]
		{
			ER_CORE_ERROR("VkResult is '{0}' in {1}:{2}", vk::to_string(result), file, line);
			ER_CORE_ASSERT(false);
		}
	}

	void SetDebugUtilsObjectName(
	    const vk::raii::Device& device, vk::ObjectType objectType, uint64_t handle, std::string_view name
	)
	{
		const vk::DebugUtilsObjectNameInfoEXT nameInfo{
		    .objectType = objectType, .objectHandle = handle, .pObjectName = name.data()
		};
		device.setDebugUtilsObjectNameEXT(nameInfo);
	}

	void CmdImageBarrier(
	    const vk::raii::CommandBuffer&   cmd,
	    vk::Image                        image,
	    vk::PipelineStageFlags2          srcStage,
	    vk::AccessFlags2                 srcAccess,
	    vk::PipelineStageFlags2          dstStage,
	    vk::AccessFlags2                 dstAccess,
	    vk::ImageLayout                  oldLayout,
	    vk::ImageLayout                  newLayout,
	    const vk::ImageSubresourceRange& range
	)
	{
		const vk::ImageMemoryBarrier2 barrier{
		    .srcStageMask        = srcStage,
		    .srcAccessMask       = srcAccess,
		    .dstStageMask        = dstStage,
		    .dstAccessMask       = dstAccess,
		    .oldLayout           = oldLayout,
		    .newLayout           = newLayout,
		    .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
		    .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
		    .image               = image,
		    .subresourceRange    = range
		};

		cmd.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(barrier));
	}

	void CmdTransitionImage(
	    const vk::raii::CommandBuffer& cmd,
	    vk::Image                      image,
	    vk::ImageLayout                oldLayout,
	    vk::ImageLayout                newLayout,
	    vk::ImageAspectFlags           aspect,
	    uint32_t                       mipLevels,
	    uint32_t                       layerCount
	)
	{
		CmdImageBarrier(
		    cmd,
		    image,
		    vk::PipelineStageFlagBits2::eAllCommands,
		    vk::AccessFlagBits2::eMemoryWrite,
		    vk::PipelineStageFlagBits2::eAllCommands,
		    vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite,
		    oldLayout,
		    newLayout,
		    vk::ImageSubresourceRange{aspect, 0, mipLevels, 0, layerCount}
		);
	}
}        // namespace Eruption::Vulkan::Utils
