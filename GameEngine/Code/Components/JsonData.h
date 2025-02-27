#pragma once

#include "Utils/Parser.h"

namespace Engine::Components
{

	class JsonData
	{
	public:
		nlohmann::json data;

		SERIALIZABLE(PROPERTY(JsonData, data))
	};


}

