#pragma once

#include <reimu/core/event.h>
#include <reimu/core/result.h>
#include <reimu/graphics/renderer.h>
#include <reimu/graphics/vector.h>
#include <reimu/video/input.h>

#include <string>
#include <queue>

#ifdef REIMU_WIN32

#include <windows.h>

#endif

namespace reimu::video {

enum class WindowOptions {
    NoDecoration,
    Resizable
};

enum class NativeHandleType {
    Wayland,
    Win32
};

struct WaylandNativeHandle {
    struct wl_surface *surface;
    struct wl_display *display;
};

struct Win32NativeHandle {
    void *hinstance;
    void *hwnd;
};

struct NativeWindowHandle {
    NativeHandleType type;
    union {
        WaylandNativeHandle wayland;
        Win32NativeHandle win32;
    };
};

class Window : public EventDispatcher {
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

    static Result<Window *, ReimuError> create(const Vector2i &m_size);
    virtual ~Window();

    /**
     * @brief Set the size of the window
     * 
     * @param size 
     */
    virtual void set_size(const Vector2i &size);

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

    virtual void render() = 0;

    virtual void show_window() = 0;
    virtual void hide_window() = 0;
    virtual void sync_window() = 0;

    /**
     * @brief Begin an interactive move of the window
     */
    virtual void begin_move() = 0;

    /**
     * @brief Get the native handle for the Window object
     * 
     * Guaranteed to be valid for the lifetime of the Window. 
     * Returned returned in a struct that must be freed by the caller.
     * 
     * @return NativeWindowHandle
     */
    virtual Result<NativeWindowHandle *, ReimuError> get_native_handle() = 0;

    inline void set_renderer(graphics::Renderer *r) {
        m_renderer = r;
    }

    void queue_input_event(InputEvent ev) {
        m_input_queue.push(ev);
    }
    
    bool has_event() const {
        return !m_input_queue.empty();
    }

    bool get_event(InputEvent &ev) {
        if (m_input_queue.empty()) {
            return false;
        }

        ev = m_input_queue.front();
        m_input_queue.pop();
    
        return true;
    }

protected:
    graphics::Renderer *m_renderer = nullptr;
    std::queue<InputEvent> m_input_queue;
};

}
