#include "SwapChain.h"

#include "Eruption/Platform/Vulkan/VulkanContext.h"

namespace Eruption::Vulkan
{
	SwapChain::SwapChain(const SwapChainSpecification& specification) : m_Specification(specification)
	{
		QuerySupport();
		Create();
		CreateImageViews();
	}

	void SwapChain::QuerySupport()
	{
		const vk::raii::PhysicalDevice& physicalDevice = VulkanContext::Get().GetPhysicalDevice()->GetVkHandle();
		const vk::raii::SurfaceKHR&     surface        = VulkanContext::Get().GetInstance()->GetSurface();

		m_SupportDetails = {
		    .Capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface),
		    .Formats      = physicalDevice.getSurfaceFormatsKHR(surface),
		    .PresentModes = physicalDevice.getSurfacePresentModesKHR(surface)
		};
	}

	void SwapChain::Create()
	{
		const auto [format, colorSpace] = ChooseSurfaceFormat();

		m_ImageFormat = format;
		m_Extent      = ChooseExtent();

		const vk::SwapchainCreateInfoKHR createInfo{
		    .surface          = VulkanContext::Get().GetInstance()->GetSurface(),
		    .minImageCount    = ChooseImageCount(),
		    .imageFormat      = format,
		    .imageColorSpace  = colorSpace,
		    .imageExtent      = m_Extent,
		    .imageArrayLayers = 1,
		    .imageUsage       = m_Specification.ImageUsage,
		    .imageSharingMode = vk::SharingMode::eExclusive,
		    .preTransform     = m_SupportDetails.Capabilities.currentTransform,
		    .compositeAlpha   = m_Specification.CompositeAlpha,
		    .presentMode      = ChoosePresentMode(),
		    .clipped          = m_Specification.EnableClipping ? vk::True : vk::False,
		    .oldSwapchain     = *m_SwapChain
		};

		m_SwapChain = vk::raii::SwapchainKHR(VulkanContext::GetVkDevice(), createInfo);
		m_Images    = m_SwapChain.getImages();
	}

	void SwapChain::CreateImageViews()
	{
		m_ImageViews.clear();
		m_ImageViews.reserve(m_Images.size());

		vk::ImageViewCreateInfo createInfo{
		    .viewType         = vk::ImageViewType::e2D,
		    .format           = m_ImageFormat,
		    .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
		};

		for (uint32_t i = 0; i < m_Images.size(); ++i)
		{
			createInfo.image = m_Images[i];
			m_ImageViews.emplace_back(VulkanContext::GetVkDevice(), createInfo);

#ifdef ER_DEBUG
			Utils::SetDebugUtilsObjectName(
			    VulkanContext::GetVkDevice(),
			    vk::ObjectType::eImageView,
			    *m_ImageViews[i],
			    std::format("SwapChainImageView[{}]", i)
			);
#endif
		}
	}

	std::expected<uint32_t, vk::Result> SwapChain::AcquireNextImage(
	    vk::Semaphore signalSemaphore, vk::Fence signalFence, uint64_t timeout
	)
	{
		try
		{
			const auto [result, imageIndex] = m_SwapChain.acquireNextImage(timeout, signalSemaphore, signalFence);
			if (result == vk::Result::eSuccess || result == vk::Result::eSuboptimalKHR)
				return imageIndex;
			return std::unexpected(result);
		}
		catch (const vk::OutOfDateKHRError&)
		{
			return std::unexpected(vk::Result::eErrorOutOfDateKHR);
		}
	}

	std::expected<void, vk::Result> SwapChain::Present(
	    uint32_t imageIndex, std::span<const vk::Semaphore> waitSemaphores
	)
	{
		const vk::SwapchainKHR swapchain = *m_SwapChain;

		const vk::PresentInfoKHR presentInfo{
		    .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
		    .pWaitSemaphores    = waitSemaphores.data(),
		    .swapchainCount     = 1,
		    .pSwapchains        = &swapchain,
		    .pImageIndices      = &imageIndex
		};

		const vk::Result result = VulkanContext::Get().GetDevice()->Present(presentInfo);
		if (result == vk::Result::eSuccess || result == vk::Result::eSuboptimalKHR)
			return {};
		return std::unexpected(result);
	}

	void SwapChain::Recreate(const vk::Extent2D& newExtent)
	{
		// TODO: Tricky place for synchronization
		VulkanContext::GetVkDevice().waitIdle();

		m_Specification.DesiredExtent = newExtent;
		// TODO: Do we really need to query support again
		QuerySupport();

		m_ImageViews.clear();
		Create();
		CreateImageViews();

		ER_CORE_TRACE_TAG("Renderer", "Swapchain recreated at {}x{}", m_Extent.width, m_Extent.height);
	}

	vk::SurfaceFormatKHR SwapChain::ChooseSurfaceFormat() const
	{
		const auto it = std::ranges::find_if(m_SupportDetails.Formats, [this](const vk::SurfaceFormatKHR& f) {
			return f.format == m_Specification.PreferredFormat && f.colorSpace == m_Specification.PreferredColorSpace;
		});
		return it != m_SupportDetails.Formats.end() ? *it : m_SupportDetails.Formats.front();
	}

	vk::PresentModeKHR SwapChain::ChoosePresentMode() const
	{
		// FIFO is always supported and is the correct choice when v-sync is requested.
		if (m_Specification.VSync)
			return vk::PresentModeKHR::eFifo;

		const auto it = std::ranges::find(m_SupportDetails.PresentModes, m_Specification.PreferredPresentMode);
		return it != m_SupportDetails.PresentModes.end() ? *it : vk::PresentModeKHR::eFifo;
	}

	vk::Extent2D SwapChain::ChooseExtent() const
	{
		const vk::SurfaceCapabilitiesKHR& caps = m_SupportDetails.Capabilities;

		if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max())
			return caps.currentExtent;

		return {
		    std::clamp(m_Specification.DesiredExtent.width, caps.minImageExtent.width, caps.maxImageExtent.width),
		    std::clamp(m_Specification.DesiredExtent.height, caps.minImageExtent.height, caps.maxImageExtent.height)
		};
	}

	uint32_t SwapChain::ChooseImageCount() const
	{
		const vk::SurfaceCapabilitiesKHR& caps = m_SupportDetails.Capabilities;

		uint32_t imageCount = std::max(m_Specification.DesiredImageCount, caps.minImageCount);
		if (caps.maxImageCount > 0)
			imageCount = std::min(imageCount, caps.maxImageCount);

		return imageCount;
	}
}        // namespace Eruption::Vulkan
