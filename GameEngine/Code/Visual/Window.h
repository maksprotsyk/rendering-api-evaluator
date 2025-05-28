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
		static const wchar_t* getChildWindowClassName();

	private:
		static LRESULT CALLBACK windowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		LRESULT CALLBACK handleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	private:
		HWND m_window;
		std::function<void(WPARAM, bool)> m_onKeyStateChanged = nullptr;

	};
}