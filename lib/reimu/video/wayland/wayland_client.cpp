#include <reimu/core/logger.h>
#include <reimu/core/result.h>
#include <reimu/graphics/vector.h>
#include <reimu/video/driver.h>

#include <linux/input-event-codes.h>
#include <sys/mman.h>

#include <wayland-client.h>
#include <wayland-cursor.h>

#include <assert.h>
#include <string.h>

#include <xkbcommon/xkbcommon.h>

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

static void keyboard_keymap(void *data, struct wl_keyboard *keyboard, uint32_t format,
        int32_t fd, uint32_t size);
static void keyboard_enter(void *data, struct wl_keyboard *keyboard, uint32_t serial,
        struct wl_surface *surface, struct wl_array *keys);
static void keyboard_leave(void *data, struct wl_keyboard *keyboard, uint32_t serial,
        struct wl_surface *surface);
static void keyboard_key(void *data, struct wl_keyboard *keyboard, uint32_t serial,
        uint32_t time, uint32_t key, uint32_t state);
static void keyboard_modifiers(void *data, struct wl_keyboard *keyboard, uint32_t serial,
        uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked,
        uint32_t group);
static void keyboard_repeat_info(void *data, struct wl_keyboard *keyboard, int32_t rate,
        int32_t delay);

static void output_geometry(void *data, struct wl_output *wl_output, int32_t x, int32_t y,
        int32_t physical_width, int32_t physical_height, int32_t subpixel, const char *make,
        const char *model, int32_t transform);
static void output_mode(void *data, struct wl_output *wl_output, uint32_t flags, int32_t width,
        int32_t height, int32_t refresh);
static void output_done(void *data, struct wl_output *wl_output);

static void output_scale(void *data, struct wl_output *wl_output, int32_t factor);

static void output_name(void *data, struct wl_output *wl_output, const char *name);

static void output_description(void *data, struct wl_output *wl_output, const char *description);

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

static const wl_keyboard_listener keyboard_listener = {
    .keymap = keyboard_keymap,
    .enter = keyboard_enter,
    .leave = keyboard_leave,
    .key = keyboard_key,
    .modifiers = keyboard_modifiers,
    .repeat_info = keyboard_repeat_info
};

