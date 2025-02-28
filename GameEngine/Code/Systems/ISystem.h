#pragma once

#include "nlohmann/json.hpp"

namespace Engine::Systems
{
	class ISystem
	{
	public:
		void setConfig(const nlohmann::json& config);

		virtual void onStart() = 0;
		virtual void onUpdate(float dt) = 0;
		virtual void onStop() = 0;
		virtual int getPriority() const = 0;

		virtual ~ISystem() = default;
	protected:
		nlohmann::json m_config;
	};
}