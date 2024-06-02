#include <reimu/core/logger.h>
#include <reimu/core/result.h>
#include <reimu/graphics/vector.h>
#include <reimu/video/driver.h>

#include <linux/input-event-codes.h>

#include <wayland-client.h>

#include <assert.h>
#include <string.h>

#include "../egl/egl.h"

#include "driver.h"
#include "window.h"
#include "xdg-shell-client-protocol.h"

reimu::video::Driver *wayland_init();

// Sends a ping to the window server indicating we are still responsive
static void xdg_ping(void *data, struct xdg_wm_base *wm_base, uint32_t serial);
static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial);
static void xdg_toplevel_configure(void *data, xdg_toplevel *toplevel, int32_t width,
        int32_t height, struct wl_array *states);
static void xdg_toplevel_close(void *data, xdg_toplevel *toplevel);
static void xdg_toplevel_configure_bounds(void *data, xdg_toplevel *xdg_toplevel,
        int32_t width, int32_t height);
static void xdg_toplevel_wm_capabilities(void *data, xdg_toplevel *xdg_toplevel,
        wl_array *capabilities);

static void wl_seat_capabilities(void *data, struct wl_seat *seat, uint32_t capabilities);
static void wl_seat_name(void *data, struct wl_seat *seat, const char *name);

static void registry_handler(void *data, struct wl_registry *registry, uint32_t name,
        const char *interface, uint32_t version);
static void registry_remove_handler(void *data, struct wl_registry *registry, uint32_t name);

static void pointer_enter(void *data, struct wl_pointer *pointer, uint32_t serial,
        struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y);
static void pointer_leave(void *data, struct wl_pointer *pointer, uint32_t serial,
        struct wl_surface *surface);
static void pointer_move(void *data, struct wl_pointer *pointer, uint32_t time,
        wl_fixed_t surface_x, wl_fixed_t surface_y);
static void pointer_button(void *data, struct wl_pointer *pointer, uint32_t serial,
        uint32_t time, uint32_t button, uint32_t state);
static void pointer_frame(void *data, struct wl_pointer *pointer);

static const xdg_wm_base_listener wm_base_listener = {
    .ping = xdg_ping,
};

static const wl_registry_listener registry_listener = {
    .global = registry_handler,
    .global_remove = registry_remove_handler
};

static const xdg_surface_listener surface_listener = {
    .configure = xdg_surface_configure
};

static const xdg_toplevel_listener toplevel_listener = {
    .configure = xdg_toplevel_configure,
    .close = xdg_toplevel_close,
    .configure_bounds = xdg_toplevel_configure_bounds,
    .wm_capabilities = xdg_toplevel_wm_capabilities
};

static const wl_seat_listener seat_listener = {
    .capabilities = wl_seat_capabilities,
    .name = wl_seat_name
};

static const wl_pointer_listener pointer_listener = {
    .enter = pointer_enter,
    .leave = pointer_leave,
    .motion = pointer_move,
    .button = pointer_button,
    .axis = nullptr,
    .frame = pointer_frame,
    .axis_source = nullptr,
    .axis_stop = nullptr,
    .axis_discrete = nullptr
};

reimu::video::Window *WaylandDriver::window_create(const reimu::Vector2i &size) {  
    wl_surface *surface = wl_compositor_create_surface(compositor);
    if (!surface) {
        reimu::logger::fatal("Failed to create surface");
    }

    xdg_surface *xdg_surface = xdg_wm_base_get_xdg_surface(wm_base, surface);
    if (!xdg_surface) {
        reimu::logger::fatal("Failed to create xdg surface");
    }

    // An XDG 'toplevel' describes a normal window, as opposed to a transient window
    xdg_toplevel *xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);
    if (!xdg_toplevel) {
        reimu::logger::fatal("Failed to get XDG toplevel");
    }

    wl_surface_commit(surface);

    auto *win = new WaylandWindow {
        *this,
        surface,
        xdg_surface,
        xdg_toplevel,
        size
    };

    wl_surface_set_user_data(surface, win);

    xdg_surface_add_listener(xdg_surface, &surface_listener, win);
    xdg_toplevel_add_listener(xdg_toplevel, &toplevel_listener, win);

    windows.push_back(win);

    return win;
}

int WaylandDriver::get_window_client_handle() {
    return wl_display_get_fd(display);
}

void WaylandDriver::window_client_dispatch() {
    wl_display_dispatch(display);

    for (auto &win : windows) {
        if (win->has_event()) {
            win->dispatch_event("wm_input"_hashid);
        }
    }

    wl_display_flush(display);
}

void WaylandDriver::finish() {
    if (compositor) {
        wl_compositor_destroy(compositor);
    }

    if (registry) {
        wl_registry_destroy(registry);
    }

    if (display) {
        wl_display_disconnect(display);
    }
}

reimu::video::Driver *wayland_init() {
    WaylandDriver *d = new WaylandDriver;

    // Connect to the Wayland server
    auto display = wl_display_connect(NULL);
    if (!display) {
        reimu::logger::warn("Failed to connect to wayland server");
        return nullptr;
    }

    d->display = display;

    auto registry = wl_display_get_registry(display);
    if (!registry) {
        d->finish();
        delete d;

        reimu::logger::fatal("Failed to connect to wayland server");
    }

    d->registry = registry;

    // Get the wayland registry and bind to global objects as necessary
    wl_registry_add_listener(registry, &registry_listener, d);
    wl_display_roundtrip(display);

    // Make sure we found the compositor and 'xdg_wm_base'

    // The XDG protocols involve most of the actual window management,
    // the Wayland protocols just involve creating and managing objects including surfaces and
    // buffers, whereas through xdg-shell protocol we can assign attributes such as a title
    // to our windows.

    if (!(d->compositor && d->wm_base)) {
        d->finish();
        delete d;

        reimu::logger::fatal("Failed to create wayland bindings");
    }
    
    return d;
}

