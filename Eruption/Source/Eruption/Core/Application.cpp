#include "Application.h"

#include "Eruption/Core/Input.h"
#include "Eruption/Renderer/Renderer.h"

#include <algorithm>
#include <atomic>
#include <csignal>

namespace Eruption
{
	namespace
	{
		std::atomic_bool s_StopRequested = false;

		extern "C" void SignalHandler(int)
		{
			s_StopRequested.store(true, std::memory_order_relaxed);
		}
	}        // namespace

	Application::Application(const ApplicationSpecification& specification) : m_Specification(specification)
	{
		Log::Init();

		std::signal(SIGINT, SignalHandler);
		std::signal(SIGTERM, SignalHandler);

		ER_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

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

		Renderer::Init(m_Window, Renderer::Config{.VSync = specification.VSync});
	}

	Application::~Application()
	{
		Renderer::WaitIdle();
		m_LayerStack.Clear();
		Renderer::Shutdown();

		s_Instance = nullptr;
		Log::Shutdown();
	}

	void Application::Run()
	{
		OnInit();

		m_IsRunning = true;
		while (m_IsRunning)
		{
			if (s_StopRequested.load(std::memory_order_relaxed))
				break;

			ProcessEvents();

			if (!m_Minimized)
			{
				Renderer::BeginFrame();

				for (const Scope<Layer>& layer : m_LayerStack)
					layer->OnUpdate(m_DeltaTime);

				for (const Scope<Layer>& layer : m_LayerStack)
					layer->OnRender();

				Renderer::EndFrame();
			}

			Input::ClearReleasedKeys();

			const float time = GetTime();
			m_FrameTime      = time - m_LastFrameTime;
			m_DeltaTime      = std::min(m_FrameTime.GetSeconds(), 0.0333f);
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
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e) { return OnWindowResize(e); });
		dispatcher.Dispatch<WindowMinimizeEvent>([this](WindowMinimizeEvent& e) { return OnWindowMinimize(e); });
		dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& e) { return OnWindowClose(e); });

		for (const Scope<Layer>& layer : std::views::reverse(m_LayerStack))
		{
			if (event.IsHandled)
				break;

			layer->OnEvent(event);
		}
	}

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

	Application& Application::Get()
	{
		ER_CORE_ASSERT(s_Instance, "Application not created!");
		return *s_Instance;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		const uint32_t width = e.GetWidth(), height = e.GetHeight();
		m_Minimized = (width == 0 || height == 0);

		if (!m_Minimized)
			Renderer::OnWindowResize(width, height);

		return false;        // let layers react too
	}

	bool Application::OnWindowMinimize(WindowMinimizeEvent& e)
	{
		m_Minimized = e.IsMinimized();
		return false;
	}

	bool Application::OnWindowClose(WindowCloseEvent&)
	{
		Stop();
		return false;
	}
}        // namespace Eruption
