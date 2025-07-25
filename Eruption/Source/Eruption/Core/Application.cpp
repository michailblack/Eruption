#include "Application.h"

#include "Eruption/Core/Input.h"
#include "Eruption/Core/Timer.h"

#include "Eruption/Renderer/Renderer.h"

#include <glm/ext/scalar_common.hpp>

namespace Eruption
{
	Application* Application::s_Instance = nullptr;

	Application::Application(const ApplicationSpecification& specification) : m_Specification(specification)
	{
		Log::Init();

		s_Instance = this;

		if (!specification.WorkingDirectory.empty())
			std::filesystem::current_path(specification.WorkingDirectory);

		WindowSpecification windowSpec;
		windowSpec.Title      = specification.Name;
		windowSpec.Width      = specification.WindowWidth;
		windowSpec.Height     = specification.WindowHeight;
		windowSpec.Fullscreen = specification.Fullscreen;
		windowSpec.VSync      = specification.VSync;

		m_Window = std::unique_ptr<Window>(Window::Create(windowSpec));
		m_Window->Init();

		// Init renderer and execute command queue to compile all shaders
		// Renderer::Init();

		if (specification.StartMaximized)
			m_Window->Maximize();
		else
			m_Window->CenterWindow();
		m_Window->SetResizable(specification.Resizable);
	}

	Application::~Application()
	{
		Log::Shutdown();
	}

	void Application::Run()
	{
		OnInit();
		while (m_Running)
		{
			static uint64_t s_FrameCounter = 0;

			ProcessEvents();

			if (!m_Minimized)
			{
				Timer cpuTimer;

				m_Window->SwapBuffers();

				m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % Renderer::GetConfig().FramesInFlight;
			}

			Input::ClearReleasedKeys();

			const float time = GetTime();
			m_FrameTime      = time - m_LastFrameTime;
			m_DeltaTime      = glm::min<float>(m_FrameTime, 0.0333f);
			m_LastFrameTime  = time;

			++s_FrameCounter;
		}

		OnShutdown();
	}

	void Application::Close()
	{
		m_Running = false;
	}

	void Application::OnShutdown()
	{}

	void Application::ProcessEvents() const
	{
		Input::TransitionPressedKeys();
		Input::TransitionPressedButtons();

		m_Window->ProcessEvents();
	}

	float Application::GetTime()
	{
		return static_cast<float>(glfwGetTime());
	}

	void Application::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e) { return OnWindowResize(e); });
		dispatcher.Dispatch<WindowMinimizeEvent>([this](WindowMinimizeEvent& e) { return OnWindowMinimize(e); });
		dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& e) { return OnWindowClose(e); });
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		const uint32_t width = e.GetWidth(), height = e.GetHeight();
		if (width == 0 || height == 0)
			return false;

		return false;
	}

	bool Application::OnWindowMinimize(WindowMinimizeEvent& e)
	{
		m_Minimized = e.IsMinimized();
		return false;
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		Close();
		return false;        // give other things a chance to react to window close
	}
}        // namespace Eruption