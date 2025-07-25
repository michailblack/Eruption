#include "RendererContext.h"

#include "Eruption/Core/Assert.h"

#include "Eruption/Platform/Vulkan/VulkanContext.h"

#include "Eruption/Renderer/RendererAPI.h"

namespace Eruption
{
	Ref<RendererContext> RendererContext::Create()
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::Type::Vulkan: return CreateRef<VulkanContext>();
			case RendererAPI::Type::None:   break;
		}
		ER_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}        // namespace Eruption