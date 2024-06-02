#include <reimu/video/video.h>
#include <reimu/video/driver.h>
#include <reimu/core/event.h>
#include <reimu/gui/widget.h>
#include <reimu/gui/window.h>

#include <list>

using namespace reimu;

int main() {
    video::init();

    auto win = gui::Window::create(Vector2i(640, 480)).ensure();

    auto *root = &win->root();
    root->layout.layout_direction = gui::LayoutDirection::Horizontal;

    auto btn1 = new gui::Button();
    auto btn2 = new gui::Button();
    auto btn3 = new gui::Button();
    auto btn4 = new gui::Button();
    auto btn5 = new gui::Button();

    btn1->layout.set_padding(gui::Size::from_pixels(5));
    btn2->layout = btn1->layout;
    btn3->layout = btn1->layout;
    btn4->layout = btn1->layout;
    btn5->layout = btn1->layout;

    auto *grid = new gui::GridBox();

    grid->layout.width = gui::Size::from_percent(1);
    grid->layout.height = gui::Size::from_percent(1);

    root->add_child(grid);

    grid->add_row(gui::Size::from_percent(0.3));

    grid->add_item(btn1, gui::Size::from_layout_factor(1));
    grid->add_item(btn2, gui::Size::from_layout_factor(2));

    grid->add_row(gui::Size::from_pixels(50));

    grid->add_item(btn3, gui::Size::from_percent(0.5));
    grid->add_item(btn4, gui::Size::from_layout_factor(1));
    grid->add_item(btn5, gui::Size::from_layout_factor(1));

    auto event_loop = EventLoop::create().ensure();
    event_loop->watch_os_handle(video::get_driver()->get_window_client_handle(), [&win, &event_loop]() {
        video::get_driver()->window_client_dispatch();

        if (!win->is_open()) {
            event_loop->end();
        }
    });

    win->render();
    event_loop->run();

    return 0;
}
