#pragma once

#include <string>
#include <vector>

namespace reimu::gui {

struct MessageBoxButton {
    std::string text;
    int value;
};

struct MessageBoxButtons {
    std::vector<MessageBoxButton> buttons;

    constexpr static inline MessageBoxButtons ok_cancel() {
        return {{
            { "OK", 1 },
            { "Cancel", 0 }
        }};
    }

    constexpr static inline MessageBoxButtons yes_no() {
        return {{
            { "Yes", 1 },
            { "No", 0 }
        }};
    }
};

int message_box(const std::string &title, const std::string &message, const MessageBoxButtons &buttons);

};
