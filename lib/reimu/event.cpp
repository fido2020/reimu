#include <reimu/core/event.h>

namespace reimu {

void EventDispatcher::bind_event_callback(StringID event_id, EventCallback callback) {
    m_event_callbacks[event_id] = std::move(callback);
}

void EventDispatcher::bind_to_event(StringID event_id, EventDispatcher &other, StringID sub_event) {
    other.bind_event_callback(sub_event, [this, event_id]() {
        this->dispatch_event(event_id);
    });
}

void EventDispatcher::dispatch_event(StringID event_name) {
    auto it = m_event_callbacks.find(event_name);

    if (it != m_event_callbacks.end()) {
        it->second();
    }
}

void EventLoop::dispatch_event(StringID event_name) {
    std::lock_guard<std::mutex> lock(m_event_queue_mutex);
    m_event_queue.push(event_name);
    wake_up_loop();
}

}
