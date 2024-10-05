#pragma once

#include <Windows.h>
#include <functional>

namespace Engine::Visual
{
	class DirectXRenderer;

	class Window
	{
	public:
		bool initWindow(HINSTANCE hInstance);
		void showWindow(int nCmdShow);
		bool update();
		void SetOnKetStateChanged(const std::function<void(WPARAM, bool)>& callback);

	private:
		friend DirectXRenderer;

		HWND _window;
		std::function<void(WPARAM, bool)> _onKeyStateChanged = nullptr;

		static LRESULT CALLBACK windowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		LRESULT CALLBACK handleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	};
}