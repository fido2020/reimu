#pragma once

#include <reimu/core/error.h>
#include <reimu/core/logger.h>

#include <utility>

namespace reimu {

namespace detail {
    template<typename E>
    struct ErrTag {};
}

template<typename T, Error E>
class Result {
public:
    Result(E&& e, const detail::ErrTag<E>& tag)
        : m_err{std::move(e)}, m_is_err{true} {
        (void) tag;
    }

    Result(T&& data)
        : m_data{std::move(data)}, m_is_err{false} {}

    ~Result() {
        if (m_is_err) {
            m_err.~E();
        } else {
            m_data.~T();
        }
    }

    inline bool is_err() const {
        return m_is_err;
    }

    inline T &&ensure() {
        if (m_is_err) {
            logger::fatal("Unexpected error value: {}", m_err.as_string());
        }

        return move_val();
    }

    inline E &&move_err() {
        return std::move(m_err);
    }

    inline T &&move_val() {
        return std::move(m_data);
    }

private:
    union {
        E m_err;
        T m_data;
    };

    bool m_is_err;
};

template<Error E>
class Result<void, E> {
public:
    Result(E&& e, const detail::ErrTag<E>& tag)
        : m_err{std::move(e)}, m_is_err{true} {
        (void) tag;
    }

    Result()
        : m_is_err{false} {}

    ~Result() {
        if (m_is_err) {
            m_err.~E();
        }
    }

    inline bool is_err() const {
        return m_is_err;
    }

    inline E &&move_err() {
        return std::move(m_err);
    }

    inline void move_val() {
        return;
    }

private:
    union {
        E m_err;
    };

    bool m_is_err;
};

}

#define OK(...) { __VA_OPT__(std::move(__VA_ARGS__)) }
#define ERR(x) { std::move(x), {} }
#define TRY(x) ({ auto v = (x); if (v.is_err()) return ERR(v.move_err()); v.move_val(); })
