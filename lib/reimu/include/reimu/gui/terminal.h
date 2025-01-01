#pragma once

#include <reimu/gui/widget.h>

#include <string_view>
#include <memory>

#define BUFFERED_LINES_DEFAULT 100

namespace reimu::gui {

struct TerminalPrivateData;

class TerminalWidget : public Widget {
public:
    enum class EraseMode {
        CursorToEnd,
        StartToCursor,
        All
    };

    TerminalWidget(std::shared_ptr<graphics::Font> font);
    ~TerminalWidget() override;

    void put_char(uint32_t c);
    void put_line_utf8(std::string_view line);

    void move_cursor(int x, int y);
    void set_cursor(const Vector2i &pos);
    void set_cursor_visible(bool visible);

    void backspace();

    Vector2i get_cursor() const;
    Vector2i get_size() const;

    void erase_display(EraseMode mode);
    void erase_line(EraseMode mode);

    void repaint(UIPainter &painter) override;
    void update_layout() override;

    void reset_attributes();
    void set_bold(bool bold);

    void invert_colors() {
        std::swap(m_fg_color, m_bg_color);
    }

    void set_fg_color(const Color &color) { m_fg_color = color; }
    void set_bg_color(const Color &color) { m_bg_color = color; }

    void set_fg_color(uint8_t color_8bit);
    void set_bg_color(uint8_t color_8bit);

    void line_break();

private:
    std::list<std::vector<uint32_t>> m_lines;
    TerminalPrivateData *m_data;

    std::shared_ptr<graphics::Font> m_font;

    Color m_fg_color = Color::white();
    Color m_bg_color = Color::black();

    bool m_has_requested_repaint = false;
};

}
