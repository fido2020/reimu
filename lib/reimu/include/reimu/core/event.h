#pragma once

#include <reimu/os/error.h>
#include <reimu/os/handle.h>
#include <reimu/core/result.h>
#include <reimu/core/string_id.h>
#include <reimu/core/result.h>
#include <reimu/os/error.h>

#include <functional>
#include <queue>
#include <map>

namespace reimu {

using EventCallback = std::function<void()>;

class EventDispatcher {
public:
    void bind_event_callback(StringID event_id, EventCallback callback);
    void dispatch_event(StringID event_name);

private:
    std::map<StringID, EventCallback> m_event_callbacks;
};

class EventLoop {
public:
    static Result<EventLoop *, OSError> create();

    /**
     * @brief Watch an OS handle for events, calling callback when they occur.
    */
    virtual Result<void, OSError> watch_os_handle(os_handle_t fd, EventCallback callback) = 0;

    /**
     * @brief Stop watching an OS handle for events.
    */
    virtual void unwatch_os_handle(os_handle_t fd) = 0;

    /**
     * @brief Indefinitely polls and dispatches events.
    */
    virtual void run() = 0;

    /**
     * @brief Ends the event loop
     */
    virtual void end() = 0;

    virtual void *add_timer(long interval, EventCallback callback, bool oneshot) = 0;

    void remove_timer(void *timer) {
        auto *event = (Timer *)timer;

        event->callback = nullptr;
    }

protected:
    struct Timer {
        bool oneshot;

        long time_due;
        long interval;

        std::function<void()> callback;

        inline bool operator>(const Timer &other) const {
            return time_due > other.time_due;
        }
    };

    std::vector<Timer> m_timers;
    std::priority_queue<Timer, std::vector<Timer>, std::greater<Timer>> m_timer_queue;
};

}
