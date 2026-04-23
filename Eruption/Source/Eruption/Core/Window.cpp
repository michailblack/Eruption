#include "Window.h"

#include "Eruption/Core/Events/ApplicationEvent.h"
#include "Eruption/Core/Events/KeyEvent.h"
#include "Eruption/Core/Events/MouseEvent.h"

#include "Eruption/Core/Input.h"

namespace Eruption
{
	static bool s_GLFWInitialized = false;

	static void GLFWErrorCallback(int error, const char* description)
	{
		ER_CORE_ERROR_TAG("GLFW", "GLFW Error ({0}): {1}", error, description);
	}

	Window::Window(const WindowSpecification& specification) : m_Specification(specification)
	{
		ER_CORE_INFO_TAG(
		    "GLFW",
		    "Creating window {0} ({1}, {2})",
		    m_Specification.Title,
		    m_Specification.Width,
		    m_Specification.Height
		);

		if (!s_GLFWInitialized)
		{
			const int success = glfwInit();
			ER_CORE_ASSERT(success, "Could not initialize GLFW!");
			glfwSetErrorCallback(GLFWErrorCallback);

			s_GLFWInitialized = true;
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		if (m_Specification.Fullscreen)
		{
			GLFWmonitor*       primaryMonitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode           = glfwGetVideoMode(primaryMonitor);

			glfwWindowHint(GLFW_DECORATED, false);
			glfwWindowHint(GLFW_RED_BITS, mode->redBits);
			glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
			glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
			glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

			m_Window = glfwCreateWindow(
			    mode->width, mode->height, m_Specification.Title.c_str(), primaryMonitor, nullptr
			);
		}
		else
		{
			m_Window = glfwCreateWindow(
			    static_cast<int>(m_Specification.Width),
			    static_cast<int>(m_Specification.Height),
			    m_Specification.Title.c_str(),
			    nullptr,
			    nullptr
			);
		}

		glfwSetWindowUserPointer(m_Window, this);

		if (glfwRawMouseMotionSupported())
			glfwSetInputMode(m_Window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
		else
			ER_CORE_WARN_TAG("Platform", "Raw mouse motion not supported.");

		SetCallbacks();

		// Update window size to actual size
		{
			int width, height;
			glfwGetWindowSize(m_Window, &width, &height);
			m_Specification.Width  = width;
			m_Specification.Height = height;
		}
	}

	Window::~Window()
	{
		if (m_Window)
		{
			glfwDestroyWindow(m_Window);
			m_Window = nullptr;
		}

		glfwTerminate();
		s_GLFWInitialized = false;
	}

	void Window::SetCallbacks() const
	{
		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height) {
			auto& wnd = *static_cast<Window*>(glfwGetWindowUserPointer(window));

			wnd.m_Specification.Width  = width;
			wnd.m_Specification.Height = height;

			WindowResizeEvent event(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
			wnd.RaiseEvent(event);
		});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window) {
			const auto& wnd = *static_cast<Window*>(glfwGetWindowUserPointer(window));

			WindowCloseEvent event;
			wnd.RaiseEvent(event);
		});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
			const auto& wnd = *static_cast<Window*>(glfwGetWindowUserPointer(window));
			switch (action)
			{
				case GLFW_PRESS:
				{
					Input::UpdateKeyState(static_cast<KeyCode>(key), KeyState::Pressed);

					KeyPressedEvent event(static_cast<KeyCode>(key), 0);
					wnd.RaiseEvent(event);
					break;
				}
				case GLFW_RELEASE:
				{
					Input::UpdateKeyState(static_cast<KeyCode>(key), KeyState::Released);

					KeyReleasedEvent event(static_cast<KeyCode>(key));
					wnd.RaiseEvent(event);
					break;
				}
				case GLFW_REPEAT:
				{
					Input::UpdateKeyState(static_cast<KeyCode>(key), KeyState::Held);

					KeyPressedEvent event(static_cast<KeyCode>(key), 1);
					wnd.RaiseEvent(event);
					break;
				}
			}
		});

		glfwSetCharCallback(m_Window, [](GLFWwindow* window, uint32_t codepoint) {
			const auto& wnd = *static_cast<Window*>(glfwGetWindowUserPointer(window));

			KeyTypedEvent event(static_cast<KeyCode>(codepoint));
			wnd.RaiseEvent(event);
		});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods) {
			const auto& wnd = *static_cast<Window*>(glfwGetWindowUserPointer(window));
			switch (action)
			{
				case GLFW_PRESS:
				{
					Input::UpdateButtonState(static_cast<MouseButton>(button), KeyState::Pressed);

					MouseButtonPressedEvent event(static_cast<MouseButton>(button));
					wnd.RaiseEvent(event);
					break;
				}
				case GLFW_RELEASE:
				{
					Input::UpdateButtonState(static_cast<MouseButton>(button), KeyState::Released);

					MouseButtonReleasedEvent event(static_cast<MouseButton>(button));
					wnd.RaiseEvent(event);
					break;
				}
			}
		});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset) {
			const auto& wnd = *static_cast<Window*>(glfwGetWindowUserPointer(window));

			MouseScrolledEvent event(static_cast<float>(xOffset), static_cast<float>(yOffset));
			wnd.RaiseEvent(event);
		});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double x, double y) {
			const auto& wnd = *static_cast<Window*>(glfwGetWindowUserPointer(window));

			MouseMovedEvent event(static_cast<float>(x), static_cast<float>(y));
			wnd.RaiseEvent(event);
		});
	}

	void Window::ProcessEvents()
	{
		glfwPollEvents();
	}

	void Window::SwapBuffers() const
	{
		glfwSwapBuffers(m_Window);
	}

	void Window::SetTitle(const std::string& title)
	{
		m_Specification.Title = title;
		glfwSetWindowTitle(m_Window, m_Specification.Title.c_str());
	}

	void Window::SetResizable(bool resizable) const
	{
		glfwSetWindowAttrib(m_Window, GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
	}

	void Window::Maximize() const
	{
		glfwMaximizeWindow(m_Window);
	}

	void Window::Restore() const
	{
		glfwRestoreWindow(m_Window);
	}

	void Window::CenterWindow() const
	{
		const GLFWvidmode* videoMode = glfwGetVideoMode(glfwGetPrimaryMonitor());

		const int x = (videoMode->width / 2) - (m_Specification.Width / 2);
		const int y = (videoMode->height / 2) - (m_Specification.Height / 2);

		glfwSetWindowPos(m_Window, x, y);
	}

	void Window::RaiseEvent(Event& event) const
	{
		if (m_Specification.EventCallback)
			m_Specification.EventCallback(event);
	}

	std::pair<uint32_t, uint32_t> Window::GetFramebufferSize() const
	{
		int width, height;
		glfwGetFramebufferSize(m_Window, &width, &height);

		return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
	}

	std::pair<float, float> Window::GetWindowPos() const
	{
		int x, y;
		glfwGetWindowPos(m_Window, &x, &y);
		return {static_cast<float>(x), static_cast<float>(y)};
	}
}        // namespace Eruption
