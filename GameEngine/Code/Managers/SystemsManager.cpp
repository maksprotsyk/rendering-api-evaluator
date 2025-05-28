#include "SystemsManager.h"

#include "Utils/DebugMacros.h"

namespace Engine
{

	//////////////////////////////////////////////////////////////////////////

	void SystemsManager::addSystem(std::unique_ptr<Systems::ISystem>&& system)
	{
		m_addedSystems.emplace(std::move(system));
	}

	//////////////////////////////////////////////////////////////////////////

	void SystemsManager::removeSystem(Systems::ISystem* system)
	{
		m_removedSystems.emplace(system);
	}

	//////////////////////////////////////////////////////////////////////////

	void SystemsFactory::loadSystemFromJson(SystemsManager& manager, const nlohmann::json& systemJson)
	{
		ASSERT(systemJson.contains(k_typenameField), "System must have a {} field", k_typenameField);
		if (!systemJson.contains(k_typenameField))
		{
			return;
		}
		std::string type = systemJson[k_typenameField].get<std::string>();
		auto creator = m_systemCreators.find(type);
		if (creator == m_systemCreators.end())
		{
			return;
		}
		creator->second(manager, systemJson);
	}

	//////////////////////////////////////////////////////////////////////////

	void SystemsManager::processAddedSystems()
	{
		while (!m_addedSystems.empty())
		{
			m_addedSystems.front()->onStart();
			m_systems.emplace(std::move(m_addedSystems.front()));
			m_addedSystems.pop();
		}
	}

	//////////////////////////////////////////////////////////////////////////

	void SystemsManager::processRemovedSystems()
	{
		while (!m_removedSystems.empty())
		{
			auto itr = std::find_if(m_systems.begin(), m_systems.end(), [this](const std::unique_ptr<Systems::ISystem>& s) {return s.get() == m_removedSystems.front(); });
			if (itr != m_systems.end())
			{
				(*itr)->onStop();
				m_systems.erase(itr);
			}
			m_removedSystems.pop();
		}
	}

	//////////////////////////////////////////////////////////////////////////

	void SystemsManager::update(float dt) const
	{
		for (const std::unique_ptr<Systems::ISystem>& system : m_systems)
		{
			system->onUpdate(dt);
		}
	}

	//////////////////////////////////////////////////////////////////////////

	void SystemsManager::stop() const
	{
		for (const std::unique_ptr<Systems::ISystem>& system : m_systems)
		{
			system->onStop();
		}
	}

	//////////////////////////////////////////////////////////////////////////

	void SystemsManager::clear()
	{
		ASSERT(m_addedSystems.empty(), "There are still systems to be added");
		ASSERT(m_removedSystems.empty(), "There are still systems to be removed");
		m_systems.clear();
	}

	//////////////////////////////////////////////////////////////////////////

	bool SystemsManager::LessPriority::operator()(const std::unique_ptr<Systems::ISystem>& lhs, const std::unique_ptr<Systems::ISystem>& rhs) const
	{
		if (lhs->getPriority() != rhs->getPriority())
		{
			return 	lhs->getPriority() < rhs->getPriority();
		}
		return lhs < rhs;
	}

	//////////////////////////////////////////////////////////////////////////
}