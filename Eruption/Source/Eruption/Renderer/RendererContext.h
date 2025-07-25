#pragma once
#include "Eruption/Core/Base.h"

struct GLFWwindow;

namespace Eruption
{
	class RendererContext
	{
	public:
		RendererContext()          = default;
		virtual ~RendererContext() = default;

		virtual void Init(GLFWwindow* window) = 0;

		static Ref<RendererContext> Create();
	};
}        // namespace Eruption
