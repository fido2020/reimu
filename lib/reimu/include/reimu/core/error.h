#pragma once

#include <concepts>
#include <format>
#include <string>

namespace reimu {

template<typename T>
concept Error = requires(T t){
    { t.as_string() } -> std::convertible_to<std::string>;
};

}

template<reimu::Error E>
struct std::formatter<E> {
    constexpr auto parse(std::format_parse_context &ctx) {
        return ctx.begin();
    }

    template<typename FmtCtx>
    auto format(const E &error, FmtCtx &ctx) const {
        return std::format_to(ctx.out(), "{}", error.as_string());
    }
};
