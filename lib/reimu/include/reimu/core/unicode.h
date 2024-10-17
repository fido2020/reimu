#pragma once

#include <reimu/core/error.h>
#include <reimu/core/optional.h>
#include <reimu/core/result.h>

#include <stddef.h>

#include <string_view>

namespace reimu {

DEF_SIMPLE_ERROR(InvalidUTF8Sequence, "Invalid UTF-8 sequence");

Result<std::u32string, InvalidUTF8Sequence> to_utf32(std::string_view utf8);
Optional<uint32_t> single_utf8_to_utf32(const char *utf8, size_t n, size_t &consumed);

}
