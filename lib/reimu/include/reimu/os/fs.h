#pragma once

#include <reimu/core/result.h>
#include <reimu/os/error.h>

#include <string>

namespace reimu::os {

// Ensures the path specified in 'path' exists.
// Creates any path components with mode 'mode'
Result<int, OSError> make_path(const std::string &path, int mode);

}
