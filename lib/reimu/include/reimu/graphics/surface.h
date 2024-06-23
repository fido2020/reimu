#pragma once

#include <reimu/graphics/texture.h>
#include <reimu/graphics/vector.h>

#include <memory>
#include <vector>

namespace reimu::graphics {

class Surface {
public:
    Surface(Texture *tex);
    ~Surface();

    /**
     * @brief Resize the surface including any underlying texture
    */
    void resize(const Vector2i &size);

    /**
     * @brief Get the pointer to the software buffer of this surface
    */
    inline uint8_t *buffer() {
        return m_buffer.data();
    }

    /**
     * @brief Get the Texture for this surface
    */
    inline class Texture &texture() {
        return *m_texture;
    }

    /**
     * @brief Sync the Texture object with the software buffer
    */
    void update();

    inline Vector2i size() const {
        return m_size;
    }

    inline uint32_t bytes_per_pixel() const {
        return get_color_format_info(m_color_format).bytes_per_pixel;
    }

    inline uint32_t stride() const {
        return bytes_per_pixel() * m_size.x;
    }

private:
    void make_buffer();

    ColorFormat m_color_format;

    Vector2i m_size;
    std::vector<uint8_t> m_buffer;

    std::unique_ptr<Texture> m_texture;
};

}
