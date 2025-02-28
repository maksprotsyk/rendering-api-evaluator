#pragma once

#include "ComponentsManager.h"

namespace Engine
{
	//////////////////////////////////////////////////////////////////////////

	template<typename Component>
	void ComponentsManager::registerComponent()
	{
		auto sparseSet = std::make_unique<Utils::SparseSet<Component, EntityID>>();
		m_sparseSets[Utils::getTypeName<Component>()] = std::move(sparseSet);
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename Component, typename Serializer>
	void ComponentsManager::registerComponent()
	{
		registerComponent<Component>();
		auto creatorMethod = [this](EntityID id, const nlohmann::json& val)
			{
				Serializer serializer{};
				Utils::Parser::fillFromJson(serializer, val);
				Utils::SparseSet<Component, EntityID>& compSet = getComponentSet<Component>();

				if constexpr (std::is_same<Component, Serializer>::value)
				{
					if constexpr (std::is_move_constructible<Component>::value)
					{
						compSet.addElement(id, std::move(serializer));
					}
					else
					{
						compSet.addElement(id, serializer);
					}
				}
				else
				{
					Component comp{};
					serializer.fill(comp);

					if constexpr (std::is_move_constructible<Component>::value)
					{
						compSet.addElement(id, std::move(comp));
					}
					else
					{
						compSet.addElement(id, comp);
					}
				}
			};

		m_componentCreators[Utils::getTypeName<Component>()] = creatorMethod;
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename Component>
	const Utils::SparseSet<Component, EntityID>& ComponentsManager::getComponentSet() const
	{
		std::string name = Utils::getTypeName<Component>();
		return *(std::static_pointer_cast<Utils::SparseSet<Component, EntityID>>(m_sparseSets.find(name)->second));
	}

	//////////////////////////////////////////////////////////////////////////

	template<typename Component>
	Utils::SparseSet<Component, EntityID>& ComponentsManager::getComponentSet()
	{
		std::string name = Utils::getTypeName<Component>();
		return *((Utils::SparseSet<Component, EntityID>*)m_sparseSets[name].get());
	}

	//////////////////////////////////////////////////////////////////////////

	template <typename... Components>
	std::vector<EntityID> ComponentsManager::entitiesWithComponents()
	{
		std::vector<std::string> names = Utils::getTypeNames<Components...>();
		std::vector<Utils::SparseSetBase<EntityID>*> sets;
		sets.reserve(names.size());
		for (const auto& name : names)
		{
			sets.push_back(m_sparseSets[name].get());
		}
		std::sort(
			sets.begin(), sets.end(),
			[](const auto& set1, const auto& set2)
			{
				return set1->size() < set2->size();
			}
		);
		std::vector<EntityID> result;
		for (const EntityID& id : sets[0]->getIds())
		{
			bool isPresent = true;
			for (int setIdx = 1; setIdx < sets.size(); setIdx++)
			{
				if (!sets[setIdx]->isPresent(id))
				{
					isPresent = false;
					break;
				}
			}
			if (isPresent)
			{
				result.push_back(id);
			}
		}

		return result;
	}

	//////////////////////////////////////////////////////////////////////////
}