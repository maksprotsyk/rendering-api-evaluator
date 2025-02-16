// GameEngine.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <windowsx.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <iostream>
#include <filesystem>

#include "Visual/Window.h"
#include "Events/NativeInputEvents.h"
#include "Managers/EventsManager.h"
#include "Managers/SystemsManager.h"
#include "Managers/ComponentsManager.h"
#include "Systems/RenderingSystem.h"
#include "Systems/PhysicsSystem.h"
#include "Systems/InputSystem.h"
#include "Systems/StatsSystem.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    AllocConsole();

    FILE* fpOut = nullptr;
    FILE* fpIn = nullptr;

    // Redirect stdout to the console
    if (freopen_s(&fpOut, "CONOUT$", "w", stdout) != 0) {
        std::cerr << "Failed to redirect stdout to console!" << std::endl;
        return 1;
    }

    // Redirect stdin to the console
    if (freopen_s(&fpIn, "CONIN$", "r", stdin) != 0) {
        std::cerr << "Failed to redirect stdin to console!" << std::endl;
        return 1;
    }

    LPWSTR cmdLine = GetCommandLineW();
    int argc;
    LPWSTR* argv = CommandLineToArgvW(cmdLine, &argc);
    std::string jsonPath = "../../Configs/config.json";

    if (argc > 1)
    {
        jsonPath = Engine::Utils::wstringToString(argv[1]);
    }

    int width = 1000;
    int height = 750;
    if (argc > 3)
    {
        width = std::stoi(Engine::Utils::wstringToString(argv[2]));
        height = std::stoi(Engine::Utils::wstringToString(argv[3]));
    }

    Engine::Visual::Window window;


    if (!window.initWindow(hInstance, width, height))
    {
        return 0;
    }

    window.showWindow(nCmdShow);
    window.SetOnKetStateChanged([](WPARAM param, bool state)
        {
            Engine::EventsManager::get().emit(Engine::Events::NativeKeyStateChanged{ param, state });
        });

    bool nativeExitRequested = false;
    Engine::EventsManager::get().subscribe<Engine::Events::NativeExitRequested>(
		[&nativeExitRequested](const Engine::Events::NativeExitRequested&)
		{
			nativeExitRequested = true;
		}
	);

    Engine::SystemsManager& systemsManager = Engine::SystemsManager::get();
    systemsManager.addSystem(std::make_unique<Engine::Systems::StatsSystem>());
    systemsManager.addSystem(std::make_unique<Engine::Systems::RenderingSystem>(window));
    systemsManager.addSystem(std::make_unique<Engine::Systems::PhysicsSystem>());
    systemsManager.addSystem(std::make_unique<Engine::Systems::InputSystem>());

    Engine::ComponentsManager& componentsManager = Engine::ComponentsManager::get();
    Engine::EntitiesManager& entitiesManager = Engine::EntitiesManager::get();

    const nlohmann::json& json = Engine::Utils::Parser::readJson(jsonPath);
    for (const nlohmann::json& entityJson: json["Entities"])
    {
        Engine::EntityID id = entitiesManager.createEntity();
        for (const nlohmann::json& compJson : entityJson["Components"])
        {
            componentsManager.createComponentFromJson(id, compJson);
        }
    }

    float dt = 0;
    while (!nativeExitRequested)
    {
        // Measure the time taken for the frame

        bool needToExit = window.update();
        if (needToExit)
        {
            break;
        }

		systemsManager.processAddedSystems();
		systemsManager.processRemovedSystems();
        
        // Update all systems
        auto start = std::chrono::high_resolution_clock::now();
        systemsManager.update(dt);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = end - start;
        dt = elapsed.count();
    
    }

    systemsManager.stop();

    

    return 0;
}