#pragma once

#include <stddef.h>

#include <reimu/graphics/vector.h>

namespace reimu::graphics {

struct ColorFormatInfo {
    int bytes_per_pixel;
};

enum class ColorFormat {
    RGBA8,
};

ColorFormatInfo get_color_format_info(ColorFormat format);

class Texture {
public:
    virtual ~Texture() = default;

    /**
     * @brief Reallocate the underlying texture.
    */
    virtual void replace(ColorFormat fmt, const Vector2i &size) = 0;

    virtual void update(const void *data, size_t size) = 0;

    inline ColorFormat color_format() const { return m_format; }
    const Vector2i &size() const { return m_size; }

protected:
    Texture(ColorFormat fmt, const Vector2i &size) : m_format(fmt), m_size(size) {}

    ColorFormat m_format;
    Vector2i m_size;
};

}
