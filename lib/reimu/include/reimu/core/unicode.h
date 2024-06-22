#pragma once

#include <reimu/core/error.h>
#include <reimu/core/result.h>

#include <string>

namespace reimu {

DEF_SIMPLE_ERROR(InvalidUTF8Sequence, "Invalid UTF-8 sequence");

Result<std::u32string, InvalidUTF8Sequence> to_utf32(const std::string &utf8);
    
}
