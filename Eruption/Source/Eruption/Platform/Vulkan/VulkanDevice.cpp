#include "VulkanDevice.h"

#include "Eruption/Platform/Vulkan/VulkanContext.h"

namespace Eruption
{
	namespace
	{
		enum PhysicalDeviceScore : uint32_t
		{
			ER_NOT_SUITABLE_DEVICE_SCORE = 0u,

			ER_SUITABLE_DEVICE_BASE_SCORE = 1u,

			ER_DEVICE_TYPE_DISCRETE_SCORE   = 5000u,
			ER_DEVICE_TYPE_INTEGRATED_SCORE = 1000u,
			ER_DEVICE_TYPE_VIRTUAL_SCORE    = 500u,
			ER_DEVICE_TYPE_CPU_SCORE        = 100u,
			ER_DEVICE_TYPE_OTHER_SCORE      = 50u,

			ER_DEVICE_HAS_DEDICATED_COMPUTE_QUEUE_SCORE  = 100u,
			ER_DEVICE_HAS_DEDICATED_TRANSFER_QUEUE_SCORE = 100u,
		};
	}

	/* ------------------------------------------------------------------------------------------------------- */
	/* ----------------------------------------- QueueFamilySelector ----------------------------------------- */
	/* ------------------------------------------------------------------------------------------------------- */
	QueueFamilyIndices QueueFamiliesSelector::Select() const
	{
		return {
		    .Graphics = SelectGraphics(),
		    .Compute  = SelectCompute(),
		    .Transfer = SelectTransfer(),
		    .Present  = SelectPresent()
		};
	}

	void QueueFamiliesSelector::CacheFamilies()
	{
		const std::vector<vk::QueueFamilyProperties> properties = m_Device.getQueueFamilyProperties();
		m_Families.reserve(properties.size());

		for (uint32_t i = 0; i < properties.size(); ++i)
		{
			m_Families.push_back(
			    Candidate{
			        .Index      = i,
			        .Count      = properties[i].queueCount,
			        .Flags      = properties[i].queueFlags,
			        .CanPresent = m_Surface ? static_cast<bool>(m_Device.getSurfaceSupportKHR(i, m_Surface)) : false
			    }
			);
		}
	}

	int32_t QueueFamiliesSelector::SelectGraphics() const
	{
		const auto it = std::ranges::find_if(m_Families, [](const Candidate& family) {
			return static_cast<bool>(family.Flags & vk::QueueFlagBits::eGraphics);
		});

		if (it != m_Families.end())
			return static_cast<int32_t>(it->Index);

		return QueueFamilyIndices::INVALID;
	}

	int32_t QueueFamiliesSelector::SelectPresent() const
	{
		// Prefer graphics + present
		auto it = std::ranges::find_if(m_Families, [](const Candidate& family) {
			return family.CanPresent && (family.Flags & vk::QueueFlagBits::eGraphics);
		});

		ER_CORE_VERIFY(it != m_Families.end(), "No queue family supports both graphics and present operations!");

		if (it != m_Families.end())
			return static_cast<int32_t>(it->Index);

		// Any present queue
		it = std::ranges::find_if(m_Families, [](const Candidate& f) { return f.CanPresent; });

		if (it != m_Families.end())
			return static_cast<int32_t>(it->Index);

		return QueueFamilyIndices::INVALID;
	}

	int32_t QueueFamiliesSelector::SelectCompute() const
	{
		// Dedicated compute (no graphics)
		const auto it = std::ranges::find_if(m_Families, [](const Candidate& family) {
			return (family.Flags & vk::QueueFlagBits::eCompute) && !(family.Flags & vk::QueueFlagBits::eGraphics);
		});

		if ((it != m_Families.end()))
			return static_cast<int32_t>(it->Index);

		return SelectGraphics();
	}

	int32_t QueueFamiliesSelector::SelectTransfer() const
	{
		// Dedicated transfer (no graphics/compute)
		const auto it = std::ranges::find_if(m_Families, [](const Candidate& family) {
			return (family.Flags & vk::QueueFlagBits::eTransfer) && !(family.Flags & vk::QueueFlagBits::eGraphics) &&
			       !(family.Flags & vk::QueueFlagBits::eCompute);
		});

		if (it != m_Families.end())
			return static_cast<int32_t>(it->Index);

		// Prefer compute over graphics for transfer
		return SelectCompute();
	}

