#include "render_pass.h"

#include <assert.h>

#include "renderer.h"
#include "texture.h"

namespace reimu::graphics {

WebGPURenderPass::WebGPURenderPass(WebGPURenderer &renderer, WGPURenderPipeline pipeline,
        WGPUBindGroupLayout bind_layout, const BindingDefinition *bindings, size_t num_bindings)
        : pipeline(pipeline), bind_layout(bind_layout), m_renderer(renderer) {
    m_bind_entries.resize(num_bindings);

    memset(m_bind_entries.data(), 0, sizeof(WGPUBindGroupEntry) * num_bindings);
    for (size_t i = 0; i < m_bind_entries.size(); i++) {
        auto type = bindings[i].type;

        m_bind_entries[i].binding = i;

        // If the binding is a uniform buffer, create a buffer object for it
        if (type == BindingType::UniformBuffer) {
            WGPUBufferDescriptor buffer_desc = {};
            buffer_desc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform;
            buffer_desc.size = bindings[i].uniform_buffer.size;
            buffer_desc.mappedAtCreation = false;

            auto buffer = m_renderer.create_buffer(buffer_desc);
            assert(buffer);

            m_bind_entries[i].buffer = buffer;
            m_bind_entries[i].size = buffer_desc.size;

            m_bindings.push_back(Binding {
                .buffer = buffer,
                .type = type
            });
        } else {
            m_bindings.push_back(Binding {
                .type = type
            });
        }
    }
}
    
WebGPURenderPass::~WebGPURenderPass() {
    if (m_bind_group) {
        wgpuBindGroupRelease(m_bind_group);
    }

    for (auto &binding : m_bindings) {
        if (binding.type == BindingType::UniformBuffer) {
            wgpuBufferRelease(binding.buffer);
        }
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
        m_old_bind_groups.push_back(m_bind_group);
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

    for (auto &group : m_old_bind_groups) {
        wgpuBindGroupRelease(group);
    }
    m_old_bind_groups.clear();

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

    if (m_bindings[index].texture == tex) {
        return;
    }

    auto *texture = (WebGPUTexture *)tex;

    auto &entry = m_bind_entries[index];
    entry.binding = index;

    // If tex is null, make the binding null
    if (tex) {
        entry.textureView = texture->view();
    } else {
        entry.textureView = nullptr;
    }

    m_bindings[index].texture = tex;

    m_bindings_changed = true;
}

void WebGPURenderPass::bind_uniform_buffer(int index, const void *data, size_t size) {
    auto &binding = m_bindings[index];

    m_renderer.write_buffer(binding.buffer, 0, data, size);
}

}
