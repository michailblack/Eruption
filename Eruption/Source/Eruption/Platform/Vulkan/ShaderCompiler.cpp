#include "ShaderCompiler.h"

#include <slang-com-ptr.h>
#include <slang.h>

namespace Eruption::Vulkan
{
	namespace
	{
		std::mutex                           s_CompileMutex;
		Slang::ComPtr<slang::IGlobalSession> s_GlobalSession;

		slang::IGlobalSession* GlobalSession()
		{
			if (!s_GlobalSession)
			{
				if (SLANG_FAILED(slang::createGlobalSession(s_GlobalSession.writeRef())))
					return nullptr;
			}
			return s_GlobalSession.get();
		}

		vk::ShaderStageFlagBits ToVkStage(SlangStage stage)
		{
			switch (stage)
			{
				case SLANG_STAGE_VERTEX:   return vk::ShaderStageFlagBits::eVertex;
				case SLANG_STAGE_FRAGMENT: return vk::ShaderStageFlagBits::eFragment;
				case SLANG_STAGE_COMPUTE:  return vk::ShaderStageFlagBits::eCompute;
				case SLANG_STAGE_GEOMETRY: return vk::ShaderStageFlagBits::eGeometry;
				case SLANG_STAGE_HULL:     return vk::ShaderStageFlagBits::eTessellationControl;
				case SLANG_STAGE_DOMAIN:   return vk::ShaderStageFlagBits::eTessellationEvaluation;
				default:                   return vk::ShaderStageFlagBits::eAll;
			}
		}

		std::string BlobToString(slang::IBlob* blob)
		{
			if (!blob || !blob->getBufferPointer())
				return {};
			return std::string(static_cast<const char*>(blob->getBufferPointer()), blob->getBufferSize());
		}
	}        // namespace

	std::expected<std::vector<ShaderStageData>, std::string> ShaderCompiler::Compile(
	    const std::filesystem::path& slangFile
	)
	{
		std::scoped_lock lock(s_CompileMutex);

		slang::IGlobalSession* global = GlobalSession();
		if (!global)
			return std::unexpected("Failed to create Slang global session");

		slang::TargetDesc target{};
		target.format  = SLANG_SPIRV;
		target.profile = global->findProfile("spirv_1_5");

		const std::string searchPath = slangFile.parent_path().string();
		const char*       searchPtr  = searchPath.c_str();

		// Preserve entry-point names in the SPIR-V (otherwise Slang renames them all to "main", which
		// breaks VkPipelineShaderStageCreateInfo::pName when a module has multiple stages).
		const std::array compilerOptions = {
		    slang::CompilerOptionEntry{
		                               .name  = slang::CompilerOptionName::VulkanUseEntryPointName,
		                               .value = {.kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1}
		    }
		};

		slang::SessionDesc sessionDesc{};
		sessionDesc.targets         = &target;
		sessionDesc.targetCount     = 1;
		sessionDesc.searchPaths     = &searchPtr;
		sessionDesc.searchPathCount = 1;
		// GLM stores matrices column-major; matching the shader layout lets us upload matrices verbatim.
		sessionDesc.defaultMatrixLayoutMode  = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;
		sessionDesc.compilerOptionEntries    = const_cast<slang::CompilerOptionEntry*>(compilerOptions.data());
		sessionDesc.compilerOptionEntryCount = static_cast<uint32_t>(compilerOptions.size());

		Slang::ComPtr<slang::ISession> session;
		if (SLANG_FAILED(global->createSession(sessionDesc, session.writeRef())))
			return std::unexpected("Failed to create Slang session");

		Slang::ComPtr<slang::IBlob> diagnostics;

		const std::string moduleName = slangFile.stem().string();
		slang::IModule*   module     = session->loadModule(moduleName.c_str(), diagnostics.writeRef());
		if (!module)
			return std::unexpected(
			    std::format("Failed to load shader '{}':\n{}", slangFile.string(), BlobToString(diagnostics))
			);

		const SlangInt32 entryPointCount = module->getDefinedEntryPointCount();
		if (entryPointCount == 0)
			return std::unexpected(std::format("Shader '{}' defines no entry points", slangFile.string()));

		std::vector<slang::IComponentType*>            components;
		std::vector<Slang::ComPtr<slang::IEntryPoint>> entryPoints;
		components.reserve(entryPointCount + 1);
		components.push_back(module);

		for (SlangInt32 i = 0; i < entryPointCount; ++i)
		{
			Slang::ComPtr<slang::IEntryPoint> entryPoint;
			if (SLANG_FAILED(module->getDefinedEntryPoint(i, entryPoint.writeRef())))
				return std::unexpected(std::format("Failed to get entry point {} of '{}'", i, slangFile.string()));
			components.push_back(entryPoint.get());
			entryPoints.push_back(std::move(entryPoint));
		}

		Slang::ComPtr<slang::IComponentType> program;
		if (SLANG_FAILED(session->createCompositeComponentType(
		        components.data(), static_cast<SlangInt>(components.size()), program.writeRef(), diagnostics.writeRef()
		    )))
			return std::unexpected(
			    std::format("Failed to compose '{}':\n{}", slangFile.string(), BlobToString(diagnostics))
			);

		Slang::ComPtr<slang::IComponentType> linked;
		if (SLANG_FAILED(program->link(linked.writeRef(), diagnostics.writeRef())))
			return std::unexpected(
			    std::format("Failed to link '{}':\n{}", slangFile.string(), BlobToString(diagnostics))
			);

		slang::ProgramLayout* layout = linked->getLayout(0, diagnostics.writeRef());
		if (!layout)
			return std::unexpected(
			    std::format("Failed to reflect '{}':\n{}", slangFile.string(), BlobToString(diagnostics))
			);

		std::vector<ShaderStageData> stages;
		stages.reserve(entryPointCount);

		for (SlangInt32 i = 0; i < entryPointCount; ++i)
		{
			Slang::ComPtr<slang::IBlob> spirvBlob;
			if (SLANG_FAILED(linked->getEntryPointCode(i, 0, spirvBlob.writeRef(), diagnostics.writeRef())))
				return std::unexpected(
				    std::format("Codegen failed for '{}':\n{}", slangFile.string(), BlobToString(diagnostics))
				);

			slang::EntryPointReflection* entryReflection = layout->getEntryPointByIndex(i);

			const auto*  words     = static_cast<const uint32_t*>(spirvBlob->getBufferPointer());
			const size_t wordCount = spirvBlob->getBufferSize() / sizeof(uint32_t);

			stages.push_back(
			    ShaderStageData{
			        .Stage      = ToVkStage(entryReflection->getStage()),
			        .EntryPoint = entryReflection->getName(),
			        .SpirV      = std::vector<uint32_t>(words, words + wordCount)
			    }
			);
		}

		return stages;
	}
}        // namespace Eruption::Vulkan
