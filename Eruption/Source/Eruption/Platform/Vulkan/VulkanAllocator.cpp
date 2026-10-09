#include "VmaUsage.h"        // must precede the header so the opaque typedefs match VMA's definitions

#include "Eruption/Platform/Vulkan/Device.h"
#include "Eruption/Platform/Vulkan/Instance.h"
#include "Eruption/Platform/Vulkan/PhysicalDevice.h"

#include "VulkanAllocator.h"

namespace Eruption::Vulkan
{
	namespace
	{
		struct VmaUsageMapping
		{
			VmaMemoryUsage           Usage;
			VmaAllocationCreateFlags Flags;
		};

		VmaUsageMapping MapMemoryUsage(MemoryUsage usage)
		{
			switch (usage)
			{
				case MemoryUsage::GpuOnly: return {VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 0};
				case MemoryUsage::CpuToGpu:
					return {
					    VMA_MEMORY_USAGE_AUTO,
					    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT
					};
				case MemoryUsage::GpuToCpu:
					return {
					    VMA_MEMORY_USAGE_AUTO,
					    VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT
					};
			}
			ER_CORE_VERIFY(false, "Invalid MemoryUsage");
			return {VMA_MEMORY_USAGE_AUTO, 0};
		}
	}        // namespace

	VulkanAllocator::VulkanAllocator(const Instance& instance, const Device& device) : m_Device(device)
	{
		ER_CORE_INFO_TAG("Renderer", "Creating Vulkan memory allocator");

		VmaVulkanFunctions functions{};
		functions.vkGetInstanceProcAddr = instance.GetContext().getDispatcher()->vkGetInstanceProcAddr;
		functions.vkGetDeviceProcAddr   = instance.GetVkHandle().getDispatcher()->vkGetDeviceProcAddr;

		VmaAllocatorCreateInfo createInfo{};
		createInfo.flags            = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		createInfo.instance         = *instance.GetVkHandle();
		createInfo.physicalDevice   = *device.GetPhysicalDevice()->GetVkHandle();
		createInfo.device           = *device.GetVkHandle();
		createInfo.pVulkanFunctions = &functions;
		createInfo.vulkanApiVersion = VK_API_VERSION_1_3;

		VK_CHECK_RESULT(vmaCreateAllocator(&createInfo, &m_Allocator));
	}

	VulkanAllocator::~VulkanAllocator()
	{
		if (m_Allocator)
			vmaDestroyAllocator(m_Allocator);
	}

	AllocatedBuffer VulkanAllocator::CreateBuffer(
	    const vk::BufferCreateInfo& createInfo, MemoryUsage usage, std::string_view debugName
	) const
	{
		const auto [vmaUsage, vmaFlags] = MapMemoryUsage(usage);

		VmaAllocationCreateInfo allocCreateInfo{};
		allocCreateInfo.usage = vmaUsage;
		allocCreateInfo.flags = vmaFlags;

		const VkBufferCreateInfo rawCreateInfo = createInfo;

		VkBuffer          rawBuffer  = VK_NULL_HANDLE;
		VmaAllocation     allocation = nullptr;
		VmaAllocationInfo allocationInfo{};
		VK_CHECK_RESULT(
		    vmaCreateBuffer(m_Allocator, &rawCreateInfo, &allocCreateInfo, &rawBuffer, &allocation, &allocationInfo)
		);

		if (!debugName.empty())
			vmaSetAllocationName(m_Allocator, allocation, debugName.data());

		return AllocatedBuffer{
		    .Buffer     = rawBuffer,
		    .Allocation = allocation,
		    .MappedData = allocationInfo.pMappedData,
		    .Size       = createInfo.size
		};
	}

	void VulkanAllocator::DestroyBuffer(const AllocatedBuffer& buffer) const
	{
		if (buffer.Buffer)
			vmaDestroyBuffer(m_Allocator, buffer.Buffer, buffer.Allocation);
	}

	AllocatedImage VulkanAllocator::CreateImage(
	    const vk::ImageCreateInfo& createInfo, MemoryUsage usage, std::string_view debugName
	) const
	{
		const auto [vmaUsage, vmaFlags] = MapMemoryUsage(usage);

		VmaAllocationCreateInfo allocCreateInfo{};
		allocCreateInfo.usage = vmaUsage;
		allocCreateInfo.flags = vmaFlags;

		const VkImageCreateInfo rawCreateInfo = createInfo;

		VkImage       rawImage   = VK_NULL_HANDLE;
		VmaAllocation allocation = nullptr;
		VK_CHECK_RESULT(vmaCreateImage(m_Allocator, &rawCreateInfo, &allocCreateInfo, &rawImage, &allocation, nullptr));

		if (!debugName.empty())
			vmaSetAllocationName(m_Allocator, allocation, debugName.data());

		return AllocatedImage{.Image = rawImage, .Allocation = allocation};
	}

	void VulkanAllocator::DestroyImage(const AllocatedImage& image) const
	{
		if (image.Image)
			vmaDestroyImage(m_Allocator, image.Image, image.Allocation);
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
}        // namespace Eruption::Vulkan
