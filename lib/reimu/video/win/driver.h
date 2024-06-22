#pragma once

#include <reimu/core/logger.h>
#include <reimu/video/driver.h>
#include <reimu/video/input.h>

class WindowsDriver final : public reimu::video::Driver {
public:
    reimu::video::Window *window_create(const reimu::Vector2i &size) override;
    
    int get_window_client_handle();
    void window_client_dispatch();
    
    void finish() override;

    void *dummy;
};
