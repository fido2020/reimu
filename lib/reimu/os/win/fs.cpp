#include <reimu/os/fs.h>

#include <assert.h>

#include <fcntl.h>
#include <sys/stat.h>

namespace reimu::os {

Result<void, OSError> make_path(const std::string &path, int mode) {
    return ERR(ENOSYS);
}

reimu::Result<std::string, reimu::OSError> default_shell_path() {
    return {"C:\\Windows\\System32\\cmd.exe"};
}

reimu::Result<size_t, reimu::OSError> write(os_handle_t handle, const void *buffer, size_t size) {
    DWORD written;
    if (!WriteFile(handle, buffer, size, &written, nullptr)) {
        return ERR(reimu::OSError{GetLastError()});
    }

    return OK(written);
}

reimu::Result<void, reimu::OSError> close(os_handle_t handle) {
    if (!CloseHandle(handle)) {
        return ERR(reimu::OSError{GetLastError()});
    }

    return OK();
}

}
