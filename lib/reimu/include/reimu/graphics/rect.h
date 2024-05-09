#pragma once

#include <reimu/graphics/vector.h>

namespace reimu {

template <typename T> struct Rect {
    Vector2<T> begin;
    Vector2<T> end;
};

using Rectf = Rect<float>;
using Recti = Rect<int>;

}
