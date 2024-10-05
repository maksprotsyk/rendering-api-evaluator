// GameEngine.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <windowsx.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <iostream>

#include "Visual/Window.h"
#include "Events/NativeInputEvents.h"
#include "Managers/EventsManager.h"
#include "Managers/SystemsManager.h"
#include "Managers/ComponentsManager.h"
#include "Systems/RenderingSystem.h"
#include "Systems/PhysicsSystem.h"
#include "Systems/InputSystem.h"

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

    Engine::Visual::Window window;
    if (!window.initWindow(hInstance))
    {
        return 0;
    }

    window.showWindow(nCmdShow);
    window.SetOnKetStateChanged([](WPARAM param, bool state)
        {
            Engine::EventsManager::get().emit(Engine::Events::NativeKeyStateChanged{ param, state });
        });

    Engine::SystemsManager& systemsManager = Engine::SystemsManager::get();
    systemsManager.addSystem(std::make_unique<Engine::Systems::RenderingSystem>(window));
    systemsManager.addSystem(std::make_unique<Engine::Systems::PhysicsSystem>());
    systemsManager.addSystem(std::make_unique<Engine::Systems::InputSystem>());

    Engine::ComponentsManager& componentsManager = Engine::ComponentsManager::get();
    Engine::EntitiesManager& entitiesManager = Engine::EntitiesManager::get();

    const nlohmann::json& json = Engine::Utils::Parser::readJson("../Resources/test.json");
    for (const nlohmann::json& entityJson: json["Entities"])
    {
        Engine::EntityID id = entitiesManager.createEntity();
        for (const nlohmann::json& compJson : entityJson["Components"])
        {
            componentsManager.createComponentFromJson(id, compJson);
        }
    }

    const float targetFrameTime = 1.0f / 60.0f; // 60 FPS
    while (true)
    {
        // Measure the time taken for the frame
        auto start = std::chrono::high_resolution_clock::now();

        bool needToExit = window.update();
        if (needToExit)
        {
            break;
        }

        // Update all systems
        systemsManager.update(targetFrameTime);

        // Measure the time taken for the frame
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = end - start;

        // Sleep to maintain consistent frame time
        if (elapsed.count() < targetFrameTime)
        {
            std::this_thread::sleep_for(std::chrono::duration<float>(targetFrameTime - elapsed.count()));
        }
    }

    return 0;
}