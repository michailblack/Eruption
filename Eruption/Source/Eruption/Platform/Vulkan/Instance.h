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
		Instance(Instance&&)                 = delete;
		Instance& operator=(Instance&&)      = delete;

		[[nodiscard]] const vk::raii::Context&    GetContext() const { return m_Context; }
		[[nodiscard]] const vk::raii::Instance&   GetVkHandle() const { return m_Instance; }
		[[nodiscard]] const vk::raii::SurfaceKHR& GetSurface() const { return m_Surface; }

		[[nodiscard]] VpCapabilities GetVpCapabilities() const { return m_VpCapabilities; }

		[[nodiscard]] static constexpr const VpProfileProperties& GetProfile() { return s_Profile; }

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

		static constexpr VpProfileProperties s_Profile{
		    .profileName = VP_KHR_ROADMAP_2022_NAME, .specVersion = VP_KHR_ROADMAP_2022_SPEC_VERSION
		};
	};
}        // namespace Eruption::Vulkan
