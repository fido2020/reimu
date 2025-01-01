#pragma once

#include <reimu/core/file.h>
#include <reimu/core/result.h>
#include <reimu/core/resource.h>
#include <memory>
#include <mutex>

namespace reimu::graphics {

namespace detail {
    struct FontData;
}

class Font : public Resource
{
public:
    ~Font();

    static Result<Font *, ReimuError> create(File &file);

    StringID obj_type_id() const override;
    
    static consteval StringID type_id() {
        return "font"_hashid;
    }

    void *get_handle();

    std::mutex m_lock;
    
private:
    Font();

    std::unique_ptr<detail::FontData> m_data;
};

}
