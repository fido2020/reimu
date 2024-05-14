#pragma once

#include <reimu/graphics/texture.h>

#include "webgpu.h"

namespace reimu::graphics {

class WebGPURenderer;

class WebGPUTexture : public Texture {
public:
    WebGPUTexture(WebGPURenderer &renderer, ColorFormat fmt, const Vector2i &size,
            WGPUTexture texture, WGPUTextureView view)
        : Texture(fmt, size), m_renderer(renderer), m_texture(texture), m_view(view) {}

    void update(const void *data, size_t size) override;

    inline WGPUTextureView view() {
        return m_view;
    }

private:
    WebGPURenderer &m_renderer;

    WGPUTexture m_texture;
    WGPUTextureView m_view;
};

}
