#include <reimu/core/event.h>

#include <map>
#include <memory>

#include <assert.h>
#include <windows.h>

#define WIN32_MESSAGE_LOOP ((HANDLE)-1)

namespace reimu {

class WindowsEventLoop : public EventLoop {
public:
    WindowsEventLoop() {
        m_event_handle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        m_handles.push_back(m_event_handle);
    }

    ~WindowsEventLoop() {
        CloseHandle(m_event_handle);
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

        auto n_count = m_handles.size();
        while (!m_has_ended 
            && (result = MsgWaitForMultipleObjects(n_count, m_handles.data(), false, INFINITE, QS_ALLINPUT))
                != WAIT_FAILED) {
            if (result == WAIT_OBJECT_0 + n_count) {
                // We have a message in the win32 message loop
                while (PeekMessage(&win_msg, NULL, 0, 0, PM_REMOVE) > 0) {
                    TranslateMessage(&win_msg);
                    DispatchMessage(&win_msg);
                }
       
                // We have a special callback id for the win32 message loop
                auto cb = m_callbacks.find(WIN32_MESSAGE_LOOP);
                if (cb != m_callbacks.end()) {
                    (*cb->second)();
                }

            } else if (m_handles[result - WAIT_OBJECT_0] == m_event_handle) {
                // We have an event in our event queue
                std::queue<StringID> events;

                {
                    std::lock_guard<std::mutex> lock(m_event_queue_mutex);
                    std::swap(events, m_event_queue);
                }

                while (!events.empty()) {
                    auto event = events.front();
                    events.pop();

                    EventDispatcher::dispatch_event(event);
                }
            } else {
                // We have an event
                auto cb = m_callbacks.find(m_handles[result - WAIT_OBJECT_0]);
                if (cb != m_callbacks.end()) {
                    (*cb->second)();
                }
            }

            n_count = m_handles.size();
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

    void wake_up_loop() override {
        SetEvent(m_event_handle);
    }

    HANDLE m_event_handle;

    bool m_has_ended = false;
    std::vector<HANDLE> m_handles;
    std::map<HANDLE, std::unique_ptr<EventCallback>> m_callbacks;
};

Result<EventLoop *, OSError> EventLoop::create() {
    return OK(new WindowsEventLoop());
}

}
