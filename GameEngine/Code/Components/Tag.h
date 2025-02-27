#pragma once

#include "Utils/Parser.h"

namespace Engine::Components
{

	class Tag
	{
	public:
		std::string tag;

		SERIALIZABLE(PROPERTY(Tag, tag))

	};



}

