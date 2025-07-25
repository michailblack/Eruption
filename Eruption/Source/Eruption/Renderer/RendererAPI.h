#pragma once

namespace Eruption
{
	class RendererAPI
	{
	public:
		enum class Type
		{
			None = 0,
			Vulkan
		};

	public:
		static void SetAPI(Type api);
		static Type GetAPI() { return s_CurrentRendererAPI; }

	private:
		inline static Type s_CurrentRendererAPI = Type::Vulkan;
	};
}        // namespace Eruption