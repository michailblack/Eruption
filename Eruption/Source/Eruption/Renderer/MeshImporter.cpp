#include "MeshImporter.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <stb_image.h>

namespace Eruption
{
	namespace
	{
		constexpr unsigned int IMPORT_FLAGS = aiProcess_Triangulate | aiProcess_GenSmoothNormals |
		                                      aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices |
		                                      aiProcess_FlipUVs | aiProcess_PreTransformVertices |
		                                      aiProcess_GenBoundingBoxes;

		glm::vec2 ToVec2(const aiVector3D& v)
		{
			return {v.x, v.y};
		}
		glm::vec3 ToVec3(const aiVector3D& v)
		{
			return {v.x, v.y, v.z};
		}

		void ExtractGeometry(
		    const aiScene*              scene,
		    std::vector<Vertex>&        vertices,
		    std::vector<uint32_t>&      indices,
		    std::vector<Mesh::SubMesh>& subMeshes
		)
		{
			for (uint32_t m = 0; m < scene->mNumMeshes; ++m)
			{
				const aiMesh* mesh = scene->mMeshes[m];

				const auto vertexOffset = static_cast<int32_t>(vertices.size());
				const auto indexOffset  = static_cast<uint32_t>(indices.size());

				for (uint32_t v = 0; v < mesh->mNumVertices; ++v)
				{
					Vertex vertex;
					vertex.Position = ToVec3(mesh->mVertices[v]);
					if (mesh->HasNormals())
						vertex.Normal = ToVec3(mesh->mNormals[v]);
					if (mesh->HasTextureCoords(0))
						vertex.TexCoord = ToVec2(mesh->mTextureCoords[0][v]);
					if (mesh->HasTangentsAndBitangents())
						vertex.Tangent = glm::vec4(ToVec3(mesh->mTangents[v]), 1.0f);
					vertices.push_back(vertex);
				}

				for (uint32_t f = 0; f < mesh->mNumFaces; ++f)
				{
					const aiFace& face = mesh->mFaces[f];
					for (uint32_t i = 0; i < face.mNumIndices; ++i)
						indices.push_back(face.mIndices[i]);
				}

				subMeshes.push_back(
				    Mesh::SubMesh{
				        .IndexOffset   = indexOffset,
				        .IndexCount    = static_cast<uint32_t>(indices.size()) - indexOffset,
				        .VertexOffset  = vertexOffset,
				        .MaterialIndex = mesh->mMaterialIndex
				    }
				);
			}
		}

		Ref<Vulkan::Texture2D> LoadEmbedded(const aiTexture* texture)
		{
			using namespace Vulkan;

			// Compressed (e.g. PNG/JPEG) embedded data: mHeight == 0, mWidth == byte size.
			if (texture->mHeight == 0)
			{
				int      width = 0, height = 0, channels = 0;
				stbi_uc* pixels = stbi_load_from_memory(
				    reinterpret_cast<const stbi_uc*>(texture->pcData),
				    static_cast<int>(texture->mWidth),
				    &width,
				    &height,
				    &channels,
				    STBI_rgb_alpha
				);
				if (!pixels)
					return nullptr;

				const TextureSpecification spec{
				    .Width  = static_cast<uint32_t>(width),
				    .Height = static_cast<uint32_t>(height),
				    .Format = vk::Format::eR8G8B8A8Srgb
				};
				auto result = CreateRef<Texture2D>(spec, pixels, "EmbeddedTexture");
				stbi_image_free(pixels);
				return result;
			}

			// Uncompressed BGRA texels.
			std::vector<uint8_t> rgba(static_cast<size_t>(texture->mWidth) * texture->mHeight * 4);
			for (size_t i = 0; i < static_cast<size_t>(texture->mWidth) * texture->mHeight; ++i)
			{
				rgba[i * 4 + 0] = texture->pcData[i].r;
				rgba[i * 4 + 1] = texture->pcData[i].g;
				rgba[i * 4 + 2] = texture->pcData[i].b;
				rgba[i * 4 + 3] = texture->pcData[i].a;
			}
			const TextureSpecification spec{
			    .Width = texture->mWidth, .Height = texture->mHeight, .Format = vk::Format::eR8G8B8A8Srgb
			};
			return CreateRef<Texture2D>(spec, rgba.data(), "EmbeddedTexture");
		}

		void ExtractMaterials(
		    const aiScene* scene, const std::filesystem::path& directory, std::vector<ImportedMaterial>& outMaterials
		)
		{
			for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
			{
				const aiMaterial* material = scene->mMaterials[i];
				ImportedMaterial  imported;

				aiColor4D color(1.0f, 1.0f, 1.0f, 1.0f);
				if (material->Get(AI_MATKEY_BASE_COLOR, color) == AI_SUCCESS ||
				    material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
					imported.BaseColorFactor = {color.r, color.g, color.b, color.a};

				aiTextureType type = aiTextureType_BASE_COLOR;
				if (material->GetTextureCount(type) == 0)
					type = aiTextureType_DIFFUSE;

				if (aiString texturePath; material->GetTexture(type, 0, &texturePath) == AI_SUCCESS)
				{
					if (const aiTexture* embedded = scene->GetEmbeddedTexture(texturePath.C_Str()))
						imported.AlbedoTexture = LoadEmbedded(embedded);
					else
						imported.AlbedoTexture = Vulkan::Texture2D::LoadFromFile(directory / texturePath.C_Str());
				}

				outMaterials.push_back(std::move(imported));
			}
		}
	}        // namespace

	std::optional<ImportedModel> MeshImporter::Load(const std::filesystem::path& path)
	{
		Assimp::Importer importer;
		const aiScene*   scene = importer.ReadFile(path.string(), IMPORT_FLAGS);
		if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode)
		{
			ER_CORE_ERROR_TAG("Renderer", "Failed to import model '{}': {}", path.string(), importer.GetErrorString());
			return std::nullopt;
		}

		std::vector<Vertex>        vertices;
		std::vector<uint32_t>      indices;
		std::vector<Mesh::SubMesh> subMeshes;
		ExtractGeometry(scene, vertices, indices, subMeshes);

		if (vertices.empty() || indices.empty())
		{
			ER_CORE_ERROR_TAG("Renderer", "Model '{}' contains no geometry", path.string());
			return std::nullopt;
		}

		ImportedModel model;
		model.Geometry = CreateRef<Mesh>(vertices, indices, std::move(subMeshes));
		ExtractMaterials(scene, path.parent_path(), model.Materials);

		ER_CORE_INFO_TAG(
		    "Renderer",
		    "Imported '{}': {} vertices, {} indices, {} materials",
		    path.filename().string(),
		    vertices.size(),
		    indices.size(),
		    model.Materials.size()
		);
		return model;
	}

	Ref<Mesh> MeshImporter::LoadMesh(const std::filesystem::path& path)
	{
		std::optional<ImportedModel> model = Load(path);
		return model ? model->Geometry : nullptr;
	}
}        // namespace Eruption
