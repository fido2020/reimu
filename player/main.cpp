#include "reimu/core/error.h"
#include "reimu/core/logger.h"
#include "reimu/core/result.h"
#include "reimu/gui/layout.h"
#include "reimu/gui/widget.h"
#include <reimu/gui/window.h>
#include <reimu/video/video.h>
#include <memory>

#include "controls.h"
#include "device.h"

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

    register_devices();

    auto dev = default_audio_device().ensure();
    reimu::logger::debug("Using default audio device: {}", dev->name());

    auto ctx = dev->connect({
        AudioSampleFormat::Float32,
        48000,
        2
    }).ensure();

    reimu::logger::debug("Connected to audio device with format: {} Hz, {} channels, {}", 
        ctx->get_sample_format().sample_rate, 
        ctx->get_sample_format().channels, 
        (int)ctx->get_sample_format().sample_format);

    ctx->start_playback();

    // A note (440hz) sine wave, 2 channels, normalized -1 to 1
    char sine[48000 * 4];
    for (size_t i = 0; i < sizeof(sine); i += 8) {
        float sample = sinf(2.0f * M_PI * (i / 8.0f) / 48000.0f * 440.0f);
        memcpy(sine + i, &sample, sizeof(float));
        memcpy(sine + i + 4, &sample, sizeof(float));
    }

    ctx->queue_frames(sine, sizeof(sine) / 8);

    MediaPlayerApp app;
    app.run();

    return 0;
}
