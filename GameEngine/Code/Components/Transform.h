#pragma once

#include "Managers/ComponentsManager.h"
#include "Utils/Vector.h"

namespace Engine::Components
{

	class Transform
	{
	public:
		Utils::Vector3 position;
		Utils::Vector3 rotation;
		Utils::Vector3 scale = Utils::Vector3(1.0f, 1.0f, 1.0f);

		SERIALIZABLE(
			PROPERTY(Transform, position),
			PROPERTY(Transform, rotation),
			PROPERTY(Transform, scale)
		)
	};



}

