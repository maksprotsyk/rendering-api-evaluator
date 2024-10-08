#pragma once

#include "Managers/ComponentsManager.h"
#include "nlohmann/json.hpp"

namespace Engine::Components
{

	class JsonData
	{
	public:
		nlohmann::json data;

		SERIALIZABLE(PROPERTY(JsonData, data))
	};


}

