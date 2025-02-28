#pragma once

#include "SystemsManager.h"

namespace Engine
{
	//////////////////////////////////////////////////////////////////////////

	template<class T>
	void SystemsManager::registerSystem()
	{
		auto creatorMethod = [this](const nlohmann::json& val)
		{
			std::unique_ptr<T> system = std::make_unique<T>();
			system->setConfig(val);
			addSystem(std::move(system));
		};

		m_systemCreators[Utils::getTypeName<T>()] = creatorMethod;
	}

	//////////////////////////////////////////////////////////////////////////
}

