#include "driver.h"

#include "window.h"

#include <windows.h>

reimu::video::Driver *win32_init();

LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) {
    switch (msg) {
    case WM_CREATE: {
        CREATESTRUCT *create_data = (CREATESTRUCT *)l_param;

        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)create_data->lpCreateParams);

        // Get the window object from the create params
        Win32Window *win = (Win32Window *)create_data->lpCreateParams;

        win->handle = hwnd;
        win->hinstance = create_data->hInstance;

        break;
    } case WM_CLOSE:
        auto *win = (Win32Window *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

        win->dispatch_event("wm_close"_hashid);

        break;
    }

    return DefWindowProc(hwnd, msg, w_param, l_param);
}

reimu::video::Window *WindowsDriver::window_create(const reimu::Vector2i &size) {
    WNDCLASSEX wc;
    HWND hwnd;
    MSG msg;

    auto h_instance = GetModuleHandle(nullptr);

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

    auto *win = new Win32Window(size);

    // Pass the window object to the window procedure
    hwnd = CreateWindowEx(0, "reimu", "reimu", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        size.x, size.y, nullptr, nullptr, h_instance, win);

    if (!hwnd) {
        reimu::logger::warn("win32: Failed to create window");
        return nullptr;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    return win;
}

int WindowsDriver::get_window_client_handle() {
    return 0;
}

void WindowsDriver::window_client_dispatch() {

}

void WindowsDriver::finish() {

}

reimu::video::Driver *win32_init() {
    reimu::logger::debug("win32: Initializing driver");

    auto *d = new WindowsDriver;

    return d;
}
