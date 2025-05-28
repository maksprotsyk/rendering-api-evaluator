#pragma once

#include "SystemsManager.h"

namespace Engine
{
	//////////////////////////////////////////////////////////////////////////

	template<typename System>
	void SystemsFactory::registerSystem()
	{
		auto creatorMethod = [this](SystemsManager& manager, const nlohmann::json& val)
		{
			std::unique_ptr<System> system = std::make_unique<System>();
			system->setConfig(val);
			manager.addSystem(std::move(system));
		};

		m_systemCreators[Utils::getTypeName<System>()] = creatorMethod;
	}

	//////////////////////////////////////////////////////////////////////////
}

