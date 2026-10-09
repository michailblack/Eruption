#include "Shader.h"

#include "Eruption/Platform/Vulkan/VulkanContext.h"

namespace Eruption::Vulkan
{
	Shader::Shader(std::filesystem::path slangFile) : m_Path(std::move(slangFile))
	{
		std::expected<std::vector<ShaderStageData>, std::string> compiled = ShaderCompiler::Compile(m_Path);
		if (!compiled)
		{
			ER_CORE_ERROR_TAG("Renderer", "Shader compilation failed:\n{}", compiled.error());
			ER_CORE_VERIFY(false, "Shader compilation failed");
			return;
		}

		CreateModules(*compiled);
		ER_CORE_INFO_TAG("Renderer", "Compiled shader '{}' ({} stages)", m_Path.filename().string(), m_Modules.size());
	}

	void Shader::CreateModules(const std::vector<ShaderStageData>& stages)
	{
		const vk::raii::Device& device = VulkanContext::GetVkDevice();

		m_Modules.clear();
		m_Stages.clear();
		m_EntryPoints.clear();
		m_Modules.reserve(stages.size());
		m_Stages.reserve(stages.size());
		m_EntryPoints.reserve(stages.size());

		for (const ShaderStageData& stage : stages)
		{
			m_Modules.emplace_back(
			    device,
			    vk::ShaderModuleCreateInfo{
			        .codeSize = stage.SpirV.size() * sizeof(uint32_t), .pCode = stage.SpirV.data()
			    }
			);
			m_Stages.push_back(stage.Stage);
			m_EntryPoints.push_back(stage.EntryPoint);
		}
	}

	bool Shader::Reload()
	{
		std::expected<std::vector<ShaderStageData>, std::string> compiled = ShaderCompiler::Compile(m_Path);
		if (!compiled)
		{
			ER_CORE_ERROR_TAG("Renderer", "Shader reload failed (keeping previous):\n{}", compiled.error());
			return false;
		}

		VulkanContext::GetVkDevice().waitIdle();        // no in-flight use of the old modules
		CreateModules(*compiled);
		ER_CORE_INFO_TAG("Renderer", "Reloaded shader '{}'", m_Path.filename().string());
		return true;
	}

	std::vector<vk::PipelineShaderStageCreateInfo> Shader::GetStageCreateInfos() const
	{
		std::vector<vk::PipelineShaderStageCreateInfo> infos;
		infos.reserve(m_Modules.size());

		for (size_t i = 0; i < m_Modules.size(); ++i)
		{
			infos.push_back(
			    vk::PipelineShaderStageCreateInfo{
			        .stage = m_Stages[i], .module = *m_Modules[i], .pName = m_EntryPoints[i].c_str()
			    }
			);
		}
		return infos;
	}

	Ref<Shader> Shader::Create(const std::filesystem::path& slangFile)
	{
		return CreateRef<Shader>(slangFile);
	}
}        // namespace Eruption::Vulkan
