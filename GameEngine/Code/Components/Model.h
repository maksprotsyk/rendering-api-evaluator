#pragma once

#include <string>

#include "Managers/ComponentsManager.h"
#include "Visual/DirectXRenderer.h"

namespace Engine::Components
{

	class Model
	{
	public:
		Visual::DirectXRenderer::Model model;
		std::string path;

		SERIALIZABLE(
			PROPERTY(Model, path)
			)
	};



}

