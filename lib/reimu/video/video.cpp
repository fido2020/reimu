#include <reimu/core/logger.h>

#include <reimu/video/driver.h>

#include <assert.h>

reimu::video::Driver *wayland_init();
reimu::video::Driver *win32_init();

namespace reimu::video {

static Driver *driver;

void init() {
#ifdef REIMU_VIDEO_WAYLAND
    if (!driver)
        driver = wayland_init();
#endif

#ifdef REIMU_WIN32
    if (!driver)
        driver = win32_init();
#endif

    if (!driver) {
        logger::fatal("Failed to intialize any video driver");
    }
}

reimu::video::Driver *get_driver() {
    assert(driver);

    return driver;
}

}
