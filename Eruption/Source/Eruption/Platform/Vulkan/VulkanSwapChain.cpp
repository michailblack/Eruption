#include "VulkanSwapChain.h"

#include "Eruption/Platform/Vulkan/VulkanContext.h"

namespace Eruption
{
	VulkanSwapChain::VulkanSwapChain(const SwapChainSpecification& spec) : m_Specification(spec)
	{
		ER_CORE_ASSERT(m_Specification.Surface, "Invalid surface provided to swap chain!");

		m_SupportDetails = QuerySupport(
		    VulkanContext::GetCurrentDevice()->GetPhysicalDevice()->GetVulkanPhysicalDevice(), m_Specification.Surface
		);

		ER_CORE_ASSERT(
		    !m_SupportDetails.Formats.empty() && !m_SupportDetails.PresentModes.empty(),
		    "Swap chain support is inadequate!"
		);

		Create();
	}

	VulkanSwapChain::~VulkanSwapChain()
	{
		Destroy();
	}

	std::expected<uint32_t, vk::Result> VulkanSwapChain::AcquireNextImage(
	    vk::Semaphore signalSemaphore, vk::Fence signalFence, uint64_t timeout
	)
	{
		const vk::Device vkDevice = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		uint32_t imageIndex;

		const vk::Result result = vkDevice.acquireNextImageKHR(
		    m_SwapChain, timeout, signalSemaphore, signalFence, &imageIndex
		);

		if (result == vk::Result::eSuccess)
		{
			m_CurrentImageIndex = imageIndex;
			return imageIndex;
		}

		if (result == vk::Result::eSuboptimalKHR)
		{
			m_IsSuboptimal      = true;
			m_CurrentImageIndex = imageIndex;
			return imageIndex;
		}

		return std::unexpected(result);
	}

	std::expected<void, vk::Result> VulkanSwapChain::Present(
	    uint32_t imageIndex, std::span<const vk::Semaphore> waitSemaphores
	)
	{
		const Ref<VulkanDevice> device = VulkanContext::GetCurrentDevice();

		const vk::PresentInfoKHR presentInfo(waitSemaphores, m_SwapChain, imageIndex);

		vk::Result result;
		{
			// Lock the present queue during presentation
			device->LockQueue(QueueType::Present);
			result = device->GetQueue(QueueType::Present).presentKHR(presentInfo);
			device->UnlockQueue(QueueType::Present);
		}

		if (result == vk::Result::eSuccess)
			return {};

		if (result == vk::Result::eSuboptimalKHR)
		{
			m_IsSuboptimal = true;
			return {};
		}

		return std::unexpected(result);
	}

	void VulkanSwapChain::Recreate(const vk::Extent2D& newExtent)
	{
		ER_CORE_INFO_TAG("Renderer", "Recreating swap chain with extent {}x{}", newExtent.width, newExtent.height);

		const vk::Device vulkanDevice = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		vulkanDevice.waitIdle();

		m_Specification.DesiredExtent = newExtent;

		const vk::SwapchainKHR oldSwapChain = m_SwapChain;

		for (auto& imageView : m_ImageViews)
			vulkanDevice.destroyImageView(imageView);
		m_ImageViews.clear();

		// Create new swap chain
		Create();

		if (oldSwapChain)
			vulkanDevice.destroySwapchainKHR(oldSwapChain);

		m_IsSuboptimal = false;
	}

	void VulkanSwapChain::Create()
	{
		const Ref<VulkanDevice>         device         = VulkanContext::GetCurrentDevice();
		const Ref<VulkanPhysicalDevice> physicalDevice = device->GetPhysicalDevice();
		const vk::Device                vkDevice       = device->GetVulkanDevice();

		const vk::SurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat();
		const vk::PresentModeKHR   presentMode   = ChoosePresentMode();
		const vk::Extent2D         extent        = ChooseExtent();
		const uint32_t             imageCount    = ChooseImageCount();

		m_ImageFormat = surfaceFormat.format;
		m_Extent      = extent;

		vk::SwapchainCreateInfoKHR swapChainCreateInfo{};
		swapChainCreateInfo.surface          = m_Specification.Surface;
		swapChainCreateInfo.minImageCount    = imageCount;
		swapChainCreateInfo.imageFormat      = surfaceFormat.format;
		swapChainCreateInfo.imageColorSpace  = surfaceFormat.colorSpace;
		swapChainCreateInfo.imageExtent      = extent;
		swapChainCreateInfo.imageArrayLayers = 1;
		swapChainCreateInfo.imageUsage       = m_Specification.ImageUsage;

		// Handle queue families
		const QueueFamilyIndices& indices             = physicalDevice->GetQueueFamilyIndices();
		const std::set<uint32_t>  uniqueQueueFamilies = indices.GetUniqueIndices();

		std::vector<uint32_t> queueFamilyIndices(uniqueQueueFamilies.begin(), uniqueQueueFamilies.end());

		if (queueFamilyIndices.size() > 1)
		{
			swapChainCreateInfo.imageSharingMode      = vk::SharingMode::eConcurrent;
			swapChainCreateInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
			swapChainCreateInfo.pQueueFamilyIndices   = queueFamilyIndices.data();
		}
		else
		{
			swapChainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
		}

		swapChainCreateInfo.preTransform   = m_SupportDetails.Capabilities.currentTransform;
		swapChainCreateInfo.compositeAlpha = m_Specification.CompositeAlpha;
		swapChainCreateInfo.presentMode    = presentMode;
		swapChainCreateInfo.clipped        = m_Specification.EnableClipping ? VK_TRUE : VK_FALSE;
		swapChainCreateInfo.oldSwapchain   = m_SwapChain;        // For recreation

		m_SwapChain = vkDevice.createSwapchainKHR(swapChainCreateInfo);
		m_Images    = vkDevice.getSwapchainImagesKHR(m_SwapChain);

		CreateImageViews();
	}

