#include "RendererDevice.h"

namespace Eruption
{
	RendererDevice::RendererDevice(const Scope<Window>& window)
	{
		m_Instance       = CreateRef<Vulkan::Instance>(window);
		m_PhysicalDevice = CreateRef<Vulkan::PhysicalDevice>(m_Instance);
		m_Device         = CreateRef<Vulkan::Device>(m_Instance, m_PhysicalDevice);
	}
}        // namespace Eruption