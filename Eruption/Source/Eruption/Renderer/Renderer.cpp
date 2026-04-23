#include "Renderer.h"

namespace Eruption
{
	namespace
	{
		struct
		{
			Renderer::Config Config;

			Ref<RendererDevice> RendererDevice;
		} s_Data;
	}        // namespace

	void Renderer::Init(const Scope<Window>& window)
	{
		s_Data.RendererDevice = CreateRef<RendererDevice>(window);
	}

	Ref<RendererDevice> Renderer::GetRendererDevice()
	{
		return s_Data.RendererDevice;
	}

	const Renderer::Config& Renderer::GetConfig()
	{
		return s_Data.Config;
	}
}        // namespace Eruption