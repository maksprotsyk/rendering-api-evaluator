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
	class ComponentsManager;

	class ComponentsFactory
	{
	public:

		template<typename Component, typename Serializer>
		void registerComponent();

		void createComponentFromJson(ComponentsManager& manager, EntityID id, const nlohmann::json& value);
	private:
		static constexpr const char* k_typenameField = "typename";
		std::unordered_map<std::string, std::function<void(ComponentsManager&, EntityID, const nlohmann::json&)>> m_componentCreators;
	};


	class ComponentsManager
	{
	public:

		void destroyEntity(EntityID id);

		template<typename Component>
		const Utils::SparseSet<Component, EntityID>& getComponentSet() const;

		template<typename Component>
		Utils::SparseSet<Component, EntityID>& getComponentSet();

		template <typename... Components>
		std::vector<EntityID> entitiesWithComponents();

		template<typename Component>
		void createSet();

		void clear();

	private:
		std::unordered_map<std::string, std::unique_ptr<Utils::SparseSetBase<EntityID>>> m_sparseSets;
	};
}

#include "ComponentsManager.inl"