#pragma once

#include <format>
#include <string>

#include <string.h>

#include <reimu/core/error.h>

namespace reimu {

struct OSError {
    OSError(int e)
        : err_no(e) {}

    std::string as_string() const {
        return std::format("OSError ({}): {}", err_no, strerror(err_no));
    }

    int err_no;
};

}
