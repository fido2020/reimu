#include "render_pass.h"

namespace reimu::graphics {

WebGPURenderPass::~WebGPURenderPass() {
    wgpuRenderPipelineRelease(pipeline);
    wgpuBindGroupLayoutRelease(bind_layout);
}

}
