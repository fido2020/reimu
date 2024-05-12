#include <reimu/graphics/texture.h>

namespace reimu::graphics {

static constexpr ColorFormatInfo format_RGBA8 = {4};

ColorFormatInfo get_color_format_info(ColorFormat format) {
    switch (format) {
    case ColorFormat::RGBA8:
        return format_RGBA8;
    }
}

}
