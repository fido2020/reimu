#include "driver.h"

#include "window.h"

#include <windows.h>
#include <windowsx.h>
#include <winuser.h>

reimu::video::Driver *win32_init();

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

int WindowsDriver::get_window_client_handle() {
    // Unfortunately the way win32 works it has a distinct message queue,
    // so we can't (to my knowledge) just take a handle and wait on it
    return -1;
}

void WindowsDriver::window_client_dispatch() {
    MSG msg;
    while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void WindowsDriver::finish() {

}

reimu::video::Driver *win32_init() {
    reimu::logger::debug("win32: Initializing driver");

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
