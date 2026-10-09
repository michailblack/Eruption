#include "Texture.h"

#include "Eruption/Platform/Vulkan/Buffer.h"
#include "Eruption/Platform/Vulkan/VulkanContext.h"

#include <stb_image.h>

#include <cmath>

namespace Eruption::Vulkan
{
	namespace
	{
		uint32_t BytesPerPixel(vk::Format format)
		{
			switch (format)
			{
				case vk::Format::eR8G8B8A8Srgb:
				case vk::Format::eR8G8B8A8Unorm: return 4;
				default:                         ER_CORE_ASSERT(false, "Unsupported texture format"); return 4;
			}
		}

		uint32_t CalculateMipLevels(uint32_t width, uint32_t height)
		{
			return static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
		}
	}        // namespace

	Texture2D::Texture2D(const TextureSpecification& spec, const void* pixels, std::string_view name) : m_Spec(spec)
	{
		ER_CORE_ASSERT(m_Spec.Width > 0 && m_Spec.Height > 0, "Texture dimensions must be non-zero");

		m_MipLevels = m_Spec.GenerateMips ? CalculateMipLevels(m_Spec.Width, m_Spec.Height) : 1;

		vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
		if (m_MipLevels > 1)
			usage |= vk::ImageUsageFlagBits::eTransferSrc;        // blit source for mip generation

		const vk::ImageCreateInfo imageInfo{
		    .imageType   = vk::ImageType::e2D,
		    .format      = m_Spec.Format,
		    .extent      = {m_Spec.Width, m_Spec.Height, 1},
		    .mipLevels   = m_MipLevels,
		    .arrayLayers = 1,
		    .samples     = vk::SampleCountFlagBits::e1,
		    .tiling      = vk::ImageTiling::eOptimal,
		    .usage       = usage
		};
		m_Image = VulkanContext::Get().GetAllocator().CreateImage(imageInfo, MemoryUsage::GpuOnly, name);

		const vk::raii::Device& device = VulkanContext::GetVkDevice();

		m_View = vk::raii::ImageView(
		    device,
		    vk::ImageViewCreateInfo{
		        .image            = m_Image.Image,
		        .viewType         = vk::ImageViewType::e2D,
		        .format           = m_Spec.Format,
		        .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, m_MipLevels, 0, 1}
		}
		);

		const float
		    maxAnisotropy = VulkanContext::Get().GetPhysicalDevice()->GetProperties().limits.maxSamplerAnisotropy;

		m_Sampler = vk::raii::Sampler(
		    device,
		    vk::SamplerCreateInfo{
		        .magFilter        = m_Spec.Filter,
		        .minFilter        = m_Spec.Filter,
		        .mipmapMode       = vk::SamplerMipmapMode::eLinear,
		        .addressModeU     = m_Spec.AddressMode,
		        .addressModeV     = m_Spec.AddressMode,
		        .addressModeW     = m_Spec.AddressMode,
		        .anisotropyEnable = vk::True,
		        .maxAnisotropy    = maxAnisotropy,
		        .maxLod           = static_cast<float>(m_MipLevels),
		        .borderColor      = vk::BorderColor::eFloatOpaqueWhite
		    }
		);

