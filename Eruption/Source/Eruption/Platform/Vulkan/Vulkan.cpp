#include "Vulkan.h"

#include "Eruption/Platform/Vulkan/VulkanContext.h"

namespace Eruption::VulkanUtils
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
	    vk::Device device, vk::ObjectType objectType, const void* handle, std::string_view name
	)
	{
		vk::DebugUtilsObjectNameInfoEXT nameInfo{};
		nameInfo.objectType   = objectType;
		nameInfo.objectHandle = reinterpret_cast<uint64_t>(handle);
		nameInfo.pObjectName  = name.data();

		device.setDebugUtilsObjectNameEXT(nameInfo, *VulkanContext::Get()->GetDLD());
	}
}        // namespace Eruption::VulkanUtils