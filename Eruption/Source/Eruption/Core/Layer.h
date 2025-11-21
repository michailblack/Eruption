#pragma once
#include "Eruption/Core/DeltaTime.h"
#include "Eruption/Core/Events/Event.h"

#include <string>

namespace Eruption
{
	class Layer
	{
	public:
		explicit Layer(const std::string& debugName = "Layer") : m_DebugName(debugName) {}
		virtual ~Layer() = default;

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate(DeltaTime dt) {}
		virtual void OnEvent(Event& event) {}
		virtual void OnImGuiRender() {}

		void SetEnabled(bool isEnabled) { m_IsEnabled = isEnabled; }

		bool IsEnabled() const { return m_IsEnabled; }

		const std::string& GetDebugName() const { return m_DebugName; }

	protected:
		std::string m_DebugName;
		bool        m_IsEnabled = true;
	};
}        // namespace Eruption
