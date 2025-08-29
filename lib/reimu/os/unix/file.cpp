#include <reimu/os/fs.h>
#include <reimu/core/file.h>

#include <fcntl.h>

namespace reimu::os {

class UNIXFile : public File {
public:
    UNIXFile(int fd, FileMode mode) : m_fd{fd}, m_mode{mode} {
        
    }

    Result<std::vector<uint8_t>, ReimuError> read(size_t up_to) override {
        std::vector<uint8_t> data{};

        data.resize(up_to);

        auto result = ::read(m_fd, data.data(), up_to);
        if (result < 0) {
            return ERR(ReimuError::IOError);
        }

        if ((size_t)result < up_to) {
            m_eof = true;
        }

        data.resize(result);
        m_off += result;

        return OK(std::move(data));
    }

    Result<size_t, ReimuError> write(const void *buffer, size_t size) override {
        if (m_mode == FileMode::ReadOnly) {
            return ERR(ReimuError::AccessError);
        }
        
        auto result = ::write(m_fd, buffer, size);
        if (result < 0) {
            return ERR(ReimuError::IOError);
        }

        m_off += result;

        return OK((size_t)result);
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

        auto result = lseek(m_fd, offset, whence);
        if (result < 0) {
            return ERR(ReimuError::IOError);
        }

        // Check for EOF condition
        switch (mode) {
        case SeekMode::Absolute:
            m_eof = result < offset;
            break;
        case SeekMode::Relative:
            m_eof = result < (ssize_t)m_off + offset;
            break;
        case SeekMode::End:
            m_eof = offset >= 0;
            break;
        default:
            __builtin_unreachable();
        }

        m_off = result;

        return OK();
    }

    Result<void, ReimuError> seek_to_end() override {
        auto result = lseek(m_fd, 0, SEEK_END);
        if (result < 0) {
            return ERR(ReimuError::IOError);
        }

        m_off = result;

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
        return m_eof;
    }
    
private:
    int m_fd;
    FileMode m_mode;
    bool m_eof = false;

    size_t m_off = 0;
};

Result<std::unique_ptr<File>, reimu::OSError> open(const std::string &path, FileMode mode) {
    int mode_num = 0;

    switch(mode) {
    case FileMode::ReadOnly:
        mode_num = O_RDONLY;
        break;
    case FileMode::ReadWrite:
        mode_num = O_RDWR;
        break;
    default:
        return ERR(EINVAL);
    }

    auto fd = ::open(path.c_str(), mode_num);
    if (fd < 0) {
        return ERR(errno);
    }

    auto file = std::make_unique<UNIXFile>(fd, mode);

    return OK(std::move(file));
}

}