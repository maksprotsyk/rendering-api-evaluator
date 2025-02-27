#include "ComponentsManager.h"

namespace Engine
{

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

	void ComponentsManager::destroyEntity(EntityID id)
	{
		for (auto& [name, componentsSet] : _sparseSets)
		{
			componentsSet->removeElement(id);
		}
	}

	void ComponentsManager::clear()
	{
		for (auto& [name, componentsSet] : _sparseSets)
		{
			componentsSet->clear();
		}
	}

}