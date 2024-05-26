#include <reimu/graphics/painter.h>

#include <reimu/core/logger.h>

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
    uint8_t *buffer = m_surface.buffer() + clipped_rect.y * stride + clipped_rect.x * 4;
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

Painter &Painter::draw_rect_gradient(const Rectf &rect, const Color &c1, const Color &c2,
        const Vector2f &p1, const Vector2f &p2) {
    Recti clipped_rect = get_visible_rect(m_surface, rect);

    if (clipped_rect.width() <= 0 || clipped_rect.height() <= 0) {
        return *this;
    }

    auto length = (p2 - p1).magnitude();
    auto normalized = (p2 - p1).normalize();

    float dy = normalized.y / length;
    float dx = normalized.x / length;

    uint8_t *row = m_surface.buffer() + m_surface.stride() * clipped_rect.y;

    float y = clipped_rect.y / length;
    uint32_t rows = clipped_rect.height();
    while (rows--) {
        uint32_t *pixel = ((uint32_t*)row) + clipped_rect.x;

        float blend = y + clipped_rect.x / length;
        uint32_t cols = clipped_rect.width();
        while (cols--) {
            uint16_t blend_factor = std::clamp<uint16_t>(static_cast<uint16_t>(blend * 255), 0, 255);
            uint16_t one_minus = 255 - blend_factor;

            // Split up red-blue, alpha-green
            uint32_t rb = (c1.value & 0x00ff00ff) * one_minus + (c2.value & 0x00ff00ff) * blend_factor;
            uint32_t ag = ((c1.value & 0xff00ff00) >> 8) * one_minus + ((c2.value & 0xff00ff00) >> 8) * blend_factor;

            *(pixel++) = ((rb & 0xff00ff00) >> 8) | ((ag & 0xff00ff00));
            blend += dx;
        }

        row += m_surface.stride();
        y += dy;
    }

    return *this;
}

Painter &Painter::draw_rect_outline(const Rectf &rect, const Color &color, int thickness) {
    return *this;
}

Painter &Painter::draw_line(const Vector2f &begin, const Vector2f &end, const Color &color, int thickness) {
    return *this;
}

}
