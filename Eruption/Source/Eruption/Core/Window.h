#pragma once
#include "Eruption/Platform/Vulkan/Vulkan.h"

#include "Eruption/Renderer/RendererContext.h"

#include <GLFW/glfw3.h>

#include <string>

namespace Eruption
{
	class VulkanSwapChain;

	struct WindowSpecification
	{
		std::string Title      = "Eruption";
		uint32_t    Width      = 1600;
		uint32_t    Height     = 900;
		bool        Fullscreen = false;
		bool        VSync      = true;
	};

	class Window
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>;

	public:
		explicit Window(const WindowSpecification& specification);
		virtual ~Window();

		virtual void Init();
		virtual void ProcessEvents();
		virtual void SwapBuffers();

		virtual void SetTitle(const std::string& title);
		virtual void SetResizable(bool resizable) const;
		virtual void SetVSync(bool enabled);

		virtual void Maximize();
		virtual void CenterWindow();

		void SetEventCallback(const EventCallbackFn& callback) { m_Data.EventCallback = callback; }

		[[nodiscard]] uint32_t GetWidth() const { return m_Data.Width; }
		[[nodiscard]] uint32_t GetHeight() const { return m_Data.Height; }

		[[nodiscard]] virtual std::pair<uint32_t, uint32_t> GetSize() const { return {m_Data.Width, m_Data.Height}; }

		[[nodiscard]] virtual std::pair<float, float> GetWindowPos() const;

		[[nodiscard]] virtual bool IsVSync() const { return m_Specification.VSync; }

		[[nodiscard]] virtual const std::string& GetTitle() const { return m_Data.Title; }

		[[nodiscard]] virtual Ref<RendererContext> GetRendererContext() const { return m_RendererContext; }
		[[nodiscard]] void*                        GetNativeWindow() const { return m_Window; }

	public:
		static Window* Create(const WindowSpecification& specification = WindowSpecification{});

	private:
		virtual void Shutdown();

	private:
		WindowSpecification m_Specification;

		struct WindowData
		{
			std::string     Title;
			uint32_t        Width, Height;
			EventCallbackFn EventCallback;
		} m_Data;

		Ref<RendererContext>   m_RendererContext;
		Scope<VulkanSwapChain> m_SwapChain;

		GLFWwindow* m_Window;

		float m_LastFrameTime = 0.0f;
	};
}        // namespace Eruption
