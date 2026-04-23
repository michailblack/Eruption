#pragma once
#include "Eruption/Core/Base.h"

#include "Eruption/Platform/Vulkan/Device.h"
#include "Eruption/Platform/Vulkan/Instance.h"
#include "Eruption/Platform/Vulkan/SwapChain.h"

namespace Eruption
{
	class Window;

	class RendererDevice
	{
	public:
		explicit RendererDevice(const Scope<Window>& window);
		~RendererDevice() = default;

		[[nodiscard]] const Ref<Vulkan::Instance>&       GetInstance() const { return m_Instance; }
		[[nodiscard]] const Ref<Vulkan::PhysicalDevice>& GetPhysicalDevice() const { return m_PhysicalDevice; }
		[[nodiscard]] const Ref<Vulkan::Device>&         GetDevice() const { return m_Device; }

	private:
		Ref<Vulkan::Instance>       m_Instance;
		Ref<Vulkan::PhysicalDevice> m_PhysicalDevice;
		Ref<Vulkan::Device>         m_Device;
		Ref<Vulkan::SwapChain>      m_SwapChain;
	};
}        // namespace Eruption
