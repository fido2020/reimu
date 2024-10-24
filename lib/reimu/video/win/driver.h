#pragma once

#include <reimu/core/logger.h>
#include <reimu/video/driver.h>
#include <reimu/video/input.h>

#include <windows.h>

class WindowsDriver final : public reimu::video::Driver {
public:
    reimu::video::Window *window_create(const reimu::Vector2i &size) override;
    
    os_handle_t get_window_client_handle();
    void window_client_dispatch();

    reimu::Vector2u get_display_size() override;
    
    void finish() override;

    WNDCLASSEX wc;
};
