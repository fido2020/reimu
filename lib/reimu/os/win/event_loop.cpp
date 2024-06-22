#include <reimu/core/event.h>

#include <map>
#include <memory>

#include <windows.h>

namespace reimu {

class WindowsEventLoop : public EventLoop {
public:
    WindowsEventLoop() {}

    ~WindowsEventLoop() {

    }

    Result<void, OSError> watch_os_handle(int fd, EventCallback cb) override {
        auto callback_ptr = std::make_unique<EventCallback>(std::move(cb));

        m_callbacks.emplace(fd, std::move(callback_ptr));

        return OK();
    }

    void unwatch_os_handle(int fd) override {
        m_callbacks.erase(fd);
    }

    void *add_timer(long interval, EventCallback callback, bool oneshot) override {
        return nullptr;
    }

    void run() override {
        MSG win_msg;
        while (!m_has_ended && GetMessage(&win_msg, nullptr, 0, 0)) {
            TranslateMessage(&win_msg);
            DispatchMessage(&win_msg);
        }
    }

    void end() override {
        m_has_ended = true;
    }

    bool m_has_ended = false;
    std::map<int, std::unique_ptr<EventCallback>> m_callbacks;
};

Result<EventLoop *, OSError> EventLoop::create() {
    return OK(new WindowsEventLoop());
}

}
