#pragma once

#include <reimu/core/hash.h>

namespace reimu {

struct StringID {
    uint64_t hash;
};

constexpr bool operator==(const StringID &l, const StringID &r) {
    return l.hash == r.hash;
}

constexpr bool operator!=(const StringID &l, const StringID &r) {
    return l.hash != r.hash;
}

constexpr bool operator<(const StringID &l, const StringID &r) {
    return l.hash < r.hash;
}

constexpr bool operator>(const StringID &l, const StringID &r) {
    return l.hash > r.hash;
}

}

consteval reimu::StringID operator""_hashid(const char *str, size_t len) {
    return { reimu::string_hash<uint64_t>(str, len) };
}
