#include <reimu/core/logger.h>

#include <assert.h>

#include "driver.h"

reimu::video::Driver *wayland_init();

namespace reimu::video {

static Driver *driver;

void init() {
    driver = wayland_init();
    
    if (!driver) {
        logger::fatal("Failed to intialize any video driver");
    }
}

reimu::video::Driver *get_driver() {
    assert(driver);

    return driver;
}

}
