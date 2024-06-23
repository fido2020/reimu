#include <reimu/graphics/surface.h>

namespace reimu::graphics {

Surface::Surface(Texture *tex) {
    m_color_format = tex->color_format();
    m_size = tex->size();

    m_texture = std::unique_ptr<Texture>{tex};

    make_buffer();
}

Surface::~Surface() {
    m_texture = nullptr;
}

void Surface::resize(const Vector2i &size) {
    m_texture->replace(m_color_format, size);
    m_size = size;

    make_buffer();
}

void Surface::update() {
    m_texture->update(m_buffer.data(), m_buffer.size());
}

void Surface::make_buffer() {
    size_t buffer_size = m_size.x * m_size.y;
    
    buffer_size *= get_color_format_info(m_color_format).bytes_per_pixel;

    m_buffer.resize(buffer_size);
}

}
