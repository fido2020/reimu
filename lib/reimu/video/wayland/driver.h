#pragma once

#include <reimu/core/logger.h>
#include <reimu/video/driver.h>

#include <assert.h>
#include <wayland-client.h>

#include "../egl/egl.h"

#include "xdg-shell-client-protocol.h"

class WaylandDriver final : public reimu::video::Driver {
public:
    reimu::video::Window *window_create(const reimu::Vector2i &size) override;
    
    int get_window_client_handle();
    void window_client_dispatch();
    
    void finish() override;

    void display_roundtrip() {
        wl_display_roundtrip(display);
    }

    wl_display *display = nullptr;
    wl_registry *registry = nullptr;
    wl_compositor *compositor = nullptr;
    xdg_wm_base *wm_base = nullptr;
};
