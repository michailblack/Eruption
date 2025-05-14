#include <vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <ranges>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

static constexpr uint32_t WINDOW_WIDTH{800u};
static constexpr uint32_t WINDOW_HEIGHT{600u};
static constexpr uint32_t MAX_FRAMES_IN_FLIGHT{3u};

#ifdef ER_DIST
static constexpr bool ENABLE_VALIDATION_LAYERS{false};
constexpr std::array  VALIDATION_LAYERS{""};
#else
static constexpr bool ENABLE_VALIDATION_LAYERS{true};
constexpr std::array  VALIDATION_LAYERS{"VK_LAYER_KHRONOS_validation"};
#endif

static constexpr std::array DEVICE_EXTENSIONS{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

struct Vertex
{
	glm::vec2 Position;
	glm::vec2 TexCoords;
	glm::vec3 Color;
};

struct PerView
{
	glm::mat4 Model;
	glm::mat4 View;
	glm::mat4 Projection;
};

static const std::vector<Vertex> VERTICES{
    {
     {-0.5f, -0.5f},
     {1.0f, 0.0f},
     {1.0f, 0.0f, 0.0f},
     },
    {
     {0.5f, -0.5f},
     {0.0f, 0.0f},
     {0.0f, 1.0f, 0.0f},
     },
    {
     {0.5f, 0.5f},
     {0.0f, 1.0f},
     {0.0f, 0.0f, 1.0f},
     },
    {
     {-0.5f, 0.5f},
     {1.0f, 1.0f},
     {1.0f, 1.0f, 1.0f},
     }
};

static const std::vector<uint16_t> INDICES{0, 1, 2, 2, 3, 0};

struct QueueFamilyIndices
{
	std::optional<uint32_t> GraphicsFamily;
	std::optional<uint32_t> PresentFamily;

	bool IsComplete()
	{
		return GraphicsFamily.has_value() && PresentFamily.has_value();
	}
};

struct SwapChainSupportDetails
{
	vk::SurfaceCapabilitiesKHR        Capabilities;
	std::vector<vk::SurfaceFormatKHR> Formats;
	std::vector<vk::PresentModeKHR>   PresentModes;
};

static std::vector<uint8_t> ReadFile(std::string_view fileName)
{
	std::ifstream file{fileName.data(), std::ios::ate | std::ios::binary};

	if (!file.is_open())
		throw std::runtime_error("Failed to open the file!");

	const size_t fileSize{static_cast<size_t>(file.tellg())};
	file.seekg(0);

	std::vector<uint8_t> buffer(fileSize);

	file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
	file.close();

	return buffer;
}

class HelloTriangleApplication
{
public:
	void Run()
	{
		InitWindow();
		InitVulkan();
		MainLoop();
		Cleanup();
	}

private:
	void InitWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		m_Window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", nullptr, nullptr);
		glfwSetWindowUserPointer(m_Window, this);
		glfwSetWindowSizeCallback(m_Window, FramebufferResizeCallback);
	}

	void InitVulkan()
	{
		PopulateVulkanDebugMessengerCreateInfo();
		CreateInstance();
		SetupDebugMessenger();
		CreateSurface();
		PickPhysicalDevice();
		CreateLogicalDevice();
		CreateSwapChain();
		CreateSwapChainImageViews();
		CreateRenderPass();
		CreateDescriptorSetLayout();
		CreateGraphicsPipeline();
		CreateFramebuffers();
		CreateCommandPool();
		CreateTextureImage();
		CreateTextureImageView();
		CreateTextureSampler();
		CreateVertexBuffer();
		CreateIndexBuffer();
		CreateUniformBuffers();
		CreateDescriptorPool();
		CreateDescriptorSets();
		CreateCommandBuffers();
		CreateSyncObjects();
	}

	void PopulateVulkanDebugMessengerCreateInfo()
	{
		m_DebugMessengerCreateInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
		                                             vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
		                                             vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
		m_DebugMessengerCreateInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
		                                         vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
		                                         vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
		m_DebugMessengerCreateInfo.pfnUserCallback = VulkanDebugCallback;
		m_DebugMessengerCreateInfo.pUserData       = nullptr;
	}

	void CreateInstance()
	{
		vk::ApplicationInfo appInfo{};
		appInfo.pApplicationName   = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName        = "No Engine";
		appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion         = VK_API_VERSION_1_0;

		const std::vector<const char*>& requiredExtension{GetRequiredExtensions()};

		vk::InstanceCreateInfo instanceCreateInfo{};
		instanceCreateInfo.pApplicationInfo        = &appInfo;
		instanceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(requiredExtension.size());
		instanceCreateInfo.ppEnabledExtensionNames = requiredExtension.data();
		instanceCreateInfo.enabledLayerCount       = 0u;
		instanceCreateInfo.pNext                   = nullptr;

		if constexpr (ENABLE_VALIDATION_LAYERS)
		{
			const std::vector<vk::LayerProperties> availableLayers{vk::enumerateInstanceLayerProperties()};
			for (const char* layerName : VALIDATION_LAYERS)
			{
				bool layerIsFound{std::ranges::any_of(availableLayers, [layerName](const auto& availableLayer) {
					return strcmp(layerName, availableLayer.layerName) == 0u;
				})};

				if (!layerIsFound)
					throw std::runtime_error("Validation layers requested, but not available!");
			}

			instanceCreateInfo.enabledLayerCount   = static_cast<uint32_t>(VALIDATION_LAYERS.size());
			instanceCreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

			instanceCreateInfo.pNext = &m_DebugMessengerCreateInfo;
		}

		if (vk::createInstance(&instanceCreateInfo, nullptr, &m_Instance) != vk::Result::eSuccess)
			throw std::runtime_error("Failed to create Vulkan instance!");
	}

	static std::vector<const char*> GetRequiredExtensions()
	{
		uint32_t     glfwExtensionsCount{0u};
		const char** glfwExtensions{nullptr};
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);

		std::vector<const char*> requiredExtension{glfwExtensions, glfwExtensions + glfwExtensionsCount};

		if (ENABLE_VALIDATION_LAYERS)
			requiredExtension.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		return requiredExtension;
	}

	void SetupDebugMessenger()
	{
		if constexpr (ENABLE_VALIDATION_LAYERS)
		{
			m_DebugMessengerDLDI.init(m_Instance, vkGetInstanceProcAddr);
			m_DebugMessenger =
			    m_Instance.createDebugUtilsMessengerEXT(m_DebugMessengerCreateInfo, nullptr, m_DebugMessengerDLDI);
		}
	}

	void DestroyDebugMessenger()
	{
		if constexpr (ENABLE_VALIDATION_LAYERS)
		{
			m_Instance.destroyDebugUtilsMessengerEXT(m_DebugMessenger, nullptr, m_DebugMessengerDLDI);
		}
	}

	void CreateSurface()
	{
		VkSurfaceKHR surfaceKHR{};
		if (glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &surfaceKHR) != VK_SUCCESS)
			throw std::runtime_error("Failed to create window surface!");
		m_Surface = vk::SurfaceKHR{surfaceKHR};
	}

	void PickPhysicalDevice()
	{
		const std::vector<vk::PhysicalDevice> physicalDevices{m_Instance.enumeratePhysicalDevices()};
		if (physicalDevices.empty())
			throw std::runtime_error("Failed to find GPUs with Vulkan support!");

		std::ranges::find_if(physicalDevices, [this](const auto& physicalDevice) {
			if (IsPhysicalDeviceSuitable(physicalDevice))
			{
				m_PhysicalDevice = physicalDevice;
				return true;
			}

			return false;
		});

		if (m_PhysicalDevice == VK_NULL_HANDLE)
			throw std::runtime_error("Failed to find a suitable GPU!");
	}

	bool IsPhysicalDeviceSuitable(vk::PhysicalDevice physicalDevice)
	{
		QueueFamilyIndices queueFamilyIndices{FindQueueFamilies(physicalDevice)};

		const bool extensionsAvailable{CheckPhysicalDeviceExtensionsSupport(physicalDevice)};

		bool swapChainAdequate{false};
		if (extensionsAvailable)
		{
			SwapChainSupportDetails swapChainSupportDetails{QuerySwapChainSupportDetailes(physicalDevice)};
			swapChainAdequate =
			    !swapChainSupportDetails.Formats.empty() && !swapChainSupportDetails.PresentModes.empty();
		}

		const vk::PhysicalDeviceFeatures supportedFeatures{physicalDevice.getFeatures()};

		return queueFamilyIndices.IsComplete() && extensionsAvailable && swapChainAdequate &&
		       supportedFeatures.samplerAnisotropy;
	}

	QueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice physicalDevice)
	{
		QueueFamilyIndices queueFamilyIndices{};

		const std::vector<vk::QueueFamilyProperties> queueFamilies{physicalDevice.getQueueFamilyProperties()};

		uint32_t queueFamilyIndex{0u};
		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
				queueFamilyIndices.GraphicsFamily = queueFamilyIndex;

			vk::Bool32 presentSupport{physicalDevice.getSurfaceSupportKHR(queueFamilyIndex, m_Surface)};
			if (presentSupport)
				queueFamilyIndices.PresentFamily = queueFamilyIndex;

			if (queueFamilyIndices.IsComplete())
				break;

			++queueFamilyIndex;
		}

		return queueFamilyIndices;
	}

	static bool CheckPhysicalDeviceExtensionsSupport(vk::PhysicalDevice physicalDevice)
	{
		std::vector<vk::ExtensionProperties> availableExtensions{
		    physicalDevice.enumerateDeviceExtensionProperties(nullptr)
		};

		std::set<std::string> requiredExtensions{DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end()};
		for (const auto& availableExtension : availableExtensions)
			requiredExtensions.erase(availableExtension.extensionName);

		return requiredExtensions.empty();
	}

	SwapChainSupportDetails QuerySwapChainSupportDetailes(vk::PhysicalDevice physicalDevice)
	{
		SwapChainSupportDetails swapChainSupportDetails{};
		swapChainSupportDetails.Formats      = physicalDevice.getSurfaceFormatsKHR(m_Surface);
		swapChainSupportDetails.PresentModes = physicalDevice.getSurfacePresentModesKHR(m_Surface);
		swapChainSupportDetails.Capabilities = physicalDevice.getSurfaceCapabilitiesKHR(m_Surface);

		return swapChainSupportDetails;
	}

	static vk::SurfaceFormatKHR
	    ChooseSwapChainSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableSurfaceFormats)
	{
		// for (const auto& availableSurfaceFormat : availableSurfaceFormats)
		// {
		// 	if (availableSurfaceFormat.format == vk::Format::eB8G8R8A8Srgb &&
		// availableSurfaceFormat.colorSpace ==
		// vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear) 		return
		// availableSurfaceFormat;
		// }
		const auto foundIt{std::ranges::find_if(availableSurfaceFormats, [](const auto& availableSurfaceFormat) {
			if (availableSurfaceFormat.format == vk::Format::eB8G8R8A8Srgb &&
			    availableSurfaceFormat.colorSpace == vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear)
				return true;

			return false;
		})};

		if (foundIt != availableSurfaceFormats.end())
			return *foundIt;

		return availableSurfaceFormats[0u];
	}

	static vk::PresentModeKHR ChooseSwapChainPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
	{
		vk::PresentModeKHR chosenPresentMode{vk::PresentModeKHR::eFifo};
		std::ranges::find_if(availablePresentModes, [&chosenPresentMode](const auto& availablePresentMode) {
			if (availablePresentMode == vk::PresentModeKHR::eMailbox)
			{
				chosenPresentMode = availablePresentMode;
				return true;
			}

			return false;
		});

		return chosenPresentMode;
	}

	VkExtent2D ChooseSwapChainExtent(const vk::SurfaceCapabilitiesKHR& surfaceCapabilities)
	{
		if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			return surfaceCapabilities.currentExtent;

		int width, height;
		glfwGetFramebufferSize(m_Window, &width, &height);

		VkExtent2D actualExtent{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

		actualExtent.width = std::clamp(
		    actualExtent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width
		);
		actualExtent.height = std::clamp(
		    actualExtent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height
		);

		return actualExtent;
	}

	void CreateLogicalDevice()
	{
		QueueFamilyIndices queueFamilyIndices{FindQueueFamilies(m_PhysicalDevice)};
		std::set<uint32_t> uniqueQueueFamilyIndices{
		    queueFamilyIndices.GraphicsFamily.value(), queueFamilyIndices.PresentFamily.value()
		};

		std::vector<vk::DeviceQueueCreateInfo> deviceQueueCreateInfos{};

		constexpr float queuePriority{1.0f};
		for (const uint32_t uniqueQueueFamilyIndex : uniqueQueueFamilyIndices)
		{
			vk::DeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.queueFamilyIndex = uniqueQueueFamilyIndex;
			queueCreateInfo.queueCount       = 1u;
			queueCreateInfo.pQueuePriorities = &queuePriority;

			deviceQueueCreateInfos.emplace_back(queueCreateInfo);
		}

		vk::PhysicalDeviceFeatures physicalDeviceFeatures{};
		physicalDeviceFeatures.samplerAnisotropy = vk::True;

		vk::DeviceCreateInfo logicalDeviceCreateInfo{};
		logicalDeviceCreateInfo.pQueueCreateInfos       = deviceQueueCreateInfos.data();
		logicalDeviceCreateInfo.queueCreateInfoCount    = static_cast<uint32_t>(deviceQueueCreateInfos.size());
		logicalDeviceCreateInfo.pEnabledFeatures        = &physicalDeviceFeatures;
		logicalDeviceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
		logicalDeviceCreateInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

		if constexpr (ENABLE_VALIDATION_LAYERS)
		{
			logicalDeviceCreateInfo.enabledLayerCount   = static_cast<uint32_t>(VALIDATION_LAYERS.size());
			logicalDeviceCreateInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
		}
		else
		{
			logicalDeviceCreateInfo.enabledLayerCount = 0u;
		}

		if (m_PhysicalDevice.createDevice(&logicalDeviceCreateInfo, nullptr, &m_Device) != vk::Result::eSuccess)
			throw std::runtime_error("Failed to create logical device!");

		m_GraphicsQueue = m_Device.getQueue(queueFamilyIndices.GraphicsFamily.value(), 0u);
		m_PresentQueue  = m_Device.getQueue(queueFamilyIndices.PresentFamily.value(), 0u);
	}

	void CreateSwapChain()
	{
		SwapChainSupportDetails swapChainSupportDetails{QuerySwapChainSupportDetailes(m_PhysicalDevice)};

		vk::SurfaceFormatKHR surfaceFormat{ChooseSwapChainSurfaceFormat(swapChainSupportDetails.Formats)};
		vk::PresentModeKHR   presentMode{ChooseSwapChainPresentMode(swapChainSupportDetails.PresentModes)};
		vk::Extent2D         extent{ChooseSwapChainExtent(swapChainSupportDetails.Capabilities)};

		uint32_t imageCount{swapChainSupportDetails.Capabilities.minImageCount + 1u};
		if (swapChainSupportDetails.Capabilities.maxImageCount > 0u &&
		    imageCount > swapChainSupportDetails.Capabilities.maxImageCount)
			imageCount = swapChainSupportDetails.Capabilities.maxImageCount;

		vk::SwapchainCreateInfoKHR createInfo{};
		createInfo.surface          = m_Surface;
		createInfo.minImageCount    = imageCount;
		createInfo.imageFormat      = surfaceFormat.format;
		createInfo.imageColorSpace  = surfaceFormat.colorSpace;
		createInfo.imageExtent      = extent;
		createInfo.imageArrayLayers = 1u;
		createInfo.imageUsage       = vk::ImageUsageFlagBits::eColorAttachment;

		QueueFamilyIndices queueFamilyIndices{FindQueueFamilies(m_PhysicalDevice)};
		const uint32_t     queueIndices[]{
            queueFamilyIndices.GraphicsFamily.value(), queueFamilyIndices.PresentFamily.value()
        };

		if (queueFamilyIndices.GraphicsFamily == queueFamilyIndices.PresentFamily)
		{
			createInfo.imageSharingMode      = vk::SharingMode::eExclusive;
			createInfo.queueFamilyIndexCount = 0u;
			createInfo.pQueueFamilyIndices   = nullptr;
		}
		else
		{
			createInfo.imageSharingMode      = vk::SharingMode::eConcurrent;
			createInfo.queueFamilyIndexCount = 2u;
			createInfo.pQueueFamilyIndices   = queueIndices;
		}

		createInfo.preTransform   = swapChainSupportDetails.Capabilities.currentTransform;
		createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		createInfo.presentMode    = presentMode;
		createInfo.clipped        = VK_TRUE;
		createInfo.oldSwapchain   = VK_NULL_HANDLE;

		m_SwapChain            = m_Device.createSwapchainKHR(createInfo);
		m_SwapChainImages      = m_Device.getSwapchainImagesKHR(m_SwapChain);
		m_SwapChainImageFormat = surfaceFormat.format;
		m_SwapChainExtent      = extent;
	}

	vk::ImageView CreateImageView(vk::Image textureImage, vk::Format format)
	{
		const vk::ImageViewCreateInfo imageViewCreateInfo(
		    {}, textureImage, vk::ImageViewType::e2D, format, {},
		    vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0u, 1u, 0u, 1u)
		);

		return m_Device.createImageView(imageViewCreateInfo);
	}

	void CreateSwapChainImageViews()
	{
		m_SwapChainImageViews.resize(m_SwapChainImages.size());

		for (uint32_t i{0u}; i < m_SwapChainImages.size(); ++i)
			m_SwapChainImageViews[i] = CreateImageView(m_SwapChainImages[i], m_SwapChainImageFormat);
	}

	void CreateRenderPass()
	{
		vk::AttachmentDescription colorAttachmentDescription{};
		colorAttachmentDescription.format         = m_SwapChainImageFormat;
		colorAttachmentDescription.samples        = vk::SampleCountFlagBits::e1;
		colorAttachmentDescription.loadOp         = vk::AttachmentLoadOp::eClear;
		colorAttachmentDescription.storeOp        = vk::AttachmentStoreOp::eStore;
		colorAttachmentDescription.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
		colorAttachmentDescription.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		colorAttachmentDescription.initialLayout  = vk::ImageLayout::eUndefined;
		colorAttachmentDescription.finalLayout    = vk::ImageLayout::ePresentSrcKHR;

		vk::AttachmentReference colorAttachmentReference{};
		colorAttachmentReference.attachment = 0u;
		colorAttachmentReference.layout     = vk::ImageLayout::eColorAttachmentOptimal;

		vk::SubpassDescription subpass{};
		subpass.pipelineBindPoint    = vk::PipelineBindPoint::eGraphics;
		subpass.inputAttachmentCount = 0u;
		subpass.pInputAttachments    = nullptr;
		subpass.colorAttachmentCount = 1u;
		subpass.pColorAttachments    = &colorAttachmentReference;

		vk::SubpassDependency subpassDependency{};
		subpassDependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
		subpassDependency.dstSubpass    = 0u;
		subpassDependency.srcStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		subpassDependency.srcAccessMask = {};
		subpassDependency.dstStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		subpassDependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

		vk::RenderPassCreateInfo renderPassCreateInfo{};
		renderPassCreateInfo.attachmentCount = 1u;
		renderPassCreateInfo.pAttachments    = &colorAttachmentDescription;
		renderPassCreateInfo.subpassCount    = 1u;
		renderPassCreateInfo.pSubpasses      = &subpass;
		renderPassCreateInfo.dependencyCount = 1u;
		renderPassCreateInfo.pDependencies   = &subpassDependency;

		m_HelloTriangleRenderPass = m_Device.createRenderPass(renderPassCreateInfo);
	}

	vk::ShaderModule CreateShaderModule(const std::vector<uint8_t>& shaderByteCode)
	{
		vk::ShaderModule shaderModule{VK_NULL_HANDLE};

		vk::ShaderModuleCreateInfo createInfo{};
		createInfo.pCode    = reinterpret_cast<const uint32_t*>(shaderByteCode.data());
		createInfo.codeSize = shaderByteCode.size();

		shaderModule = m_Device.createShaderModule(createInfo);

		return shaderModule;
	}

	void CreateDescriptorSetLayout()
	{
		constexpr std::array descriptorSetLayoutBindings{
		    vk::DescriptorSetLayoutBinding(
		        0u, vk::DescriptorType::eUniformBuffer, 1u, vk::ShaderStageFlagBits::eVertex
		    ),
		    vk::DescriptorSetLayoutBinding(
		        1u, vk::DescriptorType::eCombinedImageSampler, 1u, vk::ShaderStageFlagBits::eFragment
		    )
		};

		const vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCI(
		    {}, descriptorSetLayoutBindings.size(), descriptorSetLayoutBindings.data()
		);
		m_HelloTriangleDescriptorSetLayout = m_Device.createDescriptorSetLayout(descriptorSetLayoutCI);
	}

	void CreateGraphicsPipeline()
	{
		// Create shader modules
		const std::vector<uint8_t>& vertexShaderByteCode{ReadFile("Eruption-Core/Source/Shaders/vert.spv")};
		const std::vector<uint8_t>& fragmentShaderByteCode{ReadFile("Eruption-Core/Source/Shaders/frag.spv")};

		vk::ShaderModule vertexShaderModule{CreateShaderModule(vertexShaderByteCode)};
		vk::ShaderModule fragmentShaderModule{CreateShaderModule(fragmentShaderByteCode)};

		vk::PipelineShaderStageCreateInfo vertexShaderStageCreateInfo{};
		vertexShaderStageCreateInfo.stage  = vk::ShaderStageFlagBits::eVertex;
		vertexShaderStageCreateInfo.module = vertexShaderModule;
		vertexShaderStageCreateInfo.pName  = "main";

		vk::PipelineShaderStageCreateInfo fragmentShaderStageCreateInfo{};
		fragmentShaderStageCreateInfo.stage  = vk::ShaderStageFlagBits::eFragment;
		fragmentShaderStageCreateInfo.module = fragmentShaderModule;
		fragmentShaderStageCreateInfo.pName  = "main";

		vk::PipelineShaderStageCreateInfo shaderStagesCreateInfos[]{
		    vertexShaderStageCreateInfo, fragmentShaderStageCreateInfo
		};

		// Dynamic state
		constexpr std::array dynamicStates{vk::DynamicState::eViewport, vk::DynamicState::eScissor};

		vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
		dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicStateCreateInfo.pDynamicStates    = dynamicStates.data();

		// Vertex input
		vk::VertexInputBindingDescription vertexInputBingingDescription{};
		vertexInputBingingDescription.binding   = 0u;
		vertexInputBingingDescription.stride    = sizeof(Vertex);
		vertexInputBingingDescription.inputRate = vk::VertexInputRate::eVertex;

		constexpr std::array vertexInputAttributeDescriptions{
		    vk::VertexInputAttributeDescription{0u, 0u,    vk::Format::eR32G32Sfloat, offsetof(Vertex,  Position)},
		    vk::VertexInputAttributeDescription{1u, 0u,    vk::Format::eR32G32Sfloat, offsetof(Vertex, TexCoords)},
		    vk::VertexInputAttributeDescription{2u, 0u, vk::Format::eR32G32B32Sfloat, offsetof(Vertex,     Color)}
		};

		vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
		vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1u;
		vertexInputStateCreateInfo.pVertexBindingDescriptions    = &vertexInputBingingDescription;
		vertexInputStateCreateInfo.vertexAttributeDescriptionCount =
		    static_cast<uint32_t>(vertexInputAttributeDescriptions.size());
		vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data();

		// Input assembly
		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{};
		inputAssemblyStateCreateInfo.topology               = vk::PrimitiveTopology::eTriangleList;
		inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

		// Viewports and scissors
		vk::Viewport viewport{};
		viewport.x        = 0.0f;
		viewport.y        = 0.0f;
		viewport.width    = static_cast<float>(m_SwapChainExtent.width);
		viewport.height   = static_cast<float>(m_SwapChainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vk::Rect2D scissor{};
		scissor.offset = vk::Offset2D{0u, 0u};
		scissor.extent = m_SwapChainExtent;

		vk::PipelineViewportStateCreateInfo viewportStateCreateInfo{};
		viewportStateCreateInfo.viewportCount = 1u;
		viewportStateCreateInfo.scissorCount  = 1u;

		// Rasterization
		vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{};
		rasterizationStateCreateInfo.depthClampEnable        = VK_FALSE;
		rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterizationStateCreateInfo.polygonMode             = vk::PolygonMode::eFill;
		rasterizationStateCreateInfo.cullMode                = vk::CullModeFlagBits::eBack;
		rasterizationStateCreateInfo.frontFace               = vk::FrontFace::eCounterClockwise;
		rasterizationStateCreateInfo.depthBiasEnable         = VK_FALSE;
		rasterizationStateCreateInfo.lineWidth               = 1.0f;

		// Multisampling
		vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};
		multisampleStateCreateInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;
		multisampleStateCreateInfo.sampleShadingEnable  = VK_FALSE;

		// Blending
		vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.blendEnable    = VK_FALSE;
		colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
		                                      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

		vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
		colorBlendStateCreateInfo.logicOpEnable   = VK_FALSE;
		colorBlendStateCreateInfo.attachmentCount = 1u;
		colorBlendStateCreateInfo.pAttachments    = &colorBlendAttachment;

		// Pipeline layout
		vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
		pipelineLayoutCreateInfo.setLayoutCount = 1u;
		pipelineLayoutCreateInfo.pSetLayouts    = &m_HelloTriangleDescriptorSetLayout;

		m_HelloTrianglePipelineLayout = m_Device.createPipelineLayout(pipelineLayoutCreateInfo);

		vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
		graphicsPipelineCreateInfo.stageCount          = 2u;
		graphicsPipelineCreateInfo.pStages             = shaderStagesCreateInfos;
		graphicsPipelineCreateInfo.pVertexInputState   = &vertexInputStateCreateInfo;
		graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
		graphicsPipelineCreateInfo.pTessellationState  = nullptr;
		graphicsPipelineCreateInfo.pViewportState      = &viewportStateCreateInfo;
		graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
		graphicsPipelineCreateInfo.pMultisampleState   = &multisampleStateCreateInfo;
		graphicsPipelineCreateInfo.pDepthStencilState  = nullptr;
		graphicsPipelineCreateInfo.pColorBlendState    = &colorBlendStateCreateInfo;
		graphicsPipelineCreateInfo.pDynamicState       = &dynamicStateCreateInfo;
		graphicsPipelineCreateInfo.layout              = m_HelloTrianglePipelineLayout;
		graphicsPipelineCreateInfo.renderPass          = m_HelloTriangleRenderPass;
		graphicsPipelineCreateInfo.subpass             = 0u;

		const auto creationResult{m_Device.createGraphicsPipeline(VK_NULL_HANDLE, graphicsPipelineCreateInfo)};
		if (creationResult.result != vk::Result::eSuccess)
			throw std::runtime_error("Failed to create graphics pipeline!");
		m_HelloTrianglePipeline = creationResult.value;

		m_Device.destroyShaderModule(vertexShaderModule);
		m_Device.destroyShaderModule(fragmentShaderModule);
	}

	void CreateFramebuffers()
	{
		m_HelloTriangleFramebuffers.resize(m_SwapChainImageViews.size());

		for (uint32_t i{0u}; i < m_SwapChainImageViews.size(); ++i)
		{
			vk::FramebufferCreateInfo framebufferCreateInfo{};
			framebufferCreateInfo.renderPass      = m_HelloTriangleRenderPass;
			framebufferCreateInfo.attachmentCount = 1u;
			framebufferCreateInfo.pAttachments    = &m_SwapChainImageViews[i];
			framebufferCreateInfo.width           = m_SwapChainExtent.width;
			framebufferCreateInfo.height          = m_SwapChainExtent.height;
			framebufferCreateInfo.layers          = 1u;

			m_HelloTriangleFramebuffers[i] = m_Device.createFramebuffer(framebufferCreateInfo);
		}
	}

	void CreateCommandPool()
	{
		QueueFamilyIndices queueFamilyIndices{FindQueueFamilies(m_PhysicalDevice)};

		vk::CommandPoolCreateInfo commandPoolCreateInfo{};
		commandPoolCreateInfo.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
		commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndices.GraphicsFamily.value();

		m_CommandPool = m_Device.createCommandPool(commandPoolCreateInfo);
	}

	uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
	{
		const vk::PhysicalDeviceMemoryProperties memoryProperties{m_PhysicalDevice.getMemoryProperties()};

		for (uint32_t i{0u}; i < memoryProperties.memoryTypeCount; ++i)
		{
			const auto& type{memoryProperties.memoryTypes[i]};
			if (typeFilter & (1u << i) && (type.propertyFlags & properties) == properties)
				return i;
		}

		throw std::runtime_error("Failed to find memory type");
	}

	std::pair<vk::Buffer, vk::DeviceMemory>
	    CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
	{
		vk::Buffer       buffer{};
		vk::DeviceMemory bufferMemory{};

		const vk::BufferCreateInfo bufferCreateInfo{{}, size, usage, vk::SharingMode::eExclusive};
		buffer = m_Device.createBuffer(bufferCreateInfo);

		const vk::MemoryRequirements memoryRequirements{m_Device.getBufferMemoryRequirements(buffer)};
		const vk::MemoryAllocateInfo memoryAllocateInfo{
		    memoryRequirements.size, FindMemoryType(memoryRequirements.memoryTypeBits, properties)
		};

		bufferMemory = m_Device.allocateMemory(memoryAllocateInfo);
		m_Device.bindBufferMemory(buffer, bufferMemory, 0u);

		return std::make_pair(buffer, bufferMemory);
	}

	vk::CommandBuffer BeginSingleTimeCommands()
	{
		const vk::CommandBufferAllocateInfo commandBufferAI{m_CommandPool, vk::CommandBufferLevel::ePrimary, 1u};
		vk::CommandBuffer                   commandBuffer{m_Device.allocateCommandBuffers(commandBufferAI)[0u]};

		commandBuffer.begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

		return commandBuffer;
	}

	void EndSingleTimeCommands(vk::CommandBuffer commandBuffer)
	{
		commandBuffer.end();

		vk::SubmitInfo submitInfo{};
		submitInfo.commandBufferCount = 1u;
		submitInfo.pCommandBuffers    = &commandBuffer;
		m_GraphicsQueue.submit({submitInfo});
		m_GraphicsQueue.waitIdle();

		m_Device.freeCommandBuffers(m_CommandPool, {commandBuffer});
	}

	void CopyBuffers(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size)
	{
		vk::CommandBuffer commandBuffer{BeginSingleTimeCommands()};

		commandBuffer.copyBuffer(
		    srcBuffer, dstBuffer,
		    {
		        vk::BufferCopy{0u, 0u, size}
        }
		);

		EndSingleTimeCommands(commandBuffer);
	}

	std::pair<vk::Image, vk::DeviceMemory> CreateImage2D(
	    uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage,
	    vk::MemoryPropertyFlags properties
	)
	{
		vk::Image        image{VK_NULL_HANDLE};
		vk::DeviceMemory imageMemory{VK_NULL_HANDLE};

		const vk::ImageCreateInfo imageCreateInfo(
		    {}, vk::ImageType::e2D, format, vk::Extent3D(width, height, 1u), 1u, 1u, vk::SampleCountFlagBits::e1,
		    tiling, usage, vk::SharingMode::eExclusive, 0u, nullptr, vk::ImageLayout::eUndefined
		);

		image = m_Device.createImage(imageCreateInfo);

		const vk::MemoryRequirements memoryRequirements{m_Device.getImageMemoryRequirements(image)};
		const vk::MemoryAllocateInfo allocateInfo(
		    memoryRequirements.size, FindMemoryType(memoryRequirements.memoryTypeBits, properties)
		);

		imageMemory = m_Device.allocateMemory(allocateInfo);
		m_Device.bindImageMemory(image, imageMemory, 0u);

		return {image, imageMemory};
	}

	void TransitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
	{
		vk::CommandBuffer commandBuffer{BeginSingleTimeCommands()};

		vk::ImageMemoryBarrier barrier(
		    {}, {}, oldLayout, newLayout, vk::QueueFamilyIgnored, vk::QueueFamilyIgnored, image,
		    vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0u, 1u, 0u, 1u)
		);

		vk::PipelineStageFlags srcStageMask{};
		vk::PipelineStageFlags dstStageMask{};

		if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
		{
			barrier.srcAccessMask = {};
			barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

			srcStageMask = vk::PipelineStageFlagBits::eTopOfPipe;
			dstStageMask = vk::PipelineStageFlagBits::eTransfer;
		}
		else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
		         newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
		{
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

			srcStageMask = vk::PipelineStageFlagBits::eTransfer;
			dstStageMask = vk::PipelineStageFlagBits::eFragmentShader;
		}
		else
		{
			throw std::invalid_argument("Unsupported layout transition!");
		}

		commandBuffer.pipelineBarrier(srcStageMask, dstStageMask, {}, {}, {}, {barrier});

		EndSingleTimeCommands(commandBuffer);
	}

	void CopyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height)
	{
		vk::CommandBuffer commandBuffer{BeginSingleTimeCommands()};

		const vk::BufferImageCopy region(
		    0u, 0u, 0u, vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0u, 0u, 1u), vk::Offset3D(0, 0, 0),
		    vk::Extent3D(width, height, 1u)
		);
		commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, {region});

		EndSingleTimeCommands(commandBuffer);
	}

	void CreateTextureImage()
	{
		int textureWidth{0}, textureHeight{0}, textureChannels{0};

        // INFO: texture is not uploaded to git
		stbi_uc* pixels{stbi_load(
		    "Eruption-Core/Source/Textures/texture.jpg", &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha
		)};

		const VkDeviceSize imageSize{textureWidth * textureHeight * 4u};

		if (!pixels)
			throw std::runtime_error("Failed to load texture image!");

		vk::Buffer       stagingBuffer{};
		vk::DeviceMemory stagingBufferMemory{};
		std::tie(stagingBuffer, stagingBufferMemory) = CreateBuffer(
		    imageSize, vk::BufferUsageFlagBits::eTransferSrc,
		    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
		);

		auto* data{m_Device.mapMemory(stagingBufferMemory, 0u, imageSize)};
		memcpy(data, pixels, imageSize);
		m_Device.unmapMemory(stagingBufferMemory);

		stbi_image_free(pixels);

		std::tie(m_TextureImage, m_TextureImageMemory) = CreateImage2D(
		    static_cast<uint32_t>(textureWidth), static_cast<uint32_t>(textureHeight), vk::Format::eR8G8B8A8Srgb,
		    vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
		    vk::MemoryPropertyFlagBits::eDeviceLocal
		);

		TransitionImageLayout(
		    m_TextureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal
		);
		CopyBufferToImage(
		    stagingBuffer, m_TextureImage, static_cast<uint32_t>(textureWidth), static_cast<uint32_t>(textureHeight)
		);
		TransitionImageLayout(
		    m_TextureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eTransferDstOptimal,
		    vk::ImageLayout::eShaderReadOnlyOptimal
		);

		m_Device.destroyBuffer(stagingBuffer);
		m_Device.freeMemory(stagingBufferMemory);
	}

	void CreateTextureImageView()
	{
		m_TextureImageView = CreateImageView(m_TextureImage, vk::Format::eR8G8B8A8Srgb);
	}

	void CreateTextureSampler()
	{
		const vk::PhysicalDeviceProperties properties{m_PhysicalDevice.getProperties()};

		vk::SamplerCreateInfo samplerCreateInfo{};
		samplerCreateInfo.magFilter               = vk::Filter::eLinear;
		samplerCreateInfo.minFilter               = vk::Filter::eLinear;
		samplerCreateInfo.mipmapMode              = vk::SamplerMipmapMode::eLinear;
		samplerCreateInfo.addressModeU            = vk::SamplerAddressMode::eRepeat;
		samplerCreateInfo.addressModeV            = vk::SamplerAddressMode::eRepeat;
		samplerCreateInfo.addressModeW            = vk::SamplerAddressMode::eRepeat;
		samplerCreateInfo.mipLodBias              = 0.0f;
		samplerCreateInfo.anisotropyEnable        = vk::True;
		samplerCreateInfo.maxAnisotropy           = properties.limits.maxSamplerAnisotropy;
		samplerCreateInfo.compareEnable           = vk::False;
		samplerCreateInfo.compareOp               = vk::CompareOp::eAlways;
		samplerCreateInfo.minLod                  = 0.0f;
		samplerCreateInfo.maxLod                  = 0.0f;
		samplerCreateInfo.borderColor             = vk::BorderColor::eIntOpaqueBlack;
		samplerCreateInfo.unnormalizedCoordinates = vk::False;

		m_TextureSampler = m_Device.createSampler(samplerCreateInfo);
	}

	void CreateVertexBuffer()
	{
		const vk::DeviceSize bufferSize{sizeof(VERTICES[0u]) * VERTICES.size()};

		vk::Buffer       stagingBuffer{};
		vk::DeviceMemory stagingBufferMemory{};
		std::tie(stagingBuffer, stagingBufferMemory) = CreateBuffer(
		    bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
		    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
		);

		void* mapData{m_Device.mapMemory(stagingBufferMemory, 0u, bufferSize)};
		memcpy(mapData, VERTICES.data(), bufferSize);
		m_Device.unmapMemory(stagingBufferMemory);

		std::tie(m_VertexBuffer, m_VertexBufferMemory) = CreateBuffer(
		    bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
		    vk::MemoryPropertyFlagBits::eDeviceLocal
		);

		CopyBuffers(stagingBuffer, m_VertexBuffer, bufferSize);

		m_Device.destroyBuffer(stagingBuffer);
		m_Device.freeMemory(stagingBufferMemory);
	}

	void CreateIndexBuffer()
	{
		const vk::DeviceSize bufferSize{sizeof(INDICES[0u]) * INDICES.size()};

		vk::Buffer       stagingBuffer{};
		vk::DeviceMemory stagingBufferMemory{};
		std::tie(stagingBuffer, stagingBufferMemory) = CreateBuffer(
		    bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
		    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
		);

		void* mapData{m_Device.mapMemory(stagingBufferMemory, 0u, bufferSize)};
		memcpy(mapData, INDICES.data(), bufferSize);
		m_Device.unmapMemory(stagingBufferMemory);

		std::tie(m_IndexBuffer, m_IndexBufferMemory) = CreateBuffer(
		    bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
		    vk::MemoryPropertyFlagBits::eDeviceLocal
		);

		CopyBuffers(stagingBuffer, m_IndexBuffer, bufferSize);

		m_Device.destroyBuffer(stagingBuffer);
		m_Device.freeMemory(stagingBufferMemory);
	}

	void CreateUniformBuffers()
	{
		const vk::DeviceSize buffersSize{sizeof(PerView)};

		m_PerViewUBOs.resize(MAX_FRAMES_IN_FLIGHT);
		m_PerViewUBOsMemory.resize(MAX_FRAMES_IN_FLIGHT);
		m_PerViewUBOsMapped.resize(MAX_FRAMES_IN_FLIGHT);

		for (auto&& [ubo, uboMemory, uboMapped] :
		     std::ranges::zip_view(m_PerViewUBOs, m_PerViewUBOsMemory, m_PerViewUBOsMapped))
		{
			std::tie(ubo, uboMemory) = CreateBuffer(
			    buffersSize, vk::BufferUsageFlagBits::eUniformBuffer,
			    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
			);

			uboMapped = reinterpret_cast<PerView*>(m_Device.mapMemory(uboMemory, 0u, buffersSize));
		}
	}

	void CreateDescriptorPool()
	{
		constexpr std::array descriptorPoolSizes{
		    vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)                                   },
		    vk::DescriptorPoolSize{
		                           vk::DescriptorType::eCombinedImageSampler, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)}
		};

		const vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo{
		    {}, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT), descriptorPoolSizes.size(), descriptorPoolSizes.data()
		};

		m_DescriptorPool = m_Device.createDescriptorPool(descriptorPoolCreateInfo);
	}

	void CreateDescriptorSets()
	{
		const std::vector<vk::DescriptorSetLayout> descriptorSetLayouts(
		    MAX_FRAMES_IN_FLIGHT, m_HelloTriangleDescriptorSetLayout
		);

		const vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo{
		    m_DescriptorPool, static_cast<uint32_t>(descriptorSetLayouts.size()), descriptorSetLayouts.data()
		};

		m_DescriptorSets = m_Device.allocateDescriptorSets(descriptorSetAllocateInfo);

		for (uint32_t i{0u}; i < m_DescriptorSets.size(); ++i)
		{
			const vk::DescriptorBufferInfo descriptorBufferInfo(m_PerViewUBOs[i], 0u, sizeof(PerView));
			const vk::DescriptorImageInfo  descriptorImageInfo(
                m_TextureSampler, m_TextureImageView, vk::ImageLayout::eShaderReadOnlyOptimal
            );

			const std::array descriptorWriteSets{
			    vk::WriteDescriptorSet(
			        m_DescriptorSets[i], 0u, 0u, 1u, vk::DescriptorType::eUniformBuffer, nullptr, &descriptorBufferInfo
			    ),
			    vk::WriteDescriptorSet(
			        m_DescriptorSets[i], 1u, 0u, 1u, vk::DescriptorType::eCombinedImageSampler, &descriptorImageInfo
			    ),
			};

			m_Device.updateDescriptorSets(descriptorWriteSets, {});
		}
	}

	void CreateCommandBuffers()
	{
		vk::CommandBufferAllocateInfo commandBufferAllocateInfo{};
		commandBufferAllocateInfo.commandPool        = m_CommandPool;
		commandBufferAllocateInfo.level              = vk::CommandBufferLevel::ePrimary;
		commandBufferAllocateInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

		m_CommandBuffers = m_Device.allocateCommandBuffers(commandBufferAllocateInfo);
	}

	void RecordCommandBuffer(uint32_t imageIndex)
	{
		auto& currentCommandBuffer{m_CommandBuffers[m_CurrentFrameIndex]};

		vk::CommandBufferBeginInfo commandBufferBeginInfo{};
		currentCommandBuffer.begin(commandBufferBeginInfo);

		vk::ClearValue clearValue{
		    vk::ClearColorValue{0.0f, 0.0f, 0.0f, 0.0f}
		};
		vk::RenderPassBeginInfo renderPassBeginInfo{};
		renderPassBeginInfo.renderPass        = m_HelloTriangleRenderPass;
		renderPassBeginInfo.framebuffer       = m_HelloTriangleFramebuffers[imageIndex];
		renderPassBeginInfo.renderArea.offset = vk::Offset2D{0, 0};
		renderPassBeginInfo.renderArea.extent = m_SwapChainExtent;
		renderPassBeginInfo.clearValueCount   = 1u;
		renderPassBeginInfo.pClearValues      = &clearValue;

		currentCommandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
		currentCommandBuffer.bindIndexBuffer(m_IndexBuffer, 0u, vk::IndexType::eUint16);
		currentCommandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_HelloTrianglePipeline);
		currentCommandBuffer.bindVertexBuffers(0u, {m_VertexBuffer}, {0u});
		currentCommandBuffer.bindDescriptorSets(
		    vk::PipelineBindPoint::eGraphics, m_HelloTrianglePipelineLayout, 0u,
		    {m_DescriptorSets[m_CurrentFrameIndex]}, {}
		);

		vk::Viewport viewport{};
		viewport.x        = 0.0f;
		viewport.y        = 0.0f;
		viewport.width    = static_cast<float>(m_SwapChainExtent.width);
		viewport.height   = static_cast<float>(m_SwapChainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		currentCommandBuffer.setViewport(0u, 1u, &viewport);

		vk::Rect2D scissor{};
		scissor.offset = vk::Offset2D{0u, 0u};
		scissor.extent = m_SwapChainExtent;
		currentCommandBuffer.setScissor(0u, 1u, &scissor);

		currentCommandBuffer.drawIndexed(std::size(INDICES), 1u, 0u, 0u, 0u);

		currentCommandBuffer.endRenderPass();

		currentCommandBuffer.end();
	}

	void CreateSyncObjects()
	{
		vk::SemaphoreCreateInfo semaphoreCreateInfo{};

		vk::FenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;

		m_SwapChainImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		for (uint32_t i{0u}; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			m_SwapChainImageAvailableSemaphores[i] = m_Device.createSemaphore(semaphoreCreateInfo);
			m_RenderFinishedSemaphores[i]          = m_Device.createSemaphore(semaphoreCreateInfo);
			m_InFlightFences[i]                    = m_Device.createFence(fenceCreateInfo);
		}
	}

	void CleanupSwapChain()
	{
		for (auto framebuffer : m_HelloTriangleFramebuffers)
			m_Device.destroyFramebuffer(framebuffer);

		for (auto imageView : m_SwapChainImageViews)
			m_Device.destroyImageView(imageView);

		m_Device.destroySwapchainKHR(m_SwapChain);
	}

	void RecreateSwapChain()
	{
		int width{0}, height{0};
		glfwGetWindowSize(m_Window, &width, &height);
		while (width == 0 || height == 0)
		{
			glfwGetWindowSize(m_Window, &width, &height);
			glfwPollEvents();
		}

		m_Device.waitIdle();

		CleanupSwapChain();

		CreateSwapChain();
		CreateSwapChainImageViews();
		CreateFramebuffers();
	}

	void MainLoop()
	{
		while (!glfwWindowShouldClose(m_Window))
		{
			glfwPollEvents();
			DrawFrame();
		}

		m_Device.waitIdle();
	}

	void DrawFrame()
	{
		if (m_Device.waitForFences({m_InFlightFences[m_CurrentFrameIndex]}, VK_TRUE, UINT64_MAX) !=
		    vk::Result::eSuccess)
			throw std::runtime_error("Failed to wait for fences");

		const auto acquireResult{m_Device.acquireNextImageKHR(
		    m_SwapChain, UINT64_MAX, m_SwapChainImageAvailableSemaphores[m_CurrentFrameIndex]
		)};
		if (acquireResult.result == vk::Result::eErrorOutOfDateKHR)
		{
			RecreateSwapChain();
			return;
		}
		else if (acquireResult.result != vk::Result::eSuccess && acquireResult.result != vk::Result::eSuboptimalKHR)
			throw std::runtime_error("Failed to acquire next framebuffer image");
		const uint32_t imageIndex{acquireResult.value};

		m_Device.resetFences({m_InFlightFences[m_CurrentFrameIndex]});

		m_CommandBuffers[m_CurrentFrameIndex].reset();
		RecordCommandBuffer(imageIndex);

		constexpr std::array waitStages{vk::PipelineStageFlags{vk::PipelineStageFlagBits::eColorAttachmentOutput}};
		const std::array     waitSemaphores{m_SwapChainImageAvailableSemaphores[m_CurrentFrameIndex]};

		UpdateUniformBuffer();

		vk::SubmitInfo submitInfo{};
		submitInfo.waitSemaphoreCount = 1u;
		submitInfo.pWaitSemaphores    = waitSemaphores.data();
		submitInfo.pWaitDstStageMask  = waitStages.data();
		submitInfo.commandBufferCount = 1u;
		submitInfo.pCommandBuffers    = &m_CommandBuffers[m_CurrentFrameIndex];

		const std::array signalSemaphores{m_RenderFinishedSemaphores[m_CurrentFrameIndex]};
		submitInfo.signalSemaphoreCount = 1u;
		submitInfo.pSignalSemaphores    = signalSemaphores.data();

		m_GraphicsQueue.submit({submitInfo}, m_InFlightFences[m_CurrentFrameIndex]);

		const std::array swapChains{m_SwapChain};

		vk::PresentInfoKHR presentInfo{};
		presentInfo.waitSemaphoreCount = 1u;
		presentInfo.pWaitSemaphores    = signalSemaphores.data();
		presentInfo.swapchainCount     = 1u;
		presentInfo.pSwapchains        = swapChains.data();
		presentInfo.pImageIndices      = &imageIndex;

		const vk::Result presentResult{m_PresentQueue.presentKHR(presentInfo)};
		if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR ||
		    m_FrameBufferHasResized)
		{
			m_FrameBufferHasResized = false;
			RecreateSwapChain();
		}
		else if (presentResult != vk::Result::eSuccess)
			throw std::runtime_error("Failed to present queue");

		m_CurrentFrameIndex = ++m_CurrentFrameIndex % MAX_FRAMES_IN_FLIGHT;
	}

	void UpdateUniformBuffer()
	{
		auto& currentPerView{m_PerViewUBOs[m_CurrentFrameIndex]};
		auto& currentPerViewMapped{m_PerViewUBOsMapped[m_CurrentFrameIndex]};

		static auto startTime{std::chrono::high_resolution_clock::now()};

		auto        currentTime{std::chrono::high_resolution_clock::now()};
		const float time{std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count()};

		currentPerViewMapped->Model =
		    glm::rotate(glm::mat4{1.0f}, time * glm::radians(90.0f), glm::vec3{0.0f, 0.0f, 1.0f});
		currentPerViewMapped->View =
		    glm::lookAt(glm::vec3{2.0f, 2.0f, 2.0f}, glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.0f, 0.0f, 1.0f});
		currentPerViewMapped->Projection = glm::perspective(
		    glm::radians(45.0f),
		    static_cast<float>(m_SwapChainExtent.width) / static_cast<float>(m_SwapChainExtent.height), 0.1f, 10.0f
		);
		currentPerViewMapped->Projection[1][1] *= -1.0f;
	}

	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		auto* app{reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window))};
		app->m_FrameBufferHasResized = true;
	}

	void Cleanup()
	{
		for (uint32_t i{0u}; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			m_Device.destroySemaphore(m_SwapChainImageAvailableSemaphores[i]);
			m_Device.destroySemaphore(m_RenderFinishedSemaphores[i]);
			m_Device.destroyFence(m_InFlightFences[i]);
		}

		m_Device.destroyCommandPool(m_CommandPool);

		m_Device.destroyPipeline(m_HelloTrianglePipeline);
		m_Device.destroyPipelineLayout(m_HelloTrianglePipelineLayout);
		m_Device.destroyRenderPass(m_HelloTriangleRenderPass);

		CleanupSwapChain();

		m_Device.destroySampler(m_TextureSampler);

		m_Device.destroyImageView(m_TextureImageView);
		m_Device.destroyImage(m_TextureImage);
		m_Device.freeMemory(m_TextureImageMemory);

		for (auto&& [ubo, uboMemory] : std::ranges::zip_view(m_PerViewUBOs, m_PerViewUBOsMemory))
		{
			m_Device.destroyBuffer(ubo);
			m_Device.freeMemory(uboMemory);
		}

		m_Device.destroyDescriptorPool(m_DescriptorPool);

		m_Device.destroyDescriptorSetLayout(m_HelloTriangleDescriptorSetLayout);

		m_Device.destroyBuffer(m_VertexBuffer);
		m_Device.freeMemory(m_VertexBufferMemory);

		m_Device.destroyBuffer(m_IndexBuffer);
		m_Device.freeMemory(m_IndexBufferMemory);

		m_Device.destroy();
		m_Instance.destroySurfaceKHR(m_Surface);

		if constexpr (ENABLE_VALIDATION_LAYERS)
			DestroyDebugMessenger();

		m_Instance.destroy();

		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}

	static vk::Bool32 VulkanDebugCallback(
	    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity, vk::DebugUtilsMessageTypeFlagsEXT messageTypes,
	    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData
	)
	{
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}

