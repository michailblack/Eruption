#pragma once
#include "Eruption/Platform/Vulkan/Vulkan.h"

namespace Eruption::Vulkan
{
	class Instance;
	class PhysicalDevice;

	class Device
	{
	public:
		explicit Device(const Ref<Instance>& instance, const Ref<PhysicalDevice>& physicalDevice);
		~Device() = default;

		[[nodiscard]] const vk::raii::Device& GetVkHandle() const { return m_Device; }

	private:
		vk::raii::Device m_Device = nullptr;

		vk::raii::Queue m_GraphicsQueue = nullptr;
		vk::raii::Queue m_ComputeQueue = nullptr;
		vk::raii::Queue m_TransferQueue = nullptr;
	};
}        // namespace Eruption
