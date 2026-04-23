#pragma once
#include "Eruption/Platform/Vulkan/Vulkan.h"

namespace Eruption::Vulkan
{
	class Instance;

	enum class QueueType
	{
		Graphics,
		Compute,
		Transfer,
		Present
	};

	class QueueFamilyIndices
	{
	public:
		static constexpr uint32_t INVALID = ~0;

	public:
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

			const auto add = [&unique](const uint32_t idx) {
				if (idx != INVALID)
					unique.insert(idx);
			};

			add(Graphics);
			add(Compute);
			add(Transfer);
			add(Present);

			return unique;
		}
	};

	class QueueFamiliesSelector
	{
	private:
		struct Candidate
		{
			uint32_t       Index;
			uint32_t       Count;
			vk::QueueFlags Flags;
			bool           CanPresent;
		};

	public:
		explicit QueueFamiliesSelector(const vk::raii::PhysicalDevice& device, const vk::raii::SurfaceKHR& surface) :
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
		std::vector<Candidate> m_Families;

		const vk::raii::PhysicalDevice& m_PhysicalDevice;
		const vk::raii::SurfaceKHR&     m_Surface;
	};

	class PhysicalDeviceSelector
	{
	private:
		struct Candidate
		{
			QueueFamilyIndices       QueueFamilyIndices;
			vk::raii::PhysicalDevice PhysicalDevice;
		};

	public:
		explicit PhysicalDeviceSelector(const Ref<Instance>& instance) : m_InstanceRef(instance) { ProcessDevices(); }

		[[nodiscard]] std::pair<vk::raii::PhysicalDevice, QueueFamilyIndices> Select() const;

	private:
		void ProcessDevices();

		[[nodiscard]] bool IsCandidateSuitable(const Candidate& candidate) const;

	private:
		[[nodiscard]] uint32_t ScoreDevice(const Candidate& candidate) const;

	private:
		const Ref<Instance>& m_InstanceRef;

		std::vector<Candidate> m_Candidates;
	};

	class PhysicalDevice
	{
	public:
		explicit PhysicalDevice(const Ref<Instance>& instance);
		~PhysicalDevice() = default;

		PhysicalDevice(const PhysicalDevice&)            = delete;
		PhysicalDevice& operator=(const PhysicalDevice&) = delete;

		PhysicalDevice(PhysicalDevice&&) noexcept            = default;
		PhysicalDevice& operator=(PhysicalDevice&&) noexcept = default;

		[[nodiscard]] bool IsExtensionSupported(const char* extension) const;

		[[nodiscard]] const vk::raii::PhysicalDevice& GetVkHandle() const { return m_PhysicalDevice; }

		[[nodiscard]] const vk::PhysicalDeviceProperties2&       GetProperties() const { return m_Properties; }
		[[nodiscard]] const vk::PhysicalDeviceMemoryProperties2& GetMemoryProperties() const
		{
			return m_MemoryProperties;
		}

		[[nodiscard]] vk::Format GetDepthFormat() const { return m_DepthFormat; }

		[[nodiscard]] const QueueFamilyIndices& GetQueueFamilyIndices() const { return m_QueueFamilyIndices; }
		[[nodiscard]] const std::vector<vk::DeviceQueueCreateInfo>& GetQueueCreateInfos() const
		{
			return m_QueueCreateInfos;
		}

	private:
		void SetupQueueCreateInfos();
		void FindDepthFormat();

	private:
		vk::raii::PhysicalDevice m_PhysicalDevice = nullptr;

		vk::PhysicalDeviceProperties2           m_Properties;
		vk::PhysicalDeviceFeatures2             m_Features;
		vk::PhysicalDeviceMemoryProperties2     m_MemoryProperties;
		std::vector<vk::QueueFamilyProperties2> m_SupportedQueueFamilies;
		std::vector<vk::ExtensionProperties>    m_SupportedExtensions;

		QueueFamilyIndices m_QueueFamilyIndices;

		std::vector<vk::DeviceQueueCreateInfo> m_QueueCreateInfos;

		vk::Format m_DepthFormat = vk::Format::eUndefined;
	};
}        // namespace Eruption::Vulkan