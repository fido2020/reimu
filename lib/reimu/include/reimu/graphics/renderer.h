#pragma once

#include <reimu/core/result.h>

namespace reimu::video {

class Window;

}

namespace reimu::graphics {

class RenderStrategy {
    virtual ~RenderStrategy() = default;

    virtual void draw(class Renderer *renderer) = 0;
};

class Renderer {
public:
    virtual ~Renderer() = default;

    /**
     * @brief Dispatch render queue and swap buffers
     */
    virtual void render() = 0;
};

Result<Renderer *, ReimuError> create_attach_renderer(video::Window *window);

} // namespace reimu::graphics
