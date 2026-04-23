#include "Device.h"

#include "Eruption/Platform/Vulkan/Instance.h"
#include "Eruption/Platform/Vulkan/PhysicalDevice.h"

namespace Eruption::Vulkan
{
	Device::Device(const Ref<Instance>& instance, const Ref<PhysicalDevice>& physicalDevice)
	{
		const auto& queueCreateInfos   = physicalDevice->GetQueueCreateInfos();
		const auto& queueFamilyIndices = physicalDevice->GetQueueFamilyIndices();

		std::vector<const char*> requiredDeviceExtensions;

		if (!physicalDevice->IsExtensionSupported(vk::KHRSwapchainExtensionName))
			throw std::runtime_error("Selected physical device does not provide swap chain support");

		requiredDeviceExtensions.push_back(vk::KHRSwapchainExtensionName);

		const vk::StructureChain<
		    vk::PhysicalDeviceFeatures2,
		    vk::PhysicalDeviceVulkan13Features,
		    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
		    deviceFeaturesChain{{}, {.dynamicRendering = vk::True}, {.extendedDynamicState = vk::True}};

		const vk::DeviceCreateInfo deviceCreateInfo{
		    .pNext                   = deviceFeaturesChain.get<vk::PhysicalDeviceFeatures2>(),
		    .queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size()),
		    .pQueueCreateInfos       = queueCreateInfos.data(),
		    .enabledExtensionCount   = static_cast<uint32_t>(requiredDeviceExtensions.size()),
		    .ppEnabledExtensionNames = requiredDeviceExtensions.data()
		};
		const VkDeviceCreateInfo rawDeviceCreateInfo = deviceCreateInfo;

		const VpDeviceCreateInfo vpDeviceCreateInfo{
		    .pCreateInfo             = &rawDeviceCreateInfo,
		    .enabledFullProfileCount = 1,
		    .pEnabledFullProfiles    = &instance->GetVpProfileProperties()
		};

		VkDevice rawDevice = VK_NULL_HANDLE;
		VK_CHECK_RESULT(vpCreateDevice(
		    instance->GetVpCapabilities(), *physicalDevice->GetVkHandle(), &vpDeviceCreateInfo, nullptr, &rawDevice
		));

		m_Device = vk::raii::Device(physicalDevice->GetVkHandle(), rawDevice);

		m_GraphicsQueue = vk::raii::Queue(m_Device, queueFamilyIndices.Graphics, 0);
		m_ComputeQueue  = vk::raii::Queue(m_Device, queueFamilyIndices.Compute, 0);
		m_TransferQueue = vk::raii::Queue(m_Device, queueFamilyIndices.Transfer, 0);
	}

	// void Device::LockQueue(QueueType queueType)
	// {
	// 	switch (queueType)
	// 	{
	// 		case QueueType::Present:  [[fallthrough]];
	// 		case QueueType::Graphics: m_GraphicsQueueMutex.lock(); return;
	// 		case QueueType::Compute:  m_ComputeQueueMutex.lock(); return;
	// 		case QueueType::Transfer: m_TransferQueueMutex.lock(); return;
	// 	}
	//
	// 	ER_CORE_ASSERT(false, "Invalid queue type!");
	// }
	//
	// void Device::UnlockQueue(QueueType queueType)
	// {
	// 	switch (queueType)
	// 	{
	// 		case QueueType::Present:  [[fallthrough]];
	// 		case QueueType::Graphics: m_GraphicsQueueMutex.unlock(); return;
	// 		case QueueType::Compute:  m_ComputeQueueMutex.unlock(); return;
	// 		case QueueType::Transfer: m_TransferQueueMutex.unlock(); return;
	// 	}
	//
	// 	ER_CORE_ASSERT(false, "Invalid queue type!");
	// }
	//
	// vk::Queue Device::GetQueue(QueueType queueType) const
	// {
	// 	switch (queueType)
	// 	{
	// 		case QueueType::Graphics: return m_GraphicsQueue;
	// 		case QueueType::Compute:  return m_ComputeQueue;
	// 		case QueueType::Transfer: return m_TransferQueue;
	// 		case QueueType::Present:  return m_GraphicsQueue;
	// 	}
	//
	// 	ER_CORE_ASSERT(false, "Invalid queue type!");
	// 	return VK_NULL_HANDLE;
	// }
}        // namespace Eruption::Vulkan