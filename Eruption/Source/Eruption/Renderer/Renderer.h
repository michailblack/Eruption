#pragma once
#include "Eruption/Renderer/RendererDevice.h"

namespace Eruption
{
	class Renderer
	{
	public:
		struct Config
		{
			uint32_t FramesInFlight = 3;
		};

	public:
		static void Init(const Scope<Window>& window);

		static Ref<RendererDevice> GetRendererDevice();

		static const Config& GetConfig();
	};

}        // namespace Eruption
