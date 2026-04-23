#include "Vulkan.h"

namespace Eruption::Vulkan::Utils
{
	void CheckResult(vk::Result result, const char* file, int line)
	{
		if (result != vk::Result::eSuccess)
		{
			ER_CORE_ERROR("vk::Result is '{0}' in {1}:{2}", vk::to_string(result), file, line);
			ER_CORE_ASSERT(false);
		}
	}

	void SetDebugUtilsObjectName(
	    const vk::raii::Device& device, vk::ObjectType objectType, const void* handle, std::string_view name
	)
	{
		const vk::DebugUtilsObjectNameInfoEXT debugNameInfo{
		    .objectType = objectType, .objectHandle = reinterpret_cast<uint64_t>(handle), .pObjectName = name.data()
		};

		device.setDebugUtilsObjectNameEXT(debugNameInfo);
	}
}        // namespace Eruption::Vulkan::Utils