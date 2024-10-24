#include <reimu/core/event.h>

#include <map>
#include <memory>

#include <assert.h>
#include <windows.h>

#define WIN32_MESSAGE_LOOP ((HANDLE)-1)

namespace reimu {

class WindowsEventLoop : public EventLoop {
public:
    WindowsEventLoop() {}

    ~WindowsEventLoop() {

    }

    Result<void, OSError> watch_os_handle(os_handle_t fd, EventCallback cb) override {
        auto callback_ptr = std::make_unique<EventCallback>(std::move(cb));

        auto it = m_callbacks.find(fd);
        if (it == m_callbacks.end() && fd != WIN32_MESSAGE_LOOP) {
            m_handles.push_back((HANDLE)fd);
        }

        logger::debug("Watching handle {}", fd);

        m_callbacks.emplace(fd, std::move(callback_ptr));

        return OK();
    }

    void unwatch_os_handle(os_handle_t fd) override {
        m_callbacks.erase(fd);
        m_handles.erase(std::remove(m_handles.begin(), m_handles.end(), fd), m_handles.end());
    }

    void *add_timer(long interval, EventCallback callback, bool oneshot) override {
        return nullptr;
    }

    void run() override {
        MSG win_msg;
        DWORD result;

        while (!m_has_ended 
            && (result = MsgWaitForMultipleObjects(m_handles.size(), m_handles.data(), false, INFINITE, QS_ALLINPUT))
                != WAIT_FAILED) {
            if (result == WAIT_OBJECT_0 + m_handles.size()) {
                // We have a message in the win32 message loop
                
                // We have a special callback id for the win32 message loop
                auto cb = m_callbacks.find(WIN32_MESSAGE_LOOP);
                if (cb != m_callbacks.end()) {
                    (*cb->second)();
                }
            } else {
                // We have an event
                auto cb = m_callbacks.find(m_handles[result - WAIT_OBJECT_0]);
                if (cb != m_callbacks.end()) {
                    (*cb->second)();
                }
            }
        }

        if (result == WAIT_FAILED) {
            // Print handles:
            for (auto handle : m_handles) {
                logger::debug("Handle: {}", handle);
            }

            logger::fatal("MsgWaitForMultipleObjects failed: {}", GetLastError());
        }
    }

    void end() override {
        m_has_ended = true;
    }

    bool m_has_ended = false;
    std::vector<HANDLE> m_handles;
    std::map<HANDLE, std::unique_ptr<EventCallback>> m_callbacks;
};

Result<EventLoop *, OSError> EventLoop::create() {
    return OK(new WindowsEventLoop());
}

}
