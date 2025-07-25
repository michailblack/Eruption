#pragma once
#include "Eruption/Core/Application.h"

#include "Eruption/Renderer/RendererConfig.h"
#include "Eruption/Renderer/RendererContext.h"

namespace Eruption
{

	class Renderer
	{
	public:
		static Ref<RendererContext> GetContext() { return Application::Get().GetWindow().GetRendererContext(); }

		static const RendererConfig& GetConfig();
	};

}        // namespace Eruption
