#include "include/reimu/core/unicode.h"
#include <cstdint>
#include <reimu/core/error.h>
#include <reimu/core/file.h>
#include <reimu/core/unicode.h>

namespace reimu {

Result<uint32_t, ReimuError> File::get_utf8(uint8_t encoded[5]) {
    auto first_byte = get_byte();
    if (!first_byte.has_some()) {
        return ERR(ReimuError::EndOfFile);
    }

    auto num_bytes_opt = utf8_codepoint_num_bytes(first_byte.ensure());
    if (!num_bytes_opt.has_some()) {
        return ERR(ReimuError::EncodingError);
    }

    auto num_bytes = num_bytes_opt.value();

    uint8_t bytes[4] = {0};
    bytes[0] = first_byte.value();

    for (int i = 1; i < num_bytes; i++) {
        auto byte = get_byte();
        if (!byte.has_some()) {
            return ERR(ReimuError::EndOfFile);
        }
        bytes[i] = byte.ensure();
    }

    size_t consumed = 0;
    auto codepoint = single_utf8_to_utf32(bytes, num_bytes, consumed);

    if (!codepoint.has_some()) {
        return ERR(ReimuError::EncodingError);
    }

    // Copy the read bytes to the output array
    if (encoded) {
        encoded[0] = bytes[0];
        encoded[1] = bytes[1];
        encoded[2] = bytes[2];
        encoded[3] = bytes[3];
        encoded[4] = 0;  // Null-terminate the string
    }

    return OK(codepoint.value());
}

Result<size_t, ReimuError> File::write(const void *buffer, size_t size) {
    (void)buffer;
    (void)size;

    return ERR(ReimuError::NotImplemented);
}

Result<void, ReimuError> File::seek_to_end() {
    return seek(0, SeekMode::End);
}

} // namespace reimu
