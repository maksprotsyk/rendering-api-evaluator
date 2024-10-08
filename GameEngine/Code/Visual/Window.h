#pragma once

#include <Windows.h>
#include <functional>

namespace Engine::Visual
{
	class Window
	{
	public:
		bool initWindow(HINSTANCE hInstance, int width, int height);
		void showWindow(int nCmdShow);
		bool update();
		void SetOnKetStateChanged(const std::function<void(WPARAM, bool)>& callback);
		HWND getHandle() const;

	private:

		HWND _window;
		std::function<void(WPARAM, bool)> _onKeyStateChanged = nullptr;

		static LRESULT CALLBACK windowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		LRESULT CALLBACK handleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	};
}