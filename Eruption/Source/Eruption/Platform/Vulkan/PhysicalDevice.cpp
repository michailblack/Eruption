#include "PhysicalDevice.h"

#include "Eruption/Platform/Vulkan/Instance.h"

namespace Eruption::Vulkan
{
	namespace
	{
		enum PhysicalDeviceScore : uint32_t
		{
			ER_NOT_SUITABLE_DEVICE_SCORE  = 0u,
			ER_SUITABLE_DEVICE_BASE_SCORE = 1u,

			ER_DEVICE_TYPE_DISCRETE_SCORE   = 5000u,
			ER_DEVICE_TYPE_INTEGRATED_SCORE = 1000u,
			ER_DEVICE_TYPE_VIRTUAL_SCORE    = 500u,
			ER_DEVICE_TYPE_CPU_SCORE        = 100u,
			ER_DEVICE_TYPE_OTHER_SCORE      = 50u,

			ER_DEVICE_HAS_DEDICATED_COMPUTE_QUEUE_SCORE  = 100u,
			ER_DEVICE_HAS_DEDICATED_TRANSFER_QUEUE_SCORE = 100u,
		};
	}        // namespace

	/* --------------------------------------------- Queue selection --------------------------------------------- */

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
		// Prefer a family that supports both graphics and present.
		auto it = std::ranges::find_if(m_Families, [](const Candidate& f) {
			return f.CanPresent && (f.Flags & vk::QueueFlagBits::eGraphics);
		});
		if (it != m_Families.end())
			return it->Index;

