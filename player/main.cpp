#include "reimu/core/error.h"
#include "reimu/core/event.h"
#include "reimu/core/logger.h"
#include "reimu/core/result.h"
#include "reimu/gui/layout.h"
#include "reimu/gui/widget.h"
#include <reimu/gui/window.h>
#include <reimu/video/video.h>
#include <reimu/video/driver.h>
#include <reimu/os/fs.h>
#include <algorithm>

#include "controls.h"
#include "device.h"
#include "decoder.h"

#if defined(_WIN32)

#include <windows.h>
#include <commdlg.h>

#include <codecvt>
#include <locale>

std::string open_file() {
    WCHAR path[MAX_PATH] = {};
    OPENFILENAMEW ofn = {sizeof(ofn)};
    ofn.hwndOwner = HWND_DESKTOP;
    ofn.lpstrFilter = L"Audio files\0*.mp3;*.wav;*.flac;*.m4a;*.ogg;*.opus\0All Files\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;


    BOOL ok = GetOpenFileNameW(&ofn);
    if (ok) {
        return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().to_bytes(path);
    }

    return {};
}

#else

std::string open_file() {
    return "test.wav";
}

#endif

class AudioControlProviderImpl : public AudioControlProvider {
public:
    AudioControlProviderImpl(Decoder &decoder, AudioContext &ctx)
        : m_decoder(decoder), m_ctx(ctx) {}

    void set_loop(reimu::EventLoop *loop) {
        m_event_loop = loop;
    }

    void play() override {
        m_ctx.start_playback();
        m_decoder.start();
    }

    void pause() override {
        m_decoder.stop();
        m_ctx.stop_playback();
    }

    void stop() override {}
    void seek(float position) override {
        m_ctx.stop_playback();
        m_decoder.seek(position);
        m_ctx.clear_queue();
    }
    void set_volume(float volume) override {}

    bool is_playing() const override { return m_ctx.is_playing(); }
    bool is_paused() const override { return false; }

    float volume() const override { return 1.0f; }

    long track_progress_us() const override { return saved_progress_us; }
    void set_track_progress_us(long progress) {
        if (saved_progress_us / 100000 != progress / 100000) {
            saved_progress_us = progress;

            if (m_event_loop) {
                m_event_loop->dispatch_event("audio_status_changed"_hashid);
            }
        }
    }

    long track_duration_us() const override {
        return m_decoder.track_duration_us();
    }

private:
    long saved_progress_us;
    Decoder &m_decoder;
    AudioContext &m_ctx;

    reimu::EventLoop *m_event_loop = nullptr;
};

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

        delete m_event_loop;
    }

    void run(AudioControlProviderImpl &p) {
        auto control_box = std::make_unique<PlayerControls>(
            p
        );

        p.set_loop(m_event_loop);

        m_main_window->set_title("reimu-player");
        m_main_window->render();

        m_main_window->set_size({
            400, 200
        });

        control_box->layout.width = reimu::gui::Size::from_percent(1);
        control_box->layout.height = reimu::gui::Size::from_percent(1);

        m_main_window->root().add_child(control_box.get());

        m_event_loop->watch_os_handle(reimu::video::get_driver()->get_window_client_handle(), [this]() {
            reimu::video::get_driver()->window_client_dispatch();

            if (!m_main_window->is_open()) {
                m_event_loop->end();
            }
        });

        m_event_loop->bind_event_callback("audio_status_changed"_hashid, [&]() {
            control_box->dispatch_event("ui_repaint"_hashid);
            m_main_window->render();
        });

        m_event_loop->run();

        m_main_window = nullptr;
    }

private:
    std::unique_ptr<reimu::gui::Window> m_main_window;

    reimu::EventLoop *m_event_loop = reimu::EventLoop::create().ensure();
};

int main(int argc, char **argv) {
    std::string path = "test.wav";
    if (argc > 1) {
        path = argv[1];
    } else {
        path = open_file();
    }

    reimu::logger::debug("Selected file: {}", path);

    std::shared_ptr<reimu::File> audio_file = reimu::os::open(path, reimu::FileMode::ReadOnly)
        .ensure();

    reimu::video::init();

    register_devices();

    auto dev = default_audio_device().ensure();
    reimu::logger::debug("Using default audio device: {}", dev->name());

    auto ctx = dev->connect({
        AudioSampleFormat::Signed16,
        48000,
        2
    }).ensure();

    auto fmt = ctx->get_sample_format();

    auto decode = Decoder{fmt};
    auto prov = AudioControlProviderImpl{decode, *ctx};

    decode.on_decoded_data = [&](const uint8_t *data, size_t samples_per_channel, long timestamp_us, bool invalidate_buffer) {
        if (invalidate_buffer) {
            ctx->clear_queue();
            ctx->start_playback();
        }

        if (!data)
            return;

        auto chunk = data;
        auto bytes_per_frame = fmt.channels * fmt.sample_size();
        auto end = data + samples_per_channel * bytes_per_frame;
        auto samples_left = samples_per_channel;

        while (chunk < end) {
            if (decode.has_pending_seek()) {
                reimu::logger::debug("Flushing decode buffer due to seek");
                break;
            }

            auto chunk_samples = std::min(128, (int)samples_left);
            auto ts_diff = (samples_per_channel - samples_left) * 1000000 / fmt.sample_rate;

            ctx->queue_frames(chunk, chunk_samples);
            prov.set_track_progress_us(
                ctx->get_current_timestamp(timestamp_us + ts_diff));

            chunk += chunk_samples * bytes_per_frame;
            samples_left -= chunk_samples;       
        }
    };

    reimu::logger::debug("Connected to audio device with format: {} Hz, {} channels, {}", 
        ctx->get_sample_format().sample_rate, 
        ctx->get_sample_format().channels, 
        (int)ctx->get_sample_format().sample_format);

    ctx->start_playback();
    decode.load(
        audio_file
    ).ensure();

    decode.start();

    MediaPlayerApp app;
    app.run(prov);

    return 0;
}
