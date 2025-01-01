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
    Optional() : m_has_some{false} {}

    Optional(T data)
        : m_data{std::move(data)}, m_has_some{true} {}

    Optional(const detail::NoneTag &tag)
        : m_has_some{false} {}

    ~Optional() {
        if (m_has_some) {
            m_data.~T();
        }
    }

    inline bool has_some() const {
        return m_has_some;
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