		it = std::ranges::find_if(m_Families, [](const Candidate& f) {
			return static_cast<bool>(f.Flags & vk::QueueFlagBits::eGraphics);
		});
		return it != m_Families.end() ? it->Index : QueueFamilyIndices::INVALID;
	}

	uint32_t QueueFamiliesSelector::SelectPresent() const
	{
		auto it = std::ranges::find_if(m_Families, [](const Candidate& f) {
			return f.CanPresent && (f.Flags & vk::QueueFlagBits::eGraphics);
		});
		if (it != m_Families.end())
			return it->Index;

		it = std::ranges::find_if(m_Families, [](const Candidate& f) { return f.CanPresent; });
		return it != m_Families.end() ? it->Index : QueueFamilyIndices::INVALID;
	}

	uint32_t QueueFamiliesSelector::SelectCompute() const
	{
		// Dedicated compute (no graphics) for async compute.
		const auto it = std::ranges::find_if(m_Families, [](const Candidate& f) {
			return (f.Flags & vk::QueueFlagBits::eCompute) && !(f.Flags & vk::QueueFlagBits::eGraphics);
		});
		return it != m_Families.end() ? it->Index : SelectGraphics();
	}

	uint32_t QueueFamiliesSelector::SelectTransfer() const
	{
		// Dedicated transfer (no graphics/compute) for async uploads.
		const auto it = std::ranges::find_if(m_Families, [](const Candidate& f) {
			return (f.Flags & vk::QueueFlagBits::eTransfer) && !(f.Flags & vk::QueueFlagBits::eGraphics) &&
			       !(f.Flags & vk::QueueFlagBits::eCompute);
		});
		return it != m_Families.end() ? it->Index : SelectCompute();
	}

	/* -------------------------------------------- Device selection --------------------------------------------- */

	std::pair<vk::raii::PhysicalDevice, QueueFamilyIndices> PhysicalDeviceSelector::Select() const
	{
		ER_CORE_VERIFY(!m_Candidates.empty(), "No suitable physical device found!");
		const auto& [indices, physicalDevice] = m_Candidates.front();
		return {physicalDevice, indices};
	}

	void PhysicalDeviceSelector::ProcessDevices()
	{
		std::vector<vk::raii::PhysicalDevice> physicalDevices = m_Instance->GetVkHandle().enumeratePhysicalDevices();
		ER_CORE_VERIFY(!physicalDevices.empty(), "No Vulkan physical devices found!");

		m_Candidates.reserve(physicalDevices.size());
		for (vk::raii::PhysicalDevice& physicalDevice : physicalDevices)
		{
			const QueueFamiliesSelector queueFamilySelector(physicalDevice, m_Instance->GetSurface());
			m_Candidates.emplace_back(
			    Candidate{.Indices = queueFamilySelector.Select(), .PhysicalDevice = std::move(physicalDevice)}
			);
		}

		std::ranges::sort(m_Candidates, [this](const Candidate& a, const Candidate& b) {
			return ScoreDevice(a) > ScoreDevice(b);
		});

		// Drop everything that isn't actually usable so Select() never returns an unsuitable device.
		std::erase_if(m_Candidates, [this](const Candidate& c) {
			return ScoreDevice(c) == ER_NOT_SUITABLE_DEVICE_SCORE;
		});
	}

	bool PhysicalDeviceSelector::IsCandidateSuitable(const Candidate& candidate) const
	{
		if (!candidate.Indices.IsComplete())
			return false;

		VkBool32 supported = VK_FALSE;
		VK_CHECK_RESULT(vpGetPhysicalDeviceProfileSupport(
		    m_Instance->GetVpCapabilities(),
		    *m_Instance->GetVkHandle(),
		    *candidate.PhysicalDevice,
		    &Instance::GetProfile(),
		    &supported
		));

		return supported == VK_TRUE;
	}

	uint32_t PhysicalDeviceSelector::ScoreDevice(const Candidate& candidate) const
	{
		if (!IsCandidateSuitable(candidate))
			return ER_NOT_SUITABLE_DEVICE_SCORE;

		uint32_t score = ER_SUITABLE_DEVICE_BASE_SCORE;

		switch (candidate.PhysicalDevice.getProperties().deviceType)
		{
			case vk::PhysicalDeviceType::eDiscreteGpu:   score += ER_DEVICE_TYPE_DISCRETE_SCORE; break;
			case vk::PhysicalDeviceType::eIntegratedGpu: score += ER_DEVICE_TYPE_INTEGRATED_SCORE; break;
			case vk::PhysicalDeviceType::eVirtualGpu:    score += ER_DEVICE_TYPE_VIRTUAL_SCORE; break;
			case vk::PhysicalDeviceType::eCpu:           score += ER_DEVICE_TYPE_CPU_SCORE; break;
			default:                                     score += ER_DEVICE_TYPE_OTHER_SCORE; break;
		}

		if (candidate.Indices.HasDedicatedCompute())
			score += ER_DEVICE_HAS_DEDICATED_COMPUTE_QUEUE_SCORE;
		if (candidate.Indices.HasDedicatedTransfer())
			score += ER_DEVICE_HAS_DEDICATED_TRANSFER_QUEUE_SCORE;

		return score;
	}

	/* ---------------------------------------------- PhysicalDevice --------------------------------------------- */

	PhysicalDevice::PhysicalDevice(const Ref<Instance>& instance)
	{
		const PhysicalDeviceSelector selector(instance);
		auto [selectedPhysicalDevice, queueFamilyIndices] = selector.Select();

		m_PhysicalDevice      = std::move(selectedPhysicalDevice);
		m_QueueFamilyIndices  = queueFamilyIndices;
		m_Properties          = m_PhysicalDevice.getProperties2();
		m_MemoryProperties    = m_PhysicalDevice.getMemoryProperties2();
		m_SupportedExtensions = m_PhysicalDevice.enumerateDeviceExtensionProperties();

		const vk::PhysicalDeviceProperties& props = m_Properties.properties;
		ER_CORE_INFO_TAG("Renderer", "Selected GPU: {}", props.deviceName.data());
		ER_CORE_INFO_TAG("Renderer", "\tType: {}", vk::to_string(props.deviceType));
		ER_CORE_INFO_TAG(
		    "Renderer",
		    "\tDriver: {}.{}.{}",
		    vk::apiVersionMajor(props.driverVersion),
		    vk::apiVersionMinor(props.driverVersion),
		    vk::apiVersionPatch(props.driverVersion)
		);
		ER_CORE_INFO_TAG(
		    "Renderer",
		    "\tVulkan: {}.{}.{}",
		    vk::apiVersionMajor(props.apiVersion),
		    vk::apiVersionMinor(props.apiVersion),
		    vk::apiVersionPatch(props.apiVersion)
		);

		SetupQueueCreateInfos();
		FindDepthFormat();
		FindMaxSampleCount();
	}

	bool PhysicalDevice::IsExtensionSupported(std::string_view extension) const
	{
		return std::ranges::any_of(m_SupportedExtensions, [extension](const vk::ExtensionProperties& e) {
			return extension == e.extensionName;
		});
	}

	void PhysicalDevice::SetupQueueCreateInfos()
	{
		const std::set<uint32_t> uniqueFamilies = m_QueueFamilyIndices.GetUniqueIndices();

		m_QueuePriorities.assign(uniqueFamilies.size(), 1.0f);
		m_QueueCreateInfos.reserve(uniqueFamilies.size());

		uint32_t i = 0;
		for (const uint32_t queueFamilyIndex : uniqueFamilies)
		{
			m_QueueCreateInfos.push_back(
			    vk::DeviceQueueCreateInfo{
			        .queueFamilyIndex = queueFamilyIndex, .queueCount = 1, .pQueuePriorities = &m_QueuePriorities[i]
			    }
			);
			++i;
		}
	}

	void PhysicalDevice::FindDepthFormat()
	{
		constexpr std::array DEPTH_FORMATS_BY_PRIORITY = {
		    vk::Format::eD32Sfloat,
		    vk::Format::eD32SfloatS8Uint,
		    vk::Format::eD24UnormS8Uint,
		    vk::Format::eD16UnormS8Uint,
		    vk::Format::eD16Unorm
		};

		for (const vk::Format format : DEPTH_FORMATS_BY_PRIORITY)
		{
			const vk::FormatProperties props = m_PhysicalDevice.getFormatProperties(format);
			if (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
			{
				m_DepthFormat = format;
				break;
			}
		}

		ER_CORE_ASSERT(m_DepthFormat != vk::Format::eUndefined, "No suitable depth format found!");
	}

	void PhysicalDevice::FindMaxSampleCount()
	{
		const vk::SampleCountFlags counts = m_Properties.properties.limits.framebufferColorSampleCounts &
		                                    m_Properties.properties.limits.framebufferDepthSampleCounts;

		for (const vk::SampleCountFlagBits bit :
		     {vk::SampleCountFlagBits::e8, vk::SampleCountFlagBits::e4, vk::SampleCountFlagBits::e2})
		{
			if (counts & bit)
			{
				m_MaxSampleCount = bit;
				return;
			}
		}

		m_MaxSampleCount = vk::SampleCountFlagBits::e1;
	}
}        // namespace Eruption::Vulkan
