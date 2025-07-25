#include "VulkanContext.h"

namespace Eruption
{
	namespace Utils
	{
		namespace
		{
			bool CheckDriverAPIVersionSupport(uint32_t minimumSupportedVersion)
			{
				const uint32_t instanceVersion = vk::enumerateInstanceVersion();
				if (instanceVersion < minimumSupportedVersion)
				{
					ER_CORE_FATAL("Incompatible Vulkan driver version!");
					ER_CORE_FATAL(
					    "\tYou have {}.{}.{}",
					    vk::apiVersionMajor(instanceVersion),
					    vk::apiVersionMinor(instanceVersion),
					    vk::apiVersionPatch(instanceVersion)
					);
					ER_CORE_FATAL(
					    "\tYou need at least {}.{}.{}",
					    vk::apiVersionMajor(minimumSupportedVersion),
					    vk::apiVersionMinor(minimumSupportedVersion),
					    vk::apiVersionPatch(minimumSupportedVersion)
					);

					return false;
				}

				return true;
			}

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

#ifdef ER_DEBUG
	void VulkanValidation::Init()
	{
		const std::vector instanceLayerProperties = vk::enumerateInstanceLayerProperties();
		ER_CORE_INFO_TAG("Renderer", "Vulkan Instance Layers:");
		for (const vk::LayerProperties& layer : instanceLayerProperties)
		{
			constexpr std::array VALIDATION_LAYERS = {
			    "VK_LAYER_KHRONOS_validation",
			};

			const char* layerName = layer.layerName;
			ER_CORE_INFO_TAG("Renderer", "\t{0}", layerName);

			for (const char* requestedLayer : VALIDATION_LAYERS)
			{
				if (strcmp(requestedLayer, layerName) == 0)
					m_Layers.emplace_back(requestedLayer);
			}
		}

		ER_CORE_INFO_TAG("Renderer", "Enabled Layers:");
		for (const char* layer : m_Layers)
			ER_CORE_INFO_TAG("Renderer", "\t{0}", layer);

		m_Extensions.push_back(vk::EXTDebugUtilsExtensionName);

		m_DebugMessengerCreateInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
		                                             vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
		                                             vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
		m_DebugMessengerCreateInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
		                                         vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
		                                         vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
		m_DebugMessengerCreateInfo.pfnUserCallback = Utils::VulkanDebugUtilsMessengerCallback;
		m_DebugMessengerCreateInfo.pUserData       = nullptr;
	}

	void VulkanValidation::Destroy(
	    vk::Instance vulkanInstance, const vk::detail::DispatchLoaderDynamic& dispatcher
	) const
	{
		vulkanInstance.destroyDebugUtilsMessengerEXT(m_DebugMessenger, nullptr, dispatcher);
	}

	void VulkanValidation::CreateDebugMessenger(
	    vk::Instance vulkanInstance, const vk::detail::DispatchLoaderDynamic& dispatcher
	)
	{
		if (m_DebugMessenger != VK_NULL_HANDLE)
			return;

		m_DebugMessenger = vulkanInstance.createDebugUtilsMessengerEXT(m_DebugMessengerCreateInfo, nullptr, dispatcher);
	}

#endif

	VulkanContext::~VulkanContext()
	{
		m_Allocator->Destroy();
		m_Device->Destroy();

#ifdef ER_DEBUG
		m_Validation->Destroy(m_VulkanInstance, *m_DispatchLoaderDynamic);
#endif

		m_VulkanInstance.destroy();
		m_VulkanInstance = VK_NULL_HANDLE;
	}

	void VulkanContext::Init(GLFWwindow* window)
	{
		ER_CORE_INFO_TAG("Renderer", "VulkanContext::Create");

		ER_CORE_ASSERT(glfwVulkanSupported(), "GLFW must support Vulkan!");

		if (!Utils::CheckDriverAPIVersionSupport(VK_API_VERSION_1_4))
		{
			ER_CORE_ERROR("Incompatible Vulkan driver version.\nUpdate your GPU drivers!");
			ER_CORE_VERIFY(false);
		}

		vk::ApplicationInfo appInfo{};
		appInfo.pApplicationName = "Eruption";
		appInfo.pEngineName      = "Eruption";
		appInfo.apiVersion       = VK_API_VERSION_1_4;

		uint32_t     glfwExtensionsCount = 0u;
		const char** glfwExtensions      = nullptr;
		glfwExtensions                   = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);

		std::vector<const char*> requiredExtension{glfwExtensions, glfwExtensions + glfwExtensionsCount};
		requiredExtension.push_back(vk::KHRGetPhysicalDeviceProperties2ExtensionName);

		vk::InstanceCreateInfo instanceCreateInfo{};
		instanceCreateInfo.pApplicationInfo = &appInfo;
#ifdef ER_DEBUG
		m_Validation = CreateScope<VulkanValidation>();
		m_Validation->Init();

		instanceCreateInfo.setPEnabledLayerNames(m_Validation->GetRequiredLayers());

		requiredExtension.append_range(m_Validation->GetRequiredExtensions());
		instanceCreateInfo.setPEnabledExtensionNames(requiredExtension);

		const vk::StructureChain instanceCreateChain{instanceCreateInfo, m_Validation->GetDebugMessengerCreateInfo()};
#else
		instanceCreateInfo.setPEnabledLayerNames({});
		instanceCreateInfo.setPEnabledExtensionNames(requiredExtension);

		const vk::StructureChain instanceCreateChain{instanceCreateInfo};
#endif

		m_VulkanInstance = vk::createInstance(instanceCreateChain.get<vk::InstanceCreateInfo>());

		// Create surface
		VkSurfaceKHR surface = VK_NULL_HANDLE;
		VK_CHECK_RESULT(glfwCreateWindowSurface(m_VulkanInstance, window, nullptr, &surface));
		m_Surface = vk::SurfaceKHR(surface);

		PhysicalDeviceRequirements requirements{};
		requirements.Extensions = {vk::KHRSwapchainExtensionName};

		requirements.Features.features.samplerAnisotropy                   = vk::True;
		requirements.Features.features.wideLines                           = vk::True;
		requirements.Features.features.fillModeNonSolid                    = vk::True;
		requirements.Features.features.independentBlend                    = vk::True;
		requirements.Features.features.pipelineStatisticsQuery             = vk::True;
		requirements.Features.features.shaderStorageImageReadWithoutFormat = vk::True;

		requirements.Surface = m_Surface;

		m_PhysicalDevice = VulkanPhysicalDevice::Select(requirements);

#ifdef ER_DEBUG
		m_Device = CreateRef<VulkanDevice>(m_PhysicalDevice);
#else
		m_Device = CreateRef<VulkanDevice>(m_PhysicalDevice);
#endif

		m_DispatchLoaderDynamic = CreateRef<vk::detail::DispatchLoaderDynamic>();
		m_DispatchLoaderDynamic->init(m_VulkanInstance, m_Device->GetVulkanDevice());

#ifdef ER_DEBUG
		m_Validation->CreateDebugMessenger(m_VulkanInstance, *m_DispatchLoaderDynamic);
#endif

		m_Allocator = CreateRef<VulkanAllocator>();
		m_Allocator->Init(m_VulkanInstance, m_Device);

		m_PipelineCache = m_Device->GetVulkanDevice().createPipelineCache(vk::PipelineCacheCreateInfo{});
	}
}        // namespace Eruption