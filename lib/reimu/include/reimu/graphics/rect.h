#pragma once

#include <reimu/graphics/vector.h>

namespace reimu {

template <typename T>
struct Rect : public Vector4<T> {
    static Rect from_size(const Vector2<T> &top_left, const Vector2<T> &size) {
        return { top_left.x, top_left.y, top_left.x + size.x, top_left.y + size.y };
    }

    inline T width() const {
        return this->z - this->x;
    }

    inline T height() const {
        return this->w - this->y;
    }

    inline Vector2<T> top_left() const {
        return { this->x, this->y };
    }

    inline Vector2<T> top_right() const {
        return { this->z, this->y };
    }

    inline Vector2<T> bottom_right() const {
        return { this->z, this->w };
    }

    inline Vector2<T> bottom_left() const {
        return { this->x, this->w };
    }

    inline Vector2<T> size() const {
        return { width(), height() };
    }

    inline Rect<T> intersect(const Rect<T> &other) const {
        T x1 = std::max(this->x, other.x);
        T y1 = std::max(this->y, other.y);
        T x2 = std::min(this->z, other.z);
        T y2 = std::min(this->w, other.w);

        return { x1, y1, x2, y2 };
    }
};

using Rectf = Rect<float>;
using Recti = Rect<int>;

}
