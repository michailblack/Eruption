#pragma once
#include "Eruption/Core/Base.h"

#include "Eruption/Platform/Vulkan/VulkanDevice.h"

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include <vk_mem_alloc.h>

#include <expected>
#include <source_location>
#include <span>
#include <string_view>
#include <unordered_map>

namespace Eruption
{
	enum class MemoryUsage
	{
		Auto,                     // VMA decides based on usage flags
		AutoPreferDevice,         // Prefer device-local memory
		AutoPreferHost,           // Prefer host-visible memory
		GpuOnly,                  // Device local, no CPU access
		CpuOnly,                  // Host visible and coherent
		CpuToGpu,                 // Staging buffer pattern
		GpuToCpu,                 // Readback pattern
		CpuCopy,                  // CPU-side buffer
		GpuLazilyAllocated        // Mobile GPU optimization
	};

	enum class AllocationCreateFlagBits : uint32_t
	{
		None                           = 0,
		DedicatedMemory                = 1 << 0,         // Use dedicated allocation
		NeverAllocate                  = 1 << 1,         // Fail if allocation would be required
		Mapped                         = 1 << 2,         // Create persistently mapped
		UserDataCopyString             = 1 << 3,         // VMA copies name string
		UpperAddress                   = 1 << 4,         // Allocate in upper address (for testing)
		DontBind                       = 1 << 5,         // Don't bind memory to resource
		WithinBudget                   = 1 << 6,         // Fail if allocation would exceed memory budget
		CanAlias                       = 1 << 7,         // Can share memory with other resources
		HostAccessSequentialWrite      = 1 << 8,         // CPU writes sequentially
		HostAccessRandom               = 1 << 9,         // CPU access is random
		HostAccessAllowTransferInstead = 1 << 10,        // Can use staging instead of direct access
		StrategyMinMemory              = 1 << 11,        // Minimize memory usage
		StrategyMinTime                = 1 << 12,        // Minimize allocation time
	};

	using AllocationCreateFlags = vk::Flags<AllocationCreateFlagBits>;

	struct MemoryStats
	{
		struct HeapStats
		{
			uint64_t BlockCount;
			uint64_t AllocationCount;
			uint64_t BlockBytes;
			uint64_t AllocationBytes;
		};

		uint64_t               TotalAllocatedBytes;
		uint64_t               TotalAvailableBytes;
		std::vector<HeapStats> HeapStats;
	};

	struct MemoryBudget
	{
		uint64_t BlockBytes;
		uint64_t AllocationBytes;
		uint64_t Usage;
		uint64_t Budget;
	};

	struct AllocationInfo
	{
		uint64_t Offset;
		uint64_t Size;
		void*    MappedData;
		uint32_t MemoryType;
		void*    UserData;
	};

	class VulkanAllocator
	{
	public:
		VulkanAllocator()  = default;
		~VulkanAllocator() = default;

		VulkanAllocator(const VulkanAllocator&)            = delete;
		VulkanAllocator& operator=(const VulkanAllocator&) = delete;
		VulkanAllocator(VulkanAllocator&&)                 = delete;
		VulkanAllocator& operator=(VulkanAllocator&&)      = delete;

		void Init(vk::Instance vkInstance, const Ref<VulkanDevice>& device);
		void Destroy();

		[[nodiscard]] std::expected<VmaAllocation, vk::Result> AllocateBuffer(
		    const vk::BufferCreateInfo& createInfo,
		    MemoryUsage                 usage,
		    vk::Buffer&                 outBuffer,
		    AllocationCreateFlags       flags = AllocationCreateFlagBits::None
		);

		[[nodiscard]] std::expected<VmaAllocation, vk::Result> AllocateImage(
		    const vk::ImageCreateInfo& createInfo,
		    MemoryUsage                usage,
		    vk::Image&                 outImage,
		    AllocationCreateFlags      flags = AllocationCreateFlagBits::None
		);

		template <typename T = void>
		[[nodiscard]] T* MapMemory(VmaAllocation allocation)
		{
			void* mappedData = nullptr;
			VK_CHECK_RESULT(vmaMapMemory(m_Allocator, allocation, &mappedData));
			return static_cast<T*>(mappedData);
		}

