#include "ComponentsManager.h"

namespace Engine
{
	std::unique_ptr<ComponentsManager> ComponentsManager::_instance = nullptr;

	ComponentsManager& ComponentsManager::get()
	{
		if (!_instance)
		{
			_instance = std::make_unique<ComponentsManager>();
		}
		return *_instance;
	}

	void ComponentsManager::createComponentFromJson(EntityID id, const nlohmann::json& value)
	{
		if (!value.contains("typename"))
		{
			return;
		}

		std::string type = value["typename"].get<std::string>();
		auto creator = _componentCreators.find(type);
		if (creator == _componentCreators.end())
		{
			return;
		}

		creator->second(id, value);
	}

}