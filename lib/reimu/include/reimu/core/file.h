#pragma once

#include "error.h"
#include <webgpu.h>
#ifdef _WIN32
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#include <reimu/core/result.h>
#include <reimu/core/optional.h>

#include <vector>
#include <algorithm>

namespace reimu {

enum class FileMode {
    ReadOnly,
    ReadWrite
};

enum class SeekMode {
    Absolute,
    Relative,
    End
};

class File {
public:
    virtual ~File() = default;

    virtual Optional<uint8_t> get_byte() = 0;
    virtual Result<uint32_t, ReimuError> get_utf8(uint8_t *encoded = nullptr);

    virtual Result<std::vector<uint8_t>, ReimuError> read(size_t up_to) = 0;
    virtual Result<size_t, ReimuError> write(const void *buffer, size_t size);

    virtual Result<void, ReimuError> seek(ssize_t offset, SeekMode whence = SeekMode::Absolute) = 0;
    virtual Result<void, ReimuError> seek_to_end();

    virtual size_t offset() = 0;
    virtual size_t file_size() = 0;

    virtual bool is_eof() const = 0;
};

class TextStream : public File {
public:
    TextStream(const std::string &data)
        : m_data(data), m_pos(0) {}
    ~TextStream() = default;

    Optional<uint8_t> get_byte() override {
        if (m_pos >= m_data.size()) {
            return OPT_NONE;
        }
        return OPT_SOME((uint8_t)m_data[m_pos++]);
    }

    Result<std::vector<uint8_t>, ReimuError> read(size_t up_to) override {
        if (m_pos >= m_data.size()) {
            return OK(std::vector<uint8_t>{});
        }

        size_t to_read = std::min(up_to, m_data.size() - m_pos);
        std::vector<uint8_t> buffer(to_read);
        memcpy(buffer.data(), m_data.data() + m_pos, to_read);

        m_pos += to_read;

        return OK(buffer);
    }

    Result<void, ReimuError> seek(ssize_t offset, SeekMode whence = SeekMode::Absolute) override {
        size_t new_pos = 0;
        switch (whence) {
        case SeekMode::Absolute:
            new_pos = (size_t)offset;
            break;
        case SeekMode::Relative:
            new_pos = m_pos + offset;
            break;
        case SeekMode::End:
            new_pos = m_data.size() + offset;
            break;
        }

        m_pos = std::clamp(new_pos, (size_t)0, m_data.size());
        return OK();
    }

    size_t offset() override {
        return m_pos;
    }

    size_t file_size() override {
        return m_data.size();
    }

    bool is_eof() const override {
        return m_pos >= m_data.size();
    }

private:
    std::string m_data;
    size_t m_pos;
};

}
