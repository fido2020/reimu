#pragma once

#include <reimu/graphics/color.h>
#include <reimu/graphics/rect.h>

#include <string>

namespace reimu::graphics {

class Painter {
public:
    Painter &draw_rect_filled(const Rectf &rect, const Color &color);

    Painter &draw_rect_outline(const Rectf &rect, const Color &color, int thickness);
    Painter &draw_line(const Vector2f &begin, const Vector2f &end, const Color &color, int thickness);

    Painter &draw_text(const std::string &text, const Vector2f &pos, const Color &color);
};

}
