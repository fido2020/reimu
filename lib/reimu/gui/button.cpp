#include <reimu/gui/widget.h>

namespace reimu::gui {

Button::Button() {
    bind_event_callback("on_mouse_down"_hashid, [this]() {
        is_pressed = true;

        // TODO: request repaint without a layout change
        signal_layout_changed();
    });

    bind_event_callback("on_mouse_up"_hashid, [this]() {
        if (is_pressed) {
            dispatch_event("on_click"_hashid);
        }

        is_pressed = false;
    });
}

void Button::repaint(UIPainter &painter) {
    Widget::repaint(painter);

    graphics::Painter p{ *m_surface };

    painter.begin(p);

    painter.draw_button(label, is_pressed);

    painter.end();
}

}
