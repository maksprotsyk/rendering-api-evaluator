#include "RenderingSystem.h"

#include <codecvt>

#include "Components/Transform.h"
#include "Components/Tag.h"
#include "Components/Model.h"
#include "Components/JsonData.h"
#include "Utils/BasicUtils.h"
#include "Managers/GameController.h"


namespace Engine::Systems
{
	RenderingSystem::RenderingSystem(const Visual::Window& window): _window(window)
	{
	}
	void RenderingSystem::onStart()
	{
		auto& compManager = GameController::get().getComponentsManager();

		const auto& tagSet = compManager.getComponentSet<Components::Tag>();
		const auto& jsonDataSet = compManager.getComponentSet<Components::JsonData>();
		for (EntityID id: compManager.entitiesWithComponents<Components::JsonData, Components::Tag>())
		{
			const std::string& tag = tagSet.getElement(id).tag;
			if (tag == "Config")
			{
				const nlohmann::json& data = jsonDataSet.getElement(id).data;
				if (data.contains("Renderer"))
				{
					const std::string& renderer = data["Renderer"];
					if (renderer == "DirectX")
					{
						_renderer = std::make_unique<Visual::DirectXRenderer>();
					}
					else if (renderer == "Vulkan")
					{
						_renderer = std::make_unique<Visual::VulkanRenderer>();
					}
					else if (renderer == "OpenGL")
					{
						_renderer = std::make_unique<Visual::OpenGLRenderer>();
					}
				}
				break;
			}
		}

		if (!_renderer)
		{
			_renderer = std::make_unique<Visual::DirectXRenderer>();
		}

		_renderer->init(_window);


		auto& modelSet = compManager.getComponentSet<Components::Model>();
		auto& transformSet = compManager.getComponentSet<Components::Transform>();
		for (EntityID id : compManager.entitiesWithComponents<Components::Model, Components::Transform>())
		{
			Components::Model& model = modelSet.getElement(id);

			if (!_renderer->loadModel(model.path))
			{
				// TODO: asserts, error handling
				continue;
			}

			model.instance = _renderer->createModelInstance(model.path);
		}


		for (EntityID id : compManager.entitiesWithComponents<Components::Tag>())
		{
			if (tagSet.getElement(id).tag == "MainCamera")
			{
				_cameraId = id;
			}
		}

	}

	void RenderingSystem::onUpdate(float dt)
	{
		auto& compManager = GameController::get().getComponentsManager();
		const auto& cameraTransform = compManager.getComponentSet<Components::Transform>().getElement(_cameraId);
		_renderer->setCameraProperties(cameraTransform.position, cameraTransform.rotation);

		auto& modelSet = compManager.getComponentSet<Components::Model>();
		const auto& transformSet = compManager.getComponentSet<Components::Transform>();

		_renderer->clearBackground(0.0f, 0.2f, 0.4f, 1.0f);
		for (EntityID id : compManager.entitiesWithComponents<Components::Model, Components::Transform>())
		{

			Components::Model& model = modelSet.getElement(id);
			if (model.markedForDestroy)
			{
				_renderer->destroyModelInstance(*model.instance);
				modelSet.removeElement(id);
				continue;
			}
			
			if (!model.instance)
			{
				model.instance = _renderer->createModelInstance(model.path);
			}

			const Components::Transform& transform = transformSet.getElement(id);
			_renderer->draw(*model.instance, transform.position, transform.rotation, transform.scale);
		}
		_renderer->render();
	}

	void RenderingSystem::onStop()
	{
		_renderer->cleanUp();
	}

	int RenderingSystem::getPriority() const
	{
		return 0;
	}
	
}