#include <reimu/graphics/renderer.h>
#include <reimu/video/window.h>
#include <reimu/video/video.h>

int main() {
    reimu::video::init();

    auto win = reimu::video::Window::make({800, 600}).set_title("ハーロー").create().ensure();

    auto renderer = reimu::graphics::create_attach_renderer(win).ensure();

    usleep(500000);

    reimu::graphics::BindingDefinition texture_binding;
    texture_binding.index = 0;
    texture_binding.type = reimu::graphics::BindingType::Texture;
    texture_binding.visibility = reimu::graphics::ShaderStage::Fragment;

    auto render_pass = renderer->create_render_pass(&texture_binding, 1).ensure();

    auto tex = renderer->create_texture({400, 400}, reimu::graphics::ColorFormat::RGBA8).ensure();

    uint32_t buf[400 * 400];

    for (int i = 0; i < 400; i++) {
        uint32_t val = rand();

        for (int j = 0; j < 400; j++) {
            buf[i * 400 + j] = val * rand();
        }
    }

    tex->update(buf, 400*400*4);

    class Strategy : public reimu::graphics::RenderStrategy {
    public:
        Strategy(reimu::graphics::Texture *tex) : tex(tex) {}

        reimu::graphics::Texture *tex;

        void draw(reimu::graphics::RenderPass &pass) override {
            pass.bind_texture(0, tex);

            pass.draw(4);
        };
    };

    render_pass->set_strategy(std::make_shared<Strategy>(tex));

    win->sync_window();
    renderer->render();

    for(;;);

}
