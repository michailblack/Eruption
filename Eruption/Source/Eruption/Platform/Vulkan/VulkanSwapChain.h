#pragma once
#include "Eruption/Platform/Vulkan/VulkanDevice.h"

#include <expected>
#include <span>

namespace Eruption
{
	struct SwapChainSpecification
	{
		vk::SurfaceKHR                Surface;
		vk::Extent2D                  DesiredExtent;
		vk::Format                    PreferredFormat      = vk::Format::eB8G8R8A8Srgb;
		vk::ColorSpaceKHR             PreferredColorSpace  = vk::ColorSpaceKHR::eSrgbNonlinear;
		vk::PresentModeKHR            PreferredPresentMode = vk::PresentModeKHR::eMailbox;
		vk::ImageUsageFlags           ImageUsage           = vk::ImageUsageFlagBits::eColorAttachment;
		vk::CompositeAlphaFlagBitsKHR CompositeAlpha       = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		bool                          EnableClipping       = true;
	};

	struct SwapChainSupportDetails
	{
		vk::SurfaceCapabilitiesKHR        Capabilities;
		std::vector<vk::SurfaceFormatKHR> Formats;
		std::vector<vk::PresentModeKHR>   PresentModes;
	};

	class VulkanSwapChain
	{
	public:
		explicit VulkanSwapChain(const SwapChainSpecification& spec);
		~VulkanSwapChain();

		VulkanSwapChain(const VulkanSwapChain&)            = delete;
		VulkanSwapChain& operator=(const VulkanSwapChain&) = delete;
		VulkanSwapChain(VulkanSwapChain&&)                 = delete;
		VulkanSwapChain& operator=(VulkanSwapChain&&)      = delete;

		[[nodiscard]] std::expected<uint32_t, vk::Result> AcquireNextImage(
		    vk::Semaphore signalSemaphore,
		    vk::Fence     signalFence = VK_NULL_HANDLE,
		    uint64_t      timeout     = std::numeric_limits<uint64_t>::max()
		);

		[[nodiscard]] std::expected<void, vk::Result> Present(
		    uint32_t imageIndex, std::span<const vk::Semaphore> waitSemaphores
		);

		void Recreate(const vk::Extent2D& newExtent);

		[[nodiscard]] vk::SwapchainKHR GetVulkanSwapChain() const { return m_SwapChain; }
		[[nodiscard]] vk::Format       GetImageFormat() const { return m_ImageFormat; }
		[[nodiscard]] vk::Extent2D     GetExtent() const { return m_Extent; }
		[[nodiscard]] uint32_t         GetImageCount() const { return static_cast<uint32_t>(m_Images.size()); }
		[[nodiscard]] const std::vector<vk::Image>&     GetImages() const { return m_Images; }
		[[nodiscard]] const std::vector<vk::ImageView>& GetImageViews() const { return m_ImageViews; }
		[[nodiscard]] vk::ImageView GetImageView(uint32_t index) const { return m_ImageViews[index]; }

		[[nodiscard]] bool IsSuboptimal() const { return m_IsSuboptimal; }

	private:
		void Create();
		void Destroy();
		void CreateImageViews();

		[[nodiscard]] vk::SurfaceFormatKHR ChooseSurfaceFormat() const;
		[[nodiscard]] vk::PresentModeKHR   ChoosePresentMode() const;
		[[nodiscard]] vk::Extent2D         ChooseExtent() const;
		[[nodiscard]] uint32_t             ChooseImageCount() const;

		[[nodiscard]] static SwapChainSupportDetails QuerySupport(
		    vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface
		);

	private:
		SwapChainSupportDetails m_SupportDetails;
		SwapChainSpecification  m_Specification;

		std::vector<vk::Image>     m_Images;
		std::vector<vk::ImageView> m_ImageViews;

		vk::SwapchainKHR m_SwapChain;
		vk::Format       m_ImageFormat;
		vk::Extent2D     m_Extent;

		bool m_IsSuboptimal = false;

		uint32_t m_CurrentImageIndex = 0;
	};
}        // namespace Eruption
