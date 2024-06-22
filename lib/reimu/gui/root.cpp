#include <reimu/gui/widget.h>

#include <reimu/gui/window.h>

namespace reimu::gui {

RootContainer::RootContainer(Window *win, const Vector2f &viewport_size, bool decorate)
    : m_viewport_size(viewport_size), m_decorate(decorate) {
    m_window = win;

    if (m_decorate) {
        auto *close_button = new Button;
        close_button->layout.width = Size::from_pixels(18);
        close_button->layout.height = Size::from_pixels(18);
        close_button->layout.position = LayoutPositioning::Absolute;
        add_child(close_button);

        close_button->bind_event_callback("on_click"_hashid, [this]() {
            if (m_window)
                m_window->close();
        });

        m_window_controls.push_back(std::unique_ptr<Widget>(close_button));
    }

    bind_event_callback("ui_repaint"_hashid, [this]() {
        m_repaint = true;
    });

    bind_event_callback("on_mouse_down"_hashid, [this]() {
        // TODO: use the Style object
        auto titlebar_rect = Rectf{1 + 3, 1 + 3, m_viewport_size.x - 1 - 3, (float)24 + 1 + 3};
        if (titlebar_rect.contains(m_window->pointer())) {
            m_window->begin_move();
        }
    });
}

void RootContainer::repaint(UIPainter &painter) {
    m_repaint = false;

    Box::repaint(painter);

    if (m_decorate) {
        graphics::Painter p{ *m_surface };

        painter.begin(p);
        painter.draw_frame(window_title, true);
        painter.end();

        for (auto &control : m_window_controls) {
            control->repaint(painter);
        }
    }
}

void RootContainer::update_layout(const Vector2f &viewport_size) {
    m_recalculate_layout = false;

    calculated_layout.font_size = 16;
    calculated_layout.root_font_size = 16;
    calculated_layout.left_padding = 0;
    calculated_layout.right_padding = 0;
    calculated_layout.top_padding = 0;
    calculated_layout.bottom_padding = 0;

    calculated_layout.inner_size = m_viewport_size -
        Vector2f { calculated_layout.left_padding + calculated_layout.right_padding,
            calculated_layout.top_padding + calculated_layout.bottom_padding };

    if (m_decorate) {
        // TODO: use Style object
        calculated_layout.inner_size -= {6, 6 + 24 + 2};
    }

    calculated_layout.outer_size = m_viewport_size;

    bounds = {0, 0, viewport_size.x, viewport_size.y};

    FlowBox::update_layout();

    if (m_decorate) {
        float xpos = viewport_size.x - 8;
        float ypos = 6;
        for (auto &control : m_window_controls) {
            float new_xpos = xpos - control->calculated_layout.outer_size.x;
            control->bounds = {new_xpos, ypos,
                xpos, ypos + control->calculated_layout.inner_size.y};

            xpos = new_xpos - 2;
        }
    }
}

void RootContainer::signal_layout_changed() {
    m_recalculate_layout = true;
}

Rectf RootContainer::inner_bounds() const {
    if (m_decorate) {
        // TODO: use Style object
        return {3, 3 + 24 + 2,
            m_viewport_size.x - 3, m_viewport_size.y - 3};
    }
    
    return {0, 0, m_viewport_size.x, m_viewport_size.y};
}

}
