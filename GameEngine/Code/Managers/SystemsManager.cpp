#include "SystemsManager.h"

#include "Utils/DebugMacros.h"

namespace Engine
{

	void SystemsManager::addSystem(std::unique_ptr<Systems::ISystem>&& system)
	{
		_addedSystems.emplace(std::move(system));
	}

	void SystemsManager::removeSystem(Systems::ISystem* system)
	{
		_removedSystems.emplace(system);
	}

	void SystemsManager::loadSystemFromJson(const nlohmann::json& systemJson)
	{
		ASSERT(systemJson.contains("typename"), "System must have a typename field");
		if (!systemJson.contains("typename"))
		{
			return;
		}
		std::string type = systemJson["typename"].get<std::string>();
		auto creator = m_systemCreators.find(type);
		if (creator == m_systemCreators.end())
		{
			return;
		}
		creator->second(systemJson);
	}

	void SystemsManager::processAddedSystems()
	{
		while (!_addedSystems.empty())
		{
			_addedSystems.front()->onStart();
			_systems.emplace(std::move(_addedSystems.front()));
			_addedSystems.pop();
		}
	}

	void SystemsManager::processRemovedSystems()
	{
		while (!_removedSystems.empty())
		{
			auto itr = std::find_if(_systems.begin(), _systems.end(), [this](const std::unique_ptr<Systems::ISystem>& s) {return s.get() == _removedSystems.front(); });
			if (itr != _systems.end())
			{
				(*itr)->onStop();
				_systems.erase(itr);
			}
			_removedSystems.pop();
		}
	}

	void SystemsManager::update(float dt) const
	{
		for (const std::unique_ptr<Systems::ISystem>& system : _systems)
		{
			system->onUpdate(dt);
		}
	}

	void SystemsManager::stop() const
	{
		for (const std::unique_ptr<Systems::ISystem>& system : _systems)
		{
			system->onStop();
		}
	}

	void SystemsManager::clear()
	{
		ASSERT(_addedSystems.empty(), "There are still systems to be added");
		ASSERT(_removedSystems.empty(), "There are still systems to be removed");
		_systems.clear();
	}

	bool SystemsManager::LessPriority::operator()(const std::unique_ptr<Systems::ISystem>& lhs, const std::unique_ptr<Systems::ISystem>& rhs) const
	{
		if (lhs->getPriority() != rhs->getPriority())
		{
			return 	lhs->getPriority() < rhs->getPriority();
		}
		return lhs < rhs;
	}
}