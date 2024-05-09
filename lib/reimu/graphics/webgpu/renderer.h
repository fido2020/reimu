#pragma once

#include <reimu/graphics/renderer.h>
#include <reimu/video/window.h>

#include <webgpu/webgpu.h>

namespace reimu::graphics {

class WebGPURenderer : public Renderer {
public:
    static WebGPURenderer *create(video::Window *window);

    ~WebGPURenderer() override;

    void render() override;

private:
    WebGPURenderer() = default;

    static Result<WGPUSurface, ReimuError> create_bind_window_surface(WGPUInstance instance,
            video::Window *window);
    static WGPUAdapter create_adapter(WGPUInstance instance,
            const WGPURequestAdapterOptions &adapter_options);
    static WGPUDevice create_device(WGPUAdapter adapter, const WGPUDeviceDescriptor &device_desc);

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
