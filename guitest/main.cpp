#include <reimu/graphics/renderer.h>
#include <reimu/video/window.h>
#include <reimu/video/video.h>

int main() {
    reimu::video::init();

    auto win = reimu::video::Window::make({800, 600}).set_title("ハーロー").create().ensure();

    auto renderer = reimu::graphics::create_attach_renderer(win).ensure();

    usleep(500000);
    
    win->sync_window();
    renderer->render();

    usleep(1000000);

}
