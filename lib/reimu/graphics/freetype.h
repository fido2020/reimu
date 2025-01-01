#pragma once

#include <reimu/core/result.h>

#include <ft2build.h>
#include FT_FREETYPE_H

// Private wrapper class for FreeType

#include <cassert>
#include <mutex>
#include <vector>

struct FreeTypeError {
    FT_Error error;

    std::string as_string() const {
        return FT_Error_String(error);
    }
};

class FreeType {
public:
    FreeType();

    static inline FreeType& instance() {
        assert(m_instance);
        return *m_instance;
    }

    // Thread-safe wrapper functions for FreeType
    reimu::Result<FT_Face, FreeTypeError> new_face(std::vector<uint8_t> &data, FT_Long index);
    reimu::Result<void, FreeTypeError> done_face(FT_Face face);

private:
    static FreeType* m_instance;

    std::mutex m_lock;
    FT_Library m_library = nullptr;
};
