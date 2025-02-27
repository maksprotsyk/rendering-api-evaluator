#pragma once

#include "ISystem.h"
#include "Visual/DirectXRenderer.h"
#include "Visual/OpenGLRenderer.h"
#include "Visual/VulkanRenderer.h"

#include "Visual/Window.h"
#include "Components/Transform.h"
#include "Managers/EntitiesManager.h"

namespace Engine::Systems
{
	class RenderingSystem: public ISystem
	{
	public:
		explicit RenderingSystem(const Visual::Window& window);
		void onStart() override;
		void onUpdate(float dt) override;
		void onStop() override;
		int getPriority() const override;

	private:
		const Visual::Window& _window;
		std::unique_ptr<Visual::IRenderer> _renderer;

		EntityID _cameraId = -1;
	};
}