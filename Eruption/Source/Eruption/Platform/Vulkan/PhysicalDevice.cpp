#include "PhysicalDevice.h"

#include "Eruption/Platform/Vulkan/Instance.h"

namespace Eruption::Vulkan
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
		const std::vector<vk::QueueFamilyProperties2> properties = m_PhysicalDevice.getQueueFamilyProperties2();
		m_Families.reserve(properties.size());

		for (uint32_t i = 0; i < properties.size(); ++i)
		{
			m_Families.push_back(
			    Candidate{
			        .Index      = i,
			        .Count      = properties[i].queueFamilyProperties.queueCount,
			        .Flags      = properties[i].queueFamilyProperties.queueFlags,
			        .CanPresent = static_cast<bool>(m_PhysicalDevice.getSurfaceSupportKHR(i, m_Surface))

			    }
			);
		}
	}

	uint32_t QueueFamiliesSelector::SelectGraphics() const
	{
		// Prefer graphics + present
		auto it = std::ranges::find_if(m_Families, [](const Candidate& family) {
			return family.CanPresent && (family.Flags & vk::QueueFlagBits::eGraphics);
		});

		if (it != m_Families.end())
			return it->Index;

		// Any graphics queue
		it = std::ranges::find_if(m_Families, [](const Candidate& family) {
			return static_cast<bool>(family.Flags & vk::QueueFlagBits::eGraphics);
		});

		if (it != m_Families.end())
			return it->Index;

		return QueueFamilyIndices::INVALID;
	}

	uint32_t QueueFamiliesSelector::SelectPresent() const
	{
		// Prefer graphics + present
		auto it = std::ranges::find_if(m_Families, [](const Candidate& family) {
			return family.CanPresent && (family.Flags & vk::QueueFlagBits::eGraphics);
		});

		if (it != m_Families.end())
			return it->Index;

		// Any present queue
		it = std::ranges::find_if(m_Families, [](const Candidate& f) { return f.CanPresent; });

		if (it != m_Families.end())
			return it->Index;

		return QueueFamilyIndices::INVALID;
	}

	uint32_t QueueFamiliesSelector::SelectCompute() const
	{
		// Dedicated compute (no graphics)
		const auto it = std::ranges::find_if(m_Families, [](const Candidate& family) {
			return (family.Flags & vk::QueueFlagBits::eCompute) && !(family.Flags & vk::QueueFlagBits::eGraphics);
		});

		if ((it != m_Families.end()))
			return it->Index;

		return SelectGraphics();
	}

	uint32_t QueueFamiliesSelector::SelectTransfer() const
	{
		// Dedicated transfer (no graphics/compute)
		const auto it = std::ranges::find_if(m_Families, [](const Candidate& family) {
			return (family.Flags & vk::QueueFlagBits::eTransfer) && !(family.Flags & vk::QueueFlagBits::eGraphics) &&
			       !(family.Flags & vk::QueueFlagBits::eCompute);
		});

		if (it != m_Families.end())
			return it->Index;

		// Prefer compute over graphics for transfer
		return SelectCompute();
	}

	std::pair<vk::raii::PhysicalDevice, QueueFamilyIndices> PhysicalDeviceSelector::Select() const
	{
		const auto& [queueFamilyIndices, physicalDevice] = m_Candidates.front();
		return {physicalDevice, queueFamilyIndices};
	}

	void PhysicalDeviceSelector::ProcessDevices()
	{
		const vk::raii::Instance& vkInstance = m_InstanceRef->GetVkHandle();

		std::vector<vk::raii::PhysicalDevice> physicalDevices = vkInstance.enumeratePhysicalDevices();
		ER_CORE_VERIFY(!physicalDevices.empty(), "No physical devices found!");

		m_Candidates.reserve(physicalDevices.size());

		for (vk::raii::PhysicalDevice& physicalDevice : physicalDevices)
		{
			QueueFamiliesSelector queueFamilySelector(physicalDevice, m_InstanceRef->GetSurfaceHandle());
			QueueFamilyIndices    queueFamilyIndices = queueFamilySelector.Select();

			m_Candidates.emplace_back(
			    Candidate{
			        .QueueFamilyIndices = std::move(queueFamilyIndices), .PhysicalDevice = std::move(physicalDevice)
			    }
			);
		}

		std::ranges::sort(m_Candidates, [this](const Candidate& a, const Candidate& b) {
			return ScoreDevice(a) > ScoreDevice(b);
		});
	}

	bool PhysicalDeviceSelector::IsCandidateSuitable(const Candidate& candidate) const
	{
		VkBool32 supported = VK_FALSE;
		VK_CHECK_RESULT(vpGetPhysicalDeviceProfileSupport(
		    m_InstanceRef->GetVpCapabilities(),
		    *m_InstanceRef->GetVkHandle(),
		    *candidate.PhysicalDevice,
		    &m_InstanceRef->GetVpProfileProperties(),
		    &supported
		));

		if (!supported)
			return false;

		if (!candidate.QueueFamilyIndices.IsComplete())
			return false;

		return true;
	}

	uint32_t PhysicalDeviceSelector::ScoreDevice(const Candidate& candidate) const
	{
		if (!IsCandidateSuitable(candidate))
			return ER_NOT_SUITABLE_DEVICE_SCORE;

		uint32_t score = ER_SUITABLE_DEVICE_BASE_SCORE;

		switch (const vk::PhysicalDeviceProperties2& properties = candidate.PhysicalDevice.getProperties2();
		        properties.properties.deviceType)
		{
			case vk::PhysicalDeviceType::eDiscreteGpu:   score += ER_DEVICE_TYPE_DISCRETE_SCORE; break;
			case vk::PhysicalDeviceType::eIntegratedGpu: score += ER_DEVICE_TYPE_INTEGRATED_SCORE; break;
			case vk::PhysicalDeviceType::eVirtualGpu:    score += ER_DEVICE_TYPE_VIRTUAL_SCORE; break;
			case vk::PhysicalDeviceType::eCpu:           score += ER_DEVICE_TYPE_CPU_SCORE; break;
			case vk::PhysicalDeviceType::eOther:         score += ER_DEVICE_TYPE_OTHER_SCORE; break;
		}

		if (candidate.QueueFamilyIndices.HasDedicatedCompute())
			score += ER_DEVICE_HAS_DEDICATED_COMPUTE_QUEUE_SCORE;

		if (candidate.QueueFamilyIndices.HasDedicatedTransfer())
			score += ER_DEVICE_HAS_DEDICATED_TRANSFER_QUEUE_SCORE;

		return score;
	}

	PhysicalDevice::PhysicalDevice(const Ref<Instance>& instance)
	{
		const PhysicalDeviceSelector selector(instance);
		const auto& [selectedPhysicalDevice, queueFamiliesIndices] = selector.Select();

		if (queueFamiliesIndices.Graphics != queueFamiliesIndices.Present)
			throw std::runtime_error("No queue family supports both graphics and present operations!");

		m_PhysicalDevice         = selectedPhysicalDevice;
		m_Properties             = selectedPhysicalDevice.getProperties2();
		m_Features               = selectedPhysicalDevice.getFeatures2();
		m_MemoryProperties       = selectedPhysicalDevice.getMemoryProperties2();
		m_SupportedQueueFamilies = selectedPhysicalDevice.getQueueFamilyProperties2();
		m_SupportedExtensions    = selectedPhysicalDevice.enumerateDeviceExtensionProperties();

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

	bool PhysicalDevice::IsExtensionSupported(const char* extension) const
	{
		return std::ranges::any_of(
		    m_SupportedExtensions, [extension](const vk::ExtensionProperties& extensionProperties) {
			    return strcmp(extension, extensionProperties.extensionName.data()) == 0;
		    }
		);
	}

	void PhysicalDevice::SetupQueueCreateInfos()
	{
		constexpr float DEFAULT_QUEUE_PRIORITY = 0.0f;
		for (const uint32_t queueFamilyIndex : m_QueueFamilyIndices.GetUniqueIndices())
		{
			const vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
			    .queueFamilyIndex = queueFamilyIndex, .queueCount = 1, .pQueuePriorities = &DEFAULT_QUEUE_PRIORITY
			};

			m_QueueCreateInfos.emplace_back(deviceQueueCreateInfo);
		}
	}

	void PhysicalDevice::FindDepthFormat()
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
}        // namespace Eruption::Vulkan