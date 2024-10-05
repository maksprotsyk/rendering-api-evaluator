#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <functional>

#include "Utils/SparseSet.h"
#include "Utils/BasicUtils.h"
#include "Utils/Parser.h"

#include "Managers/EntitiesManager.h"

namespace Engine
{
	class ComponentsManager
	{
	public:

		static ComponentsManager& get();

		void createComponentFromJson(EntityID id, const nlohmann::json& value);

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


	private:
		static std::unique_ptr<ComponentsManager> _instance;
		std::unordered_map<std::string, std::unique_ptr<Utils::SparseSetBase<EntityID>>> _sparseSets;
		std::unordered_map<std::string, std::function<void(EntityID, const nlohmann::json& val)>> _componentCreators;
	};

	template <typename Component>
	class ComponentRegisterer
	{
	public:
		explicit ComponentRegisterer();
	};

	template <typename Component, typename Serializer>
	class SerializableComponentRegisterer
	{
	public:
		explicit SerializableComponentRegisterer();
	};
}

namespace Engine
{
	template<typename Component>
	void ComponentsManager::registerComponent()
	{
		auto sparseSet = std::make_unique<Utils::SparseSet<Component, EntityID>>();
		_sparseSets[Utils::getTypeName<Component>()] = std::move(sparseSet);
	}

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

		_componentCreators[Utils::getTypeName<Component>()] = creatorMethod;
	}

	template<typename Component>
	const Utils::SparseSet<Component, EntityID>& ComponentsManager::getComponentSet() const
	{
		std::string name = Utils::getTypeName<Component>();
		return *(std::static_pointer_cast<Utils::SparseSet<Component, EntityID>>(_sparseSets.find(name)->second));
	}

	template<typename Component>
	Utils::SparseSet<Component, EntityID>& ComponentsManager::getComponentSet()
	{
		std::string name = Utils::getTypeName<Component>();
		return *((Utils::SparseSet<Component, EntityID>*)_sparseSets[name].get());
	}

	template <typename... Components>
	std::vector<EntityID> ComponentsManager::entitiesWithComponents()
	{
		std::vector<std::string> names = Utils::getTypeNames<Components...>();
		std::vector<Utils::SparseSetBase<EntityID>*> sets;
		sets.reserve(names.size());
		for (const auto& name : names)
		{
			sets.push_back(_sparseSets[name].get());
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

	template <typename Component>
	ComponentRegisterer<Component>::ComponentRegisterer()
	{
		ComponentsManager::get().registerComponent<Component>();
	}

	template <typename Component, typename Serializer>
	SerializableComponentRegisterer<Component, Serializer>::SerializableComponentRegisterer()
	{
		ComponentsManager::get().registerComponent<Component, Serializer>();
	}
}

#define REGISTER_COMPONENT(T) \
static const Engine::ComponentRegisterer<T> reg = Engine::ComponentRegisterer<T>();

#define REGISTER_SERIALIZABLE_COMPONENT(T) \
static const Engine::SerializableComponentRegisterer<T, T> reg = Engine::SerializableComponentRegisterer<T, T>();

#define REGISTER_SERIALIZABLE_COMPONENT_S(T, S) \
static const Engine::SerializableComponentRegisterer<T, S> reg = Engine::SerializableComponentRegisterer<T, S>();