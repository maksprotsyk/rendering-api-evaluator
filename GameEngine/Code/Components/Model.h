#pragma once

#include <string>

#include "Managers/ComponentsManager.h"
#include "Visual/IRenderer.h"

namespace Engine::Components
{

	class Model
	{
	public:
		std::unique_ptr<Visual::IRenderer::AbstractModel> model = nullptr;

		std::string path;

		SERIALIZABLE(
			PROPERTY(Model, path)
			)
	};



}

