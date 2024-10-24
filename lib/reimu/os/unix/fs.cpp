#include <reimu/os/fs.h>

#include <assert.h>

#include <fcntl.h>
#include <sys/stat.h>

#include <stdio.h>

namespace reimu::os {

Result<void, OSError> make_path(const std::string &path, int mode) {
    int e = mkdir(path.c_str(), mode);
    if (e == -1) {
        if (errno == EEXIST) {
            return OK();
        }

        if (errno == ENOENT) {
            std::string_view parent = path;

            // Discard any slash on the end
            if (parent.ends_with('/')) {
                parent = parent.substr(0, parent.length() - 1);
            }

            // Find the last path separator, if we got ENOENT and there's no
            // path sepatator something weird happened
            size_t sep_pos = parent.rfind('/');
            assert(sep_pos != std::string::npos);

            parent = parent.substr(0, sep_pos);

            return make_path(std::string{parent}, mode);
        }

        return ERR(errno);
    }

    return OK();
}

Result<std::string, OSError> default_shell_path() {
    auto path = getenv("SHELL");
    if (path == nullptr) {
        return ERR(OSError{ENOENT});
    }

    return OK(std::string{path});
}

reimu::Result<size_t, reimu::OSError> write(os_handle_t handle, const void *buffer, size_t size) {
    auto ret = ::write(handle, buffer, size);
    if (ret < 0) {
        return ERR(reimu::OSError{errno});
    }

    return OK(ret);
}

reimu::Result<void, reimu::OSError> close(os_handle_t handle) {
    if (::close(handle) < 0) {
        return ERR(reimu::OSError{errno});
    }

    return OK();
}

}
