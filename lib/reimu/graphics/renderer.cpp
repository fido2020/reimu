#include <reimu/graphics/renderer.h>

#include "webgpu/renderer.h"

namespace reimu::graphics {

Result<Renderer *, ReimuError> create_attach_renderer(video::Window *window) {
    Renderer *r = WebGPURenderer::create(window);
    if (r) {
        return OK(r);
    }

    return ERR(ReimuError::NoSuitableRenderer);
}

}
