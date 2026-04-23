#include "Application.h"

#include "Eruption/Core/Input.h"
#include "Eruption/Core/Timer.h"
#include "Eruption/Renderer/Renderer.h"

#include <glm/ext/scalar_common.hpp>

namespace Eruption
{
	static Application* s_Application = nullptr;

	Application::Application(const ApplicationSpecification& specification) : m_Specification(specification)
	{
		Log::Init();

		s_Application = this;

		if (!specification.WorkingDirectory.empty())
			std::filesystem::current_path(specification.WorkingDirectory);

		WindowSpecification windowSpec;
		windowSpec.Title         = specification.Name;
		windowSpec.Width         = specification.WindowWidth;
		windowSpec.Height        = specification.WindowHeight;
		windowSpec.Fullscreen    = specification.Fullscreen;
		windowSpec.EventCallback = [this](Event& event) { RaiseEvent(event); };

		m_Window = CreateScope<Window>(windowSpec);

		if (specification.StartMaximized)
			m_Window->Maximize();
		else
			m_Window->CenterWindow();

		m_Window->SetResizable(specification.Resizable);

		Renderer::Init(m_Window);
	}

	Application::~Application()
	{
		Log::Shutdown();
	}

	void Application::Run()
	{
		OnInit();

		m_IsRunning = true;

		while (m_IsRunning)
		{
			ProcessEvents();

			if (!m_Minimized)
			{
				HandledQueuedEvents();

				for (const std::unique_ptr<Layer>& layer : m_LayerStack)
					layer->OnUpdate(m_DeltaTime);

				m_Window->SwapBuffers();
			}

			Input::ClearReleasedKeys();

			const float time = GetTime();
			m_FrameTime      = time - m_LastFrameTime;
			m_DeltaTime      = glm::min<float>(m_FrameTime, 0.0333f);
			m_LastFrameTime  = time;
		}

		OnShutdown();
	}

	void Application::Stop()
	{
		m_IsRunning = false;
	}

	void Application::RaiseEvent(Event& event)
	{
		for (const std::unique_ptr<Layer>& layer : std::views::reverse(m_LayerStack))
		{
			layer->OnEvent(event);
			if (event.IsHandled)
				break;
		}
	}

	void Application::ProcessEvents() const
	{
		Input::TransitionPressedKeys();
		Input::TransitionPressedButtons();

		m_Window->ProcessEvents();
	}
	void Application::HandledQueuedEvents()
	{
		// m_EventBus.ProcessQueue();
	}

	float Application::GetTime()
	{
		return static_cast<float>(glfwGetTime());
	}

	Application& Application::Get()
	{
		ER_CORE_ASSERT(s_Application);
		return *s_Application;
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
		Stop();
		return false;        // give other things a chance to react to window close
	}
}        // namespace Eruption