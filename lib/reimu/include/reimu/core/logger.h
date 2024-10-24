#pragma once

#include <format>

#include <stdio.h>
#include <stdlib.h>

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
[[noreturn ]] void fatal(std::format_string<Args...> f, Args&&... args){
    fprintf(stderr, "[reimu] fatal: %s\n",
        std::vformat(f.get(), std::make_format_args(args...)).c_str());

    __builtin_unreachable();
}

template<typename ...Args>
void debug(std::wformat_string<Args...> f, Args&&... args){
    fprintf(stderr, "[reimu] %ls\n",
        std::vformat(f.get(), std::make_wformat_args(args...)).c_str());
}

template<typename ...Args>
void warn(std::wformat_string<Args...> f, Args&&... args){
    fprintf(stderr, "[reimu] warn: %ls\n",
        std::vformat(f.get(), std::make_wformat_args(args...)).c_str());
}

template<typename ...Args>
[[noreturn ]] void fatal(std::wformat_string<Args...> f, Args&&... args){
    fprintf(stderr, "[reimu] fatal: %ls\n",
        std::vformat(f.get(), std::make_wformat_args(args...)).c_str());

    __builtin_unreachable();
}

} // namespace logger

} // namespace reimu
