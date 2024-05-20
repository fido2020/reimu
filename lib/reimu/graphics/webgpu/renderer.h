#pragma once

#include <reimu/graphics/renderer.h>
#include <reimu/video/window.h>

#include <map>
#include <set>

#include "render_pass.h"
#include "webgpu.h"

namespace reimu::graphics {

class WebGPURenderer : public Renderer {
public:
    static WebGPURenderer *create(video::Window *window);

    ~WebGPURenderer() override;

    void render() override;
    Result<void, ReimuError> load_shader(const std::string &name, const char *data) override;
    Result<RenderPass *, ReimuError> create_render_pass(const BindingDefinition *bindings,
        size_t num_bindings) override;
    Result<Texture *, ReimuError> create_texture(const Vector2i &size, ColorFormat color_format)
        override;

    void resize_viewport(const Vector2i &size) override;
    ColorFormat display_surface_color_format() const override;
    
    void on_destroy_render_pass(RenderPass *render_pass);
    void write_texture(const WGPUImageCopyTexture &destination, void const *data, size_t dataSize,
        const WGPUTextureDataLayout &dataLayout, const WGPUExtent3D &writeSize);

    WGPUTexture create_texture_obj(const WGPUTextureDescriptor &desc);
    WGPUBindGroup create_bind_group(const WGPUBindGroupDescriptor &desc);
    WGPUBuffer create_buffer(const WGPUBufferDescriptor &desc);
    void write_buffer(WGPUBuffer buffer, size_t offset, const void *data, size_t size);

private:
    WebGPURenderer() = default;

    Result<void, ReimuError> create_swap_chain();

    static WGPUBindGroupLayoutEntry convert_binding_definition(const BindingDefinition &binding);

    static Result<WGPUSurface, ReimuError> create_bind_window_surface(WGPUInstance instance,
            video::Window *window);
    static WGPUAdapter create_adapter(WGPUInstance instance,
            const WGPURequestAdapterOptions &adapter_options);
    static WGPUDevice create_device(WGPUAdapter adapter, const WGPUDeviceDescriptor &device_desc);

    std::map<std::string, WGPUShaderModule> m_shaders;
    std::set<WebGPURenderPass *> m_render_passes;

    WGPUInstance m_instance = nullptr;
    WGPUSurface m_surface = nullptr;
    WGPUAdapter m_adapter = nullptr;
    WGPUDevice m_device = nullptr;
    WGPUSwapChain m_swap_chain = nullptr;

    WGPUQueue m_cmd_queue = nullptr;

    WGPUErrorCallback m_error_callback;
    WGPUQueueWorkDoneCallback m_queue_callback;
};

} 
