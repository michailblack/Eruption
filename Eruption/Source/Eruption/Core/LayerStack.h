#pragma once
#include "Eruption/Core/Layer.h"

#include <vector>

namespace Eruption
{
	class LayerStack
	{
	public:
		~LayerStack()
		{
			for (auto& layer : m_Layers)
				layer->OnDetach();
		}

		template <typename TLayer, typename... Args>
		    requires std::is_base_of_v<Layer, TLayer>
		TLayer& PushLayer(Args&&... args)
		{
			auto  layer = CreateScope<TLayer>(std::forward<Args>(args)...);
			auto& ref   = *layer;
			m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, std::move(layer));
			++m_LayerInsertIndex;
			ref.OnAttach();
			return ref;
		}

		template <typename TLayer, typename... Args>
		    requires std::is_base_of_v<Layer, TLayer>
		TLayer& PushOverlay(Args&&... args)
		{
			auto  layer = CreateScope<TLayer>(std::forward<Args>(args)...);
			auto& ref   = *layer;
			m_Layers.emplace_back(std::move(layer));
			ref.OnAttach();
			return ref;
		}

		void Clear()
		{
			for (auto& layer : m_Layers)
				layer->OnDetach();
			m_Layers.clear();
			m_LayerInsertIndex = 0;
		}

		[[nodiscard]] std::vector<Scope<Layer>>::iterator         begin() { return m_Layers.begin(); }
		[[nodiscard]] std::vector<Scope<Layer>>::iterator         end() { return m_Layers.end(); }
		[[nodiscard]] std::vector<Scope<Layer>>::reverse_iterator rbegin() { return m_Layers.rbegin(); }
		[[nodiscard]] std::vector<Scope<Layer>>::reverse_iterator rend() { return m_Layers.rend(); }

		[[nodiscard]] std::vector<Scope<Layer>>::const_iterator         begin() const { return m_Layers.begin(); }
		[[nodiscard]] std::vector<Scope<Layer>>::const_iterator         end() const { return m_Layers.end(); }
		[[nodiscard]] std::vector<Scope<Layer>>::const_reverse_iterator rbegin() const { return m_Layers.rbegin(); }
		[[nodiscard]] std::vector<Scope<Layer>>::const_reverse_iterator rend() const { return m_Layers.rend(); }

	private:
		std::vector<Scope<Layer>> m_Layers;
		size_t                    m_LayerInsertIndex = 0;
	};
}        // namespace Eruption
