#pragma once

#include <reimu/graphics/render_pass.h>

#include <string.h>
#include <vector>

#include "webgpu.h"

namespace reimu::graphics {

class WebGPURenderer;
class Texture;

class WebGPURenderPass : public RenderPass {
public:
    WebGPURenderPass(WebGPURenderer &renderer, WGPURenderPipeline pipeline,
            WGPUBindGroupLayout bind_layout, size_t num_bindings);

    ~WebGPURenderPass() override;

    void render(WGPUTextureView output, WGPUCommandEncoder encoder);
    void update_bindings();

    void draw(int num_vertices) override;
    void bind_texture(int index, Texture *texture) override;
    void bind_uniform_buffer(int index, const void *data, size_t size) override;

    inline void set_strategy(std::shared_ptr<RenderStrategy> strategy) override {
        this->strategy = strategy;
    }

    WGPURenderPipeline pipeline = nullptr;
    WGPUBindGroupLayout bind_layout = nullptr;

    std::shared_ptr<RenderStrategy> strategy = nullptr;

private:
    WebGPURenderer &m_renderer;

    WGPURenderPassEncoder m_pass_encoder = nullptr;

    bool m_bindings_changed = false;
    WGPUBindGroup m_bind_group = nullptr;

    std::vector<WGPUBindGroupEntry> m_bind_entries;
};

}
