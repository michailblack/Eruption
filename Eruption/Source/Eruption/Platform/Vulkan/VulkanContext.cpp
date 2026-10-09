#include "VulkanContext.h"

namespace Eruption::Vulkan
{
	namespace
	{
		Scope<VulkanContext> s_Context;
	}

	VulkanContext::VulkanContext(const Scope<Window>& window)
	{
		m_Instance       = CreateRef<Instance>(window);
		m_PhysicalDevice = CreateRef<PhysicalDevice>(m_Instance);
		m_Device         = CreateRef<Device>(m_Instance, m_PhysicalDevice);
		m_Allocator      = CreateScope<VulkanAllocator>(*m_Instance, *m_Device);

		const vk::raii::Device& device    = m_Device->GetVkHandle();
		const uint32_t          gfxFamily = m_Device->GetQueueFamilyIndex(QueueType::Graphics);

		m_ImmediatePool = vk::raii::CommandPool(
		    device,
		    vk::CommandPoolCreateInfo{.flags = vk::CommandPoolCreateFlagBits::eTransient, .queueFamilyIndex = gfxFamily}
		);
		vk::raii::CommandBuffers buffers(
		    device,
		    vk::CommandBufferAllocateInfo{
		        .commandPool = *m_ImmediatePool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1
		    }
		);
		m_ImmediateCmd   = std::move(buffers.front());
		m_ImmediateFence = vk::raii::Fence(device, vk::FenceCreateInfo{});
	}

	void VulkanContext::ImmediateSubmit(const std::function<void(const vk::raii::CommandBuffer&)>& record)
	{
		Get().ImmediateSubmitImpl(record);
	}

	void VulkanContext::ImmediateSubmitImpl(const std::function<void(const vk::raii::CommandBuffer&)>& record)
	{
		std::scoped_lock _(m_ImmediateMutex);

		m_Device->GetVkHandle().resetFences(*m_ImmediateFence);
		m_ImmediatePool.reset();

		m_ImmediateCmd.begin(vk::CommandBufferBeginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
		record(m_ImmediateCmd);
		m_ImmediateCmd.end();

		const vk::CommandBufferSubmitInfo cmdInfo{.commandBuffer = *m_ImmediateCmd};
		const vk::SubmitInfo2             submit{.commandBufferInfoCount = 1, .pCommandBufferInfos = &cmdInfo};
		m_Device->Submit(QueueType::Graphics, submit, *m_ImmediateFence);

		VK_CHECK_RESULT(
		    m_Device->GetVkHandle().waitForFences(*m_ImmediateFence, vk::True, std::numeric_limits<uint64_t>::max())
		);
	}

	void VulkanContext::Init(const Scope<Window>& window)
	{
		ER_CORE_ASSERT(!s_Context, "VulkanContext already initialized!");
		s_Context = Scope<VulkanContext>(new VulkanContext(window));
	}

	void VulkanContext::Shutdown()
	{
		s_Context.reset();
	}

	VulkanContext& VulkanContext::Get()
	{
		ER_CORE_ASSERT(s_Context, "VulkanContext accessed before initialization!");
		return *s_Context;
	}

	const vk::raii::Device& VulkanContext::GetVkDevice()
	{
		return Get().m_Device->GetVkHandle();
	}

	bool VulkanContext::IsInitialized()
	{
		return static_cast<bool>(s_Context);
	}
}        // namespace Eruption::Vulkan
