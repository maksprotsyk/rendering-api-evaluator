#pragma once

#include <set>
#include <memory>

namespace Engine
{
	using EntityID = int;

	class EntitiesManager
	{
	public:
		EntityID createEntity();
		void destroyEntity(EntityID id);
		void clear();

	private:
		std::set<EntityID> m_takenIds;
	};
		
}

