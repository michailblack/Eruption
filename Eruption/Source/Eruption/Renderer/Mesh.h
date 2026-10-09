#pragma once
#include "Eruption/Platform/Vulkan/Buffer.h"
#include "Eruption/Platform/Vulkan/Pipeline.h"

#include <glm/glm.hpp>

#include <span>

namespace Eruption
{
	struct Vertex
	{
		glm::vec3 Position{0.0f};
		glm::vec3 Normal{0.0f, 0.0f, 1.0f};
		glm::vec2 TexCoord{0.0f};
		glm::vec4 Tangent{1.0f, 0.0f, 0.0f, 1.0f};

		bool operator==(const Vertex&) const = default;

		// Layout matching Mesh.slang's VSInput (locations 0..3).
		[[nodiscard]] static Vulkan::VertexInputDescription GetVertexInputDescription();
	};

	// A renderable mesh: one device-local vertex + index buffer, optionally split into submeshes that
	// reference different materials.
	class Mesh
	{
	public:
		struct SubMesh
		{
			uint32_t IndexOffset   = 0;
			uint32_t IndexCount    = 0;
			int32_t  VertexOffset  = 0;
			uint32_t MaterialIndex = 0;
		};

		Mesh(std::span<const Vertex> vertices, std::span<const uint32_t> indices, std::vector<SubMesh> subMeshes = {});

		void Bind(const vk::raii::CommandBuffer& cmd) const;
		void Draw(const vk::raii::CommandBuffer& cmd) const;
		void DrawSubMesh(const vk::raii::CommandBuffer& cmd, const SubMesh& subMesh) const;

		[[nodiscard]] const std::vector<SubMesh>& GetSubMeshes() const { return m_SubMeshes; }
		[[nodiscard]] uint32_t                    GetVertexCount() const { return m_VertexCount; }
		[[nodiscard]] uint32_t                    GetIndexCount() const { return m_IndexCount; }

		[[nodiscard]] static Ref<Mesh> CreateCube(float size = 1.0f);

	private:
		Ref<Vulkan::Buffer> m_VertexBuffer;
		Ref<Vulkan::Buffer> m_IndexBuffer;

		uint32_t             m_VertexCount = 0;
		uint32_t             m_IndexCount  = 0;
		std::vector<SubMesh> m_SubMeshes;
	};
}        // namespace Eruption
