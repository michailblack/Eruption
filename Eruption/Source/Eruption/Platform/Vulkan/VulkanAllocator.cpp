#include "VulkanAllocator.h"

namespace Eruption
{
	namespace Utils
	{
		namespace
		{
			void VmaDebugAllocationCallback(
			    VmaAllocator allocator, uint32_t memoryType, VkDeviceMemory memory, VkDeviceSize size, void* pUserData
			)
			{
				ER_CORE_TRACE_TAG("VulkanAllocator", "Allocated {} bytes of memory type {}", size, memoryType);
			}

			void VmaDebugFreeCallback(
			    VmaAllocator allocator, uint32_t memoryType, VkDeviceMemory memory, VkDeviceSize size, void* pUserData
			)
			{
				ER_CORE_TRACE_TAG("VulkanAllocator", "Freed {} bytes of memory type {}", size, memoryType);
			}

			std::string FormatBytes(uint64_t bytes)
			{
				constexpr uint64_t KiB = 1024;
				constexpr uint64_t MiB = KiB * 1024;
				constexpr uint64_t GiB = MiB * 1024;

				if (bytes >= GiB)
					return std::format("{:.2f} GiB", static_cast<double>(bytes) / GiB);
				if (bytes >= MiB)
					return std::format("{:.2f} MiB", static_cast<double>(bytes) / MiB);
				if (bytes >= KiB)
					return std::format("{:.2f} KiB", static_cast<double>(bytes) / KiB);
				return std::format("{} B", bytes);
			}

			VmaMemoryUsage ConvertMemoryUsage(MemoryUsage usage)
			{
				switch (usage)
				{
					case MemoryUsage::Auto:               return VMA_MEMORY_USAGE_AUTO;
					case MemoryUsage::AutoPreferDevice:   return VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
					case MemoryUsage::AutoPreferHost:     return VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
					case MemoryUsage::GpuOnly:            return VMA_MEMORY_USAGE_GPU_ONLY;
					case MemoryUsage::CpuOnly:            return VMA_MEMORY_USAGE_CPU_ONLY;
					case MemoryUsage::CpuToGpu:           return VMA_MEMORY_USAGE_CPU_TO_GPU;
					case MemoryUsage::GpuToCpu:           return VMA_MEMORY_USAGE_GPU_TO_CPU;
					case MemoryUsage::CpuCopy:            return VMA_MEMORY_USAGE_CPU_COPY;
					case MemoryUsage::GpuLazilyAllocated: return VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED;
				}
				ER_CORE_ASSERT(false, "Invalid memory usage");
				return VMA_MEMORY_USAGE_AUTO;
			}

