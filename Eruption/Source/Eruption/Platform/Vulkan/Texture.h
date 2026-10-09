#pragma once
#include "Eruption/Platform/Vulkan/VulkanAllocator.h"

#include <glm/glm.hpp>

#include <filesystem>

namespace Eruption::Vulkan
{
	struct TextureSpecification
	{
		uint32_t               Width        = 1;
		uint32_t               Height       = 1;
		vk::Format             Format       = vk::Format::eR8G8B8A8Srgb;
		bool                   GenerateMips = true;
		vk::Filter             Filter       = vk::Filter::eLinear;
		vk::SamplerAddressMode AddressMode  = vk::SamplerAddressMode::eRepeat;
	};

	// TODO: Maybe get rid of the distinction and make the dimensions of the texture to be a part of specification
	class Texture2D
	{
	public:
		Texture2D(const TextureSpecification& spec, const void* pixels = nullptr, std::string_view name = {});
		~Texture2D();

		Texture2D(const Texture2D&)            = delete;
		Texture2D& operator=(const Texture2D&) = delete;

		[[nodiscard]] vk::ImageView GetView() const { return *m_View; }
		[[nodiscard]] vk::Sampler   GetSampler() const { return *m_Sampler; }
		[[nodiscard]] uint32_t      GetWidth() const { return m_Spec.Width; }
		[[nodiscard]] uint32_t      GetHeight() const { return m_Spec.Height; }

		[[nodiscard]] vk::DescriptorImageInfo GetDescriptorInfo() const
		{
			return {*m_Sampler, *m_View, vk::ImageLayout::eShaderReadOnlyOptimal};
		}

		[[nodiscard]] static Ref<Texture2D> LoadFromFile(
		    const std::filesystem::path& path, bool srgb = true, bool generateMips = true
		);

		[[nodiscard]] static Ref<Texture2D> CreateFlat(const glm::vec4& color, std::string_view name = "FlatTexture");

	private:
		void Upload(const void* pixels);
		void GenerateMips(const vk::raii::CommandBuffer& cmd);

	private:
		TextureSpecification m_Spec;
		uint32_t             m_MipLevels = 1;

		AllocatedImage      m_Image;
		vk::raii::ImageView m_View    = nullptr;
		vk::raii::Sampler   m_Sampler = nullptr;
	};
}        // namespace Eruption::Vulkan
