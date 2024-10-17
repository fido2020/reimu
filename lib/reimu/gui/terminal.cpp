#include <reimu/gui/terminal.h>

#include <reimu/core/unicode.h>
#include <reimu/graphics/text.h>

#include "terminal_grid.h"

namespace reimu::gui {

extern uint32_t term_256_colors[];

struct TerminalPrivateData {
    term::Grid grid{80, 25};
};

TerminalWidget::TerminalWidget() {
    m_data = new TerminalPrivateData;
}

TerminalWidget::~TerminalWidget() {
    delete m_data;
}

void TerminalWidget::put_char(uint32_t c) {
    m_data->grid.put_cell_at_cursor(term::Cell(c, m_fg_color.value, m_bg_color.value));

    if (!m_has_requested_repaint) {
        m_has_requested_repaint = true;
        
        dispatch_event("ui_repaint"_hashid);
    }
}

void TerminalWidget::put_line_utf8(std::string_view line) {
    auto utf32 = to_utf32(line).ensure();
    for (auto c : utf32) {
        put_char(c);
    }
}

void TerminalWidget::move_cursor(int x, int y) {
    m_data->grid.move_cursor(x, y);
}

void TerminalWidget::set_cursor(const Vector2i &pos) {
    m_data->grid.set_cursor(pos.x, pos.y);
}

Vector2i TerminalWidget::get_cursor() const {
    return m_data->grid.get_cursor();
}

Vector2i TerminalWidget::get_size() const {
    return {
        m_data->grid.get_row_size(),
        m_data->grid.get_num_visible_rows()
    };
}

void TerminalWidget::erase_display(EraseMode mode) {
    switch (mode) {
    case EraseMode::CursorToEnd:
        m_data->grid.erase_display(m_data->grid.get_cursor(),
            {m_data->grid.get_row_size() - 1, m_data->grid.get_num_visible_rows() - 1});
        break;
    case EraseMode::StartToCursor:
        m_data->grid.erase_display({0, 0},
            m_data->grid.get_cursor());
        break;
    case EraseMode::All:
        m_data->grid.erase_display({0, 0},
            {m_data->grid.get_row_size() - 1, m_data->grid.get_num_visible_rows() - 1});
        break;
    }

    if (!m_has_requested_repaint) {
        m_has_requested_repaint = true;
        
        dispatch_event("ui_repaint"_hashid);
    }
}

void TerminalWidget::erase_line(EraseMode mode) {
    auto cur = m_data->grid.get_cursor();

    switch (mode) {
    case EraseMode::CursorToEnd:
        m_data->grid.erase_line(cur.y, cur.x, m_data->grid.get_row_size() - 1);
        break;
    case EraseMode::StartToCursor:
        m_data->grid.erase_line(cur.y, 0, cur.x);
        break;
    case EraseMode::All:
        m_data->grid.erase_line(cur.y, 0, m_data->grid.get_row_size() - 1);
        break;
    }

    if (!m_has_requested_repaint) {
        m_has_requested_repaint = true;
        
        dispatch_event("ui_repaint"_hashid);
    }
}

void TerminalWidget::repaint(UIPainter &painter) {
    Widget::repaint(painter);

    m_has_requested_repaint = false;

    graphics::Painter gfx_painter{ *m_surface };

    // TODO: WOW this is inefficient and really bad
    auto text_obj = graphics::Text{};
    text_obj.set_font_size_px(16);

    auto draw_cell = [&](uint32_t ch, int row, int col, uint32_t fg_color, uint32_t bg_color) {
        auto cell_rect = Rectf::from_size({col * 8.f, row * 16.f}, {16.f, 16.f});

        // We use UINT32_MAX to mark that the cell is empty
        if (ch == UINT32_MAX) {
            gfx_painter.draw_rect(cell_rect, m_bg_color);
            return;
        }

        gfx_painter.draw_rect(cell_rect, Color(bg_color));

        auto text = std::u32string(1, ch);
        text_obj.set_text(text);

        text_obj.set_color(fg_color);
        text_obj.render(*m_surface, cell_rect);
    };

    m_data->grid.paint(draw_cell);
}

void TerminalWidget::update_layout() {
    Widget::update_layout();

    m_data->grid.resize(calculated_layout.inner_size.x / 8, calculated_layout.inner_size.y / 16);
}

void TerminalWidget::reset_attributes() {
    m_fg_color = Color::white();
    m_bg_color = Color::black();
}

void TerminalWidget::set_bold(bool bold) {
    // TODO
}

void TerminalWidget::set_fg_color(uint8_t color_8bit) {
    // Select the foreground color from the 8-bit color palette
    m_fg_color = term_256_colors[color_8bit];
}

void TerminalWidget::set_bg_color(uint8_t color_8bit) {
    m_bg_color = term_256_colors[color_8bit];
}

void TerminalWidget::line_break() {
    m_data->grid.carriage_return();
    m_data->grid.next_row();
}

// Array of the default 256 color palette in form RGBA (0xAABBGGRR)
uint32_t term_256_colors[] = {
    0xFF000000, 0xFF0000AA, 0xFF00AA00, 0xFF00AAAA, 0xFFAA0000, 0xFFAA00AA, 0xFFAA5500, 0xFFAAAAAA,
    0xFF555555, 0xFF5555FF, 0xFF55FF55, 0xFF55FFFF, 0xFFFF5555, 0xFFFF55FF, 0xFFFFFF55, 0xFFFFFFFF,
    
    // 216 colors
    0xFF000000, 0xFF5F0000, 0xFF870000, 0xFFAF0000, 0xFFD70000, 0xFFFF0000, 0xFF005F00, 0xFF5F5F00,
    0xFF875F00, 0xFFAF5F00, 0xFFD75F00, 0xFFFF5F00, 0xFF008700, 0xFF5F8700, 0xFF878700, 0xFFAF8700,
    0xFFD78700, 0xFFFF8700, 0xFF00AF00, 0xFF5FAF00, 0xFF87AF00, 0xFFAFAF00, 0xFFD7AF00, 0xFFFFAF00,
    0xFF00D700, 0xFF5FD700, 0xFF87D700, 0xFFAFD700, 0xFFD7D700, 0xFFFFD700, 0xFF00FF00, 0xFF5FFF00,
    0xFF87FF00, 0xFFAFFF00, 0xFFD7FF00, 0xFFFFFF00, 0xFF00005F, 0xFF5F005F, 0xFF87005F, 0xFFAF005F,
    0xFFD7005F, 0xFFFF005F, 0xFF005F5F, 0xFF5F5F5F, 0xFF875F5F, 0xFFAF5F5F, 0xFFD75F5F, 0xFFFF5F5F,
    0xFF00875F, 0xFF5F875F, 0xFF87875F, 0xFFAF875F, 0xFFD7875F, 0xFFFF875F, 0xFF00AF5F, 0xFF5FAF5F,
    0xFF87AF5F, 0xFFAFAF5F, 0xFFD7AF5F, 0xFFFFAF5F, 0xFF00D75F, 0xFF5FD75F, 0xFF87D75F, 0xFFAFD75F,
    0xFFD7D75F, 0xFFFFD75F, 0xFF00FF5F, 0xFF5FFF5F, 0xFF87FF5F, 0xFFAFFF5F, 0xFFD7FF5F, 0xFFFFFF5F,
    0xFF000087, 0xFF5F0087, 0xFF870087, 0xFFAF0087, 0xFFD70087, 0xFFFF0087, 0xFF005F87, 0xFF5F5F87,
    0xFF875F87, 0xFFAF5F87, 0xFFD75F87, 0xFFFF5F87, 0xFF008787, 0xFF5F8787, 0xFF878787, 0xFFAF8787,
    0xFFD78787, 0xFFFF8787, 0xFF00AF87, 0xFF5FAF87, 0xFF87AF87, 0xFFAFAF87, 0xFFD7AF87, 0xFFFFAF87,
    0xFF00D787, 0xFF5FD787, 0xFF87D787, 0xFFAFD787, 0xFFD7D787, 0xFFFFD787, 0xFF00FF87, 0xFF5FFF87,
    0xFF87FF87, 0xFFAFFF87, 0xFFD7FF87, 0xFFFFFF87, 0xFF0000AF, 0xFF5F00AF, 0xFF8700AF, 0xFFAF00AF,
    0xFFD700AF, 0xFFFF00AF, 0xFF005FAF, 0xFF5F5FAF, 0xFF875FAF, 0xFFAF5FAF, 0xFFD75FAF, 0xFFFF5FAF,
    0xFF0087AF, 0xFF5F87AF, 0xFF8787AF, 0xFFAF87AF, 0xFFD787AF, 0xFFFF87AF, 0xFF00AFAF, 0xFF5FAFAF,
    0xFF87AFAF, 0xFFAFAFAF, 0xFFD7AFAF, 0xFFFFAFAF, 0xFF00D7AF, 0xFF5FD7AF, 0xFF87D7AF, 0xFFAFD7AF,
    0xFFD7D7AF, 0xFFFFD7AF, 0xFF00FFAF, 0xFF5FFFAF, 0xFF87FFAF, 0xFFAFFF, 0xFFD7FFAF, 0xFFFFFFAF,
    0xFF0000D7, 0xFF5F00D7, 0xFF8700D7, 0xFFAF00D7, 0xFFD700D7, 0xFFFF00D7, 0xFF005FD7, 0xFF5F5FD7,
    0xFF875FD7, 0xFFAF5FD7, 0xFFD75FD7, 0xFFFF5FD7, 0xFF0087D7, 0xFF5F87D7, 0xFF8787D7, 0xFFAF87D7,
    0xFFD787D7, 0xFFFF87D7, 0xFF00AFD7, 0xFF5FAFD7, 0xFF87AFD7, 0xFFAFAFD7, 0xFFD7AFD7, 0xFFFFAFD7,
    0xFF00D7D7, 0xFF5FD7D7, 0xFF87D7D7, 0xFFAFD7D7, 0xFFD7D7D7, 0xFFFFD7D7, 0xFF00FFD7, 0xFF5FFFD7,
    0xFF87FFD7, 0xFFAFFFD7, 0xFFD7FFD7, 0xFFFFFFD7, 0xFF0000FF, 0xFF5F00FF, 0xFF8700FF, 0xFFAF00FF,
    0xFFD700FF, 0xFFFF00FF, 0xFF005FFF, 0xFF5F5FFF, 0xFF875FFF, 0xFFAF5FFF, 0xFFD75FFF, 0xFFFF5FFF,
    0xFF0087FF, 0xFF5F87FF, 0xFF8787FF, 0xFFAF87FF, 0xFFD787FF, 0xFFFF87FF, 0xFF00AFFF, 0xFF5FAFFF,
    0xFF87AFFF, 0xFFAFAFFF, 0xFFD7AFFF, 0xFFFFAFFF, 0xFF00D7FF, 0xFF5FD7FF, 0xFF87D7FF, 0xFFAFD7FF,
    0xFFD7D7FF, 0xFFFFD7FF, 0xFF00FFFF, 0xFF5FFFFF, 0xFF87FFFF, 0xFFAFFFFF, 0xFFD7FFFF, 0xFFFFFFFF,

    // Grayscale
    0xFF080808, 0xFF121212, 0xFF1C1C1C, 0xFF262626, 0xFF303030, 0xFF3A3A3A, 0xFF444444, 0xFF4E4E4E,
    0xFF585858, 0xFF626262, 0xFF6C6C6C, 0xFF767676, 0xFF808080, 0xFF8A8A8A, 0xFF949494, 0xFF9E9E9E,
    0xFFA8A8A8, 0xFFB2B2B2, 0xFFBCBCBC, 0xFFC6C6C6, 0xFFD0D0D0, 0xFFDADADA, 0xFFE4E4E4, 0xFFEEEEEE,
};

} // namespace reimu::gui
