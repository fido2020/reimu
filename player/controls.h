#pragma once

#include "reimu/gui/style.h"
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

    virtual long track_progress_us() const = 0;
    virtual long track_duration_us() const = 0;
};

class PlayerControls : public reimu::gui::FlowBox {
public:
    PlayerControls(AudioControlProvider &provider);

    void repaint(reimu::gui::UIPainter &p) override;

private:
    AudioControlProvider &m_provider;

    std::unique_ptr<reimu::gui::Label> m_duration;
    std::unique_ptr<reimu::gui::Canvas> m_progress;

    std::unique_ptr<reimu::gui::GridBox> m_play_controls_box;

    std::unique_ptr<reimu::gui::Button> m_play_button;
    std::unique_ptr<reimu::gui::Widget> m_stop_button;
    std::unique_ptr<reimu::gui::Widget> m_back_button;
    std::unique_ptr<reimu::gui::Widget> m_fwd_button;
    std::unique_ptr<reimu::gui::Widget> m_shuffle_button;
};
