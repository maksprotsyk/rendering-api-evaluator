#pragma once

#include <Windows.h>

namespace Engine::Events
{

	struct NativeKeyStateChanged
	{
		WPARAM key;
		bool pressed;
	};
	
	struct NativeExitRequested
	{

	};
}

