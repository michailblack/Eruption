#pragma once
#include "Eruption/Core/Base.h"

#include "Eruption/Core/Events/ApplicationEvent.h"

#include "Eruption/Core/DeltaTime.h"
#include "Eruption/Core/LayerStack.h"
#include "Eruption/Core/Window.h"

#include <string>

namespace Eruption
{
	struct ApplicationSpecification
	{
		std::string Name = "Eruption";
		std::string WorkingDirectory;
		uint32_t    WindowWidth    = 1600;
		uint32_t    WindowHeight   = 900;
		bool        Fullscreen     = false;
		bool        Resizable      = true;
		bool        StartMaximized = false;
		bool        VSync          = true;
	};

	class Application
	{
	public:
		explicit Application(const ApplicationSpecification& specification);
		virtual ~Application();

		virtual void OnInit() {}
		virtual void OnShutdown() {};

		void Run();
		void Stop();

		void RaiseEvent(Event& event);

		template <typename TLayer>
		    requires(std::is_base_of_v<Layer, TLayer>)
		void PushLayer()
		{
			m_LayerStack.PushLayer<TLayer>();
		}

		template <typename TLayer>
		    requires(std::is_base_of_v<Layer, TLayer>)
		void PushOverlay()
		{
			m_LayerStack.PushOverlay<TLayer>();
		}

		[[nodiscard]] const ApplicationSpecification& GetSpecification() const { return m_Specification; }

		[[nodiscard]] const Scope<Window>& GetWindow() const { return m_Window; }

		[[nodiscard]] DeltaTime GetDeltaTime() const { return m_DeltaTime; }
		[[nodiscard]] DeltaTime GetFrameTime() const { return m_FrameTime; }

		[[nodiscard]] static float GetTime();

		[[nodiscard]] static Application& Get();

	private:
		void ProcessEvents() const;
		void HandledQueuedEvents();

		bool OnWindowResize(WindowResizeEvent& e);
		bool OnWindowMinimize(WindowMinimizeEvent& e);
		bool OnWindowClose(WindowCloseEvent& e);

	private:
		ApplicationSpecification m_Specification;

		LayerStack m_LayerStack;

		Scope<Window> m_Window;

		DeltaTime m_DeltaTime;
		DeltaTime m_FrameTime;
		float     m_LastFrameTime = 0.0f;

		bool m_IsRunning = false;
		bool m_Minimized = false;
	};

	// Implemented by CLIENT
	Application* CreateApplication(int argc, char** argv);
}        // namespace Eruption
