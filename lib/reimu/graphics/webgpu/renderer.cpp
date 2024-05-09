#include "renderer.h"

#include <reimu/core/logger.h>

#include <assert.h>
#include <vector>

#include "webgpu.h"

namespace reimu::graphics {

WebGPURenderer *WebGPURenderer::create(video::Window *window) {
    WGPUInstanceDescriptor desc = {};
    desc.nextInChain = nullptr;

    auto *renderer = new WebGPURenderer;

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
    WGPUSwapChainDescriptor swap_chain_desc = {};
    swap_chain_desc.nextInChain = nullptr;

    swap_chain_desc.width = static_cast<uint32_t>(window->get_size().x);
    swap_chain_desc.height = static_cast<uint32_t>(window->get_size().y);

    swap_chain_desc.usage = WGPUTextureUsage_RenderAttachment;
    swap_chain_desc.format = WGPUTextureFormat_BGRA8Unorm;
    swap_chain_desc.presentMode = WGPUPresentMode_Fifo;

    renderer->m_swap_chain = wgpuDeviceCreateSwapChain(device, renderer->m_surface, &swap_chain_desc);
    if (!renderer->m_swap_chain) {
        logger::warn("Failed to create WebGPU swap chain");

        delete renderer;
        return nullptr;
    }

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

    WGPUCommandEncoderDescriptor encoder_desc = {};
    encoder_desc.label = "command encoder";

    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(m_device, &encoder_desc);
    assert(encoder);

    WGPURenderPassColorAttachment color_attachment = {};
    color_attachment.view = texture_view;
    color_attachment.loadOp = WGPULoadOp_Clear;
    color_attachment.storeOp = WGPUStoreOp_Store;
    color_attachment.clearValue = {1.0f, 0.5f, 0.0f, 1.0f};

    WGPURenderPassDescriptor pass_desc = {};
    pass_desc.colorAttachmentCount = 1;
    pass_desc.colorAttachments = &color_attachment;
    pass_desc.depthStencilAttachment = nullptr;

    auto pass_encoder = wgpuCommandEncoderBeginRenderPass(encoder, &pass_desc);

    wgpuRenderPassEncoderEnd(pass_encoder);
    wgpuRenderPassEncoderRelease(pass_encoder);

    WGPUCommandBuffer cmd_buffer = wgpuCommandEncoderFinish(encoder, nullptr);
    assert(cmd_buffer);

    wgpuCommandEncoderRelease(encoder);

    wgpuQueueSubmit(m_cmd_queue, 1, &cmd_buffer);

    wgpuCommandBufferRelease(cmd_buffer);
    wgpuCommandEncoderRelease(encoder);

    wgpuSwapChainPresent(m_swap_chain);
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
        [](WGPURequestAdapterStatus s, WGPUAdapter a, char const *msg, void *data) {
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
        char const *msg, void *data) -> void {
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
