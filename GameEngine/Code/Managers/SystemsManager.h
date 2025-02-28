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

	private:
		static constexpr const char* k_typenameField = "typename";

		std::set<std::unique_ptr<Systems::ISystem>, LessPriority> m_systems;
		std::queue<Systems::ISystem*> m_removedSystems;
		std::queue<std::unique_ptr<Systems::ISystem>> m_addedSystems;

		std::unordered_map<std::string, std::function<void(const nlohmann::json&)>> m_systemCreators;
	};

}

#include "SystemsManager.inl"

