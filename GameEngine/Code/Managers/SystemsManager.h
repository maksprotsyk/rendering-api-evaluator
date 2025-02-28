#pragma once

#include <vector>
#include <set>
#include <queue>
#include <memory>
#include <functional>

#include "Utils/SparseSet.h"
#include "Utils/BasicUtils.h"
#include "Systems/ISystem.h"


namespace Engine
{
	class SystemsManager
	{
	public:
		void addSystem(std::unique_ptr<Systems::ISystem>&& system);
		void removeSystem(Systems::ISystem* system);
		void loadSystemFromJson(const nlohmann::json& systemJson);
		void update(float dt) const;
		void stop() const;
		void clear();
		void processAddedSystems();
		void processRemovedSystems();

		template <class T>
		void registerSystem();

	private:

		struct LessPriority
		{
			bool operator()(const std::unique_ptr<Systems::ISystem>& lhs, const std::unique_ptr<Systems::ISystem>& rhs) const;
		};

		std::set<std::unique_ptr<Systems::ISystem>, LessPriority> _systems;

		std::queue<Systems::ISystem*> _removedSystems;
		std::queue<std::unique_ptr<Systems::ISystem>> _addedSystems;

		std::unordered_map<std::string, std::function<void(const nlohmann::json&)>> m_systemCreators;
	};


	template<class T>
	inline void SystemsManager::registerSystem()
	{
		auto creatorMethod = [this](const nlohmann::json& val)
		{
			std::unique_ptr<T> system = std::make_unique<T>();
			system->setConfig(val);
			addSystem(std::move(system));
		};

		m_systemCreators[Utils::getTypeName<T>()] = creatorMethod;
	}
}

