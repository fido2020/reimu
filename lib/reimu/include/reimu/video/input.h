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

enum Key {
    Invalid = 0,
    Return = '\n',
    Tab = '\t',
    Backspace = '\b',
    Space = ' ',
    Escape = 0xE100,
    F1 = 0xE101,
    F2 = 0xE102,
    F3 = 0xE103,
    F4 = 0xE104,
    F5 = 0xE105,
    F6 = 0xE106,
    F7 = 0xE107,
    F8 = 0xE108,
    F9 = 0xE109,
    F10 = 0xE10A,
    F11 = 0xE10B,
    F12 = 0xE10C,
    PrintScreen = 0xE10D,
    ScrollLock = 0xE10E,
    Pause = 0xE10F,
    Insert = 0xE110,
    Home = 0xE111,
    PageUp = 0xE112,
    Delete = 0xE113,
    End = 0xE114,
    PageDown = 0xE115,
    Right = 0xE116,
    Left = 0xE117,
    Down = 0xE118,
    Up = 0xE119,
    NumLock = 0xE11A,
    Ctrl = 0xE11B,
    Shift = 0xE11C,
    Alt = 0xE11D,
    Win = 0xE11E,
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

struct KeyboardEvent {
    bool is_down = true;

    bool is_ctrl = false;
    bool is_shift = false;
    bool is_alt = false;
    bool is_win = false;

    int key;
};

struct InputEvent {
    enum {
        Mouse,
        Keyboard
    } type;
    
    union {
        KeyboardEvent key;
        MouseEvent mouse = {};
    };
};

}
