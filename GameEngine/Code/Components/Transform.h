#pragma once

#include "Managers/ComponentsManager.h"
#include "Utils/Vector.h"

namespace Engine::Components
{

	class Transform
	{
	public:
		Utils::Vector position;

		SERIALIZABLE(
			PROPERTY(Transform, position)
		)
	};



}

