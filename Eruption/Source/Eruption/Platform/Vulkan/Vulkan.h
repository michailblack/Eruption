#pragma once
#include <vulkan/vulkan_raii.hpp>

#include <string_view>

namespace Eruption::Vulkan
{
	enum class QueueType : uint8_t
	{
		Graphics,
		Compute,
		Transfer,
		Present
	};

	namespace Utils
	{
		void CheckResult(vk::Result result, const char* file, int line);

		void SetDebugUtilsObjectName(
		    const vk::raii::Device& device, vk::ObjectType objectType, uint64_t handle, std::string_view name
		);

		template <typename THandle>
		void SetDebugUtilsObjectName(
		    const vk::raii::Device& device, vk::ObjectType objectType, THandle handle, std::string_view name
		)
		{
			SetDebugUtilsObjectName(
			    device, objectType, reinterpret_cast<uint64_t>(static_cast<typename THandle::NativeType>(handle)), name
			);
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
		);

		void CmdTransitionImage(
		    const vk::raii::CommandBuffer& cmd,
		    vk::Image                      image,
		    vk::ImageLayout                oldLayout,
		    vk::ImageLayout                newLayout,
		    vk::ImageAspectFlags           aspect     = vk::ImageAspectFlagBits::eColor,
		    uint32_t                       mipLevels  = 1,
		    uint32_t                       layerCount = 1
		);
	}        // namespace Utils
}        // namespace Eruption::Vulkan

#define VK_CHECK_RESULT(result) \
	::Eruption::Vulkan::Utils::CheckResult(static_cast<vk::Result>(result), __FILE__, __LINE__)
