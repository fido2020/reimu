#include "controls.h"
#include "reimu/core/logger.h"
#include "reimu/graphics/painter.h"
#include <reimu/gui/window.h>
#include "reimu/gui/layout.h"
#include "reimu/gui/widget.h"
#include <memory>

const auto control_btn_layout = []() consteval  {
    auto layout = reimu::gui::LayoutProperties{
        .width = reimu::gui::Size::from_em(3.f),
        .height = reimu::gui::Size::from_em(2.f),
    };

    layout.set_y_padding(reimu::gui::Size::from_em(0.5f));
    layout.set_x_padding(reimu::gui::Size::from_em(0.25f));
    return layout;
}();

PlayerControls::PlayerControls(AudioControlProvider &provider)
    : m_provider(provider) {
    layout.layout_direction = reimu::gui::LayoutDirection::Vertical;
    layout.set_padding(reimu::gui::Size::from_em(0.5f));
    
    m_duration = std::make_unique<reimu::gui::Label>();
    m_duration->set_text("00:00/00:00");
    m_duration->layout = reimu::gui::LayoutProperties {
        .width = reimu::gui::Size::from_em(4.f),
        .height = reimu::gui::Size::from_em(1.5f),
    };

    m_duration->layout.set_y_padding(reimu::gui::Size::from_em(0.5f));

    m_progress = std::make_unique<reimu::gui::Canvas>();
    auto progress_layout = reimu::gui::LayoutProperties {
        .width = reimu::gui::Size::from_layout_factor(1.f),
        .height = reimu::gui::Size::from_em(1.5f),
    };
    progress_layout.left_padding = (reimu::gui::Size::from_em(.5f));
    progress_layout.set_y_padding(reimu::gui::Size::from_em(1.25f));

    m_progress->layout = progress_layout;

    m_progress->on_paint = [this](reimu::graphics::Painter &painter) {
        float progress = m_provider.track_progress_us() / static_cast<float>(m_provider.track_duration_us());
        float filled_width = painter.surface_size().x * progress;
        float height = painter.surface_size().y;

        painter.draw_rect({0, 0, filled_width, height}, reimu::Color::from_rgb_hex(0x00FF00));
        painter.draw_rect({filled_width, 0, (float)painter.surface_size().x, height}, reimu::Color::from_rgb_hex(0x000000));
    };

    m_progress->bind_event_callback("on_mouse_down"_hashid, [this]() {
        float seek = (m_window->pointer().x - m_progress->bounds.x) / (float)m_progress->bounds.width()
            * m_provider.track_duration_us() / 1'000'000.f;
        m_provider.seek(seek);
    });

    m_play_button = std::make_unique<reimu::gui::Button>();
    m_play_button->layout = control_btn_layout;
    m_play_button->label = "Play";
    m_play_button->bind_event_callback("on_click"_hashid, [this]() {
        if (!m_provider.is_playing()) {
            m_provider.play();
        } else {
            m_provider.pause();
        }
    });

    m_stop_button = std::make_unique<reimu::gui::Button>();
    m_stop_button->layout = control_btn_layout;
    m_back_button = std::make_unique<reimu::gui::Button>();
    m_back_button->layout = control_btn_layout;
    m_fwd_button = std::make_unique<reimu::gui::Button>();
    m_fwd_button->layout = control_btn_layout;
    m_shuffle_button = std::make_unique<reimu::gui::Button>();
    m_shuffle_button->layout = control_btn_layout;

    m_play_controls_box = std::make_unique<reimu::gui::GridBox>();
    m_play_controls_box->layout.height = reimu::gui::Size::from_percent(1.f);
    m_play_controls_box->layout.width = reimu::gui::Size::from_percent(1.f);

    m_play_controls_box->add_row(reimu::gui::Size::from_em(3.f));

    m_play_controls_box->add_item(m_duration.get(), reimu::gui::Size::from_em(6.f));
    m_play_controls_box->add_item(m_progress.get(), reimu::gui::Size::from_layout_factor(1.f));

    m_play_controls_box->add_row(reimu::gui::Size::from_em(3.5f));

    m_play_controls_box->add_item(m_play_button.get(), reimu::gui::Size::from_layout_factor(1));
    m_play_controls_box->add_item(m_stop_button.get(), reimu::gui::Size::from_layout_factor(1));
    m_play_controls_box->add_item(m_back_button.get(), reimu::gui::Size::from_layout_factor(1));
    m_play_controls_box->add_item(m_fwd_button.get(), reimu::gui::Size::from_layout_factor(1));
    m_play_controls_box->add_item(m_shuffle_button.get(), reimu::gui::Size::from_layout_factor(1));

    add_child(m_play_controls_box.get());
}

void PlayerControls::repaint(reimu::gui::UIPainter &painter) {
    m_play_button->label = m_provider.is_playing() ? "Pause" : "Play";
    
    auto track_progress_min = m_provider.track_progress_us() / 1'000'000 / 60;
    auto track_progress_sec = (m_provider.track_progress_us() / 1'000'000) % 60;
    auto track_duration_min = m_provider.track_duration_us() / 1'000'000 / 60;
    auto track_duration_sec = (m_provider.track_duration_us() / 1'000'000) % 60;

    auto duration_string = track_duration_min >= 60 ?
        std::format("{:02}:{:02}:{:02}/{:02}:{:02}:{:02}",
            track_progress_min / 60,
            track_progress_min % 60,
            track_progress_sec,
            track_duration_min / 60,
            track_duration_min % 60,
            track_duration_sec
        ) :
        std::format("{:02}:{:02}/{:02}:{:02}",
            track_progress_min,
            track_progress_sec,
            track_duration_min,
            track_duration_sec
        );

    m_duration->set_text(
        duration_string
    );

    reimu::gui::FlowBox::repaint(painter);
}
