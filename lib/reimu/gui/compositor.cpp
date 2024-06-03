#include "compositor.h"

namespace reimu::gui {

Compositor::Compositor(graphics::Renderer *renderer) {
    reimu::graphics::BindingDefinition bindings[] = {
        {
            .uniform_buffer = {
                .size = sizeof(UBO)
            },
            .visibility = reimu::graphics::ShaderStage::Vertex,
            .type = reimu::graphics::BindingType::UniformBuffer,
            .index = 0,
        },
        {
            .visibility = reimu::graphics::ShaderStage::Fragment,
            .type = reimu::graphics::BindingType::Texture,
            .index = 1,
        }
    };
    
    auto render_pass = renderer->create_render_pass(bindings, 2).ensure();

    m_render_pass = std::unique_ptr<graphics::RenderPass>(render_pass);
    m_render_pass->set_strategy(this);
}

void Compositor::draw(graphics::Renderer &renderer, graphics::RenderPass &pass) {
    auto viewport_size = renderer.get_viewport_size();
    auto view_transform = reimu::Matrix4();
    
    view_transform.translate(-1.f, 1.f);
    view_transform.scale(2.0f / viewport_size.x, -2.0f / viewport_size.y);

    UBO ubo;
    ubo.view = view_transform;

    for (const auto &clip : m_clips) {
        Vector4i source = clip.source;
        Vector2i dest = clip.dest;

        ubo.source = vector_static_cast<float>(source);
        ubo.target = vector_static_cast<float>(Recti{dest.x, dest.y, source.z - source.x + dest.x, source.w - source.y + dest.y});

        pass.bind_uniform_buffer(0, &ubo, sizeof(ubo));
        pass.bind_texture(1, clip.tex);

        pass.draw(4);
    }
}

}
