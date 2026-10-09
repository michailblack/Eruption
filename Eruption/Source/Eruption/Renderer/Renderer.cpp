#include "Renderer.h"

#include "Eruption/Core/Window.h"
#include "Eruption/Platform/Vulkan/VulkanContext.h"

#include <atomic>

namespace Eruption
{
	using namespace Vulkan;

	namespace
	{
		struct FrameData
		{
			vk::raii::CommandPool   CommandPool    = nullptr;
			vk::raii::CommandBuffer CommandBuffer  = nullptr;
			vk::raii::Semaphore     ImageAvailable = nullptr;
			vk::raii::Fence         InFlight       = nullptr;

			AllocatedImage      Depth;
			vk::raii::ImageView DepthView = nullptr;
		};

		struct RendererData
		{
			Renderer::Config Config;

			Scope<SwapChain> Swapchain;

			std::vector<FrameData>           Frames;                // size == FramesInFlight
			std::vector<vk::raii::Semaphore> RenderFinished;        // size == swapchain image count

			std::vector<RenderCommand> SubmissionQueue;
			std::vector<RenderCommand> ExecutionQueue;

			Scope<RenderThread> Thread;

			uint32_t              CurrentFrame  = 0;
			std::atomic<uint64_t> AbsoluteFrame = 0;

			std::atomic<bool>     NeedsResize   = false;
			std::atomic<uint32_t> PendingWidth  = 0;
			std::atomic<uint32_t> PendingHeight = 0;

			std::mutex DeletionMutex;

			std::vector<std::pair<uint64_t, std::move_only_function<void()>>> Deletions;

			vk::Format DepthFormat = vk::Format::eUndefined;
			glm::vec4  ClearColor  = {0.10f, 0.10f, 0.12f, 1.0f};
		};

		Scope<RendererData> s_Data;

		void CreateDepthResources()
		{
			const vk::Extent2D      extent    = s_Data->Swapchain->GetExtent();
			const VulkanAllocator&  allocator = VulkanContext::Get().GetAllocator();
			const vk::raii::Device& device    = VulkanContext::GetVkDevice();

			for (uint32_t i = 0; i < s_Data->Frames.size(); ++i)
			{
				FrameData& frame = s_Data->Frames[i];

				const vk::ImageCreateInfo imageInfo{
				    .imageType   = vk::ImageType::e2D,
				    .format      = s_Data->DepthFormat,
				    .extent      = {extent.width, extent.height, 1},
				    .mipLevels   = 1,
				    .arrayLayers = 1,
				    .samples     = vk::SampleCountFlagBits::e1,
				    .tiling      = vk::ImageTiling::eOptimal,
				    .usage       = vk::ImageUsageFlagBits::eDepthStencilAttachment
				};
				frame.Depth = allocator.CreateImage(imageInfo, MemoryUsage::GpuOnly, std::format("DepthBuffer[{}]", i));

				frame.DepthView = vk::raii::ImageView(
				    device,
				    vk::ImageViewCreateInfo{
				        .image            = frame.Depth.Image,
				        .viewType         = vk::ImageViewType::e2D,
				        .format           = s_Data->DepthFormat,
				        .subresourceRange = {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1}
                }
				);
			}
		}

		void DestroyDepthResources()
		{
			for (FrameData& frame : s_Data->Frames)
			{
				frame.DepthView = nullptr;
				VulkanContext::Get().GetAllocator().DestroyImage(frame.Depth);
				frame.Depth = {};
			}
		}

		void CreateSyncAndCommandResources()
		{
			const vk::raii::Device& device = VulkanContext::GetVkDevice();
			const uint32_t gfxFamily       = VulkanContext::Get().GetDevice()->GetQueueFamilyIndex(QueueType::Graphics);

			s_Data->Frames.resize(s_Data->Config.FramesInFlight);
			for (FrameData& frame : s_Data->Frames)
			{
				frame.CommandPool = vk::raii::CommandPool(
				    device,
				    vk::CommandPoolCreateInfo{
				        .flags = vk::CommandPoolCreateFlagBits::eTransient, .queueFamilyIndex = gfxFamily
				    }
				);

				vk::raii::CommandBuffers buffers(
				    device,
				    vk::CommandBufferAllocateInfo{
				        .commandPool        = *frame.CommandPool,
				        .level              = vk::CommandBufferLevel::ePrimary,
				        .commandBufferCount = 1
				    }
				);
				frame.CommandBuffer = std::move(buffers.front());

				frame.ImageAvailable = vk::raii::Semaphore(device, vk::SemaphoreCreateInfo{});
				frame.InFlight       = vk::raii::Fence(
				    device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled}
				);
			}

