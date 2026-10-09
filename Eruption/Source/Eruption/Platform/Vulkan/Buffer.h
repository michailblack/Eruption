#pragma once
#include "Eruption/Platform/Vulkan/VulkanAllocator.h"

namespace Eruption::Vulkan
{
	class Buffer
	{
	public:
		Buffer(vk::DeviceSize size, vk::BufferUsageFlags usage, MemoryUsage memoryUsage, std::string_view name = {});
		~Buffer();

		Buffer(const Buffer&)            = delete;
		Buffer& operator=(const Buffer&) = delete;
		Buffer(Buffer&& other) noexcept;
		Buffer& operator=(Buffer&& other) noexcept;

		void SetData(const void* data, vk::DeviceSize size, vk::DeviceSize offset = 0);

		[[nodiscard]] vk::Buffer        GetHandle() const { return m_Allocation.Buffer; }
		[[nodiscard]] vk::DeviceSize    GetSize() const { return m_Allocation.Size; }
		[[nodiscard]] void*             GetMappedData() const { return m_Allocation.MappedData; }
		[[nodiscard]] vk::DeviceAddress GetDeviceAddress() const;

		[[nodiscard]] static Ref<Buffer> CreateVertex(
		    const void* data, vk::DeviceSize size, std::string_view name = "VertexBuffer"
		);
		[[nodiscard]] static Ref<Buffer> CreateIndex(
		    const void* data, vk::DeviceSize size, std::string_view name = "IndexBuffer"
		);
		[[nodiscard]] static Ref<Buffer> CreateUniform(vk::DeviceSize size, std::string_view name = "UniformBuffer");
		[[nodiscard]] static Ref<Buffer> CreateStorage(vk::DeviceSize size, std::string_view name = "StorageBuffer");

	private:
		void Release();

	private:
		AllocatedBuffer      m_Allocation;
		vk::BufferUsageFlags m_Usage;
		MemoryUsage          m_MemoryUsage;
	};
}        // namespace Eruption::Vulkan
