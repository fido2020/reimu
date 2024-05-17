#pragma once

#include <reimu/graphics/color.h>
#include <reimu/graphics/rect.h>
#include <reimu/graphics/surface.h>

#include <string>

namespace reimu::graphics {

class Painter {
public:
    Painter(Surface &surface);

    ~Painter() {
        m_surface.update();
    }

    Painter &draw_rect(const Rectf &rect, const Color &color);

    Painter &draw_rect_outline(const Rectf &rect, const Color &color, int thickness);
    Painter &draw_line(const Vector2f &begin, const Vector2f &end, const Color &color, int thickness);

private:
    Surface &m_surface;
};

}
