#include <cstdint>
#include <reimu/graphics/text.h>

#include <cassert>

#include "freetype.h"
#include "freetype/freetype.h"

FreeType* FreeType::m_instance = new FreeType();

FreeType::FreeType() {
    assert(!m_instance);
    m_instance = this;

    if (FT_Error e = FT_Init_FreeType(&m_library); e) {
        reimu::logger::warn("Error {} initializing freetype!", FT_Error_String(e));
        reimu::logger::fatal("Failed to initialize Freetype!");
    }
}

reimu::Result<FT_Face, FreeTypeError> FreeType::new_face(std::vector<uint8_t> &data, FT_Long index) {
    std::unique_lock lockFT(m_lock);

    FT_Face face;
    if(FT_Error e = FT_New_Memory_Face(m_library, data.data(), data.size(), index, &face); e) {
        return ERR(FreeTypeError{ e });
    }

    return OK(face);
}

reimu::Result<void, FreeTypeError> FreeType::done_face(FT_Face face) {
    std::unique_lock lockFT(m_lock);

    auto e = FT_Done_Face(face);
    if (e) {
        return ERR(FreeTypeError{ e });
    }

    return OK();
}

namespace reimu::graphics {

Text::Text() {}

Text::Text(std::u32string text) : Text() { set_text(std::move(text)); }

void Text::render(Surface &dest, const Rectf &bounds) {
    if (!m_font.get()) {
        return;
    }

    Recti final_bounds = {
        (int)bounds.x, (int)bounds.y, std::min((int)bounds.z, dest.size().x),
        std::min((int)bounds.w, dest.size().y), 
    };

    if (final_bounds.z <= bounds.x || final_bounds.w <= bounds.y) {
        return;
    }

    std::unique_lock fontLock(m_font->m_lock);
    FT_Face face = reinterpret_cast<FT_Face>(m_font->get_handle());

    if (FT_Set_Pixel_Sizes(face, 0, m_pixel_size)) {
        reimu::logger::warn("Failed to set font size!");
        return;
    }

    bool use_kerning = FT_HAS_KERNING(face);
    // In 64ths of a pixel so r shift by 6
    unsigned int pixel_line_height = face->size->metrics.height >> 6;

    // Compose a vector of glyphs
    std::vector<unsigned int> glyphs;

    for (int codepoint : m_text) {
        if (codepoint == '\n') {
            glyphs.push_back('\n');
        } else if (codepoint != '\r') { // Ignore carriage returns
            glyphs.push_back(FT_Get_Char_Index(face, codepoint));
        }
    }

    if (glyphs.size() == 0) {
        return;
    }

    int x_pos = final_bounds.x;
    int y_pos = final_bounds.y;

    auto surface_size = dest.size();
    uint32_t *surface_buffer = (uint32_t*)dest.buffer();

    assert(dest.bytes_per_pixel() == 4);
    
    FT_Vector kerning = { 0, 0 };
    int prev_glyph = 0;
    for (unsigned int glyph : glyphs) {
        if (glyph == '\n') {
            y_pos += static_cast<int>(pixel_line_height);
            x_pos = 0;
            continue;
        }

        if (use_kerning && prev_glyph) {
            FT_Get_Kerning(face, prev_glyph, glyph, FT_KERNING_DEFAULT, &kerning);
            x_pos += kerning.x >> 6; // Offset the x position for kerning
        }

        // TODO: settings to tweak whether to enable font smoothing
        constexpr bool font_smoothing = false;

        if (FT_Load_Glyph(face, glyph, font_smoothing ? FT_LOAD_NO_BITMAP : FT_LOAD_NO_HINTING | FT_LOAD_MONOCHROME)) {
            continue;
        }

        if (FT_Render_Glyph(face->glyph, font_smoothing ? FT_RENDER_MODE_NORMAL : FT_RENDER_MODE_MONO)) {
            continue;
        }

        FT_GlyphSlot slot = face->glyph;

        // Start of the font blit
        // Ascender is the difference between the baseline and the top of the glyph
        int y_offset = y_pos + (face->size->metrics.ascender >> 6) - slot->bitmap_top;
        assert(y_offset >= 0);

        int x_off = 0;
        if (!use_kerning) {
            x_off += face->glyph->metrics.horiBearingX >> 6;
        }

        int x_min = std::max(0, final_bounds.x - x_pos);
        int x_max = std::min((int)slot->bitmap.width, final_bounds.z - x_pos);

        // Copy the glyph into the texture
        for (unsigned y = 0; y < slot->bitmap.rows && y_offset < final_bounds.w; y++, y_offset++) {
            if (y_offset + (int)y < 0) {
                continue;
            }
            
            auto *dst = surface_buffer + y_offset * surface_size.x + x_pos + x_off;

            uint8_t *src = (uint8_t*)slot->bitmap.buffer + y * slot->bitmap.pitch;

            Color c = m_color;
            for (int x = x_min; x < x_max; x++) {
                if (font_smoothing && src[x]) {
                    // Copy the pixel into the surface buffer
                    c.a = src[x];

                    dst[x] = (Color(dst[x]) * c).value;
                } else if (src[x >> 3] & (1 << (7 - (x & 7)))) {
                    dst[x] = c.value;
                }
            }
        }

        // Advance the x position
        x_pos += slot->advance.x >> 6;
        prev_glyph = glyph;
    }
}

void Text::set_font(std::shared_ptr<Font> font) {
    m_font = std::move(font);

    m_stale_geometry = true;
}

void Text::set_text(std::u32string text) {
    m_text = std::move(text);

    m_stale_geometry = true;
}

void Text::set_color(const Color& color) {
    m_color = color;
}

void Text::set_font_size_px(int size_px) {
    m_pixel_size = size_px;

    m_stale_geometry = true;
}

Vector2f Text::text_geometry() {
    if (!m_font.get()) {
        return { 0, 0 };
    }

    if (m_stale_geometry) {
        std::unique_lock fontLock(m_font->m_lock);
        FT_Face face = reinterpret_cast<FT_Face>(m_font->get_handle());

        if (FT_Set_Pixel_Sizes(face, 0, m_pixel_size)) {
            reimu::logger::warn("Failed to set font size!");
            return {0, 0};
        }

        bool use_kerning = FT_HAS_KERNING(face);
        // In 64ths of a pixel so r shift by 6
        unsigned int pixel_line_height = face->size->metrics.height >> 6;

        float y_pos = 0;

        float x_pos = 0;
        float x_max = 0;

        FT_Vector kerning = { 0, 0 };
        int prev_glyph = 0;

        for (auto codepoint : m_text) {
            if (codepoint == '\n') {
                y_pos += (float)pixel_line_height;
                
                x_max = std::max(x_max, x_pos);
                x_pos = 0;

                prev_glyph = 0;
            } else if (codepoint != '\r') { // Ignore carriage returns
                auto glyph = FT_Get_Char_Index(face, codepoint);

                if (use_kerning && prev_glyph) {
                    x_pos += kerning.x >> 6; // Offset the x position for kerning
                    
                    FT_Get_Kerning(face, prev_glyph, glyph, FT_KERNING_DEFAULT, &kerning);
                }

                if (FT_Load_Glyph(face, glyph, FT_LOAD_ADVANCE_ONLY)) {
                    continue;
                }

                // Advance the x position
                x_pos += face->glyph->metrics.horiAdvance >> 6;
                prev_glyph = glyph;
            }
        }

        x_max = std::max(x_max, x_pos);
        m_text_size = { x_max, y_pos + pixel_line_height };

        m_stale_geometry = false;
    }

    return m_text_size;
}

} // namespace Arclight
