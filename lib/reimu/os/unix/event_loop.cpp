#include <reimu/core/event.h>

#include <map>
#include <memory>

#include <sys/epoll.h>

#define EPOLL_MAX_EVENTS 64

namespace reimu {

class UNIXEventLoop : public EventLoop {
public:
    UNIXEventLoop(int epoll_fd)
        : m_epoll_fd(epoll_fd) {}

    ~UNIXEventLoop() {
        close(m_epoll_fd);
    }

    Result<void, OSError> watch_os_handle(int fd, EventCallback cb) override {
        auto callback_ptr = std::make_unique<EventCallback>(std::move(cb));
        
        // Give a pointer to our callback to epoll
        epoll_event event = {};
        event.events = EPOLLIN;
        event.data.ptr = callback_ptr.get();

        if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &event) < 0) {
            return ERR(errno);
        }

        m_callbacks.emplace(fd, std::move(callback_ptr));

        return OK();
    }

    void unwatch_os_handle(int fd) override {
        if (epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, nullptr) < 0) {
            logger::fatal("Failed to remove epoll event: {}", strerror(errno));
        }

        m_callbacks.erase(fd);
    }

    void *add_timer(long interval, EventCallback callback, bool oneshot) override {
        return nullptr;
    }

    void run() override {
        epoll_event events[EPOLL_MAX_EVENTS];

        int num_events;
        while (!m_has_ended
            && (num_events = epoll_wait(m_epoll_fd, events, EPOLL_MAX_EVENTS, -1)) >= 0) {
            // Grab each event and call the callback
            for (int i = 0; i < num_events; i++) {
                auto *callback = (EventCallback *)events[i].data.ptr;
                (*callback)();
            }

            if (m_callbacks.empty() && m_timer_queue.empty()) {
                m_has_ended = true;
            }
        }
    }

    void end() override {
        m_has_ended = true;
    }

    bool m_has_ended = false;
    int m_epoll_fd;
    std::map<int, std::unique_ptr<EventCallback>> m_callbacks;
};

Result<EventLoop *, OSError> EventLoop::create() {
    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        return ERR(errno);
    }

    return new UNIXEventLoop {
        epoll_fd
    };
}

}
