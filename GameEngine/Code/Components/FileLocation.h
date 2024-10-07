#pragma once

#include "Managers/ComponentsManager.h"

namespace Engine::Components
{

	class FileLocation
	{
	public:
		std::string path;

		SERIALIZABLE(PROPERTY(FileLocation, path))

	};



}

