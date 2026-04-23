#pragma once
#include "Eruption/Platform/Vulkan/Vulkan.h"

#include <expected>
#include <span>

namespace Eruption::Vulkan
{
	class Instance;
	class PhysicalDevice;
	class Device;

	struct SwapChainSpecification
	{
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

	class SwapChain
	{
	public:
		explicit SwapChain(
		    const Ref<Instance>&          instance,
		    const Ref<PhysicalDevice>&    physicalDevice,
		    const Ref<Device>&            device,
		    const SwapChainSpecification& specification
		);
		~SwapChain();

		SwapChain(const SwapChain&)            = delete;
		SwapChain& operator=(const SwapChain&) = delete;

		SwapChain(SwapChain&&) noexcept            = default;
		SwapChain& operator=(SwapChain&&) noexcept = default;

		[[nodiscard]] std::expected<uint32_t, vk::Result> AcquireNextImage(
		    vk::Semaphore signalSemaphore,
		    vk::Fence     signalFence = VK_NULL_HANDLE,
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

		[[nodiscard]] const std::vector<vk::raii::ImageView>& GetImageViews() const { return m_ImageViews; }
		[[nodiscard]] const vk::raii::ImageView& GetImageView(uint32_t index) const { return m_ImageViews[index]; }

		[[nodiscard]] bool IsSuboptimal() const { return m_IsSuboptimal; }

	private:
		void Create(const Ref<Instance>& instance, const Ref<Device>& device);
		void Destroy();
		void CreateImageViews(const Ref<Device>& device);

		[[nodiscard]] vk::SurfaceFormatKHR ChooseSurfaceFormat() const;
		[[nodiscard]] vk::PresentModeKHR   ChoosePresentMode() const;
		[[nodiscard]] vk::Extent2D         ChooseExtent() const;
		[[nodiscard]] uint32_t             ChooseMinImageCount() const;

		[[nodiscard]] static SwapChainSupportDetails QuerySupport(
		    const vk::raii::PhysicalDevice& physicalDevice, const vk::raii::SurfaceKHR& surface
		);

	private:
		SwapChainSupportDetails m_SupportDetails;
		SwapChainSpecification  m_Specification;

		vk::raii::SwapchainKHR m_SwapChain = nullptr;

		std::vector<vk::Image>           m_Images;
		std::vector<vk::raii::ImageView> m_ImageViews;

		vk::Format   m_ImageFormat;
		vk::Extent2D m_Extent;

		uint32_t m_CurrentImageIndex = 0;

		bool m_IsSuboptimal = false;
	};
}        // namespace Eruption::Vulkan
