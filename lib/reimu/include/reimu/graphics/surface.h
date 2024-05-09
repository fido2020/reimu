#pragma once

#include <reimu/graphics/rect.h>

#include <memory>

namespace reimu::graphics {

class Surface {
public:
    virtual void copy(const std::shared_ptr<Surface> &src, const Rectf &src_rect, const Rectf &dst_rect) = 0;
};

}
