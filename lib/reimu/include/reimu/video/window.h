#pragma once

#include <reimu/core/result.h>
#include <reimu/graphics/vector.h>

#include <string>

namespace reimu::video {

enum class WindowOptions {
    NoDecoration,
    Resizable
};

class Window {
public:
    class WindowBuilder {
    public:
        WindowBuilder &set_size(const Vector2i &size) {
            m_size = size;

            return *this;
        }

        WindowBuilder &set_pos(const Vector2i &pos) {
            m_pos = pos;
            m_has_pos = true;

            return *this;
        }

        WindowBuilder &set_title(std::string title) {
            m_title = std::move(title);

            return *this;
        }

        WindowBuilder &set_options(WindowOptions opt) {
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

    static WindowBuilder make(const Vector2i &m_size) {
        return WindowBuilder{}.set_size(m_size);
    }

    static Result<Window *, ReimuError> create(const Vector2i &m_size);

    virtual ~Window();

    virtual void set_size(const Vector2i &size) = 0;
    virtual void set_title(const std::string &title) = 0;

    virtual Vector2i get_size() const = 0;

    virtual void show_window() = 0;
    virtual void hide_window() = 0;
};

}
