#pragma once
#include <vulkan/vulkan.hpp>

namespace Eruption::VulkanUtils
{
	void CheckResult(vk::Result result, const char* file, int line);
	void SetDebugUtilsObjectName(
	    vk::Device device, vk::ObjectType objectType, const void* handle, std::string_view name
	);
}        // namespace Eruption::VulkanUtils

#define VK_CHECK_RESULT(result)                                                                    \
	{                                                                                              \
		::Eruption::VulkanUtils::CheckResult(static_cast<vk::Result>(result), __FILE__, __LINE__); \
	}