			VmaAllocationCreateFlags ConvertAllocationFlags(AllocationCreateFlags flags)
			{
				VmaAllocationCreateFlags vmaFlags = 0;

				if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AllocationCreateFlagBits::DedicatedMemory))
					vmaFlags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
				if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AllocationCreateFlagBits::NeverAllocate))
					vmaFlags |= VMA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT;
				if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AllocationCreateFlagBits::Mapped))
					vmaFlags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
				if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AllocationCreateFlagBits::UserDataCopyString))
					vmaFlags |= VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
				if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AllocationCreateFlagBits::UpperAddress))
					vmaFlags |= VMA_ALLOCATION_CREATE_UPPER_ADDRESS_BIT;
				if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AllocationCreateFlagBits::DontBind))
					vmaFlags |= VMA_ALLOCATION_CREATE_DONT_BIND_BIT;
				if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AllocationCreateFlagBits::WithinBudget))
					vmaFlags |= VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT;
				if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AllocationCreateFlagBits::CanAlias))
					vmaFlags |= VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT;
				if (static_cast<uint32_t>(flags) &
				    static_cast<uint32_t>(AllocationCreateFlagBits::HostAccessSequentialWrite))
					vmaFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
				if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AllocationCreateFlagBits::HostAccessRandom))
					vmaFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
				if (static_cast<uint32_t>(flags) &
				    static_cast<uint32_t>(AllocationCreateFlagBits::HostAccessAllowTransferInstead))
					vmaFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT;
				if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AllocationCreateFlagBits::StrategyMinMemory))
					vmaFlags |= VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT;
				if (static_cast<uint32_t>(flags) & static_cast<uint32_t>(AllocationCreateFlagBits::StrategyMinTime))
					vmaFlags |= VMA_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT;

				return vmaFlags;
			}
		}        // namespace
	}        // namespace Utils

	/* ------------------------------------------------------------------------------------------------------- */
	/* ----------------------------------------- VulkanAllocator --------------------------------------------- */
	/* ------------------------------------------------------------------------------------------------------- */
	void VulkanAllocator::Init(vk::Instance vkInstance, const Ref<VulkanDevice>& device)
	{
		ER_CORE_INFO_TAG("VulkanAllocator", "VulkanAllocator::Init");

		m_Device                                       = device;
		const Ref<VulkanPhysicalDevice> physicalDevice = device->GetPhysicalDevice();

		VmaVulkanFunctions vulkanFunctions{};
		vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
		vulkanFunctions.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;

		VmaAllocatorCreateInfo allocatorInfo{};
		allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_4;
		allocatorInfo.physicalDevice   = physicalDevice->GetVulkanPhysicalDevice();
		allocatorInfo.device           = device->GetVulkanDevice();
		allocatorInfo.instance         = vkInstance;
		allocatorInfo.pVulkanFunctions = &vulkanFunctions;

		if (physicalDevice->IsExtensionSupported(vk::KHRDedicatedAllocationExtensionName))
			allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;

		if (physicalDevice->IsExtensionSupported(vk::KHRBindMemory2ExtensionName))
			allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT;

		if (physicalDevice->IsExtensionSupported(vk::KHRBufferDeviceAddressExtensionName))
			allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

		if (physicalDevice->IsExtensionSupported(vk::EXTMemoryBudgetExtensionName))
			allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;

#ifdef ER_DEBUG
		VmaDeviceMemoryCallbacks memoryCallbacks{};
		memoryCallbacks.pfnAllocate          = Utils::VmaDebugAllocationCallback;
		memoryCallbacks.pfnFree              = Utils::VmaDebugFreeCallback;
		allocatorInfo.pDeviceMemoryCallbacks = &memoryCallbacks;
#endif

		VK_CHECK_RESULT(vmaCreateAllocator(&allocatorInfo, &m_Allocator));

		const MemoryStats stats = CalculateStats();
		ER_CORE_INFO_TAG(
		    "VulkanAllocator", "Total available GPU memory: {}", Utils::FormatBytes(stats.TotalAvailableBytes)
		);
	}

	void VulkanAllocator::Destroy()
	{
#ifdef ER_DEBUG
		// Check for leaked allocations
		{
			std::lock_guard lock(m_TrackerMutex);
			if (!m_AllocationTracker.empty())
			{
				ER_CORE_ERROR_TAG("VulkanAllocator", "Leaked {} allocations:", m_AllocationTracker.size());
				for (const auto& [deviceSize, name, location] : m_AllocationTracker | std::ranges::views::values)
				{
					ER_CORE_ERROR_TAG(
					    "VulkanAllocator",
					    "\t- {} ({}) at {}:{}",
					    name,
					    Utils::FormatBytes(deviceSize),
					    location.file_name(),
					    location.line()
					);
				}
			}
		}
#endif

		if (m_Allocator)
		{
			vmaDestroyAllocator(m_Allocator);
			m_Allocator = VK_NULL_HANDLE;
		}
	}

	std::expected<VmaAllocation, vk::Result> VulkanAllocator::AllocateBuffer(
	    const vk::BufferCreateInfo& createInfo, MemoryUsage usage, vk::Buffer& outBuffer, AllocationCreateFlags flags
	)
	{
		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = Utils::ConvertMemoryUsage(usage);
		allocInfo.flags = Utils::ConvertAllocationFlags(flags);

		auto          vkCreateInfo = static_cast<VkBufferCreateInfo>(createInfo);
		VkBuffer      vkBuffer;
		VmaAllocation allocation;

		const auto result = static_cast<vk::Result>(
		    vmaCreateBuffer(m_Allocator, &vkCreateInfo, &allocInfo, &vkBuffer, &allocation, nullptr)
		);

		if (result != vk::Result::eSuccess)
		{
			ER_CORE_ERROR_TAG(
			    "VulkanAllocator",
			    "Failed to allocate buffer of size {}: {}",
			    Utils::FormatBytes(createInfo.size),
			    vk::to_string(result)
			);

			return std::unexpected(result);
		}

		outBuffer = vkBuffer;

#ifdef ER_DEBUG
		{
			std::lock_guard   lock(m_TrackerMutex);
			VmaAllocationInfo info;
			vmaGetAllocationInfo(m_Allocator, allocation, &info);
			m_AllocationTracker[allocation] = {
			    .Size = info.size, .Name = "Buffer", .Location = std::source_location::current()
			};
		}
#endif

		return allocation;
	}

	std::expected<VmaAllocation, vk::Result> VulkanAllocator::AllocateImage(
	    const vk::ImageCreateInfo& createInfo, MemoryUsage usage, vk::Image& outImage, AllocationCreateFlags flags
	)
	{
		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage = Utils::ConvertMemoryUsage(usage);
		allocInfo.flags = Utils::ConvertAllocationFlags(flags);

		auto          vkCreateInfo = static_cast<VkImageCreateInfo>(createInfo);
		VkImage       vkImage;
		VmaAllocation allocation;

		const auto result = static_cast<vk::Result>(
		    vmaCreateImage(m_Allocator, &vkCreateInfo, &allocInfo, &vkImage, &allocation, nullptr)
		);

		if (result != vk::Result::eSuccess)
		{
			ER_CORE_ERROR_TAG(
			    "VulkanAllocator",
			    "Failed to allocate image {}x{}x{}: {}",
			    createInfo.extent.width,
			    createInfo.extent.height,
			    createInfo.extent.depth,
			    vk::to_string(result)
			);

			return std::unexpected(result);
		}

		outImage = vkImage;

#ifdef ER_DEBUG
		{
			std::lock_guard   lock(m_TrackerMutex);
			VmaAllocationInfo info;
			vmaGetAllocationInfo(m_Allocator, allocation, &info);
			m_AllocationTracker[allocation] = {
			    .Size = info.size, .Name = "Image", .Location = std::source_location::current()
			};
		}
#endif

		return allocation;
	}

	void VulkanAllocator::UnmapMemory(VmaAllocation allocation) const
	{
		vmaUnmapMemory(m_Allocator, allocation);
	}

	void VulkanAllocator::DestroyBuffer(vk::Buffer buffer, VmaAllocation allocation)
	{
		ER_CORE_ASSERT(buffer);
		ER_CORE_ASSERT(allocation);

#ifdef ER_DEBUG
		{
			std::lock_guard lock(m_TrackerMutex);
			m_AllocationTracker.erase(allocation);
		}
#endif

		vmaDestroyBuffer(m_Allocator, buffer, allocation);
	}

	void VulkanAllocator::DestroyImage(vk::Image image, VmaAllocation allocation)
	{
		ER_CORE_ASSERT(image);
		ER_CORE_ASSERT(allocation);

#ifdef ER_DEBUG
		{
			std::lock_guard lock(m_TrackerMutex);
			m_AllocationTracker.erase(allocation);
		}
#endif

		vmaDestroyImage(m_Allocator, static_cast<VkImage>(image), allocation);
	}

	void VulkanAllocator::FreeMemory(VmaAllocation allocation)
	{
		ER_CORE_ASSERT(allocation);

#ifdef ER_DEBUG
		{
			std::lock_guard lock(m_TrackerMutex);
			m_AllocationTracker.erase(allocation);
		}
#endif

		vmaFreeMemory(m_Allocator, allocation);
	}

	void VulkanAllocator::FlushAllocation(VmaAllocation allocation, vk::DeviceSize offset, vk::DeviceSize size) const
	{
		VK_CHECK_RESULT(vmaFlushAllocation(m_Allocator, allocation, offset, size));
	}

	void VulkanAllocator::InvalidateAllocation(
	    VmaAllocation allocation, vk::DeviceSize offset, vk::DeviceSize size
	) const
	{
		VK_CHECK_RESULT(vmaInvalidateAllocation(m_Allocator, allocation, offset, size));
	}

	AllocationInfo VulkanAllocator::GetAllocationInfo(VmaAllocation allocation) const
	{
		VmaAllocationInfo vmaInfo;
		vmaGetAllocationInfo(m_Allocator, allocation, &vmaInfo);

		return AllocationInfo{
		    .Offset     = vmaInfo.offset,
		    .Size       = vmaInfo.size,
		    .MappedData = vmaInfo.pMappedData,
		    .MemoryType = vmaInfo.memoryType,
		    .UserData   = vmaInfo.pUserData
		};
	}

	void VulkanAllocator::SetAllocationName(VmaAllocation allocation, std::string_view name)
	{
		vmaSetAllocationName(m_Allocator, allocation, name.data());

#ifdef ER_DEBUG
		{
			std::lock_guard lock(m_TrackerMutex);
			if (const auto it = m_AllocationTracker.find(allocation); it != m_AllocationTracker.end())
				it->second.Name = name;
		}
#endif
	}

	void VulkanAllocator::SetAllocationUserData(VmaAllocation allocation, void* userData) const
	{
		vmaSetAllocationUserData(m_Allocator, allocation, userData);
	}

	MemoryStats VulkanAllocator::CalculateStats() const
	{
		const Ref<VulkanPhysicalDevice> physicalDevice = m_Device->GetPhysicalDevice();

		VmaTotalStatistics stats;
		vmaCalculateStatistics(m_Allocator, &stats);

		MemoryStats result{};
		result.TotalAllocatedBytes = stats.total.statistics.allocationBytes;
		result.TotalAvailableBytes = 0;

		const std::vector<MemoryBudget> budgets = GetBudget();
		for (const auto& budget : budgets)
			result.TotalAvailableBytes += budget.Budget;

		result.HeapStats.reserve(VK_MAX_MEMORY_HEAPS);
		for (uint32_t i = 0; i < physicalDevice->GetMemoryProperties().memoryProperties.memoryHeapCount; ++i)
		{
			const VmaStatistics& heapStats = stats.memoryHeap[i].statistics;
			result.HeapStats.push_back(
			    {.BlockCount      = heapStats.blockCount,
			     .AllocationCount = heapStats.allocationCount,
			     .BlockBytes      = heapStats.blockBytes,
			     .AllocationBytes = heapStats.allocationBytes}
			);
		}

		return result;
	}

	std::vector<MemoryBudget> VulkanAllocator::GetBudget() const
	{
		const Ref<VulkanPhysicalDevice> physicalDevice = m_Device->GetPhysicalDevice();

		const vk::PhysicalDeviceMemoryProperties& memProps = physicalDevice->GetMemoryProperties().memoryProperties;

		std::vector<VmaBudget> vmaBudgets(memProps.memoryHeapCount);
		vmaGetHeapBudgets(m_Allocator, vmaBudgets.data());

		std::vector<MemoryBudget> budgets;
		budgets.reserve(memProps.memoryHeapCount);

		for (uint32_t i = 0; i < memProps.memoryHeapCount; ++i)
		{
			budgets.push_back(
			    {.BlockBytes      = vmaBudgets[i].statistics.blockBytes,
			     .AllocationBytes = vmaBudgets[i].statistics.allocationBytes,
			     .Usage           = vmaBudgets[i].usage,
			     .Budget          = vmaBudgets[i].budget}
			);
		}

		return budgets;
	}

	std::string VulkanAllocator::BuildStatsString(bool detailed) const
	{
		char* statsString = nullptr;
		vmaBuildStatsString(m_Allocator, &statsString, detailed ? VK_TRUE : VK_FALSE);

		std::string result(statsString);
		vmaFreeStatsString(m_Allocator, statsString);

		return result;
	}

	void VulkanAllocator::SetCurrentFrameIndex(uint32_t frameIndex) const
	{
		vmaSetCurrentFrameIndex(m_Allocator, frameIndex);
	}

	std::optional<uint32_t> VulkanAllocator::FindMemoryTypeIndex(
	    uint32_t memoryTypeBits, vk::MemoryPropertyFlags requiredFlags
	) const
	{
		VmaAllocationCreateInfo allocInfo{};
		allocInfo.usage          = VMA_MEMORY_USAGE_UNKNOWN;
		allocInfo.requiredFlags  = static_cast<VkMemoryPropertyFlags>(requiredFlags);
		allocInfo.memoryTypeBits = memoryTypeBits;

		uint32_t   memoryTypeIndex;
		const auto result = static_cast<vk::Result>(
		    vmaFindMemoryTypeIndex(m_Allocator, memoryTypeBits, &allocInfo, &memoryTypeIndex)
		);

		if (result != vk::Result::eSuccess)
			return std::nullopt;

		return memoryTypeIndex;
	}
}        // namespace Eruption
