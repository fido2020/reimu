#include <reimu/gui/widget.h>

namespace reimu::gui {

Button::Button(CreateTexture create_texture)
    : Widget(create_texture) {
    m_surface = std::make_unique<graphics::Surface>(m_create_texture_fn({ 1, 1 }));
}

void Button::repaint(UIPainter &painter) {
    Widget::repaint(painter);

    //painter.draw_button(m_surface->texture(), label, is_pressed);
    graphics::Painter p(*m_surface);

    p.draw_rect({ 0, 0, bounds.width(), bounds.height() }, { 128, 128, 128, 255 });
}

}
