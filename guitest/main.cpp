#include <reimu/video/window.h>
#include <reimu/video/video.h>

int main() {
    reimu::video::init();

    auto win = reimu::video::Window::make({800, 600}).set_title("ハーロー").create().ensure();
}
