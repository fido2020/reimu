#include <reimu/video/window.h>

#include <reimu/video/video.h>

#include "driver.h"

namespace reimu::video {

Window::~Window() {}

Result<Window *, ReimuError> Window::create(const Vector2i &size) {
    auto *win = get_driver()->window_create(size);
    if (!win) {
        return ERR(
            ReimuError{ ReimuError::WindowCreationFailed }
        );
    }

    return OK(win);
}

}
