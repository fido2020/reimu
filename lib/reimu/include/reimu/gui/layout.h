#pragma once

#include <reimu/graphics/color.h>
#include <reimu/graphics/rect.h>
#include <reimu/graphics/vector.h>

#include <list>

namespace reimu::gui {

struct CalculatedLayout
{
    Vector2f inner_size;
    Vector2f outer_size;

    float font_size;
    float root_font_size;
    
    float left_padding;
    float right_padding;
    float top_padding;
    float bottom_padding;
};

enum class LayoutUnit {
    Zero,
    Inherit,
    Pixels,
    Percent,
    RootEm,
    Em,
    LayoutFactor
};

enum class ContentJustify {
    Left,
    Top,
    Center,
    Right,
    Bottom
};

enum class LayoutDirection {
    Horizontal,
    Vertical
};

struct Size {
    float value;
    LayoutUnit unit;

    static consteval Size zero() {
        return Size { 0, LayoutUnit::Zero };
    }

    static consteval Size inherit() {
        return Size { 0, LayoutUnit::Inherit };
    }

    static consteval Size from_pixels(float value) {
        return Size { value, LayoutUnit::Pixels };
    }

    static consteval Size from_percent(float value) {
        return Size { value, LayoutUnit::Percent };
    }

    static consteval Size from_root_em(float value) {
        return Size { value, LayoutUnit::RootEm };
    }

    static consteval Size from_em(float value) {
        return Size { value, LayoutUnit::Em };
    }

    static consteval Size from_layout_factor(float value) {
        return Size { value, LayoutUnit::LayoutFactor };
    }

    float as_pixels(float parent_value, const CalculatedLayout &parent) {
        switch(unit) {
            case LayoutUnit::Zero:
            case LayoutUnit::LayoutFactor:
                return 0;
            case LayoutUnit::Pixels:
                return value;
            case LayoutUnit::Percent:
                return value * parent_value;
            case LayoutUnit::RootEm:
                return value * parent.root_font_size;
            case LayoutUnit::Em:
                return value * parent.font_size;
            case LayoutUnit::Inherit:
            default:
                return parent_value;
        }
    }
};

struct LayoutProperties {
    Size width = Size::from_pixels(200);
    Size height = Size::from_pixels(200);

    Size left_padding = Size::zero();
    Size right_padding = Size::zero();
    Size top_padding = Size::zero();
    Size bottom_padding = Size::zero();

    Size left_margin = Size::zero();
    Size right_margin = Size::zero();
    Size top_margin = Size::zero();
    Size bottom_margin = Size::zero();

    Size font_size = Size::inherit();

    LayoutDirection layout_direction;

    inline void set_padding(const Size &sz) {
        left_padding = sz;
        right_padding = sz;
        top_padding = sz;
        bottom_padding = sz;
    }

    void calculate_layout(CalculatedLayout &out, const CalculatedLayout *parent);
};

}
