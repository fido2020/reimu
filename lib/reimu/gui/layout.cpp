#include <reimu/gui/layout.h>

namespace reimu::gui {

void LayoutProperties::calculate_layout(CalculatedLayout &out, const CalculatedLayout *parent) {
    if (parent) {
        const auto &parent_layout = *parent;

        out.root_font_size = parent_layout.root_font_size;
        out.font_size =
            font_size.as_pixels(parent_layout.font_size, parent_layout);

        out.left_padding =
            left_padding.as_pixels(parent_layout.left_padding, parent_layout);
        out.right_padding =
            right_padding.as_pixels(parent_layout.right_padding, parent_layout);
        out.top_padding =
            top_padding.as_pixels(parent_layout.top_padding, parent_layout);
        out.bottom_padding =
            bottom_padding.as_pixels(parent_layout.bottom_padding, parent_layout);

        out.outer_size = Vector2f(
            width.as_pixels(parent_layout.inner_size.x, parent_layout),
            height.as_pixels(parent_layout.inner_size.y, parent_layout)
        );

        out.inner_size = Vector2f(
            out.outer_size.x - out.left_padding - out.right_padding,
            out.outer_size.y - out.top_padding - out.bottom_padding
        );
    }
}

}
