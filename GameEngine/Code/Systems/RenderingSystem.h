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
		explicit RenderingSystem();
		void onStart() override;
		void onUpdate(float dt) override;
		void onStop() override;
		int getPriority() const override;
	private:
		const Visual::Window& m_window;
		std::unique_ptr<Visual::IRenderer> m_renderer;

		EntityID m_cameraId = -1;
	};
}