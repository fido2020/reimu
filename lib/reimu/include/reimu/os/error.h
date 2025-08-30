#pragma once

#include <format>
#include <string>

#include <string.h>

#include <reimu/core/error.h>

namespace reimu {

std::string platform_err_to_str(int err_no);

struct OSError : public ErrorBase {
    OSError(int e)
        : err_no(e) {}

    OSError(int e, std::string detail)
        : err_no(e), detail(std::move(detail)) {}

    std::string as_string() const override {
        auto error_str = platform_err_to_str(err_no);

        if (!detail.empty()) {
            return std::format("OSError ({}) \"{}\": {}", err_no, detail, error_str);
        }

        return std::format("OSError ({}): {}", err_no, error_str);
    }

    int err_no;
    std::string detail;
};

}
