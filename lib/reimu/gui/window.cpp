#include <reimu/gui/window.h>

#include <reimu/video/driver.h>
#include <reimu/video/video.h>

#include "compositor.h"

namespace reimu::gui {

Result<Window *, ReimuError> Window::create(const Vector2i &size) {
    auto window = TRY(video::Window::create(size));
    auto renderer = TRY(graphics::create_attach_renderer(window));

    return OK(new Window{ window, renderer });
}

Window::~Window() {

}

Window::Window(video::Window *window, graphics::Renderer *renderer)
        : m_raw_window(window), m_renderer(renderer) {
    // Set up callback so Widgets can create textures
    m_create_texture_fn = [this](const Vector2i &size) -> graphics::Texture * {
        return m_renderer->create_texture(size, m_renderer->display_surface_color_format())
            .ensure();
    };

    m_raw_window->bind_event_callback("wm_close"_hashid, [this]() {
        close();
    });

    m_raw_window->bind_event_callback("wm_input"_hashid, [this]() {
        process_input();
        render();
    });

    m_compositor = std::make_unique<Compositor>(renderer);

    m_root = std::make_unique<RootContainer>(this,
        vector_static_cast<float>(renderer->get_viewport_size()));
    m_root->create_texture_if_needed(m_create_texture_fn);

    m_res_mgr = std::make_shared<ResourceManager>();
}

void Window::render() {
    DefaultUIPainter painter{*m_res_mgr};

    if (m_root->needs_layout_update()) {
        m_root->update_layout(vector_static_cast<float>(m_renderer->get_viewport_size()));
        
        m_root->repaint(painter);

        m_compositor->clear_clips();
        m_root->add_clips(m_compositor->get_add_clip_fn());
    } else if(m_root->needs_repaint()) {
        m_root->repaint(painter);
    }

    m_raw_window->render();
}

void Window::set_size(const Vector2i &size) {
    m_raw_window->set_size(size);
}

void Window::set_title(const std::string &title) {
    m_raw_window->set_title(title);
    m_root->window_title = title;
}

void Window::run_until_close() {
    auto event_loop = EventLoop::create().ensure();
    event_loop->watch_os_handle(video::get_driver()->get_window_client_handle(), [this, &event_loop]() {
        video::get_driver()->window_client_dispatch();

        if (!is_open()) {
            event_loop->end();
        }
    });

    event_loop->run();

    delete event_loop;
}

void Window::set_is_decorated(bool is_decorated) {
    m_root->set_is_decorated(is_decorated);
}

Widget *Window::get_focused_widget() {
    return m_focused_widget;
}

void Window::set_focused_widget(Widget *widget) {
    if (widget == m_focused_widget) {
        return;
    }

    if (m_focused_widget) {
        m_focused_widget->dispatch_event("on_focus_lost"_hashid);
    }

    m_focused_widget = widget;

    if (m_focused_widget) {
        m_focused_widget->dispatch_event("on_focus_gained"_hashid);
    }
}

void Window::process_input() {
    auto &event = m_last_input_event;
    while (m_raw_window->get_event(event)) {
        if (event.type == event.Mouse) {
            auto mouse = event.mouse;

            if (mouse.is_enter || mouse.is_move) {
                m_pointer = mouse.pos;
            }

            auto *widget = m_root->get_widget_at(m_pointer);
            if (mouse.is_leave) {
                set_mouse_widget(nullptr);
            } else if (widget) {
                set_mouse_widget(widget);

                if (mouse.is_move) {
                    widget->dispatch_event("on_mouse_move"_hashid);
                }

                if (mouse.is_button) {
                    switch(mouse.button) {
                    case video::MouseButton::Left:
                        if (mouse.state == video::MouseButtonState::Pressed) {
                            widget->dispatch_event("on_mouse_down"_hashid);

                            auto bounds = widget->bounds;
                        } else {
                            widget->dispatch_event("on_mouse_up"_hashid);
                        }
                    }
                }
            }

            // Regardless if the widget is null or not,
            // we still want to set the focused widget
            if (mouse.is_button && mouse.button == video::MouseButton::Left
                    && mouse.state == video::MouseButtonState::Pressed) {
                set_focused_widget(widget);
            }
        } else if (event.type == event.Keyboard) {
            auto key = event.key;
            if (key.is_down) {
                auto *widget = get_focused_widget();
                if (widget) {
                    widget->dispatch_event("on_key_down"_hashid);
                }
            }
        }
    }
}

void Window::set_mouse_widget(Widget *widget) {
    if (m_mouse_widget != widget) {
        if (m_mouse_widget) {
            m_mouse_widget->dispatch_event("on_mouse_leave"_hashid);
        }

        if (widget) {
            widget->dispatch_event("on_mouse_enter"_hashid);
        }
        
        m_mouse_widget = widget;
    }
}

}
