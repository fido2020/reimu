#include <reimu/gui/dialog.h>

#include <reimu/gui/window.h>

namespace reimu::gui {

int message_box(const std::string &title, const std::string &message, const MessageBoxButtons &buttons) {
    Window *win;
    int return_value = 0;

    if (auto r = Window::create({ 400, 200 }); !r.is_err()) {
        win = r.move_val();
    } else {
        // If we failed to make the window just return 0
        return 0;
    }

    win->set_title(title);

    auto button_box = std::make_unique<GridBox>();
    button_box->layout.width = Size::from_percent(1);
    button_box->layout.height = Size::from_percent(1);

    win->root().add_child(button_box.get());

    // Add label
    button_box->add_row(Size::from_layout_factor(1));

    auto label = std::make_unique<Label>();
    label->layout.set_padding(Size::from_root_em(0.5f));
    label->set_text(message);

    button_box->add_item(label.get(), Size::from_layout_factor(1));

    // Add buttons
    button_box->add_row(Size::from_root_em(2.75f));

    std::vector<std::unique_ptr<Button>> button_controls;
    for (const auto &button : buttons.buttons) {
        auto btn = new Button;
        btn->label = button.text;
        btn->layout.set_padding(Size::from_root_em(0.5f));

        btn->bind_event_callback("on_click"_hashid, [win, &return_value, value = button.value]() {
            return_value = value;
            win->close();
        });

        button_box->add_item(btn, Size::from_root_em(10));
        button_controls.push_back(std::unique_ptr<Button>(btn));
    }

    win->render();
    win->run_until_close();

    delete win;
    
    return return_value;
}

}
