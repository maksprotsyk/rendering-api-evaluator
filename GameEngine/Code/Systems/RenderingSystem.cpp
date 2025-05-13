#include "RenderingSystem.h"

#include "imgui.h"
#include "backends/imgui_impl_win32.h"

#include "Components/Transform.h"
#include "Components/Tag.h"
#include "Components/Model.h"
#include "Events/UIEvents.h"
#include "Utils/BasicUtils.h"
#include "Utils/DebugMacros.h"
#include "Managers/GameController.h"

REGISTER_SYSTEM(Engine::Systems::RenderingSystem);

namespace Engine::Systems
{
	//////////////////////////////////////////////////////////////////////////

	RenderingSystem::RenderingSystem() : m_window(GameController::get().getWindow())
	{
		m_rendererCreators["DirectX"] = []() { return std::make_unique<Visual::DirectXRenderer>(); };
		m_rendererCreators["Vulkan"] = []() { return std::make_unique<Visual::VulkanRenderer>(); };
		m_rendererCreators["OpenGL"] = []() { return std::make_unique<Visual::OpenGLRenderer>(); };

		m_rendererNames.reserve(m_rendererCreators.size());
		for (const auto& pair : m_rendererCreators)
		{
			m_rendererNames.push_back(pair.first);
		}

		m_uiController = std::make_unique<Visual::UIController>(m_rendererNames);

		EventsManager& eventsManager = GameController::get().getEventsManager();
		eventsManager.subscribe<Events::RendererUpdate>([this](const Events::RendererUpdate& i_event) {m_nextRendererName = i_event.rendererName; });
	}

	//////////////////////////////////////////////////////////////////////////

	void RenderingSystem::onStart()
	{

#ifdef _SHOWUI
		m_uiController->init();
#endif
		if (m_config.contains("renderer"))
		{
			setRenderer(m_config["renderer"]);
		}
		else
		{
			setRenderer("");
		}

		m_nextRendererName = m_rendererName;

		auto& gameController = GameController::get();
		auto& compManager = gameController.getComponentsManager();
		auto& tagSet = compManager.getComponentSet<Components::Tag>();

		for (EntityID id : compManager.entitiesWithComponents<Components::Tag>())
		{
			if (tagSet.getElement(id).tag == "MainCamera")
			{
				m_cameraId = id;
			}
		}

	}

	//////////////////////////////////////////////////////////////////////////

	void RenderingSystem::onUpdate(float dt)
	{
		auto& gameController = GameController::get();
		auto& compManager = gameController.getComponentsManager();
		const auto& cameraTransform = compManager.getComponentSet<Components::Transform>().getElement(m_cameraId);
		m_renderer->setCameraProperties(cameraTransform.position, cameraTransform.rotation);

		auto& modelSet = compManager.getComponentSet<Components::Model>();
		const auto& transformSet = compManager.getComponentSet<Components::Transform>();

		m_renderer->clearBackground(0.0f, 0.2f, 0.4f, 1.0f);
		for (EntityID id : compManager.entitiesWithComponents<Components::Model, Components::Transform>())
		{

			Components::Model& model = modelSet.getElement(id);
			if (model.markedForDestroy)
			{
				m_renderer->destroyModelInstance(*model.instance);
				modelSet.removeElement(id);
				continue;
			}
			
			if (!model.instance)
			{
				bool loadResult = m_renderer->loadModel(gameController.getConfigRelativePath(model.path));
				ASSERT(loadResult, "Failed to load model: {}", gameController.getConfigRelativePath(model.path));
				if (!loadResult)
				{
					continue;
				}
				model.instance = m_renderer->createModelInstance(gameController.getConfigRelativePath(model.path));
			}

			const Components::Transform& transform = transformSet.getElement(id);
			m_renderer->draw(*model.instance, transform.position, transform.rotation, transform.scale);
		}

#ifdef _SHOWUI
		m_renderer->startUIRender();
		m_uiController->render();
		m_renderer->endUIRender();
#endif

		m_renderer->render();

		if (m_nextRendererName != m_rendererName)
		{
			removeRenderer();
			setRenderer(m_nextRendererName);
		}
	}

	//////////////////////////////////////////////////////////////////////////

	void RenderingSystem::onStop()
	{
		removeRenderer();
#ifdef _SHOWUI
		m_uiController->cleanUp();
#endif
	}

	//////////////////////////////////////////////////////////////////////////

	int RenderingSystem::getPriority() const
	{
		return 10;
	}

	//////////////////////////////////////////////////////////////////////////

	void RenderingSystem::removeRenderer()
	{
		auto& compManager = GameController::get().getComponentsManager();

		auto& modelSet = compManager.getComponentSet<Components::Model>();

		for (EntityID id : compManager.entitiesWithComponents<Components::Model, Components::Transform>())
		{
			Components::Model& model = modelSet.getElement(id);
			if (model.instance)
			{
				m_renderer->destroyModelInstance(*model.instance);
				model.instance = nullptr;
			}
		}

		m_renderer->cleanUp();
		m_renderer = nullptr;
	}

	//////////////////////////////////////////////////////////////////////////

	void RenderingSystem::setRenderer(const std::string& rendererName)
	{
		const auto& itr = m_rendererCreators.find(rendererName);
		if (itr != m_rendererCreators.end())
		{
			m_renderer = itr->second();
			m_rendererName = rendererName;
		}

		if (!m_renderer)
		{
			const auto& itr = m_rendererCreators.begin();
			m_renderer = itr->second();
			m_rendererName = itr->first;
		}

		m_renderer->init(m_window);

		auto& gameController = GameController::get();
		auto& compManager = gameController.getComponentsManager();
		auto& modelSet = compManager.getComponentSet<Components::Model>();

		for (EntityID id : compManager.entitiesWithComponents<Components::Model, Components::Transform>())
		{
			Components::Model& model = modelSet.getElement(id);

			bool loadResult = m_renderer->loadModel(gameController.getConfigRelativePath(model.path));
			ASSERT(loadResult, "Failed to load model: {}", gameController.getConfigRelativePath(model.path));
			if (!loadResult)
			{
				continue;
			}

			model.instance = m_renderer->createModelInstance(gameController.getConfigRelativePath(model.path));
		}

		m_uiController->setRenderer(m_rendererName);
	}

	//////////////////////////////////////////////////////////////////////////
	
}