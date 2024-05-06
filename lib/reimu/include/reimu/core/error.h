#pragma once

#include <concepts>
#include <format>
#include <string>
#include <memory>
#include <type_traits>

namespace reimu {

namespace detail {
    template <typename ...Ts>
    struct ErrorUnion;

    template<typename T, typename ...Ts>
    struct ErrorUnion<T, Ts...> {
        union {
            T e;
            ErrorUnion<Ts...> other;
        };
    };

    template<typename T>
    struct ErrorUnion<T> {
        union {
            T e;
        };
    };
}

template<typename T>
concept Error = requires(T t){
    { t.as_string() } -> std::convertible_to<std::string>;
};

class ErrorBase {
public:
    virtual ~ErrorBase() = default;
    virtual std::string as_string() const = 0;
};

// Allows multiple errors derived from this to be returned in one result,
// far from efficient
class ErrorBox {
public:
    ErrorBox(const ErrorBox&) = delete;

    ErrorBox(ErrorBox &&other) {
        m_storage = std::move(other.m_storage);
    }

    template<typename E>
    ErrorBox(E &&value) {
        m_storage = std::make_unique<E>(std::move(value));
    }

    std::string as_string() const {
        return m_storage->as_string();
    }

private:
    std::unique_ptr<ErrorBase> m_storage;
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
