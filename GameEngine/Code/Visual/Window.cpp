#include "Window.h"

#include <windowsx.h>

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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

        WNDCLASSEXW childWc = {};
        childWc.cbSize = sizeof(childWc);
        childWc.style = CS_OWNDC;
        childWc.lpfnWndProc = windowProc;
        childWc.hInstance = GetModuleHandle(nullptr);
        childWc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        childWc.hbrBackground = nullptr;
        childWc.lpszClassName = getChildWindowClassName();

        RegisterClassExW(&childWc);

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
                PostQuitMessage(0);
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

    const wchar_t* Window::getChildWindowClassName()
    {
        return TEXT("ChildWindowClass");
    }

    //////////////////////////////////////////////////////////////////////////

    LRESULT CALLBACK Window::windowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
        {
            return true;
        }

        Window* pThis;

        if (uMsg == WM_NCCREATE)
        {
            CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
            pThis = (Window*)pCreate->lpCreateParams;

            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        }
        else 
        {
            pThis = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        }

        if (pThis)
        {
            return pThis->handleMessage(hwnd, uMsg, wParam, lParam);
        }
        else 
        {
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
    }

    //////////////////////////////////////////////////////////////////////////

}