#include <reimu/gui/widget.h>

namespace reimu::gui {

void Label::repaint(UIPainter &painter) {
    Widget::repaint(painter);
    
    graphics::Painter p{ *m_surface };

    painter.begin(p);

    painter.draw_label(m_text);

    painter.end();
}

void Label::set_text(const std::string &text) {
    m_text = text;

    if (m_parent) {
        m_parent->dispatch_event("ui_repaint"_hashid);
    }
}

}
