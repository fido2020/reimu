#pragma once

#include "controls.h"
#include "reimu/core/logger.h"
#include "reimu/graphics/painter.h"
#include "reimu/graphics/rect.h"
#include "reimu/gui/layout.h"
#include "reimu/gui/widget.h"
#include <memory>

const auto control_btn_layout = []() consteval  {
    auto layout = reimu::gui::LayoutProperties{
        .width = reimu::gui::Size::from_em(3.f),
        .height = reimu::gui::Size::from_em(2.5f),
    };

    layout.set_padding(reimu::gui::Size::from_em(0.25f));
    return layout;
}();

PlayerControls::PlayerControls(AudioControlProvider &provider)
    : m_provider(provider) {
    layout.layout_direction = reimu::gui::LayoutDirection::Vertical;
    layout.set_padding(reimu::gui::Size::from_em(0.5f));

    m_duration = std::make_unique<reimu::gui::Canvas>();
    auto duration_layout = reimu::gui::LayoutProperties {
        .width = reimu::gui::Size::from_percent(1.f),
        .height = reimu::gui::Size::from_em(2.f),
    };
    duration_layout.set_padding(reimu::gui::Size::from_em(0.5f));

    m_duration->layout = duration_layout;

    m_duration->on_paint = [this](reimu::graphics::Painter &painter) {
        reimu::logger::debug("container size: {}x{}", this->calculated_layout.inner_size.x, this->calculated_layout.inner_size.y);
        reimu::logger::debug("canvas size: {}x{}", m_duration->calculated_layout.inner_size.x, m_duration->calculated_layout.inner_size.y);

        float progress = m_provider.track_progress_ms() / static_cast<float>(m_provider.track_duration_ms());
        float filled_width = painter.surface_size().x * progress;
        float height = painter.surface_size().y;

        painter.draw_rect({0, 0, filled_width, height}, reimu::Color::from_rgb_hex(0x00FF00));
        painter.draw_rect({filled_width, 0, (float)painter.surface_size().x, height}, reimu::Color::from_rgb_hex(0x000000));
    };

    add_child(m_duration.get());

    m_play_button = std::make_unique<reimu::gui::Button>();
    m_play_button->layout = control_btn_layout;
    m_stop_button = std::make_unique<reimu::gui::Button>();
    m_stop_button->layout = control_btn_layout;
    m_back_button = std::make_unique<reimu::gui::Button>();
    m_back_button->layout = control_btn_layout;
    m_fwd_button = std::make_unique<reimu::gui::Button>();
    m_fwd_button->layout = control_btn_layout;
    m_shuffle_button = std::make_unique<reimu::gui::Button>();
    m_shuffle_button->layout = control_btn_layout;

    m_play_controls_box = std::make_unique<reimu::gui::GridBox>();
    m_play_controls_box->layout.height = reimu::gui::Size::from_em(3.f);
    m_play_controls_box->layout.width = reimu::gui::Size::from_percent(1.f);

    m_play_controls_box->add_row(reimu::gui::Size::from_percent(1.f));

    m_play_controls_box->add_item(m_play_button.get(), reimu::gui::Size::from_layout_factor(1));
    m_play_controls_box->add_item(m_stop_button.get(), reimu::gui::Size::from_layout_factor(1));
    m_play_controls_box->add_item(m_back_button.get(), reimu::gui::Size::from_layout_factor(1));
    m_play_controls_box->add_item(m_fwd_button.get(), reimu::gui::Size::from_layout_factor(1));
    m_play_controls_box->add_item(m_shuffle_button.get(), reimu::gui::Size::from_layout_factor(1));

    add_child(m_play_controls_box.get());
}