			s_Data->RenderFinished.clear();
			for (uint32_t i = 0; i < s_Data->Swapchain->GetImageCount(); ++i)
				s_Data->RenderFinished.emplace_back(device, vk::SemaphoreCreateInfo{});
		}

		void RecreateRenderFinishedSemaphores()
		{
			const vk::raii::Device& device = VulkanContext::GetVkDevice();
			s_Data->RenderFinished.clear();
			for (uint32_t i = 0; i < s_Data->Swapchain->GetImageCount(); ++i)
				s_Data->RenderFinished.emplace_back(device, vk::SemaphoreCreateInfo{});
		}

		void RecreateSwapChain()
		{
			uint32_t width  = s_Data->PendingWidth.load();
			uint32_t height = s_Data->PendingHeight.load();
			if (width == 0 || height == 0)
				return;

			VulkanContext::GetVkDevice().waitIdle();

			DestroyDepthResources();
			s_Data->Swapchain->Recreate(vk::Extent2D{width, height});
			RecreateRenderFinishedSemaphores();
			CreateDepthResources();

			s_Data->NeedsResize = false;
		}

		void FlushDeletions(uint64_t completedFrame)
		{
			std::scoped_lock lock(s_Data->DeletionMutex);
			std::erase_if(s_Data->Deletions, [completedFrame](auto& entry) {
				if (entry.first <= completedFrame)
				{
					entry.second();
					return true;
				}
				return false;
			});
		}

