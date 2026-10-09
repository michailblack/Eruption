#pragma once
#include "Eruption/Platform/Vulkan/Device.h"
#include "Eruption/Platform/Vulkan/Instance.h"
#include "Eruption/Platform/Vulkan/PhysicalDevice.h"
#include "Eruption/Platform/Vulkan/VulkanAllocator.h"

#include <functional>
#include <mutex>

namespace Eruption
{
	class Window;
}

namespace Eruption::Vulkan
{
	class VulkanContext
	{
	public:
		VulkanContext(const VulkanContext&)            = delete;
		VulkanContext(VulkanContext&&)                 = delete;
		VulkanContext& operator=(const VulkanContext&) = delete;
		VulkanContext& operator=(VulkanContext&&)      = delete;

		static void Init(const Scope<Window>& window);
		static void Shutdown();

		static void ImmediateSubmit(const std::function<void(const vk::raii::CommandBuffer&)>& record);

		[[nodiscard]] static VulkanContext&          Get();
		[[nodiscard]] static const vk::raii::Device& GetVkDevice();

		[[nodiscard]] static bool IsInitialized();

		[[nodiscard]] const Ref<Instance>&       GetInstance() const { return m_Instance; }
		[[nodiscard]] const Ref<PhysicalDevice>& GetPhysicalDevice() const { return m_PhysicalDevice; }
		[[nodiscard]] const Ref<Device>&         GetDevice() const { return m_Device; }
		[[nodiscard]] const VulkanAllocator&     GetAllocator() const { return *m_Allocator; }

	private:
		explicit VulkanContext(const Scope<Window>& window);

		void ImmediateSubmitImpl(const std::function<void(const vk::raii::CommandBuffer&)>& record);

	private:
		Ref<Instance>          m_Instance;
		Ref<PhysicalDevice>    m_PhysicalDevice;
		Ref<Device>            m_Device;
		Scope<VulkanAllocator> m_Allocator;

		std::mutex              m_ImmediateMutex;
		vk::raii::CommandPool   m_ImmediatePool  = nullptr;
		vk::raii::CommandBuffer m_ImmediateCmd   = nullptr;
		vk::raii::Fence         m_ImmediateFence = nullptr;
	};
}        // namespace Eruption::Vulkan
