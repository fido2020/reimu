#pragma once

#include <stdint.h>

namespace reimu {

template<typename ...T>
class Message {
    Message(T&&... values)

private:
    uint8_t *data;
};

}
