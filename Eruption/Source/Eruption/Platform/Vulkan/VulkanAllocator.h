#pragma once
#include "Eruption/Platform/Vulkan/Vulkan.h"

typedef struct VmaAllocator_T*  VmaAllocator;
typedef struct VmaAllocation_T* VmaAllocation;

namespace Eruption::Vulkan
{
	class Instance;
	class Device;

	enum class MemoryUsage : uint8_t
	{
		GpuOnly,
		CpuToGpu,
		GpuToCpu,
	};

	struct AllocatedBuffer
	{
		vk::Buffer     Buffer     = nullptr;
		VmaAllocation  Allocation = nullptr;
		void*          MappedData = nullptr;
		vk::DeviceSize Size       = 0;

		[[nodiscard]] explicit operator bool() const { return static_cast<bool>(Buffer); }
	};

	struct AllocatedImage
	{
		vk::Image     Image      = nullptr;
		VmaAllocation Allocation = nullptr;

		[[nodiscard]] explicit operator bool() const { return static_cast<bool>(Image); }
	};

	class VulkanAllocator
	{
	public:
		VulkanAllocator(const Instance& instance, const Device& device);
		~VulkanAllocator();

		VulkanAllocator(const VulkanAllocator&)            = delete;
		VulkanAllocator& operator=(const VulkanAllocator&) = delete;

		[[nodiscard]] AllocatedBuffer CreateBuffer(
		    const vk::BufferCreateInfo& createInfo, MemoryUsage usage, std::string_view debugName = {}
		) const;
		void DestroyBuffer(const AllocatedBuffer& buffer) const;

		[[nodiscard]] AllocatedImage CreateImage(
		    const vk::ImageCreateInfo& createInfo, MemoryUsage usage, std::string_view debugName = {}
		) const;
		void DestroyImage(const AllocatedImage& image) const;

		void FlushAllocation(VmaAllocation allocation, vk::DeviceSize offset = 0, vk::DeviceSize size = ~0ull) const;
		void InvalidateAllocation(
		    VmaAllocation allocation, vk::DeviceSize offset = 0, vk::DeviceSize size = ~0ull
		) const;

		[[nodiscard]] VmaAllocator GetHandle() const { return m_Allocator; }

	private:
		const Device& m_Device;
		VmaAllocator  m_Allocator = nullptr;
	};
}        // namespace Eruption::Vulkan
