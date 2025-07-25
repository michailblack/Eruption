#include "Renderer.h"

namespace Eruption
{
	namespace
	{
		RendererConfig s_RendererConfig;
	}

	const RendererConfig& Renderer::GetConfig()
	{
		return s_RendererConfig;
	}
}        // namespace Eruption