#pragma once
#include "Eruption/Core/DeltaTime.h"
#include "Eruption/Core/Events/Event.h"

#include <string>

namespace Eruption
{
	class Layer
	{
	public:
		explicit Layer(std::string name = "Layer") : m_Name(std::move(name)) {}
		virtual ~Layer() = default;

		virtual void OnAttach() {}
		virtual void OnDetach() {}

		virtual void OnUpdate(DeltaTime dt) {}

		virtual void OnRender() {}

		virtual void OnEvent(Event& event) {}

		[[nodiscard]] const std::string& GetName() const { return m_Name; }

	protected:
		std::string m_Name;
	};
}        // namespace Eruption
