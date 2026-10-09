#pragma once
#include "Eruption/Platform/Vulkan/SwapChain.h"
#include "Eruption/Renderer/RenderThread.h"

#include <glm/glm.hpp>

#include <functional>

namespace Eruption
{
	class Window;

	using RenderCommand = std::move_only_function<void(const vk::raii::CommandBuffer&)>;

	class Renderer
	{
	public:
		struct Config
		{
			uint32_t        FramesInFlight = 2;
			ThreadingPolicy Threading      = ThreadingPolicy::MultiThreaded;
			bool            VSync          = true;
			glm::vec4       ClearColor     = {0.10f, 0.10f, 0.12f, 1.0f};
		};

		static void Init(const Scope<Window>& window, const Config& config);
		static void Init(const Scope<Window>& window);
		static void Shutdown();

		static void WaitIdle();

		static void BeginFrame();
		static void EndFrame();

		static void Submit(RenderCommand&& command);

		static void SubmitResourceFree(std::move_only_function<void()>&& fn);

		static void ImmediateSubmit(const std::function<void(const vk::raii::CommandBuffer&)>& fn);

		static void OnWindowResize(uint32_t width, uint32_t height);

		[[nodiscard]] static const Config&      GetConfig();
		[[nodiscard]] static uint32_t           GetFramesInFlight();
		[[nodiscard]] static uint32_t           GetCurrentFrameIndex();
		[[nodiscard]] static Vulkan::SwapChain& GetSwapChain();
		[[nodiscard]] static vk::Format         GetSwapChainFormat();
		[[nodiscard]] static vk::Format         GetDepthFormat();
		[[nodiscard]] static vk::Extent2D       GetViewportExtent();

		static void SetClearColor(const glm::vec4& color);
	};
}        // namespace Eruption
