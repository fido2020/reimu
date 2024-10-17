#pragma once

#include <reimu/core/logger.h>
#include <reimu/video/driver.h>
#include <reimu/video/input.h>

#include <assert.h>
#include <wayland-client.h>

#include <list>

#include "xdg-shell-client-protocol.h"

class WaylandDriver final : public reimu::video::Driver {
public:
    reimu::video::Window *window_create(const reimu::Vector2i &size) override;
    
    int get_window_client_handle();
    void window_client_dispatch();

    reimu::Vector2u get_display_size() override;
    
    void finish() override;

    void display_roundtrip() {
        wl_display_roundtrip(display);
    }

    wl_display *display = nullptr;
    wl_registry *registry = nullptr;
    wl_compositor *compositor = nullptr;
    xdg_wm_base *wm_base = nullptr;
    
    wl_seat *seat = nullptr;
    wl_pointer *pointer = nullptr;
    wl_keyboard *keyboard = nullptr;

    wl_output *output = nullptr;

    wl_shm *shm = nullptr;

    struct wl_cursor_theme *cursor_theme = nullptr;
    struct wl_cursor *cursor = nullptr;
    wl_buffer *cursor_buffer = nullptr;
    wl_surface *cursor_surface = nullptr;

    struct xkb_context *xkb_context = nullptr;
    struct xkb_state *xkb_state = nullptr;

    // 'serial' value of the last mouse button event, may be used for dragging the window
    uint32_t mouse_serial;

    // Mouse events get formed over multiple Wayland events,
    // keep track of event data and which window is active
    reimu::video::MouseEvent mouse_event;
    class WaylandWindow *mouse_window = nullptr;

    reimu::video::KeyboardEvent keyboard_event;
    class WaylandWindow *keyboard_window = nullptr;

    std::list<class WaylandWindow *> windows; 

    std::list<reimu::Vector2u> display_sizes;
};
