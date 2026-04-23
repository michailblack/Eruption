#include "Instance.h"

#include "Eruption/Core/Window.h"

namespace Eruption::Vulkan
{
#ifdef ER_DEBUG
	constexpr bool g_EnableValidationLayers = true;
#elif
	constexpr bool g_EnableValidationLayers = false;
#endif

	namespace Utils
	{
		namespace
		{
			vk::Bool32 VulkanDebugUtilsMessengerCallback(
			    vk::DebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
			    vk::DebugUtilsMessageTypeFlagsEXT             messageTypes,
			    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
			    [[maybe_unused]] void*                        pUserData
			)
			{
				std::string labels, objects;
				if (pCallbackData->cmdBufLabelCount)
				{
					labels = std::format("\tLabels({}): \n", pCallbackData->cmdBufLabelCount);
					for (uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; ++i)
					{
						const auto&       label    = pCallbackData->pCmdBufLabels[i];
						const std::string colorStr = std::format(
						    "[ {}, {}, {}, {} ]", label.color[0], label.color[1], label.color[2], label.color[3]
						);
						labels.append(
						    std::format(
						        "\t\t- Command Buffer Label[{0}]: name: {1}, color: {2}\n",
						        i,
						        label.pLabelName ? label.pLabelName : "NULL",
						        colorStr
						    )
						);
					}
				}

				if (pCallbackData->objectCount)
				{
					objects = std::format("\tObjects({}): \n", pCallbackData->objectCount);
					for (uint32_t i = 0; i < pCallbackData->objectCount; ++i)
					{
						const auto& object = pCallbackData->pObjects[i];
						objects.append(
						    std::format(
						        "\t\t- Object[{0}] name: {1}, type: {2}, handle: {3:#x}\n",
						        i,
						        object.pObjectName ? object.pObjectName : "NULL",
						        vk::to_string(object.objectType),
						        object.objectHandle
						    )
						);
					}
				}

				const std::string message = std::format(
				    "{0} {1} message: \n\t{2}\n {3} {4}",
				    vk::to_string(messageTypes),
				    vk::to_string(messageSeverity),
				    pCallbackData->pMessage,
				    labels,
				    objects
				);

				switch (messageSeverity)
				{
					case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
						ER_CORE_TRACE_TAG("Validation", "{0}", message);
						break;
					case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
						ER_CORE_INFO_TAG("Validation", "{0}", message);
						break;
					case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
						ER_CORE_WARN_TAG("Validation", "{0}", message);
						break;
					case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
						ER_CORE_ERROR_TAG("Validation", "{0}", message);
						break;
				}

				return VK_FALSE;
			}
		}        // namespace
	}        // namespace Utils

	Instance::Instance(const Scope<Window>& window)
	{
		ER_CORE_INFO_TAG("Renderer", "Instance::Create");

		ER_CORE_ASSERT(glfwVulkanSupported(), "GLFW must support Vulkan!");

		constexpr VpCapabilitiesCreateInfo capabilitiesCreateInfo{
		    .flags = VP_PROFILE_CREATE_STATIC_BIT, .apiVersion = VK_API_VERSION_1_4, .pVulkanFunctions = nullptr
		};
		vpCreateCapabilities(&capabilitiesCreateInfo, nullptr, &m_VpCapabilities);

		VkBool32 supported = VK_FALSE;
		VK_CHECK_RESULT(vpGetInstanceProfileSupport(m_VpCapabilities, nullptr, &s_VpProfileProperties, &supported));
		if (!supported)
		{
			ER_CORE_FATAL("Required Vulkan profile '{}' is not supported!", VP_KHR_ROADMAP_2022_NAME);
			ER_CORE_FATAL("Update your GPU drivers or check hardware requirements.");
			ER_CORE_VERIFY(false);
		}

		const std::vector<const char*>& requiredExtensions = GetRequiredExtensions();
		const std::vector<const char*>& requiredLayers     = GetRequiredLayers();

		constexpr vk::ApplicationInfo appInfo{
		    .pApplicationName   = "Eruption",
		    .applicationVersion = vk::makeApiVersion(0, 1, 0, 0),
		    .pEngineName        = "Eruption",
		    .engineVersion      = vk::makeApiVersion(0, 1, 0, 0),
		    .apiVersion         = VP_KHR_ROADMAP_2026_MIN_API_VERSION
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
		    .pCreateInfo              = &rawInstanceCreateInfo,
		    .flags                    = 0,
		    .enabledFullProfileCount  = 1,
		    .pEnabledFullProfiles     = &s_VpProfileProperties,
		    .enabledProfileBlockCount = 0,
		    .pEnabledProfileBlocks    = nullptr
		};

		VkInstance rawInstance;
		VK_CHECK_RESULT(vpCreateInstance(m_VpCapabilities, &vpInstanceCreateInfo, nullptr, &rawInstance));

		m_Instance = vk::raii::Instance(m_Context, rawInstance);

		CreateDebugUtilsMessenger();

		CreateSurface(window);
	}

