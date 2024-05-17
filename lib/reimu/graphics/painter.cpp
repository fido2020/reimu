#include <reimu/graphics/painter.h>

namespace reimu::graphics {

/**
 * Get the intersection between the rect given and the surface bounds
*/
static Recti get_visible_rect(const Surface &surface, const Rectf &rect) {
    return Recti::from_size({0, 0}, surface.size())
        .intersect((Recti)vector_static_cast<int>((Vector4f)rect));
}

Painter::Painter(Surface &surface)
    : m_surface(surface) {}

Painter &Painter::draw_rect(const Rectf &rect, const Color &color) {
    Recti clipped_rect = get_visible_rect(m_surface, rect);

    if (clipped_rect.width() <= 0 || clipped_rect.height() <= 0) {
        return *this;
    }

    // TODO: Consider vectorising this
    uint32_t stride = m_surface.stride();
    uint8_t *buffer = m_surface.buffer() + clipped_rect.y * stride;
    uint32_t pixel_value = color.value;
    
    size_t rows = clipped_rect.height();
    size_t pixels_per_row = clipped_rect.width();
    while (rows--) {
        for (uint32_t i = 0; i < pixels_per_row; i++) {
            reinterpret_cast<uint32_t*>(buffer)[i] = pixel_value;
        }

        buffer += stride;
    }

    return *this;
}

Painter &Painter::draw_rect_outline(const Rectf &rect, const Color &color, int thickness) {

}

Painter &Painter::draw_line(const Vector2f &begin, const Vector2f &end, const Color &color, int thickness) {

}

}
