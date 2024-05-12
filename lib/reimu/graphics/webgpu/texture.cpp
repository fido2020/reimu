#include "texture.h"

#include "renderer.h"
#include "webgpu.h"

namespace reimu::graphics {

void WebGPUTexture::update(const void *data, size_t size) {
    WGPUImageCopyTexture image_copy_texture = {};
    image_copy_texture.texture = m_texture;
    image_copy_texture.mipLevel = 0;
    // Offset in the texture
    image_copy_texture.origin = {0, 0, 0};
    image_copy_texture.aspect = WGPUTextureAspect_All;

    auto stride = get_color_format_info(m_format).bytes_per_pixel * m_size.x;

    WGPUTextureDataLayout source_layout = {};
    source_layout.offset = 0;
    source_layout.bytesPerRow = stride;
    source_layout.rowsPerImage = m_size.y;

    m_renderer.write_texture(image_copy_texture, data, size, source_layout,
        WGPUExtent3D{
            (uint32_t)m_size.x, (uint32_t)m_size.y, 1
        });
}

}
