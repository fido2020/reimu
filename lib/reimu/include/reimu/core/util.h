#pragma once

#define ALWAYS_INLINE __attribute__((always_inline))

namespace reimu {

[[noreturn]] inline static void unreachable() {
    __builtin_unreachable();
}

}
