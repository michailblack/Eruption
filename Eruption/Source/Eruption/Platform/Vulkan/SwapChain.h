#pragma once
#include "Eruption/Platform/Vulkan/Vulkan.h"

#include <expected>
#include <span>

namespace Eruption::Vulkan
{
	struct SwapChainSpecification
	{
		vk::Extent2D                  DesiredExtent;
		uint32_t                      DesiredImageCount    = 3;
		vk::Format                    PreferredFormat      = vk::Format::eB8G8R8A8Srgb;
		vk::ColorSpaceKHR             PreferredColorSpace  = vk::ColorSpaceKHR::eSrgbNonlinear;
		vk::PresentModeKHR            PreferredPresentMode = vk::PresentModeKHR::eMailbox;
		vk::ImageUsageFlags           ImageUsage           = vk::ImageUsageFlagBits::eColorAttachment;
		vk::CompositeAlphaFlagBitsKHR CompositeAlpha       = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		bool                          EnableClipping       = true;
		bool                          VSync                = false;        // forces FIFO when true
	};

	struct SwapChainSupportDetails
	{
		vk::SurfaceCapabilitiesKHR        Capabilities;
		std::vector<vk::SurfaceFormatKHR> Formats;
		std::vector<vk::PresentModeKHR>   PresentModes;
	};

	class SwapChain
	{
	public:
		explicit SwapChain(const SwapChainSpecification& specification);
		~SwapChain() = default;

		SwapChain(const SwapChain&)            = delete;
		SwapChain& operator=(const SwapChain&) = delete;

		[[nodiscard]] std::expected<uint32_t, vk::Result> AcquireNextImage(
		    vk::Semaphore signalSemaphore,
		    vk::Fence     signalFence = nullptr,
		    uint64_t      timeout     = std::numeric_limits<uint64_t>::max()
		);

		[[nodiscard]] std::expected<void, vk::Result> Present(
		    uint32_t imageIndex, std::span<const vk::Semaphore> waitSemaphores
		);

		void Recreate(const vk::Extent2D& newExtent);

		[[nodiscard]] const vk::raii::SwapchainKHR& GetVkHandle() const { return m_SwapChain; }

		[[nodiscard]] vk::Format   GetImageFormat() const { return m_ImageFormat; }
		[[nodiscard]] vk::Extent2D GetExtent() const { return m_Extent; }

		[[nodiscard]] uint32_t GetImageCount() const { return static_cast<uint32_t>(m_Images.size()); }
		[[nodiscard]] const std::vector<vk::Image>& GetImages() const { return m_Images; }
		[[nodiscard]] vk::Image                     GetImage(uint32_t index) const { return m_Images[index]; }
		[[nodiscard]] const vk::raii::ImageView&    GetImageView(uint32_t index) const { return m_ImageViews[index]; }

	private:
		void Create();
		void CreateImageViews();

		[[nodiscard]] vk::SurfaceFormatKHR ChooseSurfaceFormat() const;
		[[nodiscard]] vk::PresentModeKHR   ChoosePresentMode() const;
		[[nodiscard]] vk::Extent2D         ChooseExtent() const;
		[[nodiscard]] uint32_t             ChooseImageCount() const;

		void QuerySupport();

	private:
		SwapChainSpecification  m_Specification;
		SwapChainSupportDetails m_SupportDetails;

		vk::raii::SwapchainKHR m_SwapChain = nullptr;

		std::vector<vk::Image>           m_Images;
		std::vector<vk::raii::ImageView> m_ImageViews;

		vk::Format   m_ImageFormat = vk::Format::eUndefined;
		vk::Extent2D m_Extent;
	};
}        // namespace Eruption::Vulkan