		if (pixels)
			Upload(pixels);
	}

	Texture2D::~Texture2D()
	{
		// TODO: Shouldn't they be destoryed also?
		m_View    = nullptr;
		m_Sampler = nullptr;
		if (m_Image)
			VulkanContext::Get().GetAllocator().DestroyImage(m_Image);
	}

	void Texture2D::Upload(const void* pixels)
	{
		const VulkanAllocator& allocator = VulkanContext::Get().GetAllocator();

		const vk::DeviceSize imageSize = static_cast<vk::DeviceSize>(m_Spec.Width) *
		                                 static_cast<vk::DeviceSize>(m_Spec.Height) * BytesPerPixel(m_Spec.Format);

		const vk::BufferCreateInfo stagingInfo{
		    .size        = imageSize,
		    .usage       = vk::BufferUsageFlagBits::eTransferSrc,
		    .sharingMode = vk::SharingMode::eExclusive
		};
		const AllocatedBuffer staging = allocator.CreateBuffer(stagingInfo, MemoryUsage::CpuToGpu, "TextureStaging");
		std::memcpy(staging.MappedData, pixels, imageSize);
		allocator.FlushAllocation(staging.Allocation);

		VulkanContext::ImmediateSubmit([&](const vk::raii::CommandBuffer& cmd) {
			// Whole image -> transfer destination.
			Utils::CmdImageBarrier(
			    cmd,
			    m_Image.Image,
			    vk::PipelineStageFlagBits2::eTopOfPipe,
			    {},
			    vk::PipelineStageFlagBits2::eCopy,
			    vk::AccessFlagBits2::eTransferWrite,
			    vk::ImageLayout::eUndefined,
			    vk::ImageLayout::eTransferDstOptimal,
			    {vk::ImageAspectFlagBits::eColor, 0, m_MipLevels, 0, 1}
			);

			const vk::BufferImageCopy region{
			    .imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
			    .imageExtent      = {m_Spec.Width, m_Spec.Height, 1}
			};
			cmd.copyBufferToImage(staging.Buffer, m_Image.Image, vk::ImageLayout::eTransferDstOptimal, region);

			if (m_MipLevels > 1)
			{
				GenerateMips(cmd);
			}
			else
			{
				Utils::CmdImageBarrier(
				    cmd,
				    m_Image.Image,
				    vk::PipelineStageFlagBits2::eCopy,
				    vk::AccessFlagBits2::eTransferWrite,
				    vk::PipelineStageFlagBits2::eFragmentShader,
				    vk::AccessFlagBits2::eShaderSampledRead,
				    vk::ImageLayout::eTransferDstOptimal,
				    vk::ImageLayout::eShaderReadOnlyOptimal,
				    {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
				);
			}
		});

		allocator.DestroyBuffer(staging);
	}

	void Texture2D::GenerateMips(const vk::raii::CommandBuffer& cmd)
	{
		int32_t mipWidth  = static_cast<int32_t>(m_Spec.Width);
		int32_t mipHeight = static_cast<int32_t>(m_Spec.Height);

		for (uint32_t level = 1; level < m_MipLevels; ++level)
		{
			// TODO: It is assumed that GenerateMips() is called inside Upload() in a certain point
			Utils::CmdImageBarrier(
			    cmd,
			    m_Image.Image,
			    vk::PipelineStageFlagBits2::eCopy,
			    vk::AccessFlagBits2::eTransferWrite,
			    vk::PipelineStageFlagBits2::eBlit,
			    vk::AccessFlagBits2::eTransferRead,
			    vk::ImageLayout::eTransferDstOptimal,
			    vk::ImageLayout::eTransferSrcOptimal,
			    {vk::ImageAspectFlagBits::eColor, level - 1, 1, 0, 1}
			);

			const int32_t nextWidth  = std::max(mipWidth / 2, 1);
			const int32_t nextHeight = std::max(mipHeight / 2, 1);

			const vk::ImageBlit2 blit{
			    .srcSubresource = {vk::ImageAspectFlagBits::eColor, level - 1, 0, 1},
			    .srcOffsets     = std::array{vk::Offset3D{0, 0, 0}, vk::Offset3D{mipWidth, mipHeight, 1}},
			    .dstSubresource = {vk::ImageAspectFlagBits::eColor, level, 0, 1},
			    .dstOffsets     = std::array{vk::Offset3D{0, 0, 0}, vk::Offset3D{nextWidth, nextHeight, 1}}
			};

			cmd.blitImage2(
			    vk::BlitImageInfo2{
			        .srcImage       = m_Image.Image,
			        .srcImageLayout = vk::ImageLayout::eTransferSrcOptimal,
			        .dstImage       = m_Image.Image,
			        .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
			        .regionCount    = 1,
			        .pRegions       = &blit,
			        .filter         = vk::Filter::eLinear
			    }
			);

			Utils::CmdImageBarrier(
			    cmd,
			    m_Image.Image,
			    vk::PipelineStageFlagBits2::eBlit,
			    vk::AccessFlagBits2::eTransferRead,
			    vk::PipelineStageFlagBits2::eFragmentShader,
			    vk::AccessFlagBits2::eShaderSampledRead,
			    vk::ImageLayout::eTransferSrcOptimal,
			    vk::ImageLayout::eShaderReadOnlyOptimal,
			    {vk::ImageAspectFlagBits::eColor, level - 1, 1, 0, 1}
			);

			mipWidth  = nextWidth;
			mipHeight = nextHeight;
		}

		Utils::CmdImageBarrier(
		    cmd,
		    m_Image.Image,
		    vk::PipelineStageFlagBits2::eCopy,
		    vk::AccessFlagBits2::eTransferWrite,
		    vk::PipelineStageFlagBits2::eFragmentShader,
		    vk::AccessFlagBits2::eShaderSampledRead,
		    vk::ImageLayout::eTransferDstOptimal,
		    vk::ImageLayout::eShaderReadOnlyOptimal,
		    {vk::ImageAspectFlagBits::eColor, m_MipLevels - 1, 1, 0, 1}
		);
	}

	Ref<Texture2D> Texture2D::LoadFromFile(const std::filesystem::path& path, bool srgb, bool generateMips)
	{
		int      width = 0, height = 0, channels = 0;
		stbi_uc* pixels = stbi_load(path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
		if (!pixels)
		{
			ER_CORE_ERROR_TAG("Renderer", "Failed to load texture '{}': {}", path.string(), stbi_failure_reason());
			return CreateFlat({1.0f, 0.0f, 1.0f, 1.0f}, "MissingTexture");        // magenta fallback
		}

		const TextureSpecification spec{
		    .Width        = static_cast<uint32_t>(width),
		    .Height       = static_cast<uint32_t>(height),
		    .Format       = srgb ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm,
		    .GenerateMips = generateMips
		};

		auto texture = CreateRef<Texture2D>(spec, pixels, path.filename().string());

		stbi_image_free(pixels);

		return texture;
	}

	Ref<Texture2D> Texture2D::CreateFlat(const glm::vec4& color, std::string_view name)
	{
		const std::array<uint8_t, 4> pixel{
		    static_cast<uint8_t>(std::clamp(color.r, 0.0f, 1.0f) * 255.0f),
		    static_cast<uint8_t>(std::clamp(color.g, 0.0f, 1.0f) * 255.0f),
		    static_cast<uint8_t>(std::clamp(color.b, 0.0f, 1.0f) * 255.0f),
		    static_cast<uint8_t>(std::clamp(color.a, 0.0f, 1.0f) * 255.0f)
		};

		const TextureSpecification spec{
		    .Width = 1, .Height = 1, .Format = vk::Format::eR8G8B8A8Unorm, .GenerateMips = false
		};

		return CreateRef<Texture2D>(spec, pixel.data(), name);
	}
}        // namespace Eruption::Vulkan
