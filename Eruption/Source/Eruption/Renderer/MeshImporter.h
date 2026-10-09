#pragma once
#include "Eruption/Platform/Vulkan/Texture.h"
#include "Eruption/Renderer/Mesh.h"

#include <glm/glm.hpp>

#include <filesystem>
#include <optional>

namespace Eruption
{
	struct ImportedMaterial
	{
		Ref<Vulkan::Texture2D> AlbedoTexture;        // null if the source material has none
		glm::vec4              BaseColorFactor = glm::vec4(1.0f);
	};

	struct ImportedModel
	{
		Ref<Mesh>                     Geometry;
		std::vector<ImportedMaterial> Materials;
	};

	// Loads a model with assimp. All meshes are triangulated, merged into a single vertex/index buffer
	// and split into submeshes (one per source mesh) that index into Materials. Missing normals/tangents
	// are generated. Node transforms are baked in (static models).
	class MeshImporter
	{
	public:
		[[nodiscard]] static std::optional<ImportedModel> Load(const std::filesystem::path& path);

		// Geometry only (ignores materials/textures).
		[[nodiscard]] static Ref<Mesh> LoadMesh(const std::filesystem::path& path);
	};
}        // namespace Eruption