static void xdg_ping(void *, xdg_wm_base *wm_base, uint32_t serial) {
    xdg_wm_base_pong(wm_base, serial);
}

static void xdg_surface_configure(void *data, xdg_surface *surface, uint32_t serial) {
    auto *win = (WaylandWindow *)data;

    xdg_surface_ack_configure(surface, serial);

    wl_surface_commit(win->surface);
}

static void xdg_toplevel_configure(void *, xdg_toplevel *, int32_t width,
        int32_t height, struct wl_array *) {
    reimu::logger::debug("width: {}, height: {}", width, height);
}

static void xdg_toplevel_close(void *data, xdg_toplevel *) {
    reimu::logger::debug("user wants to close window");

    auto *win = (WaylandWindow *)data;

    win->dispatch_event("wm_close"_hashid);
}

static void xdg_toplevel_configure_bounds(void *, xdg_toplevel *, int32_t width, int32_t height) {
    reimu::logger::debug("xdg_toplevel_configure_bounds: width: {}, height: {}", width, height);
}

static void xdg_toplevel_wm_capabilities(void *, xdg_toplevel *, wl_array *) {
    reimu::logger::debug("xdg_toplevel_wm_capabilities");
}

static void wl_seat_capabilities(void *data, struct wl_seat *seat, uint32_t capabilities) {
    auto *d = (WaylandDriver *)data;

    if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
        d->pointer = wl_seat_get_pointer(seat);

        wl_pointer_add_listener(d->pointer, &pointer_listener, d);
    } else if (d->pointer) {
        wl_pointer_release(d->pointer);

        d->pointer = nullptr;
    }

    if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
        d->keyboard = wl_seat_get_keyboard(seat);
    } else if (d->keyboard) {
        wl_keyboard_release(d->keyboard);

        d->keyboard = nullptr;
    }
}

static void wl_seat_name(void *data, struct wl_seat *seat, const char *name) {
    auto *d = (WaylandDriver *)data;
    reimu::logger::debug("wl_seat_name: '{}'", name);
}

static void pointer_enter(void *data, struct wl_pointer *pointer, uint32_t serial,
        struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y) {
    auto *d = (WaylandDriver *)data;

    auto *win = (WaylandWindow *)wl_surface_get_user_data(surface);

    d->mouse_event = {};
    d->mouse_window = win;

    d->mouse_event.is_enter = true;

    d->mouse_event.pos = {
        static_cast<float>(wl_fixed_to_double(x)),
        static_cast<float>(wl_fixed_to_double(y))
    };
}

static void pointer_leave(void *data, struct wl_pointer *pointer, uint32_t serial,
        struct wl_surface *surface) {
    auto *d = (WaylandDriver *)data;

    d->mouse_event.is_leave = true;
}

static void pointer_move(void *data, struct wl_pointer *pointer, uint32_t serial,
        wl_fixed_t x, wl_fixed_t y) {
    auto *d = (WaylandDriver *)data;

    d->mouse_event.is_move = true;

    d->mouse_event.pos = {
        static_cast<float>(wl_fixed_to_double(x)),
        static_cast<float>(wl_fixed_to_double(y))
    };
}

static void pointer_button(void *data, struct wl_pointer *pointer, uint32_t serial,
        uint32_t time, uint32_t button, uint32_t state) {
    auto *d = (WaylandDriver *)data;

    d->mouse_event.is_button = true;
    
    switch (button) {
        case BTN_MIDDLE:
            d->mouse_event.button = reimu::video::MouseButton::Middle;
            break;
        case BTN_RIGHT:
            d->mouse_event.button = reimu::video::MouseButton::Right;
            break;
        case BTN_LEFT:
        default:
            d->mouse_event.button = reimu::video::MouseButton::Left;
            break;
    }

    if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
        d->mouse_event.state = reimu::video::MouseButtonState::Pressed;
    } else {
        d->mouse_event.state = reimu::video::MouseButtonState::Released;
    }
}

static void pointer_frame(void *data, struct wl_pointer *pointer) {
    auto *d = (WaylandDriver *)data;

    // If we received this event, we have recieved a full mouse event
    auto *win = d->mouse_window;
    if (win) {
        win->queue_input_event({
            .type = reimu::video::InputEvent::Mouse,
            .mouse = d->mouse_event
        });

        // Check if the mouse has left the window
        if (d->mouse_event.is_leave) {
            d->mouse_window = nullptr;
        }
    }
}

static void registry_handler(void *data, struct wl_registry *registry, uint32_t name,
        const char *interface, uint32_t version) {
    auto *d = (WaylandDriver *)data;

    reimu::logger::debug("interface: '{}', version: {}, name: {}", interface, version, name);

    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        d->compositor = (wl_compositor *)wl_registry_bind(registry, name, &wl_compositor_interface,
            version);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        d->wm_base = (xdg_wm_base *)wl_registry_bind(registry, name, &xdg_wm_base_interface,
            version);
        xdg_wm_base_add_listener(d->wm_base, &wm_base_listener, d);
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
        d->seat = (wl_seat *)wl_registry_bind(registry, name, &wl_seat_interface, version);
        wl_seat_add_listener(d->seat, &seat_listener, d);
    }
}

static void registry_remove_handler(void *, struct wl_registry *, uint32_t) {
    
}
