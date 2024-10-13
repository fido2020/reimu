#pragma once

#include <reimu/graphics/vector.h>

namespace reimu::video {

class Window;

class Driver {
public:
    /**
     * @brief Create a window with the given size.
     * 
     * @param size 
     * @return Window* 
     */
    virtual Window *window_create(const Vector2i &size) = 0;
    
    /**
     * @brief Get the file descriptor for the window client.
     * 
     * @return int 
     */
    virtual int get_window_client_handle() = 0;

    /**
     * @brief Process any events from the window server.
     * 
     * May block if there is nothing to read
     */
    virtual void window_client_dispatch() = 0;

    virtual void finish() = 0;

    virtual Vector2u get_display_size() = 0;
};

}
