#pragma once

#include <reimu/graphics/vector.h>

namespace reimu::video {

enum class MouseButton {
    Left,
    Right,
    Middle
};

enum class MouseButtonState {
    Pressed,
    Released
};

struct MouseEvent {
    // Did the mouse enter/leave the window
    bool is_enter = false;
    bool is_leave = false;
    // Was the mouse moved
    bool is_move = false;
    // Was a button pressed/released
    bool is_button = false;

    Vector2f pos;
    MouseButton button;
    MouseButtonState state;
};

struct InputEvent {
    enum {
        Mouse,
        Keyboard
    } type;
    
    union {
        int key;
        MouseEvent mouse = {};
    };
};

}
