#pragma once

#include <vector>
#include <set>
#include <queue>
#include <memory>

#include "Utils/SparseSet.h"
#include "Systems/ISystem.h"


namespace Engine
{
	class SystemsManager
	{
	public:
		static SystemsManager& get();
		void addSystem(std::unique_ptr<Systems::ISystem>&& system);
		void removeSystem(Systems::ISystem* system);
		void update(float dt);

	private:

		struct LessPriority
		{
			bool operator()(const std::unique_ptr<Systems::ISystem>& lhs, const std::unique_ptr<Systems::ISystem>& rhs) const;
		};

		std::set<std::unique_ptr<Systems::ISystem>, LessPriority> _systems;

		std::queue<Systems::ISystem*> _removedSystems;
		std::queue<std::unique_ptr<Systems::ISystem>> _addedSystems;

		static std::unique_ptr<SystemsManager> _instance;
	};
}

