#pragma once

#include <reimu/core/error.h>
#include <reimu/core/resource_manager.h>
#include <reimu/gui/widget.h>
#include <reimu/video/window.h>
#include <reimu/video/input.h>

#include <memory>

namespace reimu::gui {

class Compositor;

class Window {
public:
    static Result<Window *, ReimuError> create(const Vector2i &size);
    ~Window();

    /**
     * @brief Resize the window
     * 
     * @param size 
     */
    void set_size(const Vector2i &size);

    /**
     * @brief Set the title of the window
     * 
     * @param title 
     */
    void set_title(const std::string &title);

    void render();

    /**
     * @brief Get the root widget
     * 
     * @return RootContainer& 
     */
    RootContainer &root() { return *m_root; }

    bool is_open() const { return m_is_open; }
    void close() { m_is_open = false; }

    /**
     * @brief Let the user drag the window
     */
    void begin_move() { m_raw_window->begin_move(); }

    /**
     * @brief Get the last known position of the mouse cursor
     */
    const Vector2f &pointer() const { return m_pointer; }

    /**
     * @brief Run the window event loop, until the window is closed
     */
    void run_until_close();

    void set_is_decorated(bool is_decorated);

    Widget *get_focused_widget();
    void set_focused_widget(Widget *widget);

    video::InputEvent get_last_input_event() const {
        return m_last_input_event;
    }

    std::shared_ptr<ResourceManager> resource_manager() {
        return m_res_mgr;
    }

private:
    Window(video::Window *window, graphics::Renderer *renderer);

    void process_input();

    void set_mouse_widget(Widget *widget);

    bool m_is_open = true;

    CreateTextureFn m_create_texture_fn;

    Widget *m_mouse_widget = nullptr;
    Widget *m_focused_widget = nullptr;

    std::unique_ptr<RootContainer> m_root;
    std::unique_ptr<video::Window> m_raw_window;
    std::unique_ptr<graphics::Renderer> m_renderer;

    std::unique_ptr<Compositor> m_compositor;

    std::shared_ptr<ResourceManager> m_res_mgr;

    Vector2f m_pointer = {0, 0};

    video::InputEvent m_last_input_event;
};

}
