#include "Buffer.h"

#include "Eruption/Platform/Vulkan/VulkanContext.h"

namespace Eruption::Vulkan
{
	namespace
	{
		[[nodiscard]] bool IsHostVisible(MemoryUsage usage)
		{
			return usage == MemoryUsage::CpuToGpu || usage == MemoryUsage::GpuToCpu;
		}
	}        // namespace

	Buffer::Buffer(vk::DeviceSize size, vk::BufferUsageFlags usage, MemoryUsage memoryUsage, std::string_view name) :
	    m_Usage(usage), m_MemoryUsage(memoryUsage)
	{
		ER_CORE_ASSERT(size > 0, "Cannot create a zero-sized buffer");

		const vk::BufferCreateInfo createInfo{.size = size, .usage = usage, .sharingMode = vk::SharingMode::eExclusive};
		m_Allocation = VulkanContext::Get().GetAllocator().CreateBuffer(createInfo, memoryUsage, name);
	}

	Buffer::~Buffer()
	{
		Release();
	}

	void Buffer::Release()
	{
		if (m_Allocation)
			VulkanContext::Get().GetAllocator().DestroyBuffer(m_Allocation);
		m_Allocation = {};
	}

	// TODO: Tricky construction
	Buffer::Buffer(Buffer&& other) noexcept :
	    m_Allocation(std::exchange(other.m_Allocation, {})), m_Usage(other.m_Usage), m_MemoryUsage(other.m_MemoryUsage)
	{}

	// TODO: Tricky construction
	Buffer& Buffer::operator=(Buffer&& other) noexcept
	{
		if (this != &other)
		{
			Release();
			m_Allocation  = std::exchange(other.m_Allocation, {});
			m_Usage       = other.m_Usage;
			m_MemoryUsage = other.m_MemoryUsage;
		}
		return *this;
	}

	void Buffer::SetData(const void* data, vk::DeviceSize size, vk::DeviceSize offset)
	{
		ER_CORE_ASSERT(offset + size <= m_Allocation.Size, "Buffer::SetData out of range");

		const VulkanAllocator& allocator = VulkanContext::Get().GetAllocator();

		if (IsHostVisible(m_MemoryUsage))
		{
			ER_CORE_ASSERT(m_Allocation.MappedData, "Host-visible buffer is not mapped");
			std::memcpy(static_cast<std::byte*>(m_Allocation.MappedData) + offset, data, size);
			allocator.FlushAllocation(m_Allocation.Allocation, offset, size);
			return;
		}

		const vk::BufferCreateInfo stagingInfo{
		    .size = size, .usage = vk::BufferUsageFlagBits::eTransferSrc, .sharingMode = vk::SharingMode::eExclusive
		};
		const AllocatedBuffer staging = allocator.CreateBuffer(stagingInfo, MemoryUsage::CpuToGpu, "StagingBuffer");
		std::memcpy(staging.MappedData, data, size);
		allocator.FlushAllocation(staging.Allocation);

		VulkanContext::ImmediateSubmit([&](const vk::raii::CommandBuffer& cmd) {
			cmd.copyBuffer(
			    staging.Buffer, m_Allocation.Buffer, vk::BufferCopy{.srcOffset = 0, .dstOffset = offset, .size = size}
			);
		});

		allocator.DestroyBuffer(staging);
	}

	vk::DeviceAddress Buffer::GetDeviceAddress() const
	{
		return VulkanContext::GetVkDevice().getBufferAddress(
		    vk::BufferDeviceAddressInfo{.buffer = m_Allocation.Buffer}
		);
	}

	Ref<Buffer> Buffer::CreateVertex(const void* data, vk::DeviceSize size, std::string_view name)
	{
		auto buffer = CreateRef<Buffer>(
		    size,
		    vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst |
		        vk::BufferUsageFlagBits::eShaderDeviceAddress,
		    MemoryUsage::GpuOnly,
		    name
		);

		if (data)
			buffer->SetData(data, size);

		return buffer;
	}

	Ref<Buffer> Buffer::CreateIndex(const void* data, vk::DeviceSize size, std::string_view name)
	{
		auto buffer = CreateRef<Buffer>(
		    size,
		    vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
		    MemoryUsage::GpuOnly,
		    name
		);

		if (data)
			buffer->SetData(data, size);

		return buffer;
	}

	Ref<Buffer> Buffer::CreateUniform(vk::DeviceSize size, std::string_view name)
	{
		return CreateRef<Buffer>(size, vk::BufferUsageFlagBits::eUniformBuffer, MemoryUsage::CpuToGpu, name);
	}

	Ref<Buffer> Buffer::CreateStorage(vk::DeviceSize size, std::string_view name)
	{
		return CreateRef<Buffer>(
		    size,
		    vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst |
		        vk::BufferUsageFlagBits::eShaderDeviceAddress,
		    MemoryUsage::GpuOnly,
		    name
		);
	}
}        // namespace Eruption::Vulkan
