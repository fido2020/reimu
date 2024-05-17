#include <reimu/graphics/renderer.h>
#include <reimu/graphics/matrix.h>
#include <reimu/graphics/transform.h>
#include <reimu/video/window.h>
#include <reimu/video/video.h>

#include <reimu/gui/widget.h>

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
    Compositor(reimu::graphics::Texture *tex) : tex(tex) {}

    void add_clip(reimu::graphics::Texture *tex, reimu::Vector4f source, reimu::Vector2f dest) {
        m_clips.push_back(Clip {
            .source = source,
            .dest = {dest.x, dest.y, source.z - source.x + dest.x, source.w - source.y + dest.y},
            .tex = tex
        });
    }

    void clear_clips() {
        m_clips.clear();
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
            reimu::logger::debug("binding ({}x{})", clip.tex->size().x, clip.tex->size().y);
            pass.bind_texture(1, clip.tex);
            //pass.bind_texture(1, tex);
            reimu::logger::debug("done binding");

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

    auto create_texture_fn = [renderer](reimu::Vector2i size) -> reimu::graphics::Texture *{
        return renderer->create_texture(size, reimu::graphics::ColorFormat::RGBA8).ensure();
    };


    auto tex = renderer->create_texture({100,100}, reimu::graphics::ColorFormat::RGBA8).ensure();
    tex->replace(reimu::graphics::ColorFormat::RGBA8, {800, 600});

    auto comp = std::make_shared<Compositor>(tex);

    using namespace reimu;
    auto add_clip_fn = [comp](const Rectf &src, const Vector2f &dest, graphics::Texture *tex) {
        reimu::logger::debug("adding clip: {} {} {} {}", dest.x, dest.y, dest.x + src.z, dest.y + src.w);
        comp->add_clip(tex, src, dest);
    };

    auto root = new gui::RootContainer(create_texture_fn, vector_static_cast<float>(renderer->get_viewport_size()));
    root->layout.layout_direction = gui::LayoutDirection::Horizontal;

    auto btn1 = new gui::Button(create_texture_fn);
    auto btn2 = new gui::Button(create_texture_fn);
    auto btn3 = new gui::Button(create_texture_fn);
    auto btn4 = new gui::Button(create_texture_fn);
    auto btn5 = new gui::Button(create_texture_fn);

    btn1->layout.width = gui::Size::from_pixels(200);
    btn1->layout.height = gui::Size::from_pixels(50);
    btn1->layout.set_padding(gui::Size::from_pixels(5));
    btn2->layout = btn1->layout;
    btn3->layout = btn1->layout;
    btn4->layout = btn1->layout;
    btn5->layout = btn1->layout;

    root->add_child(btn1);
    root->add_child(btn2);
    root->add_child(btn3);
    root->add_child(btn4);
    root->add_child(btn5);

    auto ui_painter = gui::UIPainter{};

    root->update_layout(vector_static_cast<float>(renderer->get_viewport_size()));
    root->repaint(ui_painter);

    root->add_clips(add_clip_fn);

    render_pass->set_strategy(comp);

    win->sync_window();

    for(;;) {
        renderer->render();

        usleep(1000000);
        //usleep(1000000 / 60);
    }

}
