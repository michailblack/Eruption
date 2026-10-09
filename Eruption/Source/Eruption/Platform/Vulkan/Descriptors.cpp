#include "Descriptors.h"

#include "Eruption/Platform/Vulkan/VulkanContext.h"

namespace Eruption::Vulkan
{
	/* ------------------------------------------- DescriptorSetLayout ------------------------------------------- */

	DescriptorSetLayout::Builder& DescriptorSetLayout::Builder::AddBinding(
	    uint32_t binding, vk::DescriptorType type, vk::ShaderStageFlags stages, uint32_t count
	)
	{
		m_Bindings.push_back(
		    vk::DescriptorSetLayoutBinding{
		        .binding = binding, .descriptorType = type, .descriptorCount = count, .stageFlags = stages
		    }
		);
		return *this;
	}

	Ref<DescriptorSetLayout> DescriptorSetLayout::Builder::Build() const
	{
		return CreateRef<DescriptorSetLayout>(m_Bindings);
	}

	DescriptorSetLayout::DescriptorSetLayout(const std::vector<vk::DescriptorSetLayoutBinding>& bindings)
	{
		m_Layout = vk::raii::DescriptorSetLayout(
		    VulkanContext::GetVkDevice(),
		    vk::DescriptorSetLayoutCreateInfo{
		        .bindingCount = static_cast<uint32_t>(bindings.size()), .pBindings = bindings.data()
		    }
		);
	}

	/* ------------------------------------------- DescriptorAllocator ------------------------------------------ */

	namespace
	{
		// Reasonable mix for a general-purpose renderer; scaled by the pool's set count.
		constexpr std::array POOL_RATIOS = {
		    std::pair{       vk::DescriptorType::eUniformBuffer, 2.0f},
		    std::pair{       vk::DescriptorType::eStorageBuffer, 1.0f},
		    std::pair{vk::DescriptorType::eCombinedImageSampler, 2.0f},
		    std::pair{        vk::DescriptorType::eSampledImage, 2.0f},
		    std::pair{             vk::DescriptorType::eSampler, 1.0f},
		    std::pair{        vk::DescriptorType::eStorageImage, 1.0f},
		};
	}        // namespace

	DescriptorAllocator::DescriptorAllocator(uint32_t initialSetsPerPool) : m_SetsPerPool(initialSetsPerPool)
	{}

	vk::raii::DescriptorPool DescriptorAllocator::CreatePool(uint32_t setCount)
	{
		std::vector<vk::DescriptorPoolSize> sizes;
		sizes.reserve(POOL_RATIOS.size());
		for (const auto& [type, ratio] : POOL_RATIOS)
			sizes.push_back({type, static_cast<uint32_t>(ratio * static_cast<float>(setCount))});

		return vk::raii::DescriptorPool(
		    VulkanContext::GetVkDevice(),
		    vk::DescriptorPoolCreateInfo{
		        .maxSets = setCount, .poolSizeCount = static_cast<uint32_t>(sizes.size()), .pPoolSizes = sizes.data()
		    }
		);
	}

	vk::DescriptorSet DescriptorAllocator::Allocate(vk::DescriptorSetLayout layout)
	{
		if (m_ReadyPools.empty())
			m_ReadyPools.push_back(CreatePool(m_SetsPerPool));

		const auto tryAllocate = [&]() -> vk::DescriptorSet {
			const vk::DescriptorSetAllocateInfo allocInfo{
			    .descriptorPool = *m_ReadyPools.back(), .descriptorSetCount = 1, .pSetLayouts = &layout
			};
			vk::raii::DescriptorSets sets(VulkanContext::GetVkDevice(), allocInfo);
			return sets.front().release();        // hand out a non-owning handle; the pool owns it
		};

		try
		{
			return tryAllocate();
		}
		catch (const vk::OutOfPoolMemoryError&)
		{}
		catch (const vk::FragmentedPoolError&)
		{}

		// Current pool is exhausted: retire it and grow.
		m_FullPools.push_back(std::move(m_ReadyPools.back()));
		m_ReadyPools.pop_back();
		m_SetsPerPool = std::min(m_SetsPerPool * 2, 4096u);
		m_ReadyPools.push_back(CreatePool(m_SetsPerPool));

		return tryAllocate();
	}

	void DescriptorAllocator::Reset()
	{
		for (vk::raii::DescriptorPool& pool : m_ReadyPools)
			pool.reset();
		for (vk::raii::DescriptorPool& pool : m_FullPools)
		{
			pool.reset();
			m_ReadyPools.push_back(std::move(pool));
		}
		m_FullPools.clear();
	}
}        // namespace Eruption::Vulkan
