#pragma once

#include <reimu/video/window.h>

#include <EGL/egl.h>

#include <wayland-egl.h>

#include "driver.h"
#include "xdg-shell-client-protocol.h"

struct wl_surface;
struct xdg_surface;
struct xdg_toplevel;
struct wl_egl_window;

class WaylandWindow final : public reimu::video::Window {
public:
    WaylandWindow(WaylandDriver &driver, wl_surface *surface, xdg_surface *xdg_surface,
            xdg_toplevel *xdg_toplevel) : driver(driver), surface(surface),
            xdg_surface(xdg_surface), xdg_toplevel(xdg_toplevel) {}

    void set_size(const reimu::Vector2i &size) override {
        this->size = size;

        xdg_surface_set_window_geometry(xdg_surface, 0, 0, size.x, size.y);

        wl_surface_damage_buffer(surface, 0, 0, size.x, size.y);

        sync_window();
    }

    void set_title(const std::string &title) override {
        xdg_toplevel_set_title(xdg_toplevel, title.c_str());

        sync_window();
    }

    reimu::Vector2i get_size() const override {
        return size;
    }

    void show_window() override {

    }

    void hide_window() override {
        xdg_toplevel_set_minimized(xdg_toplevel);
    }

    WaylandDriver &driver;

    wl_surface *surface;
    xdg_surface *xdg_surface;
    xdg_toplevel *xdg_toplevel;

    reimu::Vector2i size;

private:
    void sync_window() {
        driver.display_roundtrip();
    }
};
