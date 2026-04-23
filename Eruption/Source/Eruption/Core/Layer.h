#pragma once
#include "Eruption/Core/DeltaTime.h"

namespace Eruption
{
	class Layer
	{
	public:
		virtual ~Layer() = default;

		virtual void OnEvent(Event& event) {}

		virtual void OnUpdate(DeltaTime dt) {}
		virtual void OnRender() {}

		// TODO: Implement
		// void TransitionTo(Scope<Layer>> layer);
		// void Suspend();
	};
}        // namespace Eruption
