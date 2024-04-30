#pragma once

#include <format>
#include <string>

#include <string.h>

#include <reimu/core/error.h>

namespace reimu {

struct OSError : public ErrorBase {
    OSError(int e)
        : err_no(e) {}

    std::string as_string() const override {
        return std::format("OSError ({}): {}", err_no, strerror(err_no));
    }

    int err_no;
};

}
