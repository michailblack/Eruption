#pragma once
#include "Eruption/Platform/Vulkan/VulkanAllocator.h"
#include "Eruption/Platform/Vulkan/VulkanDevice.h"

#include "Eruption/Renderer/Renderer.h"
#include "Eruption/Renderer/RendererContext.h"

namespace Eruption
{
#ifdef ER_DEBUG
	class VulkanValidation
	{
	public:
		void Init();
		void Destroy(vk::Instance vulkanInstance, const vk::detail::DispatchLoaderDynamic& dispatcher) const;

		void CreateDebugMessenger(vk::Instance vulkanInstance, const vk::detail::DispatchLoaderDynamic& dispatcher);

		[[nodiscard]] const std::vector<const char*>& GetRequiredLayers() const { return m_Layers; }
		[[nodiscard]] const std::vector<const char*>& GetRequiredExtensions() const { return m_Extensions; }

		[[nodiscard]] const vk::DebugUtilsMessengerCreateInfoEXT& GetDebugMessengerCreateInfo() const
		{
			return m_DebugMessengerCreateInfo;
		}

	private:
		std::vector<const char*> m_Layers;
		std::vector<const char*> m_Extensions;

		vk::DebugUtilsMessengerCreateInfoEXT m_DebugMessengerCreateInfo;
		vk::DebugUtilsMessengerEXT           m_DebugMessenger;
	};
#endif

	class VulkanContext : public RendererContext
	{
	public:
		VulkanContext() = default;
		~VulkanContext() override;

		void Init(GLFWwindow* window) override;

		[[nodiscard]] vk::Instance         GetVulkanInstance() const { return m_VulkanInstance; }
		[[nodiscard]] Ref<VulkanDevice>    GetDevice() const { return m_Device; }
		[[nodiscard]] Ref<VulkanAllocator> GetAllocator() const { return m_Allocator; }
		[[nodiscard]] vk::SurfaceKHR       GetSurface() const { return m_Surface; }

		[[nodiscard]] Ref<vk::detail::DispatchLoaderDynamic> GetDLD() const { return m_DispatchLoaderDynamic; }

		static Ref<VulkanContext>   Get() { return As<VulkanContext>(Renderer::GetContext()); }
		static vk::Instance         GetInstance() { return Get()->GetVulkanInstance(); }
		static Ref<VulkanDevice>    GetCurrentDevice() { return Get()->GetDevice(); }
		static Ref<VulkanAllocator> GetCurrentAllocator() { return Get()->GetAllocator(); }

	private:
		Ref<VulkanPhysicalDevice> m_PhysicalDevice;
		Ref<VulkanDevice>         m_Device;
		Ref<VulkanAllocator>      m_Allocator;

		Ref<vk::detail::DispatchLoaderDynamic> m_DispatchLoaderDynamic;

		vk::Instance m_VulkanInstance;

		vk::SurfaceKHR m_Surface;

		vk::PipelineCache m_PipelineCache;

#ifdef ER_DEBUG
		Scope<VulkanValidation> m_Validation;
#endif
	};

}        // namespace Eruption
