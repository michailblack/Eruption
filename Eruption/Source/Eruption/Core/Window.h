#pragma once
#include <GLFW/glfw3.h>

#include <string>

namespace Eruption
{
	struct WindowSpecification
	{
		std::string Title      = "Eruption";
		uint32_t    Width      = 1600;
		uint32_t    Height     = 900;
		bool        Fullscreen = false;

		using EventCallbackFn = std::function<void(Event&)>;
		EventCallbackFn EventCallback;
	};

	class Window
	{
	public:
		explicit Window(const WindowSpecification& specification);
		~Window();

		void ProcessEvents();
		void SwapBuffers() const;

		void SetTitle(const std::string& title);
		void SetResizable(bool resizable) const;

		void Maximize() const;
		void Restore() const;
		void CenterWindow() const;

		void RaiseEvent(Event& event) const;

		[[nodiscard]] uint32_t GetWidth() const { return m_Specification.Width; }
		[[nodiscard]] uint32_t GetHeight() const { return m_Specification.Height; }

		[[nodiscard]] std::pair<uint32_t, uint32_t> GetFramebufferSize() const;

		[[nodiscard]] std::pair<float, float> GetWindowPos() const;

		[[nodiscard]] const std::string& GetTitle() const { return m_Specification.Title; }

		[[nodiscard]] const GLFWwindow* GetNativeWindow() const { return m_Window; }
		[[nodiscard]] GLFWwindow*       GetNativeWindow() { return m_Window; }

	private:
		void SetCallbacks() const;

	private:
		WindowSpecification m_Specification;

		GLFWwindow* m_Window = nullptr;
	};
}        // namespace Eruption
