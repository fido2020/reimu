#include <reimu/graphics/font.h>

#define FIXED_FONT_PATH "font.ttf"

#include "freetype.h"

namespace reimu::graphics {

struct detail::FontData {
    FT_Face face;
    std::vector<uint8_t> data;
};

Result<Font *, ReimuError> Font::create() {
    FILE *file = fopen(FIXED_FONT_PATH, "rb");
    if (!file) {
        return ERR(ReimuError::FileNotFound);
    }

    std::vector<uint8_t> font_data;

    auto r = FreeType::instance().new_face(file, 0, font_data); 
    if (r.is_err()) {
        return ERR(ReimuError::FailedToLoadFont);
    }

    auto font = new Font();
    font->m_data = std::make_unique<detail::FontData>(r.ensure(), std::move(font_data));

    return OK(font);
}

Font::Font() {}
Font::~Font() {
    FreeType::instance().done_face(m_data->face);
}

void *Font::get_handle() {
    return m_data->face;
}

}
