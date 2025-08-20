#pragma once

#include <memory>
#include <reimu/gui/widget.h>

class AudioControlProvider {
public:
    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual void seek(float position) = 0;
    virtual void set_volume(float volume) = 0;

    virtual bool is_playing() const = 0;
    virtual bool is_paused() const = 0;

    virtual float volume() const = 0;

    virtual long track_progress_ms() const = 0;
    virtual long track_duration_ms() const = 0;
};

class PlayerControls : public reimu::gui::FlowBox {
public:
    PlayerControls(AudioControlProvider &provider);

private:
    AudioControlProvider &m_provider;

    std::unique_ptr<reimu::gui::Canvas> m_duration;

    std::unique_ptr<reimu::gui::GridBox> m_play_controls_box;

    std::unique_ptr<reimu::gui::Widget> m_play_button;
    std::unique_ptr<reimu::gui::Widget> m_stop_button;
    std::unique_ptr<reimu::gui::Widget> m_back_button;
    std::unique_ptr<reimu::gui::Widget> m_fwd_button;
    std::unique_ptr<reimu::gui::Widget> m_shuffle_button;
};
