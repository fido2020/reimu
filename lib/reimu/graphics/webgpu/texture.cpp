#include "texture.h"

#include "renderer.h"
#include "webgpu.h"

namespace reimu::graphics {

WebGPUTexture::~WebGPUTexture() {
    logger::debug("Destroying texture!");
    wgpuTextureViewRelease(m_view);

    wgpuTextureDestroy(m_texture);
    wgpuTextureRelease(m_texture);
}

void WebGPUTexture::replace(ColorFormat fmt, const Vector2i &size) {
    m_format = fmt;
    m_size = size;

    wgpuTextureViewRelease(m_view);
    wgpuTextureDestroy(m_texture);
    wgpuTextureRelease(m_texture);
    
    auto tex_format = webgpu::convert_color_format(fmt);

    WGPUTextureDescriptor texture_desc = {};
    texture_desc.size.width = size.x;
    texture_desc.size.height = size.y;
    texture_desc.size.depthOrArrayLayers = 1;
    texture_desc.mipLevelCount = 1;
    texture_desc.sampleCount = 1;
    texture_desc.dimension = WGPUTextureDimension_2D;
    texture_desc.format = tex_format;
    texture_desc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;

    m_texture = m_renderer.create_texture_obj(texture_desc);
    if (!m_texture) {
        logger::fatal("Failed to create texture");
    }

    WGPUTextureViewDescriptor texture_view_desc = {};
    texture_view_desc.format = tex_format;
    texture_view_desc.dimension = WGPUTextureViewDimension_2D;
    texture_view_desc.baseMipLevel = 0;
    texture_view_desc.mipLevelCount = 1;
    texture_view_desc.baseArrayLayer = 0;
    texture_view_desc.arrayLayerCount = 1;
    texture_view_desc.aspect = WGPUTextureAspect_All;

    m_view = wgpuTextureCreateView(m_texture, &texture_view_desc);
    if (!m_view) {
        logger::fatal("Failed to create texture view");
    }
}

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
