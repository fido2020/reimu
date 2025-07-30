#include "driver.h"

#include "window.h"

#include <windows.h>
#include <windowsx.h>
#include <winuser.h>

reimu::video::Driver *win32_init();

reimu::video::Key win32_to_reimu_key_code(WPARAM key) {
    switch (key)
    {
    case VK_TAB:
        return reimu::video::Key::Tab;
    case VK_RETURN:
        return reimu::video::Key::Return;
    case VK_SHIFT:
        return reimu::video::Key::Shift;
    case VK_CONTROL:
        return reimu::video::Key::Ctrl;
    case VK_MENU:
        return reimu::video::Key::Alt;
    case VK_PAUSE:
        return reimu::video::Key::Pause;
    case VK_ESCAPE:
        return reimu::video::Key::Escape;
    case VK_SPACE:
        return reimu::video::Key::Space;
    case VK_PRIOR:
        return reimu::video::Key::PageUp;
    case VK_NEXT:
        return reimu::video::Key::PageDown;
    case VK_END:
        return reimu::video::Key::End;
    case VK_HOME:
        return reimu::video::Key::Home;
    case VK_LEFT:
        return reimu::video::Key::Left;
    case VK_UP:
        return reimu::video::Key::Up;
    case VK_RIGHT:
        return reimu::video::Key::Right;
    case VK_DOWN:
        return reimu::video::Key::Down;
    case VK_SNAPSHOT:
        return reimu::video::Key::PrintScreen;
    case VK_INSERT:
        return reimu::video::Key::Insert;
    case VK_DELETE:
        return reimu::video::Key::Delete;
    case VK_LWIN:
    case VK_RWIN:
        return reimu::video::Key::Win;
    case VK_NUMLOCK:
        return reimu::video::Key::NumLock;
    case VK_SCROLL:
        return reimu::video::Key::ScrollLock;
    case VK_F1:
        return reimu::video::Key::F1;
    case VK_F2:
        return reimu::video::Key::F2;
    case VK_F3:
        return reimu::video::Key::F3;
    case VK_F4:
        return reimu::video::Key::F4;
    case VK_F5:
        return reimu::video::Key::F5;
    case VK_F6:
        return reimu::video::Key::F6;
    case VK_F7:
        return reimu::video::Key::F7;
    case VK_F8:
        return reimu::video::Key::F8;
    case VK_F9:
        return reimu::video::Key::F9;
    case VK_F10:
        return reimu::video::Key::F10;
    case VK_F11:
        return reimu::video::Key::F11;
    case VK_F12:
        return reimu::video::Key::F12;
    default:
        return reimu::video::Key::Invalid;
    }
}

LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) {
    auto *win = (Win32Window *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    
    switch (msg) {
    case WM_CREATE: {
        CREATESTRUCT *create_data = (CREATESTRUCT *)l_param;

        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)create_data->lpCreateParams);

        // Get the window object from the create params
        win = (Win32Window *)create_data->lpCreateParams;

        win->handle = hwnd;
        win->hinstance = create_data->hInstance;

        break;
    } case WM_CLOSE: {
        win->dispatch_event("wm_close"_hashid);

        break;
    } case WM_PAINT: {
        win->render();
        break;
        
    } case WM_LBUTTONDOWN: {
        reimu::video::MouseEvent event;

        event.is_button = true;
        event.button = reimu::video::MouseButton::Left;
        event.state = reimu::video::MouseButtonState::Pressed;

        event.pos.x = GET_X_LPARAM(l_param);
        event.pos.y = GET_Y_LPARAM(l_param);

        win->queue_input_event({
            .type = reimu::video::InputEvent::Mouse,
            .mouse = event
        });

        break;
    } case WM_LBUTTONUP: {
        reimu::video::MouseEvent event;

        event.is_button = true;
        event.button = reimu::video::MouseButton::Left;
        event.state = reimu::video::MouseButtonState::Released;

        event.pos.x = GET_X_LPARAM(l_param);
        event.pos.y = GET_Y_LPARAM(l_param);

        win->queue_input_event({
            .type = reimu::video::InputEvent::Mouse,
            .mouse = event
        });

        break;
    } case WM_MOUSEMOVE: {
        reimu::video::MouseEvent event;

        event.is_move = true;
        event.pos.x = GET_X_LPARAM(l_param);
        event.pos.y = GET_Y_LPARAM(l_param);

        win->queue_input_event({
            .type = reimu::video::InputEvent::Mouse,
            .mouse = event
        });

        break;
    } case WM_MOUSELEAVE: {
        reimu::video::MouseEvent event;

        event.is_leave = true;
        event.pos.x = GET_X_LPARAM(l_param);
        event.pos.y = GET_Y_LPARAM(l_param);

        win->queue_input_event({
            .type = reimu::video::InputEvent::Mouse,
            .mouse = event
        });

        break;
    } case WM_KEYDOWN: {
        reimu::video::KeyboardEvent event;

        auto key = win32_to_reimu_key_code(w_param);
        if (key == reimu::video::Key::Invalid) {
            // Will get handled by TranslateMessage
            break;
        }

        event.is_ctrl = GetAsyncKeyState(VK_CONTROL) != 0;
        event.is_shift = GetAsyncKeyState(VK_SHIFT) != 0;
        event.is_alt = GetAsyncKeyState(VK_MENU) != 0;
        event.is_win = GetAsyncKeyState(VK_LWIN) != 0;
        event.is_down = true;
        event.key = key;

        win->queue_input_event({
            .type = reimu::video::InputEvent::Keyboard,
            .key = event
        });

        break;
    } case WM_CHAR: {
        reimu::video::KeyboardEvent event;

        event.is_ctrl = GetAsyncKeyState(VK_CONTROL) != 0;
        event.is_shift = GetAsyncKeyState(VK_SHIFT) != 0;
        event.is_alt = GetAsyncKeyState(VK_MENU) != 0;
        event.is_win = GetAsyncKeyState(VK_LWIN) != 0;
        event.is_down = true;

        event.key = (reimu::video::Key)w_param;

        win->queue_input_event({
            .type = reimu::video::InputEvent::Keyboard,
            .key = event
        });
    }
    }

    if (win && win->has_event()) {
        win->dispatch_event("wm_input"_hashid);
    }

    return DefWindowProc(hwnd, msg, w_param, l_param);
}

reimu::video::Window *WindowsDriver::window_create(const reimu::Vector2i &size) {
    HWND hwnd;

    auto h_instance = GetModuleHandle(nullptr);

    auto *win = new Win32Window(size);

    int x = (GetSystemMetrics(SM_CXSCREEN) - size.x) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - size.y) / 2;

    // Pass the window object to the window procedure
    hwnd = CreateWindowEx(0, "reimu", "reimu", WS_POPUP | WS_MINIMIZEBOX, x, y,
        size.x, size.y, nullptr, nullptr, h_instance, win);

    if (!hwnd) {
        reimu::logger::warn("win32: Failed to create window");
        return nullptr;
    }

    window_client_dispatch();

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    return win;
}

os_handle_t WindowsDriver::get_window_client_handle() {
    // Unfortunately the way win32 works it has a distinct message queue,
    // so we can't (to my knowledge) just take a handle and wait on it
    return (HANDLE)-1;
}

void WindowsDriver::window_client_dispatch() {
    MSG msg;
    while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

reimu::Vector2u WindowsDriver::get_display_size() {
    return reimu::Vector2u(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
}

void WindowsDriver::finish() {

}

reimu::video::Driver *win32_init() {
    reimu::logger::debug("win32: Initializing driver");

    SetProcessDPIAware();

    auto *d = new WindowsDriver;

    auto h_instance = GetModuleHandle(nullptr);

    WNDCLASSEX &wc = d->wc;

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = window_proc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = h_instance;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = "reimu";
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

    if (!RegisterClassEx(&wc)) {
        reimu::logger::warn("win32: Failed to register window class");
        return nullptr;
    }

    return d;
}
