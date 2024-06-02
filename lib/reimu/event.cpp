#include <reimu/core/event.h>

namespace reimu {

void EventDispatcher::bind_event_callback(StringID event_id, EventCallback callback) {
    m_event_callbacks[event_id] = std::move(callback);
}

void EventDispatcher::dispatch_event(StringID event_name) {
    auto it = m_event_callbacks.find(event_name);

    if (it != m_event_callbacks.end()) {
        it->second();
    }
}

}
