#include "Device.h"

#include "Eruption/Platform/Vulkan/Instance.h"
#include <vulkan/vulkan.hpp>

namespace Eruption::Vulkan
{
	Device::Device(const Ref<Instance>& instance, Ref<PhysicalDevice> physicalDevice) :
	    m_PhysicalDevice(std::move(physicalDevice)), m_Indices(m_PhysicalDevice->GetQueueFamilyIndices())
	{
		ER_CORE_INFO_TAG("Renderer", "Creating logical device");

		if (!m_PhysicalDevice->IsExtensionSupported(vk::KHRSwapchainExtensionName))
			throw std::runtime_error("Selected physical device does not support swapchains!");

		std::vector<const char*> deviceExtensions = {vk::KHRSwapchainExtensionName};

		vk::StructureChain<
		    vk::PhysicalDeviceFeatures2,
		    vk::PhysicalDeviceVulkan12Features,
		    vk::PhysicalDeviceVulkan13Features>
		    features;

		auto& features2             = features.get<vk::PhysicalDeviceFeatures2>().features;
		features2.samplerAnisotropy = vk::True;
		features2.fillModeNonSolid  = vk::True;

		auto& features12                                     = features.get<vk::PhysicalDeviceVulkan12Features>();
		features12.bufferDeviceAddress                       = vk::True;
		features12.descriptorIndexing                        = vk::True;
		features12.runtimeDescriptorArray                    = vk::True;
		features12.descriptorBindingPartiallyBound           = vk::True;
		features12.descriptorBindingVariableDescriptorCount  = vk::True;
		features12.shaderSampledImageArrayNonUniformIndexing = vk::True;
		features12.timelineSemaphore                         = vk::True;
		features12.scalarBlockLayout                         = vk::True;

		auto& features13            = features.get<vk::PhysicalDeviceVulkan13Features>();
		features13.dynamicRendering = vk::True;
		features13.synchronization2 = vk::True;

		const std::vector<vk::DeviceQueueCreateInfo>& queueCreateInfos = m_PhysicalDevice->GetQueueCreateInfos();

		const vk::DeviceCreateInfo deviceCreateInfo{
		    .pNext                   = &features.get<vk::PhysicalDeviceFeatures2>(),
		    .queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size()),
		    .pQueueCreateInfos       = queueCreateInfos.data(),
		    .enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size()),
		    .ppEnabledExtensionNames = deviceExtensions.data()
		};
		const VkDeviceCreateInfo rawDeviceCreateInfo = deviceCreateInfo;

		const VpDeviceCreateInfo vpDeviceCreateInfo{
		    .pCreateInfo             = &rawDeviceCreateInfo,
		    .enabledFullProfileCount = 1,
		    .pEnabledFullProfiles    = &Instance::GetProfile()
		};

		VkDevice rawDevice = VK_NULL_HANDLE;
		VK_CHECK_RESULT(vpCreateDevice(
		    instance->GetVpCapabilities(), *m_PhysicalDevice->GetVkHandle(), &vpDeviceCreateInfo, nullptr, &rawDevice
		));

		m_Device = vk::raii::Device(m_PhysicalDevice->GetVkHandle(), rawDevice);

		m_GraphicsQueue = vk::raii::Queue(m_Device, m_Indices.Graphics, 0);
		m_ComputeQueue  = vk::raii::Queue(m_Device, m_Indices.Compute, 0);
		m_TransferQueue = vk::raii::Queue(m_Device, m_Indices.Transfer, 0);
	}

	const vk::raii::Queue& Device::GetQueue(QueueType type) const
	{
		switch (type)
		{
			case QueueType::Graphics:
			case QueueType::Present:  return m_GraphicsQueue;
			case QueueType::Compute:  return m_ComputeQueue;
			case QueueType::Transfer: return m_TransferQueue;
		}
		ER_CORE_VERIFY(false, "Invalid queue type");
		return m_GraphicsQueue;
	}

	uint32_t Device::GetQueueFamilyIndex(QueueType type) const
	{
		switch (type)
		{
			case QueueType::Graphics: return m_Indices.Graphics;
			case QueueType::Present:  return m_Indices.Present;
			case QueueType::Compute:  return m_Indices.Compute;
			case QueueType::Transfer: return m_Indices.Transfer;
		}
		ER_CORE_VERIFY(false, "Invalid queue type");
		return QueueFamilyIndices::INVALID;
	}

	std::mutex& Device::QueueMutex(QueueType type) const
	{
		switch (type)
		{
			case QueueType::Graphics:
			case QueueType::Present:  return m_GraphicsMutex;
			case QueueType::Compute:  return m_ComputeMutex;
			case QueueType::Transfer: return m_TransferMutex;
		}
		ER_CORE_VERIFY(false, "Invalid queue type");
		return m_GraphicsMutex;
	}

	void Device::Submit(QueueType type, const vk::SubmitInfo2& submit, vk::Fence signalFence) const
	{
		const std::scoped_lock _(QueueMutex(type));
		GetQueue(type).submit2(submit, signalFence);
	}

	vk::Result Device::Present(const vk::PresentInfoKHR& presentInfo) const
	{
		const std::scoped_lock _(QueueMutex(QueueType::Present));
		try
		{
			return GetQueue(QueueType::Present).presentKHR(presentInfo);
		}
		catch (const vk::OutOfDateKHRError&)
		{
			return vk::Result::eErrorOutOfDateKHR;
		}
	}
}        // namespace Eruption::Vulkan
