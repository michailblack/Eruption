#include "Instance.h"

#include "Eruption/Core/Window.h"

#include <GLFW/glfw3.h>

namespace Eruption::Vulkan
{
	namespace
	{
#ifdef ER_DEBUG
		constexpr bool g_EnableValidationLayers = true;
#else
		constexpr bool g_EnableValidationLayers = false;
#endif

		VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugUtilsMessengerCallback(
		    vk::DebugUtilsMessageSeverityFlagBitsEXT      severity,
		    vk::DebugUtilsMessageTypeFlagsEXT             types,
		    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
		    [[maybe_unused]] void*                        pUserData
		)
		{
			std::string objects;
			for (uint32_t i = 0; i < pCallbackData->objectCount; ++i)
			{
				const vk::DebugUtilsObjectNameInfoEXT& object = pCallbackData->pObjects[i];
				objects.append(
				    std::format(
				        "\n\t\tObject[{0}] type: {1}, handle: {2:#x}, name: {3}",
				        i,
				        vk::to_string(object.objectType),
				        object.objectHandle,
				        object.pObjectName ? object.pObjectName : "NULL"
				    )
				);
			}

			const std::string message = std::format(
			    "{0}: {1}{2}", vk::to_string(types), pCallbackData->pMessage, objects
			);

			switch (severity)
			{
				case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
					ER_CORE_TRACE_TAG("Vulkan", "{0}", message);
					break;
				case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo: ER_CORE_INFO_TAG("Vulkan", "{0}", message); break;
				case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
					ER_CORE_WARN_TAG("Vulkan", "{0}", message);
					break;
				case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
					ER_CORE_ERROR_TAG("Vulkan", "{0}", message);
					break;
				default: break;
			}

			return VK_FALSE;
		}
	}        // namespace

	Instance::Instance(const Scope<Window>& window)
	{
		ER_CORE_INFO_TAG("Renderer", "Creating Vulkan instance (profile '{}')", s_Profile.profileName);

		ER_CORE_ASSERT(glfwVulkanSupported(), "GLFW reports Vulkan is not supported on this system!");

		constexpr VpCapabilitiesCreateInfo capabilitiesCreateInfo{
		    .flags = VP_PROFILE_CREATE_STATIC_BIT, .apiVersion = VK_API_VERSION_1_4, .pVulkanFunctions = nullptr
		};
		VK_CHECK_RESULT(vpCreateCapabilities(&capabilitiesCreateInfo, nullptr, &m_VpCapabilities));

		VkBool32 supported = VK_FALSE;
		VK_CHECK_RESULT(vpGetInstanceProfileSupport(m_VpCapabilities, nullptr, &s_Profile, &supported));
		if (!supported)
		{
			ER_CORE_FATAL(
			    "Required Vulkan profile '{}' is not supported at the instance level!", s_Profile.profileName
			);
			ER_CORE_FATAL("Update your GPU drivers / Vulkan runtime, or lower the target profile.");
			ER_CORE_VERIFY(false);
		}

		const std::vector<const char*> requiredExtensions = GetRequiredExtensions();
		const std::vector<const char*> requiredLayers     = GetRequiredLayers();

		constexpr vk::ApplicationInfo appInfo{
		    .pApplicationName   = "Eruption",
		    .applicationVersion = vk::makeApiVersion(0u, 1u, 0u, 0u),
		    .pEngineName        = "Eruption",
		    .engineVersion      = vk::makeApiVersion(0u, 1u, 0u, 0u),
		    .apiVersion         = VK_API_VERSION_1_4
		};

		const vk::InstanceCreateInfo instanceCreateInfo{
		    .pApplicationInfo        = &appInfo,
		    .enabledLayerCount       = static_cast<uint32_t>(requiredLayers.size()),
		    .ppEnabledLayerNames     = requiredLayers.data(),
		    .enabledExtensionCount   = static_cast<uint32_t>(requiredExtensions.size()),
		    .ppEnabledExtensionNames = requiredExtensions.data()
		};
		const VkInstanceCreateInfo rawInstanceCreateInfo = instanceCreateInfo;

		const VpInstanceCreateInfo vpInstanceCreateInfo{
		    .pCreateInfo = &rawInstanceCreateInfo, .enabledFullProfileCount = 1, .pEnabledFullProfiles = &s_Profile
		};

		VkInstance rawInstance = VK_NULL_HANDLE;
		VK_CHECK_RESULT(vpCreateInstance(m_VpCapabilities, &vpInstanceCreateInfo, nullptr, &rawInstance));

		m_Instance = vk::raii::Instance(m_Context, rawInstance);

		CreateDebugUtilsMessenger();
		CreateSurface(window);
	}

	Instance::~Instance()
	{
		if (m_VpCapabilities)
			vpDestroyCapabilities(m_VpCapabilities, nullptr);
	}

	std::vector<const char*> Instance::GetRequiredExtensions() const
	{
		uint32_t     glfwExtensionCount = 0;
		const char** glfwExtensions     = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		ER_CORE_ASSERT(glfwExtensions, "GLFW failed to report required Vulkan instance extensions!");

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if constexpr (g_EnableValidationLayers)
			extensions.emplace_back(vk::EXTDebugUtilsExtensionName);

		const std::vector<vk::ExtensionProperties> available = m_Context.enumerateInstanceExtensionProperties();
		for (const char* required : extensions)
		{
			const bool found = std::ranges::any_of(available, [required](const vk::ExtensionProperties& p) {
				return std::strcmp(p.extensionName, required) == 0;
			});

			if (!found)
				throw std::runtime_error(std::format("Required instance extension not supported: {}", required));
		}

		return extensions;
	}

	std::vector<const char*> Instance::GetRequiredLayers() const
	{
		std::vector<const char*> layers;

		if constexpr (g_EnableValidationLayers)
			layers.emplace_back("VK_LAYER_KHRONOS_validation");

		const std::vector<vk::LayerProperties> available = m_Context.enumerateInstanceLayerProperties();
		for (const char* required : layers)
		{
			const bool found = std::ranges::any_of(available, [required](const vk::LayerProperties& p) {
				return std::strcmp(p.layerName, required) == 0;
			});

			if (!found)
			{
				ER_CORE_WARN_TAG(
				    "Renderer", "Requested validation layer '{}' not available; continuing without it.", required
				);
				return {};
			}
		}

		return layers;
	}

	void Instance::CreateDebugUtilsMessenger()
	{
		if constexpr (!g_EnableValidationLayers)
			return;

		constexpr vk::DebugUtilsMessageSeverityFlagsEXT
		    severityFlags = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
		                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;

		constexpr vk::DebugUtilsMessageTypeFlagsEXT typeFlags = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
		                                                        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
		                                                        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;

		const vk::DebugUtilsMessengerCreateInfoEXT createInfo{
		    .messageSeverity = severityFlags, .messageType = typeFlags, .pfnUserCallback = DebugUtilsMessengerCallback
		};

#ifdef ER_DEBUG
		m_DebugMessenger = m_Instance.createDebugUtilsMessengerEXT(createInfo);
#endif
	}

	void Instance::CreateSurface(const Scope<Window>& window)
	{
		VkSurfaceKHR rawSurface = VK_NULL_HANDLE;
		if (glfwCreateWindowSurface(*m_Instance, window->GetNativeWindow(), nullptr, &rawSurface) != VK_SUCCESS)
			throw std::runtime_error("Failed to create window surface!");

		m_Surface = vk::raii::SurfaceKHR(m_Instance, rawSurface);
	}
}        // namespace Eruption::Vulkan
