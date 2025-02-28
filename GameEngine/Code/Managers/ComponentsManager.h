#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <functional>

#include "Utils/SparseSet.h"
#include "Utils/BasicUtils.h"
#include "Utils/Parser.h"

#include "EntitiesManager.h"

namespace Engine
{
	class ComponentsManager
	{
	public:

		void createComponentFromJson(EntityID id, const nlohmann::json& value);

		void destroyEntity(EntityID id);

		template<typename Component>
		void registerComponent();

		template<typename Component, typename Serializer>
		void registerComponent();

		template<typename Component>
		const Utils::SparseSet<Component, EntityID>& getComponentSet() const;

		template<typename Component>
		Utils::SparseSet<Component, EntityID>& getComponentSet();

		template <typename... Components>
		std::vector<EntityID> entitiesWithComponents();

		void clear();

	private:
		static constexpr const char* k_typenameField = "typename";

		std::unordered_map<std::string, std::unique_ptr<Utils::SparseSetBase<EntityID>>> m_sparseSets;
		std::unordered_map<std::string, std::function<void(EntityID, const nlohmann::json&)>> m_componentCreators;
	};
}

#include "ComponentsManager.inl"