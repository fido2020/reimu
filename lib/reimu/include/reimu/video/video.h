#pragma once

#include <reimu/graphics/vector.h>

namespace reimu::video {

/**
 * @brief Initialize video using the first available driver
*/
void init();

Vector2u get_display_size();

class Driver *get_driver();

}
