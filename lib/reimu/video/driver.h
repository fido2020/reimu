#pragma once

#include <reimu/graphics/vector.h>

namespace reimu::video {

class Window;

class Driver {
public:
    virtual Window *window_create(const Vector2i &size) = 0;

    virtual void finish() = 0;
};

}
