#include <reimu/gui/widget.h>

namespace reimu::gui {

RootContainer::RootContainer(const Vector2f &viewport_size)
    : m_viewport_size(viewport_size) {

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
    calculated_layout.outer_size = m_viewport_size;

    bounds = {0, 0, viewport_size.x, viewport_size.y};

    Box::update_layout();
}

void RootContainer::signal_layout_changed() {

}

}
