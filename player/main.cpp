#include "reimu/core/error.h"
#include "reimu/core/result.h"
#include "reimu/gui/layout.h"
#include "reimu/gui/widget.h"
#include <reimu/gui/window.h>
#include <reimu/video/video.h>
#include <memory>

#include "controls.h"

class FakeAudioControlProvider : public AudioControlProvider {
    void play() override {}
    void pause() override {}
    void stop() override {}
    void seek(float position) override {}
    void set_volume(float volume) override {}

    bool is_playing() const override { return false; }
    bool is_paused() const override { return false; }

    float volume() const override { return 1.0f; }

    long track_progress_ms() const override { return 42069; }
    long track_duration_ms() const override { return 60000; }
};

FakeAudioControlProvider control_provider;

template<typename T, typename E>
using Result = reimu::Result<T, E>;

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
        auto control_box = std::make_unique<PlayerControls>(
            static_cast<AudioControlProvider&>(control_provider)
        );

        control_box->layout.width = reimu::gui::Size::from_percent(1);
        control_box->layout.height = reimu::gui::Size::from_percent(1);

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
