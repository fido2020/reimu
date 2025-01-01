#pragma once

#include <reimu/core/optional.h>
#include <reimu/core/string_id.h>

#include <reimu/core/file.h>

#include <memory>

namespace reimu {

class Resource {
public:
    virtual StringID obj_type_id() const = 0;

    template<typename T>
    static Optional<std::shared_ptr<T>> as(std::shared_ptr<Resource> ptr) {
        if (T::type_id() == ptr->obj_type_id()) {
            return std::static_pointer_cast<T>(ptr);
        }

        return {};
    }
};

};