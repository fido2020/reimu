#pragma once

#include <reimu/core/result.h>
#include <memory>
#include <mutex>

namespace reimu::graphics {

namespace detail {
    struct FontData;
}

class Font
{
public:
    static Result<Font *, ReimuError> create();

    ~Font();

    void *get_handle();

    std::mutex m_lock;
    
private:
    Font();

    std::unique_ptr<detail::FontData> m_data;
};

}