static const wl_output_listener output_listener = {
    .geometry = output_geometry,
    .mode = output_mode,
    .done = output_done,
    .scale = output_scale,
    .name = output_name,
    .description = output_description
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

    wl_surface_commit(surface);

    wl_display_roundtrip(display);

    win->set_size(size);

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

reimu::Vector2u WaylandDriver::get_display_size() {
    return display_sizes.front();
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

    if (!(d->compositor && d->wm_base && d->shm)) {
        d->finish();
        delete d;

        reimu::logger::fatal("Failed to create wayland bindings");
    }

    if (d->seat) {
        auto *cursor_theme = wl_cursor_theme_load(nullptr, 24, d->shm);
        assert(cursor_theme);

        auto *cursor = wl_cursor_theme_get_cursor(cursor_theme, "left_ptr");
        assert(cursor);

        auto *cursor_buffer = wl_cursor_image_get_buffer(cursor->images[0]);
        assert(cursor_buffer);

        auto *cursor_surface = wl_compositor_create_surface(d->compositor);
        assert(cursor_surface);

        wl_surface_attach(cursor_surface, cursor_buffer, 0, 0);
        wl_surface_commit(cursor_surface);

        d->cursor_theme = cursor_theme;
        d->cursor = cursor;
        d->cursor_buffer = cursor_buffer;
        d->cursor_surface = cursor_surface;
    }

    d->xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    assert(d->xkb_context);

    wl_display_roundtrip(display);

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

static void xdg_toplevel_configure(void *data, xdg_toplevel *, int32_t width,
        int32_t height, struct wl_array *) {
    reimu::logger::debug("width: {}, height: {}", width, height);

    if (width == 0 || height == 0) {
        return;
    }

    auto *win = (WaylandWindow *)data;

    win->set_size({width, height});
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

        wl_keyboard_add_listener(d->keyboard, &keyboard_listener, d);
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

    d->mouse_window = win;
    d->mouse_event.is_enter = true;
    d->mouse_serial = serial;

    d->mouse_event.pos = {
        static_cast<float>(wl_fixed_to_double(x)),
        static_cast<float>(wl_fixed_to_double(y))
    };

    // Set our cursor image, otherwise the cursor may be misaligned,
    // or the image may be say a text edit or resize cursor
    wl_pointer_set_cursor(pointer, serial, d->cursor_surface, d->cursor->images[0]->hotspot_x,
        d->cursor->images[0]->hotspot_y);
}

static void pointer_leave(void *data, struct wl_pointer *pointer, uint32_t serial,
        struct wl_surface *surface) {
    auto *d = (WaylandDriver *)data;

    d->mouse_event.is_leave = true;
    d->mouse_serial = serial;
}

static void pointer_move(void *data, struct wl_pointer *pointer, uint32_t serial,
        wl_fixed_t x, wl_fixed_t y) {
    auto *d = (WaylandDriver *)data;

    d->mouse_event.is_move = true;
    d->mouse_serial = serial;

    d->mouse_event.pos = {
        static_cast<float>(wl_fixed_to_double(x)),
        static_cast<float>(wl_fixed_to_double(y))
    };
}

static void pointer_button(void *data, struct wl_pointer *pointer, uint32_t serial,
        uint32_t time, uint32_t button, uint32_t state) {
    auto *d = (WaylandDriver *)data;

    d->mouse_event.is_button = true;
    d->mouse_serial = serial;
    
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

    d->mouse_event = {};
}

static void keyboard_keymap(void *data, struct wl_keyboard *keyboard, uint32_t format,
        int32_t fd, uint32_t size) {
    auto *d = (WaylandDriver *)data;

    assert(format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1);
    struct my_state *state = (struct my_state *)data;

    char *map_shm = (char *)mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
    assert(map_shm != MAP_FAILED);

    struct xkb_keymap *keymap = xkb_keymap_new_from_string( d->xkb_context, map_shm,
        XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);

    d->xkb_state = xkb_state_new(keymap);

    munmap(map_shm, size);
    close(fd);
}

static void keyboard_enter(void *data, struct wl_keyboard *keyboard, uint32_t serial,
        struct wl_surface *surface, struct wl_array *keys) {
    auto *d = (WaylandDriver *)data;

    auto *win = (WaylandWindow *)wl_surface_get_user_data(surface);
    d->keyboard_window = win;
}

static void keyboard_leave(void *data, struct wl_keyboard *keyboard, uint32_t serial,
        struct wl_surface *surface) {
    auto *d = (WaylandDriver *)data;

    d->keyboard_window = nullptr;
}

static uint32_t xkb_keysym_to_reimu_keycode(xkb_keysym_t key) {
    switch (key) {
        case XKB_KEY_BackSpace:
            return reimu::video::Key::Backspace;
        case XKB_KEY_Return:
            return reimu::video::Key::Return;
        case XKB_KEY_Tab:
            return reimu::video::Key::Tab;
        case XKB_KEY_Escape:
            return reimu::video::Key::Escape;
        case XKB_KEY_F1:
            return reimu::video::Key::F1;
        case XKB_KEY_F2:
            return reimu::video::Key::F2;
        case XKB_KEY_F3:
            return reimu::video::Key::F3;
        case XKB_KEY_F4:
            return reimu::video::Key::F4;
        case XKB_KEY_F5:
            return reimu::video::Key::F5;
        case XKB_KEY_F6:
            return reimu::video::Key::F6;
        case XKB_KEY_F7:
            return reimu::video::Key::F7;
        case XKB_KEY_F8:
            return reimu::video::Key::F8;
        case XKB_KEY_F9:
            return reimu::video::Key::F9;
        case XKB_KEY_F10:
            return reimu::video::Key::F10;
        case XKB_KEY_F11:
            return reimu::video::Key::F11;
        case XKB_KEY_F12:
            return reimu::video::Key::F12;
        case XKB_KEY_Print:
            return reimu::video::Key::PrintScreen;
        case XKB_KEY_Scroll_Lock:
            return reimu::video::Key::ScrollLock;
        case XKB_KEY_Pause:
            return reimu::video::Key::Pause;
        case XKB_KEY_Insert:
            return reimu::video::Key::Insert;
        case XKB_KEY_Home:
            return reimu::video::Key::Home;
        case XKB_KEY_Page_Up:
            return reimu::video::Key::PageUp;
        case XKB_KEY_Delete:
            return reimu::video::Key::Delete;
        case XKB_KEY_End:
            return reimu::video::Key::End;
        case XKB_KEY_Page_Down:
            return reimu::video::Key::PageDown;
        case XKB_KEY_Right:
            return reimu::video::Key::Right;
        case XKB_KEY_Left:
            return reimu::video::Key::Left;
        case XKB_KEY_Down:
            return reimu::video::Key::Down;
        case XKB_KEY_Up:
            return reimu::video::Key::Up;
        case XKB_KEY_Num_Lock:
            return reimu::video::Key::NumLock;
        default:
            if (key < 0x100)
                return key;
            return 0;
    };
}

static void keyboard_key(void *data, struct wl_keyboard *keyboard, uint32_t serial,
        uint32_t time, uint32_t key, uint32_t state) {
    auto *d = (WaylandDriver *)data;

    // Get the key code and build an input event
    auto *win = d->keyboard_window;
    if (win && state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        // To turn an evdev code into an xkb code, we need to add 8???
        auto xkb_key = xkb_state_key_get_one_sym(d->xkb_state, key + 8);

        uint32_t key = xkb_keysym_to_reimu_keycode(xkb_key);

        d->keyboard_event.key = key;

        win->queue_input_event({
            .type = reimu::video::InputEvent::Keyboard,
            .key = d->keyboard_event
        });
    }
}

static void keyboard_modifiers(void *data, struct wl_keyboard *keyboard, uint32_t serial,
        uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked,
        uint32_t group) {
    auto *d = (WaylandDriver *)data;

    xkb_state_update_mask(d->xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, group);

    // Get which modifiers are pressed from XKB and build an input event
    auto &ev = d->keyboard_event;

    ev.is_ctrl = xkb_state_mod_name_is_active(d->xkb_state, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_DEPRESSED) == 1;
    ev.is_shift = xkb_state_mod_name_is_active(d->xkb_state, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_DEPRESSED) == 1;
    ev.is_alt = xkb_state_mod_name_is_active(d->xkb_state, XKB_MOD_NAME_ALT, XKB_STATE_MODS_DEPRESSED) == 1;
    ev.is_win = xkb_state_mod_name_is_active(d->xkb_state, XKB_MOD_NAME_LOGO, XKB_STATE_MODS_DEPRESSED) == 1; 
}

static void keyboard_repeat_info(void *data, struct wl_keyboard *keyboard, int32_t rate,
        int32_t delay) {
    auto *d = (WaylandDriver *)data;
}

static reimu::Vector2u output_size;

static void output_geometry(void *data, struct wl_output *wl_output, int32_t x, int32_t y,
        int32_t physical_width, int32_t physical_height, int32_t subpixel, const char *make,
        const char *model, int32_t transform) {
    
}

static void output_mode(void *data, struct wl_output *wl_output, uint32_t flags, int32_t width,
        int32_t height, int32_t refresh) {
    reimu::logger::debug("output_mode: {}x{}@{}", width, height, refresh);

    output_size = {(uint32_t)width, (uint32_t)height};
}

static void output_done(void *data, struct wl_output *wl_output) {
    auto *d = (WaylandDriver *)data;

    d->display_sizes.push_back(output_size);
}

static void output_scale(void *data, struct wl_output *wl_output, int32_t factor) {
    
}

static void output_name(void *data, struct wl_output *wl_output, const char *name) {
    
}

static void output_description(void *data, struct wl_output *wl_output, const char *description) {
    
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
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        d->shm = (wl_shm *)wl_registry_bind(registry, name, &wl_shm_interface, version);
    } else if (strcmp(interface, wl_output_interface.name) == 0) {
        d->output = (wl_output *)wl_registry_bind(registry, name, &wl_output_interface, version);
        wl_output_add_listener(d->output, &output_listener, d);
    }
}

static void registry_remove_handler(void *, struct wl_registry *, uint32_t) {
    
}
