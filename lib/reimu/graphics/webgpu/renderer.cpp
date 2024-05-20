#include "renderer.h"

#include <reimu/core/logger.h>

#include <assert.h>
#include <vector>

#include "texture.h"
#include "webgpu.h"

#define SWAP_CHAIN_FORMAT WGPUTextureFormat_BGRA8Unorm

namespace reimu::graphics {

WebGPURenderer *WebGPURenderer::create(video::Window *window) {
    WGPUInstanceDescriptor desc = {};
    desc.nextInChain = nullptr;

    auto *renderer = new WebGPURenderer;

    renderer->m_viewport_size = window->get_size();

    auto instance = wgpuCreateInstance(&desc);
    if (!instance) {
        logger::warn("Failed to create WebGPU instance");

        delete renderer;
        return nullptr;
    }
    renderer->m_instance = instance;

    // Create a WebGPU surface using the window
    auto surface_result = create_bind_window_surface(instance, window);
    if (surface_result.is_err()) {
        logger::warn("Failed to create WebGPU surface: {}", surface_result.move_err());
        
        delete renderer;
        return nullptr;
    }
    
    auto surface = surface_result.move_val();
    if (!surface) {
        logger::warn("Failed to create WebGPU surface");

        delete renderer;
        return nullptr;
    }

    renderer->m_surface = surface;

    // Use our new surface to request an adapter
    WGPURequestAdapterOptions adapter_options = {};
    adapter_options.compatibleSurface = renderer->m_surface;

    auto adapter = create_adapter(instance, adapter_options);
    if (!adapter) {
        logger::warn("Failed to create WebGPU adapter");

        delete renderer;
        return nullptr;
    }
    renderer->m_adapter = adapter;

    std::vector<WGPUFeatureName> adapter_features;
    size_t num_features = wgpuAdapterEnumerateFeatures(adapter, nullptr);

    adapter_features.resize(num_features);
    wgpuAdapterEnumerateFeatures(adapter, adapter_features.data());

    for (auto feature : adapter_features) {
        logger::debug("Adapter feature: {:x}", (uint64_t)feature);
    }

    WGPUDeviceDescriptor device_desc = {};
    device_desc.label = "reimu";
    // Required features or limits can go here
    device_desc.defaultQueue.label = "queue";

    WGPUDevice device = create_device(adapter, device_desc);
    if (!device) {
        logger::warn("Failed to create WebGPU device");

        delete renderer;
        return nullptr;
    }
    renderer->m_device = device;

    renderer->m_error_callback = [](WGPUErrorType type, const char *message, void *) {
        logger::fatal("WebGPU device error ({:x}): {}", (uint64_t)type, message);
    };
    wgpuDeviceSetUncapturedErrorCallback(device, renderer->m_error_callback, nullptr);

    auto queue = wgpuDeviceGetQueue(device);
    if (!queue) {
        logger::warn("Failed to get WebGPU queue");

        delete renderer;
        return nullptr;
    }

    renderer->m_cmd_queue = queue;

    renderer->m_queue_callback = [](WGPUQueueWorkDoneStatus status, void *) {
        logger::debug("Queue work done status: {}", (uint64_t)status);
    };
    wgpuQueueOnSubmittedWorkDone(queue, renderer->m_queue_callback, nullptr);

    // Create a swap chain
    if(renderer->create_swap_chain().is_err()) {
        logger::warn("Failed to create WebGPU swap chain");

        delete renderer;
        return nullptr;
    }

    // TODO: remove
    renderer->load_shader("default", R"(
        struct Uniform {
            view_transform: mat4x4f,
            source_region: vec4f,
            target_region: vec4f,
        };

        @group(0) @binding(0) var<uniform> data: Uniform; 
        @group(0) @binding(1) var tex: texture_2d<f32>;

        struct VertexOutput {
            @builtin(position) position: vec4f,
            @location(0) uv : vec2f
        };

        @vertex
        fn vertex_main(@builtin(vertex_index) in_vertex_index: u32) -> VertexOutput {
            var p = vec2f(0.0, 0.0);
            var uv = vec2f(0.0, 0.0);
            if (in_vertex_index == 0u) {
                p = data.target_region.xy;
                uv = data.source_region.xy;
            } else if (in_vertex_index == 1u) {
                p = data.target_region.zy;
                uv = data.source_region.zy;
            } else if (in_vertex_index == 2u) {
                p = data.target_region.xw;
                uv = data.source_region.xw;
            } else {
                p = data.target_region.zw;
                uv = data.source_region.zw;
            }

            return VertexOutput(
                data.view_transform * vec4f(p, 0.0, 1.0),
                uv
            );
        }

        @fragment
        fn fragment_main(in: VertexOutput) -> @location(0) vec4f {
            return textureLoad(tex, vec2i(in.uv), 0).rgba;
        })").ensure();

    window->set_renderer(renderer);

    return renderer;
}

WebGPURenderer::~WebGPURenderer() {
    if (m_cmd_queue) {
        wgpuQueueRelease(m_cmd_queue);
    }

    if (m_swap_chain) {
        wgpuSwapChainRelease(m_swap_chain);
    }

    if (m_device) {
        wgpuDeviceRelease(m_device);
    }

    if (m_adapter) {
        wgpuAdapterRelease(m_adapter);
    }
    
    if (m_surface) {
        wgpuSurfaceRelease(m_surface);
    }

    if (m_instance) {
        wgpuInstanceRelease(m_instance);
    }
}

void WebGPURenderer::render() {
    // Draw the frame
    auto texture_view = wgpuSwapChainGetCurrentTextureView(m_swap_chain);
    if (!texture_view) {
        logger::warn("Failed to get current texture view");
        return;
    }

    for (auto &pass : m_render_passes) {
        WGPUCommandEncoderDescriptor encoder_desc = {};
        encoder_desc.label = "command encoder";

        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(m_device, &encoder_desc);
        assert(encoder);

        pass->render(texture_view, encoder);

        WGPUCommandBufferDescriptor cmd_buffer_desc{};
        cmd_buffer_desc.label = "command buffer";

        WGPUCommandBuffer cmd_buffer = wgpuCommandEncoderFinish(encoder, &cmd_buffer_desc);
        assert(cmd_buffer);

        wgpuCommandEncoderRelease(encoder);

        wgpuQueueSubmit(m_cmd_queue, 1, &cmd_buffer);

        wgpuCommandBufferRelease(cmd_buffer);
    }

    wgpuTextureViewRelease(texture_view);
    wgpuSwapChainPresent(m_swap_chain);
}

Result<void, ReimuError> WebGPURenderer::load_shader(const std::string &name, const char *data) {
    WGPUShaderModuleWGSLDescriptor wgsl_desc = {};
    wgsl_desc.chain = {
        .next = nullptr,
        .sType = WGPUSType_ShaderModuleWGSLDescriptor,
    };

    wgsl_desc.code = data;
    
    WGPUShaderModuleDescriptor shader_desc = {};
    shader_desc.nextInChain = &wgsl_desc.chain;

    WGPUShaderModule shader = wgpuDeviceCreateShaderModule(m_device, &shader_desc);
    if (!shader) {
        logger::warn("Failed to create shader module: {}", name);
        
        return ERR(ReimuError::RendererShaderCompilationFailed);
    }

    m_shaders.insert({name, shader});

    return OK();
}

Result<Texture *, ReimuError> WebGPURenderer::create_texture(const Vector2i &size, ColorFormat format) {
    auto tex_format = webgpu::convert_color_format(format);

    WGPUTextureDescriptor texture_desc = {};
    texture_desc.size.width = size.x;
    texture_desc.size.height = size.y;
    texture_desc.size.depthOrArrayLayers = 1;
    texture_desc.mipLevelCount = 1;
    texture_desc.sampleCount = 1;
    texture_desc.dimension = WGPUTextureDimension_2D;
    texture_desc.format = tex_format;
    texture_desc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;

    WGPUTexture texture = wgpuDeviceCreateTexture(m_device, &texture_desc);
    if (!texture) {
        logger::warn("Failed to create texture");

        return ERR(ReimuError::RendererError);
    }

    WGPUTextureViewDescriptor texture_view_desc = {};
    texture_view_desc.format = tex_format;
    texture_view_desc.dimension = WGPUTextureViewDimension_2D;
    texture_view_desc.baseMipLevel = 0;
    texture_view_desc.mipLevelCount = 1;
    texture_view_desc.baseArrayLayer = 0;
    texture_view_desc.arrayLayerCount = 1;
    texture_view_desc.aspect = WGPUTextureAspect_All;

    WGPUTextureView texture_view = wgpuTextureCreateView(texture, &texture_view_desc);
    if (!texture_view) {
        logger::warn("Failed to create texture view");

        return ERR(ReimuError::RendererError);
    }

    return new WebGPUTexture(*this, format, size, texture, texture_view);
}

Result<RenderPass *, ReimuError> WebGPURenderer::create_render_pass(const BindingDefinition *bindings,
        size_t num_bindings) {
    auto shader_module = m_shaders.at("default");
    assert(shader_module);

    WGPUBindGroupLayout bind_group_layout = nullptr;
    if (num_bindings > 0) {
        // Construct the bind group layout
        std::vector<WGPUBindGroupLayoutEntry> bind_group_layout_entries;
        for (int i = 0; i < num_bindings; i++) {
            auto entry = convert_binding_definition(bindings[i]);

            logger::debug("entry stage: {}", entry.visibility);

            bind_group_layout_entries.push_back(entry);
        }

        WGPUBindGroupLayoutDescriptor bind_group_layout_desc = {};
        bind_group_layout_desc.entryCount = bind_group_layout_entries.size();
        bind_group_layout_desc.entries = bind_group_layout_entries.data();

        bind_group_layout = wgpuDeviceCreateBindGroupLayout(m_device, &bind_group_layout_desc);
        assert(bind_group_layout);
    }

    WGPUPipelineLayoutDescriptor layout_desc = {};
    layout_desc.bindGroupLayoutCount = bind_group_layout ? 1 : 0;
    layout_desc.bindGroupLayouts = &bind_group_layout;

    logger::debug("wgpu pipeline bind group layouts: {}, layout: {:x}",
        layout_desc.bindGroupLayoutCount, (uintptr_t)layout_desc.bindGroupLayouts[0]);

    auto pipeline_layout = wgpuDeviceCreatePipelineLayout(m_device, &layout_desc);
    assert(pipeline_layout);
      
    // Create the pipeline
    WGPURenderPipelineDescriptor pipeline_desc = {};
    pipeline_desc.label = "render pipeline";
    
    pipeline_desc.vertex = {
        .nextInChain = nullptr,
        .module = shader_module,
        .entryPoint = "vertex_main",
        .constantCount = 0,
        .constants = nullptr,
        .bufferCount = 0,
        .buffers = nullptr,
    };

    pipeline_desc.primitive = {
        .nextInChain = nullptr,
        .topology = WGPUPrimitiveTopology_TriangleStrip,
        .stripIndexFormat = WGPUIndexFormat_Undefined,
        .frontFace = WGPUFrontFace_CCW,
        .cullMode = WGPUCullMode_None,
    };

    WGPUBlendState blend_state = {};
    blend_state.color.srcFactor = WGPUBlendFactor_SrcAlpha;
    blend_state.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    blend_state.color.operation = WGPUBlendOperation_Add;

    blend_state.alpha.srcFactor = WGPUBlendFactor_One;
    blend_state.alpha.dstFactor = WGPUBlendFactor_One;
    blend_state.alpha.operation = WGPUBlendOperation_Add;

    WGPUColorTargetState color_target = {};
    color_target.format = SWAP_CHAIN_FORMAT;
    color_target.blend = &blend_state;
    color_target.writeMask = WGPUColorWriteMask_All;

    WGPUFragmentState fragment_state = {};
    fragment_state.module = shader_module;
    fragment_state.entryPoint = "fragment_main";
    fragment_state.constantCount = 0;
    fragment_state.constants = nullptr;

    fragment_state.targetCount = 1;
    fragment_state.targets = &color_target;

    pipeline_desc.fragment = &fragment_state;

    pipeline_desc.depthStencil = nullptr;

    pipeline_desc.multisample.count = 1;
    pipeline_desc.multisample.mask = 0xffffffff;

    pipeline_desc.layout = pipeline_layout;

    auto pipeline = wgpuDeviceCreateRenderPipeline(m_device, &pipeline_desc);
    assert(pipeline);

    wgpuPipelineLayoutRelease(pipeline_layout);

    auto render_pass = new WebGPURenderPass{*this, pipeline, bind_group_layout, bindings, num_bindings};

    render_pass->pipeline = pipeline;

    m_render_passes.emplace(render_pass);

    return OK(render_pass);
}

void WebGPURenderer::resize_viewport(const Vector2i &size) {
    m_viewport_size = size;

    wgpuSwapChainRelease(m_swap_chain);

    create_swap_chain().ensure();
}

ColorFormat WebGPURenderer::display_surface_color_format() const {
    return ColorFormat::RGBA8;
}

void WebGPURenderer::write_texture(const WGPUImageCopyTexture &destination, void const *data,
        size_t dataSize, const WGPUTextureDataLayout &dataLayout, const WGPUExtent3D &writeSize) {
    wgpuQueueWriteTexture(m_cmd_queue, &destination, data, dataSize, &dataLayout, &writeSize);
}

WGPUTexture WebGPURenderer::create_texture_obj(const WGPUTextureDescriptor &desc) {
    return wgpuDeviceCreateTexture(m_device, &desc);
}

WGPUBindGroup WebGPURenderer::create_bind_group(const WGPUBindGroupDescriptor &desc) {
    return wgpuDeviceCreateBindGroup(m_device, &desc);
}

WGPUBuffer WebGPURenderer::create_buffer(const WGPUBufferDescriptor &desc) {
    return wgpuDeviceCreateBuffer(m_device, &desc);
}

void WebGPURenderer::write_buffer(WGPUBuffer buffer, size_t offset, const void *data, size_t size) {
    wgpuQueueWriteBuffer(m_cmd_queue, buffer, offset, data, size);
}

Result<void, ReimuError> WebGPURenderer::create_swap_chain() {
    WGPUSwapChainDescriptor swap_chain_desc = {};
    swap_chain_desc.nextInChain = nullptr;

    swap_chain_desc.width = static_cast<uint32_t>(m_viewport_size.x);
    swap_chain_desc.height = static_cast<uint32_t>(m_viewport_size.y);

    swap_chain_desc.usage = WGPUTextureUsage_RenderAttachment;
    swap_chain_desc.format = SWAP_CHAIN_FORMAT;
    swap_chain_desc.presentMode = WGPUPresentMode_Fifo;

    m_swap_chain = wgpuDeviceCreateSwapChain(m_device, m_surface, &swap_chain_desc);
    if (!m_swap_chain) {
        return ERR(ReimuError::RendererError);
    }

    return OK();
}

WGPUBindGroupLayoutEntry WebGPURenderer::convert_binding_definition(const BindingDefinition &binding) {
    WGPUBindGroupLayoutEntry entry = {};
    entry.binding = binding.index;
    entry.visibility = webgpu::convert_shader_stage(binding.visibility);

    switch (binding.type) {
    case BindingType::UniformBuffer:
        entry.buffer.type = WGPUBufferBindingType_Uniform;
        break;
    case BindingType::Texture:
        // For now we force texture samples to be floats
        entry.texture.nextInChain = nullptr;
        entry.texture.sampleType = WGPUTextureSampleType_Float;
        entry.texture.viewDimension = WGPUTextureViewDimension_2D;
        entry.texture.multisampled = false;
        break;
    default:
        logger::fatal("Unsupported binding type");
    }

    return entry;
}

Result<WGPUSurface, ReimuError> WebGPURenderer::create_bind_window_surface(WGPUInstance instance, video::Window *window) {
    auto win_handle = TRY(window->get_native_handle());

    // Create a WebGPU surface based on the window backend
    switch (win_handle->type) {
    case video::NativeHandleType::Wayland: {
        WGPUSurfaceDescriptorFromWaylandSurface wayland_desc = {};
        wayland_desc.chain.next = nullptr;
        wayland_desc.chain.sType = WGPUSType_SurfaceDescriptorFromWaylandSurface;

        wayland_desc.display = win_handle->wayland.display;
        wayland_desc.surface = win_handle->wayland.surface;

        WGPUSurfaceDescriptor surface_desc;
        surface_desc.nextInChain = &wayland_desc.chain;
        surface_desc.label = NULL;

        return wgpuInstanceCreateSurface(instance, &surface_desc);
    }
    }

    return ERR(ReimuError::RendererUnsupportedWindowBackend);
}

WGPUAdapter WebGPURenderer::create_adapter(WGPUInstance instance, const WGPURequestAdapterOptions &adapter_options) {
    struct AdapterRequest {
        WGPUAdapter adapter = nullptr;
        bool completed = false;
    } adapter_request;

    // Request an adapter using a lambda for the callback
    wgpuInstanceRequestAdapter(instance, &adapter_options,
        [](WGPURequestAdapterStatus s, WGPUAdapter a, char const *, void *data) {
            auto *request = (AdapterRequest *)data;

            if (s == WGPURequestAdapterStatus_Success) {
                request->adapter = a;
            }

            request->completed = true;
        }, &adapter_request);

    assert(adapter_request.completed);

    return adapter_request.adapter;
}

WGPUDevice WebGPURenderer::create_device(WGPUAdapter adapter, const WGPUDeviceDescriptor &device_desc) {
    struct Request {
        WGPUDevice device = nullptr;
        bool completed = false;
    } request;
    
    wgpuAdapterRequestDevice(adapter, &device_desc, [](WGPURequestDeviceStatus s, WGPUDevice d, 
        char const *, void *data) -> void {
            auto *request = (Request *)data;

            if (s == WGPURequestDeviceStatus_Success) {
                request->device = d;
            }

            request->completed = true;
        }, &request);

    assert(request.completed);

    return request.device;
}

}