	/* ------------------------------------------------------------------------------------------------------- */
	/* ----------------------------------------- PhysicalDeviceSelector -------------------------------------- */
	/* ------------------------------------------------------------------------------------------------------- */
	std::pair<vk::PhysicalDevice, QueueFamilyIndices> PhysicalDeviceSelector::Select() const
	{
		const auto it = std::ranges::find_if(m_Candidates, [](const Candidate& c) { return c.IsSuitable; });
		ER_CORE_ASSERT(it != m_Candidates.end(), "No suitable physical device found!");

		if (it == m_Candidates.end())
			return {it->Device, it->QueuesIndices};

		return {m_Candidates.front().Device, m_Candidates.front().QueuesIndices};
	}

	void PhysicalDeviceSelector::CacheDevices()
	{
		const auto vkInstance = VulkanContext::GetInstance();

		const std::vector<vk::PhysicalDevice> devices = vkInstance.enumeratePhysicalDevices();
		ER_CORE_VERIFY(!devices.empty(), "No physical devices found!");

		m_Candidates.reserve(devices.size());

		for (const auto& device : devices)
			m_Candidates.push_back(EvaluateDevice(device));

		std::ranges::sort(m_Candidates, [this](const Candidate& a, const Candidate& b) {
			return ScoreDevice(a) > ScoreDevice(b);
		});
	}

	PhysicalDeviceSelector::Candidate PhysicalDeviceSelector::EvaluateDevice(vk::PhysicalDevice device) const
	{
		Candidate candidate{
		    .Device           = device,
		    .Properties       = device.getProperties(),
		    .Features         = device.getFeatures(),
		    .MemoryProperties = device.getMemoryProperties(),
		    .QueuesProperties = device.getQueueFamilyProperties(),
		    .QueuesIndices    = {},
		    .IsSuitable       = false
		};

		if (candidate.Properties.apiVersion < VK_API_VERSION_1_4)
			return candidate;

		if (!CheckRequiredExtensions(device))
			return candidate;

		if (!CheckRequiredFeatures(candidate.Features))
			return candidate;

		const QueueFamiliesSelector queuesSelector(device, m_Requirements.Surface);
		candidate.QueuesIndices = queuesSelector.Select();

		if (!candidate.QueuesIndices.IsComplete())
			return candidate;

		candidate.IsSuitable = true;
		return candidate;
	}

	bool PhysicalDeviceSelector::CheckRequiredExtensions(vk::PhysicalDevice device) const
	{
		const std::vector deviceExtensions = device.enumerateDeviceExtensionProperties();

		std::set<std::string> deviceExtensionsSet;
		for (const vk::ExtensionProperties& ext : deviceExtensions)
			deviceExtensionsSet.insert(ext.extensionName);

		return std::ranges::all_of(m_Requirements.Extensions, [&deviceExtensionsSet](const char* requiredExtension) {
			return deviceExtensionsSet.contains(requiredExtension);
		});
	}

	bool PhysicalDeviceSelector::CheckRequiredFeatures(const vk::PhysicalDeviceFeatures2& available) const
	{
		const vk::PhysicalDeviceFeatures2& required = m_Requirements.Features;

		// vk::PhysicalDeviceFeatures is a struct of only vk::Bool32 members
		// We can safely treat it as an array of vk::Bool32
		const auto* requiredArray  = reinterpret_cast<const vk::Bool32*>(&required.features);
		const auto* availableArray = reinterpret_cast<const vk::Bool32*>(&available.features);

		constexpr size_t FEATURE_COUNT = sizeof(vk::PhysicalDeviceFeatures) / sizeof(vk::Bool32);

		for (size_t i = 0; i < FEATURE_COUNT; ++i)
		{
			if (requiredArray[i] && !availableArray[i])
				return false;
		}

		return true;
	}