		void UnmapMemory(VmaAllocation allocation) const;

		void DestroyBuffer(vk::Buffer buffer, VmaAllocation allocation);
		void DestroyImage(vk::Image image, VmaAllocation allocation);
		void FreeMemory(VmaAllocation allocation);

		void FlushAllocation(
		    VmaAllocation allocation, vk::DeviceSize offset = 0, vk::DeviceSize size = VK_WHOLE_SIZE
		) const;
		void InvalidateAllocation(
		    VmaAllocation allocation, vk::DeviceSize offset = 0, vk::DeviceSize size = VK_WHOLE_SIZE
		) const;

		[[nodiscard]] AllocationInfo GetAllocationInfo(VmaAllocation allocation) const;
		void                         SetAllocationName(VmaAllocation allocation, std::string_view name);
		void                         SetAllocationUserData(VmaAllocation allocation, void* userData) const;

		[[nodiscard]] MemoryStats               CalculateStats() const;
		[[nodiscard]] std::vector<MemoryBudget> GetBudget() const;
		[[nodiscard]] std::string               BuildStatsString(bool detailed = false) const;
		void                                    SetCurrentFrameIndex(uint32_t frameIndex) const;

		[[nodiscard]] std::optional<uint32_t> FindMemoryTypeIndex(
		    uint32_t memoryTypeBits, vk::MemoryPropertyFlags requiredFlags
		) const;

		[[nodiscard]] VmaAllocator GetVmaAllocator() const { return m_Allocator; }

	private:
		Ref<VulkanDevice> m_Device;

		VmaAllocator m_Allocator = VK_NULL_HANDLE;

#ifdef ER_DEBUG
		struct AllocationTracker
		{
			vk::DeviceSize       Size;
			std::string          Name;
			std::source_location Location;
		};
		std::unordered_map<VmaAllocation, AllocationTracker> m_AllocationTracker;

		mutable std::mutex m_TrackerMutex;
#endif
	};

	template <typename T>
	concept VulkanResource = requires(T t) {
		{ t.GetVkHandle() } -> std::convertible_to<typename T::HandleType>;
		typename T::HandleType;
	};

	class VulkanAllocation
	{
	public:
		VulkanAllocation() = default;
		VulkanAllocation(VulkanAllocator* allocator, VmaAllocation allocation) :
		    m_Allocator(allocator), m_Allocation(allocation)
		{}

		~VulkanAllocation()
		{
			if (m_Allocation && m_Allocator)
				m_Allocator->FreeMemory(m_Allocation);
		}

		VulkanAllocation(const VulkanAllocation&)            = delete;
		VulkanAllocation& operator=(const VulkanAllocation&) = delete;
		VulkanAllocation(VulkanAllocation&& other) noexcept :
		    m_Allocator(other.m_Allocator), m_Allocation(std::exchange(other.m_Allocation, nullptr))
		{}
		VulkanAllocation& operator=(VulkanAllocation&& other) noexcept
		{
			if (this != &other)
			{
				if (m_Allocation && m_Allocator)
					m_Allocator->FreeMemory(m_Allocation);
				m_Allocator  = other.m_Allocator;
				m_Allocation = std::exchange(other.m_Allocation, nullptr);
			}
			return *this;
		}

		[[nodiscard]] VmaAllocation Get() const { return m_Allocation; }
		[[nodiscard]] VmaAllocation Release() { return std::exchange(m_Allocation, nullptr); }
		[[nodiscard]] bool          IsValid() const { return m_Allocation != VK_NULL_HANDLE; }

		template <typename T = void>
		[[nodiscard]] T* Map()
		{
			return m_Allocator ? m_Allocator->MapMemory<T>(m_Allocation) : nullptr;
		}

		void Unmap() const
		{
			if (m_Allocator && m_Allocation)
				m_Allocator->UnmapMemory(m_Allocation);
		}

	private:
		VulkanAllocator* m_Allocator  = nullptr;
		VmaAllocation    m_Allocation = VK_NULL_HANDLE;
	};

}        // namespace Eruption
