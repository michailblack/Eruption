#include "Window.h"

#include "Eruption/Core/Events/ApplicationEvent.h"
#include "Eruption/Core/Events/KeyEvent.h"
#include "Eruption/Core/Events/MouseEvent.h"

#include "Eruption/Core/Input.h"

#include "Eruption/Platform/Vulkan/VulkanContext.h"
#include "Eruption/Platform/Vulkan/VulkanSwapChain.h"

#include "Eruption/Renderer/RendererAPI.h"

namespace Eruption
{
	static bool s_GLFWInitialized{false};

	static void GLFWErrorCallback(int error, const char* description)
	{
		ER_CORE_ERROR_TAG("GLFW", "GLFW Error ({0}): {1}", error, description);
	}

	Window* Window::Create(const WindowSpecification& specification)
	{
		return new Window(specification);
	}

	Window::Window(const WindowSpecification& specification) :
	    m_Specification(specification), m_Data(), m_Window(nullptr)
	{}

	Window::~Window()
	{
		Window::Shutdown();
	}

	void Window::Init()
	{
		m_Data.Title  = m_Specification.Title;
		m_Data.Width  = m_Specification.Width;
		m_Data.Height = m_Specification.Height;

		ER_CORE_INFO_TAG(
		    "GLFW",
		    "Creating window {0} ({1}, {2})",
		    m_Specification.Title,
		    m_Specification.Width,
		    m_Specification.Height
		);

		if (!s_GLFWInitialized)
		{
			// TODO: glfwTerminate on system shutdown
			const int success = glfwInit();
			ER_CORE_ASSERT(success, "Could not initialize GLFW!");
			glfwSetErrorCallback(GLFWErrorCallback);

			s_GLFWInitialized = true;
		}

		if (RendererAPI::GetAPI() == RendererAPI::Type::Vulkan)
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

			m_Window = glfwCreateWindow(mode->width, mode->height, m_Data.Title.c_str(), primaryMonitor, nullptr);
		}
		else
		{
			m_Window = glfwCreateWindow(
			    static_cast<int>(m_Specification.Width),
			    static_cast<int>(m_Specification.Height),
			    m_Data.Title.c_str(),
			    nullptr,
			    nullptr
			);
		}

		// Create Renderer Context
		m_RendererContext = RendererContext::Create();
		m_RendererContext->Create(m_Window);

		auto vulkanContext = As<VulkanContext>(m_RendererContext);

		SwapChainSpecification swapChainSpecification{};
		swapChainSpecification.Surface              = vulkanContext->GetSurface();
		swapChainSpecification.DesiredExtent.width  = m_Specification.Width;
		swapChainSpecification.DesiredExtent.height = m_Specification.Height;

		m_SwapChain = CreateScope<VulkanSwapChain>(swapChainSpecification);

		// glfwMaximizeWindow(m_Window);
		glfwSetWindowUserPointer(m_Window, &m_Data);

		if (glfwRawMouseMotionSupported())
			glfwSetInputMode(m_Window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
		else
			ER_CORE_WARN_TAG("Platform", "Raw mouse motion not supported.");

		// Set GLFW callbacks
		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height) {
			auto& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

			WindowResizeEvent event(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
			data.Width  = width;
			data.Height = height;
			data.EventCallback(event);
		});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window) {
			const auto& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

			WindowCloseEvent event;
			data.EventCallback(event);
		});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
			const auto& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
			switch (action)
			{
				case GLFW_PRESS:
				{
					Input::UpdateKeyState(static_cast<KeyCode>(key), KeyState::Pressed);

					KeyPressedEvent event(static_cast<KeyCode>(key), 0);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					Input::UpdateKeyState(static_cast<KeyCode>(key), KeyState::Released);

					KeyReleasedEvent event(static_cast<KeyCode>(key));
					data.EventCallback(event);
					break;
				}
				case GLFW_REPEAT:
				{
					Input::UpdateKeyState(static_cast<KeyCode>(key), KeyState::Held);

					KeyPressedEvent event(static_cast<KeyCode>(key), 1);
					data.EventCallback(event);
					break;
				}
			}
		});

		glfwSetCharCallback(m_Window, [](GLFWwindow* window, uint32_t codepoint) {
			const auto& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

			KeyTypedEvent event(static_cast<KeyCode>(codepoint));
			data.EventCallback(event);
		});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods) {
			const auto& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
			switch (action)
			{
				case GLFW_PRESS:
				{
					Input::UpdateButtonState(static_cast<MouseButton>(button), KeyState::Pressed);

					MouseButtonPressedEvent event(static_cast<MouseButton>(button));
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					Input::UpdateButtonState(static_cast<MouseButton>(button), KeyState::Released);

					MouseButtonReleasedEvent event(static_cast<MouseButton>(button));
					data.EventCallback(event);
					break;
				}
			}
		});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset) {
			const auto& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

			MouseScrolledEvent event(static_cast<float>(xOffset), static_cast<float>(yOffset));
			data.EventCallback(event);
		});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double x, double y) {
			const auto& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

			MouseMovedEvent event(static_cast<float>(x), static_cast<float>(y));
			data.EventCallback(event);
		});

		// Update window size to actual size
		{
			int width, height;
			glfwGetWindowSize(m_Window, &width, &height);
			m_Data.Width  = width;
			m_Data.Height = height;
		}
	}

	void Window::Shutdown()
	{
		// m_SwapChain->Destroy();
		// hdelete m_SwapChain;
		// m_RendererContext.As<VulkanContext>()->GetDevice()->Destroy(
		// );        // need to destroy the device _before_ windows window destructor destroys the renderer context
		//           // (because device Destroy() asks for renderer context...)
		glfwTerminate();
		s_GLFWInitialized = false;
	}

	void Window::ProcessEvents()
	{
		glfwPollEvents();
	}

	void Window::SwapBuffers()
	{}

	void Window::SetTitle(const std::string& title)
	{
		m_Data.Title = title;
		glfwSetWindowTitle(m_Window, m_Data.Title.c_str());
	}

	void Window::SetResizable(bool resizable) const
	{
		glfwSetWindowAttrib(m_Window, GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
	}

	void Window::SetVSync(bool enabled)
	{
		m_Specification.VSync = enabled;

		// Application::Get().QueueEvent([&]() {
		// 	m_SwapChain->SetVSync(m_Specification.VSync);
		// 	m_SwapChain->OnResize(m_Specification.Width, m_Specification.Height);
		// });
	}

	void Window::Maximize()
	{
		glfwMaximizeWindow(m_Window);
	}

	void Window::CenterWindow()
	{
		const GLFWvidmode* videoMode = glfwGetVideoMode(glfwGetPrimaryMonitor());

		const int x = (videoMode->width / 2) - (m_Data.Width / 2);
		const int y = (videoMode->height / 2) - (m_Data.Height / 2);

		glfwSetWindowPos(m_Window, x, y);
	}

	std::pair<float, float> Window::GetWindowPos() const
	{
		int x, y;
		glfwGetWindowPos(m_Window, &x, &y);
		return {static_cast<float>(x), static_cast<float>(y)};
	}
}        // namespace Eruption
