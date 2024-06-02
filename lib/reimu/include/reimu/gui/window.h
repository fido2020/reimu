#pragma once

#include <reimu/core/error.h>
#include <reimu/gui/widget.h>
#include <reimu/video/window.h>

#include <memory>

namespace reimu::gui {

class Compositor;

class Window {
public:
    static Result<Window *, ReimuError> create(const Vector2i &size);

    void set_size(const Vector2i &size);
    void set_title(const std::string &title);

    void render();

    RootContainer &root() { return *m_root; }

    bool is_open() const { return m_is_open; }
    void close() { m_is_open = false; }

    const Vector2f &pointer() const { return m_pointer; }

private:
    Window(video::Window *window, graphics::Renderer *renderer);

    void process_input();

    bool m_is_open = true;

    CreateTextureFn m_create_texture_fn;

    std::unique_ptr<RootContainer> m_root;
    std::unique_ptr<video::Window> m_raw_window;
    std::unique_ptr<graphics::Renderer> m_renderer;

    std::unique_ptr<Compositor> m_compositor;

    Vector2f m_pointer = {0, 0};
};

}
