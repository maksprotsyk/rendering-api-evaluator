#include "ComponentsManager.h"

#include "Utils/DebugMacros.h"

namespace Engine
{
	//////////////////////////////////////////////////////////////////////////

	void ComponentsFactory::createComponentFromJson(ComponentsManager& manager, EntityID id, const nlohmann::json& value)
	{
		ASSERT(value.contains(k_typenameField), "Component must have a {} field", k_typenameField);
		if (!value.contains(k_typenameField))
		{
			return;
		}

		std::string type = value[k_typenameField].get<std::string>();
		auto creator = m_componentCreators.find(type);
		if (creator == m_componentCreators.end())
		{
			return;
		}

		creator->second(manager, id, value);
	}

	//////////////////////////////////////////////////////////////////////////

	void ComponentsManager::destroyEntity(EntityID id)
	{
		for (auto& [name, componentsSet] : m_sparseSets)
		{
			componentsSet->removeElement(id);
		}
	}

	//////////////////////////////////////////////////////////////////////////

	void ComponentsManager::clear()
	{
		for (auto& [name, componentsSet] : m_sparseSets)
		{
			componentsSet->clear();
		}
	}
	
	//////////////////////////////////////////////////////////////////////////
}