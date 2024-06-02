#pragma once

#include <stdint.h>

#include <string_view>

namespace reimu {

// Implementation of FNV-1a hash function

namespace detail {

static constexpr uint32_t fnv_offset_32 = 0x811c9dc5;
static constexpr uint32_t fnv_prime_32 = 0x1000193;
static constexpr uint64_t fnv_offset_64 = 0xcbf29ce484222325;
static constexpr uint64_t fnv_prime_64 = 0x100000001b3;

}

template<typename T = uint64_t>
consteval T string_hash(const char *str, size_t len);

consteval inline uint64_t string_hash(const char *str, size_t len, uint64_t hash) {
    if (len == 0) {
        return hash;
    }

    hash = (hash ^ *str) * detail::fnv_prime_64;

    return string_hash(str + 1, len - 1, hash);
}

template<>
consteval inline uint64_t string_hash<uint64_t>(const char *str, size_t len) {
    return string_hash(str, len, detail::fnv_offset_64);
}

static_assert(string_hash<uint64_t>("hello", 5) == 0xa430d84680aabd0b);

}

