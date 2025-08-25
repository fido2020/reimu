#pragma once

#include <format>
#include <string>

#include <string.h>

#include <reimu/core/error.h>

namespace reimu {

struct OSError : public ErrorBase {
    OSError(int e)
        : err_no(e) {}

    OSError(int e, std::string detail)
        : err_no(e), detail(std::move(detail)) {}

    std::string as_string() const override {
        if (!detail.empty()) {
            return std::format("OSError ({}) \"{}\": {}", err_no, detail, strerror(err_no));
        }

        return std::format("OSError ({}): {}", err_no, strerror(err_no));
    }

    int err_no;
    std::string detail;
};

}
