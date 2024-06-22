#include <reimu/gui/style.h>

#include <reimu/core/unicode.h>

#include <assert.h>

namespace reimu::gui {

UIPainter::UIPainter() {}
 
void UIPainter::begin(graphics::Painter &painter) {
    m_current_painter = &painter;
}

void UIPainter::end() {
    m_current_painter = nullptr;
}

graphics::Painter &UIPainter::get_painter() {
    assert(m_current_painter);
    return *m_current_painter;
}

DefaultUIPainter::DefaultUIPainter() {
    m_style.background_color = Color(200, 200, 190);
    m_style.text_color = Color(0, 0, 0);
    m_style.highlight_color = Color(255, 255, 255);
    m_style.shadow_color = Color(0, 0, 0);
    m_style.win_border_thickness = 2;
    m_style.win_titlebar_height = 24;
}

void DefaultUIPainter::draw_frame(const std::string &title, bool is_active) {
    auto &painter = get_painter();
    auto &style = get_style();

    auto size = vector_static_cast<float>(painter.surface_size());

    auto titlebar_rect = Rectf{1, 1, size.x - 1, (float)style.win_titlebar_height + 1};

    if (m_style.win_border_thickness > 0) {
        float thickness = style.win_border_thickness;
        painter
            // Highlight
            .draw_rect({0, 0, size.x - 1, 1}, style.highlight_color)
            .draw_rect({0, 0, 1, size.y - 1}, style.highlight_color)
            // Shadow
            .draw_rect({size.x - 1, 0, size.x, size.y}, style.shadow_color)
            .draw_rect({0, size.y - 1, size.x, size.y}, style.shadow_color)
            // Border
            .draw_rect({1, 1, size.x - 1, 1 + thickness}, style.background_color)
            .draw_rect({1, 1, 1 + thickness, size.y - 1}, style.background_color)
            .draw_rect({size.x - thickness - 1, 1, size.x - 1, size.y - 1},
                style.background_color)
            .draw_rect({1, size.y - thickness - 1, size.x - 1, size.y - 1},
                style.background_color)
            .draw_rect({thickness, thickness + 1 + style.win_titlebar_height, size.x - 1,
                thickness + 1 + style.win_titlebar_height + 1}, style.background_color);

        titlebar_rect.x += thickness;
        titlebar_rect.z -= thickness * 2;

        titlebar_rect.y += thickness;
        titlebar_rect.w += thickness;
    }

    // Window titlebar gradient
    Color c1 = Color(192, 0, 100, 255);
    Color c2 = Color(64, 32, 128, 255);

    painter.draw_rect_gradient(titlebar_rect, c1, c2, {0.f, 0.f}, {titlebar_rect.width(), 500.f});

    m_text.set_text(to_utf32(title).ensure());
    m_text.set_font_size_px(16);
    m_text.set_color(Color(255, 255, 255));
    m_text.render(painter.surface(), {
        titlebar_rect.x + 2,
        titlebar_rect.y,
        titlebar_rect.z,
        titlebar_rect.w
    });
}

void DefaultUIPainter::draw_button(const std::string &label, bool is_pressed) {
    auto &painter = get_painter();
    auto &style = get_style();

    auto size = vector_static_cast<float>(painter.surface_size());

    auto button_rect = Rectf{0, 0, size.x, size.y};

    if (is_pressed) {
        painter.draw_rect(button_rect, style.background_color)
            .draw_rect({0, 0, size.x, 1}, style.shadow_color)
            .draw_rect({0, 0, 1, size.y}, style.shadow_color)
            .draw_rect({size.x - 1, 0, size.x, size.y}, style.shadow_color)
            .draw_rect({0, size.y - 1, size.x, size.y}, style.shadow_color);
    } else {
        painter.draw_rect(button_rect, style.background_color)
            .draw_rect({0, 0, size.x, 1}, style.highlight_color)
            .draw_rect({0, 0, 1, size.y}, style.highlight_color)
            .draw_rect({size.x - 2, 0, size.x, size.y}, style.shadow_color)
            .draw_rect({0, size.y - 2, size.x, size.y}, style.shadow_color);
    }

    m_text.set_text(to_utf32(label).ensure());
    m_text.set_font_size_px(16);
    m_text.set_color(Color(0, 0, 0));

    auto text_size = m_text.text_geometry();

    m_text.render(painter.surface(), {(size.x - text_size.x) / 2, (size.y - text_size.y) / 2, size.x, size.y});
}

void DefaultUIPainter::draw_label(const std::string &label) {
    auto &painter = get_painter();
    auto &style = get_style();

    auto size = vector_static_cast<float>(painter.surface_size());

    m_text.set_text(to_utf32(label).ensure());
    m_text.set_font_size_px(16);
    m_text.set_color(Color(0, 0, 0));
    
    m_text.render(painter.surface(), {2, 2, size.x, size.y});
}

void DefaultUIPainter::draw_background() {
    auto &painter = get_painter();
    auto &style = get_style();

    auto size = vector_static_cast<float>(painter.surface_size());

    painter.draw_rect({0, 0, size.x, size.y}, style.background_color);
}

}