	uint32_t PhysicalDeviceSelector::ScoreDevice(const Candidate& candidate)
	{
		if (!candidate.IsSuitable)
			return ER_NOT_SUITABLE_DEVICE_SCORE;

		uint32_t score = ER_SUITABLE_DEVICE_BASE_SCORE;

		switch (candidate.Properties.deviceType)
		{
			case vk::PhysicalDeviceType::eDiscreteGpu:   score += ER_DEVICE_TYPE_DISCRETE_SCORE; break;
			case vk::PhysicalDeviceType::eIntegratedGpu: score += ER_DEVICE_TYPE_INTEGRATED_SCORE; break;
			case vk::PhysicalDeviceType::eVirtualGpu:    score += ER_DEVICE_TYPE_VIRTUAL_SCORE; break;
			case vk::PhysicalDeviceType::eCpu:           score += ER_DEVICE_TYPE_CPU_SCORE; break;
			case vk::PhysicalDeviceType::eOther:         score += ER_DEVICE_TYPE_OTHER_SCORE; break;
		}

		if (candidate.QueuesIndices.HasDedicatedCompute())
			score += ER_DEVICE_HAS_DEDICATED_COMPUTE_QUEUE_SCORE;

		if (candidate.QueuesIndices.HasDedicatedTransfer())
			score += ER_DEVICE_HAS_DEDICATED_TRANSFER_QUEUE_SCORE;

		return score;
	}

	VulkanPhysicalDevice::VulkanPhysicalDevice(const PhysicalDeviceRequirements& requirements)
	{
		const PhysicalDeviceSelector selector(requirements);
		const auto& [selectedDevice, queueFamiliesIndices] = selector.Select();

		m_PhysicalDevice         = selectedDevice;
		m_Properties             = selectedDevice.getProperties2();
		m_Features               = selectedDevice.getFeatures2();
		m_MemoryProperties       = selectedDevice.getMemoryProperties2();
		m_SupportedQueueFamilies = selectedDevice.getQueueFamilyProperties2();
		m_SupportedExtensions    = selectedDevice.enumerateDeviceExtensionProperties();

		m_QueueFamilyIndices = queueFamiliesIndices;

		ER_CORE_INFO_TAG("Renderer", "Selected GPU:");
		ER_CORE_INFO_TAG("Renderer", "\tName: {0}", m_Properties.properties.deviceName.data());
		ER_CORE_INFO_TAG("Renderer", "\tDevice Type: {0}", vk::to_string(m_Properties.properties.deviceType));
		ER_CORE_INFO_TAG(
		    "Renderer",
		    "\tDriver Version: {0}.{1}.{2}",
		    vk::versionMajor(m_Properties.properties.driverVersion),
		    vk::versionMinor(m_Properties.properties.driverVersion),
		    vk::versionPatch(m_Properties.properties.driverVersion)
		);
		ER_CORE_INFO_TAG(
		    "Renderer",
		    "\tVulkan Version: {0}.{1}.{2}",
		    vk::apiVersionMajor(m_Properties.properties.apiVersion),
		    vk::apiVersionMinor(m_Properties.properties.apiVersion),
		    vk::apiVersionPatch(m_Properties.properties.apiVersion)
		);

		SetupQueueCreateInfos();

		FindDepthFormat();
	}

	bool VulkanPhysicalDevice::IsExtensionSupported(const char* extension) const
	{
		return std::ranges::any_of(
		    m_SupportedExtensions,
		    [extension](const vk::ExtensionProperties& extensionProperties) {
			    return strcmp(extension, extensionProperties.extensionName.data()) == 0;
		    }
		);
	}

	void VulkanPhysicalDevice::SetupQueueCreateInfos()
	{
		constexpr float DEFAULT_QUEUE_PRIORITY = 0.0f;
		for (const uint32_t queueFamilyIndex : m_QueueFamilyIndices.GetUniqueIndices())
		{
			vk::DeviceQueueCreateInfo deviceQueueCreateInfo{};
			deviceQueueCreateInfo.queueFamilyIndex = queueFamilyIndex;
			deviceQueueCreateInfo.queueCount       = 1u;
			deviceQueueCreateInfo.pQueuePriorities = &DEFAULT_QUEUE_PRIORITY;
			m_QueueCreateInfos.emplace_back(deviceQueueCreateInfo);
		}
	}

