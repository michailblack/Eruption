#include "SwapChain.h"

#include "Eruption/Platform/Vulkan/Device.h"
#include "Eruption/Platform/Vulkan/Instance.h"
#include "Eruption/Platform/Vulkan/PhysicalDevice.h"
#include "Eruption/Renderer/Renderer.h"

namespace Eruption::Vulkan
{
	SwapChain::SwapChain(
	    const Ref<Instance>&          instance,
	    const Ref<PhysicalDevice>&    physicalDevice,
	    const Ref<Device>&            device,
	    const SwapChainSpecification& specification
	) :
	    m_Specification(specification)
	{
		m_SupportDetails = QuerySupport(physicalDevice->GetVkHandle(), instance->GetSurfaceHandle());

		Create(instance, device);
	}

	SwapChain::~SwapChain()
	{
		Destroy();
	}

	std::expected<uint32_t, vk::Result> SwapChain::AcquireNextImage(
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

	std::expected<void, vk::Result> SwapChain::Present(
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

	void SwapChain::Recreate(const vk::Extent2D& newExtent)
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

	void SwapChain::Create(const Ref<Instance>& instance, const Ref<Device>& device)
	{
		const auto [format, colorSpace]        = ChooseSurfaceFormat();
		const vk::PresentModeKHR presentMode   = ChoosePresentMode();
		const vk::Extent2D       extent        = ChooseExtent();
		const uint32_t           minImageCount = ChooseMinImageCount();

		m_ImageFormat = format;
		m_Extent      = extent;

		vk::SwapchainCreateInfoKHR swapChainCreateInfo{
		    .surface          = instance->GetSurfaceHandle(),
		    .minImageCount    = minImageCount,
		    .imageFormat      = format,
		    .imageColorSpace  = colorSpace,
		    .imageExtent      = extent,
		    .imageArrayLayers = 1,
		    .imageUsage       = m_Specification.ImageUsage,
		    .imageSharingMode = vk::SharingMode::eExclusive,
		    .preTransform     = m_SupportDetails.Capabilities.currentTransform,
		    .compositeAlpha   = m_Specification.CompositeAlpha,
		    .presentMode      = presentMode,
		    .clipped          = m_Specification.EnableClipping ? vk::True : vk::False,
		    .oldSwapchain     = m_SwapChain
		};

		m_SwapChain = vk::raii::SwapchainKHR(device->GetVkHandle(), swapChainCreateInfo);
		m_Images    = m_SwapChain.getImages();

		CreateImageViews(device);
	}

	void SwapChain::Destroy()
	{
		const vk::Device vkDevice = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		m_SwapChain.getDevice();

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

	void SwapChain::CreateImageViews(const Ref<Device>& device)
	{
		m_ImageViews.clear();
		m_ImageViews.reserve(m_Images.size());

		vk::ImageViewCreateInfo imageViewCreateInfo{
		    .viewType = vk::ImageViewType::e2D,
		    .format   = m_ImageFormat,
		    .components =
		        {vk::ComponentSwizzle::eIdentity,
		                     vk::ComponentSwizzle::eIdentity,
		                     vk::ComponentSwizzle::eIdentity,
		                     vk::ComponentSwizzle::eIdentity},
		    .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
		};

		for (uint32_t i = 0; i < m_Images.size(); i++)
		{
			imageViewCreateInfo.image = m_Images[i];
			m_ImageViews.emplace_back(device->GetVkHandle(), imageViewCreateInfo);

#ifdef ER_DEBUG
			const std::string name = std::format("SwapChainImageView_{}", i);
			Utils::SetDebugUtilsObjectName(device->GetVkHandle(), vk::ObjectType::eImageView, *m_ImageViews[i], name);
#endif
		}
	}

	vk::SurfaceFormatKHR SwapChain::ChooseSurfaceFormat() const
	{
		const auto it = std::ranges::find_if(m_SupportDetails.Formats, [this](const vk::SurfaceFormatKHR& format) {
			return format.format == m_Specification.PreferredFormat &&
			       format.colorSpace == m_Specification.PreferredColorSpace;
		});

		if (it != m_SupportDetails.Formats.end())
			return *it;

		return m_SupportDetails.Formats.front();
	}

	vk::PresentModeKHR SwapChain::ChoosePresentMode() const
	{
		const auto it = std::ranges::find(m_SupportDetails.PresentModes, m_Specification.PreferredPresentMode);

		if (it != m_SupportDetails.PresentModes.end())
			return *it;

		ER_CORE_VERIFY(std::ranges::any_of(m_SupportDetails.PresentModes, [](const vk::PresentModeKHR& presentMode) {
			return presentMode == vk::PresentModeKHR::eFifo;
		}));

		return vk::PresentModeKHR::eFifo;
	}

	vk::Extent2D SwapChain::ChooseExtent() const
	{
		// If extent is defined by surface capabilities, use it
		if (m_SupportDetails.Capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			return m_SupportDetails.Capabilities.currentExtent;

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

	uint32_t SwapChain::ChooseMinImageCount() const
	{
		uint32_t imageCount = std::max(
		    Renderer::GetConfig().FramesInFlight, m_SupportDetails.Capabilities.minImageCount
		);

		// Ensure we don't exceed maximum (0 means no maximum)
		if (m_SupportDetails.Capabilities.maxImageCount > 0)
			imageCount = std::min(imageCount, m_SupportDetails.Capabilities.maxImageCount);

		return imageCount;
	}

	SwapChainSupportDetails SwapChain::QuerySupport(
	    const vk::raii::PhysicalDevice& physicalDevice, const vk::raii::SurfaceKHR& surface
	)
	{
		return {
		    .Capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface),
		    .Formats      = physicalDevice.getSurfaceFormatsKHR(surface),
		    .PresentModes = physicalDevice.getSurfacePresentModesKHR(surface)
		};
	}
}        // namespace Eruption::Vulkan
