#pragma once

#include <string>

#include "Utils/Parser.h"
#include "Visual/ModelInstanceBase.h"

namespace Engine::Components
{

	class Model
	{
	public:

		std::string path;
		bool markedForDestroy = false;
		std::unique_ptr<Visual::IModelInstance> instance = nullptr;

		SERIALIZABLE(
			PROPERTY(Model, path)
			)
	};



}

