#pragma once

#include <reimu/core/result.h>

#include <vector>

namespace reimu {

enum class FileMode {
    ReadOnly,
    ReadWrite
};

class File {
public:
    virtual Result<std::vector<uint8_t>, ReimuError> read(size_t up_to) = 0;
    virtual Result<size_t, ReimuError> write(const void *buffer, size_t size) = 0;

    virtual Result<void, ReimuError> seek(size_t offset) = 0;
    virtual Result<void, ReimuError> seek_to_end() = 0;

    virtual size_t offset() = 0;
    virtual size_t file_size() = 0;
};

}
