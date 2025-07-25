#pragma once
#include "Eruption/Core/Events/ApplicationEvent.h"

#include "Eruption/Core/DeltaTime.h"
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

		[[nodiscard]] Window& GetWindow() const { return *m_Window; }

		static Application& Get() { return *s_Instance; }

		[[nodiscard]] static float GetTime();
		[[nodiscard]] DeltaTime    GetDeltaTime() const { return m_DeltaTime; }
		[[nodiscard]] DeltaTime    GetFrameTime() const { return m_FrameTime; }

		[[nodiscard]] const ApplicationSpecification& GetSpecification() const { return m_Specification; }

		[[nodiscard]] uint32_t GetCurrentFrameIndex() const { return m_CurrentFrameIndex; }

	private:
		void ProcessEvents() const;

		bool OnWindowResize(WindowResizeEvent& e);
		bool OnWindowMinimize(WindowMinimizeEvent& e);
		bool OnWindowClose(WindowCloseEvent& e);

	private:
		std::unique_ptr<Window>  m_Window;
		ApplicationSpecification m_Specification;
		DeltaTime                m_DeltaTime;
		DeltaTime                m_FrameTime;
		bool                     m_Running   = true;
		bool                     m_Minimized = false;

		float    m_LastFrameTime     = 0.0f;
		uint32_t m_CurrentFrameIndex = 0;

		static Application* s_Instance;
	};

	// Implemented by CLIENT
	Application* CreateApplication(int argc, char** argv);
}        // namespace Eruption
