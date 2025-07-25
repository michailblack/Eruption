#pragma once
#include <utility>

#include "Eruption/Platform/Vulkan/Vulkan.h"

namespace Eruption
{
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
		static constexpr int32_t INVALID = -1;

	public:
		int32_t Graphics = INVALID;
		int32_t Compute  = INVALID;
		int32_t Transfer = INVALID;
		int32_t Present  = INVALID;

		[[nodiscard]] bool IsComplete() const { return Graphics != INVALID && Present != INVALID; }
		[[nodiscard]] bool HasDedicatedCompute() const { return Compute != INVALID && Compute != Graphics; }
		[[nodiscard]] bool HasDedicatedTransfer() const
		{
			return Transfer != INVALID && Transfer != Graphics && Transfer != Compute;
		}

		[[nodiscard]] std::set<uint32_t> GetUniqueIndices() const
		{
			std::set<uint32_t> unique;

			const auto add = [&unique](int32_t idx) {
				if (idx != INVALID)
					unique.insert(static_cast<uint32_t>(idx));
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
		explicit QueueFamiliesSelector(vk::PhysicalDevice device, vk::SurfaceKHR surface) :
		    m_Device(device), m_Surface(surface)
		{
			ER_CORE_ASSERT(m_Surface != VK_NULL_HANDLE);

			CacheFamilies();
		}

		[[nodiscard]] QueueFamilyIndices Select() const;

	private:
		void CacheFamilies();

		[[nodiscard]] int32_t SelectGraphics() const;
		[[nodiscard]] int32_t SelectPresent() const;
		[[nodiscard]] int32_t SelectCompute() const;
		[[nodiscard]] int32_t SelectTransfer() const;

	private:
		std::vector<Candidate> m_Families;
		vk::PhysicalDevice     m_Device;
		vk::SurfaceKHR         m_Surface;
	};

	struct PhysicalDeviceRequirements
	{
		std::vector<const char*> Extensions = {
		    vk::KHRSwapchainExtensionName,
		};

		vk::PhysicalDeviceFeatures2 Features{};

		vk::SurfaceKHR Surface;
	};

	class PhysicalDeviceSelector
	{
	private:
		struct Candidate
		{
			vk::PhysicalDevice                     Device;
			vk::PhysicalDeviceProperties           Properties;
			vk::PhysicalDeviceFeatures             Features;
			vk::PhysicalDeviceMemoryProperties     MemoryProperties;
			std::vector<vk::QueueFamilyProperties> QueuesProperties;
			QueueFamilyIndices                     QueuesIndices;
			bool                                   IsSuitable;
		};

	public:
		explicit PhysicalDeviceSelector(PhysicalDeviceRequirements requirements) :
		    m_Requirements(std::move(requirements))
		{
			ER_CORE_ASSERT(m_Requirements.Surface != VK_NULL_HANDLE);

			CacheDevices();
		}

		[[nodiscard]] std::pair<vk::PhysicalDevice, QueueFamilyIndices> Select() const;

	private:
		void CacheDevices();

		[[nodiscard]] Candidate EvaluateDevice(vk::PhysicalDevice device) const;

		[[nodiscard]] bool CheckRequiredExtensions(vk::PhysicalDevice device) const;
		[[nodiscard]] bool CheckRequiredFeatures(const vk::PhysicalDeviceFeatures2& available) const;

	private:
		[[nodiscard]] static uint32_t ScoreDevice(const Candidate& candidate);

	private:
		std::vector<Candidate>     m_Candidates;
		PhysicalDeviceRequirements m_Requirements;
	};

	class VulkanPhysicalDevice
	{
	public:
		explicit VulkanPhysicalDevice(const PhysicalDeviceRequirements& requirements);
		~VulkanPhysicalDevice() = default;

		[[nodiscard]] bool IsExtensionSupported(const char* extension) const;

		[[nodiscard]] vk::PhysicalDevice        GetVulkanPhysicalDevice() const { return m_PhysicalDevice; }
		[[nodiscard]] const QueueFamilyIndices& GetQueueFamilyIndices() const { return m_QueueFamilyIndices; }

		[[nodiscard]] const vk::PhysicalDeviceProperties2&       GetProperties() const { return m_Properties; }
		[[nodiscard]] const vk::PhysicalDeviceMemoryProperties2& GetMemoryProperties() const
		{
			return m_MemoryProperties;
		}

		[[nodiscard]] vk::Format GetDepthFormat() const { return m_DepthFormat; }

		static Ref<VulkanPhysicalDevice> Select(const PhysicalDeviceRequirements& requirements)
		{
			return CreateRef<VulkanPhysicalDevice>(requirements);
		}

	private:
		void SetupQueueCreateInfos();
		void FindDepthFormat();

	private:
		vk::PhysicalDevice                      m_PhysicalDevice;
		vk::PhysicalDeviceProperties2           m_Properties;
		vk::PhysicalDeviceFeatures2             m_Features;
		vk::PhysicalDeviceMemoryProperties2     m_MemoryProperties;
		std::vector<vk::QueueFamilyProperties2> m_SupportedQueueFamilies;
		std::vector<vk::ExtensionProperties>    m_SupportedExtensions;

		QueueFamilyIndices m_QueueFamilyIndices;

		std::vector<vk::DeviceQueueCreateInfo> m_QueueCreateInfos;

		vk::Format m_DepthFormat = vk::Format::eUndefined;

	private:
		friend class VulkanDevice;
	};

	class VulkanCommandPool
	{
	public:
		VulkanCommandPool();
		virtual ~VulkanCommandPool();

		[[nodiscard]] vk::CommandBuffer AllocateCommandBuffer(QueueType queueType, bool begin) const;

		void FreeCommandBuffers(
		    QueueType queueType, const std::initializer_list<vk::CommandBuffer>& commandBuffers
		) const;

		[[nodiscard]] vk::CommandPool GetCommandPool(QueueType queueType) const;

	private:
		[[nodiscard]] static vk::CommandPool CreateCommandPool(uint32_t queueFamilyIndex);

	private:
		vk::CommandPool m_GraphicsCommandPool;
		vk::CommandPool m_ComputeCommandPool;
		vk::CommandPool m_TransferCommandPool;
	};

	class VulkanDevice
	{
	public:
		explicit VulkanDevice(const Ref<VulkanPhysicalDevice>& physicalDevice);
		~VulkanDevice() = default;

		void Destroy() const;

		void LockQueue(QueueType queueType = QueueType::Graphics);
		void UnlockQueue(QueueType queueType = QueueType::Graphics);

		[[nodiscard]] vk::CommandBuffer BeginSingleTimeCommands(QueueType queueType = QueueType::Graphics);
		void EndSingleTimeCommands(vk::CommandBuffer commandBuffer, QueueType queueType = QueueType::Graphics);

		[[nodiscard]] vk::Queue GetQueue(QueueType queueType) const;

		[[nodiscard]] const Ref<VulkanPhysicalDevice>& GetPhysicalDevice() const { return m_PhysicalDevice; }

		[[nodiscard]] vk::Device GetVulkanDevice() const { return m_LogicalDevice; }

	private:
		[[nodiscard]] Ref<VulkanCommandPool> GetThreadLocalCommandPool();
		[[nodiscard]] Ref<VulkanCommandPool> GetOrCreateThreadLocalCommandPool();

	private:
		std::map<std::thread::id, Ref<VulkanCommandPool>> m_CommandPools;

		std::mutex m_GraphicsQueueMutex;
		std::mutex m_ComputeQueueMutex;
		std::mutex m_TransferQueueMutex;

		Ref<VulkanPhysicalDevice> m_PhysicalDevice;

		vk::Device m_LogicalDevice;

		vk::Queue m_GraphicsQueue;
		vk::Queue m_ComputeQueue;
		vk::Queue m_TransferQueue;
	};
}        // namespace Eruption
