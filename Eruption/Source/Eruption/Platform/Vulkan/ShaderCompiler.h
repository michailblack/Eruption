#pragma once
#include "Eruption/Platform/Vulkan/Vulkan.h"

#include <expected>
#include <filesystem>
#include <string>

namespace Eruption::Vulkan
{
	struct ShaderStageData
	{
		vk::ShaderStageFlagBits Stage;
		std::string             EntryPoint;
		std::vector<uint32_t>   SpirV;
	};

	// Compiles Slang shaders to SPIR-V at runtime using the Slang runtime (bundled with the Vulkan SDK).
	// One entry per `[shader("...")]` entry point in the module is returned. Thread-safe.
	class ShaderCompiler
	{
	public:
		[[nodiscard]] static std::expected<std::vector<ShaderStageData>, std::string> Compile(
		    const std::filesystem::path& slangFile
		);
	};
}        // namespace Eruption::Vulkan
