#pragma once

#include <reimu/graphics/matrix.h>
#include <reimu/graphics/render_pass.h>
#include <reimu/graphics/rect.h>
#include <reimu/gui/widget.h>

#include <list>

namespace reimu::gui {

class Compositor : public graphics::RenderStrategy {
public:
    struct Clip {
        Recti source;
        Vector2i dest;

        graphics::Texture *tex;
    };

    struct UBO {
        reimu::Matrix4 view;
        reimu::Vector4f source;
        reimu::Vector4f target;
    };

    Compositor(graphics::Renderer *renderer);

    void draw(graphics::Renderer &renderer, graphics::RenderPass &pass) override;

    inline AddClipFn get_add_clip_fn() {
        return m_add_clip_fn;
    }

    inline void clear_clips() {
        m_clips.clear();
    }

private:
    AddClipFn m_add_clip_fn = [this](Recti src, Vector2i dest, graphics::Texture *tex) {
        m_clips.push_back(Clip {
            .source = src,
            .dest = dest,
            .tex = tex
        });
    };

    std::list<Clip> m_clips;
    std::unique_ptr<graphics::RenderPass> m_render_pass;
};

}

