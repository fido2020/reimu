#pragma once

#include <format>

#include <stdio.h>

namespace reimu {

namespace logger {

template<typename ...Args>
void debug(std::format_string<Args...> f, Args&&... args){
    fprintf(stderr, "[reimu] %s\n",
        std::vformat(f.get(), std::make_format_args(args...)).c_str());
}

template<typename ...Args>
void warn(std::format_string<Args...> f, Args&&... args){
    fprintf(stderr, "[reimu] warn: %s\n",
        std::vformat(f.get(), std::make_format_args(args...)).c_str());
}

template<typename ...Args>
void fatal(std::format_string<Args...> f, Args&&... args){
    fprintf(stderr, "[reimu] fatal: %s\n",
        std::vformat(f.get(), std::make_format_args(args...)).c_str());

    __builtin_unreachable();
}

} // namespace logger

} // namespace reimu
