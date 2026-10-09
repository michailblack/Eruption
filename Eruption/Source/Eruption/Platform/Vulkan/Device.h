#pragma once
#include "Eruption/Platform/Vulkan/PhysicalDevice.h"
#include "Eruption/Platform/Vulkan/Vulkan.h"

#include <mutex>

namespace Eruption::Vulkan
{
	class Instance;

	class Device
	{
	public:
		Device(const Ref<Instance>& instance, Ref<PhysicalDevice> physicalDevice);
		~Device() = default;

		Device(const Device&)            = delete;
		Device& operator=(const Device&) = delete;

		[[nodiscard]] const vk::raii::Device&    GetVkHandle() const { return m_Device; }
		[[nodiscard]] const Ref<PhysicalDevice>& GetPhysicalDevice() const { return m_PhysicalDevice; }
		[[nodiscard]] const QueueFamilyIndices&  GetQueueFamilyIndices() const { return m_Indices; }

		[[nodiscard]] const vk::raii::Queue& GetQueue(QueueType type) const;
		[[nodiscard]] uint32_t               GetQueueFamilyIndex(QueueType type) const;

		void Submit(QueueType type, const vk::SubmitInfo2& submit, vk::Fence signalFence = nullptr) const;

		vk::Result Present(const vk::PresentInfoKHR& presentInfo) const;

		void WaitIdle() const { m_Device.waitIdle(); }

	private:
		[[nodiscard]] std::mutex& QueueMutex(QueueType type) const;

	private:
		Ref<PhysicalDevice> m_PhysicalDevice;

		vk::raii::Device m_Device = nullptr;

		vk::raii::Queue m_GraphicsQueue = nullptr;
		vk::raii::Queue m_ComputeQueue  = nullptr;
		vk::raii::Queue m_TransferQueue = nullptr;

		QueueFamilyIndices m_Indices;

		mutable std::mutex m_GraphicsMutex;
		mutable std::mutex m_ComputeMutex;
		mutable std::mutex m_TransferMutex;
	};
}        // namespace Eruption::Vulkan