	void VulkanSwapChain::Destroy()
	{
		const vk::Device vkDevice = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		for (auto& imageView : m_ImageViews)
		{
			if (imageView)
				vkDevice.destroyImageView(imageView);
		}
		m_ImageViews.clear();

		if (m_SwapChain)
		{
			vkDevice.destroySwapchainKHR(m_SwapChain);
			m_SwapChain = VK_NULL_HANDLE;
		}

		m_Images.clear();
	}

	void VulkanSwapChain::CreateImageViews()
	{
		const vk::Device vkDevice = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		m_ImageViews.resize(m_Images.size());
		for (size_t i = 0; i < m_Images.size(); ++i)
		{
			const vk::ImageViewCreateInfo createInfo(
			    {},
			    m_Images[i],
			    vk::ImageViewType::e2D,
			    m_ImageFormat,
			    {},
			    vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
			);

			m_ImageViews[i] = vkDevice.createImageView(createInfo);

#ifdef ER_DEBUG
			const std::string name = std::format("SwapChainImageView_{}", i);
			VulkanUtils::SetDebugUtilsObjectName(vkDevice, vk::ObjectType::eImageView, m_ImageViews[i], name);
#endif
		}
	}

	vk::SurfaceFormatKHR VulkanSwapChain::ChooseSurfaceFormat() const
	{
		// Check if preferred format is available
		const auto it = std::ranges::find_if(m_SupportDetails.Formats, [this](const vk::SurfaceFormatKHR& format) {
			return format.format == m_Specification.PreferredFormat &&
			       format.colorSpace == m_Specification.PreferredColorSpace;
		});

		if (it != m_SupportDetails.Formats.end())
			return *it;

		// Fall back to first available format
		return m_SupportDetails.Formats.front();
	}

	vk::PresentModeKHR VulkanSwapChain::ChoosePresentMode() const
	{
		// Check if preferred mode is available
		const auto it = std::ranges::find(m_SupportDetails.PresentModes, m_Specification.PreferredPresentMode);

		if (it != m_SupportDetails.PresentModes.end())
			return *it;

		// Fallback to FIFO (always available)
		return vk::PresentModeKHR::eFifo;
	}

	vk::Extent2D VulkanSwapChain::ChooseExtent() const
	{
		// If extent is defined by surface capabilities, use it
		if (m_SupportDetails.Capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			return m_SupportDetails.Capabilities.currentExtent;

		// Otherwise, clamp desired extent to capabilities
		return {
		    std::clamp(
		        m_Specification.DesiredExtent.width,
		        m_SupportDetails.Capabilities.minImageExtent.width,
		        m_SupportDetails.Capabilities.maxImageExtent.width
		    ),
		    std::clamp(
		        m_Specification.DesiredExtent.height,
		        m_SupportDetails.Capabilities.minImageExtent.height,
		        m_SupportDetails.Capabilities.maxImageExtent.height
		    )
		};
	}

	uint32_t VulkanSwapChain::ChooseImageCount() const
	{
		uint32_t imageCount = std::max(
		    Renderer::GetConfig().FramesInFlight, m_SupportDetails.Capabilities.minImageCount
		);

		// Ensure we don't exceed maximum (0 means no maximum)
		if (m_SupportDetails.Capabilities.maxImageCount > 0)
			imageCount = std::min(imageCount, m_SupportDetails.Capabilities.maxImageCount);

		return imageCount;
	}

	SwapChainSupportDetails VulkanSwapChain::QuerySupport(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
	{
		SwapChainSupportDetails details;

		details.Capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
		details.Formats      = physicalDevice.getSurfaceFormatsKHR(surface);
		details.PresentModes = physicalDevice.getSurfacePresentModesKHR(surface);

		return details;
	}

}        // namespace Eruption
