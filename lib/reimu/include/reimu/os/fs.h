#pragma once

#include <reimu/os/error.h>
#include <reimu/os/handle.h>
#include <reimu/core/result.h>

#include <string>

namespace reimu::os {

// Ensures the path specified in 'path' exists.
// Creates any path components with mode 'mode'
Result<void, OSError> make_path(const std::string &path, int mode);

Result<std::string, OSError> default_shell_path();

Result<size_t, OSError> write(os_handle_t handle, const void *buffer, size_t size);
Result<void, reimu::OSError> close(os_handle_t handle);

}
