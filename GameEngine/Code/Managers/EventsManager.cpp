#include "SystemsManager.h"

namespace Engine
{

	std::unique_ptr<SystemsManager> SystemsManager::_instance = nullptr;

	SystemsManager& SystemsManager::get()
	{
		if (!_instance)
		{
			_instance = std::make_unique<SystemsManager>();
		}
		return *_instance;
	}

	void SystemsManager::addSystem(std::unique_ptr<Systems::ISystem>&& system)
	{
		_addedSystems.emplace(std::move(system));
	}

	void SystemsManager::removeSystem(Systems::ISystem* system)
	{
		_removedSystems.emplace(system);
	}

	void SystemsManager::update(float dt)
	{
		while (!_addedSystems.empty())
		{
			_addedSystems.front()->onStart();
			_systems.emplace(std::move(_addedSystems.front()));
			_addedSystems.pop();
		}

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

		for (const std::unique_ptr<Systems::ISystem>& system : _systems)
		{
			system->onUpdate(dt);
		}
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