#pragma once
#include "Eruption/Platform/Vulkan/ShaderCompiler.h"

namespace Eruption::Vulkan
{
	// A compiled shader program: one Slang source file compiled to a set of SPIR-V stage modules.
	// Reload() recompiles from disk (hot-reload); on failure the previous modules are kept.
	class Shader
	{
	public:
		explicit Shader(std::filesystem::path slangFile);

		Shader(const Shader&)            = delete;
		Shader& operator=(const Shader&) = delete;

		[[nodiscard]] bool Reload();

		[[nodiscard]] std::vector<vk::PipelineShaderStageCreateInfo> GetStageCreateInfos() const;
		[[nodiscard]] const std::filesystem::path&                   GetPath() const { return m_Path; }
		[[nodiscard]] bool                                           IsValid() const { return !m_Modules.empty(); }

		[[nodiscard]] static Ref<Shader> Create(const std::filesystem::path& slangFile);

	private:
		void CreateModules(const std::vector<ShaderStageData>& stages);

	private:
		std::filesystem::path m_Path;

		std::vector<vk::raii::ShaderModule>  m_Modules;
		std::vector<vk::ShaderStageFlagBits> m_Stages;
		std::vector<std::string>             m_EntryPoints;        // kept alive for pName pointers
	};
}        // namespace Eruption::Vulkan
