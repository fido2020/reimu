#pragma once

#include <reimu/core/error.h>
#include <reimu/core/logger.h>
#include <reimu/core/util.h>

#include <utility>

namespace reimu {

namespace detail {
    struct NoneTag {};

    template<typename T>
    struct SomeTag {};
}

template<typename T>
class Optional {
public:
    constexpr Optional() : m_has_some{false} {}

    constexpr Optional(const Optional &other) {
        if (other.m_has_some) {
            new(&m_data) T(other.m_data);
        }
        m_has_some = other.m_has_some;
    }

    constexpr Optional(T data)
        : m_data{std::move(data)}, m_has_some{true} {}

    constexpr Optional(const detail::NoneTag &tag)
        : m_has_some{false} {(void) tag;}

    ~Optional() {
        if (m_has_some) {
            m_data.~T();
        }
    }

    template<typename U = std::remove_cv<T>>
    T or_default(U &&def) {
        return has_some() ? move_val() : static_cast<T>(std::forward<U>(def));
    }

    constexpr inline bool has_some() const {
        return m_has_some;
    }

    constexpr inline operator bool() const {
        return has_some();
    }

    constexpr T &&value() {
        return std::move(m_data);
    }

    inline T &&ensure() {
        if (!m_has_some) {
            logger::fatal("Unexpected None");
        }

        return move_val();
    }

private:
    inline T &&move_val() {
        return std::move(m_data);
    }

    union {
        T m_data;
    };

    bool m_has_some;
};

} // namespace reimu

#define OPT_SOME(x) (reimu::Optional{x})
#define OPT_NONE (reimu::detail::NoneTag{})
