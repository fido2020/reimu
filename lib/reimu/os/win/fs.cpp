#include <reimu/os/fs.h>

#include <assert.h>

#include <fcntl.h>
#include <sys/stat.h>

namespace reimu::os {

Result<void, OSError> make_path(const std::string &path, int mode) {
    return OK();
}

}
