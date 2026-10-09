#pragma once
#include "Eruption/Platform/Vulkan/Vulkan.h"

namespace Eruption::Vulkan
{
	// A descriptor set layout built from a small list of bindings.
	class DescriptorSetLayout
	{
	public:
		class Builder
		{
		public:
			Builder& AddBinding(
			    uint32_t binding, vk::DescriptorType type, vk::ShaderStageFlags stages, uint32_t count = 1
			);
			[[nodiscard]] Ref<DescriptorSetLayout> Build() const;

		private:
			std::vector<vk::DescriptorSetLayoutBinding> m_Bindings;
		};

		explicit DescriptorSetLayout(const std::vector<vk::DescriptorSetLayoutBinding>& bindings);

		[[nodiscard]] vk::DescriptorSetLayout GetHandle() const { return *m_Layout; }

	private:
		vk::raii::DescriptorSetLayout m_Layout = nullptr;
	};

	// A growing descriptor pool allocator. Allocation never fails: when a pool is exhausted a new,
	// larger one is created. Sets stay valid until Reset()/destruction (pools own them).
	class DescriptorAllocator
	{
	public:
		explicit DescriptorAllocator(uint32_t initialSetsPerPool = 512);

		[[nodiscard]] vk::DescriptorSet Allocate(vk::DescriptorSetLayout layout);
		void                            Reset();

	private:
		vk::raii::DescriptorPool CreatePool(uint32_t setCount);

	private:
		uint32_t m_SetsPerPool;

		std::vector<vk::raii::DescriptorPool> m_FullPools;
		std::vector<vk::raii::DescriptorPool> m_ReadyPools;
	};
}        // namespace Eruption::Vulkan
