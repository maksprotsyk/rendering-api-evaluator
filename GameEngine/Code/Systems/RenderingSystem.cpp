#include "RenderingSystem.h"

#include <codecvt>

#include "Components/Transform.h"
#include "Components/Tag.h"
#include "Components/Model.h"
#include "Utils/BasicUtils.h"


namespace Engine::Systems
{
	RenderingSystem::RenderingSystem(const Visual::Window& window): _renderer(window)
	{
	}
	void RenderingSystem::onStart()
	{
		auto& modelSet = ComponentsManager::get().getComponentSet<Components::Model>();
		auto& transformSet = ComponentsManager::get().getComponentSet<Components::Transform>();
		for (EntityID id : ComponentsManager::get().entitiesWithComponents<Components::Model, Components::Transform>())
		{
			Components::Model& model = modelSet.getElement(id);
			_renderer.loadModel(model.model, model.path);

			const Components::Transform& transform = transformSet.getElement(id);
			const Utils::Vector3& position = transform.position;
			Visual::DirectXRenderer::transformModel(model.model, transform.position, transform.rotation, transform.scale);

			_renderer.createBuffersFromModel(model.model);
		}


		auto& tagSet = ComponentsManager::get().getComponentSet<Components::Tag>();
		for (EntityID id : ComponentsManager::get().entitiesWithComponents<Components::Tag>())
		{
			if (tagSet.getElement(id).tag == "MainCamera")
			{
				_cameraId = id;
			}
		}

	}

	void RenderingSystem::onUpdate(float dt)
	{
		auto& compManager = ComponentsManager::get();
		const auto& cameraTransform = compManager.getComponentSet<Components::Transform>().getElement(_cameraId);
		_renderer.setCameraProperties(cameraTransform.position, cameraTransform.rotation);

		auto& modelSet = compManager.getComponentSet<Components::Model>();
		const auto& transformSet = compManager.getComponentSet<Components::Transform>();

		_renderer.clearBackground();
		for (EntityID id : compManager.entitiesWithComponents<Components::Model, Components::Transform>())
		{
			Components::Model& model = modelSet.getElement(id);
			const Components::Transform& transform = transformSet.getElement(id);
			Visual::DirectXRenderer::transformModel(model.model, transform.position, transform.rotation, transform.scale);

			_renderer.draw(model.model);
		}
		_renderer.render();
	}

	void RenderingSystem::onStop()
	{

	}

	int RenderingSystem::getPriority() const
	{
		return 0;
	}
	
}