// GameEngine.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <windowsx.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <filesystem>

#include "Managers/GameController.h"
#include "Events/NativeInputEvents.h"

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
            Engine::GameController::get().getEventsManager().emit(Engine::Events::NativeKeyStateChanged{param, state});
        }
    );

	Engine::GameController& gameController = Engine::GameController::get();
	gameController.setWindow(window);
	gameController.setConfig(jsonPath);
	gameController.init();
	gameController.run();
	gameController.clear();

    return 0;
}