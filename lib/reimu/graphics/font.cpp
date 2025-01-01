#include <reimu/graphics/font.h>

#include <reimu/core/resource_manager.h>

#define FIXED_FONT_PATH "font.ttf"

#include "freetype.h"

namespace reimu::graphics {

struct detail::FontData {
    FT_Face face;
    std::vector<uint8_t> data;
};

Result<Font *, ReimuError> Font::create(File &file) {
    std::vector<uint8_t> font_data = file.read(file.file_size()).ensure();

    auto r = FreeType::instance().new_face(font_data, 0); 
    if (r.is_err()) {
        auto err = r.move_err();
        logger::debug("Failed to load font: {:x}", (err.error));
        return ERR(ReimuError::FailedToLoadFont);
    }

    auto font = new Font();
    font->m_data = std::make_unique<detail::FontData>(r.ensure(), std::move(font_data));

    return OK(font);
}

StringID Font::obj_type_id() const {
    return Font::type_id();
}

Font::Font() {}
Font::~Font() {
    FreeType::instance().done_face(m_data->face);
}

void *Font::get_handle() {
    return m_data->face;
}

}
