#include "reimu/core/logger.h"
#include <reimu/os/fs.h>
#include <reimu/core/file.h>

#include <fcntl.h>

#include <codecvt>

namespace reimu::os {

class Win32File : public File {
public:
    Win32File(FILE *fd, FileMode mode) : m_fd{fd}, m_mode{mode} {
        
    }

    virtual ~Win32File() {
        ::fclose(m_fd);
    }

    Optional<uint8_t> get_byte() override {
        uint8_t byte;
        auto result = ::fgetc(m_fd);
        if (result != EOF) {
            byte = static_cast<uint8_t>(result);
            m_off += 1;
            return OPT_SOME(byte);
        }

        return OPT_NONE;
    }

    Result<std::vector<uint8_t>, ReimuError> read(size_t up_to) override {
        std::vector<uint8_t> data{};

        data.resize(up_to);

        auto result = ::fread(data.data(), 1, up_to, m_fd);
        if (ferror(m_fd)) {
            return ERR(ReimuError::IOError);
        }

        data.resize(result);
        m_off += result;

        return OK(std::move(data));
    }

    Result<size_t, ReimuError> write(const void *buffer, size_t size) override {
        if (m_mode == FileMode::ReadOnly) {
            return ERR(ReimuError::AccessError);
        }
        
        auto result = ::fwrite(buffer, 1, size, m_fd);
        if (ferror(m_fd)) {
            return ERR(ReimuError::IOError);
        }

        m_off += result;

        return OK(result);
    }

    Result<void, ReimuError> seek(ssize_t offset, SeekMode mode) override {
        int whence;
        switch (mode) {
        case SeekMode::Absolute:
            whence = SEEK_SET;
            break;
        case SeekMode::Relative:
            whence = SEEK_CUR;
            break;
        case SeekMode::End:
            whence = SEEK_END;
            break;
        default:
            __builtin_unreachable();
        }

        auto result = fseek(m_fd, offset, whence);
        if (result < 0) {
            return ERR(ReimuError::IOError);
        }

        m_off = ftell(m_fd);

        return OK();
    }

    Result<void, ReimuError> seek_to_end() override {
        auto result = fseek(m_fd, 0, SEEK_END);
        if (result < 0) {
            return ERR(ReimuError::IOError);
        }

        m_off = ftell(m_fd);

        return OK();
    }

    size_t offset() override {
        return m_off;
    }

    size_t file_size() override {
        auto off = m_off;

        seek_to_end().ensure();

        auto sz = m_off;

        seek(off, SeekMode::Absolute).ensure();

        m_off = off;

        return sz;
    }

    bool is_eof() const override {
        return feof(m_fd);
    }
    
private:
    FILE *m_fd;
    FileMode m_mode;

    size_t m_off = 0;
};

Result<std::unique_ptr<File>, reimu::OSError> open(const std::string &path, FileMode mode) {
    const wchar_t *mode_str;

    switch(mode) {
    case FileMode::ReadOnly:
        mode_str = L"rb";
        break;
    case FileMode::ReadWrite:
        mode_str = L"wb+";
        break;
    default:
        return ERR(EINVAL);
    }

    // Convert utf8 path to wide string
    std::wstring wpath = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(path);

    auto fd = _wfopen(wpath.c_str(), mode_str);
    if (fd == nullptr) {
        auto e = OSError(errno, path);
        return ERR(std::move(e));
    }

    auto file = std::make_unique<Win32File>(fd, mode);

    return OK(std::move(file));
}

}