#include <reimu/gui/widget.h>
#include <cfloat>

namespace reimu::gui {

void Label::repaint(UIPainter &painter) {
    Widget::repaint(painter);
    
    graphics::Painter p{ *m_surface };
    p.clear_rect({0, 0, (float)m_surface->size().x, (float)m_surface->size().y});

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
