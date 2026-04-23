#pragma once
#include "Eruption/Core/Layer.h"

#include <vector>

namespace Eruption
{
	class LayerStack
	{
	public:
		template <typename TLayer>
		    requires(std::is_base_of_v<Layer, TLayer>)
		void PushLayer()
		{
			m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, std::make_unique<TLayer>());
			m_LayerInsertIndex++;
		}

		template <typename TLayer>
		    requires(std::is_base_of_v<Layer, TLayer>)
		void PushOverlay()
		{
			m_Layers.emplace_back(std::make_unique<Layer>());
		}

		std::vector<Scope<Layer>>::iterator         begin() { return m_Layers.begin(); }
		std::vector<Scope<Layer>>::iterator         end() { return m_Layers.end(); }
		std::vector<Scope<Layer>>::reverse_iterator rbegin() { return m_Layers.rbegin(); }
		std::vector<Scope<Layer>>::reverse_iterator rend() { return m_Layers.rend(); }

		std::vector<Scope<Layer>>::const_iterator         begin() const { return m_Layers.begin(); }
		std::vector<Scope<Layer>>::const_iterator         end() const { return m_Layers.end(); }
		std::vector<Scope<Layer>>::const_reverse_iterator rbegin() const { return m_Layers.rbegin(); }
		std::vector<Scope<Layer>>::const_reverse_iterator rend() const { return m_Layers.rend(); }

	private:
		std::vector<Scope<Layer>> m_Layers;
		unsigned int              m_LayerInsertIndex = 0;
	};
}        // namespace Eruption
