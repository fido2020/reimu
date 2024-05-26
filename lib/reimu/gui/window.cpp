#include <reimu/gui/window.h>

#include "compositor.h"

namespace reimu::gui {

Result<Window *, ReimuError> Window::create(const Vector2i &size) {
    auto window = TRY(video::Window::create(size));
    auto renderer = TRY(graphics::create_attach_renderer(window));

    return OK(new Window{ window, renderer });
}

Window::Window(video::Window *window, graphics::Renderer *renderer)
        : m_raw_window(window), m_renderer(renderer) {
    // Set up callback so Widgets can create textures
    m_create_texture_fn = [this](const Vector2i &size) -> graphics::Texture * {
        return m_renderer->create_texture(size, m_renderer->display_surface_color_format())
            .ensure();
    };

    m_compositor = std::make_unique<Compositor>(renderer);

    m_root = std::make_unique<RootContainer>(vector_static_cast<float>(renderer->get_viewport_size()));
    m_root->create_texture_if_needed(m_create_texture_fn);
}

void Window::render() {
    if (m_root->needs_layout_update()) {
        m_root->update_layout(vector_static_cast<float>(m_renderer->get_viewport_size()));
        
        DefaultUIPainter painter{};
        m_root->repaint(painter);

        m_root->add_clips(m_compositor->get_add_clip_fn());
    }

    m_renderer->render();
}

void Window::set_size(const Vector2i &size) {
    m_raw_window->set_size(size);
}

void Window::set_title(const std::string &title) {
    m_raw_window->set_title(title);
}

}
