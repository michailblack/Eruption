#pragma once
#include "Eruption/Platform/Vulkan/Vulkan.h"

#define VP_USE_OBJECT
#include <vulkan/vulkan_profiles.hpp>

namespace Eruption
{
	class Window;
}

namespace Eruption::Vulkan
{
	class Instance
	{
	public:
		explicit Instance(const Scope<Window>& window);
		~Instance();

		Instance(const Instance&)            = delete;
		Instance& operator=(const Instance&) = delete;

		Instance(Instance&& other) noexcept            = default;
		Instance& operator=(Instance&& other) noexcept = default;

		[[nodiscard]] const vk::raii::Instance&   GetVkHandle() const { return m_Instance; }
		[[nodiscard]] const vk::raii::SurfaceKHR& GetSurfaceHandle() const { return m_Surface; }

		[[nodiscard]] const VpCapabilities& GetVpCapabilities() const { return m_VpCapabilities; }

	public:
		[[nodiscard]] static constexpr const VpProfileProperties& GetVpProfileProperties()
		{
			return s_VpProfileProperties;
		}

		// static Ref<RendererContext>      Get() { return As<RendererContext>(Renderer::GetContext()); }
		// static const vk::raii::Instance& GetInstance() { return Get()->GetVulkanInstance(); }

	private:
		[[nodiscard]] std::vector<const char*> GetRequiredExtensions() const;
		[[nodiscard]] std::vector<const char*> GetRequiredLayers() const;

		void CreateDebugUtilsMessenger();

		void CreateSurface(const Scope<Window>& window);

	private:
		vk::raii::Context  m_Context;
		vk::raii::Instance m_Instance = nullptr;

		vk::raii::SurfaceKHR m_Surface = nullptr;

#ifdef ER_DEBUG
		vk::raii::DebugUtilsMessengerEXT m_DebugMessenger = nullptr;
#endif

		VpCapabilities m_VpCapabilities = VK_NULL_HANDLE;

	private:
		static constexpr VpProfileProperties s_VpProfileProperties{
		    .profileName = VP_KHR_ROADMAP_2026_NAME, .specVersion = VP_KHR_ROADMAP_2026_SPEC_VERSION
		};
	};

}        // namespace Eruption::Vulkan