		// The whole GPU frame: runs on the render thread (or inline in single-threaded mode).
		void ExecuteFrame()
		{
			const Ref<Vulkan::Device>& device   = VulkanContext::Get().GetDevice();
			const vk::raii::Device&    vkDevice = device->GetVkHandle();
			const uint64_t             frameNo  = s_Data->AbsoluteFrame.load();
			FrameData&                 frame    = s_Data->Frames[s_Data->CurrentFrame];

			// Wait for this slot's previous submission to finish before reusing its resources.
			VK_CHECK_RESULT(vkDevice.waitForFences(*frame.InFlight, vk::True, std::numeric_limits<uint64_t>::max()));

			// Everything submitted at least FramesInFlight frames ago is now GPU-complete.
			if (frameNo >= s_Data->Config.FramesInFlight)
				FlushDeletions(frameNo - s_Data->Config.FramesInFlight);

			if (s_Data->NeedsResize.load())
				RecreateSwapChain();

			const std::expected<uint32_t, vk::Result> acquired = s_Data->Swapchain->AcquireNextImage(
			    *frame.ImageAvailable
			);
			if (!acquired)
			{
				if (acquired.error() == vk::Result::eErrorOutOfDateKHR ||
				    acquired.error() == vk::Result::eSuboptimalKHR)
					s_Data->NeedsResize = true;
				s_Data->ExecutionQueue.clear();        // drop this frame's recorded work
				return;                                // fence still signalled; safe to retry next frame
			}
			const uint32_t imageIndex = *acquired;

			vkDevice.resetFences(*frame.InFlight);
			frame.CommandPool.reset();

			const vk::raii::CommandBuffer& cmd    = frame.CommandBuffer;
			const vk::Extent2D             extent = s_Data->Swapchain->GetExtent();

			cmd.begin(vk::CommandBufferBeginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

			Utils::CmdTransitionImage(
			    cmd,
			    s_Data->Swapchain->GetImage(imageIndex),
			    vk::ImageLayout::eUndefined,
			    vk::ImageLayout::eColorAttachmentOptimal
			);
			Utils::CmdTransitionImage(
			    cmd,
			    frame.Depth.Image,
			    vk::ImageLayout::eUndefined,
			    vk::ImageLayout::eDepthAttachmentOptimal,
			    vk::ImageAspectFlagBits::eDepth
			);

			vk::ClearValue colorClear{};
			colorClear.color.float32[0] = s_Data->ClearColor.r;
			colorClear.color.float32[1] = s_Data->ClearColor.g;
			colorClear.color.float32[2] = s_Data->ClearColor.b;
			colorClear.color.float32[3] = s_Data->ClearColor.a;

			vk::ClearValue depthClear{};
			depthClear.depthStencil = vk::ClearDepthStencilValue{1.0f, 0};

			const vk::RenderingAttachmentInfo colorAttachment{
			    .imageView   = *s_Data->Swapchain->GetImageView(imageIndex),
			    .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
			    .loadOp      = vk::AttachmentLoadOp::eClear,
			    .storeOp     = vk::AttachmentStoreOp::eStore,
			    .clearValue  = colorClear
			};
			const vk::RenderingAttachmentInfo depthAttachment{
			    .imageView   = *frame.DepthView,
			    .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
			    .loadOp      = vk::AttachmentLoadOp::eClear,
			    .storeOp     = vk::AttachmentStoreOp::eDontCare,
			    .clearValue  = depthClear
			};
			const vk::RenderingInfo renderingInfo{
			    .renderArea           = {{0, 0}, extent},
			    .layerCount           = 1,
			    .colorAttachmentCount = 1,
			    .pColorAttachments    = &colorAttachment,
			    .pDepthAttachment     = &depthAttachment
			};

			cmd.beginRendering(renderingInfo);

			// Flip Y so +Y is up (viewport with negative height), matching GLM/clip conventions.
			cmd.setViewport(
			    0,
			    vk::Viewport{
			        0.0f,
			        static_cast<float>(extent.height),
			        static_cast<float>(extent.width),
			        -static_cast<float>(extent.height),
			        0.0f,
			        1.0f
			    }
			);
			cmd.setScissor(
			    0,
			    vk::Rect2D{
			        {0, 0},
                    extent
            }
			);

			for (RenderCommand& command : s_Data->ExecutionQueue)
				command(cmd);
			s_Data->ExecutionQueue.clear();

			cmd.endRendering();

			Utils::CmdTransitionImage(
			    cmd,
			    s_Data->Swapchain->GetImage(imageIndex),
			    vk::ImageLayout::eColorAttachmentOptimal,
			    vk::ImageLayout::ePresentSrcKHR
			);

			cmd.end();

			const vk::SemaphoreSubmitInfo waitInfo{
			    .semaphore = *frame.ImageAvailable, .stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput
			};
			const vk::SemaphoreSubmitInfo signalInfo{
			    .semaphore = *s_Data->RenderFinished[imageIndex], .stageMask = vk::PipelineStageFlagBits2::eAllGraphics
			};
			const vk::CommandBufferSubmitInfo cmdInfo{.commandBuffer = *cmd};
			const vk::SubmitInfo2             submit{
			    .waitSemaphoreInfoCount   = 1,
			    .pWaitSemaphoreInfos      = &waitInfo,
			    .commandBufferInfoCount   = 1,
			    .pCommandBufferInfos      = &cmdInfo,
			    .signalSemaphoreInfoCount = 1,
			    .pSignalSemaphoreInfos    = &signalInfo
			};
			device->Submit(QueueType::Graphics, submit, *frame.InFlight);

			const std::array                      presentWait = {*s_Data->RenderFinished[imageIndex]};
			const std::expected<void, vk::Result> presented   = s_Data->Swapchain->Present(imageIndex, presentWait);
			if (!presented)
				s_Data->NeedsResize = true;

			s_Data->CurrentFrame = (s_Data->CurrentFrame + 1) % s_Data->Config.FramesInFlight;
			s_Data->AbsoluteFrame.fetch_add(1);
		}
	}        // namespace

	void Renderer::Init(const Scope<Window>& window)
	{
		Init(window, Config{});
	}

