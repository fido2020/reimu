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

    inline Vector2i surface_size() const {
        return m_surface.size();
    }

    Painter &draw_rect(const Rectf &rect, const Color &color);

    /**
     * @brief Draw a rectangle with a gradient
     * 
     * @param rect Rectangle to draw on the Surface
     * @param c1 First color
     * @param c2 Second color
     * @param p1 Position of the first color
     * @param p2 Position of the second color
     * @return Painter& 
     */
    Painter &draw_rect_gradient(const Rectf &rect, const Color &c1, const Color &c2, const Vector2f &p1, const Vector2f &p2);

    Painter &draw_rect_outline(const Rectf &rect, const Color &color, int thickness);
    Painter &draw_line(const Vector2f &begin, const Vector2f &end, const Color &color, int thickness);

private:
    Surface &m_surface;
};

}
