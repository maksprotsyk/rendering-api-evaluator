#pragma once

#include "Parser.h"


namespace Engine::Utils
{
    class Vector
	{
	public:
		float x;
		float y;
		float z;

		SERIALIZABLE(
			PROPERTY(Vector, x),
			PROPERTY(Vector, y),
			PROPERTY(Vector, z)
		)
	};

}
