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
    Escape = 0x100,
    F1 = 0x101,
    F2 = 0x102,
    F3 = 0x103,
    F4 = 0x104,
    F5 = 0x105,
    F6 = 0x106,
    F7 = 0x107,
    F8 = 0x108,
    F9 = 0x109,
    F10 = 0x10A,
    F11 = 0x10B,
    F12 = 0x10C,
    PrintScreen = 0x10D,
    ScrollLock = 0x10E,
    Pause = 0x10F,
    Insert = 0x110,
    Home = 0x111,
    PageUp = 0x112,
    Delete = 0x113,
    End = 0x114,
    PageDown = 0x115,
    Right = 0x116,
    Left = 0x117,
    Down = 0x118,
    Up = 0x119,
    NumLock = 0x11A,
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
