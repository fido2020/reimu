#include <reimu/os/fs.h>

#include <assert.h>

#include <fcntl.h>
#include <sys/stat.h>

namespace reimu::os {

Result<int, OSError> make_path(const std::string &path, int mode) {
    int e = mkdir(path.c_str(), mode);
    if (e == -1) {
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

    return OK(0);
}

}
