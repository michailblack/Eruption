#pragma once
#include <vulkan/vulkan_raii.hpp>

namespace Eruption::Vulkan::Utils
{
	void CheckResult(vk::Result result, const char* file, int line);

	void SetDebugUtilsObjectName(
	    const vk::raii::Device& device, vk::ObjectType objectType, const void* handle, std::string_view name
	);
}        // namespace Eruption::Vulkan::Utils

#define VK_CHECK_RESULT(result)                                                                      \
	{                                                                                                \
		::Eruption::Vulkan::Utils::CheckResult(static_cast<vk::Result>(result), __FILE__, __LINE__); \
	}
