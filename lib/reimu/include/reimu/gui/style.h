#pragma once

#include <reimu/graphics/color.h>
#include <reimu/graphics/texture.h>
#include <reimu/graphics/painter.h>

#include <string>

namespace reimu::gui {

struct UIStyle {
    Color background_color;
    Color text_color;
};

class UIPainter { //: public graphics::Painter {
public:
    /*UIPainter(graphics::Surface &surface)
        : graphics::Painter(surface) {}

    virtual void draw_label(std::string label) = 0;
    virtual void draw_button(std::string label, bool is_pressed) = 0;
    virtual void draw_textbox(std::string text, int cur_pos, bool is_active) = 0;*/
};

}