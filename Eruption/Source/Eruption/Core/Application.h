#pragma once
#include "Eruption/Core/Events/ApplicationEvent.h"
#include "Eruption/Core/Events/EventBus.h"

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
		bool        EnableImGui    = true;
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

		void Run();
		void Close();

		virtual void OnInit() {}
		virtual void OnShutdown();
		virtual void OnUpdate(DeltaTime ts) {}

		virtual void OnEvent(Event& event);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);
		void PopLayer(Layer* layer);
		void PopOverlay(Layer* layer);

		[[nodiscard]] const ApplicationSpecification& GetSpecification() const { return m_Specification; }

		[[nodiscard]] const EventBus& GetEventBus() const { return m_EventBus; }
		[[nodiscard]] EventBus&       GetEventBus() { return m_EventBus; }

		[[nodiscard]] Window& GetWindow() const { return *m_Window; }

		[[nodiscard]] DeltaTime GetDeltaTime() const { return m_DeltaTime; }
		[[nodiscard]] DeltaTime GetFrameTime() const { return m_FrameTime; }

		[[nodiscard]] uint32_t GetCurrentFrameIndex() const { return m_CurrentFrameIndex; }

		[[nodiscard]] static float GetTime();

		[[nodiscard]] static Application& Get() { return *s_Instance; }

	private:
		void ProcessEvents() const;
		void HandledQueuedEvents();

		bool OnWindowResize(WindowResizeEvent& e);
		bool OnWindowMinimize(WindowMinimizeEvent& e);
		bool OnWindowClose(WindowCloseEvent& e);

	private:
		EventBus m_EventBus;

		ApplicationSpecification m_Specification;

		LayerStack m_LayerStack;

		std::unique_ptr<Window> m_Window;

		DeltaTime m_DeltaTime;
		DeltaTime m_FrameTime;
		bool      m_Running   = true;
		bool      m_Minimized = false;

		float    m_LastFrameTime     = 0.0f;
		uint32_t m_CurrentFrameIndex = 0;

		static Application* s_Instance;
	};

	// Implemented by CLIENT
	Application* CreateApplication(int argc, char** argv);
}        // namespace Eruption
