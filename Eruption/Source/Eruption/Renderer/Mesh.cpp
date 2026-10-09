#include "Mesh.h"

#include <cstddef>

namespace Eruption
{
	Vulkan::VertexInputDescription Vertex::GetVertexInputDescription()
	{
		return {
		    .Bindings   = {vk::VertexInputBindingDescription{
		        .binding = 0, .stride = sizeof(Vertex), .inputRate = vk::VertexInputRate::eVertex
		    }},
		    .Attributes = {
		                   {.location = 0,
		         .binding  = 0,
		         .format   = vk::Format::eR32G32B32Sfloat,
		         .offset   = offsetof(Vertex, Position)},
		                   {.location = 1,
		         .binding  = 0,
		         .format   = vk::Format::eR32G32B32Sfloat,
		         .offset   = offsetof(Vertex, Normal)},
		                   {.location = 2,
		         .binding  = 0,
		         .format   = vk::Format::eR32G32Sfloat,
		         .offset   = offsetof(Vertex, TexCoord)},
		                   {.location = 3,
		         .binding  = 0,
		         .format   = vk::Format::eR32G32B32A32Sfloat,
		         .offset   = offsetof(Vertex, Tangent)}
		    }
		};
	}

	Mesh::Mesh(std::span<const Vertex> vertices, std::span<const uint32_t> indices, std::vector<SubMesh> subMeshes) :
	    m_VertexCount(static_cast<uint32_t>(vertices.size())),
	    m_IndexCount(static_cast<uint32_t>(indices.size())),
	    m_SubMeshes(std::move(subMeshes))
	{
		ER_CORE_ASSERT(!vertices.empty() && !indices.empty(), "Mesh requires vertices and indices");

		m_VertexBuffer = Vulkan::Buffer::CreateVertex(vertices.data(), vertices.size_bytes(), "Mesh.Vertices");
		m_IndexBuffer  = Vulkan::Buffer::CreateIndex(indices.data(), indices.size_bytes(), "Mesh.Indices");

		if (m_SubMeshes.empty())
			m_SubMeshes.push_back(
			    SubMesh{.IndexOffset = 0, .IndexCount = m_IndexCount, .VertexOffset = 0, .MaterialIndex = 0}
			);
	}

	void Mesh::Bind(const vk::raii::CommandBuffer& cmd) const
	{
		cmd.bindVertexBuffers(0, {m_VertexBuffer->GetHandle()}, {vk::DeviceSize{0}});
		cmd.bindIndexBuffer(m_IndexBuffer->GetHandle(), 0, vk::IndexType::eUint32);
	}

	void Mesh::Draw(const vk::raii::CommandBuffer& cmd) const
	{
		for (const SubMesh& subMesh : m_SubMeshes)
			DrawSubMesh(cmd, subMesh);
	}

	void Mesh::DrawSubMesh(const vk::raii::CommandBuffer& cmd, const SubMesh& subMesh) const
	{
		cmd.drawIndexed(subMesh.IndexCount, 1, subMesh.IndexOffset, subMesh.VertexOffset, 0);
	}

	Ref<Mesh> Mesh::CreateCube(float size)
	{
		const float h = size * 0.5f;

		// 6 faces, 4 vertices each, with per-face normals, UVs and tangents.
		struct FaceDef
		{
			glm::vec3 normal;
			glm::vec4 tangent;
			glm::vec3 corners[4];
		};

		const std::array<FaceDef, 6> faces = {
		    {
             // +X
		        {{1, 0, 0}, {0, 0, -1, 1}, {{h, -h, h}, {h, -h, -h}, {h, h, -h}, {h, h, h}}},
             // -X
		        {{-1, 0, 0}, {0, 0, 1, 1}, {{-h, -h, -h}, {-h, -h, h}, {-h, h, h}, {-h, h, -h}}},
             // +Y
		        {{0, 1, 0}, {1, 0, 0, 1}, {{-h, h, h}, {h, h, h}, {h, h, -h}, {-h, h, -h}}},
             // -Y
		        {{0, -1, 0}, {1, 0, 0, 1}, {{-h, -h, -h}, {h, -h, -h}, {h, -h, h}, {-h, -h, h}}},
             // +Z
		        {{0, 0, 1}, {1, 0, 0, 1}, {{-h, -h, h}, {h, -h, h}, {h, h, h}, {-h, h, h}}},
             // -Z
		        {{0, 0, -1}, {-1, 0, 0, 1}, {{h, -h, -h}, {-h, -h, -h}, {-h, h, -h}, {h, h, -h}}},
		     }
		};

		constexpr std::array<glm::vec2, 4> uvs = {
		    {{0, 1}, {1, 1}, {1, 0}, {0, 0}}
		};

		std::vector<Vertex>   vertices;
		std::vector<uint32_t> indices;
		vertices.reserve(24);
		indices.reserve(36);

		for (const FaceDef& face : faces)
		{
			const auto base = static_cast<uint32_t>(vertices.size());
			for (int i = 0; i < 4; ++i)
				vertices.push_back({face.corners[i], face.normal, uvs[i], face.tangent});

			for (const uint32_t offset : {0u, 1u, 2u, 2u, 3u, 0u})
				indices.push_back(base + offset);
		}

		return CreateRef<Mesh>(vertices, indices);
	}
}        // namespace Eruption
