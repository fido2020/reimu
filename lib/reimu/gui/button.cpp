#include <reimu/gui/widget.h>

namespace reimu::gui {

Button::Button() {
    bind_event_callback("on_mouse_down"_hashid, [this]() {
        is_pressed = true;

        dispatch_event("ui_repaint"_hashid);
    });

    bind_event_callback("on_mouse_up"_hashid, [this]() {
        if (is_pressed) {
            dispatch_event("on_click"_hashid);

            is_pressed = false;
            dispatch_event("ui_repaint"_hashid);
        }
    });

    bind_event_callback("on_mouse_leave"_hashid, [this]() {
        is_pressed = false;
        dispatch_event("ui_repaint"_hashid);
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
