#pragma once

#include <reimu/video/window.h>

#include <EGL/egl.h>

#include <wayland-egl.h>

#include "xdg-shell-client-protocol.h"

struct wl_surface;
struct xdg_surface;
struct xdg_toplevel;
struct wl_egl_window;

class WaylandWindow final : public reimu::video::Window {
public:
    WaylandWindow(wl_surface *surface, xdg_surface *xdg_surface, xdg_toplevel *xdg_toplevel,
            wl_egl_window *egl_window, EGLSurface egl_surface)
            : surface(surface), xdg_surface(xdg_surface), xdg_toplevel(xdg_toplevel),
            egl_window(egl_window), egl_surface(egl_surface) {}

    void set_size(const reimu::Vector2i &size) override {
        this->size = size;

        wl_egl_window_resize(egl_window, size.x, size.y, 0, 0);

        xdg_surface_set_window_geometry(xdg_surface, 0, 0, size.x, size.y);

        wl_surface_damage_buffer(surface, 0, 0, size.x, size.y);
    }

    void set_pos(const reimu::Vector2i &pos) override {
    }

    void set_title(const std::string &title) override {
        xdg_toplevel_set_title(xdg_toplevel, title.c_str());
    }

    reimu::Vector2u get_size() const override {

    }

    reimu::Vector2u get_pos() const override {

    }

    void show_window() override {

    }

    void hide_window() override {
        xdg_toplevel_set_minimized(xdg_toplevel);
    }

    wl_surface *surface;
    xdg_surface *xdg_surface;
    xdg_toplevel *xdg_toplevel;

    wl_egl_window *egl_window;
    EGLSurface egl_surface;

    reimu::Vector2i size;
};
