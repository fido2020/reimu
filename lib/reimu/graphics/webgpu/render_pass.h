#pragma once

#include <reimu/graphics/render_pass.h>

#include "webgpu.h"

namespace reimu::graphics {

class WebGPURenderer;

class WebGPURenderPass : public RenderPass {
public:
    WebGPURenderPass(WebGPURenderer &renderer, WGPURenderPipeline pipeline)
        : pipeline(pipeline), m_renderer(renderer) {}

    ~WebGPURenderPass() override;

    WGPURenderPipeline pipeline = nullptr;
    WGPUBindGroupLayout bind_layout = nullptr;

private:
    WebGPURenderer &m_renderer;
};

}
