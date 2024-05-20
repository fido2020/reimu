#include <reimu/gui/widget.h>

namespace reimu::gui {

void Button::repaint(UIPainter &painter) {
    Widget::repaint(painter);

    //painter.draw_button(m_surface->texture(), label, is_pressed);
    graphics::Painter p(*m_surface);

    p.draw_rect({ 0, 0, bounds.width(), bounds.height() }, { 128, 128, 128, 255 });
}

}