	Instance::~Instance()
	{
		// // m_Allocator->Destroy();
		// m_Device->Destroy();
		//
		// m_VulkanInstance.destroy();
		// m_VulkanInstance = VK_NULL_HANDLE;

		if (m_VpCapabilities)
			vpDestroyCapabilities(m_VpCapabilities, nullptr);
	}

	std::vector<const char*> Instance::GetRequiredExtensions() const
	{
		std::vector<const char*> requiredExtensions{};

		constexpr std::vector<const char*> requiredBaseExtensions{/*vk::KHRGetPhysicalDeviceProperties2ExtensionName*/};

		uint32_t     glfwExtensionsCount = 0u;
		const char** glfwExtensions      = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);

		const std::vector<const char*> requiredGLFWExtension{glfwExtensions, glfwExtensions + glfwExtensionsCount};

		requiredExtensions.reserve(requiredBaseExtensions.size() + requiredGLFWExtension.size());
		requiredExtensions.append_range(requiredGLFWExtension);
		requiredExtensions.append_range(requiredBaseExtensions);

		if constexpr (g_EnableValidationLayers)
		{
			requiredExtensions.reserve(requiredExtensions.size() + 1);
			requiredExtensions.emplace_back(vk::EXTDebugUtilsExtensionName);
		}

		auto       extensionProperties    = m_Context.enumerateInstanceExtensionProperties();
		const auto unsupportedExtensionIt = std::ranges::find_if(
		    requiredExtensions, [&extensionProperties](const char* requiredExtension) -> bool {
			    return std::ranges::none_of(
			        extensionProperties, [requiredExtension](const vk::ExtensionProperties& extensionProperty) -> bool {
				        return std::strcmp(extensionProperty.extensionName, requiredExtension) == 0;
			        }
			    );
		    }
		);

		if (unsupportedExtensionIt != requiredExtensions.end())
			throw std::runtime_error("Required extension not supported: " + std::string(*unsupportedExtensionIt));

		return requiredExtensions;
	}

	std::vector<const char*> Instance::GetRequiredLayers() const
	{
		std::vector<const char*> requiredLayers{};

		if constexpr (g_EnableValidationLayers)
		{
			constexpr std::array validationLayers = {"VK_LAYER_KHRONOS_validation"};
			requiredLayers.append_range(validationLayers);
		}

		auto       layerProperties    = m_Context.enumerateInstanceLayerProperties();
		const auto unsupportedLayerIt = std::ranges::find_if(
		    requiredLayers, [&layerProperties](const char* requiredLayer) -> bool {
			    return std::ranges::none_of(
			        layerProperties, [requiredLayer](const vk::LayerProperties& layerProperty) -> bool {
				        return strcmp(layerProperty.layerName, requiredLayer) == 0;
			        }
			    );
		    }
		);

		if (unsupportedLayerIt != requiredLayers.end())
			throw std::runtime_error("Required layer not supported: " + std::string(*unsupportedLayerIt));

		return requiredLayers;
	}

	void Instance::CreateDebugUtilsMessenger()
	{
		if constexpr (!g_EnableValidationLayers)
			return;

		constexpr vk::DebugUtilsMessageSeverityFlagsEXT
		    severityFlags = /*vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
		 | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |*/
		    vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;

		constexpr vk::DebugUtilsMessageTypeFlagsEXT
		    messageTypeFlags = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
		                       vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
		                       vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;

		constexpr vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
		    .messageSeverity = severityFlags,
		    .messageType     = messageTypeFlags,
		    .pfnUserCallback = Utils::VulkanDebugUtilsMessengerCallback,
		    .pUserData       = nullptr
		};

		m_DebugMessenger = m_Instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
	}

	void Instance::CreateSurface(const Scope<Window>& window)
	{
		VkSurfaceKHR rawSurface;
		if (glfwCreateWindowSurface(*m_Instance, window->GetNativeWindow(), nullptr, &rawSurface) != VK_SUCCESS)
			throw std::runtime_error("Failed to create surface!");

		m_Surface = vk::raii::SurfaceKHR(m_Instance, rawSurface);
	}
}        // namespace Eruption::Vulkan
