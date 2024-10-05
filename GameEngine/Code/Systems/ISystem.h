#pragma once

namespace Engine::Systems
{
	class ISystem
	{
	public:
		virtual void onStart() = 0;
		virtual void onUpdate(float dt) = 0;
		virtual void onStop() = 0;
		virtual int getPriority() const = 0;

		virtual ~ISystem() = default;
	};
}