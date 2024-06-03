#pragma once

#include <reimu/core/util.h>

#include <cmath>
#include <format>

namespace reimu {

template <typename T> struct Vector2 {
    T x;
    T y;

    ALWAYS_INLINE Vector2<T>& scale(const Vector2<T>& scaleVector) {
        x *= scaleVector.x;
        y *= scaleVector.y;
        return *this;
    }

    static ALWAYS_INLINE Vector2<T> scale(const Vector2<T>& l, const Vector2<T>& r) {
        return Vector2<T>{l.x * r.x, l.y * r.y};
    }

    ALWAYS_INLINE Vector2<T> normalize() {
        return (*this) * static_cast<T>(1 / sqrt((x * x) + (y * y)));
    }

    ALWAYS_INLINE T magnitude() {
        return sqrt((x * x) + (y * y));
    }

    ALWAYS_INLINE Vector2<T> rotate(float angle) {
        float c = cosf(angle);
        float s = sinf(angle);

        return Vector2<T>{c * x - s * y, s * x + c * y};
    }
};

template <typename T> inline Vector2<T> operator*(const Vector2<T>& vector, T magnitude) {
    return Vector2<T>{vector.x * magnitude, vector.y * magnitude};
}

template <typename T> inline Vector2<T> operator*(T magnitude, const Vector2<T>& vector) {
    return Vector2<T>{vector.x * magnitude, vector.y * magnitude};
}

template <typename T> inline bool operator==(const Vector2<T>& l, const Vector2<T>& r) {
    return l.x == r.x && l.y == r.y;
}

template <typename T> inline bool operator!=(const Vector2<T>& l, const Vector2<T>& r) {
    return l.x != r.x || l.y != r.y;
}

template <typename T> inline Vector2<T> operator+(const Vector2<T>& l, const Vector2<T>& r) {
    return {l.x + r.x, l.y + r.y};
}

template <typename T> inline Vector2<T>& operator+=(Vector2<T>& l, const Vector2<T>& r) {
    l.x += r.x;
    l.y += r.y;

    return l;
}

template <typename T> inline Vector2<T> operator-(const Vector2<T>& l, const Vector2<T>& r) {
    return {l.x - r.x, l.y - r.y};
}

template <typename T> inline Vector2<T>& operator-=(Vector2<T>& l, const Vector2<T>& r) {
    l.x -= r.x;
    l.y -= r.y;

    return l;
}

template <typename T> struct Vector3 {
    T x;
    T y;
    T z;
};

template <typename T> inline bool operator==(const Vector3<T>& l, const Vector3<T>& r) {
    return l.x == r.x && l.y == r.y && l.z == r.z;
}

template <typename T> inline bool operator!=(const Vector3<T>& l, const Vector3<T>& r) {
    return l.x != r.x || l.y != r.y || l.z != r.z;
}

template <typename T> struct Vector4 {
    T x;
    T y;
    T z;
    T w;
};

template<typename T, typename F>
constexpr Vector2<T> vector_static_cast(const Vector2<F>& from){
    return Vector2<T>{static_cast<T>(from.x), static_cast<T>(from.y)};
}

template<typename T, typename F>
constexpr Vector4<T> vector_static_cast(const Vector4<F>& from){
    return Vector4<T>{static_cast<T>(from.x), static_cast<T>(from.y), static_cast<T>(from.z),
        static_cast<T>(from.w)};
}

using Vector2f = Vector2<float>;
using Vector2i = Vector2<int>;
using Vector2u = Vector2<unsigned int>;
using Vector3f = Vector3<float>;
using Vector3i = Vector3<int>;
using Vector3u = Vector3<unsigned int>;
using Vector4f = Vector4<float>;
using Vector4i = Vector4<int>;
using Vector4u = Vector4<unsigned int>;

}

template<typename T>
struct std::formatter<reimu::Vector2<T>> {
    constexpr auto parse(std::format_parse_context &ctx) {
        return ctx.begin();
    }

    template<typename FmtCtx>
    auto format(const reimu::Vector2<T> &v, FmtCtx &ctx) const {
        return std::format_to(ctx.out(), "({}, {})", v.x, v.y);
    }
};

template<typename T>
struct std::formatter<reimu::Vector4<T>> {
    constexpr auto parse(std::format_parse_context &ctx) {
        return ctx.begin();
    }

    template<typename FmtCtx>
    auto format(const reimu::Vector4<T> &v, FmtCtx &ctx) const {
        return std::format_to(ctx.out(), "({}, {}, {}, {})", v.x, v.y, v.z, v.w);
    }
};