	void VulkanPhysicalDevice::FindDepthFormat()
	{
		constexpr std::array DEPTH_FORMATS_BY_PRIORITY = {
		    vk::Format::eD32SfloatS8Uint,
		    vk::Format::eD32Sfloat,
		    vk::Format::eD24UnormS8Uint,
		    vk::Format::eD16UnormS8Uint,
		    vk::Format::eD16Unorm
		};

		m_DepthFormat = vk::Format::eUndefined;
		for (const auto format : DEPTH_FORMATS_BY_PRIORITY)
		{
			const vk::FormatProperties2 properties = m_PhysicalDevice.getFormatProperties2(format);
			if (properties.formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
			{
				m_DepthFormat = format;
				break;
			}
		}

		ER_CORE_ASSERT(m_DepthFormat != vk::Format::eUndefined, "No suitable depth format is found!");
	}

	/* ------------------------------------------------------------------------------------------------------- */
	/* ----------------------------------------- VulkanCommandPool ------------------------------------------- */
	/* ------------------------------------------------------------------------------------------------------- */
	VulkanCommandPool::VulkanCommandPool()
	{
		const Ref<VulkanDevice>& device       = VulkanContext::GetCurrentDevice();
		const vk::Device         vulkanDevice = device->GetVulkanDevice();

		const QueueFamilyIndices& familyIndices = device->GetPhysicalDevice()->GetQueueFamilyIndices();

		m_GraphicsCommandPool = CreateCommandPool(familyIndices.Graphics);
		m_ComputeCommandPool  = CreateCommandPool(familyIndices.Compute);
		m_TransferCommandPool = CreateCommandPool(familyIndices.Transfer);
	}

	VulkanCommandPool::~VulkanCommandPool()
	{
		const vk::Device vulkanDevice = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		vulkanDevice.destroyCommandPool(m_GraphicsCommandPool);
		vulkanDevice.destroyCommandPool(m_ComputeCommandPool);
		vulkanDevice.destroyCommandPool(m_TransferCommandPool);
	}

	vk::CommandBuffer VulkanCommandPool::AllocateCommandBuffer(QueueType queueType, bool begin) const
	{
		const vk::Device vulkanDevice = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		const vk::CommandBufferAllocateInfo commandBufferAllocateInfo(
		    GetCommandPool(queueType), vk::CommandBufferLevel::ePrimary, 1u
		);

		const vk::CommandBuffer commandBuffer = vulkanDevice.allocateCommandBuffers(commandBufferAllocateInfo).front();

		if (begin)
			commandBuffer.begin(vk::CommandBufferBeginInfo{});

		return commandBuffer;
	}

	void VulkanCommandPool::FreeCommandBuffers(
	    QueueType queueType, const std::initializer_list<vk::CommandBuffer>& commandBuffers
	) const
	{
		const vk::Device vulkanDevice = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		vulkanDevice.freeCommandBuffers(GetCommandPool(queueType), commandBuffers);
	}

	vk::CommandPool VulkanCommandPool::GetCommandPool(QueueType queueType) const
	{
		switch (queueType)
		{
			case QueueType::Graphics: return m_GraphicsCommandPool;
			case QueueType::Compute:  return m_ComputeCommandPool;
			case QueueType::Transfer: return m_TransferCommandPool;
			case QueueType::Present:  return m_GraphicsCommandPool;
		}

		ER_CORE_ASSERT(false, "Invalid queue type!");
		return VK_NULL_HANDLE;
	}

	vk::CommandPool VulkanCommandPool::CreateCommandPool(uint32_t queueFamilyIndex)
	{
		const vk::Device vulkanDevice = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		const vk::CommandPoolCreateInfo commandPoolCreateInfo(
		    vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queueFamilyIndex
		);

		return vulkanDevice.createCommandPool(commandPoolCreateInfo);
	}

	VulkanDevice::VulkanDevice(const Ref<VulkanPhysicalDevice>& physicalDevice) : m_PhysicalDevice(physicalDevice)
	{
		std::vector<const char*> deviceExtensions;

		ER_CORE_ASSERT(m_PhysicalDevice->IsExtensionSupported(vk::KHRSwapchainExtensionName));
		deviceExtensions.push_back(vk::KHRSwapchainExtensionName);

		vk::DeviceCreateInfo deviceCreateInfo{};
		deviceCreateInfo.setQueueCreateInfos(m_PhysicalDevice->m_QueueCreateInfos);
		deviceCreateInfo.setPEnabledExtensionNames(deviceExtensions);

		vk::StructureChain deviceCreateChain{deviceCreateInfo, m_PhysicalDevice->m_Features};

		m_LogicalDevice = m_PhysicalDevice->GetVulkanPhysicalDevice().createDevice(
		    deviceCreateChain.get<vk::DeviceCreateInfo>()
		);

		m_GraphicsQueue = m_LogicalDevice.getQueue(m_PhysicalDevice->m_QueueFamilyIndices.Graphics, 0u);
		m_ComputeQueue  = m_LogicalDevice.getQueue(m_PhysicalDevice->m_QueueFamilyIndices.Compute, 0u);
		m_TransferQueue = m_LogicalDevice.getQueue(m_PhysicalDevice->m_QueueFamilyIndices.Transfer, 0u);
	}

	void VulkanDevice::Destroy() const
	{
		m_LogicalDevice.waitIdle();
		m_LogicalDevice.destroy();
	}

	void VulkanDevice::LockQueue(QueueType queueType)
	{
		switch (queueType)
		{
			case QueueType::Present:  [[fallthrough]];
			case QueueType::Graphics: m_GraphicsQueueMutex.lock(); return;
			case QueueType::Compute:  m_ComputeQueueMutex.lock(); return;
			case QueueType::Transfer: m_TransferQueueMutex.lock(); return;
		}

		ER_CORE_ASSERT(false, "Invalid queue type!");
	}

	void VulkanDevice::UnlockQueue(QueueType queueType)
	{
		switch (queueType)
		{
			case QueueType::Present:  [[fallthrough]];
			case QueueType::Graphics: m_GraphicsQueueMutex.unlock(); return;
			case QueueType::Compute:  m_ComputeQueueMutex.unlock(); return;
			case QueueType::Transfer: m_TransferQueueMutex.unlock(); return;
		}

		ER_CORE_ASSERT(false, "Invalid queue type!");
	}

	vk::CommandBuffer VulkanDevice::BeginSingleTimeCommands(QueueType queueType)
	{
		return GetOrCreateThreadLocalCommandPool()->AllocateCommandBuffer(queueType, true);
	}

	void VulkanDevice::EndSingleTimeCommands(vk::CommandBuffer commandBuffer, QueueType queueType)
	{
		const vk::Device vulkanDevice = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		ER_CORE_ASSERT(commandBuffer != VK_NULL_HANDLE, "Invalid command buffer!");
		commandBuffer.end();

		vk::SubmitInfo submitInfo{};
		submitInfo.setCommandBuffers(commandBuffer);

		const vk::Fence commandBufferFinishedFence = vulkanDevice.createFence(vk::FenceCreateInfo{});

		{
			LockQueue();
			GetQueue(queueType).submit(submitInfo, commandBufferFinishedFence);
			UnlockQueue();
		}

		VK_CHECK_RESULT(
		    vulkanDevice.waitForFences(commandBufferFinishedFence, vk::True, std::numeric_limits<uint64_t>::max())
		);

		vulkanDevice.destroyFence(commandBufferFinishedFence);
		vulkanDevice.freeCommandBuffers(GetThreadLocalCommandPool()->GetCommandPool(queueType), commandBuffer);
	}

	vk::Queue VulkanDevice::GetQueue(QueueType queueType) const
	{
		switch (queueType)
		{
			case QueueType::Graphics: return m_GraphicsQueue;
			case QueueType::Compute:  return m_ComputeQueue;
			case QueueType::Transfer: return m_TransferQueue;
			case QueueType::Present:  return m_GraphicsQueue;
		}

		ER_CORE_ASSERT(false, "Invalid queue type!");
		return VK_NULL_HANDLE;
	}

	Ref<VulkanCommandPool> VulkanDevice::GetThreadLocalCommandPool()
	{
		const auto threadID = std::this_thread::get_id();
		ER_CORE_VERIFY(m_CommandPools.contains(threadID));

		return m_CommandPools.at(threadID);
	}

	Ref<VulkanCommandPool> VulkanDevice::GetOrCreateThreadLocalCommandPool()
	{
		const auto threadID = std::this_thread::get_id();

		const auto commandPoolIt = m_CommandPools.find(threadID);
		if (commandPoolIt != m_CommandPools.end())
			return commandPoolIt->second;

		const auto commandPool   = CreateRef<VulkanCommandPool>();
		m_CommandPools[threadID] = commandPool;

		return commandPool;
	}
}        // namespace Eruption