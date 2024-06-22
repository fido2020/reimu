#pragma once

#include <reimu/graphics/color.h>
#include <reimu/graphics/rect.h>
#include <reimu/graphics/surface.h>
#include <reimu/graphics/vector.h>
#include <reimu/graphics/font.h>

#include <memory>
#include <string>

namespace reimu::graphics {

class Text final {
public:
    Text();
    Text(std::u32string text);

    void set_text(std::u32string text);
    
    void set_font(std::shared_ptr<Font> font);
    void set_font_size_px(int size_px);

    void set_color(const Color& colour);

    void render(Surface &dest, const Rectf &bounds);

    Vector2f text_geometry();

private:
    std::shared_ptr<Font> m_font = nullptr;

    Color m_color = Color(0, 0, 0);
    std::u32string m_text; // Text to render
    Rectf m_bounds;       // Bounds of the text
    int m_pixel_size = 16;

    Vector2f m_text_size;
    bool m_stale_geometry = true;
};

} // namespace Arclight
