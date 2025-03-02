#include "GameController.h"

#include "Utils/DebugMacros.h"
#include "Events/NativeInputEvents.h"


namespace Engine
{
	//////////////////////////////////////////////////////////////////////////

	std::unique_ptr<GameController> GameController::m_instance = nullptr;

	//////////////////////////////////////////////////////////////////////////

	GameController& GameController::get()
	{
		if (m_instance == nullptr)
		{
			m_instance = std::unique_ptr<GameController>(new GameController());
		}
		return *m_instance;
	}

	//////////////////////////////////////////////////////////////////////////

	void GameController::setWindow(const Visual::Window& window)
	{
		m_window = window;
	}

	//////////////////////////////////////////////////////////////////////////

	const Visual::Window& GameController::getWindow() const
	{
		return m_window;
	}

	//////////////////////////////////////////////////////////////////////////

	void GameController::setConfig(const std::string& configPath)
	{
		m_configPath = configPath;
		m_config = Utils::Parser::readJson(configPath);
	}

	//////////////////////////////////////////////////////////////////////////

	std::string GameController::getConfigRelativePath(const std::string& path) const
	{
		std::filesystem::path fullConfigPath(m_configPath);
		std::filesystem::path configDir = fullConfigPath.parent_path();
		return (configDir / path).string();
	}

	//////////////////////////////////////////////////////////////////////////

	void GameController::init()
	{
		initPrefabs();
		initEntities();
		initSystems();
	}

	//////////////////////////////////////////////////////////////////////////

	void GameController::run()
	{

		bool nativeExitRequested = false;
		m_eventsManager.subscribe<Engine::Events::NativeExitRequested>(
			[&nativeExitRequested](const Engine::Events::NativeExitRequested&)
			{
				nativeExitRequested = true;
			}
		);

		float dt = 0;
		auto start = std::chrono::high_resolution_clock::now();
		while (!nativeExitRequested)
		{
			// Measure the time taken for the frame

			bool needToExit = m_window.update();
			if (needToExit)
			{
				break;
			}

			m_systemsManager.processAddedSystems();
			m_systemsManager.processRemovedSystems();
			
			// calculating frame time
			auto end = std::chrono::high_resolution_clock::now();
			std::chrono::duration<float> elapsed = end - start;
			dt = elapsed.count();

			start = std::chrono::high_resolution_clock::now();
			m_systemsManager.update(dt);
		}

		m_systemsManager.stop();
	}

	//////////////////////////////////////////////////////////////////////////

	void GameController::clear()
	{
		m_systemsManager.clear();
		m_componentsManager.clear();
		m_entitiesManager.clear();
	}

	//////////////////////////////////////////////////////////////////////////

	EventsManager& GameController::getEventsManager()
	{
		return m_eventsManager;
	}

	//////////////////////////////////////////////////////////////////////////

	ComponentsManager& GameController::getComponentsManager()
	{
		return m_componentsManager;
	}

	//////////////////////////////////////////////////////////////////////////

	SystemsManager& GameController::getSystemsManager()
	{
		return m_systemsManager;
	}

	//////////////////////////////////////////////////////////////////////////
	
	EntitiesManager& GameController::getEntitiesManager()
	{
		return m_entitiesManager;
	}

	//////////////////////////////////////////////////////////////////////////

	const EventsManager& GameController::getEventsManager() const
	{
		return m_eventsManager;
	}

	//////////////////////////////////////////////////////////////////////////

	const ComponentsManager& GameController::getComponentsManager() const
	{
		return m_componentsManager;
	}

	const SystemsManager& GameController::getSystemsManager() const
	{
		return m_systemsManager;
	}

	//////////////////////////////////////////////////////////////////////////

	const EntitiesManager& GameController::getEntitiesManager() const
	{
		return m_entitiesManager;
	}

	//////////////////////////////////////////////////////////////////////////

	EntityID GameController::createPrefab(const std::string& prefabName)
	{
		Engine::EntityID id = m_entitiesManager.createEntity();
		const auto& prefabItr = m_prefabs.find(prefabName);

		ASSERT(prefabItr != m_prefabs.end(), "Prefab not found");
		if (prefabItr == m_prefabs.end())
		{
			return id;
		}

		const nlohmann::json& prefabJson = prefabItr->second;
		ASSERT(prefabJson.contains(k_componentsField), "Prefab must have {} field", k_componentsField);
		if (!prefabJson.contains(k_componentsField))
		{
			return id;
		}

		for (const nlohmann::json& compJson : prefabJson[k_componentsField])
		{
			m_componentsManager.createComponentFromJson(id, compJson);
		}

		return id;
	}

	//////////////////////////////////////////////////////////////////////////

	void GameController::createEntity(const nlohmann::json& entityJson)
	{
		Engine::EntityID id = m_entitiesManager.createEntity();

		if (entityJson.contains(k_componentsField))
		{
			for (const nlohmann::json& compJson : entityJson[k_componentsField])
			{
				m_componentsManager.createComponentFromJson(id, compJson);
			}
		}

		if (!entityJson.contains(k_prefabField))
		{
			return;
		}

		std::string prefabName = entityJson[k_prefabField].get<std::string>();
		const auto& prefabItr = m_prefabs.find(prefabName);
		if (prefabItr == m_prefabs.end())
		{
			return;
		}

		const nlohmann::json& prefabJson = prefabItr->second;
		if (!prefabJson.contains(k_componentsField))
		{
			return;
		}

		for (const nlohmann::json& compJson : prefabJson[k_componentsField])
		{
			m_componentsManager.createComponentFromJson(id, compJson);
		}		
		
	}

	//////////////////////////////////////////////////////////////////////////

	void GameController::initPrefabs()
	{
		for (const nlohmann::json& prefabJson : m_config[k_prefabsField])
		{
			if (!prefabJson.contains(k_nameField))
			{
				continue;
			}

			std::string prefabName = prefabJson[k_nameField].get<std::string>();
			m_prefabs[prefabName] = prefabJson;
		}
	}

	//////////////////////////////////////////////////////////////////////////

	void GameController::initEntities()
	{
		for (const nlohmann::json& entityJson : m_config[k_entitiesField])
		{
			createEntity(entityJson);
		}
	}

	//////////////////////////////////////////////////////////////////////////

	void GameController::initSystems()
	{
		for (const nlohmann::json& entityJson : m_config[k_systemsField])
		{
			m_systemsManager.loadSystemFromJson(entityJson);
		}
	}

	//////////////////////////////////////////////////////////////////////////

}