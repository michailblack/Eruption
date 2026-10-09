#pragma once
#include "Eruption/Platform/Vulkan/Vulkan.h"

#include <set>

namespace Eruption::Vulkan
{
	class Instance;

	class QueueFamilyIndices
	{
	public:
		static constexpr uint32_t INVALID = ~0u;

		uint32_t Graphics = INVALID;
		uint32_t Compute  = INVALID;
		uint32_t Transfer = INVALID;
		uint32_t Present  = INVALID;

		[[nodiscard]] bool IsComplete() const { return Graphics != INVALID && Present != INVALID; }
		[[nodiscard]] bool HasDedicatedCompute() const { return Compute != INVALID && Compute != Graphics; }
		[[nodiscard]] bool HasDedicatedTransfer() const
		{
			return Transfer != INVALID && Transfer != Graphics && Transfer != Compute;
		}

		[[nodiscard]] std::set<uint32_t> GetUniqueIndices() const
		{
			std::set<uint32_t> unique;
			for (const uint32_t idx : {Graphics, Compute, Transfer, Present})
			{
				if (idx != INVALID)
					unique.insert(idx);
			}
			return unique;
		}
	};

	class QueueFamiliesSelector
	{
	public:
		QueueFamiliesSelector(const vk::raii::PhysicalDevice& device, const vk::raii::SurfaceKHR& surface) :
		    m_PhysicalDevice(device), m_Surface(surface)
		{
			CacheFamilies();
		}

		[[nodiscard]] QueueFamilyIndices Select() const;

	private:
		void CacheFamilies();

		[[nodiscard]] uint32_t SelectGraphics() const;
		[[nodiscard]] uint32_t SelectPresent() const;
		[[nodiscard]] uint32_t SelectCompute() const;
		[[nodiscard]] uint32_t SelectTransfer() const;

	private:
		struct Candidate
		{
			uint32_t       Index;
			uint32_t       Count;
			vk::QueueFlags Flags;
			bool           CanPresent;
		};

		std::vector<Candidate> m_Families;

		const vk::raii::PhysicalDevice& m_PhysicalDevice;
		const vk::raii::SurfaceKHR&     m_Surface;
	};

	class PhysicalDeviceSelector
	{
	public:
		explicit PhysicalDeviceSelector(Ref<Instance> instance) : m_Instance(std::move(instance)) { ProcessDevices(); }

		[[nodiscard]] std::pair<vk::raii::PhysicalDevice, QueueFamilyIndices> Select() const;

	private:
		struct Candidate
		{
			QueueFamilyIndices       Indices;
			vk::raii::PhysicalDevice PhysicalDevice;
		};

		void ProcessDevices();

		[[nodiscard]] bool     IsCandidateSuitable(const Candidate& candidate) const;
		[[nodiscard]] uint32_t ScoreDevice(const Candidate& candidate) const;

		Ref<Instance>          m_Instance;
		std::vector<Candidate> m_Candidates;
	};

	class PhysicalDevice
	{
	public:
		explicit PhysicalDevice(const Ref<Instance>& instance);
		~PhysicalDevice() = default;

		PhysicalDevice(const PhysicalDevice&)            = delete;
		PhysicalDevice& operator=(const PhysicalDevice&) = delete;

		[[nodiscard]] bool IsExtensionSupported(std::string_view extension) const;

		[[nodiscard]] const vk::raii::PhysicalDevice& GetVkHandle() const { return m_PhysicalDevice; }

		[[nodiscard]] const vk::PhysicalDeviceProperties& GetProperties() const { return m_Properties.properties; }
		[[nodiscard]] const vk::PhysicalDeviceMemoryProperties& GetMemoryProperties() const
		{
			return m_MemoryProperties.memoryProperties;
		}

		[[nodiscard]] vk::Format              GetDepthFormat() const { return m_DepthFormat; }
		[[nodiscard]] vk::SampleCountFlagBits GetMaxUsableSampleCount() const { return m_MaxSampleCount; }

		[[nodiscard]] const QueueFamilyIndices& GetQueueFamilyIndices() const { return m_QueueFamilyIndices; }
		[[nodiscard]] const std::vector<vk::DeviceQueueCreateInfo>& GetQueueCreateInfos() const
		{
			return m_QueueCreateInfos;
		}

	private:
		void SetupQueueCreateInfos();
		void FindDepthFormat();
		void FindMaxSampleCount();

	private:
		vk::raii::PhysicalDevice m_PhysicalDevice = nullptr;

		vk::PhysicalDeviceProperties2        m_Properties;
		vk::PhysicalDeviceMemoryProperties2  m_MemoryProperties;
		std::vector<vk::ExtensionProperties> m_SupportedExtensions;

		QueueFamilyIndices m_QueueFamilyIndices;

		std::vector<float>                     m_QueuePriorities;
		std::vector<vk::DeviceQueueCreateInfo> m_QueueCreateInfos;

		vk::Format              m_DepthFormat    = vk::Format::eUndefined;
		vk::SampleCountFlagBits m_MaxSampleCount = vk::SampleCountFlagBits::e1;
	};
}        // namespace Eruption::Vulkan
