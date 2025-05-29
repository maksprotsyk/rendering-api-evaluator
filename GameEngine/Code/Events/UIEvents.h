#pragma once

#include <string>

namespace Engine::Events
{

	struct RendererUpdate
	{
		std::string rendererName;
	};

	struct ConfigFileUpdate
	{
		std::string configPath;
	};

	struct SendWarning
	{
		std::string message;
	};
}