	void Renderer::Init(const Scope<Window>& window, const Config& config)
	{
		ER_CORE_ASSERT(!s_Data, "Renderer already initialized!");
		s_Data             = CreateScope<RendererData>();
		s_Data->Config     = config;
		s_Data->ClearColor = config.ClearColor;

		VulkanContext::Init(window);

		const auto [fbWidth, fbHeight] = window->GetFramebufferSize();
		s_Data->PendingWidth           = fbWidth;
		s_Data->PendingHeight          = fbHeight;

		s_Data->DepthFormat = VulkanContext::Get().GetPhysicalDevice()->GetDepthFormat();

		const SwapChainSpecification swapChainSpec{
		    .DesiredExtent = {fbWidth, fbHeight},
              .DesiredImageCount = config.FramesInFlight + 1, .VSync = config.VSync
		};
		s_Data->Swapchain = CreateScope<SwapChain>(swapChainSpec);

		CreateSyncAndCommandResources();
		CreateDepthResources();

		s_Data->Thread = CreateScope<RenderThread>(config.Threading);
		s_Data->Thread->Start([] { ExecuteFrame(); });

		ER_CORE_INFO_TAG(
		    "Renderer",
		    "Renderer initialized ({} frames in flight, {})",
		    config.FramesInFlight,
		    config.Threading == ThreadingPolicy::MultiThreaded ? "multi-threaded" : "single-threaded"
		);
	}

	void Renderer::Shutdown()
	{
		if (!s_Data)
			return;

		s_Data->Thread->Stop();
		VulkanContext::GetVkDevice().waitIdle();

		// Run any remaining deferred deletions now that the GPU is idle.
		{
			std::scoped_lock lock(s_Data->DeletionMutex);
			for (auto& [frameNo, fn] : s_Data->Deletions)
				fn();
			s_Data->Deletions.clear();
		}

		DestroyDepthResources();

		s_Data.reset();
		VulkanContext::Shutdown();
	}

	void Renderer::WaitIdle()
	{
		if (!s_Data)
			return;
		s_Data->Thread->Stop();
		VulkanContext::GetVkDevice().waitIdle();
	}

	void Renderer::BeginFrame()
	{
		// All GPU work happens on the render thread during EndFrame; nothing to do here yet.
	}

	void Renderer::EndFrame()
	{
		s_Data->Thread->WaitIdle();
		std::swap(s_Data->SubmissionQueue, s_Data->ExecutionQueue);
		s_Data->Thread->Kick();
	}

	void Renderer::Submit(RenderCommand&& command)
	{
		s_Data->SubmissionQueue.emplace_back(std::move(command));
	}

	void Renderer::SubmitResourceFree(std::move_only_function<void()>&& fn)
	{
		std::scoped_lock lock(s_Data->DeletionMutex);
		s_Data->Deletions.emplace_back(s_Data->AbsoluteFrame.load(), std::move(fn));
	}

	void Renderer::ImmediateSubmit(const std::function<void(const vk::raii::CommandBuffer&)>& fn)
	{
		VulkanContext::ImmediateSubmit(fn);
	}

	void Renderer::OnWindowResize(uint32_t width, uint32_t height)
	{
		s_Data->PendingWidth  = width;
		s_Data->PendingHeight = height;
		s_Data->NeedsResize   = true;
	}

	const Renderer::Config& Renderer::GetConfig()
	{
		return s_Data->Config;
	}
	uint32_t Renderer::GetFramesInFlight()
	{
		return s_Data->Config.FramesInFlight;
	}
	uint32_t Renderer::GetCurrentFrameIndex()
	{
		return s_Data->CurrentFrame;
	}
	Vulkan::SwapChain& Renderer::GetSwapChain()
	{
		return *s_Data->Swapchain;
	}
	vk::Format Renderer::GetSwapChainFormat()
	{
		return s_Data->Swapchain->GetImageFormat();
	}
	vk::Format Renderer::GetDepthFormat()
	{
		return s_Data->DepthFormat;
	}
	vk::Extent2D Renderer::GetViewportExtent()
	{
		return s_Data->Swapchain->GetExtent();
	}

	void Renderer::SetClearColor(const glm::vec4& color)
	{
		s_Data->ClearColor = color;
	}
}        // namespace Eruption
