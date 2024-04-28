#pragma once

template<typename T>
class App {
public:
    void run();

    void process_event() {
        if(T::process_event()) {
            return;   
        }
    }
};
