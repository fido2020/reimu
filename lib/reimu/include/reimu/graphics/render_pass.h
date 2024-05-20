#pragma once

#include <stddef.h>

#include <memory>

namespace reimu::graphics {

class Texture;
class Renderer;

enum class BindingType {
    UniformBuffer,
    Texture,
};

enum class ShaderStage {
    Vertex,
    Fragment
};

struct BindingDefinition {
    union {
        struct {
            size_t size;
        } uniform_buffer;
    };

    ShaderStage visibility;
    BindingType type;
    int index;
};

class RenderStrategy;

class RenderPass {
public:
    virtual ~RenderPass() = default;

    virtual void bind_texture(int index, Texture *texture) = 0;
    virtual void bind_uniform_buffer(int index, const void *data, size_t size) = 0;
    virtual void draw(int num_vertices) = 0;

    virtual void set_strategy(RenderStrategy *strategy) = 0;
};

class RenderStrategy {
public:
    virtual void draw(Renderer &renderer, RenderPass &pass) = 0;
};

}
