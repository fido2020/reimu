#pragma once

#include <reimu/core/result.h>
#include <reimu/graphics/vector.h>

#include <string>

namespace reimu::video {

enum class WindowOptions {
    NoDecoration,
    Resizable
};

enum class NativeHandleType {
    Wayland
};

struct WaylandNativeHandle {
    struct wl_surface *surface;
    struct wl_display *display;
};

struct NativeWindowHandle {
    NativeHandleType type;
    union {
        WaylandNativeHandle wayland;
    };
};

class Window {
public:
    class Builder {
    public:
        Builder &set_size(const Vector2i &size) {
            m_size = size;

            return *this;
        }

        Builder &set_pos(const Vector2i &pos) {
            m_pos = pos;
            m_has_pos = true;

            return *this;
        }

        Builder &set_title(std::string title) {
            m_title = std::move(title);

            return *this;
        }

        Builder &set_options(WindowOptions opt) {
            m_opt = opt;

            return *this;
        }

        Result<Window *, ReimuError> create() {
            auto win = TRY(Window::create(m_size));

            win->set_title(m_title);

            return OK(win);
        }

    private:
        std::string m_title = "Window";

        Vector2i m_pos = {0, 0};
        bool m_has_pos = false;

        Vector2i m_size = {640, 480};
        WindowOptions m_opt;
    };

    static Builder make(const Vector2i &m_size) {
        return Builder{}.set_size(m_size);
    }

    virtual ~Window();

    /**
     * @brief Set the size of the window
     * 
     * @param size 
     */
    virtual void set_size(const Vector2i &size) = 0;

    /**
     * @brief Set the title of the window
     * 
     * @param title 
     */
    virtual void set_title(const std::string &title) = 0;

    /**
     * @brief Get the size of the window
     * 
     * @return Vector2i 
     */
    virtual Vector2i get_size() const = 0;

    virtual void show_window() = 0;
    virtual void hide_window() = 0;
    virtual void sync_window() = 0;

    /**
     * @brief Get the native handle for the Window object
     * 
     * Guaranteed to be valid for the lifetime of the Window. 
     * Returned returned in a struct that must be freed by the caller.
     * 
     * @return NativeWindowHandle
     */
    virtual Result<NativeWindowHandle *, ReimuError> get_native_handle() = 0;

private:
    static Result<Window *, ReimuError> create(const Vector2i &m_size);
};

}
