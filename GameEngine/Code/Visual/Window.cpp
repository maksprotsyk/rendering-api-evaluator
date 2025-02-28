#include "Window.h"

#include <windowsx.h>

namespace Engine::Visual
{
    //////////////////////////////////////////////////////////////////////////

    bool Window::initWindow(HINSTANCE hInstance, int width, int height)
    {
        const wchar_t* CLASS_NAME = TEXT("Sample Window Class");

        WNDCLASS wc = { };
        wc.lpfnWndProc = windowProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = CLASS_NAME;

        RegisterClass(&wc);

        m_window = CreateWindowEx(
            0,
            CLASS_NAME,
            TEXT("Render Tester"),
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, width, height,
            nullptr,
            nullptr,
            hInstance,
            this
        );

        return m_window != nullptr;
    }

    //////////////////////////////////////////////////////////////////////////

    void Window::showWindow(int nCmdShow)
    {
        ShowWindow(m_window, nCmdShow);
    }

    //////////////////////////////////////////////////////////////////////////

    LRESULT CALLBACK Window::handleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE)
            {
                PostQuitMessage(0); // Quit when Escape is pressed
            }
            if (m_onKeyStateChanged)
            {
                m_onKeyStateChanged(wParam, true);
            }
            return 0;

        case WM_KEYUP:
            if (m_onKeyStateChanged)
            {
                m_onKeyStateChanged(wParam, false);
            }
            return 0;

        case WM_MOUSEMOVE:
        {
            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);
            // Handle mouse movement at (xPos, yPos)
            return 0;
        }

        case WM_LBUTTONDOWN:
        {
            return 0;
        }

        case WM_RBUTTONDOWN:
        {
            return 0;
        }
        default:
            break;
        }

        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    //////////////////////////////////////////////////////////////////////////

    bool Window::update()
    {
        MSG msg = { };

        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
            {
                return true;
            }
        }
        return false;
    }

    //////////////////////////////////////////////////////////////////////////

    void Window::SetOnKetStateChanged(const std::function<void(WPARAM, bool)>& callback)
    {
        m_onKeyStateChanged = callback;
    }

    //////////////////////////////////////////////////////////////////////////

    HWND Window::getHandle() const
    {
        return m_window;
    }

    //////////////////////////////////////////////////////////////////////////

    LRESULT CALLBACK Window::windowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        Window* pThis;

        if (uMsg == WM_NCCREATE)
        {
            // Retrieve the "this" pointer from the CREATESTRUCT lParam
            CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
            pThis = (Window*)pCreate->lpCreateParams;
            // Store the pointer in the user data of the window
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        }
        else 
        {
            // Retrieve the stored "this" pointer
            pThis = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        }

        if (pThis)
        {
            // Forward the message to the member function
            return pThis->handleMessage(hwnd, uMsg, wParam, lParam);
        }
        else 
        {
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
    }

    //////////////////////////////////////////////////////////////////////////

}