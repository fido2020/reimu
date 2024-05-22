#include <reimu/video/video.h>
#include <reimu/video/driver.h>
#include <reimu/core/event.h>
#include <reimu/gui/widget.h>
#include <reimu/gui/window.h>

#include <list>

using namespace reimu;

int main() {
    video::init();

    auto win = gui::Window::create(Vector2i(800, 600)).ensure();

    auto *root = &win->root();
    root->layout.layout_direction = gui::LayoutDirection::Horizontal;

    auto btn1 = new gui::Button();
    auto btn2 = new gui::Button();
    auto btn3 = new gui::Button();
    auto btn4 = new gui::Button();
    auto btn5 = new gui::Button();

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

    auto event_loop = EventLoop::create().ensure();
    event_loop->watch_os_handle(video::get_driver()->get_window_client_handle(), []() {
        video::get_driver()->window_client_dispatch();
    });

    win->render();
    event_loop->run();

    return 0;
}
