#pragma once

namespace reimu::video {

/**
 * @brief Initialize video using the first available driver
*/
void init();
class Driver *get_driver();

}
