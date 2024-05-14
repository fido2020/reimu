#include "render_pass.h"

#include <assert.h>

#include "renderer.h"
#include "texture.h"

namespace reimu::graphics {

WebGPURenderPass::WebGPURenderPass(WebGPURenderer &renderer, WGPURenderPipeline pipeline,
        WGPUBindGroupLayout bind_layout, size_t num_bindings)
        : pipeline(pipeline), bind_layout(bind_layout), m_renderer(renderer) {
    m_bind_entries.resize(num_bindings);

    memset(m_bind_entries.data(), 0, sizeof(WGPUBindGroupEntry) * num_bindings);
    for (size_t i = 0; i < m_bind_entries.size(); i++) {
        m_bind_entries[i].binding = i;
    }
}
    
WebGPURenderPass::~WebGPURenderPass() {
    if (m_bind_group) {
        wgpuBindGroupRelease(m_bind_group);
    }

    wgpuRenderPipelineRelease(pipeline);

    if (bind_layout) {
        wgpuBindGroupLayoutRelease(bind_layout);
    }
}

void WebGPURenderPass::update_bindings() {
    logger::debug("update bindings w layout {:x}", (uintptr_t)bind_layout);
    WGPUBindGroupDescriptor desc = {};
    desc.layout = bind_layout;
    desc.entryCount = m_bind_entries.size();
    desc.entries = m_bind_entries.data();

    if (m_bind_group) {
        wgpuBindGroupRelease(m_bind_group);
    }

    m_bind_group = m_renderer.create_bind_group(desc);
    assert(m_bind_group);

    m_bindings_changed = false;

    if (m_pass_encoder) {
        wgpuRenderPassEncoderSetBindGroup(m_pass_encoder, 0, m_bind_group, 0, nullptr);
    }
    logger::debug("updated bindings, group {:x}", (uintptr_t)bind_layout);
}

void WebGPURenderPass::render(WGPUTextureView output, WGPUCommandEncoder encoder) {
    WGPURenderPassColorAttachment color_attachment = {};
    color_attachment.view = output;
    color_attachment.loadOp = WGPULoadOp_Clear;
    color_attachment.storeOp = WGPUStoreOp_Store;
    color_attachment.clearValue = {0.0f, 0.0f, 0.0f, 1.0f};

    WGPURenderPassDescriptor pass_desc = {};
    pass_desc.colorAttachmentCount = 1;
    pass_desc.colorAttachments = &color_attachment;
    pass_desc.depthStencilAttachment = nullptr;

    m_pass_encoder = wgpuCommandEncoderBeginRenderPass(encoder, &pass_desc);

    wgpuRenderPassEncoderSetPipeline(m_pass_encoder, pipeline);
    
    if (m_bind_group) {
        wgpuRenderPassEncoderSetBindGroup(m_pass_encoder, 0, m_bind_group, 0, nullptr);
    }

    if (strategy) {
        strategy->draw(*this);
    }

    wgpuRenderPassEncoderEnd(m_pass_encoder);
    wgpuRenderPassEncoderRelease(m_pass_encoder);

    m_pass_encoder = nullptr;
}

void WebGPURenderPass::draw(int num_vertices) {
    if (m_bindings_changed) {
        update_bindings();
    }

    wgpuRenderPassEncoderDraw(m_pass_encoder, num_vertices, 1, 0, 0);
}

void WebGPURenderPass::bind_texture(int index, Texture *tex) {
    assert(index < m_bind_entries.size());

    auto *texture = (WebGPUTexture *)tex;

    auto &entry = m_bind_entries[index];
    entry.binding = index;

    // If tex is null, make the binding null
    if (tex) {
        entry.textureView = texture->view();
    } else {
        entry.textureView = nullptr;
    }

    m_bindings_changed = true;
}

void WebGPURenderPass::bind_uniform_buffer(int index, const void *data, size_t size) {
    auto &entry = m_bind_entries[index];
    entry.binding = index;
    //entry.buffer = ...;
    m_bindings_changed = true;
}

}
