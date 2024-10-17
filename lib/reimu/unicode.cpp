#include <reimu/core/unicode.h>

#include <cstddef>
#include <cstdint>

namespace reimu {

Result<std::u32string, InvalidUTF8Sequence> to_utf32(std::string_view utf8) {
    std::u32string utf32;

    // Reserve enough space for the worst case
    utf32.reserve(utf8.size());

    uint8_t *data = (uint8_t *)utf8.data();

    for (unsigned i = 0; i < utf8.size(); i++) {
        uint32_t u8 = data[i];

        int codepoint_size = 1;
        if ((u8 & 0x80) == 0) {
            codepoint_size = 1;
        } else if ((u8 & 0xe0) == 0xc0) {
            codepoint_size = 2;
        } else if ((u8 & 0xf0) == 0xe0) {
            codepoint_size = 3;
        } else if ((u8 & 0xf8) == 0xf0) {
            codepoint_size = 4;
        } else {
            return ERR({});
        }

        if (i + codepoint_size > utf8.size()) {
            return ERR({});
        }

        switch (codepoint_size) {
        case 1:
            utf32.push_back(u8);
            break;
        case 2:
            utf32.push_back(((u8 & 0x1f) << 6) | (data[i + 1] & 0x3f));
            i++;
            break;
        case 3:
            utf32.push_back(((u8 & 0xf) << 12) | ((data[i + 1] & 0x3f) << 6)
                | (data[i + 2] & 0x3f));
            i += 2;
            break;
        case 4:
            utf32.push_back(((u8 & 0x7) << 18) | ((data[i + 1] & 0x3f) << 12)
                | ((data[i + 2] & 0x3f) << 6) | (data[i + 3] & 0x3f));
            i += 3;
            break;
        }
    }

    return OK(utf32);
}

Optional<uint32_t> single_utf8_to_utf32(const char *utf8, size_t n, size_t &consumed) {
    uint8_t *data = (uint8_t *)utf8;

    uint32_t u8 = data[0];

    int codepoint_size = 1;
    if ((u8 & 0x80) == 0) {
        codepoint_size = 1;
    } else if ((u8 & 0xe0) == 0xc0) {
        codepoint_size = 2;
    } else if ((u8 & 0xf0) == 0xe0) {
        codepoint_size = 3;
    } else if ((u8 & 0xf8) == 0xf0) {
        codepoint_size = 4;
    } else {
        return OPT_NONE;
    }

    if (codepoint_size > n) {
        return OPT_NONE;
    }

    consumed = codepoint_size;

    switch (codepoint_size) {
    case 1:
        return OPT_SOME(u8);
    case 2:
        return OPT_SOME(((u8 & 0x1f) << 6) | (data[1] & 0x3f));
    case 3:
        return OPT_SOME(((u8 & 0xf) << 12) | ((data[1] & 0x3f) << 6)
            | (data[2] & 0x3f));
    case 4:
        return OPT_SOME(((u8 & 0x7) << 18) | ((data[1] & 0x3f) << 12)
            | ((data[2] & 0x3f) << 6) | (data[3] & 0x3f));
    }

    return OPT_NONE;
}

}
