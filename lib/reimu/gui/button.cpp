#include <reimu/gui/widget.h>

namespace reimu::gui {

void Button::repaint(UIPainter &painter) {
    Widget::repaint(painter);

    graphics::Painter p{ *m_surface };

    painter.begin(p);

    painter.draw_button(label, is_pressed);

    painter.end();
}

}
