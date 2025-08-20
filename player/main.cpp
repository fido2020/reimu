#include "reimu/core/error.h"
#include "reimu/core/result.h"
#include "reimu/gui/layout.h"
#include "reimu/gui/widget.h"
#include <reimu/gui/window.h>
#include <reimu/video/video.h>
#include <memory>

template<typename T, typename E>
using Result = reimu::Result<T, E>;

const auto control_btn_layout = reimu::gui::LayoutProperties{
    .width = reimu::gui::Size::from_em(2.5f),
    .height = reimu::gui::Size::from_em(2.5f)
};

class MediaPlayerApp {
public:
    MediaPlayerApp() {
        m_main_window = std::unique_ptr<reimu::gui::Window>{
            reimu::gui::Window::create({400, 400})
                .ensure()
        };
    }

    ~MediaPlayerApp() {
        m_main_window->close();
    }

    void run() {
        auto btn_play = std::make_unique<reimu::gui::Button>();
        btn_play->layout = control_btn_layout;

        auto btn_prev = std::make_unique<reimu::gui::Button>();
        btn_prev->layout = control_btn_layout;

        auto btn_next = std::make_unique<reimu::gui::Button>();
        btn_next->layout = control_btn_layout;

        auto btn_shuffle = std::make_unique<reimu::gui::Button>();
        btn_shuffle->layout = control_btn_layout;

        auto control_box = std::make_unique<reimu::gui::FlowBox>();
        control_box->layout.layout_direction = reimu::gui::LayoutDirection::Horizontal;

        control_box->add_child(btn_play.get());
        control_box->add_child(btn_prev.get());
        control_box->add_child(btn_next.get());
        control_box->add_child(btn_shuffle.get());

        m_main_window->root().add_child(control_box.get());

        m_main_window->run_until_close();

        m_main_window->close();
        m_main_window = nullptr;
    }

private:
    std::unique_ptr<reimu::gui::Window> m_main_window;
};

int main() {
    reimu::video::init();

    MediaPlayerApp app;
    app.run();

    return 0;
}
