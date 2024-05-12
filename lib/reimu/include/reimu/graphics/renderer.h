#pragma once

#include <reimu/core/result.h>
#include <reimu/graphics/render_pass.h>
#include <reimu/graphics/texture.h>
#include <reimu/graphics/vector.h>

namespace reimu::video {

class Window;

}

namespace reimu::graphics {

class Renderer {
public:
    virtual ~Renderer() = default;

    /**
     * @brief Dispatch render queue and swap buffers
     */
    virtual void render() = 0;

    /**
     * @brief Load shader from provided data
     * 
     * @param name Name used to identify shader
     * @param data Renderer dependent shader code
     */
    virtual Result<void, ReimuError> load_shader(const std::string& name, const char *data) = 0;

    /**
     * @brief Create a new render pass
     * 
     * @return Result<RenderPass *, ReimuError>
     */
    virtual Result<RenderPass *, ReimuError> create_render_pass(const BindingDefinition *bindings, size_t num_bindings) = 0;

    /**
     * @brief Create a new texture
     * 
     * @param size
     * @param format
     * @return Result<Texture *, ReimuError>
     */
    virtual Result<Texture *, ReimuError> create_texture(const Vector2i& size, ColorFormat color_format) = 0;
};

Result<Renderer *, ReimuError> create_attach_renderer(video::Window *window);

} // namespace reimu::graphics
