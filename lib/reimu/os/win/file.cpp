#include "reimu/core/logger.h"
#include <reimu/os/fs.h>
#include <reimu/core/file.h>

#include <fcntl.h>

namespace reimu::os {

class Win32File : public File {
public:
    Win32File(FILE *fd, FileMode mode) : m_fd{fd}, m_mode{mode} {
        
    }

    virtual ~Win32File() {
        ::fclose(m_fd);
    }

    Result<std::vector<uint8_t>, ReimuError> read(size_t up_to) override {
        std::vector<uint8_t> data{};

        data.resize(up_to);

        logger::debug("read: offset: {}, ftell: {}, size: {}", m_off, ftell(m_fd), up_to);
        auto result = ::fread(data.data(), 1, up_to, m_fd);
        logger::debug("Read {}/{} bytes from file (ftell {})", result, up_to, ftell(m_fd));
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

    Result<void, ReimuError> seek(size_t offset) override {
        auto result = fseek(m_fd, offset, SEEK_SET);
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

        seek(off).ensure();

        m_off = off;
        logger::debug("offset: {}, ftell: {}, size: {}", off, ftell(m_fd), sz);

        return sz;
    }
    
private:
    FILE *m_fd;
    FileMode m_mode;

    size_t m_off = 0;
};

Result<std::unique_ptr<File>, reimu::OSError> open(const std::string &path, FileMode mode) {
    const char *mode_str;

    switch(mode) {
    case FileMode::ReadOnly:
        mode_str = "rb";
        break;
    case FileMode::ReadWrite:
        mode_str = "wb+";
        break;
    default:
        return ERR(EINVAL);
    }

    auto fd = ::fopen(path.c_str(), mode_str);
    if (fd == nullptr) {
        return ERR(errno);
    }

    auto file = std::make_unique<Win32File>(fd, mode);

    return OK(std::move(file));
}

}