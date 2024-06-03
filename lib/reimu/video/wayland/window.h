#pragma once

#include <reimu/video/window.h>

#include <EGL/egl.h>

#include <wayland-egl.h>

#include "driver.h"
#include "xdg-shell-client-protocol.h"

class WaylandWindow final : public reimu::video::Window {
public:
    WaylandWindow(WaylandDriver &driver, wl_surface *surface, xdg_surface *xdg_surface,
            xdg_toplevel *xdg_toplevel, const reimu::Vector2i &size) : driver(driver),
            surface(surface), xdg_surface(xdg_surface), xdg_toplevel(xdg_toplevel), size(size) {}

    ~WaylandWindow() {
        driver.windows.remove(this);
    }

    void set_size(const reimu::Vector2i &size) override {
        this->size = size;

        xdg_surface_set_window_geometry(xdg_surface, 0, 0, size.x, size.y);

        wl_surface_damage_buffer(surface, 0, 0, size.x, size.y);

        sync_window();

        Window::set_size(size);
    }

    void set_title(const std::string &title) override {
        xdg_toplevel_set_title(xdg_toplevel, title.c_str());

        sync_window();
    }

    reimu::Vector2i get_size() const override {
        return size;
    }

    void render() override {
        if (m_renderer) {
            m_renderer->render();
        }
    }

    void show_window() override {

    }

    void hide_window() override {
        xdg_toplevel_set_minimized(xdg_toplevel);
    }

    void sync_window() override {
        driver.display_roundtrip();
    }

    reimu::Result<reimu::video::NativeWindowHandle *, reimu::ReimuError> get_native_handle()
            override {
        return OK(new reimu::video::NativeWindowHandle {
                reimu::video::NativeHandleType::Wayland, {
                .wayland = {
                    .surface = surface,
                    .display = driver.display
                }
            }
        });
    }

    WaylandDriver &driver;

    struct wl_surface *surface;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;

    reimu::Vector2i size;
};
