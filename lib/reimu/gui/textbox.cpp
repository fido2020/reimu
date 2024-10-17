#include <reimu/gui/widget.h>

namespace reimu::gui {

TextBox::TextBox() {
    bind_event_callback("on_key_press", [this]() {
        
    });
}

void TextBox::repaint(UIPainter &painter) {
    
}

void TextBox::set_text(std::string text) {
    m_text = std::move(text);

    dispatch_event("on_text_input"_hashid);
}

std::string_view TextBox::get_text() const {
    return std::string_view{ m_text };
}

}