private:
	GLFWwindow*  m_Window{nullptr};
	vk::Instance m_Instance{VK_NULL_HANDLE};

	vk::detail::DispatchLoaderDynamic    m_DebugMessengerDLDI{};
	vk::DebugUtilsMessengerCreateInfoEXT m_DebugMessengerCreateInfo{};
	vk::DebugUtilsMessengerEXT           m_DebugMessenger{VK_NULL_HANDLE};

	vk::SurfaceKHR m_Surface{VK_NULL_HANDLE};

	vk::PhysicalDevice m_PhysicalDevice{VK_NULL_HANDLE};

	vk::Device m_Device{VK_NULL_HANDLE};

	vk::Queue m_GraphicsQueue{VK_NULL_HANDLE};
	vk::Queue m_PresentQueue{VK_NULL_HANDLE};

	vk::SwapchainKHR           m_SwapChain{VK_NULL_HANDLE};
	std::vector<vk::Image>     m_SwapChainImages{};
	std::vector<vk::ImageView> m_SwapChainImageViews{};
	vk::Format                 m_SwapChainImageFormat{};
	vk::Extent2D               m_SwapChainExtent{};
	bool                       m_FrameBufferHasResized{false};

	vk::RenderPass               m_HelloTriangleRenderPass{VK_NULL_HANDLE};
	vk::DescriptorSetLayout      m_HelloTriangleDescriptorSetLayout{VK_NULL_HANDLE};
	vk::PipelineLayout           m_HelloTrianglePipelineLayout{VK_NULL_HANDLE};
	vk::Pipeline                 m_HelloTrianglePipeline{VK_NULL_HANDLE};
	std::vector<vk::Framebuffer> m_HelloTriangleFramebuffers{};

	vk::Buffer       m_VertexBuffer{};
	vk::DeviceMemory m_VertexBufferMemory{};

	vk::Buffer       m_IndexBuffer{};
	vk::DeviceMemory m_IndexBufferMemory{};

	std::vector<vk::Buffer>       m_PerViewUBOs{};
	std::vector<vk::DeviceMemory> m_PerViewUBOsMemory{};
	std::vector<PerView*>         m_PerViewUBOsMapped{};

	vk::Image        m_TextureImage{VK_NULL_HANDLE};
	vk::DeviceMemory m_TextureImageMemory{VK_NULL_HANDLE};
	vk::ImageView    m_TextureImageView{VK_NULL_HANDLE};
	vk::Sampler      m_TextureSampler{VK_NULL_HANDLE};

	vk::CommandPool                m_CommandPool{VK_NULL_HANDLE};
	std::vector<vk::CommandBuffer> m_CommandBuffers{};

	vk::DescriptorPool             m_DescriptorPool{VK_NULL_HANDLE};
	std::vector<vk::DescriptorSet> m_DescriptorSets{};

	std::vector<vk::Semaphore> m_SwapChainImageAvailableSemaphores{};
	std::vector<vk::Semaphore> m_RenderFinishedSemaphores{};
	std::vector<vk::Fence>     m_InFlightFences{};

	uint32_t m_CurrentFrameIndex{0u};
};

int main()
{
	HelloTriangleApplication app{};

	try
	{
		app.Run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
