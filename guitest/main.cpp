#include <reimu/graphics/renderer.h>
#include <reimu/graphics/matrix.h>
#include <reimu/graphics/transform.h>
#include <reimu/video/window.h>
#include <reimu/video/video.h>

#include <list>

struct UBO {
    reimu::Matrix4 view;
    reimu::Vector4f source;
    reimu::Vector4f target;
};

class Compositor : public reimu::graphics::RenderStrategy {
    struct Clip {
        reimu::Vector4f source;
        reimu::Vector4f dest;
        
        reimu::graphics::Texture *tex;
    };

public:
    Compositor() : tex(tex) {}

    void add_clip(reimu::graphics::Texture *tex, reimu::Vector4f source, reimu::Vector2f dest) {
        m_clips.push_back(Clip {
            .source = source,
            .dest = {dest.x, dest.y, source.z - source.x + dest.x, source.w - source.y + dest.y},
            .tex = tex
        });
    }

    reimu::graphics::Texture *tex;

    void draw(reimu::graphics::Renderer &renderer, reimu::graphics::RenderPass &pass) override {
        auto viewport_size = renderer.get_viewport_size();
        auto view_transform = reimu::Matrix4();
        
        reimu::logger::debug("viewport: {}x{}", viewport_size.x, viewport_size.y);

        view_transform.translate(-1.f, 1.f);
        view_transform.scale(2.0f / viewport_size.x, -2.0f / viewport_size.y);

        struct UBO ubo = {
            .view = view_transform,
            .source = {0.0f, 0.0f, 200.0f, 200.0f},
            .target = {0.0f, 0.0f, 200.0f, 200.0f},
        };

        for (auto &clip : m_clips) {
            pass.bind_texture(1, clip.tex);

            ubo.source = clip.source;
            ubo.target = clip.dest;
            pass.bind_uniform_buffer(0, &ubo, sizeof(ubo));

            pass.draw(4);
        }
    };

private:
    std::list<Clip> m_clips;
};

int main() {
    reimu::video::init();

    auto win = reimu::video::Window::make({800, 600}).set_title("ハーロー").create().ensure();

    auto renderer = reimu::graphics::create_attach_renderer(win).ensure();

    usleep(500000);

    reimu::graphics::BindingDefinition bindings[] = {
        {
            .uniform_buffer = {
                .size = sizeof(struct UBO)
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

    auto tex = renderer->create_texture({400, 400}, reimu::graphics::ColorFormat::RGBA8).ensure();

    uint32_t buf[400 * 400];
    srand(time(NULL));

    for (int i = 0; i < 400; i++) {
        for (int j = 0; j < 400; j++) {
            uint32_t r = (rand() % 256) * (i / 400.0);
            uint32_t g = (rand() % 256) * (j / 400.0 );
            uint32_t b = (rand() % 256) * ((400 - i) / 400.0);

            buf[i * 400 + j] = 0xff000000 | (b << 16) | (g << 8) | r;
        }
    }

    tex->update(buf, 400*400*4);

    auto comp = std::make_shared<Compositor>();

    comp->add_clip(tex, {0, 0, 200, 200}, {0, 0});
    comp->add_clip(tex, {200, 200, 400, 400}, {150, 300});

    render_pass->set_strategy(comp);

    win->sync_window();

    for(;;) {
        renderer->render();

        usleep(1000000 / 60);
    }

}
