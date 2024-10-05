#pragma once

#include "Managers/ComponentsManager.h"

namespace Engine::Components
{

	class Tag
	{
	public:
		std::string tag;

		SERIALIZABLE(PROPERTY(Tag, tag))

	};



}

