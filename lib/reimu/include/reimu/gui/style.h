#pragma once

#include <reimu/graphics/color.h>
#include <reimu/graphics/texture.h>
#include <reimu/graphics/text.h>
#include <reimu/graphics/painter.h>

#include <string>

namespace reimu::gui {

struct UIStyle {
    Color background_color;
    Color text_color;

    Color highlight_color;
    Color shadow_color;

    int win_border_thickness;
    int win_titlebar_height;
};

class UIPainter {
public:
    UIPainter();
    virtual ~UIPainter() = default;

    void begin(graphics::Painter &painter);
    void end();

    const UIStyle &get_style() const { return m_style; }

    virtual void draw_frame(const std::string &title, bool is_active) = 0;
    virtual void draw_button(const std::string &label, bool is_pressed) = 0;
    virtual void draw_label(const std::string &label) = 0;
    virtual void draw_background() = 0;

    /*virtual void draw_textbox(std::string text, int cur_pos, bool is_pressed) = 0;*/

protected:
    graphics::Painter &get_painter();

    UIStyle m_style;
    graphics::Painter *m_current_painter;
};

class DefaultUIPainter : public UIPainter {
public:
    DefaultUIPainter();

    void draw_frame(const std::string &title, bool is_active) override;
    void draw_button(const std::string &label, bool is_pressed) override;
    void draw_label(const std::string &label) override;
    void draw_background() override;

private:
    graphics::Text m_text;
};

}
