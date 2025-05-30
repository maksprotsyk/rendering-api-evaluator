#pragma once

#include "ISystem.h"
#include "Visual/DirectXRenderer.h"
#include "Visual/OpenGLRenderer.h"
#include "Visual/VulkanRenderer.h"

#include "Visual/Window.h"
#include "Visual/UIController.h"
#include "Components/Transform.h"
#include "Managers/EntitiesManager.h"
#include "Managers/EventsManager.h"
#include "Events/StatsEvents.h"

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
		void removeRenderer();
		void setRenderer(const std::string& rendererName);
	private:
		std::map<std::string, std::function<std::unique_ptr<Visual::IRenderer>()>> m_rendererCreators;
		std::vector<std::string> m_rendererNames;

		const Visual::Window& m_window;
		std::unique_ptr<Visual::UIController> m_uiController;

		std::string m_rendererName;
		std::string m_nextRendererName;
		std::unique_ptr<Visual::IRenderer> m_renderer;

		Utils::Vector3 m_lightDirection = Utils::Vector3(0, 0, -1);
		EntityID m_cameraId = -1;

		EventListenerID m_rendererUpdateListenerId = -1;
		
	};
}